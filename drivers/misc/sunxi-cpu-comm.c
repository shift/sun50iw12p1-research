// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner SUNXI CPU Communication Driver
 * ARM-MIPS communication interface for H713 SoC
 *
 * This driver provides high-level HDMI control API through ARM-MIPS
 * communication protocol for TV capture and display control.
 *
 * Copyright (C) 2025 HY300 Linux Porting Project
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/crc32.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#define DRIVER_NAME "sunxi-cpu-comm"
#define DEVICE_NAME "sunxi-cpu-comm"
#define CLASS_NAME "cpu_comm"

/* HDMI Control Commands - from ARM_MIPS_COMMUNICATION_PROTOCOL.md */
#define MIPS_CMD_HDMI_DETECT        0x1001    /* Detect HDMI input presence */
#define MIPS_CMD_HDMI_EDID_READ     0x1002    /* Read EDID from HDMI source */
#define MIPS_CMD_HDMI_FORMAT_GET    0x1003    /* Get current video format */
#define MIPS_CMD_HDMI_FORMAT_SET    0x1004    /* Set video format/timing */
#define MIPS_CMD_HDMI_CAPTURE_START 0x1005    /* Start HDMI capture */
#define MIPS_CMD_HDMI_CAPTURE_STOP  0x1006    /* Stop HDMI capture */
#define MIPS_CMD_HDMI_BUFFER_SETUP  0x1007    /* Setup capture buffers */

/* Command response status codes */
#define MIPS_STATUS_SUCCESS         0x0000
#define MIPS_STATUS_ERROR           0xFFFF
#define MIPS_STATUS_TIMEOUT         0xFFFE
#define MIPS_STATUS_NO_SIGNAL       0xFFFD
#define MIPS_STATUS_INVALID_FORMAT  0xFFFC

/* Command communication structure - 512 bytes total */
#define CMD_BUFFER_SIZE 512
#define CMD_HEADER_SIZE 32
#define CMD_DATA_SIZE   (CMD_BUFFER_SIZE - CMD_HEADER_SIZE)

struct cpu_comm_cmd_header {
	__u32 cmd;           /* Command code */
	__u32 data_size;     /* Data payload size in bytes */
	__u32 sequence;      /* Command sequence number */
	__u32 crc;           /* CRC32 of command + data */
	__u32 timeout_ms;    /* Command timeout in milliseconds */
	__u32 flags;         /* Command flags */
	__u32 reserved[2];   /* Reserved for future use */
};

struct cpu_comm_response_header {
	__u32 status;        /* Response status code */
	__u32 data_size;     /* Response data size in bytes */
	__u32 sequence;      /* Matching command sequence */
	__u32 crc;           /* CRC32 of response + data */
	__u32 processing_time; /* MIPS processing time in us */
	__u32 flags;         /* Response flags */
	__u32 reserved[2];   /* Reserved for future use */
};

/* HDMI format information structure */
struct hdmi_format_info {
	__u32 width;         /* Video width (e.g., 1920) */
	__u32 height;        /* Video height (e.g., 1080) */
	__u32 framerate;     /* Frame rate (e.g., 60) */
	__u32 format;        /* Pixel format (YUV/RGB) */
	__u32 htotal;        /* Horizontal total */
	__u32 vtotal;        /* Vertical total */
	__u32 pclk;          /* Pixel clock */
	__u32 reserved[1];   /* Reserved for alignment */
};

/* HDMI buffer information structure */
struct hdmi_buffer_info {
	__u32 buffer_addr;   /* Physical buffer address */
	__u32 buffer_size;   /* Buffer size in bytes */
	__u32 buffer_count;  /* Number of buffers */
	__u32 reserved[1];   /* Reserved for alignment */
};

