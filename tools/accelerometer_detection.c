/*
 * HY300 Accelerometer Hardware Detection Utility
 * 
 * This utility scans I2C bus 1 for accelerometer devices and determines
 * which accelerometer model is present on the HY300 projector hardware.
 * 
 * Expected devices:
 * - STK8BA58 at I2C address 0x18
 * - KXTTJ3 at I2C address 0x0e
 * 
 * Usage: ./accelerometer_detection [--verbose] [--bus=1]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <linux/i2c.h>

#define STK8BA58_ADDR    0x18
#define KXTTJ3_ADDR      0x0e
#define DEFAULT_I2C_BUS  1

/* STK8BA58 register definitions */
#define STK8BA58_REG_CHIPID     0x00
#define STK8BA58_CHIPID_VAL     0x87

/* KXTTJ3 register definitions */
#define KXTTJ3_REG_WHO_AM_I     0x0F
#define KXTTJ3_WHO_AM_I_VAL     0x35

struct accelerometer_info {
    const char *name;
    uint8_t i2c_addr;
    uint8_t id_reg;
    uint8_t expected_id;
    const char *compatible;
    const char *driver_name;
};

static struct accelerometer_info accel_devices[] = {
    {
        .name = "STK8BA58",
        .i2c_addr = STK8BA58_ADDR,
        .id_reg = STK8BA58_REG_CHIPID,
        .expected_id = STK8BA58_CHIPID_VAL,
        .compatible = "sensortek,stk8ba58",
        .driver_name = "stk8ba58"
    },
    {
        .name = "KXTTJ3",
        .i2c_addr = KXTTJ3_ADDR,
        .id_reg = KXTTJ3_REG_WHO_AM_I,
        .expected_id = KXTTJ3_WHO_AM_I_VAL,
        .compatible = "kionix,kxtj3-1057",
        .driver_name = "kxtj3"
    }
};

static int verbose = 0;

/*
 * Read a single byte from I2C device register
 */
static int i2c_read_reg(int fd, uint8_t reg, uint8_t *value)
{
    struct i2c_smbus_ioctl_data data;
    union i2c_smbus_data smbus_data;

    data.read_write = I2C_SMBUS_READ;
    data.command = reg;
    data.size = I2C_SMBUS_BYTE_DATA;
    data.data = &smbus_data;

    if (ioctl(fd, I2C_SMBUS, &data) < 0) {
        return -1;
    }

    *value = smbus_data.byte;
    
    /* Add small delay for device response */
    usleep(1000); /* 1ms delay */
    return 0;
}

/*
//
 * Validate I2C bus availability and functionality
 */
/*
 * Check if required kernel modules are loaded
 */
static int check_kernel_modules(void)
{
    FILE *fp;
    char line[256];
    int i2c_dev_found = 0;
    int i2c_core_found = 0;
    
    fp = fopen("/proc/modules", "r");
    if (!fp) {
        if (verbose) {
            printf("Warning: Could not check kernel modules: %s\n", strerror(errno));
        }
        return 0; /* Assume OK if we cannot check */
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "i2c_dev ", 8) == 0) {
            i2c_dev_found = 1;
        } else if (strncmp(line, "i2c_core ", 9) == 0) {
            i2c_core_found = 1;
        }
    }
    
    fclose(fp);
    
    if (verbose) {
        printf("Kernel module check:\n");
        printf("  i2c_dev: %s\n", i2c_dev_found ? "loaded" : "not loaded");
        printf("  i2c_core: %s\n", i2c_core_found ? "loaded" : "not loaded");
    }
    
    if (!i2c_dev_found) {
        printf("Warning: i2c_dev kernel module not loaded\n");
        printf("Try: modprobe i2c_dev\n");
        return -1;
    }
    
    return 0;
}


static int validate_i2c_bus(int bus)
{
    char i2c_dev[32];
    int fd;
    unsigned long funcs;
    
    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", bus);
    
    /* Check if I2C device node exists */
    fd = open(i2c_dev, O_RDWR);
    if (fd < 0) {
        if (verbose) {
            printf("I2C bus %d not available: %s\n", bus, strerror(errno));
        }
        return -1;
    }
    
    /* Check I2C adapter functionality */
    if (ioctl(fd, I2C_FUNCS, &funcs) < 0) {
        if (verbose) {
            printf("Failed to get I2C bus %d functionality: %s\n", bus, strerror(errno));
        }
        close(fd);
        return -1;
    }
    
    /* Verify required I2C functions are supported */
    if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
        if (verbose) {
            printf("I2C bus %d does not support SMBUS byte data read\n", bus);
        }
        close(fd);
        return -1;
    }
    
    if (!(funcs & I2C_FUNC_I2C)) {
        if (verbose) {
            printf("I2C bus %d does not support plain I2C transactions\n", bus);
        }
        close(fd);
        return -1;
    }
    
    close(fd);
    
    if (verbose) {
        printf("I2C bus %d validated successfully\n", bus);
        printf("  Supported functions: 0x%08lx\n", funcs);
    }
    
    return 0;
}

