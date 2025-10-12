# Task 034: PhoenixSuit FEL Protocol Reverse Engineering

**Status:** completed  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Assigned:** AI Agent  
**Created:** 2025-10-12  
**Context:** H713 FEL mode incompatibility, PhoenixSuit analysis

## Objective

Reverse engineer PhoenixSuit to extract H713-specific FEL protocol implementation details that could resolve the H713 BROM USB transfer incompatibility discovered in Task 027.

## Background

### The H713 FEL Problem
Task 027 documented a fundamental H713 BROM incompatibility:
- H713 FEL mode is accessible (USB device detected)
- SoC ID correctly identified as 0x1860
- **All bulk transfers fail** with timeout or device crash
- Root cause: H713 BROM firmware bug crashes on USB device open

### Why PhoenixSuit Matters
PhoenixSuit is Allwinner's **official** H713 flashing tool, meaning:
1. It must handle H713 BROM quirks successfully
2. May contain workarounds for known H713 bugs
3. Could reveal proper H713 FEL protocol implementation
4. Likely has H713-specific memory maps and timing

### Current Status
- ✅ PhoenixSuit V1.10 downloaded and analyzed (tools/phoenixsuit/)
- ✅ Image format reverse engineered (PHOENIXSUIT_H713_FORMAT_ANALYSIS.md)
- ❌ **FEL protocol details not yet extracted** ← This task

## Prerequisites

- [x] PhoenixSuit binaries available (tools/phoenixsuit/colorfulshark-phoenixsuit/)
- [x] Existing FEL analysis documents (FEL_MODE_ANALYSIS.md, TASK_027_RESEARCH_SUMMARY.md)
- [x] Reverse engineering tools (objdump, strings, binwalk)
- [ ] Disassembler (Ghidra or IDA) for deep DLL analysis

## Acceptance Criteria

- [ ] Identify H713-specific FEL protocol differences from standard Allwinner protocol
- [ ] Extract memory map addresses (SPL load, scratch, thunk) if embedded
- [ ] Document USB transfer patterns and timing requirements
- [ ] Find any H713 BROM workarounds or special handling
- [ ] Create actionable recommendations for sunxi-tools modifications
- [ ] Deliverable: `docs/PHOENIXSUIT_FEL_PROTOCOL_ANALYSIS.md` with findings

## Implementation Steps

### 1. Analyze Key DLLs with Binary Tools

**Target Files:**
- `eFex.dll` (44 KB) - FEL/FEX protocol handler
- `Phoenix_Fes.dll` (36 KB) - FES protocol implementation
- `luaeFex.dll` (40 KB) - Lua bindings (may expose protocol functions)

**Methods:**
```bash
# Extract all strings with context
strings -n 8 eFex.dll > efex_strings.txt
strings -n 8 Phoenix_Fes.dll > phoenix_fes_strings.txt

# Analyze DLL structure
objdump -x eFex.dll > efex_structure.txt
objdump -x Phoenix_Fes.dll > phoenix_fes_structure.txt

# Look for imported functions (USB operations)
objdump -p eFex.dll | grep -A 50 "DLL Name"
objdump -p Phoenix_Fes.dll | grep -A 50 "DLL Name"

# Search for hex constants (memory addresses)
hexdump -C eFex.dll | grep -E "(00 01 04|00 12 10|00 12 3a)" 
# Looking for: 0x00104000, 0x00121000, 0x00123a00 from Task 027
```

### 2. Examine Lua Script Interfaces

Lua scripts may expose FEL protocol functions in readable form:

```bash
# Check luaeFex.dll exports
objdump -p luaeFex.dll | grep -A 100 "Export Table"

# Analyze any Lua scripts that interact with FEL
grep -r "eFex\|FEL\|FES\|upload\|download" tools/phoenixsuit/colorfulshark-phoenixsuit/*.lua
```

### 3. Search for H713 SoC ID (0x1860)

```bash
# Hex pattern for 0x1860 (little-endian: 60 18 00 00)
hexdump -C eFex.dll | grep "60 18"
hexdump -C Phoenix_Fes.dll | grep "60 18"
hexdump -C PhoenixSuit.exe | grep "60 18"

# Also check for decimal 6240 (0x1860)
strings PhoenixSuit.exe | grep -E "1860|6240"
```