/* IOCTL definitions */
#define CPU_COMM_IOC_MAGIC 'C'
#define CPU_COMM_HDMI_DETECT        _IOR(CPU_COMM_IOC_MAGIC, 1, int)
#define CPU_COMM_HDMI_READ_EDID     _IOWR(CPU_COMM_IOC_MAGIC, 2, void*)
#define CPU_COMM_HDMI_GET_FORMAT    _IOR(CPU_COMM_IOC_MAGIC, 3, struct hdmi_format_info)
#define CPU_COMM_HDMI_SET_FORMAT    _IOW(CPU_COMM_IOC_MAGIC, 4, struct hdmi_format_info)
#define CPU_COMM_HDMI_START_CAPTURE _IOW(CPU_COMM_IOC_MAGIC, 5, struct hdmi_buffer_info)
#define CPU_COMM_HDMI_STOP_CAPTURE  _IO(CPU_COMM_IOC_MAGIC, 6)
#define CPU_COMM_HDMI_SETUP_BUFFER  _IOW(CPU_COMM_IOC_MAGIC, 7, struct hdmi_buffer_info)

/* Device structure */
struct sunxi_cpu_comm_dev {
	struct device *dev;
	struct platform_device *pdev;
	
	/* Character device interface */
	dev_t dev_num;
	struct cdev cdev;
	struct class *dev_class;
	struct device *char_dev;
	
	/* Hardware resources */
	void __iomem *base;
	int irq;
	
	/* Shared memory for ARM-MIPS communication */
	dma_addr_t cmd_buffer_phys;
	void *cmd_buffer_virt;
	size_t cmd_buffer_size;
	
	dma_addr_t resp_buffer_phys;
	void *resp_buffer_virt;
	size_t resp_buffer_size;
	
	/* Communication synchronization */
	struct mutex cmd_lock;
	struct completion cmd_completion;
	atomic_t sequence_counter;
	
	/* NSI interface reference */
	struct device *nsi_dev;
	
	/* Status */
	bool initialized;
	bool mips_comm_ready;
};

/* Global device instance */
static struct sunxi_cpu_comm_dev *g_cpu_comm_dev = NULL;

/* Forward declarations */
static int sunxi_cpu_comm_send_command(struct sunxi_cpu_comm_dev *dev, 
	u32 cmd, void *data, size_t data_size, void *response, size_t *resp_size);
static irqreturn_t sunxi_cpu_comm_irq_handler(int irq, void *dev_id);

/*
 * Hardware register interface
 * Base address: 0x3061000 (from device tree)
 */
#define CPU_COMM_CTRL_REG       0x0000
#define CPU_COMM_STATUS_REG     0x0004
#define CPU_COMM_IRQ_EN_REG     0x0008
#define CPU_COMM_IRQ_STATUS_REG 0x000C
#define CPU_COMM_CMD_ADDR_REG   0x0010
#define CPU_COMM_RESP_ADDR_REG  0x0014
#define CPU_COMM_TRIGGER_REG    0x0018

/* Control register bits */
#define CPU_COMM_CTRL_ENABLE    BIT(0)
#define CPU_COMM_CTRL_RESET     BIT(1)
#define CPU_COMM_CTRL_IRQ_EN    BIT(2)

/* Status register bits */
#define CPU_COMM_STATUS_READY   BIT(0)
#define CPU_COMM_STATUS_BUSY    BIT(1)
#define CPU_COMM_STATUS_ERROR   BIT(2)

/* Interrupt bits */
#define CPU_COMM_IRQ_CMD_DONE   BIT(0)
#define CPU_COMM_IRQ_MIPS_RESP  BIT(1)
#define CPU_COMM_IRQ_ERROR      BIT(2)

/*
 * CRC32 calculation for command validation
 */
static u32 calculate_command_crc(struct cpu_comm_cmd_header *header, void *data)
{
	u32 crc = 0;
	
	/* Calculate CRC over header (excluding CRC field) and data */
	crc = crc32(crc, (u8*)header, offsetof(struct cpu_comm_cmd_header, crc));
	crc = crc32(crc, (u8*)&header->timeout_ms, 
		    sizeof(struct cpu_comm_cmd_header) - offsetof(struct cpu_comm_cmd_header, timeout_ms));
	
	if (data && header->data_size > 0) {
		crc = crc32(crc, (u8*)data, header->data_size);
	}
	
	return crc;
}

/*
 * Send command to MIPS co-processor
 */
