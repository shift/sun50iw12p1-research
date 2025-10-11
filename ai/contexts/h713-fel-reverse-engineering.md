# H713 FEL Reverse Engineering Context

## Purpose
This context provides complete information for atomic task delegations related to reverse engineering the H713 FEL protocol and memory map. Each section is self-contained for specialized agent delegation.

## Project Background

**Project:** HY300 Android Projector Linux Porting  
**Hardware:** Allwinner H713 SoC (sun50iw12p1)  
**Goal:** Boot mainline U-Boot and Linux via FEL mode  
**Current Phase:** Phase II - U-Boot Porting  

**Critical Constraint:** No UART/serial console access - all testing via USB FEL mode

## Current Technical Status

### What Works
- H713 SoC detection via USB (SoC ID: 0x1860)
- FEL version query returns valid data:
  ```
  AWUSBFEX soc=00001860(H713) 00000001 ver=0001 44 08 scratchpad=00121500 00000000 00000000
  ```
- USB device enumeration (Google/Allwinner FEL device)
- Basic FEL protocol communication established

### What Fails
- SPL upload: `ERROR -7: Operation timed out`
- Bulk transfer operations timeout
- Memory read/write operations fail
- USB connection unstable (device resets, incrementing device numbers)

### Root Cause Analysis
Current H713 entry in sunxi-tools uses H616 memory map:
```c
{ 0x1860, "H713", true, /* H713 based on H616 architecture */
  .spl_addr = 0x20000,      // May be wrong for H713
  .scratch_addr = 0x108000, // May be wrong for H713  
  .thunk_addr = 0x118000,   // May be wrong for H713
  .thunk_size = 0x8000,     // May be wrong for H713
  .swap_buffers = false,
  .needs_l2en = true,
  .sid_base = 0x03006000,
  .sid_offset = 0x200,
  .sid_sections = 4
}
```

**Hypothesis:** H713 BROM has different SRAM layout than H616, causing incorrect memory access during FEL bulk transfers.

## Technical Background: FEL Protocol

### FEL Mode Overview
- **F**el **E**ngine **L**oader - Allwinner's USB recovery mode
- Built into Boot ROM (BROM) - cannot be bricked
- Allows loading code into SRAM via USB without eMMC access
- Used for recovery, development, and initial bootloader deployment

### Memory Map Requirements
FEL protocol requires four critical addresses:

1. **`spl_addr`** - Where SPL (Secondary Program Loader) is loaded
   - Must be in SRAM A1 region
   - Must have enough space for U-Boot SPL (~196KB)
   - Typical values: 0x20000 (H616), 0x10000 (older SoCs)

2. **`scratch_addr`** - FEL protocol scratchpad area
   - Used for FEL command/response buffers
   - Must not overlap with SPL
   - Typical values: 0x108000 (H616), varies by SoC

3. **`thunk_addr`** - Small code execution area
   - Used for FEL helper functions
   - Must be in executable SRAM region
   - Typical values: 0x118000 (H616)

4. **`thunk_size`** - Size of available SRAM for thunk
   - Determines buffer sizes for operations
   - Typical values: 0x8000 (32KB)

### SRAM Layout Pattern (Allwinner SoCs)
```
SRAM A1: 0x00000000 - 0x0000FFFF (64KB, typical)
SRAM A2: 0x00040000 - 0x00043FFF (16KB, typical)
SRAM C:  0x00010000 - 0x0001FFFF (varies)
```

**H616 Reference Layout:**
- SRAM A1: 32KB at 0x00020000
- Used for SPL loading
- Scratchpad at higher SRAM regions

**H713 Unknown Layout:**
- May have different SRAM base addresses
- May have different SRAM sizes
- May have different region allocations

## Key Files and Locations

### Source Files (Read-Only Reference)
- **`build/sunxi-tools/soc_info.c`** - H713 configuration (line ~215)
- **`boot0.bin`** - First-stage bootloader (contains BROM addresses)
- **`u-boot-sunxi-with-spl.bin`** - U-Boot binary to upload (732KB)
- **`sunxi-fel-h713-new`** - Current sunxi-fel binary with H713 support

### Factory Firmware (Analysis Sources)
- **`firmware/extracted_components/`** - Extracted Android firmware
  - `boot.img` - Android kernel with FEL drivers
  - `android_kernel/` - Kernel source references
  - Device tree files with SRAM mappings

