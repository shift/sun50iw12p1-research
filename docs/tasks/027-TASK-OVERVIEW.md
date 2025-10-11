# Task 027: H713 FEL Reverse Engineering - Overview

## Quick Status

**Main Task:** 027-h713-fel-memory-map-reverse-engineering  
**Status:** pending  
**Priority:** high  
**Phase:** II - U-Boot Porting

## Problem Statement

H713 SoC detected via FEL mode but SPL upload fails with timeout. Current sunxi-fel uses H616 memory map which is incompatible with H713 BROM SRAM layout.

## Solution Approach

Reverse engineer correct H713 FEL memory addresses through 5 atomic research tasks:

### Task 027a: boot0 BROM Analysis (1-2 hours, no hardware)
Extract SRAM addresses from boot0.bin via static analysis.
- **Input:** boot0.bin
- **Output:** `H713_BROM_MEMORY_MAP.md`
- **Tools:** binwalk, strings, hexdump, objdump

### Task 027b: H616 Comparison (1 hour, no hardware)
Compare H616 vs H713 memory maps, generate test candidates.
- **Input:** 027a results + H616 config
- **Output:** `H713_MEMORY_MAP_CANDIDATES.md`
- **Dependency:** Requires 027a

### Task 027c: Factory Firmware Mining (1-2 hours, no hardware)
Extract FEL addresses from Android firmware for validation.
- **Input:** `firmware/extracted_components/`
- **Output:** `FACTORY_FEL_ADDRESSES.md`
- **Parallel:** Can run alongside 027a

### Task 027d: Memory Config Testing (2-3 hours, **HARDWARE REQUIRED**)
Systematically test candidate configurations on real device.
- **Input:** Candidates from 027a+027b+027c
- **Output:** `FEL_TESTING_RESULTS.md` + working sunxi-fel binary
- **Dependency:** Requires 027a, 027b, 027c complete
- **Hardware:** HY300 in FEL mode via USB

### Task 027e: FEL Validation (1 hour, **HARDWARE REQUIRED**)
Validate working config, create usage guide.
- **Input:** Working config from 027d
- **Output:** `H713_FEL_PROTOCOL_GUIDE.md`
- **Dependency:** Requires 027d success
- **Hardware:** HY300 in FEL mode via USB

## Execution Strategy

**Phase 1: Analysis (No Hardware - Can Start Now)**
```
027a (boot0) ──────┐
                   ├─→ 027b (comparison)
027c (factory) ────┘
```

**Phase 2: Testing (Hardware Required - After Analysis)**
```
027d (testing) ─→ 027e (validation)
```

## Task Files

- `027-h713-fel-memory-map-reverse-engineering.md` - Main task (this overview's parent)
- `027-boot0-brom-memory-analysis.md` - Subtask 027a
- `027-h616-h713-memory-comparison.md` - Subtask 027b
- `027-factory-firmware-fel-mining.md` - Subtask 027c
- `027-fel-memory-config-testing.md` - Subtask 027d
- `027-fel-protocol-validation.md` - Subtask 027e

## Context Files

- `ai/contexts/h713-fel-reverse-engineering.md` - Complete delegation context (16KB)

## Current FEL Status

**Working:**
- H713 SoC detection (ID 0x1860)
- FEL version query
- USB enumeration

**Failing:**
- SPL upload (timeout)
- Bulk transfers
- Memory operations

**Root Cause:**
Incorrect memory map (using H616 values):
```c
.spl_addr = 0x20000,      // Wrong for H713
.scratch_addr = 0x108000, // Wrong for H713
.thunk_addr = 0x118000,   // Wrong for H713
```

## Expected Outcome

- Working sunxi-fel binary for H713
- Complete FEL protocol documentation
- Reliable U-Boot SPL upload via USB
- Phase II completion unblocked

## Time Estimate

- **Analysis Phase (027a+027b+027c):** 3-5 hours, no hardware
- **Testing Phase (027d+027e):** 3-4 hours, hardware required
- **Total:** 1-2 days with hardware access

## Next Steps

1. Start Task 027a (boot0 analysis) - delegate to specialized agent
2. Start Task 027c (factory mining) - delegate to specialized agent (parallel)
3. Wait for 027a completion
4. Start Task 027b (comparison) - delegate to specialized agent
5. Synthesize results from 027a+027b+027c
6. **Request hardware access** for 027d testing phase
7. Execute 027d (testing) with device
8. Execute 027e (validation) 
9. Update project documentation
10. Complete Phase II

## Risk Assessment

- **Low Risk:** All testing via FEL mode (non-destructive, RAM-only)
- **No Bricking:** FEL mode is ROM-based recovery
- **Recoverable:** Power cycle returns to FEL mode
- **USB Instability:** Known issue, mitigated by retries and power cycles

## Blockers

- None for analysis phase (027a+027b+027c)
- Hardware access required for testing phase (027d+027e)

## Success Criteria

- SPL uploads successfully without timeout
- U-Boot executes via FEL mode
- Memory operations work reliably
- Complete documentation created
- Phase II (U-Boot Porting) complete
