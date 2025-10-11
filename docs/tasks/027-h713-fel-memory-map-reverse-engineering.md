# Task 027: H713 FEL Memory Map Reverse Engineering

**Status:** blocked  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Assigned:** AI Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md, docs/FEL_MODE_ANALYSIS.md

## Objective

Reverse engineer the correct H713 FEL protocol memory map to enable successful SPL upload and U-Boot deployment via FEL mode. Current sunxi-fel binary recognizes H713 but fails on bulk transfers with timeout errors.

## Current Status

**What Works:**
- ✅ H713 SoC detection (ID 0x1860) via USB
- ✅ FEL version query succeeds
- ✅ Device enumeration and USB communication established

**What Fails:**
- ❌ SPL upload gets `ERROR -7: Operation timed out`
- ❌ Bulk transfer operations fail
- ❌ Memory read/write operations timeout
- ❌ USB connection unstable (requires retries, device resets)

**Root Cause:**
H713 entry in sunxi-tools uses H616 memory map which is incompatible. Need correct:
- `spl_addr` - SRAM A1 address for SPL loading
- `scratch_addr` - Scratchpad area for FEL protocol
- `thunk_addr` - Code execution address
- `thunk_size` - Available SRAM size

## Prerequisites

- [x] H713 FEL mode accessible via USB
- [x] sunxi-fel binary with H713 SoC ID support
- [x] boot0.bin extracted and available for analysis
- [x] U-Boot SPL binary ready for testing
- [ ] Complete Boot ROM (BROM) memory map documented
- [ ] FEL protocol address requirements identified

## Acceptance Criteria

- [ ] H713 BROM SRAM memory map documented with addresses
- [ ] Correct `spl_addr`, `scratch_addr`, `thunk_addr` values identified
- [ ] sunxi-fel successfully uploads SPL to H713 without timeouts
- [ ] FEL bulk transfer operations complete successfully
- [ ] U-Boot SPL loads and executes via FEL mode
- [ ] Documentation includes evidence and validation methodology
- [ ] No device bricking or irreversible changes

## Implementation Steps

### 1. Analyze boot0.bin for BROM Memory Layout
**Atomic Task:** Extract SRAM addresses and FEL handler references from boot0.bin
- Use binwalk, strings, hexdump to identify BROM addresses
- Search for patterns matching ARM reset vectors
- Identify SRAM A1/A2 base addresses
- Document FEL handler entry points
- Cross-reference with Allwinner BROM documentation patterns

### 2. Compare H616 vs H713 Memory Maps
**Atomic Task:** Document differences between H616 (reference) and H713 actual memory layout
- Extract H616 memory map from sunxi-tools soc_info.c
- Identify H616 assumptions about SRAM layout
- Map H713 SRAM differences from boot0 analysis
- Document incompatibilities causing bulk transfer failures
- Propose H713-specific address corrections

### 3. Extract FEL Protocol Addresses from Factory Firmware
**Atomic Task:** Mine factory kernel/bootloader for FEL-related memory addresses
- Search factory kernel for FEL driver references
- Extract bootloader FEL mode handlers
- Identify documented SRAM regions in device trees
- Cross-reference with boot0 findings
- Validate against USB protocol captures if available

### 4. Test Alternative Memory Configurations
**Atomic Task:** Systematically test candidate memory addresses via FEL
- Create modified soc_info.c variants with candidate addresses
- Build test sunxi-fel binaries for each configuration
- Test SPL upload with each configuration (safe via FEL)
- Document results for each address combination
- Identify working configuration or narrow search space

### 5. Validate Working Configuration
**Atomic Task:** Confirm successful FEL operations with correct memory map
- Upload SPL successfully without timeouts
- Verify SPL execution via USB response
- Test memory read/write operations
- Validate U-Boot chain loading
- Document final working memory map with evidence

## Quality Validation

- [ ] All memory addresses documented with source evidence
- [ ] sunxi-fel compiles with H713 configuration
- [ ] SPL upload completes without USB errors
- [ ] U-Boot boots successfully via FEL mode
- [ ] Memory operations (read/write) work reliably
- [ ] Documentation includes reproduction steps
- [ ] Changes committed with task reference: [Task 027]

## Atomic Task Breakdown

This task requires **5 atomic delegations** to specialized agents:

