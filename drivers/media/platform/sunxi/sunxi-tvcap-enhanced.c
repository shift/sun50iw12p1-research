/*
 * Allwinner H713 TV Capture V4L2 Driver - Enhanced Version
 * 
 * Enhanced with IOMMU integration and display output pipeline based on factory analysis.
 * Implements both HDMI input capture and display output with MIPS co-processor coordination.
 *
 * Copyright (C) 2025 HY300 Linux Porting Project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/iommu.h>
#include <linux/dma-iommu.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-dev.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>
#include <media/v4l2-mem2mem.h>

/* HDMI format information structure - from sunxi-cpu-comm.c */
struct hdmi_format_info {
	u32 width;
	u32 height;
	u32 refresh_rate;
	u32 pixel_format;
	u32 color_space;
	u32 quantization;
	u32 transfer_func;
};

#define SUNXI_TVCAP_NAME "sunxi-tvcap-enhanced"
#define SUNXI_TVCAP_VERSION KERNEL_VERSION(2, 0, 0)

/* TV Capture Hardware Registers (enhanced with display output) */
#define TVTOP_BASE_OFFSET       0x0000  /* tvtop@5700000 */
#define TVCAP_BASE_OFFSET       0x1100000 /* tvcap@6800000 */

/* TV TOP Control Registers */
#define TVTOP_CTRL_REG          0x0000
#define TVTOP_STATUS_REG        0x0004
#define TVTOP_IRQ_EN_REG        0x0008
#define TVTOP_IRQ_STATUS_REG    0x000c
#define TVTOP_FORMAT_REG        0x0010
#define TVTOP_RESOLUTION_REG    0x0014

/* Enhanced Display Output Pipeline Registers */
#define TVTOP_DISP_CTRL_REG     0x0044   /* Display output control */
#define TVTOP_DISP_FORMAT_REG   0x0048   /* Display output format */
#define TVTOP_DISP_SIZE_REG     0x004c   /* Display output size */
#define TVTOP_DISP_ADDR_REG     0x0050   /* Display buffer address */
#define TVTOP_DISP_STATUS_REG   0x0054   /* Display pipeline status */
#define TVTOP_DISP_SYNC_REG     0x0058   /* Display synchronization */

/* TV TOP Control Register Bit Definitions */
#define TVTOP_CTRL_ENABLE       BIT(0)    /* Enable TV TOP subsystem */
#define TVTOP_CTRL_CAPTURE_EN   BIT(1)    /* Enable capture function */
#define TVTOP_CTRL_HDMI_EN      BIT(2)    /* Enable HDMI input */
#define TVTOP_CTRL_DMA_EN       BIT(3)    /* Enable DMA transfers */
#define TVTOP_CTRL_AUTO_FORMAT  BIT(4)    /* Auto format detection */
#define TVTOP_CTRL_DISP_EN      BIT(5)    /* Enable display output */
#define TVTOP_CTRL_RESET        BIT(31)   /* Software reset */

/* Display Pipeline Status Bits */
#define TVTOP_DISP_STATUS_ACTIVE    BIT(0)  /* Display pipeline active */
#define TVTOP_DISP_STATUS_READY     BIT(1)  /* Display ready for next frame */
#define TVTOP_DISP_STATUS_ERROR     BIT(2)  /* Display pipeline error */

/* Enhanced TV-specific clock indices */
enum tvcap_clks {
	TVCAP_CLK_BUS_TVCAP = 0,    /* Bus clock for TV capture */
	TVCAP_CLK_CAP_300M,         /* 300MHz capture clock */
	TVCAP_CLK_VINCAP_DMA,       /* DMA clock for video input capture */
	TVCAP_CLK_TVCAP,            /* Main TV capture clock */
	TVCAP_CLK_TVE,              /* TV encoder clock */
	TVCAP_CLK_DEMOD,            /* Demodulator clock */
	TVCAP_CLK_TVTOP,            /* TV TOP subsystem clock */
	TVCAP_CLK_DISP_TOP,         /* Display top-level clock */
	TVCAP_CLK_DISP_OUT,         /* Display output clock */
	TVCAP_CLK_VIDEO_PLL,        /* Video PLL clock */
	TVCAP_CLK_COUNT
};

/* Enhanced reset indices */
enum tvcap_resets {
	TVCAP_RST_BUS_DISP = 0,
	TVCAP_RST_BUS_TVCAP,
	TVCAP_RST_BUS_DEMOD,
	TVCAP_RST_DISP_TOP,         /* Display top-level reset */
	TVCAP_RST_VIDEO_OUT,        /* Video output reset */
	TVCAP_RST_COUNT
};

/* Hardware capability flags */
#define TVCAP_HW_CAP_CAPTURE        BIT(0)  /* Basic capture capability */
#define TVCAP_HW_CAP_DISPLAY        BIT(1)  /* Display output capability */
#define TVCAP_HW_CAP_IOMMU          BIT(2)  /* IOMMU support available */
#define TVCAP_HW_CAP_ZERO_COPY      BIT(3)  /* Zero-copy buffer sharing */

/* HDMI input definitions */
#define TVCAP_INPUT_HDMI	0
#define TVCAP_NUM_INPUTS	1

