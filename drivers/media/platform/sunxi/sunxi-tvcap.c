/*
 * Allwinner H713 TV Capture V4L2 Driver
 * 
 * Based on comprehensive factory firmware analysis from Task 022.
 * Implements HDMI input capture with MIPS co-processor coordination.
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

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-dev.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-dma-contig.h>

#define SUNXI_TVCAP_NAME "sunxi-tvcap"
#define SUNXI_TVCAP_VERSION KERNEL_VERSION(1, 0, 0)

/* TV Capture Hardware Registers (based on Task 022 analysis) */
#define TVTOP_BASE_OFFSET       0x0000  /* tvtop@5700000 */
#define TVCAP_BASE_OFFSET       0x1100000 /* tvcap@6800000 */
#define TV_SECONDARY_OFFSET     0x1000000 /* 0x6700000 */
#define TV_EXTENDED_OFFSET      0x1700000 /* 0x6e00000 */

/* TV TOP Control Registers */
#define TVTOP_CTRL_REG          0x0000
#define TVTOP_STATUS_REG        0x0004
#define TVTOP_IRQ_EN_REG        0x0008
#define TVTOP_IRQ_STATUS_REG    0x000c
#define TVTOP_FORMAT_REG        0x0010
#define TVTOP_RESOLUTION_REG    0x0014

/* TV Capture Status Flags */
#define TVTOP_STATUS_HDMI_CONNECTED BIT(0)
#define TVTOP_STATUS_SIGNAL_DETECTED BIT(1)
#define TVTOP_STATUS_FORMAT_DETECTED BIT(2)
#define TVTOP_STATUS_CAPTURE_ACTIVE  BIT(3)

/* TV-specific clock indices (from Task 022/019 analysis) */
enum tvcap_clks {
	TVCAP_CLK_BUS_TVCAP = 0,    /* Bus clock for TV capture */
	TVCAP_CLK_CAP_300M,         /* 300MHz capture clock */
	TVCAP_CLK_VINCAP_DMA,       /* DMA clock for video input capture */
	TVCAP_CLK_TVCAP,            /* Main TV capture clock */
	TVCAP_CLK_TVE,              /* TV encoder clock */
	TVCAP_CLK_DEMOD,            /* Demodulator clock */
	TVCAP_CLK_TVTOP,            /* TV TOP subsystem clock */
	TVCAP_CLK_COUNT
};

/* Reset indices (from Task 022 device tree analysis) */
enum tvcap_resets {
	TVCAP_RST_BUS_DISP = 0,
	TVCAP_RST_BUS_TVCAP,
	TVCAP_RST_BUS_DEMOD,
	TVCAP_RST_COUNT
};

/* Buffer management */
#define TVCAP_MIN_BUFFERS       2
#define TVCAP_MAX_BUFFERS       8
#define TVCAP_DEFAULT_BUFFERS   4

/* Supported video formats (from Task 022 Android driver analysis) */
struct tvcap_format {
	u32 fourcc;
	u32 depth;
	u32 planes;
	const char *name;
};

static const struct tvcap_format formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_YUYV,
		.depth = 16,
		.planes = 1,
		.name = "YUV 4:2:2 (YUYV)",
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.depth = 12,
		.planes = 3,
		.name = "YUV 4:2:0 Planar",
	},
	{
		.fourcc = V4L2_PIX_FMT_RGB24,
		.depth = 24,
		.planes = 1,
		.name = "RGB 8-8-8",
	},
	{
		.fourcc = V4L2_PIX_FMT_RGB32,
		.depth = 32,
		.planes = 1,
		.name = "RGB 8-8-8-8",
	},
};

/* Device structure */
struct sunxi_tvcap_dev {
	struct v4l2_device v4l2_dev;
	struct video_device video_dev;
	struct vb2_queue queue;
	struct v4l2_ctrl_handler ctrl_handler;
	struct device *dev;

	/* Hardware resources */
	void __iomem *regs;
	struct clk_bulk_data clks[TVCAP_CLK_COUNT];
	struct reset_control_bulk_data resets[TVCAP_RST_COUNT];
	int irq;

	/* Video format and input state */
	struct v4l2_format format;
	struct v4l2_input input;
	bool hdmi_connected;
	bool signal_detected;
	bool streaming;

	/* Synchronization */
	struct mutex lock;
	spinlock_t irq_lock;

