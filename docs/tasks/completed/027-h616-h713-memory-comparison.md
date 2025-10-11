# Task 027b: H616 vs H713 Memory Map Comparison

**Status:** completed  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Parent Task:** 027 - H713 FEL Memory Map Reverse Engineering  
**Assigned:** Specialized Analysis Agent  
**Created:** 2025-10-11  
**Context:** ai/contexts/h713-fel-reverse-engineering.md

## Objective

Compare H616 reference memory map with H713 boot0 analysis results to identify incompatibilities and generate candidate FEL memory addresses for testing.

## Prerequisites

- [x] H616 memory map from `build/sunxi-tools/soc_info.c`
- [ ] Task 027a completed: H713 BROM analysis results
- [ ] Understanding of FEL protocol address requirements

## Acceptance Criteria

- [ ] H616 memory map fully documented
- [ ] H713 vs H616 differences identified
- [ ] Incompatibilities causing FEL failures explained
- [ ] 3-5 candidate address configurations generated
- [ ] Each candidate includes rationale and evidence
- [ ] Deliverable: `docs/H713_MEMORY_MAP_CANDIDATES.md` created

## Implementation Steps

### 1. Extract H616 Reference Configuration
From `build/sunxi-tools/soc_info.c`:
```c
.spl_addr = 0x20000,
.scratch_addr = 0x108000,
.thunk_addr = 0x118000,
.thunk_size = 0x8000,
```

### 2. Compare SRAM Base Addresses
- H616 SRAM A1: documented value
- H713 SRAM A1: from Task 027a analysis
- Calculate offset differences

### 3. Analyze Failure Pattern
Current failure: bulk transfer timeout at SPL upload
- Which address is accessed first? (spl_addr)
- What operation fails? (memory write to SRAM)
- Why timeout? (invalid address or size)

### 4. Generate Candidate Configurations
Based on analysis, create 3-5 test configurations:
```
Candidate 1: Offset adjustment
Candidate 2: Alternative SRAM region
Candidate 3: Size-based correction
```

### 5. Prioritize Candidates
Rank by likelihood:
1. High confidence (evidence-based)
2. Medium confidence (pattern-based)
3. Low confidence (exploratory)

### 6. Create Testing Roadmap
Document testing order and expected outcomes

## Quality Validation

- [ ] All candidates have evidence/rationale
- [ ] Comparison table shows H616 vs H713
- [ ] Failure analysis explains current timeout
- [ ] Ready for testing (Task 027d)

## Next Task Dependencies

- Task 027d: Uses candidate addresses for testing
- Task 027e: Validates working configuration

## Notes

**Current failure:** SPL upload timeout with H616 addresses on H713  
**Time:** 1 hour  
**Inputs:** H616 config + Task 027a results
