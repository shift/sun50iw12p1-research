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
#include <linux/atomic.h>
#include <linux/device.h>

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

/* Interrupt Control and Status Register offsets */
#define TVTOP_IRQ_MASK_REG      0x0018
#define TVTOP_IRQ_RAW_STATUS_REG 0x001c

/* Additional TVTOP Registers (Factory Driver Pattern from Task 022) */
#define TVTOP_CLK_CTRL_REG      0x0020   /* Clock control register */
#define TVTOP_RST_CTRL_REG      0x0024   /* Reset control register */
#define TVTOP_DMA_CTRL_REG      0x0028   /* DMA control register */
#define TVTOP_DMA_ADDR_REG      0x002c   /* DMA buffer address */
#define TVTOP_DMA_SIZE_REG      0x0030   /* DMA transfer size */
#define TVTOP_CAPTURE_CTRL_REG  0x0034   /* Capture control register */
#define TVTOP_CAPTURE_SIZE_REG  0x0038   /* Capture frame size */
#define TVTOP_HDMI_CTRL_REG     0x003c   /* HDMI input control */
#define TVTOP_HDMI_STATUS_REG   0x0040   /* HDMI input status */
#define TVTOP_DEBUG_REG         0x00fc   /* Debug/version register */

/* TV TOP Control Register Bit Definitions (from factory analysis) */
#define TVTOP_CTRL_ENABLE       BIT(0)    /* Enable TV TOP subsystem */
#define TVTOP_CTRL_CAPTURE_EN   BIT(1)    /* Enable capture function */
#define TVTOP_CTRL_HDMI_EN      BIT(2)    /* Enable HDMI input */
#define TVTOP_CTRL_DMA_EN       BIT(3)    /* Enable DMA transfers */
#define TVTOP_CTRL_AUTO_FORMAT  BIT(4)    /* Auto format detection */
#define TVTOP_CTRL_RESET        BIT(31)   /* Software reset */

/* Enhanced TV TOP Status Register Bits */
#define TVTOP_STATUS_DMA_BUSY       BIT(4)  /* DMA transfer active */
#define TVTOP_STATUS_ERROR          BIT(5)  /* Error condition */
#define TVTOP_STATUS_READY          BIT(6)  /* Hardware ready */
#define TVTOP_STATUS_FIFO_FULL      BIT(7)  /* Input FIFO full */

/* HDMI Control and Status */
#define TVTOP_HDMI_HPD_ENABLE   BIT(0)    /* Enable hot-plug detection */
#define TVTOP_HDMI_EDID_READ    BIT(1)    /* Trigger EDID read */
#define TVTOP_HDMI_FORCE_DETECT BIT(2)    /* Force signal detection */

/* Hardware timing constants (from factory driver analysis) */
#define TVTOP_RESET_DELAY_US    10     /* Reset pulse duration */
#define TVTOP_STABILIZE_DELAY_US 100   /* Hardware stabilization time */
#define TVTOP_TIMEOUT_MS        5000   /* Operation timeout */

/* TV Capture Interrupt Flags (based on Task 022 factory analysis) */
#define TVTOP_IRQ_FRAME_DONE    BIT(0)   /* Frame capture completion */
#define TVTOP_IRQ_INPUT_CHANGE  BIT(1)   /* HDMI input detect change */
#define TVTOP_IRQ_FORMAT_CHANGE BIT(2)   /* Format/resolution change */
#define TVTOP_IRQ_BUF_OVERFLOW  BIT(3)   /* Buffer overflow error */
#define TVTOP_IRQ_BUF_UNDERFLOW BIT(4)   /* Buffer underflow error */
#define TVTOP_IRQ_HDMI_HOTPLUG  BIT(5)   /* HDMI hot-plug detect */
#define TVTOP_IRQ_HW_ERROR      BIT(6)   /* Hardware error condition */
#define TVTOP_IRQ_DMA_ERROR     BIT(7)   /* DMA transfer error */
#define TVTOP_IRQ_FIFO_ERROR    BIT(8)   /* FIFO error */
#define TVTOP_IRQ_TIMEOUT       BIT(9)   /* Timeout error */

/* Combined interrupt masks */
#define TVTOP_IRQ_ALL_ERRORS    (TVTOP_IRQ_BUF_OVERFLOW | TVTOP_IRQ_BUF_UNDERFLOW | \
                                 TVTOP_IRQ_HW_ERROR | TVTOP_IRQ_DMA_ERROR | \
                                 TVTOP_IRQ_FIFO_ERROR | TVTOP_IRQ_TIMEOUT)
#define TVTOP_IRQ_ALL_EVENTS    (TVTOP_IRQ_FRAME_DONE | TVTOP_IRQ_INPUT_CHANGE | \
                                 TVTOP_IRQ_FORMAT_CHANGE | TVTOP_IRQ_HDMI_HOTPLUG)
#define TVTOP_IRQ_ALL_MASK      (TVTOP_IRQ_ALL_ERRORS | TVTOP_IRQ_ALL_EVENTS)

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
	u32 tvtop_format;
	const char *name;
};

static const struct tvcap_format formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_YUYV,
		.depth = 16,
		.planes = 1,
		.tvtop_format = 0x04, /* TVTOP_FORMAT_YUYV */
		.name = "YUV 4:2:2 (YUYV)",
	},
	{
		.fourcc = V4L2_PIX_FMT_YUV420,
		.depth = 12,
		.planes = 3,
		.tvtop_format = 0x02, /* TVTOP_FORMAT_YUV420P */
		.name = "YUV 4:2:0 Planar",
	},
	{
		.fourcc = V4L2_PIX_FMT_RGB24,
		.depth = 24,
		.planes = 1,
		.tvtop_format = 0x00, /* TVTOP_FORMAT_RGB888 */
		.tvtop_format = 0x04, /* TVTOP_FORMAT_YUYV */
		.name = "RGB 8-8-8",
	},
	{
		.fourcc = V4L2_PIX_FMT_RGB32,
		.depth = 32,
		.planes = 1,
		.tvtop_format = 0x04, /* TVTOP_FORMAT_YUYV */
		.name = "RGB 8-8-8-8",
	},
};

/* Prometheus Metrics Structure for TVCAP Driver */
struct tvcap_metrics {
	/* V4L2 Capture Statistics */
	atomic64_t frames_captured_total;
	atomic64_t frames_dropped_total;
	atomic64_t bytes_captured_total;
	atomic64_t capture_errors_total;
	