### Documentation (Reference)
- **`docs/FEL_MODE_ANALYSIS.md`** - Previous FEL research
- **`docs/HY300_TESTING_METHODOLOGY.md`** - Safe testing procedures
- **`firmware/ROM_ANALYSIS.md`** - Complete ROM structure analysis
- **`USING_H713_FEL_MODE.md`** - H713 FEL usage documentation

## Analysis Methodologies

### Method 1: boot0.bin Static Analysis
**Goal:** Extract BROM/SRAM addresses from first-stage bootloader

**Tools:**
- `binwalk` - Identify embedded structures
- `strings` - Find ASCII address references
- `hexdump -C` - Manual hex analysis
- `objdump` (if ARM code identified) - Disassemble

**Search Patterns:**
- ARM reset vectors (0xEA000000 pattern)
- SRAM base addresses (0x000xxxxx, 0x001xxxxx ranges)
- FEL handler references (string "AWUSBFEX" or USB IDs)
- Memory region definitions (size/base pairs)

**Expected Findings:**
- SRAM A1 base address and size
- BROM entry points and handlers
- Memory region initialization code
- FEL protocol handler addresses

### Method 2: Factory Firmware Mining
**Goal:** Extract documented FEL addresses from Android kernel/bootloader

**Sources:**
- Device tree files (*.dts, *.dtsi) - SRAM memory regions
- Kernel FEL drivers (sunxi-fel.c or similar)
- Bootloader code (U-Boot or custom)
- System logs with memory mappings

**Search Patterns:**
```bash
# In device trees
grep -r "sram" *.dts *.dtsi
grep -r "0x00020000\|0x00010000" *.dts

# In kernel source
grep -r "FEL\|AWUSBFEX" kernel/
grep -r "spl_addr\|scratch_addr" kernel/

# In bootloader
strings boot.img | grep -i "sram\|fel"
```

**Expected Findings:**
- Documented SRAM regions in device tree
- FEL driver memory configuration
- Bootloader SPL load addresses
- Validation of boot0.bin findings

### Method 3: H616 Comparison Analysis
**Goal:** Understand what assumptions are wrong in H616 memory map

**Process:**
1. Document H616 memory map completely (soc_info.c reference)
2. Extract H713 boot0 memory initialization code
3. Compare SRAM base addresses between SoCs
4. Identify offset differences
5. Calculate H713 equivalent addresses

**H616 Known Values:**
```c
.spl_addr = 0x20000,      // SRAM A1 + 128KB offset
.scratch_addr = 0x108000, // High SRAM region
.thunk_addr = 0x118000,   // Above scratch area
.thunk_size = 0x8000,     // 32KB
```

**Analysis Questions:**
- Does H713 have same SRAM base as H616?
- Is SRAM A1 at 0x00020000 or different address?
- Is scratchpad region at same offset?
- Are SRAM sizes identical?

### Method 4: Systematic Testing
**Goal:** Test candidate memory addresses empirically via FEL

**Safety:** All FEL testing is non-destructive (RAM-only)

**Test Matrix:**
| spl_addr | scratch_addr | thunk_addr | Result | Notes |
|----------|--------------|------------|--------|-------|
| 0x20000  | 0x108000     | 0x118000   | FAIL   | Current H616 config |
| 0x10000  | 0x108000     | 0x118000   | TEST   | Older SoC pattern |
| 0x00000  | 0x100000     | 0x110000   | TEST   | Alternative layout |

**Testing Procedure:**
1. Modify `build/sunxi-tools/soc_info.c` with candidate addresses
2. Build: `nix-shell -p pkg-config libusb1 zlib dtc -c make`
3. Test upload: `./sunxi-fel spl u-boot-sunxi-with-spl.bin`
4. Document results (success, timeout, USB error)
5. Iterate with refined candidates

## Evidence-Based Development Standards

### Documentation Requirements
Every finding must include:
- **Source file and line number** for code references
- **Hex offset** for binary analysis findings
- **Command output** for empirical tests
- **Cross-validation** from multiple sources when possible

### Example Evidence Format
```
Finding: H713 SRAM A1 base address
Source: boot0.bin offset 0x1234
Evidence: Hex pattern EA 00 00 00 (ARM reset vector)
Cross-ref: Factory device tree confirms 0x00020000 region
Validation: Test with spl_addr=0x20000 succeeds
```