static int sunxi_cpu_comm_send_command(struct sunxi_cpu_comm_dev *dev, 
	u32 cmd, void *data, size_t data_size, void *response, size_t *resp_size)
{
	struct cpu_comm_cmd_header *cmd_header;
	struct cpu_comm_response_header *resp_header;
	unsigned long timeout;
	int ret = 0;
	u32 sequence;
	
	if (!dev || !dev->initialized) {
		dev_err(dev->dev, "Device not initialized\n");
		return -ENODEV;
	}
	
	if (data_size > CMD_DATA_SIZE) {
		dev_err(dev->dev, "Command data too large: %zu > %d\n", data_size, CMD_DATA_SIZE);
		return -EINVAL;
	}
	
	mutex_lock(&dev->cmd_lock);
	
	/* Prepare command buffer */
	cmd_header = (struct cpu_comm_cmd_header *)dev->cmd_buffer_virt;
	memset(cmd_header, 0, CMD_BUFFER_SIZE);
	
	sequence = atomic_inc_return(&dev->sequence_counter);
	
	cmd_header->cmd = cmd;
	cmd_header->data_size = data_size;
	cmd_header->sequence = sequence;
	cmd_header->timeout_ms = 5000; /* 5 second timeout */
	cmd_header->flags = 0;
	
	/* Copy data payload if present */
	if (data && data_size > 0) {
		memcpy((u8*)cmd_header + CMD_HEADER_SIZE, data, data_size);
	}
	
	/* Calculate and set CRC */
	cmd_header->crc = calculate_command_crc(cmd_header, 
		data_size > 0 ? (u8*)cmd_header + CMD_HEADER_SIZE : NULL);
	
	/* Ensure data is flushed to memory */
	dma_sync_single_for_device(dev->dev, dev->cmd_buffer_phys, 
				    CMD_BUFFER_SIZE, DMA_TO_DEVICE);
	
	/* Reset completion */
	reinit_completion(&dev->cmd_completion);
	
	/* Write command buffer address to hardware */
	writel(dev->cmd_buffer_phys, dev->base + CPU_COMM_CMD_ADDR_REG);
	writel(dev->resp_buffer_phys, dev->base + CPU_COMM_RESP_ADDR_REG);
	
	/* Trigger command execution */
	writel(CPU_COMM_CTRL_ENABLE | CPU_COMM_CTRL_IRQ_EN, 
	       dev->base + CPU_COMM_CTRL_REG);
	writel(1, dev->base + CPU_COMM_TRIGGER_REG);
	
	dev_dbg(dev->dev, "Sent command 0x%x, sequence %u, data_size %zu\n", 
		cmd, sequence, data_size);
	
	/* Wait for completion */
	timeout = msecs_to_jiffies(cmd_header->timeout_ms + 1000); /* Add 1s buffer */
	if (!wait_for_completion_timeout(&dev->cmd_completion, timeout)) {
		dev_err(dev->dev, "Command 0x%x timeout (sequence %u)\n", cmd, sequence);
		ret = -ETIMEDOUT;
		goto out;
	}
	
	/* Sync response buffer */
	dma_sync_single_for_cpu(dev->dev, dev->resp_buffer_phys, 
				CMD_BUFFER_SIZE, DMA_FROM_DEVICE);
	
	/* Parse response */
	resp_header = (struct cpu_comm_response_header *)dev->resp_buffer_virt;
	
	/* Validate response */
	if (resp_header->sequence != sequence) {
		dev_err(dev->dev, "Response sequence mismatch: got %u, expected %u\n",
			resp_header->sequence, sequence);
		ret = -EIO;
		goto out;
	}
	
	if (resp_header->status != MIPS_STATUS_SUCCESS) {
		dev_warn(dev->dev, "Command 0x%x failed with status 0x%x\n", 
			 cmd, resp_header->status);
		ret = -EIO;
		goto out;
	}
	
	/* Copy response data if requested */
	if (response && resp_size) {
		size_t copy_size = min(*resp_size, (size_t)resp_header->data_size);
		if (copy_size > 0) {
			memcpy(response, (u8*)resp_header + CMD_HEADER_SIZE, copy_size);
		}
		*resp_size = resp_header->data_size;
	}
	
	dev_dbg(dev->dev, "Command 0x%x completed successfully (sequence %u)\n", 
		cmd, sequence);

out:
	mutex_unlock(&dev->cmd_lock);
	return ret;
}