/*
 * Check if device is present at given I2C address
 */
static int i2c_probe_device(int bus, uint8_t addr)
{
    char i2c_dev[32];
    int fd;
    int result = 0;

    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", bus);
    
    fd = open(i2c_dev, O_RDWR);
    if (fd < 0) {
        if (verbose) {
            printf("Failed to open %s: %s\n", i2c_dev, strerror(errno));
        }
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        if (verbose) {
            printf("Failed to set I2C slave address 0x%02x: %s\n", addr, strerror(errno));
        }
        close(fd);
        return -1;
    }

    /* Try to read from device - any register access will indicate presence */
    uint8_t dummy;
    if (i2c_read_reg(fd, 0x00, &dummy) == 0) {
        result = 1; /* Device responds */
    }

    close(fd);
    return result;
}

/*
 * Detect specific accelerometer device
 */
static int detect_accelerometer(int bus, struct accelerometer_info *accel)
{
    char i2c_dev[32];
    int fd;
    uint8_t chip_id;
    int result = 0;

    if (verbose) {
        printf("Checking for %s at I2C address 0x%02x...\n", accel->name, accel->i2c_addr);
    }

    /* First check if device is present */
    if (i2c_probe_device(bus, accel->i2c_addr) <= 0) {
        if (verbose) {
            printf("  No device found at address 0x%02x\n", accel->i2c_addr);
        }
        return 0;
    }

    snprintf(i2c_dev, sizeof(i2c_dev), "/dev/i2c-%d", bus);
    
    fd = open(i2c_dev, O_RDWR);
    if (fd < 0) {
        printf("Failed to open %s: %s\n", i2c_dev, strerror(errno));
        return -1;
    }

    if (ioctl(fd, I2C_SLAVE, accel->i2c_addr) < 0) {
        printf("Failed to set I2C slave address 0x%02x: %s\n", accel->i2c_addr, strerror(errno));
        close(fd);
        return -1;
    }

    /* Read device ID register */
    if (i2c_read_reg(fd, accel->id_reg, &chip_id) == 0) {
        if (verbose) {
            printf("  Device at 0x%02x, ID register 0x%02x = 0x%02x (expected 0x%02x)\n",
                   accel->i2c_addr, accel->id_reg, chip_id, accel->expected_id);
        }
        
        if (chip_id == accel->expected_id) {
            printf("✓ Detected %s accelerometer at I2C address 0x%02x\n", accel->name, accel->i2c_addr);
            printf("  Compatible string: %s\n", accel->compatible);
            printf("  Driver name: %s\n", accel->driver_name);
            result = 1;
        } else {
            printf("✗ Device at 0x%02x has unexpected ID 0x%02x (not %s)\n", 
                   accel->i2c_addr, chip_id, accel->name);
        }
    } else {
        if (verbose) {
            printf("  Failed to read ID register 0x%02x from device at 0x%02x\n", 
                   accel->id_reg, accel->i2c_addr);
        }
    }

    close(fd);
    return result;
}

/*
 * Scan I2C bus for any devices (general scan)
 */
static void scan_i2c_bus(int bus)
{
    printf("\nScanning I2C bus %d for all devices:\n", bus);
    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    
    for (int i = 0; i < 128; i += 16) {
        printf("%02x: ", i);
        for (int j = 0; j < 16; j++) {
            int addr = i + j;
            if (addr < 0x03 || addr > 0x77) {
                printf("   ");
            } else {
                if (i2c_probe_device(bus, addr) > 0) {
                    printf("%02x ", addr);
                } else {
                    printf("-- ");
                }
            }
        }
        printf("\n");
    }
    printf("\n");
}

/*
 * Generate device tree overlay for detected accelerometer
 */
static void generate_device_tree_overlay(struct accelerometer_info *detected_accel)
{
    printf("\nSuggested device tree configuration:\n");
    printf("&i2c1 {\n");
    printf("    accelerometer: %s@%x {\n", detected_accel->driver_name, detected_accel->i2c_addr);
    printf("        compatible = \"%s\";\n", detected_accel->compatible);
    printf("        reg = <0x%02x>;\n", detected_accel->i2c_addr);
    printf("        interrupt-parent = <&pio>;\n");
    printf("        interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */\n");
    
    if (detected_accel->i2c_addr == STK8BA58_ADDR) {
        printf("        stk,direction = <2>;\n");
    }
    
    printf("        status = \"okay\";\n");
    printf("    };\n");
    printf("};\n");
}

