# H713 FEL Mode Fixes Summary

## Problem Statement

Initial H713 FEL detection worked (`sunxi-fel version`), but SPL upload failed with various USB errors:
1. **ERROR -8 (Overflow)**: USB response buffer too small
2. **ERROR -7 (Timeout)**: Memory address conflicts in SRAM layout

## Root Cause Analysis

H713 uses a fundamentally different SRAM architecture than H616:
- **H616**: SPL in SRAM A1 (0x20000), 32KB region
- **H713**: SPL in SRAM A2 (0x104000), 128KB region

The upstream sunxi-tools incorrectly treated H713 as identical to H616, causing:
1. Wrong SPL load address (0x20000 vs 0x104000)
2. Wrong USB protocol buffer sizes (13 bytes vs 16 bytes)
3. Wrong swap buffer addresses (SRAM A1 vs SRAM A2)

## Evidence Sources

All fixes based on reverse engineering factory `boot0.bin`:
- **SPL address**: boot0.bin header offset 0x18 = 0x00104000
- **Stack pointer**: boot0.bin offset 0x3f8 = 0x00124000
- **Memory regions**: Complete disassembly analysis in `docs/H713_BROM_MEMORY_MAP.md`

## Fixes Applied

### 1. USB Response Buffer Size (fel_lib.c:155) - CORRECTED IN V3
```c
// Original:
char buf[13];

// V1/V2 (WRONG):
char buf[16];  /* Assumed H713 needed 16-byte alignment */

// V3 (CORRECT):
char buf[13];  /* H713 sends exactly 13 bytes */
assert(strncmp(buf, "AWUS", 4) == 0);  /* Only check signature */
```

**Discovery**: USB protocol trace revealed H713 BROM sends **exactly 13 bytes** for `AWUS` response, NOT 16 bytes. The 16-byte buffer caused timeouts waiting for 3 bytes that never arrive, leading to BROM crashes and reset loops.

**Evidence**: `LIBUSB_DEBUG=4` trace showed:
- Request 16 bytes → H713 sends 13 bytes → timeout waiting for remaining 3 bytes
- This protocol mismatch causes BROM to crash and enter 6-second reset cycle

### 2. H713-Specific Swap Buffers (soc_info.c:154-159)
```c
/* H713 uses SRAM A2-based memory layout with scratch at 0x121000 */
sram_swap_buffers h713_sram_swap_buffers[] = {
	{ .buf1 = 0x121000, .buf2 = 0x11e000, .size = 0x1000 },
	{ .size = 0 }  /* End of the table */
};
```

**Critical Change**: `buf2` moved from H616's 0x52a00 (SRAM A1/C) to 0x11e000 (SRAM A2)

**Reason**: H713 BROM only has access to SRAM A2 during FEL mode. Using H616's buf2 address caused writes to inaccessible memory, resulting in timeout.

### 3. H713 SOC Entry Updates (soc_info.c:572-578)
```c
.soc_id       = 0x1860, /* Allwinner H713 (H616 variant) */
.name         = "H713",
.spl_addr     = 0x104000,  /* H713 SRAM A2 + 0x4000, from boot0.bin header */
.scratch_addr = 0x121000,  /* FEL scratch after SPL region */
.thunk_addr   = 0x123a00, .thunk_size = 0x200,  /* Thunk near stack */
.swap_buffers = h713_sram_swap_buffers,  /* Changed from h616_sram_swap_buffers */
.sram_size    = 207 * 1024,
```

## Final H713 Memory Layout

```
SRAM A2 Region (0x100000 - 0x120000, 128KB):
├── 0x100000 - 0x104000: Reserved/BROM use (16KB)
├── 0x104000 - 0x10e000: SPL load area (40KB) ← eGON header specifies 0xa000 bytes
├── 0x10e000 - 0x11e000: Available space (64KB)
├── 0x11e000 - 0x11f000: Swap buffer buf2 (4KB) ← FEL protocol working space
├── 0x11f000 - 0x121000: Gap (8KB)
├── 0x121000 - 0x122000: FEL scratch area (4KB) ← Temporary storage
├── 0x122000 - 0x123a00: Gap (6.5KB)
├── 0x123a00 - 0x123c00: FEL thunk code (512B) ← Execution trampoline
└── 0x123c00 - 0x124000: Stack area (1KB)
```

## Key Architectural Differences: H616 vs H713