/*
 * HDMI detection
 */
static int sunxi_cpu_comm_hdmi_detect(struct sunxi_cpu_comm_dev *dev)
{
	int ret;
	u32 detection_result = 0;
	size_t resp_size = sizeof(detection_result);
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_DETECT, 
					  NULL, 0, &detection_result, &resp_size);
	if (ret) {
		dev_err(dev->dev, "HDMI detection failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI detection result: %s\n", 
		 detection_result ? "CONNECTED" : "NOT CONNECTED");
	
	return detection_result ? 1 : 0;
}

/*
 * HDMI EDID reading
 */
static int sunxi_cpu_comm_hdmi_read_edid(struct sunxi_cpu_comm_dev *dev, 
					 u8 *edid_buffer, size_t buffer_size)
{
	int ret;
	size_t resp_size = buffer_size;
	
	if (!edid_buffer || buffer_size < 128) {
		dev_err(dev->dev, "Invalid EDID buffer\n");
		return -EINVAL;
	}
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_EDID_READ, 
					  NULL, 0, edid_buffer, &resp_size);
	if (ret) {
		dev_err(dev->dev, "HDMI EDID read failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI EDID read successfully, %zu bytes\n", resp_size);
	return resp_size;
}

/*
 * Get HDMI format information
 */
static int sunxi_cpu_comm_hdmi_get_format(struct sunxi_cpu_comm_dev *dev, 
					  struct hdmi_format_info *format)
{
	int ret;
	size_t resp_size = sizeof(*format);
	
	if (!format) {
		return -EINVAL;
	}
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_FORMAT_GET, 
					  NULL, 0, format, &resp_size);
	if (ret) {
		dev_err(dev->dev, "HDMI format get failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI format: %ux%u@%u, format=0x%x\n",
		 format->width, format->height, format->framerate, format->format);
	
	return 0;
}

/*
 * Set HDMI format
 */
static int sunxi_cpu_comm_hdmi_set_format(struct sunxi_cpu_comm_dev *dev, 
					  struct hdmi_format_info *format)
{
	int ret;
	
	if (!format) {
		return -EINVAL;
	}
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_FORMAT_SET, 
					  format, sizeof(*format), NULL, NULL);
	if (ret) {
		dev_err(dev->dev, "HDMI format set failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI format set to %ux%u@%u\n",
		 format->width, format->height, format->framerate);
	
	return 0;
}

/*
 * Start HDMI capture
 */
static int sunxi_cpu_comm_hdmi_start_capture(struct sunxi_cpu_comm_dev *dev, 
					     struct hdmi_buffer_info *buffers)
{
	int ret;
	
	if (!buffers) {
		return -EINVAL;
	}
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_CAPTURE_START, 
					  buffers, sizeof(*buffers), NULL, NULL);
	if (ret) {
		dev_err(dev->dev, "HDMI capture start failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI capture started with %u buffers\n", 
		 buffers->buffer_count);
	
	return 0;
}

/*
 * Stop HDMI capture
 */
static int sunxi_cpu_comm_hdmi_stop_capture(struct sunxi_cpu_comm_dev *dev)
{
	int ret;
	
	ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_CAPTURE_STOP, 
					  NULL, 0, NULL, NULL);
	if (ret) {
		dev_err(dev->dev, "HDMI capture stop failed: %d\n", ret);
		return ret;
	}
	
	dev_info(dev->dev, "HDMI capture stopped\n");
	return 0;
}

/*
 * Character device operations
 */
