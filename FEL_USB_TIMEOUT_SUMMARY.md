# FEL USB Timeout Investigation Summary

**Issue:** USB timeout when loading U-Boot with USB serial gadget support (744 KB) via FEL mode  
**Status:** Investigation in progress - requires device power cycle  
**Last Updated:** 2025-10-11

## Current Status

### Completed Work

1. ✅ **Identified initial timeout cause:**
   - Original USB_TIMEOUT of 10 seconds insufficient
   - 744 KB transfer at 64 KB/sec requires ~11.6 seconds

2. ✅ **Implemented timeout increase:**
   - Modified `fel_lib.c` USB_TIMEOUT to 20 seconds
   - Compiled new binary: `sunxi-fel-h713-20s-timeout`

3. ✅ **Retested with 20-second timeout:**
   - Transfer STILL times out at exactly 20 seconds
   - **Critical discovery:** Problem is NOT just timing margin
   - Transfer appears to hang indefinitely, never completing

4. ✅ **Device entered corrupted USB state:**
   - `libusb_open() ERROR -1: Input/Output Error`
   - Continuous USB reset/disconnect cycles observed
   - Device requires physical power cycle to recover

### Key Findings

**The timeout occurs at the NEW limit (20s), not old limit (10s):**
- This proves the timeout modification was applied correctly
- But also proves the transfer is taking >20 seconds (not expected 11.6s)
- Indicates underlying protocol incompatibility, not just timing margin issue

**Transfer behavior suggests:**
- Initial handshake succeeds (AWUS response received)
- Bulk transfer never starts or hangs indefinitely
- H713 BROM may have different protocol expectations than H6/H616

## Technical Analysis

### Evidence Summary

**Working:**
- ✅ FEL mode entry
- ✅ USB device enumeration (ID 1f3a:efe8)
- ✅ Initial handshake (AW_FEL_VERSION, AWUS response)

**Failing:**
- ❌ Bulk transfer hangs for >20 seconds
- ❌ Device becomes unresponsive after timeout
- ❌ libusb_open() fails with I/O error post-timeout
- ❌ Requires physical power cycle to recover

### Possible Root Causes

1. **H713 BROM Protocol Difference (60% probability)**
   - H713 may require additional handshakes during bulk transfer
   - Transfer sequence might differ from H6/H616 BROM
   - Device acknowledges readiness but never starts transfer

2. **Memory Configuration Issue (30% probability)**
   - 744 KB may exceed available SRAM regions
   - H713 SRAM layout might need adjustment
   - Memory overlap with BROM stack/heap

3. **USB Transfer Parameter Mismatch (10% probability)**
   - Chunk size (512 KB) may be too large
   - Alignment or timing requirements different
   - USB endpoint configuration incompatible

## Next Steps Required

### IMMEDIATE: Device Power Cycle

**Required before any further testing:**

The device is currently in a corrupted USB state and requires physical reset:

```bash
# Current state - will fail:
$ sudo ./sunxi-fel-h713-20s-timeout version
ERROR: libusb_open() ERROR -1: Input/Output Error

# Evidence of USB corruption:
$ dmesg | tail -20
[...] usb 1-3: reset full-speed USB device
[...] usb 1-3: USB disconnect, device number 86
[...] usb 1-3: new full-speed USB device number 87
[...] usb 1-3: reset full-speed USB device
# ^ Continuous reset cycles
```

**Recovery steps:**
1. Unplug USB cable from device OR power cycle device
2. Wait 5 seconds
3. Reconnect USB cable
4. Verify stable enumeration: `lsusb | grep 1f3a`
5. Check dmesg for single enumeration (no reset cycles)

### CRITICAL: SPL-Only Transfer Test

**Purpose:** Isolate whether issue is size-related or protocol-related

After device power cycle, run:

```bash
./test-spl-only.sh
```

This automated test will:
1. Check device stability
2. Extract SPL (first 32 KB)
3. Attempt FEL transfer of small binary
4. Interpret results and suggest next steps

**Expected outcomes:**

- ✅ **SPL succeeds:** Issue is size-related (>32 KB threshold)
  - Next: Test intermediate sizes (64 KB, 128 KB, 256 KB)
  - Next: Test split transfer method
  - Next: Test standard U-Boot (732 KB)

