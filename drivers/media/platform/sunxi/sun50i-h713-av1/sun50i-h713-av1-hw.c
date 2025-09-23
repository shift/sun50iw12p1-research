// SPDX-License-Identifier: GPL-2.0+
/*
 * Allwinner H713 AV1 Hardware Decoder Driver - Hardware Abstraction Layer
 *
 * Copyright (C) 2025 HY300 Linux Porting Project
 *
 * This file implements the hardware abstraction layer for the H713 AV1 decoder.
 */

#include <linux/delay.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include "sun50i-h713-av1.h"

/**
 * sun50i_av1_hw_reset - Reset the AV1 hardware decoder
 * @dev: Device context
 *
 * Returns: 0 on success, negative error code on failure
 */
int sun50i_av1_hw_reset(struct sun50i_av1_dev *dev)
{
	int ret;

	dev_dbg(dev->dev, "Resetting AV1 hardware\n");

	/* Assert reset */
	ret = reset_control_assert(dev->reset);
	if (ret) {
		dev_err(dev->dev, "Failed to assert reset: %d\n", ret);
		return ret;
	}

	/* Hold reset for a few microseconds */
	usleep_range(10, 20);

	/* Deassert reset */
	ret = reset_control_deassert(dev->reset);
	if (ret) {
		dev_err(dev->dev, "Failed to deassert reset: %d\n", ret);
		return ret;
	}

	/* Wait for hardware to stabilize */
	usleep_range(100, 200);

	/* Update metrics */
	atomic64_inc(&av1_metrics.hw_resets);

	dev_dbg(dev->dev, "AV1 hardware reset complete\n");
	return 0;
}

/**
 * sun50i_av1_hw_enable - Enable AV1 hardware decoder
 * @dev: Device context
 *
 * Returns: 0 on success, negative error code on failure
 */
int sun50i_av1_hw_enable(struct sun50i_av1_dev *dev)
{
	int ret;

	dev_dbg(dev->dev, "Enabling AV1 hardware\n");

	/* Enable bus clock */
	ret = clk_prepare_enable(dev->bus_clk);
	if (ret) {
		dev_err(dev->dev, "Failed to enable bus clock: %d\n", ret);
		return ret;
	}

	/* Enable mbus clock */
	ret = clk_prepare_enable(dev->mbus_clk);
	if (ret) {
		dev_err(dev->dev, "Failed to enable mbus clock: %d\n", ret);
		goto err_disable_bus_clk;
	}

	/* Reset hardware */
	ret = sun50i_av1_hw_reset(dev);
	if (ret) {
		dev_err(dev->dev, "Failed to reset hardware: %d\n", ret);
		goto err_disable_mbus_clk;
	}

	/* Enable interrupts */
	av1_write(dev, AV1_REG_INT_ENABLE,
		  AV1_INT_DECODE_DONE | AV1_INT_DECODE_ERROR | AV1_INT_FRAME_READY);

	/* Clear any pending interrupts */
	av1_write(dev, AV1_REG_INT_STATUS, 0xFFFFFFFF);

	dev_dbg(dev->dev, "AV1 hardware enabled successfully\n");
	return 0;

err_disable_mbus_clk:
	clk_disable_unprepare(dev->mbus_clk);
err_disable_bus_clk:
	clk_disable_unprepare(dev->bus_clk);
	return ret;
}

/**
 * sun50i_av1_hw_disable - Disable AV1 hardware decoder
 * @dev: Device context
 */
void sun50i_av1_hw_disable(struct sun50i_av1_dev *dev)
{
	dev_dbg(dev->dev, "Disabling AV1 hardware\n");

	/* Disable interrupts */
	av1_write(dev, AV1_REG_INT_ENABLE, 0);

	/* Clear any pending interrupts */
	av1_write(dev, AV1_REG_INT_STATUS, 0xFFFFFFFF);

	/* Disable hardware */
	av1_clear_bits(dev, AV1_REG_CTRL, AV1_CTRL_ENABLE);

	/* Disable clocks */
	clk_disable_unprepare(dev->mbus_clk);
	clk_disable_unprepare(dev->bus_clk);

	dev_dbg(dev->dev, "AV1 hardware disabled\n");
}

/**
 * sun50i_av1_hw_init - Initialize AV1 hardware decoder
 * @dev: Device context
 *
 * Returns: 0 on success, negative error code on failure
 */
int sun50i_av1_hw_init(struct sun50i_av1_dev *dev)
{
	int ret;

	dev_dbg(dev->dev, "Initializing AV1 hardware\n");

	/* Enable hardware */
	ret = sun50i_av1_hw_enable(dev);
	if (ret) {
		dev_err(dev->dev, "Failed to enable hardware: %d\n", ret);
		return ret;
	}

	/* Verify hardware is accessible */
	av1_write(dev, AV1_REG_CTRL, 0);
	if (av1_read(dev, AV1_REG_CTRL) != 0) {
		dev_err(dev->dev, "Hardware register access failed\n");
		ret = -EIO;
		goto err_disable_hw;
	}

	/* Wait for hardware to be idle */
	if (!sun50i_av1_hw_wait_idle(dev, 1000)) {
		dev_err(dev->dev, "Hardware failed to become idle\n");
		ret = -ETIMEDOUT;
		goto err_disable_hw;
	}

	dev_info(dev->dev, "AV1 hardware initialized successfully\n");
	return 0;

err_disable_hw:
	sun50i_av1_hw_disable(dev);
	return ret;
}

