#!/bin/bash
# Configuration script for Linux 6.16.7 with H713/H616 features
# Based on sunxi mainlining improvements since 6.6

cd linux-6.16.7

# Enable key configuration options for H713/H616 support
./scripts/config --enable CONFIG_ARCH_SUNXI
./scripts/config --enable CONFIG_MACH_SUN50I

# Core H616/H713 features added since 6.6
./scripts/config --enable CONFIG_DRM_PANFROST        # Mali GPU support (6.16)
./scripts/config --enable CONFIG_IOMMU_SUPPORT       # IOMMU support (6.11)
./scripts/config --enable CONFIG_SUN50I_IOMMU       # H616 IOMMU driver (6.11)
./scripts/config --enable CONFIG_CPUFREQ            # DVFS support (6.10)
./scripts/config --enable CONFIG_CPUFREQ_DT         # Device tree DVFS (6.10)
./scripts/config --enable CONFIG_THERMAL            # Thermal support (6.9)
./scripts/config --enable CONFIG_SUN8I_THERMAL      # Allwinner thermal driver (6.9)

# Essential platform support
./scripts/config --enable CONFIG_FW_LOADER          # Firmware loading for MIPS
./scripts/config --enable CONFIG_FW_LOADER_USER_HELPER
./scripts/config --enable CONFIG_MMC                # SD/eMMC support
./scripts/config --enable CONFIG_MMC_SUNXI          # Allwinner MMC driver

# WiFi support for AIC8800
./scripts/config --enable CONFIG_CFG80211
./scripts/config --enable CONFIG_MAC80211

# Power management
./scripts/config --enable CONFIG_CPU_FREQ_GOV_ONDEMAND
./scripts/config --enable CONFIG_CPU_FREQ_GOV_CONSERVATIVE

# I2C and SPI support
./scripts/config --enable CONFIG_I2C
./scripts/config --enable CONFIG_I2C_SUN6I_P2WI
./scripts/config --enable CONFIG_SPI
./scripts/config --enable CONFIG_SPI_SUN6I

# GPIO and pinctrl
./scripts/config --enable CONFIG_PINCTRL_SUN50I_H6

# DMA support
./scripts/config --enable CONFIG_DMADEVICES
./scripts/config --enable CONFIG_DMA_SUN6I

# Crypto engine (6.11)
./scripts/config --enable CONFIG_CRYPTO_DEV_SUN8I_CE

# Audio support
./scripts/config --enable CONFIG_SOUND
./scripts/config --enable CONFIG_SND
./scripts/config --enable CONFIG_SND_SOC
./scripts/config --enable CONFIG_SND_SUN8I_CODEC

# USB support
./scripts/config --enable CONFIG_USB
./scripts/config --enable CONFIG_USB_OHCI_HCD
./scripts/config --enable CONFIG_USB_EHCI_HCD

# Ethernet support
./scripts/config --enable CONFIG_STMMAC_ETH
./scripts/config --enable CONFIG_DWMAC_SUN8I

# Enable device tree support
./scripts/config --enable CONFIG_OF
./scripts/config --enable CONFIG_OF_IRQ
./scripts/config --enable CONFIG_OF_GPIO

echo "H713/H616 kernel configuration completed"
echo "New features enabled:"
echo "- Mali GPU support (Panfrost)"
echo "- IOMMU support for better memory management" 
echo "- DVFS for power management"
echo "- Thermal management"
echo "- Crypto acceleration"
echo "- All standard Allwinner peripherals"