/*
 * Write detection results to sysfs for kernel module
 */
static int write_detection_result(struct accelerometer_info *accel)
{
    FILE *fp;
    const char *sysfs_path = "/sys/class/hy300/accelerometer_type";
    
    fp = fopen(sysfs_path, "w");
    if (!fp) {
        /* Sysfs interface may not be available yet */
        if (verbose) {
            printf("Note: Could not write to %s (kernel module may not be loaded)\n", sysfs_path);
        }
        return -1;
    }
    
    fprintf(fp, "%s\n", accel->name);
    fclose(fp);
    
    if (verbose) {
        printf("Written accelerometer type '%s' to %s\n", accel->name, sysfs_path);
    }
    
    return 0;
}

static void usage(const char *prog_name)
{
    printf("Usage: %s [OPTIONS]\n", prog_name);
    printf("\n");
    printf("HY300 Accelerometer Hardware Detection Utility\n");
    printf("\n");
    printf("Options:\n");
    printf("  -v, --verbose          Enable verbose output\n");
    printf("  -b, --bus=NUM          I2C bus number (default: %d)\n", DEFAULT_I2C_BUS);
    printf("  -s, --scan             Perform full I2C bus scan\n");
    printf("  -h, --help             Show this help message\n");
    printf("\n");
    printf("This utility detects which accelerometer model is present on the HY300\n");
    printf("projector and provides the appropriate device tree configuration.\n");
}

int main(int argc, char *argv[])
{
    int bus = DEFAULT_I2C_BUS;
    int scan_mode = 0;
    int detected_count = 0;
    struct accelerometer_info *detected_accel = NULL;
    
    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"bus", required_argument, 0, 'b'},
        {"scan", no_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int c;
    while ((c = getopt_long(argc, argv, "vb:sh", long_options, NULL)) != -1) {
        switch (c) {
        case 'v':
            verbose = 1;
            break;
        case 'b':
            bus = atoi(optarg);
            break;
        case 's':
            scan_mode = 1;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }
    
    printf("HY300 Accelerometer Hardware Detection\n");
    
    /* Check kernel modules first */
    if (check_kernel_modules() < 0) {
        printf("\n⚠ Warning: Required kernel modules may not be loaded\n");
        if (!verbose) {
            printf("Use --verbose for details\n");
        }
    }

    printf("=====================================\n");
    
    if (scan_mode) {
        scan_i2c_bus(bus);
    }
    
    /* Validate I2C bus before proceeding */
    if (validate_i2c_bus(bus) < 0) {
        printf("\n✗ I2C bus %d validation failed\n", bus);
        printf("\nTroubleshooting:\n");
        printf("1. Check if I2C bus is enabled in device tree\n");
        printf("2. Verify I2C kernel module is loaded\n");
        printf("3. Check I2C bus permissions (run as root if needed)\n");
        printf("4. Try different I2C bus number with --bus option\n");
        return 1;
    }

    printf("Detecting accelerometer devices on I2C bus %d:\n\n", bus);
    
    /* Check each known accelerometer type */
    for (size_t i = 0; i < sizeof(accel_devices) / sizeof(accel_devices[0]); i++) {
        int result = detect_accelerometer(bus, &accel_devices[i]);
        if (result > 0) {
            detected_count++;
            detected_accel = &accel_devices[i];
        } else if (result < 0) {
            printf("Error checking %s\n", accel_devices[i].name);
        }
    }
    
    printf("\nDetection Summary:\n");
    printf("=================\n");
    
    if (detected_count == 0) {
        printf("✗ No accelerometer devices detected\n");
        printf("\nTroubleshooting:\n");
        printf("1. Check I2C bus wiring and pull-up resistors\n");
        printf("2. Verify I2C bus number (try --scan to see all devices)\n");
        printf("3. Check device tree I2C configuration\n");
        printf("4. Ensure accelerometer power supply is enabled\n");
        return 1;
    }
    
    if (detected_count > 1) {
        printf("⚠ Warning: Multiple accelerometers detected\n");
        printf("This may indicate a hardware configuration issue.\n");
    }
    
    if (detected_accel) {
        printf("✓ Primary accelerometer: %s at 0x%02x\n", 
               detected_accel->name, detected_accel->i2c_addr);
        
        /* Generate device tree configuration */
        generate_device_tree_overlay(detected_accel);
        
        /* Write result for kernel module if available */
        write_detection_result(detected_accel);
    }
    
    return 0;
}