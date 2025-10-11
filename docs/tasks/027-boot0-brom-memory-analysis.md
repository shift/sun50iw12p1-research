# Task 027a: boot0.bin BROM Memory Analysis

**Status:** pending  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Parent Task:** 027 - H713 FEL Memory Map Reverse Engineering  
**Assigned:** Specialized Analysis Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md

## Objective

Extract H713 BROM SRAM memory layout from boot0.bin through static binary analysis. Identify: SRAM base addresses, sizes, FEL handler locations, and memory region definitions with hex offset evidence.

## Prerequisites

- [x] boot0.bin available at `/home/shift/code/android_projector/boot0.bin`
- [x] Analysis tools: binwalk, strings, hexdump, objdump
- [x] H616 reference memory map for comparison

## Acceptance Criteria

- [ ] SRAM A1 base address identified with hex offset evidence
- [ ] SRAM region sizes documented
- [ ] ARM reset vectors identified
- [ ] Memory initialization code patterns found
- [ ] All findings include file offset + hex pattern evidence
- [ ] Results validated with multiple analysis methods
- [ ] Deliverable: `docs/H713_BROM_MEMORY_MAP.md` created

## Implementation Steps

### 1. Binary Structure Analysis
```bash
binwalk boot0.bin
hexdump -C boot0.bin | head -64
```

### 2. ARM Reset Vector Search
```bash
grep -abo $'\xEA\x00\x00\x00' boot0.bin
hexdump -C boot0.bin | grep "ea 00 00"
```

### 3. SRAM Address String Extraction
```bash
strings -t x boot0.bin | grep -iE "sram|0x0002|0x0001"
```

### 4. Memory Pattern Analysis
```bash
hexdump -C boot0.bin | grep -E "00 00 02 00|00 00 01 00"
```

### 5. Disassembly (if code found)
```bash
aarch64-unknown-linux-gnu-objdump -D -b binary -m aarch64 boot0.bin > boot0_disasm.txt
```

### 6. Create Evidence Document
Format: `docs/H713_BROM_MEMORY_MAP.md` with address table, hex dumps, confidence levels

## Quality Validation

- [ ] All addresses include source offset
- [ ] Multiple methods confirm findings
- [ ] Negative results documented
- [ ] Ready for H616 comparison (Task 027b)

## Next Task Dependencies

- Task 027b: H616 comparison requires these results
- Task 027d: Testing needs candidate addresses

## Notes

**Expected:** SRAM addresses, reset vectors, memory sizes  
**Time:** 1-2 hours  
**Environment:** Standard Linux tools
