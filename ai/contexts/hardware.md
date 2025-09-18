# HY300 Hardware Context

## SoC: Allwinner H713
- Quad-core ARM Cortex-A53
- Mali-G31 GPU
- ARM64 architecture
- sun50i platform family

## Memory Configuration
- **RAM:** 1GB or 2GB DDR3 (exact parameters unknown)
- **Storage:** 8GB, 16GB, or 32GB eMMC
- **DRAM Parameters:** Must be extracted from boot0.bin

## Wireless Module: AW869A
- Based on Aicsemi (AIC) 8800 chipset
- WiFi 6 support
- Bluetooth 5.0
- SDIO interface

## Display System
- **LCD Panel:** 1280x720 native resolution
- **Known Models:** 
  - FPC-C269XHD0204-V
  - ZTW269HD720P-V01
  - FPC-FJ035FHD01-V3_A
- **Interface:** Likely MIPI-DSI
- **Light Source:** White LED module (GPIO/PWM controlled)
- **Keystone:** Electronic keystone correction

## I/O Ports
- HDMI input
- USB 2.0 Type-A
- 3.5mm audio jack
- DC power input
- IR remote control receiver

## Critical Hardware Access Points
- **UART Pads:** TX, RX, GND on mainboard (requires soldering)
- **FEL Mode:** Button combination to enter recovery mode
- **eMMC:** Internal storage accessible via FEL mode

## Power Management
- DC power input
- LED module power control
- Sleep/wake functionality via IR remote