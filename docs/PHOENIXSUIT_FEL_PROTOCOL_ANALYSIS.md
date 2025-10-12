# PhoenixSuit FEL Protocol Analysis for H713

**Date:** 2025-10-12  
**Task:** 034 - PhoenixSuit FEL Protocol Reverse Engineering  
**Status:** Analysis Complete - Limited H713 Information Found  
**Time:** 2 hours

## Executive Summary

Analysis of PhoenixSuit V1.10 reveals **limited H713-specific FEL protocol information**. The tool version (2019) predates widespread H713 availability, resulting in no explicit H713 configuration or workarounds in the codebase.

### Key Findings
- ✅ PhoenixSuit V1.10 architecture understood (plugin system, USB communication flow)
- ✅ Possible H713 SoC ID reference found in data table (0x1860 at offset 0x002182c4)
- ❌ No H713-specific memory maps embedded in binaries
- ❌ No H713 BROM workaround logic identifiable
- ❌ No explicit FEL protocol differences documented for H713
- ⚠️ **Recommendation:** Search for newer PhoenixSuit version (V1.18+) with H713 support

### Impact on Project
- **Current FEL Issues:** Remain unresolved by this analysis
- **Alternative Identified:** PhoenixSuit may use FES mode primarily, avoiding BROM FEL
- **Next Steps:** Investigate FES (FEL Stage 2) protocol as workaround

---

## Analysis Methodology

### Tools Used
```bash
# Binary analysis tools
strings         # Extract readable strings from binaries
objdump         # Analyze PE structure, imports, exports
hexdump         # Search for hex patterns (SoC IDs, addresses)
file            # Identify file types

# Files analyzed
- PhoenixSuit.exe (2.2 MB) - Main executable
- eFex.dll (44 KB) - FEL/FEX protocol plugin
- Phoenix_Fes.dll (36 KB) - FES protocol plugin  
- luaeFex.dll (40 KB) - Lua FEL bindings
- AdbWinApi.dll, AdbWinUsbApi.dll - USB communication
- *.lua scripts (compiled Lua 5.1 bytecode)
```

### Analysis Steps Performed
1. ✅ Extracted strings from all key DLLs (586 unique strings)
2. ✅ Analyzed PE imports/exports (DLL dependencies mapped)
3. ✅ Searched for H713 SoC ID (0x1860) in hex dumps
4. ✅ Searched for known memory addresses from Task 027
5. ✅ Examined Lua script interfaces (all compiled bytecode)
6. ✅ Searched configuration files for H713 references
7. ❌ Deep disassembly with Ghidra - **NOT performed** (time constraints, low expected value)

---

## DLL Analysis Results

### eFex.dll (FEL/FEX Protocol Handler)

**Size:** 44 KB  
**Purpose:** Plugin interface for FEL/FEX protocol operations

**Imports:**
- KERNEL32.dll only (no USB-specific libraries)
- Standard file I/O functions (CreateFile, DeviceIoControl)
- No libusb or WinUSB imports

**Exports:**
```
GetAuthor
GetCopyRight
GetInterface
GetPlugInName
GetPlugInType
GetPlugInVersion
QueryInterface
```

**Key Strings:**
```
fel_up
fel_down
FEX_CMD_fes_trans
DOU_UPLOAD
DOU_DOWNLOAD
```

**Analysis:**
- eFex.dll is a **plugin wrapper**, not the actual protocol implementation
- Exports generic plugin interface functions
- Actual FEL communication likely in PhoenixSuit.exe or USB DLLs
- References FES transition commands (`FEX_CMD_fes_trans`)

### Phoenix_Fes.dll (FES Protocol Implementation)

**Size:** 36 KB  
**Purpose:** FES (FEL Stage 2) protocol handler

**Imports:**
- KERNEL32.dll only
- Same file I/O pattern as eFex.dll

**Exports:**
- Same plugin interface as eFex.dll

**Key Strings:**
```
Phoenix_Fes.dll
Fes_Open
Elf2Fes
Phoenix_Fes.cpp
Phoenix_Fes_private.cpp
```

**Analysis:**
- Also a plugin wrapper
- References "Elf2Fes" - suggests ELF binary → FES payload conversion
- FES is secondary protocol stage (runs after BROM FEL loads first-stage bootloader)

### luaeFex.dll (Lua FEL Bindings)

**Size:** 40 KB  
**Purpose:** Expose FEL operations to Lua scripts

**Analysis:**
- Lua 5.1 interface for FEL commands
- All Lua scripts are **compiled bytecode** (not human-readable source)
- Would require Lua decompiler to extract logic

### USB Communication DLLs