/* Enhanced Device structure with dual capture/output support */
struct sunxi_tvcap_dev {
	struct v4l2_device v4l2_dev;
	struct video_device video_dev_cap;    /* Capture device */
	struct video_device video_dev_out;    /* Output device */
	struct vb2_queue queue_cap;           /* Capture queue */
	struct vb2_queue queue_out;           /* Output queue */
	struct v4l2_ctrl_handler ctrl_handler;
	struct device *dev;

	/* Hardware resources */
	void __iomem *regs;
	
	/* IOMMU integration for enhanced buffer management */
	struct iommu_domain *iommu_domain;
	bool iommu_enabled;
	dma_addr_t iommu_base;
	
	struct clk_bulk_data clks[TVCAP_CLK_COUNT];
	struct reset_control_bulk_data resets[TVCAP_RST_COUNT];
	int irq;
	
	/* Hardware capabilities */
	u32 hw_capabilities;
	u32 hw_version;

	/* Video format and state */
	struct v4l2_format format_cap;        /* Capture format */
	struct v4l2_format format_out;        /* Output format */
	struct v4l2_input input;
	bool hdmi_connected;
	unsigned int current_input;           /* Current selected input */
	bool signal_detected;
	bool streaming_cap;                   /* Capture streaming state */
	bool streaming_out;                   /* Output streaming state */

	/* Display output state */
	bool display_enabled;
	
	/* Hardware state */
	bool tvtop_initialized;
	u32 current_resolution;
	u32 current_format;

	/* Synchronization */
	struct mutex lock;
	spinlock_t irq_lock;

	/* Buffer management for both capture and output */
	struct list_head buf_list_cap;        /* Capture buffers */
	struct list_head buf_list_out;        /* Output buffers */
	u32 sequence;
};

/* Enhanced Buffer structure with IOMMU and zero-copy support */
struct tvcap_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	dma_addr_t dma_addr;                  /* DMA address for direct mapping */
	dma_addr_t iommu_addr;                /* IOMMU virtual address */
	struct dma_buf *dmabuf;               /* DMA-BUF for zero-copy sharing */
	u32 buffer_flags;                     /* Buffer state flags */
	bool is_output;                       /* True for output buffers */
};

/* Display output configuration */
struct tvcap_display_config {
	u32 width;
	u32 height;
	u32 format;
	u32 refresh_rate;
	bool active;
};

/* Supported video formats */
static const struct tvcap_format {
	u32 fourcc;
	u32 depth;
	u32 planes;
	u32 tvtop_format;
	const char *name;
} formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_YUYV,
		.depth = 16,
		.planes = 1,
		.tvtop_format = 0x04,
		.name = "YUV 4:2:2 YUYV",
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.depth = 12,
		.planes = 3,
		.tvtop_format = 0x02,
		.name = "YUV 4:2:0 Planar",
	},
};

/*
 * Enhanced register access functions
 */
static inline u32 tvtop_read(struct sunxi_tvcap_dev *tvcap, u32 reg)
{
	return readl(tvcap->regs + reg);
}

static inline void tvtop_write(struct sunxi_tvcap_dev *tvcap, u32 reg, u32 val)
{
	writel(val, tvcap->regs + reg);
}

static inline void tvtop_set_bits(struct sunxi_tvcap_dev *tvcap, u32 reg, u32 bits)
{
	u32 val = tvtop_read(tvcap, reg);
	tvtop_write(tvcap, reg, val | bits);
}

static inline void tvtop_clear_bits(struct sunxi_tvcap_dev *tvcap, u32 reg, u32 bits)
{
	u32 val = tvtop_read(tvcap, reg);
	tvtop_write(tvcap, reg, val & ~bits);
}

/* External HDMI functions from sunxi-cpu-comm.c */
extern int sunxi_cpu_comm_hdmi_detect_exported(void);
extern int sunxi_cpu_comm_hdmi_read_edid_exported(u8 *edid_buffer, size_t buffer_size);
extern int sunxi_cpu_comm_hdmi_get_format_exported(struct hdmi_format_info *format);

/*
 * IOMMU Integration Functions
 */
static int tvcap_iommu_init(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	struct iommu_domain *domain;
	int ret;

	dev_info(dev, "Initializing TVCAP IOMMU integration\n");

	/* Check if IOMMU is available */
	if (!iommu_present(dev->bus)) {
		dev_info(dev, "IOMMU not available, using standard DMA\n");
		tvcap->iommu_enabled = false;
		return 0;
	}

	/* Create IOMMU domain */
	domain = iommu_domain_alloc(dev->bus);
	if (!domain) {
		dev_err(dev, "Failed to allocate IOMMU domain\n");
		return -ENOMEM;
	}

	/* Attach device to IOMMU domain */
	ret = iommu_attach_device(domain, dev);
	if (ret) {
		dev_err(dev, "Failed to attach IOMMU domain: %d\n", ret);
		iommu_domain_free(domain);
		return ret;
	}

	tvcap->iommu_domain = domain;
	tvcap->iommu_enabled = true;
	tvcap->iommu_base = 0x40000000;
	tvcap->hw_capabilities |= TVCAP_HW_CAP_IOMMU;

	dev_info(dev, "TVCAP IOMMU initialized successfully\n");
	return 0;
}

