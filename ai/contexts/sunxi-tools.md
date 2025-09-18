# Sunxi Tools Context

## Overview
Sunxi-tools provides utilities for interacting with Allwinner SoCs, particularly for FEL mode operations.

## Key Tools

### sunxi-fel
- **Purpose:** Communicate with SoC in FEL (Firmware Execute and Load) mode
- **Usage:** Backup firmware, load bootloaders, execute code in RAM

### Common Commands

```bash
# Check FEL mode connection
sunxi-fel version

# Read entire eMMC (example for 8GB)
sunxi-fel read 0x0 0x200000000 backup.img

# Write U-Boot SPL to RAM and execute
sunxi-fel spl u-boot-sunxi-with-spl.bin

# Load and execute U-Boot in RAM
sunxi-fel uboot u-boot-sunxi-with-spl.bin

# Write to specific memory address
sunxi-fel write 0x43000000 file.bin

# Execute code at address
sunxi-fel exe 0x43000000
```

## FEL Mode Entry
- **Method:** Specific button combination during boot
- **Verification:** Device appears as USB device
- **Connection:** USB-A to USB-A cable required

## Memory Map (H713)
- **SRAM A1:** 0x00010000 (64KB)
- **SRAM A2:** 0x00044000 (32KB) 
- **SRAM C:** 0x00018000 (44KB)
- **DRAM:** 0x40000000+ (after initialization)

## Safety Procedures
1. Always backup before modifications
2. Verify FEL connection before operations
3. Use correct memory addresses for target SoC
4. Test bootloaders in RAM before writing to storage

## Troubleshooting
- **No device found:** Check cable, FEL mode entry
- **Timeout errors:** USB connection issues
- **Memory errors:** Incorrect addresses or DRAM not initialized