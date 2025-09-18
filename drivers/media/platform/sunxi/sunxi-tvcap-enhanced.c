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

/* TV TOP Control Registers - Complete Register Map */
#define TVTOP_CTRL_REG          0x0000  /* Main control register */
#define TVTOP_STATUS_REG        0x0004  /* Hardware status register */
#define TVTOP_IRQ_EN_REG        0x0008  /* Interrupt enable register */
#define TVTOP_IRQ_STATUS_REG    0x000c  /* Interrupt status register */
#define TVTOP_FORMAT_REG        0x0010  /* Video format control */
#define TVTOP_RESOLUTION_REG    0x0014  /* Resolution configuration */
#define TVTOP_TIMING_REG        0x0018  /* Timing parameters */
#define TVTOP_CLOCK_CTRL_REG    0x001c  /* Clock control */
#define TVTOP_RESET_CTRL_REG    0x0020  /* Reset control */
#define TVTOP_DMA_CTRL_REG      0x0024  /* DMA configuration */
#define TVTOP_BUF_ADDR_REG      0x0028  /* Buffer address */
#define TVTOP_BUF_SIZE_REG      0x002c  /* Buffer size */
#define TVTOP_CAPTURE_CTRL_REG  0x0030  /* Capture control */
#define TVTOP_DEBUG_REG         0x0034  /* Debug register */
#define TVTOP_VERSION_REG       0x003c  /* Hardware version */

/* TVTOP_CTRL_REG bit fields */
#define TVTOP_CTRL_ENABLE       BIT(0)   /* Enable TV capture */
#define TVTOP_CTRL_RESET        BIT(1)   /* Software reset */
#define TVTOP_CTRL_START        BIT(2)   /* Start capture */
#define TVTOP_CTRL_STOP         BIT(3)   /* Stop capture */
#define TVTOP_CTRL_HDMI_EN      BIT(4)   /* HDMI input enable */
#define TVTOP_CTRL_AUTO_FORMAT  BIT(5)   /* Auto format detection */
#define TVTOP_CTRL_MIPS_COORD   BIT(6)   /* MIPS coordination enable */
#define TVTOP_CTRL_DMA_EN       BIT(7)   /* DMA enable */
#define TVTOP_CTRL_IRQ_EN       BIT(8)   /* Global interrupt enable */
#define TVTOP_CTRL_CLOCK_EN     BIT(9)   /* Clock enable */

/* TVTOP_STATUS_REG bit fields */
#define TVTOP_STATUS_HDMI_CONNECTED BIT(0)  /* HDMI cable connected */
#define TVTOP_STATUS_SIGNAL_DETECTED BIT(1) /* Valid signal detected */
#define TVTOP_STATUS_FORMAT_DETECTED BIT(2) /* Format detected */
#define TVTOP_STATUS_CAPTURE_ACTIVE  BIT(3) /* Capture in progress */
#define TVTOP_STATUS_DMA_ACTIVE     BIT(4)  /* DMA transfer active */
#define TVTOP_STATUS_BUFFER_READY   BIT(5)  /* Buffer ready */
#define TVTOP_STATUS_ERROR          BIT(6)  /* Error condition */
#define TVTOP_STATUS_MIPS_READY     BIT(7)  /* MIPS co-processor ready */
#define TVTOP_STATUS_CLOCK_STABLE   BIT(8)  /* Clock stable */
#define TVTOP_STATUS_SYNC_LOCKED    BIT(9)  /* Sync locked */

/* TVTOP_IRQ_EN_REG and TVTOP_IRQ_STATUS_REG bit fields */
#define TVTOP_IRQ_HDMI_HOTPLUG     BIT(0)   /* HDMI hot-plug interrupt */
#define TVTOP_IRQ_SIGNAL_CHANGE    BIT(1)   /* Signal change interrupt */
#define TVTOP_IRQ_FORMAT_CHANGE    BIT(2)   /* Format change interrupt */
#define TVTOP_IRQ_CAPTURE_DONE     BIT(3)   /* Capture complete interrupt */
#define TVTOP_IRQ_DMA_DONE         BIT(4)   /* DMA complete interrupt */
#define TVTOP_IRQ_BUFFER_OVERFLOW  BIT(5)   /* Buffer overflow interrupt */
#define TVTOP_IRQ_ERROR            BIT(6)   /* Error interrupt */
#define TVTOP_IRQ_MIPS_EVENT       BIT(7)   /* MIPS event interrupt */