static void tvcap_iommu_cleanup(struct sunxi_tvcap_dev *tvcap)
{
	if (!tvcap->iommu_enabled || !tvcap->iommu_domain)
		return;

	iommu_detach_device(tvcap->iommu_domain, tvcap->dev);
	iommu_domain_free(tvcap->iommu_domain);
	tvcap->iommu_domain = NULL;
	tvcap->iommu_enabled = false;
}

/*
 * Enhanced Clock Management
 */
static int tvcap_clocks_init_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	int ret, i;

	static const char * const clk_names[TVCAP_CLK_COUNT] = {
		"clk_bus_tvcap",     /* Bus interface clock */
		"cap_300m",          /* 300MHz capture clock */
		"vincap_dma_clk",    /* DMA transfer clock */
		"tvcap",             /* Main TV capture clock */
		"tve",               /* TV encoder clock */
		"demod",             /* Demodulator clock */
		"tvtop",             /* TV TOP subsystem clock */
		"disp_top",          /* Display top-level clock */
		"disp_out",          /* Display output clock */
		"video_pll"          /* Video PLL clock */
	};

	for (i = 0; i < TVCAP_CLK_COUNT; i++)
		tvcap->clks[i].id = clk_names[i];

	ret = devm_clk_bulk_get(dev, TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(dev, "Failed to get enhanced clocks: %d\n", ret);
		return ret;
	}

	dev_info(dev, "Enhanced clocks initialized: %d clocks\n", TVCAP_CLK_COUNT);
	return 0;
}

static int tvcap_clocks_enable_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	ret = clk_bulk_prepare_enable(TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(tvcap->dev, "Failed to enable enhanced clocks: %d\n", ret);
		return ret;
	}

	/* Set enhanced clock rates for display output */
	if (tvcap->hw_capabilities & TVCAP_HW_CAP_DISPLAY) {
		if (!IS_ERR_OR_NULL(tvcap->clks[TVCAP_CLK_DISP_TOP].clk))
			clk_set_rate(tvcap->clks[TVCAP_CLK_DISP_TOP].clk, 300000000);
		if (!IS_ERR_OR_NULL(tvcap->clks[TVCAP_CLK_DISP_OUT].clk))
			clk_set_rate(tvcap->clks[TVCAP_CLK_DISP_OUT].clk, 148500000);
		if (!IS_ERR_OR_NULL(tvcap->clks[TVCAP_CLK_VIDEO_PLL].clk))
			clk_set_rate(tvcap->clks[TVCAP_CLK_VIDEO_PLL].clk, 297000000);
	}

	dev_info(tvcap->dev, "Enhanced clocks enabled successfully\n");
	return 0;
}

/*
 * Enhanced Reset Management
 */
static int tvcap_resets_init_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	int ret, i;

	static const char * const reset_names[TVCAP_RST_COUNT] = {
		"rst_bus_disp",      /* Display subsystem reset */
		"rst_bus_tvcap",     /* TV capture bus reset */
		"rst_bus_demod",     /* Demodulator reset */
		"rst_disp_top",      /* Display top-level reset */
		"rst_video_out"      /* Video output reset */
	};

	for (i = 0; i < TVCAP_RST_COUNT; i++)
		tvcap->resets[i].id = reset_names[i];

	ret = devm_reset_control_bulk_get_optional_shared(dev, TVCAP_RST_COUNT, tvcap->resets);
	if (ret) {
		dev_err(dev, "Failed to get enhanced reset controls: %d\n", ret);
		return ret;
	}

	dev_info(dev, "Enhanced reset controls initialized\n");
	return 0;
}

/*
 * Display Output Pipeline Implementation
 */
static int tvcap_display_configure(struct sunxi_tvcap_dev *tvcap, 
				   struct tvcap_display_config *config)
{
	u32 ctrl_reg, size_reg;

	dev_dbg(tvcap->dev, "Configuring display pipeline: %dx%d\n",
		config->width, config->height);

	tvtop_write(tvcap, TVTOP_DISP_FORMAT_REG, config->format);
	
	size_reg = (config->height << 16) | config->width;
	tvtop_write(tvcap, TVTOP_DISP_SIZE_REG, size_reg);

	ctrl_reg = tvtop_read(tvcap, TVTOP_CTRL_REG);
	if (config->active) {
		ctrl_reg |= TVTOP_CTRL_DISP_EN;
	} else {
		ctrl_reg &= ~TVTOP_CTRL_DISP_EN;
	}
	tvtop_write(tvcap, TVTOP_CTRL_REG, ctrl_reg);

	return 0;
}

static int tvcap_display_start(struct sunxi_tvcap_dev *tvcap)
{
	u32 status;

	tvtop_set_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_DISP_EN);
	
	/* Wait for display to become ready */
	msleep(10);
	
	status = tvtop_read(tvcap, TVTOP_DISP_STATUS_REG);
	if (!(status & TVTOP_DISP_STATUS_READY)) {
		dev_warn(tvcap->dev, "Display pipeline may not be ready, continuing anyway\n");
	}

	tvcap->display_enabled = true;
	dev_info(tvcap->dev, "Display pipeline started\n");
	return 0;
}

static void tvcap_display_stop(struct sunxi_tvcap_dev *tvcap)
{
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_DISP_EN);
	tvtop_write(tvcap, TVTOP_DISP_ADDR_REG, 0);
	tvcap->display_enabled = false;
}