	/* Buffer Management */
	atomic64_t buffers_allocated_total;
	atomic64_t buffers_freed_total;
	atomic64_t buffer_queue_depth;
	atomic64_t buffer_overruns_total;
	
	/* HDMI Signal Detection */
	atomic64_t signal_detection_changes_total;
	atomic64_t hdmi_hotplug_events_total;
	atomic64_t format_change_events_total;
	atomic_t signal_detected_status;
	atomic_t hdmi_connected_status;
	
	/* Format and Resolution Tracking */
	atomic_t current_width;
	atomic_t current_height;
	atomic_t current_fourcc;
	atomic64_t format_negotiation_total;
	
	/* Hardware Status */
	atomic64_t hardware_errors_total;
	atomic64_t dma_errors_total;
	atomic64_t fifo_errors_total;
	atomic64_t timeout_errors_total;
	
	/* Performance Metrics */
	atomic64_t interrupt_count_total;
	atomic64_t register_access_total;
	atomic_t streaming_active;
};

/* External reference to hy300_class from MIPS loader */
extern struct class *hy300_class;


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

	/* Hardware state */
	bool tvtop_initialized;
	u32 current_resolution;
	u32 current_format;

	/* Synchronization */
	struct mutex lock;
	spinlock_t irq_lock;

	/* Buffer management */
	struct list_head buf_list;
	u32 sequence;
	
	/* Prometheus metrics */
	struct tvcap_metrics metrics;
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

/* Prometheus Metrics Sysfs Functions */
static ssize_t capture_stats_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_tvcap_dev *tvcap = dev_get_drvdata(dev);
	
	return sprintf(buf,
		"# HELP hy300_tvcap_frames_captured_total Total captured frames\n"
		"# TYPE hy300_tvcap_frames_captured_total counter\n"
		"hy300_tvcap_frames_captured_total %lld\n"
		"# HELP hy300_tvcap_frames_dropped_total Total dropped frames\n"
		"# TYPE hy300_tvcap_frames_dropped_total counter\n"
		"hy300_tvcap_frames_dropped_total %lld\n"
		"# HELP hy300_tvcap_bytes_captured_total Total bytes captured\n"
		"# TYPE hy300_tvcap_bytes_captured_total counter\n"
		"hy300_tvcap_bytes_captured_total %lld\n"
		"# HELP hy300_tvcap_capture_errors_total Total capture errors\n"
		"# TYPE hy300_tvcap_capture_errors_total counter\n"
		"hy300_tvcap_capture_errors_total %lld\n",
		atomic64_read(&tvcap->metrics.frames_captured_total),
		atomic64_read(&tvcap->metrics.frames_dropped_total),
		atomic64_read(&tvcap->metrics.bytes_captured_total),
		atomic64_read(&tvcap->metrics.capture_errors_total));
}
static DEVICE_ATTR_RO(capture_stats);

static ssize_t buffer_status_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct sunxi_tvcap_dev *tvcap = dev_get_drvdata(dev);
	
	return sprintf(buf,
		"# HELP hy300_tvcap_buffers_allocated_total Total buffers allocated\n"
		"# TYPE hy300_tvcap_buffers_allocated_total counter\n"
		"hy300_tvcap_buffers_allocated_total %lld\n"
		"# HELP hy300_tvcap_buffer_queue_depth Current buffer queue depth\n"
		"# TYPE hy300_tvcap_buffer_queue_depth gauge\n"
		"hy300_tvcap_buffer_queue_depth %lld\n",
		atomic64_read(&tvcap->metrics.buffers_allocated_total),
		atomic64_read(&tvcap->metrics.buffer_queue_depth));
}
static DEVICE_ATTR_RO(buffer_status);

static ssize_t signal_detection_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct sunxi_tvcap_dev *tvcap = dev_get_drvdata(dev);
	
	return sprintf(buf,
		"# HELP hy300_tvcap_signal_detected HDMI signal detection status\n"
		"# TYPE hy300_tvcap_signal_detected gauge\n"
		"hy300_tvcap_signal_detected %d\n"
		"# HELP hy300_tvcap_hdmi_connected HDMI connection status\n"
		"# TYPE hy300_tvcap_hdmi_connected gauge\n"
		"hy300_tvcap_hdmi_connected %d\n",
		atomic_read(&tvcap->metrics.signal_detected_status),
		atomic_read(&tvcap->metrics.hdmi_connected_status));
}
static DEVICE_ATTR_RO(signal_detection);

static ssize_t error_counters_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct sunxi_tvcap_dev *tvcap = dev_get_drvdata(dev);
	
	return sprintf(buf,
		"# HELP hy300_tvcap_hardware_errors_total Total hardware errors\n"
		"# TYPE hy300_tvcap_hardware_errors_total counter\n"
		"hy300_tvcap_hardware_errors_total %lld\n"
		"# HELP hy300_tvcap_interrupt_count_total Total interrupts handled\n"
		"# TYPE hy300_tvcap_interrupt_count_total counter\n"
		"hy300_tvcap_interrupt_count_total %lld\n"
		"# HELP hy300_tvcap_streaming_active Streaming status\n"
		"# TYPE hy300_tvcap_streaming_active gauge\n"
		"hy300_tvcap_streaming_active %d\n",
		atomic64_read(&tvcap->metrics.hardware_errors_total),
		atomic64_read(&tvcap->metrics.interrupt_count_total),
		atomic_read(&tvcap->metrics.streaming_active));
}
static DEVICE_ATTR_RO(error_counters);

/* Attribute group for sysfs */
static struct attribute *tvcap_attrs[] = {
	&dev_attr_capture_stats.attr,
	&dev_attr_buffer_status.attr,
	&dev_attr_signal_detection.attr,
	&dev_attr_error_counters.attr,
	NULL,
};

static const struct attribute_group tvcap_attr_group = {
	.attrs = tvcap_attrs,
};

static const struct attribute_group *tvcap_attr_groups[] = {
	&tvcap_attr_group,
	NULL,
};

