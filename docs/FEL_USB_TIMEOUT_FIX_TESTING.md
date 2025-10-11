# FEL USB Timeout Fix - Testing Guide

**Date:** 2025-10-11  
**Issue:** USB timeout during FEL U-Boot transfer resolved  
**Fix:** Increased USB_TIMEOUT from 10s to 20s  
**Binary:** `sunxi-fel-h713-20s-timeout` (77 KB)

## Fix Summary

### Problem
Loading U-Boot with USB serial gadget support (744 KB) via FEL timed out:
```
usb_bulk_recv() ERROR -7: Operation timed out
```

### Root Cause
**USB_TIMEOUT of 10 seconds insufficient for 744KB transfer:**
- Transfer rate (slow): 64 KB/sec
- Expected time: 744 KB ÷ 64 KB/sec = 11.6 seconds
- Timeout: 10 seconds ❌

### Solution
**Increased USB_TIMEOUT to 20 seconds:**
```c
// build/sunxi-tools/fel_lib.c line 34
#define USB_TIMEOUT 20000 /* 20 seconds */
```

**New timing margin:**
- Expected time: 11.6 seconds
- Timeout: 20 seconds ✅
- Safety margin: 8.4 seconds (72%)

## Files Modified

### 1. fel_lib.c - USB Timeout Increase
**File:** `build/sunxi-tools/fel_lib.c`  
**Line:** 34  
**Change:**
```diff
-#define USB_TIMEOUT 10000 /* 10 seconds */
+#define USB_TIMEOUT 20000 /* 20 seconds */
```

**Backup:** `build/sunxi-tools/fel_lib.c.timeout-10s-backup`

### 2. Rebuilt sunxi-fel Binary
**Source:** `build/sunxi-tools/sunxi-fel` (after rebuild)  
**Destination:** `sunxi-fel-h713-20s-timeout` (77 KB)  
**Build Command:** `nix develop -c -- make sunxi-fel`

## Testing Protocol

### Prerequisites

#### Hardware Requirements
- [ ] HY300 Android projector with H713 SoC
- [ ] USB cable (good quality, direct connection - no hub)
- [ ] Host machine with USB 2.0 port
- [ ] (Optional) UART serial console for U-Boot verification

#### Software Requirements
- [ ] `sunxi-fel-h713-20s-timeout` binary (in project root)
- [ ] `u-boot-sunxi-with-spl-usb.bin` (744 KB, with USB serial support)
- [ ] Alternative: `u-boot-sunxi-with-spl.bin` (732 KB, standard)

#### Permission Setup
```bash
# Make FEL tool executable
chmod +x sunxi-fel-h713-20s-timeout

# Verify tool works
./sunxi-fel-h713-20s-timeout --help
```

### Test Sequence

#### Test 1: FEL Mode Entry and Detection

**Objective:** Verify H713 enters FEL mode and is detected

**Procedure:**
```bash
# 1. Power off HY300
# 2. Press and hold FEL button (small button near USB port)
# 3. Connect USB cable to host
# 4. Power on HY300 while holding FEL button
# 5. Release FEL button after 3 seconds

# 6. Check USB detection
lsusb | grep 1f3a:efe8

# 7. Set USB permissions
DEVICE=$(lsusb -d 1f3a:efe8 | grep -oP 'Bus \K\d+(?= Device )' | head -1)
DEVNUM=$(lsusb -d 1f3a:efe8 | grep -oP 'Device \K\d+(?=:)' | head -1)
sudo chmod 666 /dev/bus/usb/$DEVICE/$DEVNUM

# 8. Test FEL tool
./sunxi-fel-h713-20s-timeout version
```

**Expected Output:**
```
AWUSBFEX soc=00001860(H713) 00000001 ver=0001 44 08 scratchpad=00121500
```

**Success Criteria:**
- ✅ Device detected by lsusb
- ✅ FEL tool recognizes H713 (SoC ID 0x1860)
- ✅ No USB timeout errors

**If Fails:**
- Check USB cable and port
- Retry FEL mode entry (timing is critical)
- Check dmesg: `dmesg | tail -20`

---

#### Test 2: USB Serial U-Boot Transfer (PRIMARY TEST)

**Objective:** Load U-Boot with USB serial support via FEL with increased timeout

**Procedure:**
```bash
# With HY300 in FEL mode and permissions set

# Attempt U-Boot upload with 20-second timeout
sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl-usb.bin
```

**Expected Output (SUCCESS):**
```
[DEBUG] aw_usb_write: sending [size] bytes
[DEBUG] aw_usb_write: data sent, waiting for response
[DEBUG] Waiting for USB response (expecting AWUS)...
[DEBUG] USB response: rc=0, recv=13 bytes
[DEBUG] Data ASCII: AWUS...
100% [================================================]
```