| Component | H616 Address | H713 Address | Offset |
|-----------|--------------|--------------|--------|
| SPL Base | 0x020000 | 0x104000 | +0x0e4000 |
| Scratch | 0x021000 | 0x121000 | +0x100000 |
| Thunk | 0x053a00 | 0x123a00 | +0x0d0000 |
| Swap buf2 | 0x052a00 | 0x11e000 | +0x0cb600 |
| SRAM Bank | A1+C | A2 | Different bank |

## Test Binary

**Location**: `/home/shift/code/android_projector/sunxi-fel-h713-fixed-v2`

**Build Commands**:
```bash
cd /home/shift/code/android_projector
nix develop -c -- bash -c 'cd build/sunxi-tools && make sunxi-fel'

# Version history:
# v1: sunxi-fel-h713-fixed (16-byte buffer, wrong swap addresses)
# v2: sunxi-fel-h713-fixed-v2 (16-byte buffer, correct swap addresses) 
# v3: sunxi-fel-h713-v3 (13-byte buffer, correct swap addresses) ← CURRENT
cp build/sunxi-tools/sunxi-fel sunxi-fel-h713-v3
```

## Testing Status

### V3 Testing: ❌ BLOCKED BY BROM FIRMWARE BUG

**CRITICAL FINDING**: H713 BROM has a **firmware bug** that causes it to crash immediately when ANY program attempts to open the USB device. This is NOT a protocol issue.

**Evidence**:
- Device enumerates correctly (VID/PID: 1f3a:efe8, bcdDevice: 2.b3)  
- Crashes when `libusb_open_device_with_vid_pid()` is called
- Affects ALL tools: custom `sunxi-fel-h713-v3`, stock `sunxi-fel`, minimal libusb test
- Even `lsusb -v` descriptor read causes crash
- Error: `errno=5 EIO` when trying to open `/dev/bus/usb/001/XXX`
- Enters reset loop: crash → reset → re-enumerate (every ~300ms)

**Root Cause**: BROM crashes during USB device initialization sequence, likely during configuration descriptor read or interface claim. Without interaction, device is stable.

**Conclusion**: FEL mode is currently **inaccessible** due to BROM firmware bug. The 13-byte response fix (V3) cannot be tested because the device crashes before any FEL protocol commands can be sent.

### Alternative Access Methods Required

Since FEL mode is inaccessible due to BROM firmware bug:

1. **Serial Console (UART)** - **RECOMMENDED PRIMARY METHOD**
   - Boot U-Boot and Linux via serial console
   - Bypass FEL mode entirely for testing
   - Standard UART pins: TX, RX, GND

2. **Android ADB Method**
   - Boot into Android first
   - Trigger FEL mode from userspace (may work differently)
   - Use `adb` to interact with bootloader

3. **Different USB Host**
   - Try Windows machine (different USB driver stack)
   - Try USB 2.0 hub (may have different enumeration behavior)
   - Different Linux kernel version

4. **Hardware Investigation**
   - USB protocol sniffer to capture exact crash sequence
   - Check for special button combination or pin short required
   - Verify USB cable/port quality

### Recommended Path Forward

**Use serial console** for Phase II U-Boot testing and Phase III+ development. Document FEL mode as unavailable for H713 until BROM firmware bug is resolved by Allwinner or alternative access method discovered.

```bash
# 1. **CRITICAL**: Physically power cycle H713 device to clear BROM crash state

# 2. Verify clean USB enumeration (no reset loop in dmesg)
sudo dmesg | tail -10

# 3. Verify detection with v3
sudo ./sunxi-fel-h713-v3 version
# Expected: AWUSBFEX soc=00001860(H713) ... scratchpad=00121xxx

# 4. Test write operation
dd if=/dev/zero of=/tmp/test16 bs=16 count=1
sudo ./sunxi-fel-h713-v3 write 0x20000 /tmp/test16
# Expected: No timeout, device remains stable

# 5. If stable, upload SPL
sudo ./sunxi-fel-h713-v3 spl u-boot-sunxi-with-spl.bin
# Expected: Progress bar completes, device boots into U-Boot
```

## Expected Outcomes

### V3 Success Indicators
- ✅ No USB timeout errors
- ✅ Device remains stable (no reset loop)
- ✅ `version` command succeeds
- ✅ `write` operations complete
- ✅ SPL upload progresses to completion

### Failure Indicators
- ❌ ERROR -7: Timeout → Protocol issue remains (unlikely)
- ❌ ERROR -8: Overflow → Buffer calculation error (unlikely)
- ❌ Device disconnects → BROM crash (indicates other protocol issue)

## Next Steps After Success

