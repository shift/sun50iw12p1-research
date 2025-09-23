# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Allwinner sunxi media platform drivers
#

# For out-of-tree module building, force enable the TVCAP module
CONFIG_VIDEO_SUNXI_TVCAP := m
obj-$(CONFIG_VIDEO_SUNXI_TVCAP) += sunxi-tvcap-enhanced.o

# Additional sunxi media drivers can be added here
# obj-$(CONFIG_VIDEO_SUNXI_CEDRUS) += cedrus/
# obj-$(CONFIG_VIDEO_SUNXI_CSI) += sun6i-csi/
# AV1 hardware decoder for H713 SoC
CONFIG_VIDEO_SUN50I_H713_AV1 := m
obj-$(CONFIG_VIDEO_SUN50I_H713_AV1) += sun50i-h713-av1/