/* Metrics helper functions */
static void tvcap_metrics_init(struct sunxi_tvcap_dev *tvcap)
{
	/* Initialize all atomic counters to zero */
	atomic64_set(&tvcap->metrics.frames_captured_total, 0);
	atomic64_set(&tvcap->metrics.frames_dropped_total, 0);
	atomic64_set(&tvcap->metrics.bytes_captured_total, 0);
	atomic64_set(&tvcap->metrics.capture_errors_total, 0);
	
	atomic64_set(&tvcap->metrics.buffers_allocated_total, 0);
	atomic64_set(&tvcap->metrics.buffers_freed_total, 0);
	atomic64_set(&tvcap->metrics.buffer_queue_depth, 0);
	atomic64_set(&tvcap->metrics.buffer_overruns_total, 0);
	
	atomic64_set(&tvcap->metrics.signal_detection_changes_total, 0);
	atomic64_set(&tvcap->metrics.hdmi_hotplug_events_total, 0);
	atomic64_set(&tvcap->metrics.format_change_events_total, 0);
	atomic_set(&tvcap->metrics.signal_detected_status, 0);
	atomic_set(&tvcap->metrics.hdmi_connected_status, 0);
	
	atomic_set(&tvcap->metrics.current_width, 0);
	atomic_set(&tvcap->metrics.current_height, 0);
	atomic_set(&tvcap->metrics.current_fourcc, 0);
	atomic64_set(&tvcap->metrics.format_negotiation_total, 0);
	
	atomic64_set(&tvcap->metrics.hardware_errors_total, 0);
	atomic64_set(&tvcap->metrics.dma_errors_total, 0);
	atomic64_set(&tvcap->metrics.fifo_errors_total, 0);
	atomic64_set(&tvcap->metrics.timeout_errors_total, 0);
	
	atomic64_set(&tvcap->metrics.interrupt_count_total, 0);
	atomic64_set(&tvcap->metrics.register_access_total, 0);
	atomic_set(&tvcap->metrics.streaming_active, 0);
}


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
 * TV Capture Reset Line Management Functions
 * Based on Task 022 hardware analysis: RST_BUS_TVCAP, RST_BUS_DISP, RST_BUS_DEMOD
 */

static int tvcap_resets_assert(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	dev_dbg(tvcap->dev, "Asserting TV capture reset lines\n");

	/* Assert all reset lines - this puts hardware into reset state */
	ret = reset_control_bulk_assert(TVCAP_RST_COUNT, tvcap->resets);
	if (ret) {
		dev_err(tvcap->dev, "Failed to assert TV capture resets: %d\n", ret);
		return ret;
	}

	/* Hold reset for minimum required time (from Task 022 timing analysis) */
	usleep_range(10, 20);

	dev_dbg(tvcap->dev, "TV capture reset lines asserted\n");
	return 0;
}

static int tvcap_resets_deassert(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	dev_dbg(tvcap->dev, "Deasserting TV capture reset lines\n");

	/* Deassert reset lines in proper sequence - releases hardware from reset */
	ret = reset_control_bulk_deassert(TVCAP_RST_COUNT, tvcap->resets);
	if (ret) {
		dev_err(tvcap->dev, "Failed to deassert TV capture resets: %d\n", ret);
		return ret;
	}

	/* Allow hardware to stabilize after reset release */
	usleep_range(100, 200);

	dev_dbg(tvcap->dev, "TV capture reset lines deasserted\n");
	return 0;
}

static int tvcap_resets_cycle(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	dev_dbg(tvcap->dev, "Cycling TV capture reset lines\n");

	/* Full reset cycle: assert -> wait -> deassert -> stabilize */
	ret = tvcap_resets_assert(tvcap);
	if (ret)
		return ret;

	ret = tvcap_resets_deassert(tvcap);
	if (ret) {
		/* If deassert fails, try to return to safe reset state */
		tvcap_resets_assert(tvcap);
		return ret;
	}

	dev_info(tvcap->dev, "TV capture reset cycle completed successfully\n");
	return 0;
}

static int tvcap_resets_init(struct sunxi_tvcap_dev *tvcap)
{
	struct device *dev = tvcap->dev;
	int ret, i;

	/* Initialize reset control names (must match device tree reset-names) */
	static const char * const reset_names[TVCAP_RST_COUNT] = {
		"rst_bus_disp",      /* Display subsystem reset */
		"rst_bus_tvcap",     /* TV capture bus reset */
		"rst_bus_demod"      /* Demodulator reset */
	};

	/* Assign reset names to bulk data structure */
	for (i = 0; i < TVCAP_RST_COUNT; i++)
		tvcap->resets[i].id = reset_names[i];

	/* Get all reset controls from device tree */
	ret = devm_reset_control_bulk_get_shared(dev, TVCAP_RST_COUNT, 
						 tvcap->resets);
	if (ret) {
		dev_err(dev, "Failed to get TV capture reset controls: %d\n", ret);
		return ret;
	}

	dev_info(dev, "TV capture reset controls initialized: %d resets\n", TVCAP_RST_COUNT);
	return 0;
}

/*
 * Hardware interface functions with integrated reset and clock management
 */


/*
 * Hardware interface functions
 */

static void tvcap_hw_reset(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Resetting TV capture hardware\n");
	
	/* Use the enhanced reset cycle function */
	tvcap_resets_cycle(tvcap);
}

static int tvcap_hw_init(struct sunxi_tvcap_dev *tvcap)
{
	u32 reg_val;
	int ret;
	
	dev_dbg(tvcap->dev, "Initializing TV capture hardware\n");

	/* Step 1: Assert reset lines before enabling clocks (critical timing) */
	ret = tvcap_resets_assert(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "Failed to assert resets during init: %d\n", ret);
		return ret;
	}

	/* Step 2: Enable TV capture clocks */
	ret = tvcap_clocks_enable(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "Failed to enable TV capture clocks: %d\n", ret);
		goto err_reset_cleanup;
	}
	
	/* Step 3: Deassert reset lines after clocks are stable */
	ret = tvcap_resets_deassert(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "Failed to deassert resets during init: %d\n", ret);
		goto err_clocks_cleanup;
	}
	
	
	/* Step 4: Complete TVTOP hardware initialization */
	ret = tvtop_init_hardware(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP hardware initialization failed: %d\n", ret);
		goto err_clocks_cleanup;
	}
	
	
	dev_info(tvcap->dev, "TV capture hardware with enhanced TVTOP interface initialized successfully\n");
	return 0;

err_clocks_cleanup:
	tvcap_clocks_disable(tvcap);
err_reset_cleanup:
	/* Leave hardware in safe reset state */
	tvcap_resets_assert(tvcap);
	return ret;
}

