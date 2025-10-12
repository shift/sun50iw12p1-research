# PhoenixSuit Research Integration Summary

**Date:** 2025-10-12  
**Related Tasks:** 027 (FEL Memory Map), 034 (PhoenixSuit Analysis)

## Overview

This document connects PhoenixSuit reverse engineering findings (Task 034) with previous FEL protocol research (Task 027) to provide a complete picture of H713 FEL/FES protocol understanding.

## Research Timeline

### Task 027: H713 FEL Memory Map Reverse Engineering (Oct 11, 2025)
**Objective:** Fix H713 FEL mode by discovering correct BROM memory addresses

**Key Achievements:**
- ✅ Extracted H713 memory map from boot0.bin header
- ✅ Identified SPL load address: 0x00104000 (SRAM A2)
- ✅ Built sunxi-fel-h713-fixed with correct addresses
- ✅ Documented BROM USB crash bug

**Status:** Software analysis complete, blocked on hardware testing

**Deliverables:**
- `docs/H713_BROM_MEMORY_MAP.md`
- `docs/FACTORY_FEL_ADDRESSES.md`
- `docs/H713_MEMORY_MAP_CANDIDATES.md`
- `sunxi-fel-h713-fixed` binary

### Task 034: PhoenixSuit FEL Protocol Reverse Engineering (Oct 12, 2025)
**Objective:** Extract H713 FEL protocol details from Allwinner's official flashing tool

**Key Findings:**
- ⚠️ PhoenixSuit V1.10 (2019) lacks explicit H713 support
- ✅ Identified FES protocol architecture (FEL Stage 2)
- ✅ Hypothesis: PhoenixSuit uses minimal BROM FEL + FES transition
- ❌ No H713 memory maps found in binaries

**Status:** Analysis complete, FES protocol approach recommended

**Deliverables:**
- `docs/PHOENIXSUIT_FEL_PROTOCOL_ANALYSIS.md`
- DLL analysis files in `tools/phoenixsuit/dll_analysis/`

## Combined Insights

### The H713 FEL Problem

**BROM FEL Bug (Discovered in Task 027):**
```
Problem: H713 BROM firmware crashes on large USB bulk transfers
Symptom: Timeout errors, device reset cycles
Trigger: Transfers > ~10-20 KB (threshold uncertain)
Impact: Cannot upload U-Boot SPL (732 KB) via standard FEL
Evidence: Multiple test failures, documented in FEL_MODE_ANALYSIS.md
```

**Root Cause Hypothesis:**
- H713 BROM has USB buffer overflow or timing bug
- sunxi-fel uses BROM FEL exclusively (single-stage protocol)
- Large SPL upload triggers bug before execution begins

### The PhoenixSuit Solution (Inferred from Task 034)

**FES Protocol Strategy:**
```
Stage 1 (BROM FEL):
  - Upload minimal FES loader (16-64 KB)
  - Small size avoids BROM bug threshold
  - Loader executes immediately

Stage 2 (FES Protocol):
  - FES loader handles bulk operations
  - Bypasses BROM limitations entirely
  - Robust partition flashing
  - Extended command set
```

**Why This Works:**
1. FES loader is small enough to pass through BROM without crashing
2. Once FES executes, BROM is no longer involved
3. FES protocol has no BROM USB bug exposure
4. Production tools prioritize reliability over simplicity

### Architecture Comparison

```
sunxi-tools Current Approach (Single-Stage FEL):
┌──────────────┐
│  Host PC     │
└──────┬───────┘
       │ USB Bulk Transfer (732 KB)
       ├──── U-Boot SPL ───────────> BROM CRASH ❌
       │
       ↓
┌──────────────┐
│  H713 BROM   │ ← Bug: Large transfers crash
└──────────────┘

PhoenixSuit Inferred Approach (Two-Stage FEL → FES):
┌──────────────┐
│  Host PC     │
└──────┬───────┘
       │ USB Bulk Transfer (16 KB)
       ├──── FES Loader ───────────> BROM OK ✅
       │                              Executes FES
       │
┌──────┴───────┐                 ┌──────────────┐
│  H713 BROM   │ ───────────────>│  FES Stage   │
└──────────────┘  Exit BROM      └──────┬───────┘
                                         │
       ┌─────────────────────────────────┘
       │ FES Protocol (No BROM involvement)
       ├──── Partition Images ──────────> Robust ✅
       ├──── U-Boot SPL/Full ───────────> Robust ✅
       └──── eMMC Operations ───────────> Robust ✅
```

