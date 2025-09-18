// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner SUNXI TV Top-level Controller Driver
 * Top-level TV subsystem controller for H713 SoC
 *
 * This driver provides TV subsystem coordination, HDMI input routing,
 * and display pipeline control for the HY300 projector.
 *
 * Copyright (C) 2025 HY300 Linux Porting Project
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <media/v4l2-common.h>

#define DRIVER_NAME "sunxi-tvtop"

/* TV Top register offsets (from 0x5700000) */
#define TVTOP_CTRL_REG          0x0000  /* Top-level control */
#define TVTOP_STATUS_REG        0x0004  /* Status register */
#define TVTOP_IRQ_EN_REG        0x0008  /* Interrupt enable */
#define TVTOP_IRQ_STATUS_REG    0x000C  /* Interrupt status */
#define TVTOP_INPUT_SEL_REG     0x0010  /* Input selection */
#define TVTOP_OUTPUT_CTRL_REG   0x0014  /* Output control */
#define TVTOP_CAPTURE_CTRL_REG  0x0018  /* Capture control */
#define TVTOP_DISPLAY_CTRL_REG  0x001C  /* Display control */
#define TVTOP_WIDTH_REG         0x0020  /* Video width */
#define TVTOP_HEIGHT_REG        0x0024  /* Video height */
#define TVTOP_FORMAT_REG        0x0028  /* Video format */
#define TVTOP_BUFFER_ADDR_REG   0x002C  /* Buffer address */
#define TVTOP_BUFFER_SIZE_REG   0x0030  /* Buffer size */

/* Control register bits */
#define TVTOP_CTRL_ENABLE       BIT(0)  /* Enable TV top */
#define TVTOP_CTRL_RESET        BIT(1)  /* Reset TV subsystem */
#define TVTOP_CTRL_HDMI_EN      BIT(2)  /* Enable HDMI input */
#define TVTOP_CTRL_DISP_EN      BIT(3)  /* Enable display output */
#define TVTOP_CTRL_CAPTURE_EN   BIT(4)  /* Enable capture */

/* Status register bits */
#define TVTOP_STATUS_READY      BIT(0)  /* TV top ready */
#define TVTOP_STATUS_BUSY       BIT(1)  /* Processing busy */
#define TVTOP_STATUS_HDMI_DET   BIT(2)  /* HDMI detected */
#define TVTOP_STATUS_SYNC_LOCK  BIT(3)  /* Sync locked */

/* Input selection values */
#define TVTOP_INPUT_HDMI1       0x01    /* HDMI input 1 */
#define TVTOP_INPUT_HDMI2       0x02    /* HDMI input 2 */
#define TVTOP_INPUT_HDMI3       0x03    /* HDMI input 3 */
#define TVTOP_INPUT_MASK        0x0F    /* Input mask */

/* Capture control bits */
#define TVTOP_CAPTURE_ENABLE    BIT(0)  /* Enable capture */
#define TVTOP_CAPTURE_START     BIT(1)  /* Start capture */
#define TVTOP_CAPTURE_STOP      BIT(2)  /* Stop capture */

/* Format control values */
#define TVTOP_FORMAT_YUV422     0x01    /* YUV 4:2:2 */
#define TVTOP_FORMAT_YUV420     0x02    /* YUV 4:2:0 */
#define TVTOP_FORMAT_RGB888     0x03    /* RGB 8:8:8 */
#define TVTOP_FORMAT_RGB565     0x04    /* RGB 5:6:5 */

/* Interrupt bits */
#define TVTOP_IRQ_FRAME_DONE    BIT(0)  /* Frame capture done */
#define TVTOP_IRQ_HDMI_CHANGE   BIT(1)  /* HDMI status change */
#define TVTOP_IRQ_ERROR         BIT(2)  /* Error occurred */
#define TVTOP_IRQ_OVERFLOW      BIT(3)  /* Buffer overflow */

/* TVTOP device structure */
struct sunxi_tvtop_dev {
	struct device *dev;
	struct platform_device *pdev;
	
	/* Hardware resources */
	void __iomem *base;
	int irq;
	