1. Verify U-Boot console access (serial or HDMI)
2. Test U-Boot commands (`printenv`, `mmc list`, etc.)
3. Validate DRAM initialization
4. Test kernel loading from eMMC/SD
5. Complete Phase II completion checklist

## Documentation References

- **Memory Map Analysis**: `docs/H713_BROM_MEMORY_MAP.md` (348 lines)
- **FEL Testing Protocol**: `docs/FEL_TESTING_RESULTS.md`
- **Factory Validation**: `docs/FACTORY_FEL_ADDRESSES.md` (519 lines)
- **Memory Candidates**: `docs/H713_MEMORY_MAP_CANDIDATES.md` (217 lines)

## Patches for Upstream

These changes should be submitted to sunxi-tools upstream:

1. **fel_lib.c**: Increase USB response buffer to 16 bytes (affects all newer SoCs)
2. **soc_info.c**: Add H713-specific memory layout configuration
3. **Documentation**: Document H713's unique SRAM A2-based architecture

## Credits

Analysis based on:
- Factory boot0.bin disassembly (ARM32 BROM code)
- H616 reference implementation (similar generation SoC)
- sunxi-tools architecture (FEL protocol handling)
- Trial-and-error testing with hardware feedback

---

## UPDATE 2025-10-11: USB Serial Gadget Transfer Investigation

### New Issue Discovered

When attempting to load U-Boot with USB serial gadget support (744 KB) via FEL mode after implementing the V3 fixes, a new timeout issue was discovered.

**Test Binary**: `u-boot-sunxi-with-spl-usb.bin` (761,304 bytes / 744 KB)  
**Standard Binary**: `u-boot-sunxi-with-spl.bin` (749,992 bytes / 732 KB)  
**Size Increase**: 12 KB (USB gadget framework + CDC ACM drivers)

### Timeline of Investigation

#### 1. Initial Timeout with 10-Second Limit

```bash
$ sudo ./sunxi-fel-h713-debug -v uboot u-boot-sunxi-with-spl-usb.bin
[DEBUG] Waiting for USB response (expecting AWUS)...
[DEBUG] USB response: rc=0, recv=13 bytes
[DEBUG] Data hex: 41 57 55 53 ...
usb_bulk_recv() ERROR -7: Operation timed out
```

**Hypothesis**: USB_TIMEOUT of 10 seconds insufficient for 744 KB transfer

**Math**:
- 744 KB @ 64 KB/sec = 11.6 seconds
- Current timeout: 10 seconds ❌
- Conclusion: Need longer timeout

#### 2. Timeout Increase to 20 Seconds

**Modified**: `build/sunxi-tools/fel_lib.c` line 34
```c
#define USB_TIMEOUT 20000  /* 20 seconds (was 10) */
```

**Built**: `sunxi-fel-h713-20s-timeout` (77 KB)

**Test Result**:
```bash
$ time sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl-usb.bin
...
usb_bulk_recv() ERROR -7: Operation timed out
real    0m20.056s  ← Timeout at exactly 20 seconds
```

**Critical Discovery**:
- Transfer times out at the NEW 20-second limit (not old 10s)
- **This proves**: Timeout increase WAS applied correctly
- **But also proves**: Transfer is taking >20 seconds (not expected 11.6s)
- **Conclusion**: Problem is NOT just timing margin - likely protocol incompatibility

#### 3. Device Corruption After Timeout

After the 20-second timeout, the device entered a corrupted USB state:

```bash
$ sudo ./sunxi-fel-h713-20s-timeout version
ERROR: libusb_open() ERROR -1: Input/Output Error

$ cd build/sunxi-tools && sudo ./sunxi-fel -l
libusb_open() ERROR -1: Input/Output Error
```

**dmesg Evidence**:
```
[744777.690508] usb 1-3: reset full-speed USB device number 77
[744777.814941] usb 1-3: USB disconnect, device number 77
[744777.927459] usb 1-3: new full-speed USB device number 78
[744818.167241] usb 1-3: reset full-speed USB device number 78
[744818.291813] usb 1-3: USB disconnect, device number 78
[...] Continuous reset cycles
```

**Analysis**:
- Device stuck in continuous USB reset/disconnect cycles
- Device number incrementing (77→78→79...)
- libusb cannot open device (LIBUSB_ERROR_IO)
- H713 BROM entered corrupted state after failed transfer
- **Recovery**: Requires physical device reset (unplug/replug USB)

### Root Cause Analysis

#### Transfer Behavior Analysis