static long sunxi_cpu_comm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sunxi_cpu_comm_dev *dev = g_cpu_comm_dev;
	int ret = 0;
	
	if (!dev) {
		return -ENODEV;
	}
	
	switch (cmd) {
	case CPU_COMM_HDMI_DETECT:
		{
			int result = sunxi_cpu_comm_hdmi_detect(dev);
			if (result < 0) {
				return result;
			}
			if (copy_to_user((void __user *)arg, &result, sizeof(result))) {
				return -EFAULT;
			}
		}
		break;
		
	case CPU_COMM_HDMI_READ_EDID:
		{
			u8 edid_buffer[256];
			int bytes_read = sunxi_cpu_comm_hdmi_read_edid(dev, edid_buffer, sizeof(edid_buffer));
			if (bytes_read < 0) {
				return bytes_read;
			}
			if (copy_to_user((void __user *)arg, edid_buffer, bytes_read)) {
				return -EFAULT;
			}
		}
		break;
		
	case CPU_COMM_HDMI_GET_FORMAT:
		{
			struct hdmi_format_info format;
			ret = sunxi_cpu_comm_hdmi_get_format(dev, &format);
			if (ret) {
				return ret;
			}
			if (copy_to_user((void __user *)arg, &format, sizeof(format))) {
				return -EFAULT;
			}
		}
		break;
		
	case CPU_COMM_HDMI_SET_FORMAT:
		{
			struct hdmi_format_info format;
			if (copy_from_user(&format, (void __user *)arg, sizeof(format))) {
				return -EFAULT;
			}
			ret = sunxi_cpu_comm_hdmi_set_format(dev, &format);
		}
		break;
		
	case CPU_COMM_HDMI_START_CAPTURE:
		{
			struct hdmi_buffer_info buffers;
			if (copy_from_user(&buffers, (void __user *)arg, sizeof(buffers))) {
				return -EFAULT;
			}
			ret = sunxi_cpu_comm_hdmi_start_capture(dev, &buffers);
		}
		break;
		
	case CPU_COMM_HDMI_STOP_CAPTURE:
		ret = sunxi_cpu_comm_hdmi_stop_capture(dev);
		break;
		
	case CPU_COMM_HDMI_SETUP_BUFFER:
		{
			struct hdmi_buffer_info buffers;
			if (copy_from_user(&buffers, (void __user *)arg, sizeof(buffers))) {
				return -EFAULT;
			}
			/* Setup buffer is same as start capture for now */
			ret = sunxi_cpu_comm_send_command(dev, MIPS_CMD_HDMI_BUFFER_SETUP, 
							  &buffers, sizeof(buffers), NULL, NULL);
		}
		break;
		
	default:
		return -ENOTTY;
	}
	
	return ret;
}

static int sunxi_cpu_comm_open(struct inode *inode, struct file *file)
{
	if (!g_cpu_comm_dev) {
		return -ENODEV;
	}
	
	file->private_data = g_cpu_comm_dev;
	return 0;
}

static int sunxi_cpu_comm_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations sunxi_cpu_comm_fops = {
	.owner          = THIS_MODULE,
	.open           = sunxi_cpu_comm_open,
	.release        = sunxi_cpu_comm_release,
	.unlocked_ioctl = sunxi_cpu_comm_ioctl,
	.compat_ioctl   = sunxi_cpu_comm_ioctl,
};

/*
 * Interrupt handler for ARM-MIPS communication
 */
static irqreturn_t sunxi_cpu_comm_irq_handler(int irq, void *dev_id)
{
	struct sunxi_cpu_comm_dev *dev = dev_id;
	u32 irq_status;
	
	/* Read interrupt status */
	irq_status = readl(dev->base + CPU_COMM_IRQ_STATUS_REG);
	
	if (irq_status & CPU_COMM_IRQ_CMD_DONE) {
		/* Command completion interrupt */
		dev_dbg(dev->dev, "Command completion interrupt\n");
		complete(&dev->cmd_completion);
		irq_status &= ~CPU_COMM_IRQ_CMD_DONE;
	}
	
	if (irq_status & CPU_COMM_IRQ_MIPS_RESP) {
		/* MIPS response ready interrupt */
		dev_dbg(dev->dev, "MIPS response ready interrupt\n");
		complete(&dev->cmd_completion);
		irq_status &= ~CPU_COMM_IRQ_MIPS_RESP;
	}
	
	if (irq_status & CPU_COMM_IRQ_ERROR) {
		/* Error interrupt */
		dev_err(dev->dev, "Communication error interrupt\n");
		complete(&dev->cmd_completion);
		irq_status &= ~CPU_COMM_IRQ_ERROR;
	}
	
	/* Clear processed interrupts */
	writel(irq_status, dev->base + CPU_COMM_IRQ_STATUS_REG);
	
	return IRQ_HANDLED;
}