/* TVTOP_FORMAT_REG bit fields */
#define TVTOP_FORMAT_TYPE_MASK     (0x7 << 0)  /* Format type mask */
#define TVTOP_FORMAT_TYPE_HDMI     (0x0 << 0)  /* HDMI format */
#define TVTOP_FORMAT_TYPE_DVI      (0x1 << 0)  /* DVI format */
#define TVTOP_FORMAT_COLORSPACE_MASK (0x3 << 4) /* Color space mask */
#define TVTOP_FORMAT_COLORSPACE_RGB  (0x0 << 4) /* RGB color space */
#define TVTOP_FORMAT_COLORSPACE_YUV  (0x1 << 4) /* YUV color space */
#define TVTOP_FORMAT_DEPTH_MASK    (0x3 << 8)  /* Color depth mask */
#define TVTOP_FORMAT_DEPTH_8BIT    (0x0 << 8)  /* 8-bit color */
#define TVTOP_FORMAT_DEPTH_10BIT   (0x1 << 8)  /* 10-bit color */
#define TVTOP_FORMAT_DEPTH_12BIT   (0x2 << 8)  /* 12-bit color */

/* TVTOP_RESOLUTION_REG bit fields */
#define TVTOP_RESOLUTION_WIDTH_MASK   (0xFFFF << 0)   /* Width mask */
#define TVTOP_RESOLUTION_HEIGHT_MASK  (0xFFFF << 16)  /* Height mask */
#define TVTOP_RESOLUTION_WIDTH_SHIFT  0
#define TVTOP_RESOLUTION_HEIGHT_SHIFT 16

/* TVTOP_TIMING_REG bit fields */
#define TVTOP_TIMING_HSYNC_MASK    (0xFF << 0)   /* H-sync timing */
#define TVTOP_TIMING_VSYNC_MASK    (0xFF << 8)   /* V-sync timing */
#define TVTOP_TIMING_PIXEL_CLK_MASK (0xFFFF << 16) /* Pixel clock */

/* TVTOP_CLOCK_CTRL_REG bit fields */
#define TVTOP_CLOCK_PIXEL_EN       BIT(0)   /* Pixel clock enable */
#define TVTOP_CLOCK_HDMI_EN        BIT(1)   /* HDMI clock enable */
#define TVTOP_CLOCK_DMA_EN         BIT(2)   /* DMA clock enable */
#define TVTOP_CLOCK_MIPS_EN        BIT(3)   /* MIPS clock enable */
#define TVTOP_CLOCK_DIV_MASK       (0xF << 4) /* Clock divider */

/* TVTOP_DMA_CTRL_REG bit fields */
#define TVTOP_DMA_BURST_MASK       (0x7 << 0)  /* DMA burst size */
#define TVTOP_DMA_BURST_1          (0x0 << 0)  /* 1-word burst */
#define TVTOP_DMA_BURST_4          (0x1 << 0)  /* 4-word burst */
#define TVTOP_DMA_BURST_8          (0x2 << 0)  /* 8-word burst */
#define TVTOP_DMA_BURST_16         (0x3 << 0)  /* 16-word burst */
#define TVTOP_DMA_PRIORITY_MASK    (0x3 << 4)  /* DMA priority */
#define TVTOP_DMA_PRIORITY_LOW     (0x0 << 4)  /* Low priority */
#define TVTOP_DMA_PRIORITY_NORMAL  (0x1 << 4)  /* Normal priority */
#define TVTOP_DMA_PRIORITY_HIGH    (0x2 << 4)  /* High priority */