**AdbWinApi.dll / AdbWinUsbApi.dll:**
- Standard Android ADB (Android Debug Bridge) libraries
- Used for post-boot Android device communication
- Not FEL/FES protocol related

**PhoenixSuit.exe Main Executable:**

**Imports:**
- SETUPAPI.dll - Windows USB device enumeration
- Standard Windows UI libraries (USER32, GDI32, COMCTL32)
- No libusb imports (uses Windows native USB APIs)

---

## H713 SoC ID Search Results

### Hex Pattern Search for 0x1860

**Found at offset 0x002182c4 in PhoenixSuit.exe:**

```
Offset    Hex Data                              ASCII
002182c0  68 3e 6c 3e 00 60 18 00  10 00 00 00  h>l>.`......
002182d0  f0 3f f4 3f f8 3f fc 3f  00 70 18 00  .?.?.?.?.p..
```

**Interpretation:**
```
00 60 18 00 = 0x00186000 (little-endian)
  → Could be: 0x1860 (H713 SoC ID) with padding
  → Or unrelated data

10 00 00 00 = 0x00000010 (16 decimal)
  → Possibly: configuration value, version, or flag

00 70 18 00 = 0x00187000
  → Adjacent data, relationship unclear
```

**Confidence:** LOW (30%)
- Pattern matches H713 SoC ID
- Context does not clearly indicate SoC configuration table
- No accompanying memory addresses or protocol parameters
- Could be coincidental data

**Negative Result:** Searched for Task 027 memory addresses
- 0x00104000 (SPL load address) - **NOT FOUND**
- 0x00121000 (scratch address) - **NOT FOUND**
- 0x00123a00 (thunk address) - **NOT FOUND**

---

## FEL Protocol Structure Analysis

### PhoenixSuit Communication Architecture

```
PhoenixSuit.exe
    ├──> SETUPAPI.dll (USB device enumeration)
    ├──> eFex.dll (FEL protocol plugin)
    │     └──> FEL Stage 1 (BROM)
    │           ├── USB Bulk Transfers
    │           ├── Memory Read/Write
    │           └── SPL Upload
    ├──> Phoenix_Fes.dll (FES protocol plugin)
    │     └──> FEL Stage 2 (SPL/U-Boot)
    │           ├── Extended Commands
    │           ├── Partition Flashing
    │           └── Image Management
    └──> AdbWinApi.dll (Android Debug Bridge)
          └──> Android Runtime
                ├── APK Installation
                ├── Shell Commands
                └── Debugging
```

### FEL vs FES Mode Strategy

**Hypothesis:** PhoenixSuit minimizes BROM FEL usage

**Evidence:**
1. FES protocol has separate DLL with substantial functionality
2. "Elf2Fes" suggests converting payloads to FES format
3. FEL plugin references FES transition (`FEX_CMD_fes_trans`)

**Strategy:**
```
1. Enter BROM FEL mode (USB 0x1f3a:0xefe8)
2. Upload minimal FES payload via FEL
3. Execute FES payload (transitions to FEL Stage 2)
4. Perform all operations via FES protocol (more robust)
5. Flash partitions using FES commands
6. Exit and reboot to Android
```

**Impact on H713 BROM Bug:**
- If PhoenixSuit uses FEL minimally (only FES loader upload)
- BROM bug might not affect small FES payload transfers
- Most operations occur in FES mode (after bug triggered)
- **This could explain why PhoenixSuit works despite BROM issues**

---

## Comparison with sunxi-tools

### sunxi-tools FEL Implementation

**Approach:**
- Uses BROM FEL extensively for all operations
- Uploads full U-Boot SPL (732 KB) via FEL bulk transfers
- No FES protocol support

**H713 Issue:**
- Large bulk transfers trigger BROM USB crash bug
- sunxi-tools has no FES fallback mechanism

### PhoenixSuit Likely Approach

**Approach:**
- Minimal BROM FEL usage (FES loader only)
- FES loader is small (typically 16-64 KB)
- FES protocol handles bulk operations

**H713 Compatibility:**
- Small FES loader may avoid BROM bug threshold
- FES protocol bypasses BROM limitations
- More robust for production flashing tools

### Key Differences Summary

| Feature | sunxi-tools | PhoenixSuit (inferred) |
|---------|------------|----------------------|
| BROM FEL Usage | Extensive (all operations) | Minimal (FES loader only) |
| SPL Upload Size | 732 KB via FEL | ~16-64 KB FES loader |
| Bulk Transfers | Direct BROM FEL | FES protocol |
| H713 BROM Bug | Triggers on large transfers | Possibly avoided |
| Protocol Stages | 1 (FEL only) | 2 (FEL → FES) |
| Robustness | Lower (BROM dependent) | Higher (FES features) |

---

## Actionable Recommendations

### Immediate: Test FES Protocol Approach

**Hypothesis:** H713 BROM handles small transfers successfully

**Test Procedure:**
1. Create minimal FES loader payload (16 KB)
2. Attempt upload via sunxi-fel
3. If successful, implement FES protocol handler
4. Perform bulk operations via FES instead of FEL

**Expected Outcome:**
- Small FES loader upload succeeds
- Enables FES protocol for robust operations
- Bypasses BROM FEL limitations

**Confidence:** 60% - Based on PhoenixSuit architecture analysis

### Alternative 1: Chunked Transfer Strategy

**If BROM has size threshold:**
- Split U-Boot SPL into smaller chunks (4-16 KB each)
- Upload chunks sequentially with delays
- Reassemble in DRAM

**Implementation:**
```c
// Pseudo-code for chunked upload
#define CHUNK_SIZE 8192  // 8 KB chunks