1. **Task 027a: boot0-brom-memory-analysis** - BROM/SRAM address extraction from boot0.bin
   - Status: pending
   - Deliverable: `docs/H713_BROM_MEMORY_MAP.md`
   
2. **Task 027b: h616-h713-memory-comparison** - Memory map difference analysis
   - Status: pending (blocked by 027a)
   - Deliverable: `docs/H713_MEMORY_MAP_CANDIDATES.md`
   
3. **Task 027c: factory-firmware-fel-mining** - FEL address extraction from factory firmware
   - Status: pending (parallel with 027a)
   - Deliverable: `docs/FACTORY_FEL_ADDRESSES.md`
   
4. **Task 027d: fel-memory-config-testing** - Systematic testing of candidate addresses
   - Status: pending (blocked by 027a+027b+027c)
   - Deliverable: `docs/FEL_TESTING_RESULTS.md` + working sunxi-fel binary
   - **Hardware Required:** HY300 device in FEL mode
   
5. **Task 027e: fel-protocol-validation** - Final configuration validation and documentation
   - Status: pending (blocked by 027d)
   - Deliverable: `docs/H713_FEL_PROTOCOL_GUIDE.md`
   - **Hardware Required:** HY300 device in FEL mode

**Task Dependencies:**
```
027a (boot0 analysis) ─┬─→ 027b (comparison) ─┐
                       │                        ├─→ 027d (testing) ─→ 027e (validation)
027c (factory mining) ─┘                        │
                                                └─→ (independent validation)
```

**Execution Strategy:**
1. Start 027a and 027c in parallel (both are analysis, no hardware)
2. Complete 027b after 027a finishes (needs boot0 results)
3. Execute 027d when all analysis complete (requires hardware)
4. Finish with 027e for final validation and documentation

Each delegation is self-contained with complete context provided in delegation prompt.

## Success Indicators

**Immediate:**
- No USB timeout errors during FEL operations
- SPL upload completes in <5 seconds
- Device remains stable (no resets during upload)

**Final Validation:**
- U-Boot prompt accessible via USB serial
- FEL mode recovery works consistently
- All memory operations complete successfully

## Hardware Safety Protocol

**Critical Safety Measures:**
- All testing via FEL mode (USB recovery, non-destructive)
- No eMMC writes during research phase
- Device can always return to FEL mode via power cycle
- No permanent changes until validation complete
- Maintain boot0.bin backup for recovery reference

## Next Task Dependencies

**Blocks:**
- Task 028: U-Boot FEL Deployment and Testing
- Task 029: Mainline Linux Kernel Boot via FEL
- Phase II Completion: U-Boot Porting

**Enables:**
- Hardware testing without eMMC modification
- Safe bootloader iteration and development
- Mainline kernel validation workflow

## External Resources

**Allwinner BROM Documentation:**
- linux-sunxi.org FEL protocol documentation
- H616 reference implementation in sunxi-tools
- Allwinner Boot ROM reverse engineering notes

**Tools Required:**
- sunxi-tools (modified with H713 support)
- binwalk, hexdump, strings for boot0 analysis
- USB debugging tools (lsusb, dmesg)
- Cross-compilation toolchain for sunxi-fel builds

## Notes

**Key Insights from Current Testing:**
- H713 SoC ID 0x1860 detected correctly
- USB enumeration works (AWUSBFEX device)
- Bulk transfers fail at operation level (not USB level)
- Device unstable during failed operations (requires retries)
- Using H616 memory map as starting reference

**Critical Unknowns:**
- Exact SRAM A1 size and layout in H713
- FEL protocol scratchpad location
- Differences from H616 in BROM implementation
- USB bulk transfer buffer requirements

**Research Strategy:**
- Start with boot0.bin static analysis (safest, most informative)
- Cross-reference factory firmware for validation
- Test systematically with evidence-based candidates
- Document all findings regardless of success/failure

**Risk Mitigation:**
- FEL mode is inherently safe (ROM-based recovery)
- No risk of bricking during memory map research
- Can iterate rapidly without eMMC writes
- USB connection issues are recoverable via power cycle

## Progress Log

**2025-10-11:** Task created after successful H713 detection but failed SPL upload
- Built sunxi-fel with H713 support (SoC ID 0x1860)
- Confirmed USB enumeration and version query work
- Identified bulk transfer timeouts as core issue
- Determined H616 memory map incompatibility as root cause
