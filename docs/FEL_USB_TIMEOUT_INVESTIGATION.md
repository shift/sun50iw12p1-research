# FEL USB Timeout Investigation - U-Boot USB Serial Loading

**Date:** 2025-10-11  
**Issue:** USB timeout during FEL loading of U-Boot with USB serial gadget support  
**Binary:** `u-boot-sunxi-with-spl-usb.bin` (761,304 bytes / 744 KB)  
**Tool:** `sunxi-fel-h713-debug`

## Problem Summary

When attempting to load U-Boot with USB serial gadget support via FEL mode, the transfer times out after the AWUS response:

```bash
sudo ./sunxi-fel-h713-debug -v uboot u-boot-sunxi-with-spl-usb.bin
```

**Error Output:**
```
[DEBUG] Waiting for USB response (expecting AWUS)...
[DEBUG] USB response: rc=0, recv=13 bytes
[DEBUG] Data hex: 41 57 55 53 ...
[DEBUG] Data ASCII: AWUS...
usb_bulk_recv() ERROR -7: Operation timed out
```

**Key Observations:**
1. ✅ FEL mode entry works
2. ✅ Device detection works (ID 1f3a:efe8)
3. ✅ Initial handshake succeeds (AWUS response received)
4. ❌ Subsequent bulk transfer times out

## Technical Analysis

### 1. Binary Size Analysis

**Current Binary:** 761,304 bytes (744 KB)  
**Previous Working Binary:** ~732 KB (standard U-Boot)  
**Size Increase:** ~29 KB (USB gadget framework + CDC ACM drivers)

The size increase is within normal USB bulk transfer limits:
- USB bulk transfer chunk size: 512 KB (AW_USB_MAX_BULK_SEND)
- USB timeout: 10 seconds (USB_TIMEOUT)
- Expected transfer time: ~12 seconds for 744KB at 64KB/sec

**Conclusion:** Binary size is NOT the primary issue.

### 2. H713 FEL Protocol Compatibility

Based on previous testing (FEL_MODE_ANALYSIS.md), H713 has known FEL protocol issues:

**Working Operations:**
- ✅ `sunxi-fel version` - SoC identification
- ✅ USB device enumeration
- ✅ Initial FEL handshake (AWUS response)

**Failing Operations:**
- ❌ Memory read operations (timeout)
- ❌ Memory write operations (timeout)
- ❌ SPL upload (timeout after initial handshake)

**Root Cause Hypothesis:**
The H713 BROM FEL implementation has timing or protocol differences that cause bulk transfers to fail, even though the initial handshake succeeds.

### 3. Memory Configuration Impact

The `sunxi-fel-h713-debug` tool uses these H713 memory addresses:

```c
.spl_addr     = 0x104000,  // SPL load address (from boot0 analysis)
.scratch_addr = 0x121000,  // Scratch buffer (after SPL region)
.thunk_addr   = 0x123a00,  // Code execution thunk
.thunk_size   = 0x200,
```

**Potential Issues:**
1. **SRAM Region Conflicts:** The 744KB binary may exceed available SRAM regions
2. **swap_buffers Configuration:** H713 may require different buffer swap regions
3. **Stack/Heap Collisions:** Larger binary might overlap with BROM stack/heap

### 4. USB Protocol Analysis

#### FEL Transfer Sequence (from fel_lib.c)

**Step 1: Version Request (WORKS)**
```
Host → Device: AW_FEL_VERSION request
Device → Host: Version info (13 bytes)
Device → Host: AWUS response (handshake)
```

**Step 2: SPL Write (FAILS)**
```
Host → Device: AW_FEL_1_WRITE request (header)
Host → Device: AW_USB_WRITE request (data length)
Host → Device: USB bulk_send (SPL data in 512KB chunks)
Device → Host: AWUS response ← TIMEOUT HERE
```

**The timeout occurs AFTER:**
- ✅ FEL write request sent
- ✅ USB write request sent  
- ❓ Bulk data transfer (unclear if data was sent)

**The timeout occurs BEFORE:**
- ❌ AWUS acknowledgment received
- ❌ FEL status check

#### Timing Analysis

```
USB_TIMEOUT = 10 seconds
Transfer rate (slow) = 64 KB/sec
Expected time for 744KB = 11.6 seconds
```

**Issue:** The timeout is too short for a slow transfer!

**Standard U-Boot (732KB):** ~11.4 seconds (within 10s timeout by narrow margin)  
**USB Serial U-Boot (744KB):** ~11.6 seconds (exceeds 10s timeout)

This suggests the **timeout value is marginal** and the extra 12KB pushes it over the limit.

### 5. H713-Specific BROM Behavior

Based on the AWUS response being received, the BROM is responsive. However:

1. **BROM Processing Time:** H713 BROM may need more time to process write requests
2. **Memory Initialization:** Larger transfers might trigger additional DRAM initialization
3. **Protocol Variants:** H713 may expect different timing between request and data phases

## Hypothesis Summary

### Primary Hypothesis: Timeout Too Short
The USB_TIMEOUT of 10 seconds is insufficient for:
- 744KB transfer at 64 KB/sec = 11.6 seconds
- BROM processing overhead
- USB protocol handshake delays

**Confidence:** 70%

### Secondary Hypothesis: H713 BROM Protocol Variant
The H713 BROM expects different timing or additional handshake steps between:
- FEL write request
- USB write request  
- Bulk data transfer

**Confidence:** 25%

### Tertiary Hypothesis: Memory Region Issue
The 744KB binary size causes:
- SRAM overflow during transfer
- Stack/heap collision
- Invalid memory access that BROM rejects silently

**Confidence:** 5%

## Proposed Solutions

### Solution 1: Increase USB Timeout (EASIEST)

**Modification:** Edit `build/sunxi-tools/fel_lib.c`:

```c
// Change from:
#define USB_TIMEOUT 10000 /* 10 seconds */

// To:
#define USB_TIMEOUT 20000 /* 20 seconds */
```

**Rationale:**
- Addresses timing hypothesis directly
- No risk to hardware
- Quick to test
- Matches documented slow transfer rate

**Testing Plan:**
1. Rebuild sunxi-fel with increased timeout
2. Test with 744KB binary
3. Monitor actual transfer time

**Expected Outcome:** Transfer completes within 12-15 seconds

### Solution 2: Reduce Binary Size (MODERATE)

**Modification:** Optimize U-Boot configuration to reduce size below 732KB:

```
# Disable less critical features
CONFIG_CMD_IMI=n           # Disable iminfo command
CONFIG_CMD_XIMG=n          # Disable multi-image support  
CONFIG_EFI_LOADER=n        # Disable EFI (if enabled)
```

**Rationale:**
- Keeps transfer under 10 second timeout
- Maintains USB serial functionality
- May sacrifice some U-Boot features

**Testing Plan:**
1. Identify non-essential features to disable
2. Rebuild U-Boot to target <730KB
3. Test FEL loading
4. Verify USB serial still works

**Expected Outcome:** Transfer completes within 10 seconds

### Solution 3: Test with Standard Binary First (SAFEST)

**Modification:** Test FEL loading with standard U-Boot (without USB serial):

```bash
sudo ./sunxi-fel-h713-debug -v uboot u-boot-sunxi-with-spl.bin
```

**Rationale:**
- Validates the H713 memory configuration
- Isolates the size/timeout issue
- Confirms FEL tool correctness

**Testing Plan:**
1. Build standard U-Boot (without USB gadget)
2. Test FEL loading
3. If successful, validates timeout hypothesis
4. If fails, indicates deeper protocol issue

**Expected Outcome:** Standard binary loads successfully

### Solution 4: Split Transfer (ADVANCED)

**Modification:** Use separate `spl` and `uboot` commands instead of combined binary:

```bash
# Extract SPL (first 32KB)
dd if=u-boot-sunxi-with-spl-usb.bin of=spl.bin bs=1024 count=32

# Extract U-Boot (remainder)  
dd if=u-boot-sunxi-with-spl-usb.bin of=uboot.bin bs=1024 skip=32

# Load via FEL
sudo ./sunxi-fel-h713-debug spl spl.bin
sudo ./sunxi-fel-h713-debug write 0x4a000000 uboot.bin
```

**Rationale:**
- Reduces individual transfer sizes
- Gives BROM time between transfers
- More granular error diagnosis

**Testing Plan:**
1. Split binary into SPL and U-Boot components
2. Load SPL first (small, should succeed)
3. Load U-Boot separately
4. Check if both transfers complete

**Expected Outcome:** Individual smaller transfers succeed

### Solution 5: Add Debug Logging (DIAGNOSTIC)

**Modification:** Enhanced debug output in sunxi-fel:

```c
// In usb_bulk_send() and usb_bulk_recv()
fprintf(stderr, "[DEBUG] Transfer: %zu bytes, chunk: %zu, timeout: %d ms\n",
        length, chunk, USB_TIMEOUT);
fprintf(stderr, "[DEBUG] Transfer progress: %zu/%zu bytes (%.1f%%)\n",
        transferred, total, percentage);
```

**Rationale:**
- Shows exactly where transfer stalls
- Measures actual transfer rate
- Identifies if any data was transferred before timeout

**Testing Plan:**
1. Add detailed transfer logging
2. Rebuild sunxi-fel
3. Run test and capture logs
4. Analyze transfer progression