/*
 * Hardware Detection and Initialization
 */
static int tvcap_detect_capabilities(struct sunxi_tvcap_dev *tvcap)
{
	u32 version_reg;

	version_reg = tvtop_read(tvcap, 0x00fc); /* Debug/version register */
	tvcap->hw_version = version_reg;
	tvcap->hw_capabilities = TVCAP_HW_CAP_CAPTURE;

	/* Detect display output capability */
	if (tvtop_read(tvcap, TVTOP_DISP_CTRL_REG) != 0xFFFFFFFF) {
		tvcap->hw_capabilities |= TVCAP_HW_CAP_DISPLAY;
		dev_info(tvcap->dev, "Display output capability detected\n");
	}

	/* Enable zero-copy if IOMMU and display both available */
	if ((tvcap->hw_capabilities & TVCAP_HW_CAP_IOMMU) &&
	    (tvcap->hw_capabilities & TVCAP_HW_CAP_DISPLAY)) {
		tvcap->hw_capabilities |= TVCAP_HW_CAP_ZERO_COPY;
		dev_info(tvcap->dev, "Zero-copy buffer sharing enabled\n");
	}

	dev_info(tvcap->dev, "Hardware capabilities: 0x%08x\n", tvcap->hw_capabilities);
	return 0;
}

static int tvcap_hw_init_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	/* Initialize enhanced clocks */
	ret = tvcap_clocks_enable_enhanced(tvcap);
	if (ret)
		return ret;

	/* Reset hardware */
	ret = reset_control_bulk_assert(TVCAP_RST_COUNT, tvcap->resets);
	if (ret) {
		dev_warn(tvcap->dev, "Failed to assert resets: %d, continuing\n", ret);
	}
	
	usleep_range(10, 20);
	
	ret = reset_control_bulk_deassert(TVCAP_RST_COUNT, tvcap->resets);
	if (ret) {
		dev_warn(tvcap->dev, "Failed to deassert resets: %d, continuing\n", ret);
	}
	
	usleep_range(100, 200);

	/* Detect hardware capabilities */
	ret = tvcap_detect_capabilities(tvcap);
	if (ret)
		return ret;

	/* Initialize TVTOP subsystem */
	tvtop_write(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_ENABLE | TVTOP_CTRL_AUTO_FORMAT);
	
	tvcap->tvtop_initialized = true;
	dev_info(tvcap->dev, "Enhanced hardware initialized successfully\n");
	return 0;
}

/*
 * Enhanced VB2 Operations for Capture
 */
static int tvcap_queue_setup_cap(struct vb2_queue *vq, unsigned int *nbuffers,
			     unsigned int *nplanes, unsigned int sizes[],
			     struct device *alloc_devs[])
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix = &tvcap->format_cap.fmt.pix;
	
	if (*nbuffers < 2)
		*nbuffers = 2;
	else if (*nbuffers > 8)
		*nbuffers = 8;
	
	*nplanes = 1;
	sizes[0] = pix->sizeimage;
	
	return 0;
}

static int tvcap_buffer_prepare_cap(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_pix_format *pix = &tvcap->format_cap.fmt.pix;
	
	if (vb2_plane_size(vb, 0) < pix->sizeimage)
		return -EINVAL;
	
	vb2_set_plane_payload(vb, 0, pix->sizeimage);
	return 0;
}

static void tvcap_buffer_queue_cap(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct tvcap_buffer *buf = container_of(vbuf, struct tvcap_buffer, vb);
	unsigned long flags;
	
	buf->dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	buf->is_output = false;
	
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_add_tail(&buf->list, &tvcap->buf_list_cap);
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
}

static int tvcap_start_streaming_cap(struct vb2_queue *vq, unsigned int count)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	
	tvcap->streaming_cap = true;
	tvcap->sequence = 0;
	
	dev_info(tvcap->dev, "Enhanced capture streaming started\n");
	return 0;
}

static void tvcap_stop_streaming_cap(struct vb2_queue *vq)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct tvcap_buffer *buf, *tmp;
	unsigned long flags;
	
	tvcap->streaming_cap = false;
	
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_for_each_entry_safe(buf, tmp, &tvcap->buf_list_cap, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
}

static const struct vb2_ops tvcap_qops_cap = {
	.queue_setup = tvcap_queue_setup_cap,
	.buf_prepare = tvcap_buffer_prepare_cap,
	.buf_queue = tvcap_buffer_queue_cap,
	.start_streaming = tvcap_start_streaming_cap,
	.stop_streaming = tvcap_stop_streaming_cap,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/*
 * Enhanced VB2 Operations for Output
 */
static int tvcap_queue_setup_out(struct vb2_queue *vq, unsigned int *nbuffers,
			     unsigned int *nplanes, unsigned int sizes[],
			     struct device *alloc_devs[])
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix = &tvcap->format_out.fmt.pix;
	
	if (*nbuffers < 2)
		*nbuffers = 2;
	else if (*nbuffers > 8)
		*nbuffers = 8;
	
	*nplanes = 1;
	sizes[0] = pix->sizeimage;
	
	return 0;
}