/*
 * Platform driver probe
 */
static int sunxi_cpu_comm_probe(struct platform_device *pdev)
{
	struct sunxi_cpu_comm_dev *dev;
	struct resource *res;
	int ret;
	
	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		return -ENOMEM;
	}
	
	dev->dev = &pdev->dev;
	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);
	
	/* Get hardware resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(dev->base)) {
		dev_err(&pdev->dev, "Failed to map registers\n");
		return PTR_ERR(dev->base);
	}
	
	dev->irq = platform_get_irq(pdev, 0);
	if (dev->irq < 0) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		return dev->irq;
	}
	
	/* Allocate command/response buffers */
	dev->cmd_buffer_size = CMD_BUFFER_SIZE;
	dev->cmd_buffer_virt = dma_alloc_coherent(&pdev->dev, dev->cmd_buffer_size,
						  &dev->cmd_buffer_phys, GFP_KERNEL);
	if (!dev->cmd_buffer_virt) {
		dev_err(&pdev->dev, "Failed to allocate command buffer\n");
		return -ENOMEM;
	}
	
	dev->resp_buffer_size = CMD_BUFFER_SIZE;
	dev->resp_buffer_virt = dma_alloc_coherent(&pdev->dev, dev->resp_buffer_size,
						   &dev->resp_buffer_phys, GFP_KERNEL);
	if (!dev->resp_buffer_virt) {
		dev_err(&pdev->dev, "Failed to allocate response buffer\n");
		ret = -ENOMEM;
		goto err_free_cmd_buffer;
	}
	
	/* Initialize synchronization */
	mutex_init(&dev->cmd_lock);
	init_completion(&dev->cmd_completion);
	atomic_set(&dev->sequence_counter, 0);
	
	/* Request IRQ */
	ret = devm_request_irq(&pdev->dev, dev->irq, sunxi_cpu_comm_irq_handler,
			       IRQF_SHARED, DRIVER_NAME, dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request IRQ %d: %d\n", dev->irq, ret);
		goto err_free_resp_buffer;
	}
	
	/* Create character device */
	ret = alloc_chrdev_region(&dev->dev_num, 0, 1, DEVICE_NAME);
	if (ret) {
		dev_err(&pdev->dev, "Failed to allocate character device region\n");
		goto err_free_resp_buffer;
	}
	
	cdev_init(&dev->cdev, &sunxi_cpu_comm_fops);
	dev->cdev.owner = THIS_MODULE;
	
	ret = cdev_add(&dev->cdev, dev->dev_num, 1);
	if (ret) {
		dev_err(&pdev->dev, "Failed to add character device\n");
		goto err_unregister_chrdev;
	}
	
	/* Create device class */
	dev->dev_class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(dev->dev_class)) {
		dev_err(&pdev->dev, "Failed to create device class\n");
		ret = PTR_ERR(dev->dev_class);
		goto err_cdev_del;
	}
	
	/* Create device node */
	dev->char_dev = device_create(dev->dev_class, &pdev->dev, dev->dev_num, NULL, DEVICE_NAME);
	if (IS_ERR(dev->char_dev)) {
		dev_err(&pdev->dev, "Failed to create device node\n");
		ret = PTR_ERR(dev->char_dev);
		goto err_class_destroy;
	}
	
	/* Initialize hardware */
	writel(0, dev->base + CPU_COMM_CTRL_REG);  /* Reset state */
	writel(CPU_COMM_IRQ_CMD_DONE | CPU_COMM_IRQ_MIPS_RESP | CPU_COMM_IRQ_ERROR,
	       dev->base + CPU_COMM_IRQ_EN_REG);   /* Enable interrupts */
	
	dev->initialized = true;
	dev->mips_comm_ready = true;  /* Assume MIPS is ready after boot */
	g_cpu_comm_dev = dev;
	
	dev_info(&pdev->dev, "SUNXI CPU Communication driver initialized\n");
	dev_info(&pdev->dev, "Character device created: /dev/%s\n", DEVICE_NAME);
	dev_info(&pdev->dev, "Command buffer: phys=0x%llx size=%zu\n", 
		 (u64)dev->cmd_buffer_phys, dev->cmd_buffer_size);
	dev_info(&pdev->dev, "Response buffer: phys=0x%llx size=%zu\n", 
		 (u64)dev->resp_buffer_phys, dev->resp_buffer_size);
	
	return 0;