### 4. Compare with sunxi-tools Source

Cross-reference PhoenixSuit findings with sunxi-tools implementation:

```bash
# sunxi-tools FEL protocol is in fel.c and fel_lib.c
# Compare protocol constants, USB operations, timing values
# Look for differences that could explain H713 incompatibility
```

### 5. Analyze USB Traffic Patterns (If Possible)

If Windows VM or protocol capture available:
- Capture PhoenixSuit USB traffic with Wireshark/USBPcap
- Analyze FEL command sequences
- Compare timing between commands
- Identify any H713-specific command variants

### 6. Deep Binary Analysis with Disassembler

**If strings/objdump yield insufficient info:**

Use Ghidra (open source) to disassemble eFex.dll:
1. Load eFex.dll into Ghidra
2. Let auto-analysis complete
3. Search for:
   - Memory address constants (0x104000, 0x121000, etc.)
   - USB bulk transfer calls
   - SoC ID checks (0x1860)
   - Protocol state machines
4. Document control flow and key functions

## Research Targets

### Primary Research Questions

1. **H713 Memory Map:**
   - Does PhoenixSuit embed H713 SPL/scratch/thunk addresses?
   - Are they the same as Task 027 findings (0x104000, 0x121000, 0x123a00)?

2. **USB Protocol Differences:**
   - Does H713 use different USB command sequences?
   - Are there timing delays between commands?
   - Different chunk sizes for bulk transfers?

3. **BROM Workarounds:**
   - Does PhoenixSuit have special handling for H713 BROM bugs?
   - Any device reset or re-enumeration logic?
   - Error recovery mechanisms?

4. **FEL vs FES Mode:**
   - When does PhoenixSuit use FEL (BROM) vs FES (SPL)?
   - Does it avoid certain FEL operations on H713?
   - Does it quickly transition to FES mode?

5. **Version Detection:**
   - How does PhoenixSuit detect H713 vs other SoCs?
   - Are there H713-specific code paths?

### Secondary Research Questions

- Image validation and checksums
- Encryption/signing for H713
- Partition table format
- Boot0 interaction patterns

## Expected Findings

### Best Case Scenario
- Find explicit H713 memory addresses in DLLs
- Discover H713 BROM workaround logic
- Extract complete FEL protocol variant for H713
- **Result:** Can modify sunxi-tools to work with H713

### Realistic Scenario
- Find partial protocol differences
- Identify USB timing requirements
- Understand FEL → FES transition strategy
- **Result:** Actionable improvements to sunxi-tools

### Worst Case Scenario
- PhoenixSuit uses obfuscation or encryption
- H713 logic is in server-side components
- No clear protocol differences identifiable
- **Result:** Document limitations, recommend alternative approaches

## Deliverables

### Primary Document: PHOENIXSUIT_FEL_PROTOCOL_ANALYSIS.md

**Structure:**
```markdown
# PhoenixSuit FEL Protocol Analysis for H713

## Executive Summary
[Key findings and recommendations]

## DLL Analysis Results
### eFex.dll
[Functions, imports, exports, strings]

### Phoenix_Fes.dll  
[FES protocol specifics]

### luaeFex.dll
[Lua interface discoveries]

## H713-Specific Findings
### Memory Addresses
[SPL, scratch, thunk if found]

### Protocol Differences
[USB commands, timing, sequences]

### BROM Workarounds
[Any special H713 handling]

## Comparison with sunxi-tools
[Differences and gaps]

## Actionable Recommendations
1. [Specific changes to try]
2. [Testing procedures]
3. [Alternative approaches if not resolvable]

## Evidence
[Screenshots, hex dumps, disassembly snippets]

## Confidence Assessment
[How reliable are these findings]

## Next Steps
[What to do with this information]
```

### Supporting Files
- `tools/phoenixsuit/dll_analysis/` - DLL strings, exports, analysis
- Updated `TASK_027_RESEARCH_SUMMARY.md` with PhoenixSuit findings
- Patch file if sunxi-tools modifications identified

## Quality Validation