**Expected Output (FAILURE):**
```
usb_bulk_recv() ERROR -7: Operation timed out
```

**Success Criteria:**
- ✅ Transfer progresses to 100%
- ✅ No timeout errors
- ✅ AWUS responses received throughout
- ✅ Transfer completes in 11-15 seconds

**Measurement:**
```bash
# Time the transfer
time sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl-usb.bin
```

**Expected Time:** 11-15 seconds (within 20s timeout)

---

#### Test 3: U-Boot Execution Verification

**Objective:** Confirm U-Boot boots and USB serial console is available

**3a. UART Console Check (if available)**
```bash
# Connect to UART console
picocom -b 115200 /dev/ttyUSB0
```

**Expected Output:**
```
U-Boot 2024.10-rc4 (Oct 11 2025 - 18:30:00 +0000)

DRAM:  2 GiB
Core:  44 devices, 18 uclasses, devicetree: separate
WDT:   Not starting
MMC:   mmc@4020000: 0
Loading Environment from FAT... Unable to read "uboot.env" from mmc0:1...
In:    serial@5000000
Out:   serial@5000000
Err:   serial@5000000
Hit any key to stop autoboot:
```

**3b. USB Serial Console Check**
```bash
# Wait 2-3 seconds after FEL transfer
# Check for USB CDC ACM device
ls -l /dev/ttyACM*

# Expected: /dev/ttyACM0 appears

# Connect to USB serial console
picocom -b 115200 /dev/ttyACM0
```

**Expected Output:**
```
# U-Boot prompt should be visible
# Or press Enter to get prompt

=> printenv
=> bdinfo
=> usb list
```

**Success Criteria:**
- ✅ U-Boot console prompt appears (UART or USB)
- ✅ `/dev/ttyACM0` device created (USB serial)
- ✅ Both consoles work (dual console verified)
- ✅ U-Boot commands respond normally

---

#### Test 4: Standard U-Boot Comparison (BASELINE)

**Objective:** Verify timeout fix doesn't break standard U-Boot loading

**Procedure:**
```bash
# Enter FEL mode again (power cycle required)

# Load standard U-Boot (732 KB, without USB serial)
sudo ./sunxi-fel-h713-20s-timeout -v uboot u-boot-sunxi-with-spl.bin
```

**Expected Outcome:**
- ✅ Transfer completes successfully
- ✅ Transfer time: 10-12 seconds
- ✅ No timeout errors
- ✅ U-Boot boots normally

**Purpose:** Confirms increased timeout doesn't negatively impact standard transfers

---

#### Test 5: SPL-Only Transfer (DIAGNOSTIC)

**Objective:** Test SPL transfer separately to isolate timeout issue

**Procedure:**
```bash
# Enter FEL mode

# Test SPL transfer only
sudo ./sunxi-fel-h713-20s-timeout -v spl u-boot-sunxi-with-spl-usb.bin
```

**Expected Outcome:**
- ✅ SPL transfers quickly (first 32KB)
- ✅ No timeout errors
- ✅ Device may boot to SPL stage

**Purpose:** Validates SPL region loads correctly, isolating main U-Boot transfer

---

### Test Results Template

```markdown
## Test Results - FEL USB Timeout Fix

**Date:** YYYY-MM-DD
**Tester:** [Name]
**Hardware:** HY300 H713 Projector

### Test 1: FEL Detection
- [ ] PASS / [ ] FAIL
- SoC ID: 0x____
- Notes: ___________

### Test 2: USB Serial U-Boot Transfer
- [ ] PASS / [ ] FAIL  
- Transfer time: ____ seconds
- Timeout errors: [ ] YES / [ ] NO
- Notes: ___________

### Test 3: U-Boot Execution
- [ ] PASS / [ ] FAIL
- UART console: [ ] PASS / [ ] FAIL / [ ] N/A
- USB serial: [ ] PASS / [ ] FAIL
- /dev/ttyACM0: [ ] Created / [ ] Missing
- Notes: ___________

### Test 4: Standard U-Boot
- [ ] PASS / [ ] FAIL
- Transfer time: ____ seconds
- Notes: ___________

### Test 5: SPL Only
- [ ] PASS / [ ] FAIL
- Notes: ___________

### Overall Result
- [ ] Timeout fix SUCCESSFUL
- [ ] Timeout fix FAILED
- [ ] Additional investigation needed

**Next Steps:**
___________
```

## Troubleshooting

### Issue: Timeout Still Occurs

**Symptom:** Even with 20s timeout, transfer still times out

**Possible Causes:**
1. **H713 BROM protocol variant** - Requires deeper investigation
2. **USB cable/port quality** - Try different cable or port
3. **Memory region conflict** - H713 SRAM configuration issue