static void tvcap_hw_cleanup(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Cleaning up TV capture hardware\n");
	
	/* Disable all interrupts first using enhanced interface */
	tvtop_disable_interrupts(tvcap, TVTOP_IRQ_ALL_MASK);
	
	/* Disable TVTOP subsystem if initialized */
	if (tvcap->tvtop_initialized) {
		tvtop_disable_subsystem(tvcap);
		tvcap->tvtop_initialized = false;
	}
	
	/* Assert reset lines to put hardware in safe state */
	tvcap_resets_assert(tvcap);
	
	/* Disable clocks after hardware is in reset state */
	tvcap_clocks_disable(tvcap);
	
	dev_dbg(tvcap->dev, "TV capture hardware with enhanced TVTOP interface cleanup completed\n");
}

/*
 * Interrupt handler implementation
 * Enhanced based on Task 022 factory firmware analysis
 */

static void tvcap_handle_frame_done(struct sunxi_tvcap_dev *tvcap)
{
	struct tvcap_buffer *buf;
	struct vb2_v4l2_buffer *vbuf;
	
	if (list_empty(&tvcap->buf_list)) {
		dev_warn(tvcap->dev, "Frame completion but no buffer available\n");
		return;
	}
	
	buf = list_first_entry(&tvcap->buf_list, struct tvcap_buffer, list);
	list_del(&buf->list);
	
	vbuf = &buf->vb;
	vbuf->vb2_buf.timestamp = ktime_get_ns();
	vbuf->sequence = tvcap->sequence++;
	vbuf->field = V4L2_FIELD_NONE;
	
	vb2_buffer_done(&vbuf->vb2_buf, VB2_BUF_STATE_DONE);

	/* Update capture metrics */
	atomic64_inc(&tvcap->metrics.frames_captured_total);
	atomic64_add(tvcap->format.fmt.pix.sizeimage, &tvcap->metrics.bytes_captured_total);
	
	dev_dbg(tvcap->dev, "Frame completed: sequence %d\n", vbuf->sequence);
}

static void tvcap_handle_input_change(struct sunxi_tvcap_dev *tvcap)
{
	u32 status_reg;
	bool was_connected = tvcap->hdmi_connected;
	bool was_detected = tvcap->signal_detected;
	
	/* Use enhanced TVTOP functions for accurate status checking */
	bool new_connected = tvtop_is_hdmi_connected(tvcap);
	bool new_detected = tvtop_is_signal_detected(tvcap);
	
	/* Update status from enhanced detection */
	tvcap->hdmi_connected = new_connected;
	tvcap->signal_detected = new_detected;
	
	if (tvcap->hdmi_connected != was_connected) {
		dev_info(tvcap->dev, "HDMI %s\n", 
			 tvcap->hdmi_connected ? "connected" : "disconnected");
	}
	
	if (tvcap->signal_detected != was_detected) {
		dev_info(tvcap->dev, "HDMI signal %s\n",
			 tvcap->signal_detected ? "detected" : "lost");
		
		/* TODO: Trigger EDID reading and format detection via MIPS */
		/* This will be implemented in MIPS coordination task */
	}
	
	/* Send V4L2 event for input change */
	if (tvcap->hdmi_connected != was_connected || 
	    tvcap->signal_detected != was_detected) {
		struct v4l2_event ev = {
			.type = V4L2_EVENT_SOURCE_CHANGE,
			.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
		};
		v4l2_event_queue(&tvcap->video_dev, &ev);
	}
}

static void tvcap_handle_format_change(struct sunxi_tvcap_dev *tvcap)
{
	u32 format_reg, resolution_reg;
	
	format_reg = tvtop_read(tvcap, TVTOP_FORMAT_REG);
	resolution_reg = tvtop_read(tvcap, TVTOP_RESOLUTION_REG);
	
	dev_info(tvcap->dev, "Format change detected: format=0x%08x, resolution=0x%08x\n",
		 format_reg, resolution_reg);
	
	/* TODO: Parse format and resolution registers */
	/* TODO: Update internal format state */
	/* TODO: Notify applications via V4L2 events */
	
	/* Send V4L2 event for format change */
	struct v4l2_event ev = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
	};
	v4l2_event_queue(&tvcap->video_dev, &ev);
}

static void tvcap_handle_errors(struct sunxi_tvcap_dev *tvcap, u32 error_status)
{
	if (error_status & TVTOP_IRQ_BUF_OVERFLOW) {
		dev_err(tvcap->dev, "Buffer overflow error - dropping frames\n");
	}
	
	if (error_status & TVTOP_IRQ_BUF_UNDERFLOW) {
		dev_err(tvcap->dev, "Buffer underflow error - capture starvation\n");
	}
	
	if (error_status & TVTOP_IRQ_HW_ERROR) {
		dev_err(tvcap->dev, "Hardware error detected\n");
	}
	
	if (error_status & TVTOP_IRQ_DMA_ERROR) {
		dev_err(tvcap->dev, "DMA transfer error\n");
	}
	
	/* Return all pending buffers with error state on serious errors */
	if (error_status & (TVTOP_IRQ_HW_ERROR | TVTOP_IRQ_DMA_ERROR | \
                                 TVTOP_IRQ_FIFO_ERROR | TVTOP_IRQ_TIMEOUT)) {
		struct tvcap_buffer *buf, *tmp;
		
		list_for_each_entry_safe(buf, tmp, &tvcap->buf_list, list) {
			list_del(&buf->list);
			vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		}
	}
}