- [ ] All DLLs analyzed with multiple methods (strings, objdump, disassembly)
- [ ] Cross-validated findings between multiple files
- [ ] Compared with Task 027 memory map analysis
- [ ] Documented evidence for each finding (hex offsets, screenshots)
- [ ] Recommendations are specific and testable
- [ ] Negative results documented (what wasn't found)

## Integration with Task 027

This task builds on Task 027's findings:

**Task 027 Discovered:**
- H713 memory addresses from boot0.bin analysis
- BROM USB transfer incompatibility
- sunxi-fel-h713-fixed binary ready for testing

**This Task Seeks:**
- Validation of Task 027 addresses from official tool
- Understanding of H713 BROM workarounds
- Protocol-level solutions to USB incompatibility

**Combined Impact:**
- If PhoenixSuit confirms Task 027 addresses → HIGH confidence
- If PhoenixSuit reveals workarounds → May resolve FEL issues
- If PhoenixSuit avoids FEL entirely → Explains why direct FEL fails

## Alternative Outcomes

### Outcome A: FEL Protocol Fix Discovered
- Update sunxi-tools with PhoenixSuit logic
- Test on hardware with modified sunxi-fel
- **Unblocks:** Phase II hardware validation via FEL

### Outcome B: FEL Unusable, FES Required
- Document that PhoenixSuit uses FES mode exclusively
- Build custom FES payload for H713
- **Unblocks:** Different approach to hardware access

### Outcome C: Insufficient Information
- Document PhoenixSuit limitations
- Recommend other approaches (SD card boot, fastboot, etc.)
- **Result:** Proceed with non-FEL methods

## Success Metrics

**Primary Success:**
- Discover actionable protocol differences that can be tested

**Secondary Success:**  
- Validate or invalidate Task 027 memory addresses
- Understand why direct FEL fails on H713

**Minimum Success:**
- Document what PhoenixSuit does differently
- Rule out certain approaches to save time

## Timeline

**Estimated Time:** 4-6 hours

**Phase 1:** Binary analysis (strings, objdump) - 2 hours  
**Phase 2:** Disassembly with Ghidra - 2 hours  
**Phase 3:** Documentation and recommendations - 1-2 hours  

**Hardware Dependency:** None (pure software analysis)

## Tools Required

**Available:**
- ✅ strings, hexdump, objdump (Nix devShell)
- ✅ PhoenixSuit binaries downloaded
- ✅ Previous analysis documents

**Optional:**
- Ghidra (for deep disassembly)
- Windows VM with Wireshark/USBPcap (for protocol capture)
- IDA Pro (alternative to Ghidra)

## Risk Assessment

**Low Risk:**
- No hardware access required
- No destructive operations
- Pure analysis of existing files

**Effort vs Payoff:**
- **High effort** if deep disassembly required
- **High payoff** if FEL protocol fix discovered
- **Medium payoff** if workarounds identified
- **Low payoff** if PhoenixSuit uses obfuscated/server logic

**Mitigation:**
- Start with quick analysis (strings, objdump)
- Only proceed to disassembly if promising leads found
- Time-box effort to 6 hours maximum

## Next Task Dependencies

**If Successful:**
- Task 027d: Update with PhoenixSuit findings
- Task 027e: Test modified sunxi-tools on hardware

**If Unsuccessful:**
- Task 035: Alternative hardware access methods (SD boot, fastboot)

## Related Documentation

- `tools/phoenixsuit/ANALYSIS_SUMMARY.md` - Initial PhoenixSuit analysis
- `tools/phoenixsuit/PHOENIXSUIT_H713_FORMAT_ANALYSIS.md` - Image format
- `docs/FEL_MODE_ANALYSIS.md` - Original FEL failure analysis
- `docs/TASK_027_RESEARCH_SUMMARY.md` - Memory map research
- `docs/H713_BROM_MEMORY_MAP.md` - boot0 analysis results
- Task 027 (subtasks a/b/c) - Complete FEL research phase

## External References

- [sunxi-tools fel.c source](https://github.com/linux-sunxi/sunxi-tools/blob/master/fel.c)
- [Allwinner FEL protocol](http://linux-sunxi.org/FEL)
- [Ghidra reverse engineering](https://ghidra-sre.org/)
- PhoenixSuit GitHub: colorfulshark/PhoenixSuit

---

**Status:** completed - Beginning DLL analysis phase  
**Next:** Start with strings and objdump analysis of key DLLs
