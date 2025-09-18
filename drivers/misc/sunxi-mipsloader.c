// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner MIPS Co-processor Loader Driver
 * 
 * Copyright (C) 2025 HY300 Linux Porting Project
 * 
 * This driver provides support for the MIPS co-processor found in
 * Allwinner H713 SoC, specifically for the HY300 projector hardware.
 * The MIPS co-processor handles display engine control, panel timing,
 * and projector-specific hardware management.
 * 
 * Based on reverse engineering of factory Android implementation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/firmware.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/of_reserved_mem.h>
#include <linux/crc32.h>
#include <linux/crypto.h>
#include <linux/delay.h>

/* MIPS Memory Layout (from factory analysis) */
#define MIPS_BOOT_CODE_ADDR     0x4b100000  /* 4KB - MIPS reset vector */
#define MIPS_FIRMWARE_ADDR      0x4b101000  /* 12MB - Main MIPS firmware */
#define MIPS_DEBUG_ADDR         0x4bd01000  /* 1MB - Debug buffer */
#define MIPS_CONFIG_ADDR        0x4be01000  /* 256KB - Configuration */
#define MIPS_DATABASE_ADDR      0x4be41000  /* 1MB - TSE database */
#define MIPS_FRAMEBUFFER_ADDR   0x4bf41000  /* 26MB - Frame buffer */
#define MIPS_TOTAL_SIZE         0x2800000   /* 40MB total */

/* Register Interface (from factory analysis) */
#define MIPS_REG_CMD            0x00    /* Command register */
#define MIPS_REG_STATUS         0x04    /* Status register */
#define MIPS_REG_DATA           0x08    /* Data register */
#define MIPS_REG_CONTROL        0x0c    /* Control register */

/* Panel Timing Configuration (from factory) */
#define PANEL_HTOTAL_TYP        2200
#define PANEL_HTOTAL_MIN        2095
#define PANEL_HTOTAL_MAX        2809
#define PANEL_VTOTAL_TYP        1125
#define PANEL_VTOTAL_MIN        1107
#define PANEL_VTOTAL_MAX        1440
#define PANEL_PCLK_TYP          148500000  /* 148.5MHz */
#define PANEL_PCLK_MIN          130000000
#define PANEL_PCLK_MAX          164000000

/* Device node and class information */
#define MIPSLOADER_DEVICE_NAME  "mipsloader"
#define MIPSLOADER_CLASS_NAME   "mips"

/* IOCTL commands */
#define MIPSLOADER_IOC_MAGIC    'M'
#define MIPSLOADER_IOC_LOAD_FW  _IOW(MIPSLOADER_IOC_MAGIC, 1, char*)
#define MIPSLOADER_IOC_RESTART  _IO(MIPSLOADER_IOC_MAGIC, 2)
#define MIPSLOADER_IOC_POWERDOWN _IO(MIPSLOADER_IOC_MAGIC, 3)
#define MIPSLOADER_IOC_GET_STATUS _IOR(MIPSLOADER_IOC_MAGIC, 4, int)

/**
 * struct mipsloader_device - MIPS loader device structure
 * @pdev: Platform device
 * @reg_base: Register base address (ioremapped)
 * @mem_base: MIPS memory base address (ioremapped) 
 * @mem_size: Size of MIPS memory region
 * @cdev: Character device
 * @device: Device structure
 * @class: Device class
 * @major: Major device number
 * @firmware_loaded: Flag indicating if firmware is loaded
 * @lock: Mutex for device access synchronization
 */
struct mipsloader_device {
	struct platform_device *pdev;
	void __iomem *reg_base;
	void __iomem *mem_base;
	size_t mem_size;
	struct cdev cdev;
	struct device *device;
	struct class *class;
	int major;
	bool firmware_loaded;
	struct mutex lock;
};

static struct mipsloader_device *mipsloader_dev;

/**
 * mipsloader_reg_read - Read from MIPS control register
 * @offset: Register offset
 * 
 * Returns register value
 */
static u32 mipsloader_reg_read(u32 offset)
{
	if (!mipsloader_dev || !mipsloader_dev->reg_base)
		return 0;
	
	return readl(mipsloader_dev->reg_base + offset);
}

/**
 * mipsloader_reg_write - Write to MIPS control register
 * @offset: Register offset
 * @value: Value to write
 */
static void mipsloader_reg_write(u32 offset, u32 value)
{
	if (!mipsloader_dev || !mipsloader_dev->reg_base)
		return;
	
	writel(value, mipsloader_dev->reg_base + offset);
}

/**
 * mipsloader_load_firmware - Load MIPS firmware from file
 * @firmware_path: Path to firmware file
 * 
 * Returns 0 on success, negative error code on failure
 */