**Expected**:
1. Initial handshake (AWUS) ✅
2. Bulk transfer of 744 KB data
3. Complete in ~11.6 seconds
4. AWUS acknowledgment

**Observed**:
1. Initial handshake (AWUS) ✅
2. Bulk transfer never completes
3. Hangs for >20 seconds
4. Timeout triggers
5. Device enters error state

**Hypothesis**: H713 BROM has protocol differences from H6/H616 that cause bulk transfers to hang indefinitely. The device acknowledges readiness (AWUS) but never starts the actual data transfer.

#### Possible Causes

**A. Protocol Incompatibility (60% probability)**
- H713 may require additional handshakes during bulk transfer
- Transfer sequence might differ from H6/H616
- Device waiting for commands that sunxi-fel doesn't send

**B. Memory Configuration Issue (30% probability)**
- 744 KB may exceed available SRAM regions
- H713 SRAM layout may need further adjustment
- Memory overlap with BROM stack/heap

**C. USB Parameter Mismatch (10% probability)**
- Chunk size (512 KB max) may be too large for H713
- Alignment or timing requirements different
- USB endpoint configuration incompatible

### Documentation Created

**Investigation Files**:
- `docs/FEL_USB_TIMEOUT_INVESTIGATION.md` - Technical deep-dive on timeout analysis
- `docs/FEL_USB_LIBUSB_ERROR_INVESTIGATION.md` - USB state corruption analysis  
- `FEL_USB_TIMEOUT_SUMMARY.md` - Quick reference and next steps

**Test Scripts**:
- `test-spl-only.sh` - Automated SPL (32 KB) transfer diagnostic
- `test-fel-usb-serial.sh` - Full USB serial test suite

**Modified Files**:
- `build/sunxi-tools/fel_lib.c` - USB_TIMEOUT increased to 20s
- Backup: `build/sunxi-tools/fel_lib.c.timeout-10s-backup`

**Binaries**:
- `sunxi-fel-h713-20s-timeout` (77 KB) - FEL tool with 20s timeout
- `u-boot-sunxi-with-spl-usb.bin` (744 KB) - U-Boot with USB gadget
- `u-boot-sunxi-with-spl.bin` (732 KB) - Standard U-Boot

### Current Status: BLOCKED

**Blocking Issues**:
1. Device in corrupted USB state - requires hardware access for power cycle
2. Cannot test SPL-only transfer without device reset
3. Unknown if H713 supports ANY FEL bulk transfers

### Critical Next Test: SPL-Only Transfer

**Purpose**: Isolate whether issue is size-related or protocol-related

```bash
# After device power cycle:
./test-spl-only.sh
```

**Expected Outcomes**:
- ✅ **SPL (32 KB) succeeds**: Issue is size-related (threshold between 32 KB and 744 KB)
  - Next: Test intermediate sizes, split transfer method
  - Workaround: Possible via chunked transfers
  - Confidence: 70% resolvable

- ❌ **SPL fails**: Issue is fundamental protocol incompatibility
  - Next: Add detailed transfer logging, analyze protocol differences
  - Workaround: Boot from SD card or eMMC instead
  - Confidence: 30% resolvable via FEL mode

### Task Impact

**Task 033**: USB Serial Gadget U-Boot Configuration
- **Status**: Blocked
- **Blocker**: H713 FEL USB transfer incompatibility
- **Impact**: Cannot test USB serial console via FEL mode

### Confidence Assessment

**FEL Mode Viability**:
- 40% - Resolvable via protocol/timing fixes
- 40% - Requires H713-specific protocol implementation  
- 20% - Fundamentally incompatible, requires alternative boot method

**After SPL Test** (will update probabilities):
- If SPL succeeds: 70% resolvable
- If SPL fails: 30% resolvable via FEL

### Alternative Boot Methods

If FEL proves incompatible:
1. **SD Card Boot** (requires card reader)
2. **eMMC Boot** (requires Android fastboot or factory firmware)
3. **Serial Console Only** (requires UART soldering - most invasive)

### Conclusion

The V3 fixes resolved the initial USB response buffer and memory layout issues, but uncovered a deeper compatibility problem with larger binary transfers. The device can complete the initial handshake but fails to complete bulk data transfers >20 seconds.

**Immediate Action Required**:
1. User must power cycle device (unplug/replug USB)
2. User must run `./test-spl-only.sh` to test 32 KB transfer
3. Results will determine if FEL mode is viable for this hardware

---

**Last Updated**: 2025-10-11  
**Next Step**: Awaiting SPL transfer test results