static int tvcap_buffer_prepare_out(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_pix_format *pix = &tvcap->format_out.fmt.pix;
	
	if (vb2_plane_size(vb, 0) < pix->sizeimage)
		return -EINVAL;
	
	vb2_set_plane_payload(vb, 0, pix->sizeimage);
	return 0;
}

static void tvcap_buffer_queue_out(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct tvcap_buffer *buf = container_of(vbuf, struct tvcap_buffer, vb);
	unsigned long flags;
	
	buf->dma_addr = vb2_dma_contig_plane_dma_addr(vb, 0);
	buf->is_output = true;
	
	/* Queue for display if enabled */
	if (tvcap->display_enabled) {
		tvtop_write(tvcap, TVTOP_DISP_ADDR_REG, lower_32_bits(buf->dma_addr));
	}
	
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_add_tail(&buf->list, &tvcap->buf_list_out);
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
}

static int tvcap_start_streaming_out(struct vb2_queue *vq, unsigned int count)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct tvcap_display_config config;
	int ret;
	
	config.width = tvcap->format_out.fmt.pix.width;
	config.height = tvcap->format_out.fmt.pix.height;
	config.format = 0x04; /* YUYV */
	config.refresh_rate = 60;
	config.active = true;
	
	ret = tvcap_display_configure(tvcap, &config);
	if (ret)
		return ret;
	
	ret = tvcap_display_start(tvcap);
	if (ret)
		return ret;
	
	tvcap->streaming_out = true;
	dev_info(tvcap->dev, "Enhanced output streaming started\n");
	return 0;
}

static void tvcap_stop_streaming_out(struct vb2_queue *vq)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct tvcap_buffer *buf, *tmp;
	unsigned long flags;
	
	tvcap_display_stop(tvcap);
	tvcap->streaming_out = false;
	
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_for_each_entry_safe(buf, tmp, &tvcap->buf_list_out, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
}

static const struct vb2_ops tvcap_qops_out = {
	.queue_setup = tvcap_queue_setup_out,
	.buf_prepare = tvcap_buffer_prepare_out,
	.buf_queue = tvcap_buffer_queue_out,
	.start_streaming = tvcap_start_streaming_out,
	.stop_streaming = tvcap_stop_streaming_out,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/*
 * V4L2 IOCTL Operations
 */
static int tvcap_querycap(struct file *file, void *priv,
			  struct v4l2_capability *cap)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	
	strscpy(cap->driver, SUNXI_TVCAP_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Allwinner H713 TV Capture Enhanced", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), 
		 "platform:%s", dev_name(tvcap->dev));
	
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT |
			   V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	
	return 0;
}

static int tvcap_enum_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;
	
	f->pixelformat = formats[f->index].fourcc;
	strscpy(f->description, formats[f->index].name, sizeof(f->description));
	
	return 0;
}

static int tvcap_g_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	*f = tvcap->format_cap;
	return 0;
}

static int tvcap_s_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	
	if (vb2_is_busy(&tvcap->queue_cap))
		return -EBUSY;
	
	tvcap->format_cap = *f;
	return 0;
}

static int tvcap_try_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	
	pix->pixelformat = V4L2_PIX_FMT_YUYV;
	v4l_bound_align_image(&pix->width, 320, 1920, 1,
			      &pix->height, 240, 1080, 1, 0);
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->height * pix->bytesperline;
	pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
	pix->field = V4L2_FIELD_NONE;
	
	return 0;
}

/* Output format operations */
static int tvcap_enum_fmt_vid_out(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f)
{
	return tvcap_enum_fmt_vid_cap(file, priv, f);
}

static int tvcap_g_fmt_vid_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	*f = tvcap->format_out;
	return 0;
}

static int tvcap_s_fmt_vid_out(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	
	if (vb2_is_busy(&tvcap->queue_out))
		return -EBUSY;
	
	tvcap->format_out = *f;
	return 0;
}

static int tvcap_try_fmt_vid_out(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	return tvcap_try_fmt_vid_cap(file, priv, f);
}
/*
 * HDMI Input Management Functions (Tasks 3.4-3.5)
 */

static int tvcap_enum_input(struct file *file, void *priv, struct v4l2_input *input)
{
	struct sunxi_tvcap_dev *dev = video_drvdata(file);
	
	if (input->index >= TVCAP_NUM_INPUTS)
		return -EINVAL;
		
	switch (input->index) {
	case TVCAP_INPUT_HDMI:
		strscpy(input->name, "HDMI Input", sizeof(input->name));
		input->type = V4L2_INPUT_TYPE_CAMERA;
		input->capabilities = V4L2_IN_CAP_DV_TIMINGS | V4L2_IN_CAP_EDID;
		input->status = 0;
		
		/* Check HDMI connection status via MIPS communication */
		if (sunxi_cpu_comm_hdmi_detect_exported() > 0) {
			dev->hdmi_connected = true;
		} else {
			dev->hdmi_connected = false;
			input->status |= V4L2_IN_ST_NO_SIGNAL;
		}
		
		dev_dbg(dev->dev, "HDMI input enumerated, connected: %s\n",
			dev->hdmi_connected ? "yes" : "no");
		break;
		
	default:
		return -EINVAL;
	}
	
	return 0;
}