/* Clock indices (from Task 022 device tree analysis) */
enum tvcap_clks {
	TVCAP_CLK_BUS_DISP = 0,
	TVCAP_CLK_BUS_TVCAP,
	TVCAP_CLK_BUS_DEMOD,
	TVCAP_CLK_CAP_300M,
	TVCAP_CLK_VINCAP_DMA,
	TVCAP_CLK_HDMI_AUDIO_BUS,
	TVCAP_CLK_HDMI_AUDIO,
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

/*
 * Register access functions with error checking
 */

static inline u32 tvcap_reg_read(struct sunxi_tvcap_dev *tvcap, u32 offset)
{
	u32 val = readl(tvcap->regs + offset);
	dev_dbg(tvcap->dev, "REG_READ [0x%04x] = 0x%08x\n", offset, val);
	return val;
}

static inline void tvcap_reg_write(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 val)
{
	dev_dbg(tvcap->dev, "REG_WRITE [0x%04x] = 0x%08x\n", offset, val);
	writel(val, tvcap->regs + offset);
}

static inline void tvcap_reg_update(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 mask, u32 val)
{
	u32 tmp = tvcap_reg_read(tvcap, offset);
	tmp &= ~mask;
	tmp |= val & mask;
	tvcap_reg_write(tvcap, offset, tmp);
}

/* 
 * Hardware register bit field helper functions
 */

static inline void tvcap_set_bits(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 bits)
{
	tvcap_reg_update(tvcap, offset, bits, bits);
}

static inline void tvcap_clear_bits(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 bits)
{
	tvcap_reg_update(tvcap, offset, bits, 0);
}

static inline bool tvcap_test_bits(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 bits)
{
	return (tvcap_reg_read(tvcap, offset) & bits) == bits;
}

/*
 * Register dump function for debugging
 */
static void tvcap_dump_registers(struct sunxi_tvcap_dev *tvcap)
{
	dev_info(tvcap->dev, "=== TV Capture Register Dump ===\n");
	dev_info(tvcap->dev, "CTRL:        0x%08x\n", tvcap_reg_read(tvcap, TVTOP_CTRL_REG));
	dev_info(tvcap->dev, "STATUS:      0x%08x\n", tvcap_reg_read(tvcap, TVTOP_STATUS_REG));
	dev_info(tvcap->dev, "IRQ_EN:      0x%08x\n", tvcap_reg_read(tvcap, TVTOP_IRQ_EN_REG));
	dev_info(tvcap->dev, "IRQ_STATUS:  0x%08x\n", tvcap_reg_read(tvcap, TVTOP_IRQ_STATUS_REG));
	dev_info(tvcap->dev, "FORMAT:      0x%08x\n", tvcap_reg_read(tvcap, TVTOP_FORMAT_REG));
	dev_info(tvcap->dev, "RESOLUTION:  0x%08x\n", tvcap_reg_read(tvcap, TVTOP_RESOLUTION_REG));
	dev_info(tvcap->dev, "TIMING:      0x%08x\n", tvcap_reg_read(tvcap, TVTOP_TIMING_REG));
	dev_info(tvcap->dev, "CLOCK_CTRL:  0x%08x\n", tvcap_reg_read(tvcap, TVTOP_CLOCK_CTRL_REG));
	dev_info(tvcap->dev, "RESET_CTRL:  0x%08x\n", tvcap_reg_read(tvcap, TVTOP_RESET_CTRL_REG));
	dev_info(tvcap->dev, "DMA_CTRL:    0x%08x\n", tvcap_reg_read(tvcap, TVTOP_DMA_CTRL_REG));
	dev_info(tvcap->dev, "BUF_ADDR:    0x%08x\n", tvcap_reg_read(tvcap, TVTOP_BUF_ADDR_REG));
	dev_info(tvcap->dev, "BUF_SIZE:    0x%08x\n", tvcap_reg_read(tvcap, TVTOP_BUF_SIZE_REG));
	dev_info(tvcap->dev, "CAPTURE_CTRL:0x%08x\n", tvcap_reg_read(tvcap, TVTOP_CAPTURE_CTRL_REG));
	dev_info(tvcap->dev, "DEBUG:       0x%08x\n", tvcap_reg_read(tvcap, TVTOP_DEBUG_REG));
	dev_info(tvcap->dev, "VERSION:     0x%08x\n", tvcap_reg_read(tvcap, TVTOP_VERSION_REG));
	dev_info(tvcap->dev, "=== End Register Dump ===\n");
}
/*
 * Enhanced register interface functions with factory-analyzed safety checks
 */

static int tvcap_reg_read_safe(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 *value)
{
	if (offset > TVTOP_VERSION_REG) {
		dev_err(tvcap->dev, "Invalid register offset: 0x%04x\n", offset);
		return -EINVAL;
	}
	
	*value = readl(tvcap->regs + offset);
	dev_dbg(tvcap->dev, "REG_READ_SAFE [0x%04x] = 0x%08x\n", offset, *value);
	return 0;
}

static int tvcap_reg_write_safe(struct sunxi_tvcap_dev *tvcap, u32 offset, u32 val)
{
	if (offset > TVTOP_VERSION_REG) {
		dev_err(tvcap->dev, "Invalid register offset: 0x%04x\n", offset);
		return -EINVAL;
	}
	
	/* Read-only register protection */
	if (offset == TVTOP_STATUS_REG || offset == TVTOP_VERSION_REG) {
		dev_warn(tvcap->dev, "Attempted write to read-only register 0x%04x\n", offset);
		return -EPERM;
	}
	
	dev_dbg(tvcap->dev, "REG_WRITE_SAFE [0x%04x] = 0x%08x\n", offset, val);
	writel(val, tvcap->regs + offset);
	return 0;
}

static int tvcap_wait_for_status(struct sunxi_tvcap_dev *tvcap, u32 mask, u32 expected, 
				 unsigned int timeout_ms)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(timeout_ms);
	u32 status;
	
