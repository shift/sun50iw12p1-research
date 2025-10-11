# Task 027e: FEL Protocol Validation

**Status:** pending  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Parent Task:** 027 - H713 FEL Memory Map Reverse Engineering  
**Assigned:** Hardware Testing Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md  
**Hardware Required:** HY300 device in FEL mode via USB

## Objective

Validate working FEL configuration from Task 027d with comprehensive testing. Confirm U-Boot SPL loads and executes reliably, enabling Phase II completion.

## Prerequisites

- [ ] Task 027d completed: Working memory configuration identified
- [ ] Working sunxi-fel binary with correct H713 addresses
- [ ] U-Boot SPL verified functional
- [ ] HY300 in FEL mode (USB connected)

## Acceptance Criteria

- [ ] SPL upload succeeds consistently (5/5 attempts)
- [ ] U-Boot SPL executes (verified via USB or serial if available)
- [ ] Memory read/write operations work reliably
- [ ] FEL chain loading tested (SPL → U-Boot)
- [ ] USB connection stable throughout operations
- [ ] Complete documentation created
- [ ] Deliverable: `docs/H713_FEL_PROTOCOL_GUIDE.md` with usage examples

## Implementation Steps

### 1. Repeat SPL Upload Test (5 iterations)
```bash
for i in {1..5}; do
  echo "Test $i/5"
  ./sunxi-fel-h713-working spl u-boot-sunxi-with-spl.bin
  if [ $? -ne 0 ]; then
    echo "FAILED on iteration $i"
    break
  fi
  sleep 2
  # Power cycle device between tests
done
```

**Expected:** 100% success rate, <5 seconds per upload

### 2. Test Memory Operations
```bash
# Write test pattern
./sunxi-fel-h713-working writel 0x00020000 0xDEADBEEF

# Read back
./sunxi-fel-h713-working readl 0x00020000
# Should return: 0xDEADBEEF

# Test multiple addresses
for addr in 0x00020000 0x00020100 0x00020200; do
  ./sunxi-fel-h713-working writel $addr 0x12345678
  result=$(./sunxi-fel-h713-working readl $addr)
  echo "Address $addr: $result"
done
```

**Expected:** All reads match written values

### 3. Test Chain Loading (SPL → U-Boot)
```bash
# Upload and execute SPL
./sunxi-fel-h713-working spl u-boot-sunxi-with-spl.bin

# If serial available, check for U-Boot prompt
# Otherwise, check USB response codes
```

**Expected:** U-Boot SPL loads and executes

### 4. Stress Test USB Stability
```bash
# Rapid upload test
for i in {1..20}; do
  ./sunxi-fel-h713-working version
  sleep 0.5
done

# Monitor dmesg for resets
dmesg | grep -i "usb reset\|device reset"
```

**Expected:** No device resets, stable enumeration

### 5. Document Final Configuration
Create `docs/H713_FEL_PROTOCOL_GUIDE.md`:

**Sections:**
- Working memory map with evidence
- Usage examples (upload SPL, read/write memory)
- Known limitations and workarounds
- Troubleshooting guide
- Integration with U-Boot development workflow

### 6. Update Project Documentation
Update these files:
- `README.md` - FEL mode now working
- `docs/PROJECT_OVERVIEW.md` - Phase II status
- `docs/HY300_TESTING_METHODOLOGY.md` - FEL testing procedures
- `USING_H713_FEL_MODE.md` - Complete usage guide

## Quality Validation

- [ ] All tests pass reliably
- [ ] Documentation complete and accurate
- [ ] Working binary committed: `sunxi-fel-h713-working`
- [ ] soc_info.c changes committed to sunxi-tools
- [ ] Usage examples tested and verified
- [ ] Integration with U-Boot workflow documented

## Success Metrics

**Reliability:**
- 100% success rate over 10 consecutive tests
- No USB timeouts or device resets
- Operations complete in expected timeframes

**Documentation:**
- Reproduction steps work for fresh users
- All commands tested and verified
- Troubleshooting guide covers common issues

**Integration:**
- Ready for U-Boot development workflow
- FEL mode usable for kernel testing
- Safe testing procedures established

## Deliverables

1. **`docs/H713_FEL_PROTOCOL_GUIDE.md`** - Complete usage guide
2. **`sunxi-fel-h713-working`** - Final binary (committed)
3. **Updated `build/sunxi-tools/soc_info.c`** - H713 configuration
4. **Git commits** - All changes with [Task 027] references
5. **Updated project docs** - Phase II status

## Documentation Template

**H713_FEL_PROTOCOL_GUIDE.md structure:**
```markdown
# H713 FEL Protocol Implementation Guide

## Summary
[Working configuration discovered]

## Memory Map
| Address | Purpose | Size | Notes |
|---------|---------|------|-------|
| 0xXXXXX | SPL load | XXX KB | SRAM A1 |

## Usage Examples
### Upload SPL
### Read Memory
### Write Memory
### Chain Load U-Boot

## Integration with Development Workflow
[How to use in daily development]

## Troubleshooting
[Common issues and solutions]

## Evidence and Validation
[Test results and proof]
```

## Next Task Dependencies

**Unblocks:**
- Task 028: U-Boot FEL Deployment
- Task 029: Mainline Linux Boot via FEL
- Phase II Completion: U-Boot Porting

**Enables:**
- Safe bootloader iteration
- Kernel testing without eMMC writes
- Complete development workflow via FEL

## Integration with Project

### Phase II Completion
This task completes Phase II (U-Boot Porting):
- ✅ U-Boot compiled with H713 support
- ✅ FEL mode working with correct memory map
- ✅ SPL upload and execution verified
- ⏭️ Ready for Phase III: Mainline Linux

### Development Workflow
FEL mode enables:
- Rapid bootloader testing
- No eMMC write risk
- Kernel iteration without flashing
- Safe experimentation

## Notes

**Time:** 1-2 hours  
**Hardware Required:** HY300 device (USB FEL access)  
**Risk:** Low (validation only, configuration already working)  
**Blockers:** None (configuration proven in Task 027d)

**Success Definition:**
- Reliable FEL operations (100% success)
- Complete documentation
- Ready for Phase III Linux porting

---

## CRITICAL UPDATE: October 11, 2025

### FEL Mode Inaccessible - Task Cannot Be Executed

**Investigation Result:** This task cannot be completed due to H713 BROM firmware bug.

**Finding:** H713 BROM crashes immediately when ANY program attempts to open the USB device, making all FEL protocol validation impossible. Device crashes occur BEFORE any FEL commands can be sent.

**Impact:**
- ❌ Cannot validate FEL protocol fixes
- ❌ Cannot test SPL upload operations
- ❌ Cannot verify memory operations
- ❌ Cannot perform U-Boot chain loading via FEL

**Alternative:** All validation must occur via serial console (UART) instead of FEL mode.

**Documentation:** See `FEL_BACKUP_IMPLEMENTATION_SUMMARY.md` for complete investigation findings.

**Task Status:** BLOCKED INDEFINITELY - Recommend deprecation in favor of serial console testing approach.