	/* Clocks */
	struct clk *bus_clk;
	struct clk *capture_clk;
	struct clk *dma_clk;
	
	/* Reset controls */
	struct reset_control *tvcap_reset;
	struct reset_control *disp_reset;
	
	/* TV subsystem state */
	bool enabled;
	bool hdmi_input_enabled;
	bool capture_enabled;
	int current_input;
	
	/* Video parameters */
	u32 width;
	u32 height;
	u32 format;
	
	/* CPU communication interface */
	struct device *cpu_comm_dev;
	
	/* Statistics */
	atomic_t frames_captured;
	atomic_t errors_count;
};

/* Global TVTOP device instance */
static struct sunxi_tvtop_dev *g_tvtop_dev = NULL;

/* Forward declarations */
static irqreturn_t sunxi_tvtop_irq_handler(int irq, void *dev_id);

/*
 * Get TVTOP device instance for other drivers
 */
struct sunxi_tvtop_dev *sunxi_tvtop_get_device(void)
{
	return g_tvtop_dev;
}
EXPORT_SYMBOL(sunxi_tvtop_get_device);

/*
 * Enable HDMI input on specified input port
 */
int sunxi_tvtop_enable_hdmi_input(struct sunxi_tvtop_dev *tvtop, int input_id)
{
	u32 input_sel;
	int ret;
	
	if (!tvtop || !tvtop->enabled) {
		return -ENODEV;
	}
	
	if (input_id < 1 || input_id > 3) {
		dev_err(tvtop->dev, "Invalid HDMI input ID: %d\n", input_id);
		return -EINVAL;
	}
	
	dev_info(tvtop->dev, "Enabling HDMI input %d\n", input_id);
	
	/* Enable clocks for TV capture */
	ret = clk_prepare_enable(tvtop->capture_clk);
	if (ret) {
		dev_err(tvtop->dev, "Failed to enable capture clock: %d\n", ret);
		return ret;
	}
	
	ret = clk_prepare_enable(tvtop->dma_clk);
	if (ret) {
		dev_err(tvtop->dev, "Failed to enable DMA clock: %d\n", ret);
		clk_disable_unprepare(tvtop->capture_clk);
		return ret;
	}
	
	/* Release reset for TV capture */
	ret = reset_control_deassert(tvtop->tvcap_reset);
	if (ret) {
		dev_err(tvtop->dev, "Failed to deassert TV capture reset: %d\n", ret);
		goto err_clocks;
	}
	
	/* Configure input selection */
	input_sel = readl(tvtop->base + TVTOP_INPUT_SEL_REG);
	input_sel &= ~TVTOP_INPUT_MASK;
	input_sel |= (input_id & TVTOP_INPUT_MASK);
	writel(input_sel, tvtop->base + TVTOP_INPUT_SEL_REG);
	
	/* Enable HDMI input in control register */
	writel(TVTOP_CTRL_ENABLE | TVTOP_CTRL_HDMI_EN, 
	       tvtop->base + TVTOP_CTRL_REG);
	
	/* Wait for HDMI detection */
	msleep(100);
	
	tvtop->hdmi_input_enabled = true;
	tvtop->current_input = input_id;
	
	dev_info(tvtop->dev, "HDMI input %d enabled successfully\n", input_id);
	return 0;

err_clocks:
	clk_disable_unprepare(tvtop->dma_clk);
	clk_disable_unprepare(tvtop->capture_clk);
	return ret;
}
EXPORT_SYMBOL(sunxi_tvtop_enable_hdmi_input);

/*
 * Setup capture path with specified V4L2 format
 */
int sunxi_tvtop_setup_capture_path(struct sunxi_tvtop_dev *tvtop, 
				   struct v4l2_format *format)
{
	u32 capture_ctrl, format_ctrl;
	
	if (!tvtop || !format) {
		return -EINVAL;
	}
	
	if (!tvtop->hdmi_input_enabled) {
		dev_err(tvtop->dev, "HDMI input not enabled\n");
		return -ENODEV;
	}
	
