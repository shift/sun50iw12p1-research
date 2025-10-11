# Task 028: Update MIPS Driver with Android Memory Layout

**Status:** completed  
**Priority:** high  
**Phase:** VIII - VM Integration & Android Analysis Integration  
**Assigned:** AI Agent  
**Created:** 2025-10-11  
**Context:** `ai/contexts/android-calibration-integration.md`

## Objective

Update `drivers/misc/sunxi-mipsloader.c` documentation with exact MIPS memory layout extracted from Android libmips.so analysis, including 40MB total allocation at 0x4b100000 with 4 distinct memory regions.

## Prerequisites

- [ ] Task 032 completed (Android firmware analysis with libmips.so extraction)
- [ ] Task 028-4 completed (context document with memory layout)
- [ ] MIPS memory layout documented: 40MB @ 0x4b100000 (Boot 4KB, Firmware 12MB, TSE 1MB, FB 26MB)

## Acceptance Criteria

- [ ] Driver file header comments updated with Android-validated memory layout
- [ ] Memory region constants defined with exact sizes and offsets
- [ ] Physical base address documented (0x4b100000)
- [ ] All 4 regions documented: Boot (4KB), Firmware (12MB), TSE (1MB), Framebuffer (26MB)
- [ ] Existing memory allocation code verified against Android values
- [ ] Driver compiles without errors after updates
- [ ] Git commit with proper task reference

## Implementation Steps

### 1. Read Current MIPS Driver
Read `drivers/misc/sunxi-mipsloader.c` to understand:
- Current memory allocation strategy
- Existing documentation and comments
- Reserved memory handling
- Memory region definitions

### 2. Create Documentation Patch
**CRITICAL:** Use patch-based editing (never Edit tool on .c files):
- Create patch file with updated header comments
- Add memory layout diagram in comments
- Document physical address: 0x4b100000
- Document total size: 40MB (0x2800000)

### 3. Add Memory Region Constants
Add defines/documentation for regions:
```c
/*
 * MIPS Co-Processor Memory Layout (from Android libmips.so analysis)
 * 
 * Physical Base: 0x4b100000
 * Total Size: 40MB (0x2800000)
 * 
 * Region Layout:
 *   Boot Region:       4KB @ offset 0x00000    - MIPS bootloader
 *   Firmware Region:  12MB @ offset 0x01000    - Main firmware (display.bin)
 *   TSE Region:        1MB @ offset 0xC01000   - Transport Stream Engine
 *   Framebuffer:      26MB @ offset 0xD01000   - Display framebuffer
 * 
 * Source: Android firmware analysis (libmips.so v1.0, Task 032)
 */
```

### 4. Verify Memory Allocation
Check if existing code allocates correct size:
- Look for reserved-memory DT binding
- Verify 40MB total allocation
- Ensure base address matches 0x4b100000

### 5. Apply Patch and Test
- Apply patch with: `patch -p0 < mips-memory-layout.patch`
- Verify patch applies cleanly
- Test compilation: `nix build .#hy300-drivers` (if available) or manual gcc test
- Fix any compilation errors

### 6. Cross-Reference Updates
Update related files:
- `sun50i-h713-hy300.dts` - verify reserved-memory node matches
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` - update MIPS section
- `docs/ANDROID_FIRMWARE_ANALYSIS_COMPLETE_SUMMARY.md` - add driver update cross-reference

## Quality Validation

- [ ] Memory layout diagram in driver comments matches Android analysis
- [ ] All 4 regions documented with correct sizes and offsets
- [ ] Physical address 0x4b100000 documented
- [ ] Driver compiles without errors or warnings
- [ ] Git commit message references Task 028
- [ ] Cross-reference documentation updated

## Next Task Dependencies

- Task 018: Firmware Loading System (benefits from exact memory layout)
- Task 028-3: Update Device Tree Panel Parameters (verify reserved-memory consistency)
- Phase IX: Hardware testing (validates memory layout on real hardware)

## Notes

- **SESSION-CRITICAL:** All .c file modifications MUST use patch-based editing
- This is documentation-only update, no functional code changes required
- Memory layout from libmips.so matches factory Android system exactly
- TSE (Transport Stream Engine) region purpose not fully understood yet
- Framebuffer at 26MB suggests significant display buffering capability
- Boot region (4KB) may contain MIPS initialization code separate from main firmware
- Total 40MB allocation is substantial - validates importance of MIPS co-processor role