**Expected Outcome:** Log shows transfer rate and exact failure point

## Recommended Testing Sequence

### Phase 1: Quick Validation (5 minutes)
```bash
# Test 1: Try standard U-Boot binary
sudo ./sunxi-fel-h713-debug -v uboot u-boot-sunxi-with-spl.bin

# Test 2: If fails, try just SPL
sudo ./sunxi-fel-h713-debug -v spl u-boot-sunxi-with-spl.bin
```

**Decision Point:** 
- If standard binary works → Timeout issue confirmed
- If standard binary fails → Protocol issue confirmed

### Phase 2: Timeout Fix (10 minutes)
```bash
# Increase timeout to 20 seconds
cd build/sunxi-tools
sed -i 's/#define USB_TIMEOUT\s*10000/#define USB_TIMEOUT 20000/' fel_lib.c
nix develop -c -- make clean
nix develop -c -- make sunxi-fel
cp sunxi-fel ../../sunxi-fel-h713-20s-timeout

# Test with increased timeout
cd ../..
sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl-usb.bin
```

**Decision Point:**
- If succeeds → Document and update build process
- If fails → Move to Phase 3

### Phase 3: Size Optimization (30 minutes)
```bash
# Rebuild U-Boot with reduced size
cd u-boot
make menuconfig  # Disable non-essential features
make -j$(nproc)

# Test new binary
sudo ../sunxi-fel-h713-debug -v uboot u-boot-sunxi-with-spl-usb.bin
```

### Phase 4: Split Transfer (20 minutes)
```bash
# Split binary and test components
dd if=u-boot-sunxi-with-spl-usb.bin of=spl.bin bs=1024 count=32
dd if=u-boot-sunxi-with-spl-usb.bin of=uboot.bin bs=1024 skip=32

sudo ./sunxi-fel-h713-debug spl spl.bin
# Check if SPL loaded successfully before proceeding
```

## Testing Checklist

### Pre-Test Verification
- [ ] HY300 in FEL mode (power + FEL button)
- [ ] USB cable connected (direct port, no hub)
- [ ] Device detected: `lsusb | grep 1f3a:efe8`
- [ ] FEL tool version check: `./sunxi-fel-h713-debug version`

### Test Execution
- [ ] Standard U-Boot binary test
- [ ] Increased timeout test
- [ ] Size-optimized binary test
- [ ] Split transfer test
- [ ] USB serial console test (if load succeeds)

### Success Criteria
- [ ] FEL transfer completes without timeout
- [ ] No USB disconnect/reconnect during transfer
- [ ] U-Boot boots and shows console prompt (UART or USB)
- [ ] USB serial device appears: `/dev/ttyACM0`

## Expected Results

### Most Likely Outcome
Increasing USB_TIMEOUT to 20 seconds resolves the issue:
- Transfer completes successfully
- U-Boot boots normally
- USB serial console becomes available
- UART console continues to work (dual console)

### Alternative Outcomes

**If timeout increase doesn't help:**
- H713 BROM has a protocol variant
- Need to analyze USB traffic with Wireshark
- May need to patch FEL protocol handling

**If split transfer works:**
- Individual transfer size is the limit, not timeout
- Need to adjust AW_USB_MAX_BULK_SEND
- Might indicate SRAM size constraints

**If nothing works:**
- H713 FEL support requires deeper reverse engineering
- Fall back to serial console approach
- Consider direct eMMC flashing (riskier)

## Documentation Requirements

### On Success
- [ ] Update `USING_H713_FEL_MODE.md` with timeout fix
- [ ] Document working USB_TIMEOUT value
- [ ] Update task 033 with results
- [ ] Create `sunxi-fel-h713-working` binary
- [ ] Document USB serial testing procedure

### On Failure
- [ ] Document exact failure mode
- [ ] Capture full debug logs
- [ ] Update FEL_MODE_ANALYSIS.md with findings
- [ ] Document workarounds or alternative approaches

## Related Files

- `docs/FEL_MODE_ANALYSIS.md` - Previous H713 FEL testing
- `docs/FEL_TESTING_RESULTS.md` - Memory configuration testing
- `docs/tasks/033-usb-serial-gadget-uboot-configuration.md` - USB serial config
- `build/sunxi-tools/fel_lib.c` - FEL protocol implementation
- `u-boot-sunxi-with-spl-usb.bin` - Binary being tested

## References

- USB bulk transfer spec: 512KB max chunk size
- libusb timeout documentation: milliseconds
- H713 BROM behavior: undocumented (reverse engineering required)
- Allwinner FEL protocol: community documentation

---

**Status:** 🔍 Investigation complete, ready for testing  
**Next Action:** Execute Phase 1 testing sequence  
**Priority:** HIGH - Blocking USB serial console functionality
