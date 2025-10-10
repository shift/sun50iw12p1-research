# Task 027: Add H713 Support to sunxi-tools

**Status:** in_progress  
**Priority:** high  
**Phase:** FEL Mode Integration  
**Created:** 2025-10-10  
**Context:** Enable FEL mode operations for H713 SoC

## Objective

Add Allwinner H713 (SoC ID 0x1860) support to sunxi-tools to enable FEL mode firmware backup and U-Boot booting for the HY300 projector.

## Prerequisites

- [x] sunxi-tools source cloned and analyzed
- [x] H713 memory maps extracted from firmware analysis
- [x] H616 configuration identified as baseline (H713 is H616 derivative)
- [x] DRAM parameters documented (firmware/DRAM_ANALYSIS.md)
- [x] SRAM layout confirmed from device tree (sun50i-h713-hy300.dts)

## Implementation

### H713 SoC Configuration

Based on H616 (0x1823) with identical memory layout:

```c
.soc_id       = 0x1860, /* Allwinner H713 */
.name         = "H713",
.spl_addr     = 0x20000,
.scratch_addr = 0x21000,
.thunk_addr   = 0x53a00, .thunk_size = 0x200,
.swap_buffers = h616_sram_swap_buffers,
.sram_size    = 207 * 1024,
.sid_base     = 0x03006000,
.sid_offset   = 0x200,
.sid_sections = generic_2k_sid_maps,
.rvbar_reg    = 0x09010040,
.rvbar_reg_alt= 0x08100040,
.ver_reg      = 0x03000024,
.watchdog     = &wd_h6_compat,
```

### Memory Map Rationale

- **SPL Address (0x20000):** Standard SRAM A1 base for H6/H616 family
- **SRAM Size (207 KiB):** SRAM A1 (32K) + SRAM C (175K) contiguous region
- **Swap Buffers:** Reuse H616 buffer configuration (tested and working)
- **SID Base (0x03006000):** Security ID register base from device tree analysis
- **RVBAR Registers:** ARM reset vector address registers for ARMv8 cores

### Files Modified

1. **sunxi-tools-source/soc_info.c:** Added H713 entry in soc_info_table[] after H616 (line 565)

### Build Process

```bash
cd sunxi-tools-source
nix-shell -p gcc gnumake libusb1 pkg-config zlib dtc --run "make sunxi-fel"
```

### Verification

```bash
./sunxi-fel --list-socs | grep H713
# Output: 1860: H713
```

## Testing Protocol

### Phase 1: Basic Device Detection ✅ COMPLETE
- [x] Verify H713 in supported SoC list
- [x] Test `sunxi-fel version` command
- [x] Confirm SoC ID 0x1860 recognized

### Phase 2: Memory Operations (CURRENT)
- [ ] Test SID (Security ID) read
- [ ] Test small memory read (256 bytes)
- [ ] Test larger memory read (1 MB)
- [ ] Test memory write operations

### Phase 3: Firmware Operations
- [ ] Test U-Boot SPL upload
- [ ] Test full U-Boot upload
- [ ] Test firmware execution
- [ ] Perform full eMMC backup

## Expected Outcomes

### Success Criteria
- ✅ H713 recognized in FEL mode
- ⏳ Memory read/write operations succeed
- ⏳ U-Boot SPL uploads without overflow errors
- ⏳ Complete firmware backup possible

### Current Status
- ✅ H713 support added to sunxi-tools
- ✅ Binary compiled and tested
- ✅ SoC detection working
- ⏳ Awaiting hardware testing for memory operations

## Quality Validation

- [x] Source code compiles without errors
- [x] H713 appears in `--list-socs` output
- [x] SoC ID 0x1860 properly recognized
- [ ] Memory operations complete successfully
- [ ] U-Boot upload works without errors
- [ ] Full firmware backup validated

## Integration with Project

### Enables Future Work
- **Hardware Testing:** Safe FEL-based U-Boot testing without eMMC modification
- **Firmware Backup:** Complete factory firmware backup via FEL mode
- **Development Workflow:** Rapid U-Boot iteration without eMMC wear
- **Recovery System:** FEL mode as fallback if eMMC boot fails

### Documentation Updates Required
- [x] docs/FEL_MODE_ANALYSIS.md - Add H713 support status
- [ ] README.md - Update FEL mode capabilities
- [ ] docs/HY300_TESTING_METHODOLOGY.md - Add FEL-based testing procedures

## Risk Assessment

### Technical Risks
- **Memory map differences:** H713 may have subtle differences from H616
- **USB timing:** FEL protocol timing may need adjustment
- **Hardware variations:** Different HY300 production runs may behave differently

### Mitigation Strategies
- Start with read-only operations (SID, memory read)
- Test with small data transfers first
- Keep factory firmware backup via alternative method
- Document any errors for upstream sunxi-tools contribution

## Upstream Contribution

### Patch Submission Plan
1. **Test thoroughly** on actual H713 hardware
2. **Document findings** including any quirks or special handling
3. **Create clean patch** against sunxi-tools master
4. **Submit to linux-sunxi** mailing list with test results
5. **Reference HY300** as test platform in submission

### Patch File
Generated patch: `sunxi-tools-h713-support.patch`

## Next Steps

1. **Hardware Testing:** Test memory operations with HY300 device in FEL mode
2. **Document Results:** Record all FEL operations and any errors
3. **Refine Configuration:** Adjust memory maps if needed based on testing
4. **Upstream Submission:** Submit patch to sunxi-tools project

## Notes

**Critical Discovery:** H713 was completely missing from sunxi-tools database, causing "Warning: no 'soc_sram_info' data for your SoC (id=1860)" and all FEL operations to fail.

**Design Decision:** Used H616 configuration as baseline since H713 is an H616 derivative with identical SRAM layout (confirmed via device tree analysis).

**Build System:** Used Nix for reproducible build environment with all required dependencies.