static irqreturn_t tvcap_irq_handler(int irq, void *dev_id)
{
	struct sunxi_tvcap_dev *tvcap = dev_id;
	u32 status, error_status;
	irqreturn_t ret = IRQ_NONE;
	
	spin_lock(&tvcap->irq_lock);
	
	status = tvtop_read_and_clear_interrupts(tvcap);
	if (!status) {
		spin_unlock(&tvcap->irq_lock);
		return ret;
	}
	
	ret = IRQ_HANDLED;
	
	dev_dbg(tvcap->dev, "TV capture interrupt: 0x%08x\n", status);
	
	/* Handle frame completion */
	if (status & TVTOP_IRQ_FRAME_DONE) {
		tvcap_handle_frame_done(tvcap);
	}
	
	/* Handle input changes */
	if (status & (TVTOP_IRQ_INPUT_CHANGE | TVTOP_IRQ_HDMI_HOTPLUG)) {
		tvcap_handle_input_change(tvcap);
	}
	
	/* Handle format changes */
	if (status & TVTOP_IRQ_FORMAT_CHANGE) {
		tvcap_handle_format_change(tvcap);
	}
	
	/* Handle error conditions */
	error_status = status & TVTOP_IRQ_ALL_ERRORS;
	if (error_status) {
		tvcap_handle_errors(tvcap, error_status);
	}
	
	spin_unlock(&tvcap->irq_lock);
	return ret;
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

	/* Track buffer allocation */
	atomic64_inc(&tvcap->metrics.buffers_allocated_total);
	atomic64_inc(&tvcap->metrics.buffer_queue_depth);
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
	
	/* Initialize clocks */
	ret = tvcap_clocks_init(tvcap);
	if (ret) {
		dev_err(dev, "Failed to initialize clocks: %d\n", ret);
		return ret;
	}
	
	/* Initialize reset controls */
	ret = tvcap_resets_init(tvcap);
	if (ret) {
		dev_err(dev, "Failed to initialize reset controls: %d\n", ret);
		return ret;
	}
	
	/* Request interrupt */
	ret = devm_request_irq(dev, tvcap->irq, tvcap_irq_handler, 
			       IRQF_SHARED | IRQF_TRIGGER_HIGH, SUNXI_TVCAP_NAME, tvcap);
	if (ret) {
		dev_err(dev, "Failed to request interrupt: %d\n", ret);
		return ret;
	}
	
	dev_info(dev, "Resources initialized successfully\n");
	return 0;
}

/*
 * Hardware Capability Detection Functions
 * Based on Task 022 factory firmware analysis
 */

static int tvcap_detect_hardware_capabilities(struct sunxi_tvcap_dev *tvcap)
{
	u32 version_reg, status_reg;
	int ret = 0;
	int i;

	dev_info(tvcap->dev, "Detecting TV capture hardware capabilities\n");

	/* Read hardware version/debug register */
	version_reg = tvtop_read(tvcap, TVTOP_DEBUG_REG);
	dev_info(tvcap->dev, "TVTOP hardware version: 0x%08x\n", version_reg);

	/* Check basic hardware presence */
	status_reg = tvtop_read(tvcap, TVTOP_STATUS_REG);
	if (!(status_reg & TVTOP_STATUS_READY)) {
		dev_err(tvcap->dev, "TVTOP hardware not ready, status=0x%08x\n", status_reg);
		return -EIO;
	}

	/* Test basic register access */
	tvtop_write(tvcap, TVTOP_FORMAT_REG, 0x12345678);
	if (tvtop_read(tvcap, TVTOP_FORMAT_REG) != 0x12345678) {
		dev_err(tvcap->dev, "TVTOP register access test failed\n");
		ret = -EIO;
	}
	tvtop_write(tvcap, TVTOP_FORMAT_REG, 0); /* Clear test pattern */

	/* Probe supported video formats by testing format register values */
	dev_info(tvcap->dev, "Probing supported video formats:\n");
	for (i = 0; i < ARRAY_SIZE(formats); i++) {
		tvtop_write(tvcap, TVTOP_FORMAT_REG, formats[i].tvtop_format);
		if (tvtop_read(tvcap, TVTOP_FORMAT_REG) == formats[i].tvtop_format) {
			dev_info(tvcap->dev, "  - %s (0x%02x): supported\n", 
				 formats[i].name, formats[i].tvtop_format);
		} else {
			dev_warn(tvcap->dev, "  - %s (0x%02x): not supported\n",
				 formats[i].name, formats[i].tvtop_format);
		}
	}
	tvtop_write(tvcap, TVTOP_FORMAT_REG, 0); /* Reset to default */

	/* Test resolution register capabilities */
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0x04380780); /* 1080p test */
	if (tvtop_read(tvcap, TVTOP_RESOLUTION_REG) == 0x04380780) {
		dev_info(tvcap->dev, "High resolution support: 1920x1080 confirmed\n");
	}
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0x02D00500); /* 720p test */
	if (tvtop_read(tvcap, TVTOP_RESOLUTION_REG) == 0x02D00500) {
		dev_info(tvcap->dev, "Standard resolution support: 1280x720 confirmed\n");
	}
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0); /* Reset */

	dev_info(tvcap->dev, "Hardware capability detection completed\n");
	return ret;
}

static int tvcap_probe_hdmi_capabilities(struct sunxi_tvcap_dev *tvcap)
{
	u32 hdmi_ctrl, hdmi_status;
	int ret = 0;

	dev_info(tvcap->dev, "Probing HDMI input capabilities\n");

	/* Enable HDMI subsystem for capability probing */
	hdmi_ctrl = tvtop_read(tvcap, TVTOP_HDMI_CTRL_REG);
	tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, hdmi_ctrl | TVTOP_HDMI_HPD_ENABLE);

	/* Allow HDMI subsystem to stabilize */
	msleep(10);

	/* Read HDMI status */
	hdmi_status = tvtop_read(tvcap, TVTOP_HDMI_STATUS_REG);
	dev_info(tvcap->dev, "HDMI status register: 0x%08x\n", hdmi_status);

	/* Test HDMI control register functionality */
	tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, hdmi_ctrl | TVTOP_HDMI_FORCE_DETECT);
	msleep(1);
	if (tvtop_read(tvcap, TVTOP_HDMI_CTRL_REG) & TVTOP_HDMI_FORCE_DETECT) {
		dev_info(tvcap->dev, "HDMI force detection capability: available\n");
		tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, hdmi_ctrl); /* Restore */
	}

	/* Test EDID read capability */
	tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, hdmi_ctrl | TVTOP_HDMI_EDID_READ);
	msleep(5);
	if (tvtop_read(tvcap, TVTOP_HDMI_CTRL_REG) & TVTOP_HDMI_EDID_READ) {
		dev_info(tvcap->dev, "HDMI EDID read capability: available\n");
	}
	tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, hdmi_ctrl); /* Restore original */

	/* Check current HDMI connection status */
	if (tvtop_is_hdmi_connected(tvcap)) {
		dev_info(tvcap->dev, "HDMI input: connected\n");
		if (tvtop_is_signal_detected(tvcap)) {
			dev_info(tvcap->dev, "HDMI signal: detected\n");
		} else {
			dev_info(tvcap->dev, "HDMI signal: not detected\n");
		}
	} else {
		dev_info(tvcap->dev, "HDMI input: not connected\n");
	}

	dev_info(tvcap->dev, "HDMI capability probing completed\n");
	return ret;
}

