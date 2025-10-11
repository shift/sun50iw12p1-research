# H713 FEL Memory Configuration Testing Results

**Date:** 2025-10-11  
**Task:** 027d - FEL Memory Configuration Testing  
**Tested Device:** HY300 Android Projector (Allwinner H713 SoC ID 0x1860)  
**Testing Tool:** sunxi-fel-h713-fixed (custom build)

## Test Status

⚠️ **HARDWARE REQUIRED** - Testing cannot proceed without physical HY300 device in FEL mode

## Candidate Configurations Ready for Testing

### Candidate 1: Boot0-Aligned (PRIMARY)
**Binary:** `sunxi-fel-h713-fixed` (already built)  
**Confidence:** 95%

```c
.spl_addr     = 0x104000,  // From boot0.bin header
.scratch_addr = 0x121000,  // After SPL region
.thunk_addr   = 0x123a00,  // Near stack top
.thunk_size   = 0x200,
```

**Test Status:** ⏳ PENDING HARDWARE  
**Expected Result:** SPL upload succeeds without timeout

---

### Candidate 2: Conservative (FALLBACK)
**Configuration:**
```c
.spl_addr     = 0x104000,
.scratch_addr = 0x110000,  // Earlier placement
.thunk_addr   = 0x120000,  // More margin
.thunk_size   = 0x200,
```

**Test Status:** ⏳ PENDING (test if Candidate 1 fails)  
**Build Required:** Yes - modify soc_info.c and rebuild

---

### Candidate 3: SRAM A1 Experiment (LOW PRIORITY)
**Configuration:**
```c
.spl_addr     = 0x104000,
.scratch_addr = 0x021000,  // SRAM A1 region
.thunk_addr   = 0x024000,
.thunk_size   = 0x200,
```

**Test Status:** ⏳ PENDING (test only if 1 & 2 fail)  
**Build Required:** Yes

---

## Hardware Test Protocol

### Prerequisites
1. **Physical Access Required:**
   - HY300 device with USB cable
   - FEL mode boot capability (power + FEL button)
   - Host machine with USB 2.0 port

2. **Software Ready:**
   - ✅ `sunxi-fel-h713-fixed` binary built
   - ✅ `u-boot-sunxi-with-spl.bin` ready (732 KB)
   - ✅ UART console optional (for debugging)

### Test Procedure

#### Step 1: Enter FEL Mode
```bash
# 1. Power off HY300
# 2. Press and hold FEL button (small button near USB)
# 3. Connect USB cable to host
# 4. Power on while holding FEL button
# 5. Release FEL button after 3 seconds
```

#### Step 2: Verify FEL Detection
```bash
./sunxi-fel-h713-fixed version
```

**Expected Output:**
```
AWUSBFEX soc=00001860(H713) 00000001 ver=0001 44 08 scratchpad=00121500
```

**Success Criteria:**
- ✅ SoC ID shows `0x1860` (H713 detected)
- ✅ `scratchpad=00121xxx` (validates memory region)
- ✅ No USB timeout or connection errors

**If Fails:**
- Check USB cable and port
- Retry FEL mode entry procedure
- Check dmesg: `dmesg | tail -20`

#### Step 3: Test SPL Upload (Candidate 1)
```bash
./sunxi-fel-h713-fixed spl u-boot-sunxi-with-spl.bin
```

**Expected Output (SUCCESS):**
```
100% [================================================]
```

**Expected Output (FAILURE - previous issue):**
```
ERROR -7: Operation timed out
```

**Success Criteria:**
- ✅ Upload completes without timeout
- ✅ Progress bar reaches 100%
- ✅ Device remains responsive after upload
- ✅ No USB disconnect/reconnect

#### Step 4: Verify U-Boot Execution (if UART available)
```bash
# Connect UART console (optional but recommended)
picocom -b 115200 /dev/ttyUSB0
```

**Expected U-Boot Output:**
```
U-Boot 2024.xx (...)
DRAM: 2048 MiB
...
Hit any key to stop autoboot:
```

#### Step 5: Test U-Boot Commands
```
# At U-Boot prompt:
bdinfo      # Check board info
printenv    # Check environment
mmc list    # Check eMMC detection
```

### Fallback Testing (If Candidate 1 Fails)

#### Build and Test Candidate 2
```bash
cd build/sunxi-tools

# Modify soc_info.c lines 568-570
nano soc_info.c
# Change to Candidate 2 values

# Rebuild
nix develop /home/shift/code/android_projector -c -- make clean
nix develop /home/shift/code/android_projector -c -- make sunxi-fel

# Test
cp sunxi-fel ../../sunxi-fel-h713-candidate2
./sunxi-fel-h713-candidate2 version
./sunxi-fel-h713-candidate2 spl u-boot-sunxi-with-spl.bin
```

---

## Test Results Table

| Candidate | spl_addr | scratch_addr | thunk_addr | Test Status | SPL Upload | U-Boot Boot | Notes |
|-----------|----------|--------------|------------|-------------|------------|-------------|-------|
| 1 (Boot0) | 0x104000 | 0x121000 | 0x123a00 | ⏳ PENDING | - | - | Requires hardware |
| 2 (Conservative) | 0x104000 | 0x110000 | 0x120000 | ⏳ PENDING | - | - | Fallback option |
| 3 (A1 Experiment) | 0x104000 | 0x021000 | 0x024000 | ⏳ PENDING | - | - | Low priority |

**Legend:**
- ⏳ PENDING - Not tested yet
- ✅ PASS - Test succeeded
- ❌ FAIL - Test failed
- ⚠️ PARTIAL - Partial success

---

## Known Issues to Watch For

### Issue 1: USB Timeout (Previous Problem)
**Symptom:** `ERROR -7: Operation timed out` during SPL upload  
**Previous Cause:** Wrong spl_addr (0x20000 instead of 0x104000)  
**Expected Resolution:** Candidate 1 should fix this

### Issue 2: USB Instability
**Symptom:** Device disconnects/reconnects during operation  
**Possible Causes:**
- USB cable quality
- USB hub issues (use direct port)
- Power supply problems
- BROM rejection of invalid memory access

**Mitigation:**
- Use high-quality USB cable
- Connect directly to host (no hub)
- Check dmesg for USB errors

### Issue 3: Memory Overlap
**Symptom:** SPL upload succeeds but U-Boot crashes  
**Possible Cause:** Thunk or scratch overlaps with BSS/stack  
**Solution:** Try Candidate 2 with more conservative spacing

---

## Success Criteria for Task Completion

- [ ] At least one candidate configuration successfully uploads SPL
- [ ] U-Boot executes and shows console output
- [ ] Configuration documented and committed to git
- [ ] Working binary available as `sunxi-fel-h713-working`
- [ ] Testing results table completed with actual data

---

## Current Status Summary

**Software Analysis:** ✅ COMPLETE  
- boot0 memory map extracted
- Factory firmware validated
- Candidate configurations generated
- sunxi-fel binary built

**Hardware Testing:** ⏳ BLOCKED - NO HARDWARE ACCESS  
**Next Action:** Requires physical HY300 device in FEL mode

---

## References

- **H713_BROM_MEMORY_MAP.md** - boot0 analysis source
- **H713_MEMORY_MAP_CANDIDATES.md** - Candidate rationale
- **FACTORY_FEL_ADDRESSES.md** - Cross-validation data
- **sunxi-fel-h713-fixed** - Primary test binary (77KB)

---

**Document Status:** Ready for hardware testing  
**Updated:** 2025-10-11  
**Awaiting:** Physical hardware access to complete testing phase