static int tvcap_g_input(struct file *file, void *priv, unsigned int *index)
{
	struct sunxi_tvcap_dev *dev = video_drvdata(file);
	
	*index = dev->current_input;
	dev_dbg(dev->dev, "Get input: %u\n", *index);
	
	return 0;
}

static int tvcap_s_input(struct file *file, void *priv, unsigned int index)
{
	struct sunxi_tvcap_dev *dev = video_drvdata(file);
	
	if (index >= TVCAP_NUM_INPUTS)
		return -EINVAL;
		
	if (index == dev->current_input)
		return 0;
		
	/* For HDMI input, verify connection via MIPS */
	if (index == TVCAP_INPUT_HDMI) {
		int hdmi_status = sunxi_cpu_comm_hdmi_detect_exported();
		if (hdmi_status <= 0) {
			dev_warn(dev->dev, "Cannot switch to HDMI: no signal detected\n");
			return -ENODEV;
		}
		dev->hdmi_connected = true;
	}
	
	dev->current_input = index;
	dev_info(dev->dev, "Input switched to: %u (%s)\n", 
		 index, index == TVCAP_INPUT_HDMI ? "HDMI" : "Unknown");
	
	return 0;
}

static int tvcap_g_edid(struct file *file, void *fh, struct v4l2_edid *edid)
{
	struct sunxi_tvcap_dev *dev = video_drvdata(file);
	u8 edid_buffer[256];
	int bytes_read;
	
	if (edid->pad != 0)
		return -EINVAL;
		
	if (dev->current_input != TVCAP_INPUT_HDMI) {
		dev_warn(dev->dev, "EDID read requested but HDMI input not selected\n");
		return -EINVAL;
	}
	
	if (!dev->hdmi_connected) {
		dev_warn(dev->dev, "EDID read requested but HDMI not connected\n");
		return -ENODEV;
	}
	
	/* Read EDID via MIPS communication */
	bytes_read = sunxi_cpu_comm_hdmi_read_edid_exported(edid_buffer, sizeof(edid_buffer));
	if (bytes_read <= 0) {
		dev_err(dev->dev, "Failed to read EDID from HDMI source: %d\n", bytes_read);
		return bytes_read;
	}
	
	/* Copy EDID data to user buffer */
	if (edid->blocks == 0) {
		edid->blocks = bytes_read / 128;
		return 0;
	}
	
	if (edid->start_block * 128 >= bytes_read) {
		dev_warn(dev->dev, "EDID start block %u beyond available data\n", edid->start_block);
		return -EINVAL;
	}
	
	edid->blocks = min_t(u32, edid->blocks, (bytes_read / 128) - edid->start_block);
	
	if (copy_to_user(edid->edid, 
			 edid_buffer + (edid->start_block * 128),
			 edid->blocks * 128)) {
		dev_err(dev->dev, "Failed to copy EDID to user buffer\n");
		return -EFAULT;
	}
	
	dev_info(dev->dev, "EDID read successfully: %u blocks from block %u\n",
		 edid->blocks, edid->start_block);
		 
	return 0;
}

static int tvcap_s_edid(struct file *file, void *fh, struct v4l2_edid *edid)
{
	/* EDID setting not supported for input capture */
	return -ENOTTY;
}

static int tvcap_query_dv_timings(struct file *file, void *fh, struct v4l2_dv_timings *timings)
{
	struct sunxi_tvcap_dev *dev = video_drvdata(file);
	struct hdmi_format_info format;
	int ret;
	
	if (dev->current_input != TVCAP_INPUT_HDMI) {
		dev_warn(dev->dev, "DV timings query requested but HDMI input not selected\n");
		return -EINVAL;
	}
	
	if (!dev->hdmi_connected) {
		dev_warn(dev->dev, "DV timings query requested but HDMI not connected\n");
		return -ENODEV;
	}
	
	/* Get current HDMI format via MIPS communication */
	ret = sunxi_cpu_comm_hdmi_get_format_exported(&format);
	if (ret < 0) {
		dev_err(dev->dev, "Failed to get HDMI format: %d\n", ret);
		return ret;
	}
	
	/* Convert HDMI format to V4L2 DV timings */
	memset(timings, 0, sizeof(*timings));
	timings->type = V4L2_DV_BT_656_1120;
	
	timings->bt.width = format.width;
	timings->bt.height = format.height;
	timings->bt.pixelclock = format.width * format.height * format.refresh_rate;
	
	/* Set standard timing parameters based on resolution */
	if (format.width == 1920 && format.height == 1080) {
		/* 1080p timing */
		timings->bt.hfrontporch = 88;
		timings->bt.hsync = 44;
		timings->bt.hbackporch = 148;
		timings->bt.vfrontporch = 4;
		timings->bt.vsync = 5;
		timings->bt.vbackporch = 36;
	} else if (format.width == 1280 && format.height == 720) {
		/* 720p timing */
		timings->bt.hfrontporch = 110;
		timings->bt.hsync = 40;
		timings->bt.hbackporch = 220;
		timings->bt.vfrontporch = 5;
		timings->bt.vsync = 5;
		timings->bt.vbackporch = 20;
	}
	
	dev_info(dev->dev, "DV timings: %ux%u@%u, pixelclock=%llu\n",
		 format.width, format.height, format.refresh_rate, timings->bt.pixelclock);
		 
	return 0;
}