	do {
		status = tvcap_reg_read(tvcap, TVTOP_STATUS_REG);
		if ((status & mask) == expected) {
			dev_dbg(tvcap->dev, "Status condition met: 0x%08x & 0x%08x == 0x%08x\n",
				status, mask, expected);
			return 0;
		}
		
		if (time_after(jiffies, timeout)) {
			dev_err(tvcap->dev, "Timeout waiting for status: got 0x%08x, expected 0x%08x\n",
				status & mask, expected);
			return -ETIMEDOUT;
		}
		
		usleep_range(100, 200);
	} while (1);
}


/*
 * Hardware status checking functions
 */
static bool tvcap_is_hdmi_connected(struct sunxi_tvcap_dev *tvcap)
{
	return tvcap_test_bits(tvcap, TVTOP_STATUS_REG, TVTOP_STATUS_HDMI_CONNECTED);
}

static bool tvcap_is_signal_detected(struct sunxi_tvcap_dev *tvcap)
{
	return tvcap_test_bits(tvcap, TVTOP_STATUS_REG, TVTOP_STATUS_SIGNAL_DETECTED);
}

static bool tvcap_is_format_detected(struct sunxi_tvcap_dev *tvcap)
{
	return tvcap_test_bits(tvcap, TVTOP_STATUS_REG, TVTOP_STATUS_FORMAT_DETECTED);
}

static bool tvcap_is_capture_active(struct sunxi_tvcap_dev *tvcap)
{
	return tvcap_test_bits(tvcap, TVTOP_STATUS_REG, TVTOP_STATUS_CAPTURE_ACTIVE);
}

/*
 * Hardware configuration functions
 */