	/* Buffer management */
	struct list_head buf_list;
	u32 sequence;
};

/* Buffer structure */
struct tvcap_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	dma_addr_t dma_addr;
};

/* Forward declarations */
static int tvcap_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			     unsigned int *nplanes, unsigned int sizes[],
			     struct device *alloc_devs[]);
static int tvcap_buffer_prepare(struct vb2_buffer *vb);
static void tvcap_buffer_queue(struct vb2_buffer *vb);
static int tvcap_start_streaming(struct vb2_queue *vq, unsigned int count);
static void tvcap_stop_streaming(struct vb2_queue *vq);

/* VB2 queue operations */
static const struct vb2_ops tvcap_qops = {
	.queue_setup = tvcap_queue_setup,
	.buf_prepare = tvcap_buffer_prepare,
	.buf_queue = tvcap_buffer_queue,
	.start_streaming = tvcap_start_streaming,
	.stop_streaming = tvcap_stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/* V4L2 IOCTL operations forward declarations */
static int tvcap_querycap(struct file *file, void *priv,
			  struct v4l2_capability *cap);
static int tvcap_enum_fmt_vid_cap(struct file *file, void *priv,
				  struct v4l2_fmtdesc *f);
static int tvcap_g_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f);
static int tvcap_s_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f);
static int tvcap_try_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_format *f);
static int tvcap_enum_input(struct file *file, void *priv,
			    struct v4l2_input *inp);
static int tvcap_g_input(struct file *file, void *priv, unsigned int *i);
static int tvcap_s_input(struct file *file, void *priv, unsigned int i);

/* V4L2 IOCTL operations */
static const struct v4l2_ioctl_ops tvcap_ioctl_ops = {
	.vidioc_querycap        = tvcap_querycap,
	.vidioc_enum_fmt_vid_cap = tvcap_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap   = tvcap_g_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap   = tvcap_s_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap = tvcap_try_fmt_vid_cap,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	.vidioc_expbuf          = vb2_ioctl_expbuf,

	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,

	.vidioc_enum_input      = tvcap_enum_input,
	.vidioc_g_input         = tvcap_g_input,
	.vidioc_s_input         = tvcap_s_input,
};

/* V4L2 file operations */
static const struct v4l2_file_operations tvcap_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = video_ioctl2,
	.open           = v4l2_fh_open,
	.release        = vb2_fop_release,
	.poll           = vb2_fop_poll,
	.mmap           = vb2_fop_mmap,
	.read           = vb2_fop_read,
};


/*
 * TV Capture Clock Management Functions
 */

