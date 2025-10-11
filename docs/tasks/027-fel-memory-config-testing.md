# Task 027d: FEL Memory Configuration Testing

**Status:** blocked  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Parent Task:** 027 - H713 FEL Memory Map Reverse Engineering  
**Assigned:** Hardware Testing Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md  
**Hardware Required:** HY300 device in FEL mode via USB
**Blocker:** No hardware access - requires physical HY300 device

## Objective

Systematically test candidate memory configurations to identify working H713 FEL addresses. Test each candidate from Tasks 027a-c until SPL upload succeeds.

## Prerequisites

- [ ] Task 027b completed: Candidate configurations generated
- [x] sunxi-fel source available at `build/sunxi-tools/`
- [x] U-Boot SPL ready: `u-boot-sunxi-with-spl.bin`
- [x] HY300 in FEL mode (USB connected)
- [x] Build environment: pkg-config, libusb1, zlib, dtc

## Acceptance Criteria

- [ ] All candidate configurations tested systematically
- [ ] Each test documented with results
- [ ] Working configuration identified (SPL upload succeeds)
- [ ] USB stability issues resolved or documented
- [ ] Test results table completed
- [ ] Deliverable: `docs/FEL_TESTING_RESULTS.md` + working `sunxi-fel` binary

## Implementation Steps

### 1. Prepare Test Environment
```bash
# Verify FEL mode
./sunxi-fel-h713-new version

# Check USB connection
lsusb | grep -i "google\|allwinner"
dmesg | tail -20
```

### 2. Create Test Script
```bash
#!/bin/bash
# test-fel-config.sh

CONFIG_NAME=$1
SPL_ADDR=$2
SCRATCH_ADDR=$3
THUNK_ADDR=$4
THUNK_SIZE=$5

echo "Testing: $CONFIG_NAME"
echo "  spl_addr=$SPL_ADDR scratch=$SCRATCH_ADDR thunk=$THUNK_ADDR size=$THUNK_SIZE"

# Modify soc_info.c
cd build/sunxi-tools
# ... edit configuration ...

# Build
make clean
make

# Test upload
./sunxi-fel spl ../../u-boot-sunxi-with-spl.bin
RESULT=$?

echo "Result: $RESULT"
```

### 3. Test Each Candidate (Priority Order)
From Task 027b candidate list, test highest confidence first:

**Test 1: Candidate A (High Confidence)**
```bash
./test-fel-config.sh "CandidateA" 0xXXXXX 0xYYYYY 0xZZZZZ 0x8000
```

Document:
- Configuration tested
- USB behavior (stable/reset)
- Error message (if any)
- Success/failure
- dmesg output

**Test 2-5: Remaining Candidates**
Repeat for each candidate configuration

### 4. Monitor USB Stability
```bash
# In separate terminal
dmesg -w | grep -i "usb\|fel"

# Check device resets
watch -n1 'lsusb | grep -c Google'
```

### 5. Validate Working Configuration
When SPL upload succeeds:
```bash
# Verify no timeout
./sunxi-fel spl u-boot-sunxi-with-spl.bin
# Should complete in <5 seconds

# Test memory operations
./sunxi-fel readl 0x00020000
./sunxi-fel writel 0x00020000 0x12345678
./sunxi-fel readl 0x00020000  # Should return 0x12345678
```

### 6. Document Final Configuration
Create `docs/FEL_TESTING_RESULTS.md` with:
- Test matrix (all candidates + results)
- Working configuration details
- Evidence (command outputs)
- USB stability notes
- Recommendations

## Quality Validation

- [ ] All candidates tested without shortcuts
- [ ] Results reproducible (test 3 times)
- [ ] Working binary saved: `sunxi-fel-h713-working`
- [ ] soc_info.c changes committed with evidence
- [ ] Memory read/write operations validated

## Safety Protocol

**FEL Mode Safety:**
- All testing via USB (non-destructive)
- No eMMC writes
- Device recoverable via power cycle
- Test incrementally (don't skip candidates)

**USB Handling:**
- Power cycle between failed tests
- Use short USB cable (reduce errors)
- Monitor dmesg for device resets
- Retry failed tests 2-3 times before moving on

## Test Results Template

| Config | spl_addr | scratch | thunk | Result | Error | USB Stable | Notes |
|--------|----------|---------|-------|--------|-------|------------|-------|
| H616   | 0x20000  | 0x108000| 0x118000| FAIL | Timeout | No | Current |
| Cand-A | 0xXXXXX  | 0xYYYYY | 0xZZZZZ | TEST | -     | -  | - |

## Next Task Dependencies

- Task 027e: Validation uses working configuration
- Phase II completion: Requires working FEL upload

## Notes

**Hardware Access Required:** Physical device connection  
**Time:** 2-3 hours (depends on number of candidates)  
**Risk:** Low (FEL mode non-destructive)  
**Blockers:** USB instability may require multiple retries

**Blocked Reason:** Hardware access required for testing phase (027d)

---

## CRITICAL UPDATE: October 11, 2025

### FEL Mode Inaccessible - BROM Firmware Bug

**Investigation Completed:** FEL mode testing is **NOT POSSIBLE** on H713 hardware due to BROM firmware bug.

**Finding:** H713 BROM crashes immediately when ANY program attempts to open the USB device. Device crashes before any FEL protocol commands can be sent, making all FEL operations (including memory configuration testing) impossible.

**Evidence:**
- Device enumerates correctly (VID/PID: 1f3a:efe8)
- Crashes on `libusb_open_device_with_vid_pid()` call
- Affects ALL tools: custom binaries, stock sunxi-fel, minimal test programs
- Error: `errno=5 EIO` when accessing device
- Enters continuous reset loop until device is power cycled

**Impact on This Task:**
- ❌ Cannot test ANY FEL memory configurations
- ❌ Cannot perform SPL uploads via FEL
- ❌ Cannot validate memory operations via FEL
- ❌ All steps in this task are blocked by BROM bug

**Alternative Path Forward:**
1. **Serial Console (UART)** - Primary method for U-Boot/Linux testing
2. **Android ADB** - Alternative for firmware access and testing
3. **eMMC/SD Boot** - Flash U-Boot via Android, boot directly

**Documentation:**
- `FEL_BACKUP_IMPLEMENTATION_SUMMARY.md` - Complete investigation findings
- `H713_FEL_PROTOCOL_ANALYSIS.md` - BROM crash analysis
- `H713_FEL_FIXES_SUMMARY.md` - Attempted fixes and conclusions

**Task Status:** This task remains **BLOCKED** indefinitely until:
1. Allwinner provides BROM firmware update, OR
2. Alternative USB access method discovered, OR  
3. Task is deprecated in favor of serial console testing approach

**Recommendation:** Deprecate this task and create new task for serial console-based hardware testing.