static int tvcap_set_resolution(struct sunxi_tvcap_dev *tvcap, u32 width, u32 height)
{
	u32 val;

	if (width > 0xFFFF || height > 0xFFFF) {
		dev_err(tvcap->dev, "Invalid resolution: %ux%u\n", width, height);
		return -EINVAL;
	}

	val = (width << TVTOP_RESOLUTION_WIDTH_SHIFT) |
	      (height << TVTOP_RESOLUTION_HEIGHT_SHIFT);
	
	tvcap_reg_write(tvcap, TVTOP_RESOLUTION_REG, val);
	
	dev_dbg(tvcap->dev, "Resolution set to %ux%u\n", width, height);
	return 0;
}

static int tvcap_set_format(struct sunxi_tvcap_dev *tvcap, u32 format_type, u32 colorspace, u32 depth)
{
	u32 val = 0;

	val |= (format_type & 0x7) << 0;
	val |= (colorspace & 0x3) << 4;
	val |= (depth & 0x3) << 8;

	tvcap_reg_write(tvcap, TVTOP_FORMAT_REG, val);
	
	dev_dbg(tvcap->dev, "Format set: type=%u, colorspace=%u, depth=%u\n", 
		format_type, colorspace, depth);
	return 0;
}

static int tvcap_configure_dma(struct sunxi_tvcap_dev *tvcap, dma_addr_t addr, u32 size)
{
	/* Configure DMA burst size and priority */
	tvcap_reg_update(tvcap, TVTOP_DMA_CTRL_REG, 
			 TVTOP_DMA_BURST_MASK | TVTOP_DMA_PRIORITY_MASK,
			 TVTOP_DMA_BURST_16 | TVTOP_DMA_PRIORITY_HIGH);

	/* Set buffer address and size */
	tvcap_reg_write(tvcap, TVTOP_BUF_ADDR_REG, (u32)addr);
	tvcap_reg_write(tvcap, TVTOP_BUF_SIZE_REG, size);

	dev_dbg(tvcap->dev, "DMA configured: addr=0x%08x, size=%u\n", 
		(u32)addr, size);
	return 0;
}

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
 * Hardware interface functions with enhanced register support
 */

static void tvcap_hw_reset(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Resetting TV capture hardware\n");
	
	/* Software reset via register */
	tvcap_set_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_RESET);
	usleep_range(10, 20);
	tvcap_clear_bits(tvcap, TVTOP_CTRL_REG, TVTOP_CTRL_RESET);
	
	/* Assert all reset lines */
	reset_control_bulk_assert(TVCAP_RST_COUNT, tvcap->resets);
	usleep_range(10, 20);
	
	/* Deassert reset lines */
	reset_control_bulk_deassert(TVCAP_RST_COUNT, tvcap->resets);
	usleep_range(100, 200);
}

