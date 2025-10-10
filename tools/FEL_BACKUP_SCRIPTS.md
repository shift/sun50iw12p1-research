# HY300 FEL Mode Backup Scripts

## Overview

Two backup scripts for HY300 firmware extraction via FEL mode with H713 support.

## Scripts

### 1. Comprehensive Backup: `hy300-fel-backup.sh`

**Purpose:** Complete extraction of all FEL-accessible firmware components

**Features:**
- Automatic FEL device detection and permission setup
- Comprehensive error handling with retry logic
- Extracts 10 components including boot0, U-Boot, SRAM regions, and DRAM samples
- Generates detailed manifest and memory map documentation
- Creates SHA256 checksums for validation
- Automatic boot0 DRAM parameter analysis
- Clear documentation of FEL limitations and next steps

**Components Extracted:**
1. SoC information and version
2. Chip ID (SID - Security Identifier)
3. boot0 bootloader (32KB) with DRAM parameters
4. U-Boot region (1MB from offset 0x8000)
5. SRAM A1 (32KB at 0x10000)
6. SRAM A2 (48KB at 0x18000)
7. SRAM C (175KB at 0x28000)
8. DRAM sample (64KB from base 0x40000000)
9. Memory map documentation
10. Backup manifest with next steps

**Usage:**
```bash
# Basic usage (creates timestamped directory)
sudo ./tools/hy300-fel-backup.sh

# Specify output directory
sudo ./tools/hy300-fel-backup.sh /path/to/backup

# From outside Nix devShell
nix develop -c -- sudo ./tools/hy300-fel-backup.sh
```

**Output Structure:**
```
fel_backup_YYYYMMDD_HHMMSS/
├── soc_info.txt           # FEL version and SoC details
├── chip_id.txt            # Security ID
├── boot0.bin              # Boot0 bootloader (32KB)
├── uboot_region.bin       # U-Boot region (1MB)
├── sram_a1.bin            # SRAM A1 (32KB)
├── sram_a2.bin            # SRAM A2 (48KB)
├── sram_c.bin             # SRAM C (175KB)
├── dram_sample.bin        # DRAM sample (64KB)
├── memory_map.txt         # Memory layout documentation
├── backup_manifest.txt    # Complete backup documentation
└── checksums.sha256       # SHA256 checksums
```

### 2. Quick Backup: `hy300-fel-quick.sh`

**Purpose:** Fast extraction of essential components for rapid testing

**Features:**
- Minimal output for scripting
- Essential components only (SoC info, chip ID, boot0)
- Fast execution (~10 seconds)
- Suitable for repeated testing and validation

**Components Extracted:**
1. SoC information
2. Chip ID
3. boot0 bootloader
4. Simple manifest

**Usage:**
```bash
# Basic usage
sudo ./tools/hy300-fel-quick.sh

# Specify output directory  
sudo ./tools/hy300-fel-quick.sh /path/to/backup
```

**Output Structure:**
```
fel_quick_YYYYMMDD_HHMMSS/
├── soc_info.txt
├── chip_id.txt
├── boot0.bin
└── manifest.txt
```

## Prerequisites

### Hardware Requirements
- HY300 projector in FEL mode
- USB connection to host computer
- FEL mode device detected as `ID 1f3a:efe8`

### Software Requirements
- `sunxi-fel-h713` binary (H713-patched version)
- Root/sudo access for USB permissions
- Linux host system

### Verifying Prerequisites

1. **Check FEL mode:**
```bash
lsusb | grep "1f3a:efe8"
# Should show: Bus XXX Device XXX: ID 1f3a:efe8
```

2. **Verify binary:**
```bash
ls -lh ./sunxi-fel-h713
# Should show ~77KB executable
```

3. **Test FEL connection:**
```bash
sudo ./sunxi-fel-h713 version
# Should show: AWUSBFEX soc=00001860(H713) ...
```

## FEL Mode Access Capabilities

### ✓ Accessible via FEL
- SRAM regions (A1, A2, C)
- DRAM (after initialization)
- SoC registers and peripheral memory
- boot0 (loaded by BROM into memory)
- SoC information and chip ID

### ✗ NOT Accessible via FEL
- eMMC block device (requires booted system)
- Large firmware images stored on eMMC
- Partition tables and filesystems
- Complete Android/Linux installations

## Complete eMMC Backup

FEL mode cannot access eMMC directly. For complete eMMC backup:

### Method 1: U-Boot UMS Mode (Recommended)

**Boot U-Boot via FEL:**
```bash
sudo ./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin
```

**At U-Boot prompt (via serial console):**
```
=> ums 0 mmc 0
```

**On host computer (device appears as USB storage):**
```bash
# Identify device
lsblk

# Create complete backup
sudo dd if=/dev/sdX of=emmc_full.img bs=1M status=progress

# Verify backup
ls -lh emmc_full.img

# Restore if needed
sudo dd if=emmc_full.img of=/dev/sdX bs=1M status=progress
```

### Method 2: Linux dd (Alternative)

1. Boot minimal Linux via FEL with initramfs
2. Load network or USB storage drivers
3. Copy eMMC device:
```bash
dd if=/dev/mmcblk0 of=/mnt/backup/emmc.img bs=1M
```

### Method 3: Android Recovery (If Available)

Use factory recovery tools if Android recovery partition is accessible.

## Boot0 Analysis

The boot0 bootloader contains critical DRAM initialization parameters.

**Analyze with provided tool:**
```bash
cd fel_backup_YYYYMMDD_HHMMSS
python3 ../tools/analyze_boot0.py boot0.bin
```

**Expected output:**
- DRAM type (DDR3/LPDDR4)
- Clock frequency
- Timing parameters
- Bank configuration
- Total memory size

## Validation Procedures