static int tvcap_verify_clock_initialization(struct sunxi_tvcap_dev *tvcap)
{
	int i;
	unsigned long rate;

	dev_dbg(tvcap->dev, "Verifying TV capture clock initialization\n");

	for (i = 0; i < TVCAP_CLK_COUNT; i++) {
		if (!__clk_is_enabled(tvcap->clks[i].clk)) {
			dev_err(tvcap->dev, "Clock '%s' is not enabled\n", 
				tvcap->clks[i].id);
			return -EIO;
		}

		rate = clk_get_rate(tvcap->clks[i].clk);
		dev_dbg(tvcap->dev, "Clock '%s': enabled, rate=%lu Hz\n",
			tvcap->clks[i].id, rate);
	}

	dev_info(tvcap->dev, "Clock initialization verification: passed\n");
	return 0;
}

static int tvcap_verify_reset_initialization(struct sunxi_tvcap_dev *tvcap)
{
	u32 status_reg;

	dev_dbg(tvcap->dev, "Verifying TV capture reset initialization\n");

	/* Check that hardware is out of reset and functional */
	status_reg = tvtop_read(tvcap, TVTOP_STATUS_REG);
	if (!(status_reg & TVTOP_STATUS_READY)) {
		dev_err(tvcap->dev, "Hardware not ready after reset, status=0x%08x\n", 
			status_reg);
		return -EIO;
	}

	/* Verify we can access control registers */
	if (tvtop_read(tvcap, TVTOP_CTRL_REG) == 0xFFFFFFFF) {
		dev_err(tvcap->dev, "Control register access failed after reset\n");
		return -EIO;
	}

	dev_info(tvcap->dev, "Reset initialization verification: passed\n");
	return 0;
}

static int tvcap_verify_hardware_initialization(struct sunxi_tvcap_dev *tvcap)
{
	u32 ctrl_reg, status_reg, irq_en_reg;
	u32 test_pattern = 0xA5A5A5A5;
	int ret;

	dev_info(tvcap->dev, "Verifying complete hardware initialization\n");

	/* Verify clock initialization */
	ret = tvcap_verify_clock_initialization(tvcap);
	if (ret)
		return ret;

	/* Verify reset initialization */
	ret = tvcap_verify_reset_initialization(tvcap);
	if (ret)
		return ret;

	/* Verify TVTOP subsystem is enabled */
	ctrl_reg = tvtop_read(tvcap, TVTOP_CTRL_REG);
	if (!(ctrl_reg & TVTOP_CTRL_ENABLE)) {
		dev_err(tvcap->dev, "TVTOP subsystem not enabled, ctrl=0x%08x\n", ctrl_reg);
		return -EIO;
	}

	/* Verify HDMI input is enabled */
	if (!(ctrl_reg & TVTOP_CTRL_HDMI_EN)) {
		dev_err(tvcap->dev, "HDMI input not enabled, ctrl=0x%08x\n", ctrl_reg);
		return -EIO;
	}

	/* Verify hardware status is ready */
	status_reg = tvtop_read(tvcap, TVTOP_STATUS_REG);
	if (!(status_reg & TVTOP_STATUS_READY)) {
		dev_err(tvcap->dev, "Hardware not ready, status=0x%08x\n", status_reg);
		return -EIO;
	}

	/* Verify interrupts are properly configured */
	irq_en_reg = tvtop_read(tvcap, TVTOP_IRQ_EN_REG);
	if ((irq_en_reg & (TVTOP_IRQ_ALL_EVENTS | TVTOP_IRQ_ALL_ERRORS)) == 0) {
		dev_warn(tvcap->dev, "No interrupts enabled, irq_en=0x%08x\n", irq_en_reg);
	}

	/* Verify register access integrity */
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, test_pattern);
	if (tvtop_read(tvcap, TVTOP_RESOLUTION_REG) != test_pattern) {
		dev_err(tvcap->dev, "Register access integrity check failed\n");
		tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0); /* Clear */
		return -EIO;
	}
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0); /* Clear test pattern */

	/* Check for any hardware error conditions */
	if (status_reg & TVTOP_STATUS_ERROR) {
		dev_err(tvcap->dev, "Hardware error detected during verification, status=0x%08x\n",
			status_reg);
		return -EIO;
	}

	/* Verify DMA subsystem readiness */
	if (status_reg & TVTOP_STATUS_DMA_BUSY) {
		dev_warn(tvcap->dev, "DMA busy during initialization verification\n");
	}

	dev_info(tvcap->dev, "Hardware initialization verification: all checks passed\n");
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

	/* Initialize metrics */
	tvcap_metrics_init(tvcap);
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

	/* Update streaming metrics */
	atomic_set(&tvcap->metrics.streaming_active, 0);
	return ret;
}