static int tvcap_clocks_enable(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	dev_dbg(tvcap->dev, "Enabling TV capture clocks
");

	/* Enable all TV capture clocks in bulk */
	ret = clk_bulk_prepare_enable(TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(tvcap->dev, "Failed to enable TV capture clocks: %d
", ret);
		return ret;
	}

	/* Set specific clock rates for optimal TV capture performance */
	/* CAP_300M clock - 300MHz for high-speed capture */
	ret = clk_set_rate(tvcap->clks[TVCAP_CLK_CAP_300M].clk, 300000000);
	if (ret)
		dev_warn(tvcap->dev, "Failed to set CAP_300M rate: %d
", ret);

	/* VINCAP_DMA clock - optimal DMA transfer rate */
	ret = clk_set_rate(tvcap->clks[TVCAP_CLK_VINCAP_DMA].clk, 200000000);
	if (ret)
		dev_warn(tvcap->dev, "Failed to set VINCAP_DMA rate: %d
", ret);

	/* TVCAP clock - main TV capture clock */
	ret = clk_set_rate(tvcap->clks[TVCAP_CLK_TVCAP].clk, 150000000);
	if (ret)
		dev_warn(tvcap->dev, "Failed to set TVCAP rate: %d
", ret);

	/* TVE clock - TV encoder clock */
	ret = clk_set_rate(tvcap->clks[TVCAP_CLK_TVE].clk, 297000000);
	if (ret)
		dev_warn(tvcap->dev, "Failed to set TVE rate: %d
", ret);

	dev_info(tvcap->dev, "TV capture clocks enabled successfully
");
	return 0;
}

static void tvcap_clocks_disable(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Disabling TV capture clocks
");

	/* Disable all TV capture clocks in bulk */
	clk_bulk_disable_unprepare(TVCAP_CLK_COUNT, tvcap->clks);

	dev_dbg(tvcap->dev, "TV capture clocks disabled
");
}

static int tvcap_clocks_init(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	int ret, i;

	/* Initialize TV-specific clock names (must match device tree) */
	static const char * const tv_clk_names[TVCAP_CLK_COUNT] = {
		"clk_bus_tvcap",     /* Bus interface clock */
		"cap_300m",          /* 300MHz capture clock */
		"vincap_dma_clk",    /* DMA transfer clock */
		"tvcap",             /* Main TV capture clock */
		"tve",               /* TV encoder clock */
		"demod",             /* Demodulator clock */
		"tvtop"              /* TV TOP subsystem clock */
	};

	/* Assign clock names to bulk data structure */
	for (i = 0; i < TVCAP_CLK_COUNT; i++)
		tvcap->clks[i].id = tv_clk_names[i];

	/* Get all TV capture clocks from device tree */
	ret = devm_clk_bulk_get(dev, TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(dev, "Failed to get TV capture clocks: %d
", ret);
		return ret;
	}

	dev_info(dev, "TV capture clocks initialized: %d clocks
", TVCAP_CLK_COUNT);
	return 0;
}

/*
 * Hardware interface functions
 */

static void tvcap_hw_reset(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Resetting TV capture hardware\n");
	
	/* Assert all reset lines */
	reset_control_bulk_assert(TVCAP_RST_COUNT, tvcap->resets);
	usleep_range(10, 20);
	
	/* Deassert reset lines */
	reset_control_bulk_deassert(TVCAP_RST_COUNT, tvcap->resets);
	usleep_range(100, 200);
}

static int tvcap_hw_init(struct sunxi_tvcap_dev *tvcap)
{
	u32 reg_val;
	int ret;
	
	dev_dbg(tvcap->dev, "Initializing TV capture hardware\n");	/* Enable TV capture clocks */
	ret = tvcap_clocks_enable(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "Failed to enable TV capture clocks: %d
", ret);
		return ret;
	}
	
	/* Reset hardware */
	tvcap_hw_reset(tvcap);
	
	/* Basic hardware initialization */
	writel(0, tvcap->regs + TVTOP_CTRL_REG);
	writel(0, tvcap->regs + TVTOP_IRQ_EN_REG);
	
	/* Clear any pending interrupts */
	reg_val = readl(tvcap->regs + TVTOP_IRQ_STATUS_REG);
	writel(reg_val, tvcap->regs + TVTOP_IRQ_STATUS_REG);
	
	dev_info(tvcap->dev, "TV capture hardware initialized\n");
	return 0;
}

static void tvcap_hw_cleanup(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Cleaning up TV capture hardware\n");
	
	/* Disable interrupts */
	writel(0, tvcap->regs + TVTOP_IRQ_EN_REG);
	
	/* Reset hardware */
	tvcap_hw_reset(tvcap);	/* Disable TV capture clocks */
	tvcap_clocks_disable(tvcap);
}

/*
 * Interrupt handler
 */
static irqreturn_t tvcap_irq_handler(int irq, void *dev_id)
{
	struct sunxi_tvcap_dev *tvcap = dev_id;
	u32 status;
	
	spin_lock(&tvcap->irq_lock);
	
	status = readl(tvcap->regs + TVTOP_IRQ_STATUS_REG);
	if (!status) {
		spin_unlock(&tvcap->irq_lock);
		return IRQ_NONE;
	}
	
	/* Clear interrupt status */
	writel(status, tvcap->regs + TVTOP_IRQ_STATUS_REG);
	
	dev_dbg(tvcap->dev, "TV capture interrupt: 0x%08x\n", status);
	
	/* TODO: Handle specific interrupt types */
	/* - HDMI hot-plug detection */
	/* - Signal format detection */
	/* - Capture completion */
	/* - Error conditions */
	
	spin_unlock(&tvcap->irq_lock);
	return IRQ_HANDLED;
}

/*
 * VB2 queue operations implementation
 */

static int tvcap_queue_setup(struct vb2_queue *vq, unsigned int *nbuffers,
			     unsigned int *nplanes, unsigned int sizes[],
			     struct device *alloc_devs[])
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct v4l2_pix_format *pix = &tvcap->format.fmt.pix;
	
	dev_dbg(tvcap->dev, "Queue setup: %d buffers requested\n", *nbuffers);
	
	if (*nbuffers < TVCAP_MIN_BUFFERS)
		*nbuffers = TVCAP_MIN_BUFFERS;
	else if (*nbuffers > TVCAP_MAX_BUFFERS)
		*nbuffers = TVCAP_MAX_BUFFERS;
	
	*nplanes = 1;
	sizes[0] = pix->sizeimage;
	
	dev_dbg(tvcap->dev, "Queue setup: %d buffers, size %u\n", 
		*nbuffers, sizes[0]);
	
	return 0;
}

static int tvcap_buffer_prepare(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct v4l2_pix_format *pix = &tvcap->format.fmt.pix;
	
	if (vb2_plane_size(vb, 0) < pix->sizeimage) {
		dev_err(tvcap->dev, "Buffer too small: %lu < %u\n",
			vb2_plane_size(vb, 0), pix->sizeimage);
		return -EINVAL;
	}
	
	vb2_set_plane_payload(vb, 0, pix->sizeimage);
	return 0;
}

static void tvcap_buffer_queue(struct vb2_buffer *vb)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vbuf = to_vb2_v4l2_buffer(vb);
	struct tvcap_buffer *buf = container_of(vbuf, struct tvcap_buffer, vb);
	unsigned long flags;
	
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_add_tail(&buf->list, &tvcap->buf_list);
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
	
	dev_dbg(tvcap->dev, "Buffer queued\n");
}

static int tvcap_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	
	dev_info(tvcap->dev, "Starting TV capture streaming\n");
	
	/* TODO: Start hardware capture */
	/* - Configure capture format */
	/* - Start DMA transfers */
	/* - Enable capture interrupts */
	
	tvcap->streaming = true;
	tvcap->sequence = 0;
	
	dev_info(tvcap->dev, "TV capture streaming started\n");
	return 0;
}

static void tvcap_stop_streaming(struct vb2_queue *vq)
{
	struct sunxi_tvcap_dev *tvcap = vb2_get_drv_priv(vq);
	struct tvcap_buffer *buf, *tmp;
	unsigned long flags;
	
	dev_info(tvcap->dev, "Stopping TV capture streaming\n");
	
	/* TODO: Stop hardware capture */
	/* - Stop DMA transfers */
	/* - Disable capture interrupts */
	
	tvcap->streaming = false;
	
	/* Return all buffers to user space */
	spin_lock_irqsave(&tvcap->irq_lock, flags);
	list_for_each_entry_safe(buf, tmp, &tvcap->buf_list, list) {
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&tvcap->irq_lock, flags);
	
	dev_info(tvcap->dev, "TV capture streaming stopped\n");
}

/*
 * V4L2 IOCTL operations implementation
 */

static int tvcap_querycap(struct file *file, void *priv,
			  struct v4l2_capability *cap)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	
	strscpy(cap->driver, SUNXI_TVCAP_NAME, sizeof(cap->driver));
	strscpy(cap->card, "Allwinner H713 TV Capture", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info), 
		 "platform:%s", dev_name(tvcap->dev));
	
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			   V4L2_CAP_READWRITE;
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
	
	*f = tvcap->format;
	return 0;
}

static int tvcap_try_fmt_vid_cap(struct file *file, void *priv,
				 struct v4l2_format *f)
{
	struct v4l2_pix_format *pix = &f->fmt.pix;
	const struct tvcap_format *fmt;
	int i;
	
	/* Find format */
	fmt = &formats[0]; /* Default to first format */
	for (i = 0; i < ARRAY_SIZE(formats); i++) {
		if (formats[i].fourcc == pix->pixelformat) {
			fmt = &formats[i];
			break;
		}
	}
	
	/* Set format parameters */
	pix->pixelformat = fmt->fourcc;
	
	/* Clamp dimensions */
	v4l_bound_align_image(&pix->width, 320, 1920, 1,
			      &pix->height, 240, 1080, 1, 0);
	
	/* Calculate other parameters */
	pix->bytesperline = (pix->width * fmt->depth) >> 3;
	pix->sizeimage = pix->height * pix->bytesperline;
	pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
	pix->field = V4L2_FIELD_NONE;
	
	return 0;
}

static int tvcap_s_fmt_vid_cap(struct file *file, void *priv,
			       struct v4l2_format *f)
{
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	int ret;
	
	if (vb2_is_busy(&tvcap->queue))
		return -EBUSY;
	
	ret = tvcap_try_fmt_vid_cap(file, priv, f);
	if (ret)
		return ret;
	
	tvcap->format = *f;
	
	dev_info(tvcap->dev, "Format set: %dx%d, fourcc: %c%c%c%c\n",
		 f->fmt.pix.width, f->fmt.pix.height,
		 (f->fmt.pix.pixelformat >> 0) & 0xff,
		 (f->fmt.pix.pixelformat >> 8) & 0xff,
		 (f->fmt.pix.pixelformat >> 16) & 0xff,
		 (f->fmt.pix.pixelformat >> 24) & 0xff);
	
	return 0;
}

static int tvcap_enum_input(struct file *file, void *priv,
			    struct v4l2_input *inp)
{
	if (inp->index > 0)
		return -EINVAL;
	
	inp->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(inp->name, "HDMI Input", sizeof(inp->name));
	inp->std = V4L2_STD_UNKNOWN;
	inp->status = 0; /* TODO: Check HDMI connection status */
	
	return 0;
}

static int tvcap_g_input(struct file *file, void *priv, unsigned int *i)
{
	*i = 0; /* Only one input */
	return 0;
}

static int tvcap_s_input(struct file *file, void *priv, unsigned int i)
{
	if (i > 0)
		return -EINVAL;
	
	/* TODO: Switch to HDMI input via MIPS coordination */
	return 0;
}

/*
 * Platform driver implementation
 */

static int tvcap_init_resources(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	struct platform_device *pdev = to_platform_device(dev);
	int ret, i;
	
	/* Get memory resource */
	tvcap->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(tvcap->regs)) {
		dev_err(dev, "Failed to map registers\n");
		return PTR_ERR(tvcap->regs);
	}
	
	/* Get interrupt */
	tvcap->irq = platform_get_irq(pdev, 0);
	if (tvcap->irq < 0) {
		dev_err(dev, "Failed to get interrupt\n");
		return tvcap->irq;
	}
	
	/* Initialize clock names - must match device tree clock-names */
	static const char * const clk_names[TVCAP_CLK_COUNT] = {
		"clk_bus_disp", "clk_bus_tvcap", "clk_bus_demod",
		"cap_300m", "vincap_dma_clk", 
		"hdmi_audio_bus", "hdmi_audio_clk"
	};
	
	for (i = 0; i < TVCAP_CLK_COUNT; i++)
		tvcap->clks[i].id = clk_names[i];
	
	ret = devm_clk_bulk_get(dev, TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(dev, "Failed to get clocks: %d\n", ret);
		return ret;
	}
	
	/* Initialize reset names */
	static const char * const reset_names[TVCAP_RST_COUNT] = {
		"reset_bus_disp", "reset_bus_tvcap", "reset_bus_demod"
	};
	
	for (i = 0; i < TVCAP_RST_COUNT; i++)
		tvcap->resets[i].id = reset_names[i];
	
	ret = devm_reset_control_bulk_get_shared(dev, TVCAP_RST_COUNT, 
						 tvcap->resets);
	if (ret) {
		dev_err(dev, "Failed to get resets: %d\n", ret);
		return ret;
	}
	
	/* Request interrupt */
	ret = devm_request_irq(dev, tvcap->irq, tvcap_irq_handler, 
			       IRQF_SHARED, SUNXI_TVCAP_NAME, tvcap);
	if (ret) {
		dev_err(dev, "Failed to request interrupt: %d\n", ret);
		return ret;
	}
	
	dev_info(dev, "Resources initialized successfully\n");
	return 0;
}

static int tvcap_init_v4l2(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	struct video_device *vdev = &tvcap->video_dev;
	struct vb2_queue *q = &tvcap->queue;
	int ret;
	
	/* Initialize V4L2 device */
	ret = v4l2_device_register(dev, &tvcap->v4l2_dev);
	if (ret) {
		dev_err(dev, "Failed to register V4L2 device: %d\n", ret);
		return ret;
	}
	
	/* Initialize control handler */
	v4l2_ctrl_handler_init(&tvcap->ctrl_handler, 0);
	tvcap->v4l2_dev.ctrl_handler = &tvcap->ctrl_handler;
	
	/* Initialize default format */
	tvcap->format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	tvcap->format.fmt.pix.width = 1920;
	tvcap->format.fmt.pix.height = 1080;
	tvcap->format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	tvcap->format.fmt.pix.field = V4L2_FIELD_NONE;
	tvcap->format.fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
	tvcap->format.fmt.pix.bytesperline = 1920 * 2;
	tvcap->format.fmt.pix.sizeimage = 1920 * 1080 * 2;
	
	/* Initialize VB2 queue */
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = tvcap;
	q->buf_struct_size = sizeof(struct tvcap_buffer);
	q->ops = &tvcap_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->min_buffers_needed = TVCAP_MIN_BUFFERS;
	q->dev = dev;
	q->lock = &tvcap->lock;
	
	ret = vb2_queue_init(q);
	if (ret) {
		dev_err(dev, "Failed to initialize VB2 queue: %d\n", ret);
		goto err_ctrl_handler;
	}
	
	/* Initialize video device */
	vdev->fops = &tvcap_fops;
	vdev->ioctl_ops = &tvcap_ioctl_ops;
	vdev->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			    V4L2_CAP_READWRITE;
	vdev->v4l2_dev = &tvcap->v4l2_dev;
	vdev->queue = q;
	vdev->lock = &tvcap->lock;
	vdev->release = video_device_release_empty;
	strscpy(vdev->name, SUNXI_TVCAP_NAME, sizeof(vdev->name));
	video_set_drvdata(vdev, tvcap);
	
	ret = video_register_device(vdev, VFL_TYPE_VIDEO, -1);
	if (ret) {
		dev_err(dev, "Failed to register video device: %d\n", ret);
		goto err_queue;
	}
	
	dev_info(dev, "V4L2 device registered as %s\n", video_device_node_name(vdev));
	return 0;
	
err_queue:
	vb2_queue_release(q);
err_ctrl_handler:
	v4l2_ctrl_handler_free(&tvcap->ctrl_handler);
	v4l2_device_unregister(&tvcap->v4l2_dev);
	return ret;
}

static void tvcap_cleanup_v4l2(struct sunxi_tvcap_dev *tvcap)
{
	video_unregister_device(&tvcap->video_dev);
	vb2_queue_release(&tvcap->queue);
	v4l2_ctrl_handler_free(&tvcap->ctrl_handler);
	v4l2_device_unregister(&tvcap->v4l2_dev);
}

static int sunxi_tvcap_probe(struct platform_device *pdev)
{
	struct sunxi_tvcap_dev *tvcap;
	int ret;
	
	dev_info(&pdev->dev, "Probing Allwinner H713 TV Capture driver\n");
	
	/* Allocate device structure */
	tvcap = devm_kzalloc(&pdev->dev, sizeof(*tvcap), GFP_KERNEL);
	if (!tvcap)
		return -ENOMEM;
	
	tvcap->dev = &pdev->dev;
	platform_set_drvdata(pdev, tvcap);
	
	/* Initialize synchronization */
	mutex_init(&tvcap->lock);
	spin_lock_init(&tvcap->irq_lock);
	INIT_LIST_HEAD(&tvcap->buf_list);
	
	/* Initialize hardware resources */
	ret = tvcap_init_resources(tvcap);
	if (ret)
		return ret;
	
	/* Initialize hardware */
	ret = tvcap_hw_init(tvcap);
	if (ret)
		return ret;
	
	/* Initialize V4L2 subsystem */
	ret = tvcap_init_v4l2(tvcap);
	if (ret)
		goto err_hw_cleanup;
	
	dev_info(&pdev->dev, "TV Capture driver probed successfully\n");
	return 0;
	
err_hw_cleanup:
	tvcap_hw_cleanup(tvcap);
	return ret;
}

static int sunxi_tvcap_remove(struct platform_device *pdev)
{
	struct sunxi_tvcap_dev *tvcap = platform_get_drvdata(pdev);
	
	dev_info(&pdev->dev, "Removing TV Capture driver\n");
	
	tvcap_cleanup_v4l2(tvcap);
	tvcap_hw_cleanup(tvcap);
	
	dev_info(&pdev->dev, "TV Capture driver removed\n");
	return 0;
}

/* Device tree matching */
static const struct of_device_id sunxi_tvcap_dt_ids[] = {
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

MODULE_DESCRIPTION("Allwinner H713 TV Capture V4L2 Driver");
MODULE_AUTHOR("HY300 Linux Porting Project");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.0");
MODULE_ALIAS("platform:" SUNXI_TVCAP_NAME);