### Verify Backup Integrity

**Check file sizes:**
```bash
cd fel_backup_YYYYMMDD_HHMMSS

# boot0 should be exactly 32KB
stat -c%s boot0.bin  # Should output: 32768

# Verify checksums
sha256sum -c checksums.sha256
```

**Validate boot0 header:**
```bash
# Check for boot0 magic bytes
hexdump -C boot0.bin | head -5
# Should show recognizable header (eGON or similar)

# Extract header strings
strings boot0.bin | head -20
# Should show "boot", "dram", or DRAM-related strings
```

**Compare with previous backups:**
```bash
# boot0 should be identical across backups
diff boot0_1.bin boot0_2.bin
# No output = identical (good)
```

## Error Handling

### FEL Device Not Found
```
ERROR: FEL device not found (ID: 1f3a:efe8)
```

**Solutions:**
1. Verify device is powered and in FEL mode
2. Check USB cable connection
3. Try different USB port
4. Run: `lsusb` to see all USB devices

### Permission Denied
```
ERROR: libusb_open() failed with LIBUSB_ERROR_ACCESS
```

**Solutions:**
1. Run script with sudo: `sudo ./tools/hy300-fel-backup.sh`
2. Or set permissions: `sudo ./tools/fel-permissions.sh`

### USB Timeout Errors
```
ERROR -7 (Timeout)
```

**Solutions:**
1. Try different USB cable (quality matters)
2. Use USB 2.0 port (not USB 3.0)
3. Reduce transfer sizes in script
4. Check for USB hub issues (connect directly)

### Device Resets During Backup
```
Device appears to reset between commands
```

**Normal behavior:**
- FEL mode may reset after certain operations
- Scripts automatically reset permissions after reset
- Data extracted before reset is safe

## Integration with Development Workflow

### Phase VI Hardware Testing Preparation

This backup is a prerequisite for safe hardware testing:

1. **Create baseline backup** before any modifications
2. **Analyze boot0** for DRAM parameters
3. **Test U-Boot via FEL** (non-destructive)
4. **Create complete eMMC backup** via U-Boot UMS
5. **Proceed with Phase VI** testing

### Backup Best Practices

**Before hardware modifications:**
```bash
# 1. Create FEL backup
sudo ./tools/hy300-fel-backup.sh

# 2. Boot U-Boot via FEL
sudo ./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin

# 3. Create complete eMMC backup via UMS
# (See Method 1 above)

# 4. Verify both backups
sha256sum fel_backup_*/boot0.bin
```

**Backup rotation:**
```bash
# Keep timestamped backups
ls -lrt fel_backup_* | tail -5  # Last 5 backups

# Archive important baselines
tar czf hy300_factory_baseline.tar.gz fel_backup_YYYYMMDD_HHMMSS/
```

## Safety Protocols

### ✅ FEL Mode is Safe
- All FEL operations are read-only
- No writes to eMMC or persistent storage
- Device recovers by power cycle
- Cannot "brick" device via FEL mode

### ⚠️ Important Notes
- Requires root/sudo for USB permissions
- USB cable quality affects reliability
- Large transfers may timeout (retry automatically)
- Device may reset between commands (handled automatically)

### 🛡️ Recovery Guarantee
If anything goes wrong:
1. Power cycle device
2. Re-enter FEL mode
3. Try backup again
4. No permanent damage possible

## Technical Details

### Memory Addresses
```
0x00000000  boot0 bootloader (32KB)
0x00008000  U-Boot region (1MB)
0x00010000  SRAM A1 (32KB)
0x00018000  SRAM A2 (48KB)
0x00028000  SRAM C (175KB)
0x40000000  DRAM base (2GB total)
```

### H713 SoC Specifics
- **SoC ID:** 0x1860
- **Based on:** H616 (identical SRAM layout)
- **SPL Address:** 0x20000
- **SRAM Size:** 207KB (32K A1 + 175K C)
- **SID Base:** 0x03006000

### FEL Protocol
- **USB VID:PID:** 1f3a:efe8 (Allwinner FEL)
- **Protocol:** Proprietary Allwinner USB protocol
- **Commands:** version, sid, read, write, exec, spl, uboot

## References

- **Implementation:** `SUNXI_TOOLS_H713_SUMMARY.md` - H713 support details
- **Usage Guide:** `USING_H713_FEL_MODE.md` - Detailed FEL mode instructions
- **Task Documentation:** `docs/tasks/027-fel-mode-firmware-backup.md`
- **Hardware Testing:** `docs/tasks/010-phase6-hardware-testing.md`
- **Analysis Tools:** `tools/analyze_boot0.py` - DRAM parameter extraction

## Troubleshooting

### Script Hangs or Freezes
1. Press Ctrl+C to abort
2. Power cycle device
3. Re-enter FEL mode
4. Retry with quick backup first: `sudo ./tools/hy300-fel-quick.sh`

### Incomplete Backups
- Check `backup_manifest.txt` for component status
- Missing components marked with ✗ or ⚠
- Some regions may not be accessible until DRAM initialized
- This is expected - FEL has limitations

### Validation Failures
- Compare with known-good backups
- Check file sizes match expected values
- Verify boot0 header with hexdump
- Run `binwalk boot0.bin` for structural analysis

## Future Enhancements

Planned improvements:
- Automated comparison with factory firmware analysis
- Integration with backup rotation and versioning
- U-Boot UMS mode automation
- Cross-validation with Android firmware analysis
- Graphical progress indicators
- Parallel component extraction for speed

## Support

For issues or questions:
1. Check documentation: `USING_H713_FEL_MODE.md`
2. Review error messages in backup output
3. Examine backup_manifest.txt for component status
4. Verify prerequisites (FEL mode, USB connection, binary)