static const struct v4l2_ioctl_ops tvcap_ioctl_ops_cap = {
	.vidioc_querycap        = tvcap_querycap,
	.vidioc_enum_fmt_vid_cap = tvcap_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap   = tvcap_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap   = tvcap_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap = tvcap_try_fmt_vid_cap,
	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,
	
	/* Input management for HDMI switching */
	.vidioc_enum_input      = tvcap_enum_input,
	.vidioc_g_input         = tvcap_g_input,
	.vidioc_s_input         = tvcap_s_input,
	.vidioc_g_edid          = tvcap_g_edid,
	.vidioc_s_edid          = tvcap_s_edid,
	.vidioc_query_dv_timings = tvcap_query_dv_timings,
};

static const struct v4l2_ioctl_ops tvcap_ioctl_ops_out = {
	.vidioc_querycap        = tvcap_querycap,
	.vidioc_enum_fmt_vid_out = tvcap_enum_fmt_vid_out,
	.vidioc_g_fmt_vid_out   = tvcap_g_fmt_vid_out,
	.vidioc_s_fmt_vid_out   = tvcap_s_fmt_vid_out,
	.vidioc_try_fmt_vid_out = tvcap_try_fmt_vid_out,
	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,
};

static const struct v4l2_file_operations tvcap_fops_cap = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open           = v4l2_fh_open,
	.release        = vb2_fop_release,
	.poll           = vb2_fop_poll,
	.mmap           = vb2_fop_mmap,
	.read           = vb2_fop_read,
};

static const struct v4l2_file_operations tvcap_fops_out = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open           = v4l2_fh_open,
	.release        = vb2_fop_release,
	.poll           = vb2_fop_poll,
	.mmap           = vb2_fop_mmap,
	.write          = vb2_fop_write,
};

/*
 * Enhanced V4L2 Initialization
 */
static int tvcap_init_v4l2_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	struct video_device *vdev_cap = &tvcap->video_dev_cap;
	struct video_device *vdev_out = &tvcap->video_dev_out;
	struct vb2_queue *q_cap = &tvcap->queue_cap;
	struct vb2_queue *q_out = &tvcap->queue_out;
	int ret;
	
	/* Initialize V4L2 device */
	ret = v4l2_device_register(dev, &tvcap->v4l2_dev);
	if (ret)
		return ret;
	
	/* Initialize control handler */
	v4l2_ctrl_handler_init(&tvcap->ctrl_handler, 0);
	tvcap->v4l2_dev.ctrl_handler = &tvcap->ctrl_handler;
	
	/* Initialize default formats */
	tvcap->format_cap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tvcap->format_cap.fmt.pix.width = 1920;
	tvcap->format_cap.fmt.pix.height = 1080;
	tvcap->format_cap.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	tvcap->format_cap.fmt.pix.field = V4L2_FIELD_NONE;
	tvcap->format_cap.fmt.pix.bytesperline = 1920 * 2;
	tvcap->format_cap.fmt.pix.sizeimage = 1920 * 1080 * 2;
	
	tvcap->format_out = tvcap->format_cap;
	tvcap->format_out.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	
	/* Initialize capture queue */
	q_cap->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q_cap->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q_cap->drv_priv = tvcap;
	q_cap->buf_struct_size = sizeof(struct tvcap_buffer);
	q_cap->ops = &tvcap_qops_cap;
	q_cap->mem_ops = &vb2_dma_contig_memops;
	q_cap->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q_cap->min_buffers_needed = 2;
	q_cap->dev = dev;
	q_cap->lock = &tvcap->lock;
	
	ret = vb2_queue_init(q_cap);
	if (ret)
		goto err_ctrl;
	
	/* Initialize output queue */
	q_out->type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	q_out->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_WRITE;
	q_out->drv_priv = tvcap;
	q_out->buf_struct_size = sizeof(struct tvcap_buffer);
	q_out->ops = &tvcap_qops_out;
	q_out->mem_ops = &vb2_dma_contig_memops;
	q_out->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q_out->min_buffers_needed = 2;
	q_out->dev = dev;
	q_out->lock = &tvcap->lock;
	
	ret = vb2_queue_init(q_out);
	if (ret)
		goto err_queue_cap;
	
	/* Initialize capture video device */
	vdev_cap->fops = &tvcap_fops_cap;
	vdev_cap->ioctl_ops = &tvcap_ioctl_ops_cap;
	vdev_cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;
	vdev_cap->v4l2_dev = &tvcap->v4l2_dev;
	vdev_cap->queue = q_cap;
	vdev_cap->lock = &tvcap->lock;
	vdev_cap->release = video_device_release_empty;
	strscpy(vdev_cap->name, SUNXI_TVCAP_NAME "-cap", sizeof(vdev_cap->name));
	video_set_drvdata(vdev_cap, tvcap);
	
	ret = video_register_device(vdev_cap, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err_queue_out;
	
	/* Initialize output video device */
	vdev_out->fops = &tvcap_fops_out;
	vdev_out->ioctl_ops = &tvcap_ioctl_ops_out;
	vdev_out->device_caps = V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING;
	vdev_out->v4l2_dev = &tvcap->v4l2_dev;
	vdev_out->queue = q_out;
	vdev_out->lock = &tvcap->lock;
	vdev_out->release = video_device_release_empty;
	strscpy(vdev_out->name, SUNXI_TVCAP_NAME "-out", sizeof(vdev_out->name));
	video_set_drvdata(vdev_out, tvcap);
	
	ret = video_register_device(vdev_out, VFL_TYPE_VIDEO, -1);
	if (ret)
		goto err_video_cap;
	
	dev_info(dev, "Enhanced V4L2 devices: %s (capture), %s (output)\n", 
		 video_device_node_name(vdev_cap),
		 video_device_node_name(vdev_out));
	return 0;
	
err_video_cap:
	video_unregister_device(vdev_cap);
err_queue_out:
	vb2_queue_release(q_out);
err_queue_cap:
	vb2_queue_release(q_cap);
err_ctrl:
	v4l2_ctrl_handler_free(&tvcap->ctrl_handler);
	v4l2_device_unregister(&tvcap->v4l2_dev);
	return ret;
}