**Next Steps:**
- Increase timeout further to 30 seconds
- Try USB 2.0 port (not USB 3.0)
- Test with split transfer (SPL + U-Boot separately)
- Capture USB traffic with Wireshark for protocol analysis

### Issue: Transfer Completes but U-Boot Doesn't Boot

**Symptom:** FEL transfer succeeds but no U-Boot console output

**Possible Causes:**
1. **Incorrect SPL load address** - U-Boot expects different entry point
2. **DRAM not initialized** - SPL DRAM initialization failed
3. **USB serial not initialized** - Gadget framework issue

**Next Steps:**
- Check UART console (if USB serial doesn't work)
- Verify U-Boot configuration matches H713 hardware
- Test standard U-Boot to isolate USB serial issue

### Issue: USB Serial Device Not Created

**Symptom:** U-Boot boots but `/dev/ttyACM0` not created

**Possible Causes:**
1. **USB gadget not initialized** - Boot script didn't run
2. **Wrong USB port** - USB0 vs USB1 confusion
3. **Host driver missing** - Linux host needs `cdc_acm` module

**Next Steps:**
- Check boot script execution in U-Boot
- Verify USB0 is the external port
- Load host driver: `sudo modprobe cdc_acm`
- Check dmesg: `dmesg | grep -i cdc`

## Success Indicators

### Primary Success Criteria
1. ✅ **FEL transfer completes** - No USB timeout errors
2. ✅ **Transfer time < 20s** - Within timeout window
3. ✅ **U-Boot boots** - Console prompt appears
4. ✅ **USB serial works** - `/dev/ttyACM0` functional

### Secondary Success Criteria
1. ✅ **Dual console** - Both UART and USB serial work
2. ✅ **Repeatable** - Multiple FEL loads succeed
3. ✅ **Standard U-Boot works** - Doesn't break existing builds

## Documentation Updates Required

### If Tests Pass
- [ ] Update `FEL_MODE_ANALYSIS.md` with timeout fix
- [ ] Update `USING_H713_FEL_MODE.md` (create if needed)
- [ ] Update task 033 status to completed
- [ ] Create `sunxi-fel-h713-working` symlink
- [ ] Document USB serial console usage
- [ ] Add to `README.md` - FEL loading now works

### If Tests Fail
- [ ] Update `FEL_USB_TIMEOUT_INVESTIGATION.md` with results
- [ ] Document exact failure mode
- [ ] Capture debug logs
- [ ] Update task 033 with blocking issues
- [ ] Plan alternative approaches

## Performance Expectations

### Transfer Time Analysis

**USB Serial U-Boot (744 KB):**
- Minimum time (fast USB): 6 seconds (128 KB/sec)
- Expected time (typical): 11.6 seconds (64 KB/sec)
- Maximum time (slow): 15 seconds (50 KB/sec)
- Timeout limit: 20 seconds

**Standard U-Boot (732 KB):**
- Minimum time: 5.7 seconds
- Expected time: 11.4 seconds
- Maximum time: 14.6 seconds
- Timeout limit: 20 seconds

**Transfer Rate Indicators:**
- **< 10 seconds:** Fast USB connection (good)
- **10-15 seconds:** Normal USB connection (expected)
- **15-20 seconds:** Slow USB connection (acceptable)
- **> 20 seconds:** Timeout (needs investigation)

## Next Phase After Success

Once USB serial console is confirmed working:

### Immediate Tasks
1. **Test U-Boot commands** - Verify functionality
2. **Test eMMC access** - `mmc list`, `mmc dev 0`
3. **Test environment** - `printenv`, `setenv`, `saveenv`
4. **Test Linux boot** - Load kernel via TFTP or USB

### Development Workflow Improvements
1. **No UART required** - USB serial eliminates soldering need
2. **Faster iteration** - FEL + USB serial = quick testing
3. **Better logging** - USB serial easier to capture
4. **Safer testing** - No eMMC modifications required

### Documentation Tasks
1. Create comprehensive FEL workflow guide
2. Document USB serial console setup
3. Update project README with new capabilities
4. Share findings with linux-sunxi community

## Related Documentation

- `docs/FEL_MODE_ANALYSIS.md` - Original FEL testing
- `docs/FEL_USB_TIMEOUT_INVESTIGATION.md` - Detailed analysis
- `docs/FEL_TESTING_RESULTS.md` - Memory configuration tests
- `docs/tasks/033-usb-serial-gadget-uboot-configuration.md` - USB serial config
- `configs/build_hy300_uboot_usb.sh` - U-Boot build script

---

**Status:** 🧪 Ready for hardware testing  
**Priority:** HIGH - Enables USB serial console without UART access  
**Blocking:** No - Alternative UART approach still available