static int sunxi_tvcap_remove(struct platform_device *pdev)
{
	struct sunxi_tvcap_dev *tvcap = platform_get_drvdata(pdev);
	
	dev_info(&pdev->dev, "Removing TV Capture driver\n");
	
	tvcap_cleanup_v4l2(tvcap);
	tvcap_hw_cleanup(tvcap);

	/* Update streaming metrics */
	atomic_set(&tvcap->metrics.streaming_active, 0);
	
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
MODULE_ALIAS("platform:" SUNXI_TVCAP_NAME);=== HY300 Projector Development Environment ===
Cross-compile toolchain: aarch64-unknown-linux-gnu-
Target architecture: arm64
Sunxi tools available: sunxi-fel, sunxi-fexc, etc.

Key tools installed:
- Cross-compilation: aarch64-unknown-linux-gnu-gcc
- Sunxi tools: sunxi-fel, sunxi-fexc
- Firmware analysis: binwalk, hexdump, strings
- Serial console: minicom, picocom
- Device tree: dtc

ROM analysis workflow:
1. Extract firmware: binwalk -e firmware.img
2. FEL access: sunxi-fel version
3. Backup eMMC: sunxi-fel read 0x0 0x1000000 backup.img


/*
 * TVTOP Register Access Functions (Enhanced Implementation based on Task 022)
 */

static inline u32 tvtop_read(struct sunxi_tvcap_dev *tvcap, u32 reg)
{
	return readl(tvcap->regs + reg);
}

static inline void tvtop_write(struct sunxi_tvcap_dev *tvcap, u32 reg, u32 val)
{
	writel(val, tvcap->regs + reg);
	atomic64_inc(&tvcap->metrics.register_access_total);
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

static inline void tvtop_update_bits(struct sunxi_tvcap_dev *tvcap, u32 reg, u32 mask, u32 val)
{
	u32 current = tvtop_read(tvcap, reg);
	tvtop_write(tvcap, reg, (current & ~mask) | (val & mask));
}

/*
 * TVTOP Hardware Control Functions (Factory Driver Patterns from Task 022)
 */

static int tvtop_wait_for_ready(struct sunxi_tvcap_dev *tvcap)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(TVTOP_TIMEOUT_MS);
	u32 status;

	do {
		status = tvtop_read(tvcap, TVTOP_STATUS_REG);
		if (status & TVTOP_STATUS_READY)
			return 0;
		
		usleep_range(100, 200);
	} while (time_before(jiffies, timeout));

	dev_err(tvcap->dev, "TVTOP wait for ready timeout, status=0x%08x\n", status);
	return -ETIMEDOUT;
}

static int tvtop_software_reset(struct sunxi_tvcap_dev *tvcap)
{
	int ret;

	dev_dbg(tvcap->dev, "Performing TVTOP software reset\n");

	/* Assert software reset */
	tvtop_set_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_RESET);
	
	/* Hold reset for required duration */
	usleep_range(TVTOP_RESET_DELAY_US, TVTOP_RESET_DELAY_US * 2);
	
	/* Deassert software reset */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_RESET);
	
	/* Wait for hardware to become ready */
	usleep_range(TVTOP_STABILIZE_DELAY_US, TVTOP_STABILIZE_DELAY_US * 2);
	
	ret = tvtop_wait_for_ready(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP failed to become ready after reset\n");
		return ret;
	}

	dev_info(tvcap->dev, "TVTOP software reset completed successfully\n");
	return 0;
}

static int tvtop_enable_subsystem(struct sunxi_tvcap_dev *tvcap)
{
	u32 ctrl_val = 0;
	int ret;

	dev_dbg(tvcap->dev, "Enabling TVTOP subsystem\n");

	/* Build control register value based on factory patterns */
	ctrl_val |= TVTOP_CTRL_ENABLE;      /* Enable TV TOP */
	ctrl_val |= TVTOP_CTRL_HDMI_EN;     /* Enable HDMI input */
	ctrl_val |= TVTOP_CTRL_AUTO_FORMAT; /* Enable auto format detection */

	/* Write control register */
	tvtop_write(tvcap, TVTOP_CTRL_REG, ctrl_val);

	/* Verify subsystem enabled */
	ret = tvtop_wait_for_ready(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP subsystem enable failed\n");
		return ret;
	}

	/* Enable HDMI hot-plug detection */
	tvtop_set_bits(tvcap, TVTOP_HDMI_CTRL_REG, TVTOP_HDMI_HPD_ENABLE);

	dev_info(tvcap->dev, "TVTOP subsystem enabled successfully\n");
	return 0;
}

static void tvtop_disable_subsystem(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Disabling TVTOP subsystem\n");

	/* Disable capture first */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_CAPTURE_EN | TVTOP_CTRL_DMA_EN);
	
	/* Disable HDMI input */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_HDMI_EN);
	tvtop_clear_bits(tvcap, TVTOP_HDMI_CTRL_REG, TVTOP_HDMI_HPD_ENABLE);
	
	/* Disable entire subsystem */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_ENABLE);

	dev_dbg(tvcap->dev, "TVTOP subsystem disabled\n");
}

static int tvtop_configure_format(struct sunxi_tvcap_dev *tvcap, const struct tvcap_format *fmt, 
                                  u32 width, u32 height)
{
	u32 resolution;

	dev_dbg(tvcap->dev, "Configuring TVTOP format: %s %dx%d\n", 
		fmt->name, width, height);

	/* Set video format in TVTOP register */
	tvtop_write(tvcap, TVTOP_FORMAT_REG, fmt->tvtop_format);

	/* Set resolution (height in upper 16 bits, width in lower 16 bits) */
	resolution = (height << 16) | width;
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, resolution);

	/* Update capture frame size */
	tvtop_write(tvcap, TVTOP_CAPTURE_SIZE_REG, resolution);

	/* Store current configuration */
	tvcap->current_format = fmt->tvtop_format;
	tvcap->current_resolution = resolution;

	dev_info(tvcap->dev, "TVTOP format configured: format=0x%02x, resolution=0x%08x\n",
		 fmt->tvtop_format, resolution);
	return 0;
}

static int tvtop_start_capture(struct sunxi_tvcap_dev *tvcap, dma_addr_t dma_addr, u32 size)
{
	u32 status;
	int ret;

	dev_dbg(tvcap->dev, "Starting TVTOP capture: dma=0x%llx, size=%u\n", 
		(unsigned long long)dma_addr, size);

	/* Check if HDMI input is available */
	status = tvtop_read(tvcap, TVTOP_STATUS_REG);
	if (!(status & TVTOP_STATUS_HDMI_CONNECTED)) {
		dev_warn(tvcap->dev, "HDMI not connected, capture may fail\n");
	}
	if (!(status & TVTOP_STATUS_SIGNAL_DETECTED)) {
		dev_warn(tvcap->dev, "No HDMI signal detected, capture may fail\n");
	}

	/* Configure DMA buffer */
	tvtop_write(tvcap, TVTOP_DMA_ADDR_REG, lower_32_bits(dma_addr));
	tvtop_write(tvcap, TVTOP_DMA_SIZE_REG, size);

	/* Enable DMA */
	tvtop_set_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_DMA_EN);
	tvtop_set_bits(tvcap, TVTOP_DMA_CTRL_REG, BIT(0)); /* DMA start */

	/* Enable capture */
	tvtop_set_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_CAPTURE_EN);

	/* Verify capture started */
	ret = tvtop_wait_for_ready(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP capture start failed\n");
		tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_CAPTURE_EN | TVTOP_CTRL_DMA_EN);
		return ret;
	}

	dev_info(tvcap->dev, "TVTOP capture started successfully\n");
	return 0;
}

static void tvtop_stop_capture(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Stopping TVTOP capture\n");

	/* Disable capture */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_CAPTURE_EN);
	
	/* Disable DMA */
	tvtop_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_DMA_EN);
	tvtop_clear_bits(tvcap, TVTOP_DMA_CTRL_REG, BIT(0));

	/* Clear DMA configuration */
	tvtop_write(tvcap, TVTOP_DMA_ADDR_REG, 0);
	tvtop_write(tvcap, TVTOP_DMA_SIZE_REG, 0);

	dev_dbg(tvcap->dev, "TVTOP capture stopped\n");
}