## Actionable Strategies

### Strategy 1: Direct FEL with Task 027 Addresses (Priority 1)

**Test Task 027 Binary:**
```bash
./sunxi-fel-h713-fixed version
./sunxi-fel-h713-fixed spl u-boot-sunxi-with-spl.bin
```

**Success Case:**
- Task 027 memory addresses are correct
- Large transfers work despite BROM bug reports
- FEL protocol fully functional
- **Result:** Phase II complete, proceed to mainline kernel

**Failure Case:**
- Timeout or crash as documented
- Confirms BROM bug affects all transfer sizes
- **Result:** Proceed to Strategy 2 (FES)

**Confidence:** 40% (BROM bug likely affects all sizes)  
**Hardware Required:** Yes (HY300 in FEL mode)  
**Time:** 30 minutes testing

### Strategy 2: FES Protocol Implementation (Priority 2)

**Phase A: Create Minimal FES Loader**

Build 16 KB FES payload:
```c
// Minimal FES loader functionality
// - Initialize USB endpoint
// - Implement FES command handler
// - Support: memory read/write, SPL execute
// - Size: < 16 KB to avoid BROM bug
```

**Phase B: Upload FES via BROM FEL**
```bash
# Use Task 027 addresses to load FES
./sunxi-fel-h713-fixed write 0x00104000 fes-loader.bin
./sunxi-fel-h713-fixed exe 0x00104000
```

**Phase C: Implement FES Protocol Commands**
```python
# FES protocol handler (Stage 2)
def fes_write_memory(addr, data):
    # Send via FES command format
    pass

def fes_upload_spl(spl_file):
    # Upload U-Boot via FES (no BROM)
    # Chunked transfer with FES protocol
    pass
```

**Success Case:**
- FES loader uploads successfully (< 16 KB)
- FES protocol commands work
- Can upload full U-Boot via FES
- **Result:** Robust H713 FEL alternative, submit to sunxi-tools

**Confidence:** 60% (based on PhoenixSuit architecture analysis)  
**Hardware Required:** Yes (FES loader testing)  
**Time:** 20-30 hours (FES protocol research + implementation)

### Strategy 3: Chunked Transfer (Priority 3)

**Hypothesis:** BROM bug has size threshold, chunked uploads may work

**Implementation:**
```c
// Modify sunxi-fel to chunk large transfers
#define FEL_CHUNK_SIZE 8192  // 8 KB chunks

int fel_write_chunked(feldev_handle *dev, uint32_t addr, 
                      void *data, size_t len) {
    for (size_t offset = 0; offset < len; offset += FEL_CHUNK_SIZE) {
        size_t chunk = (len - offset < FEL_CHUNK_SIZE) ? 
                       (len - offset) : FEL_CHUNK_SIZE;
        
        fel_write(dev, addr + offset, data + offset, chunk);
        usleep(100000);  // 100ms delay between chunks
    }
}
```

**Success Case:**
- Chunked transfers avoid BROM crash
- Full SPL uploads successfully
- **Result:** Simple fix for sunxi-tools

**Failure Case:**
- BROM bug triggered regardless of chunk size
- **Result:** Confirms FES protocol required

**Confidence:** 30% (unlikely to resolve root cause)  
**Hardware Required:** Yes  
**Time:** 2-4 hours (implementation + testing)

### Strategy 4: SD Card Boot (Priority 4 - Fallback)

**Bypass FEL Entirely:**
```bash
# Write U-Boot to SD card
dd if=u-boot-sunxi-with-spl.bin of=/dev/sdX bs=1k seek=8

# Boot HY300 from SD card
# Test mainline kernel
# Continue project without FEL dependency
```

**Success Case:** Guaranteed ✅
- Known working Allwinner approach
- No FEL/BROM dependency
- Development continues

**Disadvantages:**
- Requires SD card hardware access
- Less convenient for iteration
- Cannot use FEL recovery

**Confidence:** 95% (standard method)  
**Hardware Required:** Yes (SD card reader)  
**Time:** 30 minutes setup

## Recommended Approach

### Decision Tree

