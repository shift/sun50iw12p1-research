# Using FEL Mode with H713 Support

Quick guide for using the patched sunxi-tools with H713 HY300 projector.

## Prerequisites

1. **HY300 in FEL Mode:**
   - Power off device
   - Hold FEL button (if present) or use FEL mode entry method
   - Connect via USB (should show as `ID 1f3a:efe8`)

2. **Verify FEL Device:**
```bash
lsusb | grep "1f3a:efe8"
```

3. **Set USB Permissions:**
```bash
# Find device
FEL_DEV=$(lsusb | grep "1f3a:efe8" | awk '{print $2 "/" $4}' | tr -d ':')

# Set permissions (requires sudo)
sudo chmod 666 /dev/bus/usb/$FEL_DEV
```

## Using the Patched sunxi-fel

### Binary Location

The compiled binary with H713 support is:
```bash
./sunxi-fel-h713
```

Or rebuild from source:
```bash
cd sunxi-tools-source
nix-shell -p gcc gnumake libusb1 pkg-config zlib dtc --run "make sunxi-fel"
./sunxi-fel --list-socs | grep H713
```

### Basic Commands

**Check FEL Version:**
```bash
./sunxi-fel-h713 version
```

**Read SID (Security ID):**
```bash
./sunxi-fel-h713 sid
```

**Read Memory:**
```bash
# Read 256 bytes from SRAM
./sunxi-fel-h713 read 0x20000 0x100 sram-test.bin

# Read 1MB
./sunxi-fel-h713 read 0x40000000 0x100000 dram-test.bin
```

**Write to SRAM:**
```bash
./sunxi-fel-h713 write 0x20000 data.bin
```

**Upload U-Boot SPL:**
```bash
./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin
```

**Upload and Execute Code:**
```bash
./sunxi-fel-h713 write 0x20000 test.bin exec 0x20000
```

## Testing Protocol

### Phase 1: Device Detection ✅
```bash
./sunxi-fel-h713 version
# Should show: SoC ID: 0x1860 (H713)
```

### Phase 2: Memory Operations (PENDING)
```bash
# Test 1: SID Read
./sunxi-fel-h713 sid > sid-output.txt

# Test 2: Small SRAM Read  
./sunxi-fel-h713 read 0x20000 0x100 sram-small.bin
hexdump -C sram-small.bin | head

# Test 3: Larger SRAM Read
./sunxi-fel-h713 read 0x20000 0x8000 sram-32k.bin
ls -lh sram-32k.bin

# Test 4: DRAM Read (if accessible)
./sunxi-fel-h713 read 0x40000000 0x10000 dram-64k.bin
```

### Phase 3: U-Boot Testing (PENDING)
```bash
# Upload U-Boot SPL
./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin

# Monitor via serial console (separate terminal)
picocom -b 115200 /dev/ttyUSB0
```

## Troubleshooting

### ERROR -7 (Timeout)
- Check USB permissions
- USB cable quality (try different cable)
- USB port (try different port)
- Device actually in FEL mode

### ERROR -8 (Overflow)
- Memory address may be wrong
- Transfer size too large
- Try smaller chunks

### "No such device"
```bash
# Re-check USB device
lsusb | grep -i allwinner

# Reset USB permissions
sudo chmod 666 /dev/bus/usb/*/*
```

### Device resets after command
- Normal behavior in some cases
- FEL mode re-entered automatically
- May need to reset USB permissions after reset

## Integration with Development Workflow

### 1. U-Boot Testing
```bash
# Compile U-Boot
cd u-boot && make hy300_defconfig && make -j$(nproc)

# Upload and test via FEL
./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin

# Monitor boot via serial
picocom -b 115200 /dev/ttyUSB0
```

### 2. Firmware Backup
```bash
# Read from eMMC (if accessible via FEL)
./sunxi-fel-h713 read 0x0 0x1000000 emmc-backup-16M.bin

# Verify backup
ls -lh emmc-backup-16M.bin
binwalk emmc-backup-16M.bin
```

### 3. Rapid Iteration
```bash
#!/bin/bash
# Quick test script
make clean && make -j$(nproc) && \
./sunxi-fel-h713 spl u-boot-sunxi-with-spl.bin && \
picocom -b 115200 /dev/ttyUSB0
```

## Known Limitations (Current)

Based on FEL investigation (docs/FEL_MODE_ANALYSIS.md):
- ✅ Device detection: Working
- ⏳ Memory read/write: Untested (waiting for hardware)
- ⏳ U-Boot upload: Untested (may need tuning)
- ⏳ Firmware backup: Untested

## Safety Notes

- **FEL mode is non-destructive** - doesn't modify eMMC
- **Always backup** before flashing anything permanent
- **Test via FEL first** before committing to eMMC
- **Serial console required** for debugging U-Boot issues

## Next Steps

1. **Hardware Testing:** Execute Phase 2 & 3 protocols
2. **Document Results:** Update FEL_MODE_ANALYSIS.md with findings
3. **Refine Configuration:** Adjust if needed based on results
4. **Submit Upstream:** Patch to linux-sunxi with test results

## See Also

- `docs/FEL_MODE_ANALYSIS.md` - FEL investigation results
- `docs/tasks/completed/027-sunxi-tools-h713-support.md` - Implementation details
- `SUNXI_TOOLS_H713_SUMMARY.md` - Technical summary
- `sunxi-tools-h713-support.patch` - Upstream-ready patch