static void tvcap_cleanup_v4l2_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	video_unregister_device(&tvcap->video_dev_cap);
	video_unregister_device(&tvcap->video_dev_out);
	vb2_queue_release(&tvcap->queue_cap);
	vb2_queue_release(&tvcap->queue_out);
	v4l2_ctrl_handler_free(&tvcap->ctrl_handler);
	v4l2_device_unregister(&tvcap->v4l2_dev);
}

/*
 * Platform Driver Implementation
 */
static int tvcap_init_resources_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	struct platform_device *pdev = to_platform_device(dev);
	int ret;
	
	/* Get memory resource */
	tvcap->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(tvcap->regs))
		return PTR_ERR(tvcap->regs);
	
	/* Get interrupt */
	tvcap->irq = platform_get_irq(pdev, 0);
	if (tvcap->irq < 0)
		return tvcap->irq;
	
	/* Initialize enhanced clocks */
	ret = tvcap_clocks_init_enhanced(tvcap);
	if (ret)
		return ret;
	
	/* Initialize enhanced reset controls */
	ret = tvcap_resets_init_enhanced(tvcap);
	if (ret)
		return ret;
	
	/* Initialize IOMMU */
	ret = tvcap_iommu_init(tvcap);
	if (ret)
		dev_warn(dev, "IOMMU initialization failed: %d\n", ret);
	
	dev_info(dev, "Enhanced resources initialized\n");
	return 0;
}

static int sunxi_tvcap_probe(struct platform_device *pdev)
{
	struct sunxi_tvcap_dev *tvcap;
	int ret;
	
	dev_info(&pdev->dev, "Probing Enhanced Allwinner H713 TV Capture driver\n");
	
	tvcap = devm_kzalloc(&pdev->dev, sizeof(*tvcap), GFP_KERNEL);
	if (!tvcap)
		return -ENOMEM;
	
	tvcap->dev = &pdev->dev;
	platform_set_drvdata(pdev, tvcap);
	
	/* Initialize synchronization */
	mutex_init(&tvcap->lock);
	spin_lock_init(&tvcap->irq_lock);
	INIT_LIST_HEAD(&tvcap->buf_list_cap);
	INIT_LIST_HEAD(&tvcap->buf_list_out);
	
	/* Initialize input management */
	tvcap->current_input = TVCAP_INPUT_HDMI;
	tvcap->hdmi_connected = false;
	
	/* Initialize enhanced resources */
	ret = tvcap_init_resources_enhanced(tvcap);
	if (ret)
		return ret;
	
	/* Initialize enhanced hardware */
	ret = tvcap_hw_init_enhanced(tvcap);
	if (ret)
		goto err_cleanup;
	
	/* Initialize enhanced V4L2 */
	ret = tvcap_init_v4l2_enhanced(tvcap);
	if (ret)
		goto err_cleanup;
	
	dev_info(&pdev->dev, "Enhanced TV Capture driver probed successfully\n");
	return 0;
	
err_cleanup:
	tvcap_iommu_cleanup(tvcap);
	return ret;
}

static int sunxi_tvcap_remove(struct platform_device *pdev)
{
	struct sunxi_tvcap_dev *tvcap = platform_get_drvdata(pdev);
	
	tvcap_cleanup_v4l2_enhanced(tvcap);
	tvcap_iommu_cleanup(tvcap);
	clk_bulk_disable_unprepare(TVCAP_CLK_COUNT, tvcap->clks);
	
	dev_info(&pdev->dev, "Enhanced TV Capture driver removed\n");
	return 0;
}

static const struct of_device_id sunxi_tvcap_dt_ids[] = {
	{ .compatible = "allwinner,sun50i-h713-tvcap-enhanced" },
	{ .compatible = "allwinner,sun50i-h713-tvcap" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sunxi_tvcap_dt_ids);

static struct platform_driver sunxi_tvcap_driver = {
	.probe = sunxi_tvcap_probe,
	.remove = sunxi_tvcap_remove,
	.driver = {
		.name = SUNXI_TVCAP_NAME,
		.of_match_table = sunxi_tvcap_dt_ids,
	},
};

module_platform_driver(sunxi_tvcap_driver);

MODULE_DESCRIPTION("Enhanced Allwinner H713 TV Capture V4L2 Driver with IOMMU and Display Output");
MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("2.0.0");
MODULE_ALIAS("platform:" SUNXI_TVCAP_NAME);