### No Shortcuts Policy
- Never assume addresses without evidence
- Never skip validation steps
- Never mock/stub analysis tools
- Always test hypotheses empirically
- Document negative results (what doesn't work)

## Hardware Safety Protocols

### FEL Mode Safety
- **Non-destructive:** All FEL operations use RAM only
- **Recoverable:** Power cycle always returns to FEL mode
- **No eMMC writes:** Until validated, no persistent storage changes
- **USB-only:** No risk of boot loop or bricking

### Testing Guardrails
1. Always test via FEL mode first
2. Verify USB connection stable before upload
3. Monitor `dmesg` for USB errors during testing
4. Power cycle between test iterations if unstable
5. Never proceed with eMMC write until FEL validated

### Failure Recovery
- **USB timeout:** Power cycle device, retry FEL connection
- **Device reset:** Normal during failed upload, re-enumerate USB
- **Upload failure:** Safe, try different memory configuration
- **No response:** Power cycle, check USB cable, verify FEL mode

## Tool Usage Reference

### sunxi-fel Commands
```bash
# Verify FEL mode and read SoC info
./sunxi-fel version

# Upload SPL (this is what's currently failing)
./sunxi-fel spl u-boot-sunxi-with-spl.bin

# Read memory (for testing memory map)
./sunxi-fel readl 0x00020000

# Write memory (for testing)
./sunxi-fel writel 0x00020000 0x12345678

# Execute code at address
./sunxi-fel exe 0x00020000
```

### Binary Analysis Commands
```bash
# Extract strings with addresses
strings -t x boot0.bin | grep -i "sram\|fel"

# Hex dump with ASCII
hexdump -C boot0.bin | less

# Search for specific byte patterns
grep -abo $'\xEA\x00\x00\x00' boot0.bin

# Identify file structures
binwalk boot0.bin

# Disassemble ARM code (if identified)
aarch64-unknown-linux-gnu-objdump -D -b binary -m aarch64 boot0.bin
```

### USB Debugging Commands
```bash
# Monitor USB events
dmesg -w | grep -i "usb\|fel"

# List USB devices
lsusb -v | grep -A 20 "Google"

# Check device enumeration
ls -la /dev/bus/usb/*/

# USB traffic capture (advanced)
sudo modprobe usbmon
sudo wireshark  # Capture usb interface
```

## Delegation Strategy

### Atomic Task Breakdown
This research requires **5 independent atomic tasks**:

1. **boot0-brom-analysis** (1-2 hours)
   - Input: boot0.bin file
   - Output: SRAM address documentation with evidence
   - Tools: binwalk, strings, hexdump, objdump
   - Deliverable: BROM_MEMORY_MAP.md with addresses

2. **h616-h713-comparison** (1 hour)
   - Input: H616 soc_info.c + boot0 analysis results
   - Output: Difference analysis and H713 candidate addresses
   - Tools: Text diff, manual analysis
   - Deliverable: H713_MEMORY_MAP_CANDIDATES.md

3. **factory-firmware-mining** (1-2 hours)
   - Input: firmware/extracted_components/
   - Output: FEL addresses from factory firmware
   - Tools: grep, strings, device tree analysis
   - Deliverable: FACTORY_FEL_ADDRESSES.md

4. **memory-config-testing** (2-3 hours)
   - Input: Candidate addresses from tasks 1-3
   - Output: Empirical test results for each configuration
   - Tools: sunxi-fel, USB debugging
   - Deliverable: FEL_TESTING_RESULTS.md + working sunxi-fel binary

5. **fel-validation** (1 hour)
   - Input: Working configuration from task 4
   - Output: Complete validation and documentation
   - Tools: sunxi-fel, U-Boot testing
   - Deliverable: H713_FEL_PROTOCOL_GUIDE.md

### Delegation Requirements
Each atomic task delegation MUST include:
- Complete project context (from this file)
- Specific task objective and success criteria
- All required file paths and tool commands
- Evidence and documentation standards
- Safety protocols (FEL mode, no eMMC writes)
- Expected deliverable format

### Coordination Protocol
1. General agent delegates atomic task with full context
2. Specialized agent completes task autonomously
3. General agent verifies deliverable completeness
4. General agent commits results with task reference
5. General agent updates task status before next delegation
6. Repeat for next atomic task

## Success Metrics

### Immediate Success (Per Atomic Task)
- Deliverable document created with complete analysis
- All evidence includes source references
- Cross-validation attempted where possible
- Negative results documented (what doesn't work)

### Final Success (Complete Task 027)
- H713 memory map fully documented with evidence
- sunxi-fel uploads SPL without timeouts
- U-Boot SPL executes successfully via FEL
- All operations repeatable and reliable
- Documentation enables future FEL development

## Critical Constraints

### Environment
- **No Nix in delegated agents** - All commands must work in standard Linux environment
- **USB access required** - FEL testing needs physical device connection
- **Cross-compilation needed** - sunxi-fel builds require aarch64 toolchain
- **Factory firmware available** - All extracted components in firmware/ directory

### Time/Resource Limits
- Each atomic task should complete in 1-3 hours
- Total research phase: 1-2 days maximum
- Minimize device connection time (USB unstable)
- Document incrementally (don't wait until end)

### Technical Limitations
- No serial console (blind debugging)
- No direct BROM access (must infer from boot0)
- No official H713 documentation (reverse engineer only)
- USB connection flaky (requires retry logic)

## References and External Resources

### Allwinner FEL Protocol
- linux-sunxi.org FEL documentation: https://linux-sunxi.org/FEL
- sunxi-tools repository: https://github.com/linux-sunxi/sunxi-tools
- FEL protocol reverse engineering notes: https://linux-sunxi.org/BROM

### Similar SoC Analysis
- H616 FEL support implementation (sunxi-tools reference)
- H6 BROM analysis (similar architecture)
- A64 FEL memory map (documentation available)

### Tools and Utilities
- binwalk: Firmware analysis toolkit
- radare2: Binary analysis framework (optional, advanced)
- ghidra: Reverse engineering platform (optional, for ARM disassembly)

## Known Issues and Workarounds

### USB Instability
**Issue:** Device resets during failed operations, USB device number increments
**Workaround:** Power cycle between test iterations, use short USB cable, add retry logic

### Bulk Transfer Timeouts
**Issue:** Current H616 memory map causes immediate timeout
**Root Cause:** Incorrect SRAM addresses cause invalid memory access
**Solution:** This is what we're researching to fix

### No Serial Output
**Issue:** Cannot see U-Boot output to verify execution
**Workaround:** Use USB response codes, test memory reads to confirm execution

## Task Integration

### Related Tasks
- **Task 002:** DRAM parameter extraction (completed) - Similar analysis methodology
- **Task 003:** U-Boot compilation (completed) - SPL binary ready for upload
- **Phase II:** U-Boot Porting - This task blocks phase completion

### Upstream Dependencies
- Task 003 must be complete (U-Boot SPL built) ✅
- boot0.bin must be extracted ✅
- Factory firmware must be available ✅
- sunxi-fel with H713 support must be built ✅

### Downstream Dependencies
- Task 028: U-Boot FEL deployment (blocked by this task)
- Task 029: Linux kernel boot (blocked by successful U-Boot)
- Phase III: Mainline Linux (blocked by bootloader)

## Appendix: Current File Locations

### Binaries Ready for Testing
```
/home/shift/code/android_projector/
├── boot0.bin                          # First-stage bootloader (analysis source)
├── u-boot-sunxi-with-spl.bin          # U-Boot binary to upload (732KB)
├── sunxi-fel-h713-new                 # Current sunxi-fel with H713 support
└── build/sunxi-tools/                 # sunxi-tools source for modifications
    ├── soc_info.c                     # H713 configuration here
    └── fel.c                          # FEL protocol implementation
```

### Factory Firmware Components
```
firmware/extracted_components/
├── boot.img                           # Android kernel
├── android_kernel/                    # Kernel source references
├── system/                            # Android system partition
└── vendor/                            # Vendor binaries and configs
```

### Documentation
```
docs/
├── FEL_MODE_ANALYSIS.md              # Previous FEL research
├── HY300_TESTING_METHODOLOGY.md      # Safe testing procedures
└── tasks/027-*.md                    # This task file

firmware/
└── ROM_ANALYSIS.md                   # Complete ROM structure

ai/contexts/
└── h713-fel-reverse-engineering.md   # This context file
```

---

**Context Version:** 1.0  
**Last Updated:** 2025-10-11  
**Maintained By:** AI Agent Coordination  
**Purpose:** Self-contained context for atomic task delegations