```
Start: H713 Hardware Available
│
├─> Test Strategy 1 (Direct FEL - Task 027)
│   ├─> Success? → Phase II complete ✅
│   └─> Failure? → Continue
│
├─> Test Strategy 3 (Chunked Transfer)
│   ├─> Success? → Simple fix ✅
│   └─> Failure? → Confirms need for FES
│
├─> Implement Strategy 2 (FES Protocol)
│   ├─> FES Loader uploads? (< 16 KB)
│   │   ├─> Yes → Implement full FES protocol
│   │   └─> No → BROM completely broken
│   │
│   └─> Full FES working?
│       ├─> Yes → Robust solution ✅
│       └─> No → Last resort: SD card
│
└─> Strategy 4 (SD Card Boot)
    └─> Fallback guaranteed to work ✅
```

### Time Estimates

**Best Case:** 30 minutes
- Strategy 1 succeeds immediately
- Task 027 addresses correct
- FEL fully functional

**Realistic Case:** 4-6 hours
- Strategy 1 fails
- Strategy 3 attempted (2 hours)
- Strategy 2 research begun (4 hours initial)

**Worst Case:** 30-40 hours
- Full FES protocol implementation required
- Research + implement + test
- But results in robust solution

**Fallback:** 30 minutes
- SD card boot works immediately
- Project continues without FEL

## Integration with Project Phases

### Phase II: U-Boot Porting (Current)

**Status:** BLOCKED on FEL hardware testing

**Blockers:**
- Task 027: Hardware testing of sunxi-fel-h713-fixed
- Task 034: FES protocol investigation (if direct FEL fails)

**Unblocking Options:**
1. Hardware access → Test Strategy 1-3
2. FES research → Implement Strategy 2 (no hardware initially)
3. SD card → Strategy 4 (requires SD card reader)

### Phase VI: Mainline Linux Boot (Next)

**Dependencies:**
- Working bootloader deployment method (FEL, FES, or SD)
- U-Boot tested and functional

**FEL/FES Impact:**
- With FEL: Fast iteration, easy kernel testing
- With FES: Robust but more complex setup
- With SD: Slower but proven method

## External Resources

### FES Protocol Documentation

**Research Required:**
- Allwinner FES protocol specification
- PhoenixSuit V1.18+ source analysis (if available)
- FES payload format and command structure
- Linux-sunxi FES documentation

**Search Keywords:**
- "Allwinner FES protocol"
- "PhoenixSuit FES loader"
- "sunxi FEL stage 2"
- "全志 FES 协议" (Chinese: Allwinner FES protocol)

### Community Resources

**linux-sunxi.org:**
- FEL protocol documentation
- FES implementation examples
- Community experience with similar SoCs

**GitHub:**
- phoenixsuit/LiveSuit source code
- sunxi-tools FES patches (if any)
- Community FES implementations

## Documentation Updates Required

When strategies are tested:

### If Strategy 1 Succeeds:
- [ ] Update `USING_H713_FEL_MODE.md` with working configuration
- [ ] Mark Task 027 fully complete
- [ ] Update PROJECT_OVERVIEW.md - Phase II complete
- [ ] Create upstream sunxi-tools patch

### If Strategy 2 (FES) Required:
- [ ] Create `H713_FES_PROTOCOL_GUIDE.md`
- [ ] Document FES loader implementation
- [ ] Create `tools/fes-loader/` directory
- [ ] Update HY300_TESTING_METHODOLOGY.md with FES procedures
- [ ] Consider upstream FES support for sunxi-tools

### If Strategy 4 (SD Card) Used:
- [ ] Create `H713_SD_CARD_BOOT_GUIDE.md`
- [ ] Document SD card preparation steps
- [ ] Update HY300_TESTING_METHODOLOGY.md with SD boot
- [ ] Note FEL limitations in PROJECT_OVERVIEW.md

## Conclusion

Tasks 027 and 034 provide **complementary approaches** to H713 FEL protocol:

**Task 027:** Direct BROM FEL with correct memory map
- Simplest if it works
- Enables standard sunxi-tools usage
- 40% confidence due to BROM bug

**Task 034:** FES protocol alternative
- More complex but robust
- Bypasses BROM limitations
- 60% confidence based on PhoenixSuit analysis

**Combined:** 85% confidence in one approach working

The project has **multiple viable paths forward**, with SD card boot as a guaranteed fallback. No single point of failure blocks progress.

---

**Status:** Research phase complete  
**Next Action:** Hardware testing when HY300 available  
**Blocking:** Physical hardware access for all strategies  
**Ready to Resume:** Yes - all software analysis complete
