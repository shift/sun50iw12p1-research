#!/bin/bash

# Build script to add Prometheus metrics to MIPS loader driver
set -e

SOURCE_FILE="drivers/misc/sunxi-mipsloader.c"
BACKUP_FILE="drivers/misc/sunxi-mipsloader.c.backup"
OUTPUT_FILE="drivers/misc/sunxi-mipsloader-with-metrics.c"

echo "Building MIPS loader driver with Prometheus metrics..."

# Start with the original header
head -70 "$BACKUP_FILE" > "$OUTPUT_FILE"

# Add metrics structure definition
cat >> "$OUTPUT_FILE" << 'STRUCT_EOF'

/**
 * struct mipsloader_metrics - Prometheus metrics tracking
 * @reg_access_count: Count of register accesses by type
 * @firmware_load_attempts: Total firmware load attempts
 * @firmware_load_success: Successful firmware loads
 * @firmware_load_failures: Failed firmware loads
 * @memory_regions_used: Active memory regions
 * @communication_errors: Communication error counts
 * @last_firmware_size: Size of last loaded firmware
 * @last_firmware_crc: CRC32 of last loaded firmware
 */
struct mipsloader_metrics {
	/* Register access tracking */
	atomic64_t reg_access_count[4];  /* cmd, status, data, control */
	
	/* Firmware loading metrics */
	atomic64_t firmware_load_attempts;
	atomic64_t firmware_load_success;
	atomic64_t firmware_load_failures;
	
	/* Memory usage tracking */
	atomic64_t memory_regions_used;
	
	/* Error tracking */
	atomic64_t communication_errors;
	
	/* Status information */
	size_t last_firmware_size;
	u32 last_firmware_crc;
};

STRUCT_EOF

# Extract original device structure section (lines 71-95) and modify it
sed -n '71,82p' "$BACKUP_FILE" >> "$OUTPUT_FILE"
echo " * @metrics: Prometheus metrics tracking" >> "$OUTPUT_FILE"
sed -n '83,95p' "$BACKUP_FILE" >> "$OUTPUT_FILE"
echo "	struct mipsloader_metrics metrics;" >> "$OUTPUT_FILE"
echo "};" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Add global variables
echo "static struct mipsloader_device *mipsloader_dev;" >> "$OUTPUT_FILE"
echo "static struct class *hy300_class;" >> "$OUTPUT_FILE"
echo "" >> "$OUTPUT_FILE"

# Add helper function
cat >> "$OUTPUT_FILE" << 'HELPER_EOF'
/**
 * mipsloader_reg_offset_to_index - Convert register offset to metrics index
 */
static inline int mipsloader_reg_offset_to_index(u32 offset)
{
	switch (offset) {
	case MIPS_REG_CMD:     return 0;
	case MIPS_REG_STATUS:  return 1;
	case MIPS_REG_DATA:    return 2;
	case MIPS_REG_CONTROL: return 3;
	default:               return -1;
	}
}

HELPER_EOF

# Add modified register functions with metrics tracking
cat >> "$OUTPUT_FILE" << 'REG_EOF'
/**
 * mipsloader_reg_read - Read from MIPS control register
 * @offset: Register offset
 * 
 * Returns register value
 */
static u32 mipsloader_reg_read(u32 offset)
{
	int idx;
	u32 value;
	
	if (!mipsloader_dev || !mipsloader_dev->reg_base)
		return 0;
	
	idx = mipsloader_reg_offset_to_index(offset);
	if (idx >= 0)
		atomic64_inc(&mipsloader_dev->metrics.reg_access_count[idx]);
	
	value = readl(mipsloader_dev->reg_base + offset);
	return value;
}

/**
 * mipsloader_reg_write - Write to MIPS control register
 * @offset: Register offset
 * @value: Value to write
 */
static void mipsloader_reg_write(u32 offset, u32 value)
{
	int idx;
	
	if (!mipsloader_dev || !mipsloader_dev->reg_base)
		return;
	
	idx = mipsloader_reg_offset_to_index(offset);
	if (idx >= 0)
		atomic64_inc(&mipsloader_dev->metrics.reg_access_count[idx]);
	
	writel(value, mipsloader_dev->reg_base + offset);
}

REG_EOF