err_class_destroy:
	class_destroy(dev->dev_class);
err_cdev_del:
	cdev_del(&dev->cdev);
err_unregister_chrdev:
	unregister_chrdev_region(dev->dev_num, 1);
err_free_resp_buffer:
	dma_free_coherent(&pdev->dev, dev->resp_buffer_size,
			  dev->resp_buffer_virt, dev->resp_buffer_phys);
err_free_cmd_buffer:
	dma_free_coherent(&pdev->dev, dev->cmd_buffer_size,
			  dev->cmd_buffer_virt, dev->cmd_buffer_phys);
	return ret;
}

/*
 * Platform driver remove
 */
static int sunxi_cpu_comm_remove(struct platform_device *pdev)
{
	struct sunxi_cpu_comm_dev *dev = platform_get_drvdata(pdev);
	
	if (dev) {
		g_cpu_comm_dev = NULL;
		dev->initialized = false;
		
		/* Disable hardware */
		writel(0, dev->base + CPU_COMM_CTRL_REG);
		writel(0, dev->base + CPU_COMM_IRQ_EN_REG);
		
		/* Remove character device */
		device_destroy(dev->dev_class, dev->dev_num);
		class_destroy(dev->dev_class);
		cdev_del(&dev->cdev);
		unregister_chrdev_region(dev->dev_num, 1);
		
		/* Free DMA buffers */
		dma_free_coherent(&pdev->dev, dev->resp_buffer_size,
				  dev->resp_buffer_virt, dev->resp_buffer_phys);
		dma_free_coherent(&pdev->dev, dev->cmd_buffer_size,
				  dev->cmd_buffer_virt, dev->cmd_buffer_phys);
		
		dev_info(&pdev->dev, "SUNXI CPU Communication driver removed\n");
	}
	
	return 0;
}

/* Device tree compatibility */
static const struct of_device_id sunxi_cpu_comm_dt_ids[] = {
	{ .compatible = "allwinner,sunxi-cpu-comm" },
	{ .compatible = "allwinner,sun50i-h713-cpu-comm" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_cpu_comm_dt_ids);

static struct platform_driver sunxi_cpu_comm_driver = {
	.probe  = sunxi_cpu_comm_probe,
	.remove = sunxi_cpu_comm_remove,
	.driver = {
		.name           = DRIVER_NAME,
		.of_match_table = sunxi_cpu_comm_dt_ids,
	},
};

/*
 * Export functions for other kernel modules
 */
struct sunxi_cpu_comm_dev *sunxi_cpu_comm_get_device(void)
{
	return g_cpu_comm_dev;
}
EXPORT_SYMBOL(sunxi_cpu_comm_get_device);

int sunxi_cpu_comm_hdmi_detect_exported(void)
{
	if (!g_cpu_comm_dev) {
		return -ENODEV;
	}
	return sunxi_cpu_comm_hdmi_detect(g_cpu_comm_dev);
}
EXPORT_SYMBOL(sunxi_cpu_comm_hdmi_detect_exported);

int sunxi_cpu_comm_hdmi_read_edid_exported(u8 *edid_buffer, size_t buffer_size)
{
	if (!g_cpu_comm_dev) {
		return -ENODEV;
	}
	return sunxi_cpu_comm_hdmi_read_edid(g_cpu_comm_dev, edid_buffer, buffer_size);
}
EXPORT_SYMBOL(sunxi_cpu_comm_hdmi_read_edid_exported);

int sunxi_cpu_comm_hdmi_get_format_exported(struct hdmi_format_info *format)
{
	if (!g_cpu_comm_dev) {
		return -ENODEV;
	}
	return sunxi_cpu_comm_hdmi_get_format(g_cpu_comm_dev, format);
}
EXPORT_SYMBOL(sunxi_cpu_comm_hdmi_get_format_exported);

module_platform_driver(sunxi_cpu_comm_driver);

MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_DESCRIPTION("Allwinner SUNXI CPU Communication Driver for ARM-MIPS coordination");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);