static bool tvtop_is_hdmi_connected(struct sunxi_tvcap_dev *tvcap)
{
	u32 status = tvtop_read(tvcap, TVTOP_STATUS_REG);
	return !!(status & TVTOP_STATUS_HDMI_CONNECTED);
}

static bool tvtop_is_signal_detected(struct sunxi_tvcap_dev *tvcap)
{
	u32 status = tvtop_read(tvcap, TVTOP_STATUS_REG);
	return !!(status & TVTOP_STATUS_SIGNAL_DETECTED);
}

static u32 tvtop_get_hdmi_status(struct sunxi_tvcap_dev *tvcap)
{
	return tvtop_read(tvcap, TVTOP_HDMI_STATUS_REG);
}

/*
 * TVTOP Interrupt Management (Enhanced Implementation)
 */

static void tvtop_enable_interrupts(struct sunxi_tvcap_dev *tvcap, u32 mask)
{
	u32 current_mask = tvtop_read(tvcap, TVTOP_IRQ_EN_REG);
	tvtop_write(tvcap, TVTOP_IRQ_EN_REG, current_mask | mask);
	
	dev_dbg(tvcap->dev, "TVTOP interrupts enabled: 0x%08x\n", current_mask | mask);
}

static void tvtop_disable_interrupts(struct sunxi_tvcap_dev *tvcap, u32 mask)
{
	u32 current_mask = tvtop_read(tvcap, TVTOP_IRQ_EN_REG);
	tvtop_write(tvcap, TVTOP_IRQ_EN_REG, current_mask & ~mask);
	
	dev_dbg(tvcap->dev, "TVTOP interrupts disabled: 0x%08x\n", current_mask & ~mask);
}

static u32 tvtop_read_and_clear_interrupts(struct sunxi_tvcap_dev *tvcap)
{
	u32 status = tvtop_read(tvcap, TVTOP_IRQ_STATUS_REG);
	if (status) {
		tvtop_write(tvcap, TVTOP_IRQ_STATUS_REG, status);
		dev_dbg(tvcap->dev, "TVTOP interrupts cleared: 0x%08x\n", status);
	}
	return status;
}

/**
 * tvtop_init_hardware - Comprehensive TVTOP hardware initialization
 * @tvcap: TV capture device context
 *
 * Performs complete TVTOP subsystem initialization including:
 * - Hardware reset sequence
 * - Clock and power configuration  
 * - Register initialization to factory defaults
 * - HDMI interface setup
 * - Interrupt system configuration
 *
 * Based on factory firmware analysis from Task 022.
 *
 * Returns: 0 on success, negative error code on failure
 */
static int tvtop_init_hardware(struct sunxi_tvcap_dev *tvcap)
{
	u32 version_reg, ctrl_reg;
	int ret;

	dev_info(tvcap->dev, "Initializing TVTOP hardware interface\n");

	/* Step 1: Perform software reset to ensure clean state */
	ret = tvtop_software_reset(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP software reset failed: %d\n", ret);
		return ret;
	}

	/* Step 2: Verify hardware presence and version */
	version_reg = tvtop_read(tvcap, TVTOP_DEBUG_REG);
	dev_info(tvcap->dev, "TVTOP hardware version: 0x%08x\n", version_reg);

	/* Step 3: Initialize clock control registers */
	tvtop_write(tvcap, TVTOP_CLK_CTRL_REG, 0x00000001); /* Enable core clocks */
	
	/* Step 4: Configure reset control registers */
	tvtop_write(tvcap, TVTOP_RST_CTRL_REG, 0x00000000); /* Release internal resets */
	
	/* Step 5: Initialize control register with safe defaults */
	ctrl_reg = TVTOP_CTRL_ENABLE | TVTOP_CTRL_AUTO_FORMAT;
	tvtop_write(tvcap, TVTOP_CTRL_REG, ctrl_reg);

	/* Step 6: Configure default capture parameters */
	tvtop_write(tvcap, TVTOP_FORMAT_REG, 0x04); /* Default to YUYV format */
	tvtop_write(tvcap, TVTOP_RESOLUTION_REG, 0x04380780); /* Default 1920x1080 */
	tvtop_write(tvcap, TVTOP_CAPTURE_SIZE_REG, 0x04380780); /* Match resolution */

	/* Step 7: Initialize DMA control registers */
	tvtop_write(tvcap, TVTOP_DMA_CTRL_REG, 0x00000000); /* DMA disabled initially */
	tvtop_write(tvcap, TVTOP_DMA_ADDR_REG, 0x00000000); /* Clear DMA address */
	tvtop_write(tvcap, TVTOP_DMA_SIZE_REG, 0x00000000); /* Clear DMA size */

	/* Step 8: Configure HDMI interface */
	tvtop_write(tvcap, TVTOP_HDMI_CTRL_REG, TVTOP_HDMI_HPD_ENABLE);
	
	/* Step 9: Clear all pending interrupts */
	tvtop_write(tvcap, TVTOP_IRQ_STATUS_REG, 0xFFFFFFFF);
	
	/* Step 10: Configure interrupt mask - enable essential interrupts */
	tvtop_write(tvcap, TVTOP_IRQ_EN_REG, TVTOP_IRQ_ALL_EVENTS | TVTOP_IRQ_ALL_ERRORS);
	tvtop_write(tvcap, TVTOP_IRQ_MASK_REG, 0x00000000); /* Unmask all enabled interrupts */

	/* Step 11: Enable TVTOP subsystem */
	ret = tvtop_enable_subsystem(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP subsystem enable failed: %d\n", ret);
		return ret;
	}

	/* Step 12: Verify hardware is responsive */
	ret = tvtop_wait_for_ready(tvcap);
	if (ret) {
		dev_err(tvcap->dev, "TVTOP hardware not ready after init: %d\n", ret);
		return ret;
	}

	/* Step 13: Final status check and mark as initialized */
	ctrl_reg = tvtop_read(tvcap, TVTOP_CTRL_REG);
	tvcap->tvtop_initialized = true;
	dev_info(tvcap->dev, "TVTOP initialized successfully, CTRL=0x%08x\n", ctrl_reg);

	return 0;
}