# Add modified firmware load function with metrics
cat >> "$OUTPUT_FILE" << 'FW_EOF'
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
	
	/* Track firmware load attempt */
	atomic64_inc(&mipsloader_dev->metrics.firmware_load_attempts);
	
	if (!mipsloader_dev || !mipsloader_dev->mem_base) {
		pr_err("mipsloader: Device not initialized\n");
		atomic64_inc(&mipsloader_dev->metrics.firmware_load_failures);
		return -ENODEV;
	}
	
	/* Request firmware from userspace */
	ret = request_firmware(&fw, firmware_path, &mipsloader_dev->pdev->dev);
	if (ret) {
		dev_err(&mipsloader_dev->pdev->dev, 
			"Failed to load firmware %s: %d\n", firmware_path, ret);
		atomic64_inc(&mipsloader_dev->metrics.firmware_load_failures);
		return ret;
	}
	
	/* Validate firmware size */
	if (fw->size > (MIPS_FIRMWARE_ADDR - MIPS_BOOT_CODE_ADDR + 0xc00000)) {
		dev_err(&mipsloader_dev->pdev->dev,
			"Firmware too large: %zu bytes\n", fw->size);
		atomic64_inc(&mipsloader_dev->metrics.firmware_load_failures);
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
	
	/* Update metrics on successful load */
	atomic64_inc(&mipsloader_dev->metrics.firmware_load_success);
	atomic64_inc(&mipsloader_dev->metrics.memory_regions_used);
	mipsloader_dev->metrics.last_firmware_size = fw->size;
	mipsloader_dev->metrics.last_firmware_crc = calculated_crc;
	
	mipsloader_dev->firmware_loaded = true;
	dev_info(&mipsloader_dev->pdev->dev, "Firmware loaded successfully\n");
	
	ret = 0;

release_fw:
	release_firmware(fw);
	return ret;
}

FW_EOF

# Copy the rest of the functions up to the file_operations (lines 182-295)
sed -n '182,295p' "$BACKUP_FILE" >> "$OUTPUT_FILE"

# Add sysfs functions
cat >> "$OUTPUT_FILE" << 'SYSFS_EOF'

/* Sysfs attribute functions for Prometheus metrics */

static ssize_t memory_stats_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct mipsloader_device *mips_dev = dev_get_drvdata(dev);
	ssize_t len = 0;
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_memory_usage_bytes Memory usage in MIPS regions\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_memory_usage_bytes gauge\n");
	
	if (mips_dev->firmware_loaded) {
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"boot_code\"} 4096\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"firmware\"} %zu\n",
			mips_dev->metrics.last_firmware_size);
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"debug\"} 1048576\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"config\"} 262144\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"database\"} 1048576\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_memory_usage_bytes{region=\"framebuffer\"} 27262976\n");
	}
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_memory_regions_active Number of active memory regions\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_memory_regions_active gauge\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_memory_regions_active %lld\n",
		atomic64_read(&mips_dev->metrics.memory_regions_used));
	
	return len;
}
static DEVICE_ATTR_RO(memory_stats);

static ssize_t register_access_count_show(struct device *dev,
					  struct device_attribute *attr, char *buf)
{
	struct mipsloader_device *mips_dev = dev_get_drvdata(dev);
	ssize_t len = 0;
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_register_access_total Total register access count\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_register_access_total counter\n");
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_register_access_total{register=\"cmd\"} %lld\n",
		atomic64_read(&mips_dev->metrics.reg_access_count[0]));
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_register_access_total{register=\"status\"} %lld\n",
		atomic64_read(&mips_dev->metrics.reg_access_count[1]));
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_register_access_total{register=\"data\"} %lld\n",
		atomic64_read(&mips_dev->metrics.reg_access_count[2]));
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_register_access_total{register=\"control\"} %lld\n",
		atomic64_read(&mips_dev->metrics.reg_access_count[3]));
	
	return len;
}
static DEVICE_ATTR_RO(register_access_count);

static ssize_t firmware_status_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mipsloader_device *mips_dev = dev_get_drvdata(dev);
	ssize_t len = 0;
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_firmware_load_attempts_total Total firmware load attempts\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_firmware_load_attempts_total counter\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_firmware_load_attempts_total %lld\n",
		atomic64_read(&mips_dev->metrics.firmware_load_attempts));
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_firmware_load_success_total Successful firmware loads\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_firmware_load_success_total counter\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_firmware_load_success_total %lld\n",
		atomic64_read(&mips_dev->metrics.firmware_load_success));
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_firmware_load_failures_total Failed firmware loads\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_firmware_load_failures_total counter\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_firmware_load_failures_total %lld\n",
		atomic64_read(&mips_dev->metrics.firmware_load_failures));
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_firmware_loaded Current firmware load status\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_firmware_loaded gauge\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_firmware_loaded %d\n",
		mips_dev->firmware_loaded ? 1 : 0);
	
	if (mips_dev->firmware_loaded) {
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"# HELP hy300_mips_firmware_size_bytes Size of loaded firmware\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"# TYPE hy300_mips_firmware_size_bytes gauge\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_firmware_size_bytes %zu\n",
			mips_dev->metrics.last_firmware_size);
		
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"# HELP hy300_mips_firmware_crc32 CRC32 of loaded firmware\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"# TYPE hy300_mips_firmware_crc32 gauge\n");
		len += scnprintf(buf + len, PAGE_SIZE - len,
			"hy300_mips_firmware_crc32 %u\n",
			mips_dev->metrics.last_firmware_crc);
	}
	
	return len;
}
static DEVICE_ATTR_RO(firmware_status);