	dev_info(tvtop->dev, "Setting up capture path: %ux%u, format=0x%x\n",
		 format->fmt.pix.width, format->fmt.pix.height,
		 format->fmt.pix.pixelformat);
	
	/* Configure video format */
	switch (format->fmt.pix.pixelformat) {
	case V4L2_PIX_FMT_YUYV:
		format_ctrl = TVTOP_FORMAT_YUV422;
		break;
	case V4L2_PIX_FMT_YUV420:
		format_ctrl = TVTOP_FORMAT_YUV420;
		break;
	case V4L2_PIX_FMT_RGB24:
		format_ctrl = TVTOP_FORMAT_RGB888;
		break;
	case V4L2_PIX_FMT_RGB565:
		format_ctrl = TVTOP_FORMAT_RGB565;
		break;
	default:
		dev_err(tvtop->dev, "Unsupported pixel format: 0x%x\n",
			format->fmt.pix.pixelformat);
		return -EINVAL;
	}
	
	/* Set video dimensions */
	writel(format->fmt.pix.width, tvtop->base + TVTOP_WIDTH_REG);
	writel(format->fmt.pix.height, tvtop->base + TVTOP_HEIGHT_REG);
	writel(format_ctrl, tvtop->base + TVTOP_FORMAT_REG);
	
	/* Configure capture control */
	capture_ctrl = TVTOP_CAPTURE_ENABLE;
	writel(capture_ctrl, tvtop->base + TVTOP_CAPTURE_CTRL_REG);
	
	/* Update device state */
	tvtop->width = format->fmt.pix.width;
	tvtop->height = format->fmt.pix.height;
	tvtop->format = format->fmt.pix.pixelformat;
	tvtop->capture_enabled = true;
	
