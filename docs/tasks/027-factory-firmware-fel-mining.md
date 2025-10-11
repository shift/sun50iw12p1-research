# Task 027c: Factory Firmware FEL Address Mining

**Status:** pending  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Parent Task:** 027 - H713 FEL Memory Map Reverse Engineering  
**Assigned:** Specialized Analysis Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md

## Objective

Extract FEL-related memory addresses from factory Android firmware to validate boot0 analysis and identify documented SRAM regions.

## Prerequisites

- [x] Factory firmware at `firmware/extracted_components/`
- [x] Device tree analysis tools (dtc)
- [ ] Task 027a (boot0 analysis) provides addresses to validate

## Acceptance Criteria

- [ ] Device tree SRAM regions extracted and documented
- [ ] Kernel FEL driver references found (if any)
- [ ] Bootloader FEL addresses identified
- [ ] Factory addresses cross-referenced with boot0 findings
- [ ] Conflicts or confirmations documented
- [ ] Deliverable: `docs/FACTORY_FEL_ADDRESSES.md` created

## Implementation Steps

### 1. Search Device Trees for SRAM
```bash
cd firmware/extracted_components/
find . -name "*.dts" -o -name "*.dtsi" | xargs grep -i "sram"
find . -name "*.dts" -o -name "*.dtsi" | xargs grep -E "0x0002|0x0001"
```

### 2. Extract Memory Regions
```bash
# Look for memory node definitions
grep -r "memory@" *.dts
grep -r "reserved-memory" *.dts
```

### 3. Search Kernel for FEL Drivers
```bash
# If kernel source available
find . -name "*fel*.c" -o -name "*usb*.c" | xargs grep -l "FEL\|AWUSBFEX"

# In binaries
strings boot.img | grep -iE "fel|awusbfex|sram|0x0002"
```

### 4. Cross-Reference with boot0
Compare factory findings with Task 027a results:
- Do addresses match?
- Are sizes consistent?
- Any additional regions?

### 5. Extract Validation Evidence
For each address found:
- Source file and line number
- Context (DTS node, driver code, etc.)
- Confidence level
- Match with boot0? (yes/no/partial)

### 6. Create Findings Document
`docs/FACTORY_FEL_ADDRESSES.md` with:
- Address comparison table
- Source evidence
- Cross-validation notes
- Recommendations for testing

## Quality Validation

- [ ] All addresses have source references
- [ ] Device tree data extracted completely
- [ ] Cross-validation with boot0 complete
- [ ] Conflicts explained and documented

## Next Task Dependencies

- Task 027b: Use factory data to validate candidates
- Task 027d: Testing uses most-validated addresses first

## Notes

**Sources:** Device trees, kernel drivers, bootloader logs  
**Time:** 1-2 hours  
**Focus:** Cross-validation, not new discovery
