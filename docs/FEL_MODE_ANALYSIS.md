# HY300 FEL Mode Analysis Results

## Date: 2025-10-10

## FEL Mode Access: PARTIAL SUCCESS

### Connection Status
- ✅ FEL mode entry: **Working** (via remote button during power-on)
- ✅ USB detection: **Working** (ID 1f3a:efe8)
- ✅ sunxi-fel version: **Working** (with permission setup)
- ❌ Memory operations: **Timeout errors**
- ❌ U-Boot upload: **Overflow errors**

### SoC Identification
```
SoC ID: 0x1860 (H713)
BROM Ver: AWUSBFEX soc=00001860(unknown) 00000001 ver=0001 44 08
Scratchpad: 00121500 00000000 00000000
```

**Key Finding:** sunxi-fel recognizes the device but shows:
```
Warning: no 'soc_sram_info' data for your SoC (id=1860)
```

This means H713 (SoC ID 0x1860) is **not in the official sunxi-tools database**.

### USB Behavior
- Device stays in FEL mode continuously (doesn't exit)
- USB connection resets after each failed command
- Device number increments with each reset (094, 095, 096...)
- Connection succeeds every ~3-4 attempts
- Permission issues require `chmod 666` before each access

### Attempted Operations

#### Working:
1. ✅ `sunxi-fel version` - Shows SoC info
2. ✅ Device enumeration via lsusb

#### Failing:
1. ❌ `sunxi-fel sid` - Not supported on H713
2. ❌ `sunxi-fel dump <addr> <len> <file>` - Timeout (ERROR -7)
3. ❌ `sunxi-fel read <addr> <len> <file>` - Timeout (ERROR -7)
4. ❌ `sunxi-fel uboot <file>` - Overflow (ERROR -8) then timeout

### Error Analysis

**usb_bulk_recv() ERROR -7: Operation timed out**
- Indicates FEL protocol timing mismatch
- H713 BROM expects different timing/sequencing than sunxi-tools provides
- Memory map unknown to sunxi-tools

**usb_bulk_recv() ERROR -8: Overflow**
- USB buffer size mismatch
- H713 may have different USB buffer requirements
- Occurred when uploading U-Boot SPL

### U-Boot Upload Attempt
```
found DT name in SPL header: allwinner/sun55i-h713-hy300
```
- sunxi-fel successfully parsed our U-Boot SPL header
- Detected our device tree name correctly
- Transfer failed due to bulk transfer protocol issues

## Root Cause

The H713 is too new/proprietary for current sunxi-tools:
1. **Missing SoC database entry** - No memory map for ID 0x1860
2. **FEL protocol variant** - Allwinner may have modified the FEL protocol for H713
3. **No community support** - H713 is not in linux-sunxi wiki or tools

## Implications for Project

### Cannot Do via FEL:
- ❌ Backup boot0 bootloader directly
- ❌ Dump eMMC partitions via FEL
- ❌ Boot U-Boot via FEL (at least not with current tools)
- ❌ Read DRAM parameters from boot0 in SRAM

### Can Still Do:
- ✅ Flash eMMC directly (if we accept the risk)
- ✅ Use fastboot mode (if available in Android)
- ✅ Serial console access for U-Boot (once flashed)
- ✅ Extract info from existing firmware files (already done)

## Alternative Approaches

### Option 1: Add H713 Support to sunxi-tools
**Difficulty:** High
**Risk:** Low
**Requirements:**
- Reverse engineer H713 FEL protocol
- Find SRAM memory map for H713
- Submit patches to sunxi-tools
- May require hardware protocol analysis

### Option 2: Use Alternative FEL Tools
**Difficulty:** Medium
**Risk:** Low
**Requirements:**
- Try Allwinner's official LiveSuit/PhoenixSuit tools
- May only work on Windows
- Likely won't have U-Boot boot capability

### Option 3: Direct eMMC Flashing
**Difficulty:** Low
**Risk:** HIGH - could brick device
**Requirements:**
- Flash U-Boot directly to eMMC boot partition
- No safety net if it doesn't work
- Would need Android fastboot or factory tools

### Option 4: Wait for Serial Console
**Difficulty:** Low (hardware access required)
**Risk:** Low
**Requirements:**
- Get serial console access (UART)
- Interrupt Android boot
- Boot mainline kernel from eMMC via U-Boot
- Safest approach - can recover via Android

## Recommended Next Steps

### Immediate (Software Only):
1. ✅ Document FEL limitations (this file)
2. ⏭️ Research Allwinner LiveSuit/PhoenixSuit compatibility
3. ⏭️ Check if Android has fastboot mode enabled
4. ⏭️ Prepare complete eMMC flash image with recovery

### Near-term (Requires Hardware):
1. ⏭️ Get serial console access (UART pins)
2. ⏭️ Analyze boot messages for hints
3. ⏭️ Test U-Boot boot from eMMC
4. ⏭️ Use U-Boot UMS mode for eMMC backup

### Long-term (Community Effort):
1. ⏭️ Submit H713 info to linux-sunxi wiki
2. ⏭️ Work with sunxi community on FEL support
3. ⏭️ Contribute H713 support patches

## Conclusion

**FEL mode works for device identification but not for data transfer or U-Boot booting due to H713-specific protocol incompatibilities.**

The project can proceed via:
1. **Serial console access** (safest, requires hardware)
2. **Direct eMMC flashing** (risky, no recovery)
3. **Android fastboot** (if available, medium risk)

The safest path forward is **serial console access**, which was already planned for Phase VI hardware testing.

## Files Created
- `/home/shift/code/android_projector/tools/fel-permissions.sh` - USB permission helper
- `/home/shift/code/android_projector/tools/quick-fel-backup.sh` - FEL backup script (non-functional for H713)
- `/home/shift/code/android_projector/docs/FEL_MODE_ANALYSIS.md` - This document

## Technical Details

### sunxi-tools Version
```
sunxi-tools v1.4.2 (custom build from master)
Commit: 7540cb235691be94ac5ef0181a73dd929949fc4e
Built: 2025-03-29
```

### USB Device Info
```
Bus 001 Device XXX: ID 1f3a:efe8 Allwinner Technology sunxi SoC OTG connector in FEL/flashing mode
bcdUSB: 1.10
bDeviceClass: 0 (vendor-specific)
MaxPower: 300mA
Transfer Type: Bulk
wMaxPacketSize: 64 bytes
```

### Permission Requirements
- USB device at `/dev/bus/usb/001/XXX` needs mode `0666`
- NixOS read-only filesystem prevents persistent udev rules
- Must set permissions manually before each access
- Permissions reset with each USB device reset