	dev_info(tvtop->dev, "Capture path configured successfully\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_tvtop_setup_capture_path);

/*
 * Start video capture
 */
int sunxi_tvtop_start_capture(struct sunxi_tvtop_dev *tvtop)
{
	u32 capture_ctrl;
	
	if (!tvtop || !tvtop->capture_enabled) {
		return -ENODEV;
	}
	
	dev_info(tvtop->dev, "Starting video capture\n");
	
	/* Start capture */
	capture_ctrl = readl(tvtop->base + TVTOP_CAPTURE_CTRL_REG);
	capture_ctrl |= TVTOP_CAPTURE_START;
	writel(capture_ctrl, tvtop->base + TVTOP_CAPTURE_CTRL_REG);
	
	/* Enable capture interrupts */
	writel(TVTOP_IRQ_FRAME_DONE | TVTOP_IRQ_ERROR | TVTOP_IRQ_OVERFLOW,
	       tvtop->base + TVTOP_IRQ_EN_REG);
	
	dev_info(tvtop->dev, "Video capture started\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_tvtop_start_capture);

/*
 * Stop video capture
 */
int sunxi_tvtop_stop_capture(struct sunxi_tvtop_dev *tvtop)
{
	u32 capture_ctrl;
	
	if (!tvtop) {
		return -ENODEV;
	}
	
	dev_info(tvtop->dev, "Stopping video capture\n");
	
	/* Disable capture interrupts */
	writel(0, tvtop->base + TVTOP_IRQ_EN_REG);
	
	/* Stop capture */
	capture_ctrl = readl(tvtop->base + TVTOP_CAPTURE_CTRL_REG);
	capture_ctrl |= TVTOP_CAPTURE_STOP;
	capture_ctrl &= ~TVTOP_CAPTURE_START;
	writel(capture_ctrl, tvtop->base + TVTOP_CAPTURE_CTRL_REG);
	
	dev_info(tvtop->dev, "Video capture stopped\n");
	return 0;
}
EXPORT_SYMBOL(sunxi_tvtop_stop_capture);

/*
 * Get HDMI detection status
 */
bool sunxi_tvtop_is_hdmi_detected(struct sunxi_tvtop_dev *tvtop)
{
	u32 status;
	
	if (!tvtop || !tvtop->enabled) {
		return false;
	}
	
	status = readl(tvtop->base + TVTOP_STATUS_REG);
	return !!(status & TVTOP_STATUS_HDMI_DET);
}
EXPORT_SYMBOL(sunxi_tvtop_is_hdmi_detected);

/*
 * Get sync lock status
 */
bool sunxi_tvtop_is_sync_locked(struct sunxi_tvtop_dev *tvtop)
{
	u32 status;
	
	if (!tvtop || !tvtop->enabled) {
		return false;
	}
	
	status = readl(tvtop->base + TVTOP_STATUS_REG);
	return !!(status & TVTOP_STATUS_SYNC_LOCK);
}
EXPORT_SYMBOL(sunxi_tvtop_is_sync_locked);

/*
 * Interrupt handler for TVTOP
 */
static irqreturn_t sunxi_tvtop_irq_handler(int irq, void *dev_id)
{
	struct sunxi_tvtop_dev *tvtop = dev_id;
	u32 irq_status;
	
	/* Read interrupt status */
	irq_status = readl(tvtop->base + TVTOP_IRQ_STATUS_REG);
	
	if (irq_status & TVTOP_IRQ_FRAME_DONE) {
		/* Frame capture completed */
		dev_dbg(tvtop->dev, "Frame capture completed\n");
		atomic_inc(&tvtop->frames_captured);
		irq_status &= ~TVTOP_IRQ_FRAME_DONE;
	}
	
	if (irq_status & TVTOP_IRQ_HDMI_CHANGE) {
		/* HDMI status changed */
		dev_info(tvtop->dev, "HDMI status changed\n");
		irq_status &= ~TVTOP_IRQ_HDMI_CHANGE;
	}
	
	if (irq_status & TVTOP_IRQ_ERROR) {
		/* Error occurred */
		dev_err(tvtop->dev, "TV capture error occurred\n");
		atomic_inc(&tvtop->errors_count);
		irq_status &= ~TVTOP_IRQ_ERROR;
	}
	
	if (irq_status & TVTOP_IRQ_OVERFLOW) {
		/* Buffer overflow */
		dev_warn(tvtop->dev, "Capture buffer overflow\n");
		atomic_inc(&tvtop->errors_count);
		irq_status &= ~TVTOP_IRQ_OVERFLOW;
	}
	
	/* Clear processed interrupts */
	writel(irq_status, tvtop->base + TVTOP_IRQ_STATUS_REG);
	
	return IRQ_HANDLED;
}

/*
 * Platform driver probe
 */
static int sunxi_tvtop_probe(struct platform_device *pdev)
{
	struct sunxi_tvtop_dev *tvtop;
	struct resource *res;
	int ret;
	
	dev_info(&pdev->dev, "Probing SUNXI TV Top driver\n");
	
	tvtop = devm_kzalloc(&pdev->dev, sizeof(*tvtop), GFP_KERNEL);
	if (!tvtop) {
		return -ENOMEM;
	}
	
	tvtop->dev = &pdev->dev;
	tvtop->pdev = pdev;
	platform_set_drvdata(pdev, tvtop);
	
	/* Get hardware resources */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	tvtop->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(tvtop->base)) {
		dev_err(&pdev->dev, "Failed to map registers\n");
		return PTR_ERR(tvtop->base);
	}
	
	tvtop->irq = platform_get_irq(pdev, 0);
	if (tvtop->irq < 0) {
		dev_err(&pdev->dev, "Failed to get IRQ\n");
		return tvtop->irq;
	}
	
	/* Get clocks */
	tvtop->bus_clk = devm_clk_get(&pdev->dev, "bus");
	if (IS_ERR(tvtop->bus_clk)) {
		dev_err(&pdev->dev, "Failed to get bus clock\n");
		return PTR_ERR(tvtop->bus_clk);
	}
	
	tvtop->capture_clk = devm_clk_get(&pdev->dev, "capture");
	if (IS_ERR(tvtop->capture_clk)) {
		dev_err(&pdev->dev, "Failed to get capture clock\n");
		return PTR_ERR(tvtop->capture_clk);
	}
	
	tvtop->dma_clk = devm_clk_get(&pdev->dev, "dma");
	if (IS_ERR(tvtop->dma_clk)) {
		dev_err(&pdev->dev, "Failed to get DMA clock\n");
		return PTR_ERR(tvtop->dma_clk);
	}
	
	/* Get reset controls */
	tvtop->tvcap_reset = devm_reset_control_get(&pdev->dev, "tvcap");
	if (IS_ERR(tvtop->tvcap_reset)) {
		dev_err(&pdev->dev, "Failed to get TV capture reset\n");
		return PTR_ERR(tvtop->tvcap_reset);
	}
	
	tvtop->disp_reset = devm_reset_control_get(&pdev->dev, "disp");
	if (IS_ERR(tvtop->disp_reset)) {
		dev_err(&pdev->dev, "Failed to get display reset\n");
		return PTR_ERR(tvtop->disp_reset);
	}
	
	/* Enable bus clock */
	ret = clk_prepare_enable(tvtop->bus_clk);
	if (ret) {
		dev_err(&pdev->dev, "Failed to enable bus clock: %d\n", ret);
		return ret;
	}
	
	/* Request IRQ */
	ret = devm_request_irq(&pdev->dev, tvtop->irq, sunxi_tvtop_irq_handler,
			       IRQF_SHARED, DRIVER_NAME, tvtop);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request IRQ %d: %d\n", tvtop->irq, ret);
		goto err_clk;
	}
	
	/* Initialize hardware */
	writel(TVTOP_CTRL_RESET, tvtop->base + TVTOP_CTRL_REG);
	msleep(10);
	writel(TVTOP_CTRL_ENABLE, tvtop->base + TVTOP_CTRL_REG);
	
	/* Initialize statistics */
	atomic_set(&tvtop->frames_captured, 0);
	atomic_set(&tvtop->errors_count, 0);
	
	/* Set device state */
	tvtop->enabled = true;
	g_tvtop_dev = tvtop;
	
	dev_info(&pdev->dev, "SUNXI TV Top driver initialized successfully\n");
	return 0;

err_clk:
	clk_disable_unprepare(tvtop->bus_clk);
	return ret;
}

/*
 * Platform driver remove
 */
static int sunxi_tvtop_remove(struct platform_device *pdev)
{
	struct sunxi_tvtop_dev *tvtop = platform_get_drvdata(pdev);
	
	if (tvtop) {
		g_tvtop_dev = NULL;
		tvtop->enabled = false;
		
		/* Stop capture if running */
		if (tvtop->capture_enabled) {
			sunxi_tvtop_stop_capture(tvtop);
		}
		
		/* Disable hardware */
		writel(0, tvtop->base + TVTOP_IRQ_EN_REG);
		writel(TVTOP_CTRL_RESET, tvtop->base + TVTOP_CTRL_REG);
		
		/* Disable clocks if enabled */
		if (tvtop->hdmi_input_enabled) {
			clk_disable_unprepare(tvtop->dma_clk);
			clk_disable_unprepare(tvtop->capture_clk);
		}
		
		clk_disable_unprepare(tvtop->bus_clk);
		
		dev_info(&pdev->dev, "SUNXI TV Top driver removed\n");
		dev_info(&pdev->dev, "Statistics: frames=%d errors=%d\n",
			 atomic_read(&tvtop->frames_captured),
			 atomic_read(&tvtop->errors_count));
	}
	
	return 0;
}

/* Device tree compatibility */
static const struct of_device_id sunxi_tvtop_dt_ids[] = {
	{ .compatible = "allwinner,sunxi-tvtop" },
	{ .compatible = "allwinner,sun50i-h713-tvtop" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_tvtop_dt_ids);

static struct platform_driver sunxi_tvtop_driver = {
	.probe  = sunxi_tvtop_probe,
	.remove = sunxi_tvtop_remove,
	.driver = {
		.name           = DRIVER_NAME,
		.of_match_table = sunxi_tvtop_dt_ids,
	},
};

module_platform_driver(sunxi_tvtop_driver);

MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_DESCRIPTION("Allwinner SUNXI TV Top-level Controller Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" DRIVER_NAME);