static ssize_t communication_errors_show(struct device *dev,
					 struct device_attribute *attr, char *buf)
{
	struct mipsloader_device *mips_dev = dev_get_drvdata(dev);
	ssize_t len = 0;
	
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# HELP hy300_mips_communication_errors_total Communication error count\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"# TYPE hy300_mips_communication_errors_total counter\n");
	len += scnprintf(buf + len, PAGE_SIZE - len,
		"hy300_mips_communication_errors_total %lld\n",
		atomic64_read(&mips_dev->metrics.communication_errors));
	
	return len;
}
static DEVICE_ATTR_RO(communication_errors);

static struct attribute *mipsloader_attrs[] = {
	&dev_attr_memory_stats.attr,
	&dev_attr_register_access_count.attr,
	&dev_attr_firmware_status.attr,
	&dev_attr_communication_errors.attr,
	NULL,
};
ATTRIBUTE_GROUPS(mipsloader);

SYSFS_EOF

# Add modified probe function
cat >> "$OUTPUT_FILE" << 'PROBE_EOF'
/**
 * mipsloader_probe - Platform device probe
 */
static int mipsloader_probe(struct platform_device *pdev)
{
	struct resource *res;
	int ret;
	int i;
	dev_t devt;
	
	dev_info(&pdev->dev, "Probing MIPS loader device\n");
	
	/* Allocate device structure */
	mipsloader_dev = devm_kzalloc(&pdev->dev, sizeof(*mipsloader_dev), GFP_KERNEL);
	if (!mipsloader_dev)
		return -ENOMEM;
	
	mipsloader_dev->pdev = pdev;
	mutex_init(&mipsloader_dev->lock);
	platform_set_drvdata(pdev, mipsloader_dev);
	
	/* Initialize metrics */
	memset(&mipsloader_dev->metrics, 0, sizeof(mipsloader_dev->metrics));
	for (i = 0; i < 4; i++)
		atomic64_set(&mipsloader_dev->metrics.reg_access_count[i], 0);
	
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
	
	/* Create hy300 class if it doesn't exist */
	if (!hy300_class) {
		hy300_class = class_create("hy300");
		if (IS_ERR(hy300_class)) {
			ret = PTR_ERR(hy300_class);
			dev_err(&pdev->dev, "Failed to create hy300 class: %d\n", ret);
			hy300_class = NULL;
			goto del_cdev;
		}
	}
	
	/* Create device class */
	mipsloader_dev->class = class_create(MIPSLOADER_CLASS_NAME);
	if (IS_ERR(mipsloader_dev->class)) {
		ret = PTR_ERR(mipsloader_dev->class);
		dev_err(&pdev->dev, "Failed to create device class: %d\n", ret);
		goto del_cdev;
	}
	
	/* Create device node */
	mipsloader_dev->device = device_create_with_groups(mipsloader_dev->class, &pdev->dev,
					       devt, mipsloader_dev, mipsloader_groups, MIPSLOADER_DEVICE_NAME);
	if (IS_ERR(mipsloader_dev->device)) {
		ret = PTR_ERR(mipsloader_dev->device);
		dev_err(&pdev->dev, "Failed to create device: %d\n", ret);
		goto destroy_class;
	}
	
	/* Create additional device node in hy300 class for metrics */
	if (hy300_class) {
		struct device *metrics_dev = device_create_with_groups(hy300_class, &pdev->dev,
							MKDEV(0, 0), mipsloader_dev, mipsloader_groups, "mips");
		if (IS_ERR(metrics_dev)) {
			dev_warn(&pdev->dev, "Failed to create metrics device: %ld\n", PTR_ERR(metrics_dev));
		} else {
			dev_info(&pdev->dev, "Metrics available at /sys/class/hy300/mips/\n");
		}
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

PROBE_EOF

# Add modified remove function
cat >> "$OUTPUT_FILE" << 'REMOVE_EOF'
/**
 * mipsloader_remove - Platform device remove
 */
static void mipsloader_remove(struct platform_device *pdev)
{
	struct mipsloader_device *dev = platform_get_drvdata(pdev);
	dev_t devt = MKDEV(dev->major, 0);
	
	dev_info(&pdev->dev, "Removing MIPS loader device\n");
	
	/* Remove metrics device from hy300 class */
	if (hy300_class) {
		device_destroy(hy300_class, MKDEV(0, 0));
		/* Note: Not destroying hy300_class as other drivers may use it */
	}
	
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

REMOVE_EOF

# Copy the rest of the file (platform driver structure and module info)
sed -n '421,441p' "$BACKUP_FILE" >> "$OUTPUT_FILE"

echo "Successfully built MIPS loader driver with Prometheus metrics"
echo "Output file: $OUTPUT_FILE"