- ❌ **SPL fails:** Issue is fundamental protocol incompatibility
  - Next: Add detailed transfer logging to fel_lib.c
  - Next: Analyze H713 BROM protocol differences
  - Next: Try H6 memory layout configuration
  - Next: Consider alternative boot methods

### Additional Diagnostic Tests (After SPL test)

If SPL test succeeds, run these tests in order:

1. **Standard U-Boot (732 KB):**
   ```bash
   sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl.bin
   ```

2. **Intermediate sizes:**
   ```bash
   dd if=u-boot-sunxi-with-spl.bin of=test-64k.bin bs=1024 count=64
   sudo ./sunxi-fel-h713-20s-timeout -v write 0x104000 test-64k.bin
   ```

3. **60-second timeout (diagnostic only):**
   - Modify fel_lib.c: `#define USB_TIMEOUT 60000`
   - Rebuild: `cd build/sunxi-tools && make`
   - Test: `sudo ./sunxi-fel -v uboot u-boot-sunxi-with-spl-usb.bin`

## Documentation

### Created Files

- `docs/FEL_USB_TIMEOUT_INVESTIGATION.md` - Initial timeout analysis
- `docs/FEL_USB_LIBUSB_ERROR_INVESTIGATION.md` - USB state corruption analysis
- `FEL_USB_TIMEOUT_SUMMARY.md` - This file
- `test-spl-only.sh` - Automated SPL transfer test
- `90-sunxi-fel.rules` - Udev rules for device permissions

### Modified Files

- `build/sunxi-tools/fel_lib.c` - Increased USB_TIMEOUT to 20 seconds
- Backup: `build/sunxi-tools/fel_lib.c.timeout-10s-backup`

### Binary Artifacts

- `sunxi-fel-h713-20s-timeout` - FEL tool with 20s timeout (77 KB)
- `u-boot-sunxi-with-spl-usb.bin` - U-Boot with USB gadget support (744 KB)
- `u-boot-sunxi-with-spl.bin` - Standard U-Boot (732 KB)

## Conclusions

### What We Know

1. ✅ Timeout increase to 20s was correctly applied
2. ✅ Transfer is taking >20 seconds (not predicted 11.6s)
3. ✅ Issue is NOT just timing margin - likely protocol incompatibility
4. ✅ Device enters corrupted USB state after timeout
5. ✅ Physical power cycle required for recovery

### What We Don't Know

1. ❓ Does H713 support ANY FEL bulk transfers? (SPL test will answer)
2. ❓ Is there a size threshold below which transfers work?
3. ❓ Are H713 BROM protocol differences documented anywhere?
4. ❓ Can split transfer method work around the issue?

### Confidence Assessment

**Low confidence (40%):** Issue can be resolved via sunxi-fel modifications  
**Medium confidence (40%):** Issue requires H713-specific protocol changes  
**Low confidence (20%):** FEL mode is fundamentally incompatible with H713

The SPL transfer test will significantly update these probabilities.

## Blocking Issues

**BLOCKER:** Device requires power cycle before any testing can continue  
**BLOCKER:** Cannot proceed without SPL transfer test results

## Risk Assessment

**If FEL mode proves incompatible:**
- Alternative: Boot from SD card (requires card reader access)
- Alternative: Boot from eMMC (requires Android fastboot or factory firmware)
- Impact: Development workflow significantly slowed
- Mitigation: Establish serial console access for debugging

**If size threshold exists:**
- Workaround: Split transfer method for large binaries
- Workaround: Load minimal U-Boot, then load full version from SD/eMMC
- Impact: More complex boot procedure
- Mitigation: Automate split transfer in tooling

## References

- `docs/FEL_MODE_ANALYSIS.md` - Original H713 FEL mode discovery
- `docs/FEL_TESTING_RESULTS.md` - Early FEL testing results
- `docs/HY300_TESTING_METHODOLOGY.md` - Safe testing procedures
- `USING_H713_FEL_MODE.md` - FEL mode usage guide

## Task Status

**Task:** 033-usb-serial-gadget-uboot-configuration  
**Status:** Blocked pending FEL compatibility resolution  
**Blocker:** Cannot test USB serial U-Boot without working FEL transfer

---

**IMMEDIATE ACTION REQUIRED:**
1. Power cycle the device
2. Run `./test-spl-only.sh`
3. Report results for further analysis