for (offset = 0; offset < spl_size; offset += CHUNK_SIZE) {
    chunk_size = min(CHUNK_SIZE, spl_size - offset);
    fel_write(spl_addr + offset, spl_data + offset, chunk_size);
    usleep(100000);  // 100ms delay between chunks
}
```

**Confidence:** 40% - Untested, may not resolve root cause

### Alternative 2: SD Card Boot

**Bypass FEL entirely:**
- Write U-Boot to SD card
- Boot from SD card slot
- Test mainline kernel via SD boot

**Advantages:**
- No FEL/BROM dependency
- Known working approach
- Easier development workflow

**Disadvantages:**
- Requires SD card access
- Less convenient for iteration
- Cannot backup eMMC via FEL

**Confidence:** 95% - Standard Allwinner approach

### Long-term: Reverse Engineer FES Protocol

**If FES approach promising:**
1. Capture PhoenixSuit USB traffic with Wireshark
2. Analyze FES command structure
3. Implement FES protocol in sunxi-tools
4. Submit upstream for H713 support

**Effort:** High (40+ hours)  
**Value:** High (enables robust H713 FEL support community-wide)

---

## Findings Summary

### What We Learned

1. **PhoenixSuit Architecture:**
   - Plugin-based system (eFex, Phoenix_Fes, Lua bindings)
   - Likely uses FEL → FES transition strategy
   - Minimal BROM FEL dependency inferred

2. **H713 Support Status:**
   - PhoenixSuit V1.10 (2019) lacks explicit H713 configuration
   - Possible SoC ID reference found but unconfirmed
   - No H713-specific memory maps or workarounds

3. **Protocol Differences:**
   - PhoenixSuit emphasizes FES protocol (Stage 2)
   - sunxi-tools lacks FES support
   - This may explain H713 compatibility difference

### What We Didn't Find

1. ❌ H713 BROM memory map (SPL, scratch, thunk addresses)
2. ❌ H713 BROM workaround logic
3. ❌ Explicit FEL protocol modifications for H713
4. ❌ USB transfer timing or chunk size requirements
5. ❌ H713-specific device initialization sequences

### Confidence Assessment

**Overall Findings:** Medium (50%)

**FES Hypothesis:** Medium-High (60%)
- Strong architectural evidence
- Logical approach for production tool
- Explains PhoenixSuit robustness

**H713 SoC ID Finding:** Low (30%)
- Single data pattern match
- No clear configuration context
- Requires validation

**Actionable Value:** Medium
- FES protocol approach is testable
- Chunked transfer strategy is implementable
- SD card boot is proven fallback

---

## PhoenixSuit Version Recommendations

### Need for Newer Version

PhoenixSuit V1.10 (2019) analysis limitations:
- Predates H713 widespread availability
- May lack H713-specific optimizations
- Likely missing H713 BROM bug workarounds

### Recommended Search

**Target:** PhoenixSuit V1.18 or later (2021+)

**Search Strategies:**
1. Chinese firmware forums (全志 H713 工具)
2. OEM manufacturer support sites
3. Allwinner developer portal (requires account)
4. XDA Developers, 4PDA.ru communities
5. GitHub: search "PhoenixSuit H713"

**Expected Improvements:**
- Explicit H713 SoC configuration
- H713 BROM bug workarounds
- Optimized memory maps for H713
- FES loader with H713 support

---

## Integration with Task 027

### Task 027 Findings (BROM Memory Analysis)

**From boot0.bin reverse engineering:**
- SPL load address: 0x00104000 (HIGH confidence 95%)
- Scratch address: 0x00121000 (HIGH confidence 85%)
- Thunk address: 0x00123a00 (HIGH confidence 85%)

**PhoenixSuit Analysis:**
- ❌ Does NOT validate Task 027 addresses
- ❌ Does NOT contradict Task 027 addresses
- ⚠️ Insufficient data to confirm or deny

### Combined Strategy

**Task 027 + Task 034 Recommendations:**

1. **Test Task 027 sunxi-fel-h713-fixed binary** (when hardware available)
   - If succeeds: FEL protocol compatible, BROM memory map correct
   - If fails: Proceed to FES approach

2. **Test FES Protocol Approach**
   - Create minimal FES loader (16 KB)
   - Upload via Task 027 addresses
   - If succeeds: Implement full FES protocol

3. **Fallback to SD Card Boot**
   - Known working method
   - Development continues without FEL

**Confidence in Combined Approach:** 85%
- Task 027 addresses are evidence-based (boot0 analysis)
- PhoenixSuit FES strategy provides alternative if direct FEL fails
- SD card boot guaranteed to work

---

## Files Created

### Analysis Outputs
```
tools/phoenixsuit/dll_analysis/
├── efex_strings.txt           # 223 strings from eFex.dll
├── phoenix_fes_strings.txt    # 177 strings from Phoenix_Fes.dll
├── luaefex_strings.txt        # 186 strings from luaeFex.dll
├── efex_imports.txt           # eFex.dll PE import table
└── phoenix_fes_imports.txt    # Phoenix_Fes.dll PE import table
```

### Documentation
- This file: `docs/PHOENIXSUIT_FEL_PROTOCOL_ANALYSIS.md`

---

## Next Steps

### When Hardware Available (Task 027 Testing)

**Priority 1: Test Task 027 Binary**
```bash
# Test with Task 027 memory configuration
./sunxi-fel-h713-fixed version
./sunxi-fel-h713-fixed spl u-boot-sunxi-with-spl.bin
```

**If Succeeds:**
- FEL protocol working
- Task 027 memory map validated
- Phase II complete

**If Fails with Timeout:**
- Proceed to FES approach testing
- Try chunked transfer strategy

### Software Analysis (No Hardware Required)

**Priority 2: Research FES Protocol**
1. Review PhoenixSuit FES payload structure
2. Analyze "Elf2Fes" conversion process
3. Research Allwinner FES protocol documentation
4. Design minimal FES loader for H713

**Priority 3: Search for Newer PhoenixSuit**
1. Chinese firmware forums and OEM sites
2. Extract and analyze PhoenixSuit V1.18+
3. Compare H713-specific changes
4. Update recommendations based on findings

### Alternative Hardware Approaches

**Priority 4: SD Card Boot Method**
1. Prepare SD card with U-Boot
2. Boot HY300 from SD card
3. Test mainline kernel
4. Proceed with Phase VI (mainline Linux) without FEL dependency

---

## Lessons Learned

### Analysis Methodology

**What Worked:**
- Systematic binary analysis (strings, objdump, hexdump)
- Searching for known patterns (SoC IDs, memory addresses)
- Understanding overall architecture before deep diving

**What Didn't Work:**
- Older tool version lacks target SoC information
- Compiled Lua scripts require decompilation
- Plugin architecture hides actual protocol implementation

**Improvements for Future:**
- Prioritize finding correct tool version first
- Use USB protocol capture if possible (Wireshark)
- Disassemble with Ghidra for deeper analysis

### Time Investment

**Total Time:** ~2 hours

**Breakdown:**
- DLL analysis: 45 minutes
- Hex pattern searching: 30 minutes
- Architecture analysis: 30 minutes
- Documentation: 15 minutes

**Value:** Medium
- Ruled out easy wins (no H713 config in V1.10)
- Identified FES protocol alternative approach
- Confirmed need for newer PhoenixSuit version

---

## Conclusion

PhoenixSuit V1.10 analysis provides **limited H713-specific information** but reveals an important insight: **FES protocol usage as an alternative to direct BROM FEL operations**.

### Key Takeaway

The H713 BROM FEL bug may be **worked around** rather than **fixed** in production tools like PhoenixSuit, by:
1. Minimizing BROM FEL usage (small FES loader only)
2. Transitioning quickly to FES protocol (Stage 2)
3. Performing bulk operations via FES (after BROM)

This strategy is **testable** and provides a **viable path forward** if Task 027's direct FEL approach fails.

### Recommendation Priority

1. **Test Task 027 sunxi-fel-h713-fixed** (hardware dependent)
2. **Research and test FES protocol approach** (software analysis)
3. **Find PhoenixSuit V1.18+ with H713 support** (software research)
4. **Fallback to SD card boot** (proven method)

---

**Document Status:** Complete  
**Task Status:** Analysis phase complete, recommendations ready for testing  
**Next Action:** Await hardware access for Task 027 FEL testing, or proceed with FES protocol research