static int mipsloader_load_firmware(const char *firmware_path)
{
	const struct firmware *fw;
	int ret;
	u32 calculated_crc;
	
	if (!mipsloader_dev || !mipsloader_dev->mem_base) {
		pr_err("mipsloader: Device not initialized\n");
		return -ENODEV;
	}
	
	/* Request firmware from userspace */
	ret = request_firmware(&fw, firmware_path, &mipsloader_dev->pdev->dev);
	if (ret) {
		dev_err(&mipsloader_dev->pdev->dev, 
			"Failed to load firmware %s: %d\n", firmware_path, ret);
		return ret;
	}
	
	/* Validate firmware size */
	if (fw->size > (MIPS_FIRMWARE_ADDR - MIPS_BOOT_CODE_ADDR + 0xc00000)) {
		dev_err(&mipsloader_dev->pdev->dev,
			"Firmware too large: %zu bytes\n", fw->size);
		ret = -E2BIG;
		goto release_fw;
	}
	
	/* Calculate CRC32 for validation */
	calculated_crc = crc32(0, fw->data, fw->size);
	dev_info(&mipsloader_dev->pdev->dev,
		"Loading firmware: %zu bytes, CRC32: 0x%08x\n", 
		fw->size, calculated_crc);
	
	/* Copy firmware to MIPS memory region */
	memcpy_toio(mipsloader_dev->mem_base + (MIPS_FIRMWARE_ADDR - MIPS_BOOT_CODE_ADDR),
		    fw->data, fw->size);
	
	/* Ensure write completion */
	wmb();
	
	mipsloader_dev->firmware_loaded = true;
	dev_info(&mipsloader_dev->pdev->dev, "Firmware loaded successfully\n");
	
	ret = 0;

release_fw:
	release_firmware(fw);
	return ret;
}

/**
 * mipsloader_restart - Restart MIPS co-processor
 * 
 * Returns 0 on success, negative error code on failure
 */
static int mipsloader_restart(void)
{
	if (!mipsloader_dev->firmware_loaded) {
		dev_err(&mipsloader_dev->pdev->dev, 
			"Cannot restart: firmware not loaded\n");
		return -ENOENT;
	}
	
	/* Reset MIPS processor */
	mipsloader_reg_write(MIPS_REG_CONTROL, 0x01);  /* Reset */
	msleep(10);  /* Wait for reset */
	mipsloader_reg_write(MIPS_REG_CONTROL, 0x00);  /* Release reset */
	
	dev_info(&mipsloader_dev->pdev->dev, "MIPS co-processor restarted\n");
	return 0;
}

/**
 * mipsloader_powerdown - Power down MIPS co-processor
 * 
 * Returns 0 on success, negative error code on failure
 */
static int mipsloader_powerdown(void)
{
	/* Send powerdown command */
	mipsloader_reg_write(MIPS_REG_CMD, 0x02);  /* Powerdown command */
	
	/* Wait for acknowledgment */
	msleep(100);
	
	dev_info(&mipsloader_dev->pdev->dev, "MIPS co-processor powered down\n");
	return 0;
}

/**
 * mipsloader_open - Open device node
 */
static int mipsloader_open(struct inode *inode, struct file *file)
{
	if (!mipsloader_dev)
		return -ENODEV;
	
	file->private_data = mipsloader_dev;
	return 0;
}

/**
 * mipsloader_release - Close device node
 */
static int mipsloader_release(struct inode *inode, struct file *file)
{
	return 0;
}

/**
 * mipsloader_ioctl - Handle IOCTL commands
 */
static long mipsloader_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mipsloader_device *dev = file->private_data;
	int ret = 0;
	char firmware_path[256];
	u32 status;
	
	if (!dev)
		return -ENODEV;
	
	mutex_lock(&dev->lock);
	
	switch (cmd) {
	case MIPSLOADER_IOC_LOAD_FW:
		if (copy_from_user(firmware_path, (char __user *)arg, sizeof(firmware_path))) {
			ret = -EFAULT;
			break;
		}
		firmware_path[sizeof(firmware_path) - 1] = '\0';
		ret = mipsloader_load_firmware(firmware_path);
		break;
		
	case MIPSLOADER_IOC_RESTART:
		ret = mipsloader_restart();
		break;
		
	case MIPSLOADER_IOC_POWERDOWN:
		ret = mipsloader_powerdown();
		break;
		
	case MIPSLOADER_IOC_GET_STATUS:
		status = mipsloader_reg_read(MIPS_REG_STATUS);
		if (copy_to_user((int __user *)arg, &status, sizeof(status)))
			ret = -EFAULT;
		break;
		
	default:
		ret = -ENOTTY;
		break;
	}
	
	mutex_unlock(&dev->lock);
	return ret;
}

static const struct file_operations mipsloader_fops = {
	.owner = THIS_MODULE,
	.open = mipsloader_open,
	.release = mipsloader_release,
	.unlocked_ioctl = mipsloader_ioctl,
	.compat_ioctl = mipsloader_ioctl,
};