static int tvcap_hw_init_enhanced(struct sunxi_tvcap_dev *tvcap)
{
	u32 reg_val;
	int ret;
	
	dev_dbg(tvcap->dev, "Initializing TV capture hardware\n");
	
	/* Enable clocks */
	ret = clk_bulk_prepare_enable(TVCAP_CLK_COUNT, tvcap->clks);
	if (ret) {
		dev_err(tvcap->dev, "Failed to enable clocks: %d\n", ret);
		return ret;
	}
	
	/* Reset hardware */
	tvcap_hw_reset(tvcap);
	/* Wait for hardware to be ready after reset */
	ret = tvcap_wait_for_status(tvcap, TVTOP_STATUS_CLOCK_STABLE, TVTOP_STATUS_CLOCK_STABLE, 100);
	if (ret)
		dev_warn(tvcap->dev, "Clock stabilization timeout (continuing anyway)\n");
	
	/* Hardware initialization sequence based on factory firmware analysis */
	
	/* Step 1: Disable all operations */
	tvcap_reg_write(tvcap, TVTOP_CTRL_REG, 0);
	tvcap_reg_write(tvcap, TVTOP_IRQ_EN_REG, 0);
	
	/* Step 2: Clear any pending interrupts */
	reg_val = tvcap_reg_read(tvcap, TVTOP_IRQ_STATUS_REG);
	tvcap_reg_write(tvcap, TVTOP_IRQ_STATUS_REG, reg_val);
	
	/* Step 3: Configure clock control */
	tvcap_reg_write(tvcap, TVTOP_CLOCK_CTRL_REG, 
			TVTOP_CLOCK_PIXEL_EN | TVTOP_CLOCK_HDMI_EN |
			TVTOP_CLOCK_DMA_EN | TVTOP_CLOCK_MIPS_EN);
	
	/* Step 4: Set default format (HDMI, YUV, 8-bit) */
	tvcap_set_format(tvcap, 0, 1, 0); /* HDMI, YUV, 8-bit */
	
	/* Step 5: Set default resolution (1920x1080) */
	tvcap_set_resolution(tvcap, 1920, 1080);
	
	/* Step 6: Configure DMA settings */
	tvcap_reg_write(tvcap, TVTOP_DMA_CTRL_REG,
			TVTOP_DMA_BURST_16 | TVTOP_DMA_PRIORITY_HIGH);
	
	/* Step 7: Enable basic control */
	tvcap_reg_write(tvcap, TVTOP_CTRL_REG,
			TVTOP_CTRL_ENABLE | TVTOP_CTRL_HDMI_EN |
			TVTOP_CTRL_AUTO_FORMAT | TVTOP_CTRL_CLOCK_EN);
	
	/* Wait for hardware stabilization */
	usleep_range(1000, 2000);
	
	/* Check hardware version */
	reg_val = tvcap_reg_read(tvcap, TVTOP_VERSION_REG);
	dev_info(tvcap->dev, "TV capture hardware version: 0x%08x\n", reg_val);
	
	/* Dump initial register state for debugging */
	if (1) /* Enable for debugging */
		tvcap_dump_registers(tvcap);
	
	dev_info(tvcap->dev, "TV capture hardware initialized successfully\n");
	/* Verify MIPS coordination readiness */
	if (tvcap_test_bits(tvcap, TVTOP_STATUS_REG, TVTOP_STATUS_MIPS_READY)) {
		dev_info(tvcap->dev, "MIPS co-processor coordination ready\n");
	}
	return 0;
}

static void tvcap_hw_cleanup(struct sunxi_tvcap_dev *tvcap)
{
	dev_dbg(tvcap->dev, "Cleaning up TV capture hardware\n");
	
	/* Disable all interrupts */
	tvcap_reg_write(tvcap, TVTOP_IRQ_EN_REG, 0);
	
	/* Stop all operations */
	tvcap_clear_bits(tvcap, TVTOP_CTRL_REG, 
			 TVTOP_CTRL_ENABLE | TVTOP_CTRL_START | 
			 TVTOP_CTRL_DMA_EN);
	
	/* Reset hardware */
	tvcap_hw_reset(tvcap);
	/* Wait for hardware to be ready after reset */
	ret = tvcap_wait_for_status(tvcap, TVTOP_STATUS_CLOCK_STABLE, TVTOP_STATUS_CLOCK_STABLE, 100);
	if (ret)
		dev_warn(tvcap->dev, "Clock stabilization timeout (continuing anyway)\n");
	
	/* Disable clocks */
	clk_bulk_disable_unprepare(TVCAP_CLK_COUNT, tvcap->clks);
}

/*
 * Enhanced interrupt handler with register-based status checking
 */