/**
 * sun50i_av1_hw_deinit - Deinitialize AV1 hardware decoder
 * @dev: Device context
 */
void sun50i_av1_hw_deinit(struct sun50i_av1_dev *dev)
{
	dev_dbg(dev->dev, "Deinitializing AV1 hardware\n");

	/* Stop any ongoing operations */
	av1_clear_bits(dev, AV1_REG_CTRL, AV1_CTRL_ENABLE);

	/* Wait for hardware to stop */
	sun50i_av1_hw_wait_idle(dev, 1000);

	/* Disable hardware */
	sun50i_av1_hw_disable(dev);

	dev_dbg(dev->dev, "AV1 hardware deinitialized\n");
}

/**
 * sun50i_av1_hw_wait_idle - Wait for hardware to become idle
 * @dev: Device context
 * @timeout_ms: Timeout in milliseconds
 *
 * Returns: true if hardware became idle, false if timeout
 */
bool sun50i_av1_hw_wait_idle(struct sun50i_av1_dev *dev, unsigned int timeout_ms)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(timeout_ms);
	u32 status;

	do {
		status = av1_read(dev, AV1_REG_STATUS);
		if (status & AV1_STATUS_IDLE)
			return true;

		if (time_after(jiffies, timeout))
			break;

		usleep_range(100, 200);
	} while (1);

	dev_warn(dev->dev, "Hardware idle timeout (status: 0x%08x)\n", status);
	return false;
}

/**
 * sun50i_av1_hw_is_busy - Check if hardware is busy
 * @dev: Device context
 *
 * Returns: true if hardware is busy, false if idle
 */
bool sun50i_av1_hw_is_busy(struct sun50i_av1_dev *dev)
{
	u32 status = av1_read(dev, AV1_REG_STATUS);
	return !!(status & AV1_STATUS_BUSY);
}

/**
 * sun50i_av1_hw_start_decode - Start AV1 hardware decoding
 * @dev: Device context
 * @config: Frame configuration
 *
 * Returns: 0 on success, negative error code on failure
 */
int sun50i_av1_hw_start_decode(struct sun50i_av1_dev *dev,
			       struct av1_frame_config *config)
{
	u32 ctrl_val = 0;

	dev_dbg(dev->dev, "Starting AV1 decode\n");

	/* Verify hardware is idle */
	if (sun50i_av1_hw_is_busy(dev)) {
		dev_err(dev->dev, "Hardware busy, cannot start decode\n");
		return -EBUSY;
	}

	/* Configure frame parameters */
	av1_write(dev, AV1_REG_FRAME_CONFIG, config->format);

	/* Configure output addresses */
	av1_write(dev, AV1_REG_OUTPUT_ADDR_Y, config->image_addr[0]);
	av1_write(dev, AV1_REG_OUTPUT_ADDR_U, config->image_addr[1]);
	av1_write(dev, AV1_REG_OUTPUT_ADDR_V, config->image_addr[2]);
	av1_write(dev, AV1_REG_OUTPUT_STRIDE, config->image_width[0]);

	/* Configure metadata */
	if (config->metadata_addr && config->metadata_size) {
		av1_write(dev, AV1_REG_METADATA_ADDR, config->metadata_addr);
		av1_write(dev, AV1_REG_METADATA_SIZE, config->metadata_size);
	}

	/* Build control register value */
	ctrl_val |= AV1_CTRL_ENABLE;
	
	if (config->fbd_enable)
		ctrl_val |= AV1_CTRL_FBD_ENABLE;
	
	if (config->blue_enable)
		ctrl_val |= AV1_CTRL_BLUE_ENABLE;
	
	if (config->interlace_enable)
		ctrl_val |= AV1_CTRL_INTERLACE_ENABLE;

	/* Clear any pending interrupts */
	av1_write(dev, AV1_REG_INT_STATUS, 0xFFFFFFFF);

	/* Start decode operation */
	av1_write(dev, AV1_REG_CTRL, ctrl_val);
	av1_write(dev, AV1_REG_DECODE_START, 1);

	dev_dbg(dev->dev, "AV1 decode started (ctrl: 0x%08x)\n", ctrl_val);
	return 0;
}

/**
 * sun50i_av1_hw_stop_decode - Stop AV1 hardware decoding
 * @dev: Device context
 */
void sun50i_av1_hw_stop_decode(struct sun50i_av1_dev *dev)
{
	dev_dbg(dev->dev, "Stopping AV1 decode\n");

	/* Disable hardware */
	av1_clear_bits(dev, AV1_REG_CTRL, AV1_CTRL_ENABLE);

	/* Clear decode start */
	av1_write(dev, AV1_REG_DECODE_START, 0);

	/* Wait for hardware to stop */
	sun50i_av1_hw_wait_idle(dev, 1000);

	dev_dbg(dev->dev, "AV1 decode stopped\n");
}