/**
 * mipsloader_probe - Platform device probe
 */
static int mipsloader_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	dev_t devt;
	
	dev_info(&pdev->dev, "Probing MIPS loader device\n");
	
	/* Allocate device structure */
	mipsloader_dev = devm_kzalloc(&pdev->dev, sizeof(*mipsloader_dev), GFP_KERNEL);
	if (!mipsloader_dev)
		return -ENOMEM;
	
	mipsloader_dev->pdev = pdev;
	mutex_init(&mipsloader_dev->lock);
	platform_set_drvdata(pdev, mipsloader_dev);
	
	/* Map register region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "No register resource found\n");
		return -ENOENT;
	}
	
	mipsloader_dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(mipsloader_dev->reg_base)) {
		dev_err(&pdev->dev, "Failed to map registers\n");
		return PTR_ERR(mipsloader_dev->reg_base);
	}
	
	dev_info(&pdev->dev, "Register base mapped to %p\n", mipsloader_dev->reg_base);
	
	/* Map MIPS memory region */
	mipsloader_dev->mem_size = MIPS_TOTAL_SIZE;
	mipsloader_dev->mem_base = devm_ioremap(&pdev->dev, MIPS_BOOT_CODE_ADDR, 
						mipsloader_dev->mem_size);
	if (!mipsloader_dev->mem_base) {
		dev_err(&pdev->dev, "Failed to map MIPS memory region\n");
		return -ENOMEM;
	}
	
	dev_info(&pdev->dev, "MIPS memory region mapped: %zu bytes at %p\n",
		 mipsloader_dev->mem_size, mipsloader_dev->mem_base);
	
	/* Allocate character device number */
	ret = alloc_chrdev_region(&devt, 0, 1, MIPSLOADER_DEVICE_NAME);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate device number: %d\n", ret);
		return ret;
	}
	
	mipsloader_dev->major = MAJOR(devt);
	
	/* Initialize character device */
	cdev_init(&mipsloader_dev->cdev, &mipsloader_fops);
	mipsloader_dev->cdev.owner = THIS_MODULE;
	
	ret = cdev_add(&mipsloader_dev->cdev, devt, 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add character device: %d\n", ret);
		goto unregister_chrdev;
	}
	
	/* Create device class */
	mipsloader_dev->class = class_create(MIPSLOADER_CLASS_NAME);
	if (IS_ERR(mipsloader_dev->class)) {
		ret = PTR_ERR(mipsloader_dev->class);
		dev_err(&pdev->dev, "Failed to create device class: %d\n", ret);
		goto del_cdev;
	}
	
	/* Create device node */
	mipsloader_dev->device = device_create(mipsloader_dev->class, &pdev->dev,
					       devt, NULL, MIPSLOADER_DEVICE_NAME);
	if (IS_ERR(mipsloader_dev->device)) {
		ret = PTR_ERR(mipsloader_dev->device);
		dev_err(&pdev->dev, "Failed to create device: %d\n", ret);
		goto destroy_class;
	}
	
	dev_info(&pdev->dev, "MIPS loader driver initialized successfully\n");
	dev_info(&pdev->dev, "Device node: /dev/%s\n", MIPSLOADER_DEVICE_NAME);
	
	return 0;

destroy_class:
	class_destroy(mipsloader_dev->class);
del_cdev:
	cdev_del(&mipsloader_dev->cdev);
unregister_chrdev:
	unregister_chrdev_region(devt, 1);
	
	return ret;
}

/**
 * mipsloader_remove - Platform device remove
 */
static void mipsloader_remove(struct platform_device *pdev)
{
	struct mipsloader_device *dev = platform_get_drvdata(pdev);
	dev_t devt = MKDEV(dev->major, 0);
	
	dev_info(&pdev->dev, "Removing MIPS loader device\n");
	
	/* Cleanup device */
	device_destroy(dev->class, devt);
	class_destroy(dev->class);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(devt, 1);
	
	/* Power down MIPS if running */
	if (dev->firmware_loaded) {
		mipsloader_powerdown();
	}
	
	mipsloader_dev = NULL;
	
	dev_info(&pdev->dev, "MIPS loader driver removed\n");
}

static const struct of_device_id mipsloader_of_match[] = {
	{ .compatible = "allwinner,sunxi-mipsloader" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mipsloader_of_match);

static struct platform_driver mipsloader_driver = {
	.probe = mipsloader_probe,
	.remove = mipsloader_remove,
	.driver = {
		.name = "sunxi-mipsloader",
		.of_match_table = mipsloader_of_match,
	},
};

module_platform_driver(mipsloader_driver);

MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_DESCRIPTION("Allwinner MIPS Co-processor Loader Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:sunxi-mipsloader");