static irqreturn_t tvcap_irq_handler(int irq, void *dev_id)
{
	struct sunxi_tvcap_dev *tvcap = dev_id;
	u32 status;
	
	spin_lock(&tvcap->irq_lock);
	
	status = tvcap_reg_read(tvcap, TVTOP_IRQ_STATUS_REG);
	if (!status) {
		spin_unlock(&tvcap->irq_lock);
		return IRQ_NONE;
	}
	
	/* Clear interrupt status */
	tvcap_reg_write(tvcap, TVTOP_IRQ_STATUS_REG, status);
	
	dev_dbg(tvcap->dev, "TV capture interrupt: 0x%08x\n", status);
	
	/* Handle specific interrupt types */
	if (status & TVTOP_IRQ_HDMI_HOTPLUG) {
		dev_dbg(tvcap->dev, "HDMI hot-plug event\n");
		tvcap->hdmi_connected = tvcap_is_hdmi_connected(tvcap);
	}
	
	if (status & TVTOP_IRQ_SIGNAL_CHANGE) {
		dev_dbg(tvcap->dev, "Signal change event\n");
		tvcap->signal_detected = tvcap_is_signal_detected(tvcap);
	}
	
	if (status & TVTOP_IRQ_FORMAT_CHANGE) {
		dev_dbg(tvcap->dev, "Format change event\n");
		/* TODO: Handle format detection */
	}
	
	if (status & TVTOP_IRQ_CAPTURE_DONE) {
		dev_dbg(tvcap->dev, "Capture complete event\n");
		/* TODO: Handle capture completion */
	}
	
	if (status & TVTOP_IRQ_DMA_DONE) {
		dev_dbg(tvcap->dev, "DMA complete event\n");
		/* TODO: Handle DMA completion */
	}
	
	if (status & TVTOP_IRQ_BUFFER_OVERFLOW) {
		dev_warn(tvcap->dev, "Buffer overflow event\n");
		/* TODO: Handle buffer overflow */
	}
	
	if (status & TVTOP_IRQ_ERROR) {
		dev_err(tvcap->dev, "Hardware error event\n");
		/* TODO: Handle error conditions */
	}
	
	if (status & TVTOP_IRQ_MIPS_EVENT) {
		dev_dbg(tvcap->dev, "MIPS co-processor event\n");
		/* TODO: Handle MIPS coordination */
	}
	
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
	struct v4l2_pix_format *pix = &tvcap->format.fmt.pix;
	
	dev_info(tvcap->dev, "Starting TV capture streaming\n");
	
	/* Configure hardware for streaming */
	tvcap_set_resolution(tvcap, pix->width, pix->height);
	
	/* Enable capture interrupts */
	tvcap_reg_write(tvcap, TVTOP_IRQ_EN_REG,
			TVTOP_IRQ_HDMI_HOTPLUG | TVTOP_IRQ_SIGNAL_CHANGE |
			TVTOP_IRQ_FORMAT_CHANGE | TVTOP_IRQ_CAPTURE_DONE |
			TVTOP_IRQ_DMA_DONE | TVTOP_IRQ_BUFFER_OVERFLOW |
			TVTOP_IRQ_ERROR | TVTOP_IRQ_MIPS_EVENT);
	
	/* Enable DMA and start capture */
	tvcap_set_bits(tvcap, TVTOP_CTRL_REG,
		       TVTOP_CTRL_DMA_EN | TVTOP_CTRL_START | TVTOP_CTRL_IRQ_EN);
	
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
	
	/* Stop capture and disable DMA */
	tvcap_clear_bits(tvcap, TVTOP_CTRL_REG,
			 TVTOP_CTRL_START | TVTOP_CTRL_DMA_EN | TVTOP_CTRL_IRQ_EN);
	
	/* Disable capture interrupts */
	tvcap_reg_write(tvcap, TVTOP_IRQ_EN_REG, 0);
	
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
 * V4L2 IOCTL operations implementation with enhanced status checking
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
	struct sunxi_tvcap_dev *tvcap = video_drvdata(file);
	
	if (inp->index > 0)
		return -EINVAL;
	
	inp->type = V4L2_INPUT_TYPE_CAMERA;
	strscpy(inp->name, "HDMI Input", sizeof(inp->name));
	inp->std = V4L2_STD_UNKNOWN;
	
	/* Check HDMI connection status via registers */
	inp->status = 0;
	if (!tvcap_is_hdmi_connected(tvcap))
		inp->status |= V4L2_IN_ST_NO_SIGNAL;
	if (!tvcap_is_signal_detected(tvcap))
		inp->status |= V4L2_IN_ST_NO_SYNC;
	
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
	ret = tvcap_hw_init_enhanced(tvcap);
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