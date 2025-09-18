# Task 002: Extract DRAM Parameters from boot0.bin

**Status:** completed  
**Priority:** high  
**Phase:** II - U-Boot Porting  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Completed:** 2025-09-18  
**Context:** firmware/ROM_ANALYSIS.md, Phase II U-Boot porting

## Objective

Extract DRAM timing and configuration parameters from the boot0.bin bootloader to enable U-Boot compilation for the Allwinner H713 SoC. This is the critical blocker for Phase II U-Boot porting.

## Prerequisites

- [x] boot0.bin extracted from ROM (Task 001 completed)
- [x] Nix development environment with ARM tools
- [x] Cross-compilation toolchain (aarch64-unknown-linux-gnu-*)
- [ ] ARM disassembly tools (objdump, hexdump)
- [ ] Understanding of Allwinner DRAM parameter format

## Acceptance Criteria

- [x] Disassemble boot0.bin to identify DRAM initialization code
- [x] Extract CONFIG_DRAM_CLK frequency value (640 MHz)
- [x] Extract CONFIG_DRAM_TYPE (DDR3 = 3)
- [x] Extract CONFIG_DRAM_ZQ calibration values (0x7b7bfb)
- [x] Extract CONFIG_DRAM_ODT_EN settings (1 = enabled)
- [x] Extract timing parameters (TPR0-TPR4, EMR1, DRV)
- [x] Document all extracted parameters in analysis report
- [x] Create preliminary U-Boot defconfig with DRAM settings
- [x] Validate parameters against known H713 specifications

## Technical Background

### DRAM Parameter Challenge
The Allwinner H713 requires specific DRAM timing parameters for proper memory initialization. These parameters are embedded in the proprietary boot0.bin bootloader and must be extracted for U-Boot configuration.

### Known Parameter Categories
- **Clock Settings:** DRAM frequency and PLL configuration
- **Memory Type:** DDR3/DDR4/LPDDR variant detection
- **Timing Parameters:** CAS latency, RAS timing, refresh rates
- **Calibration Values:** ZQ calibration and ODT settings
- **Physical Layout:** Memory size, organization, and topology

## Implementation Approach

### Phase 1: Binary Analysis
1. **Disassemble boot0.bin** using aarch64 objdump
2. **Identify DRAM init functions** in ARM assembly
3. **Locate parameter structures** and constants
4. **Extract hex values** for DRAM configuration

### Phase 2: Parameter Mapping
1. **Map hex values to CONFIG_DRAM_*** names**
2. **Cross-reference with Allwinner documentation**
3. **Validate against H713 memory controller specs**
4. **Document parameter meanings and ranges**

### Phase 3: U-Boot Integration
1. **Create H713 defconfig file** with extracted parameters
2. **Configure U-Boot build system** for sun50i platform
3. **Prepare for compilation testing** (without hardware)
4. **Document build configuration process**

## Risk Assessment

### Technical Risks
- **Binary Analysis Complexity:** ARM assembly analysis requires expertise
- **Parameter Format:** Proprietary Allwinner format may be undocumented
- **Value Validation:** Incorrect parameters could cause hardware damage
- **Multiple Variants:** Different memory configurations may exist

### Mitigation Strategies
- **Conservative Approach:** Document all findings before implementation
- **Cross-Reference:** Validate against existing H713 implementations
- **Safe Testing:** Use FEL mode for any hardware testing
- **Expert Resources:** Leverage Allwinner community knowledge

## Expected Deliverables

### Analysis Documentation
- **DRAM_ANALYSIS.md** - Complete parameter extraction report
- **Parameter mapping table** - Hex values to CONFIG names
- **Technical methodology** - Process for future reference

### Configuration Files
- **sun50i-h713-hy300_defconfig** - U-Boot configuration
- **DRAM parameter definitions** - CONFIG_DRAM_* values
- **Build documentation** - Compilation instructions

### Validation Reports
- **Parameter verification** - Cross-check with specifications
- **Range validation** - Ensure values within safe limits
- **Configuration testing** - Build system verification

## Success Metrics

### Technical Validation
- [ ] All critical DRAM parameters extracted and documented
- [ ] Parameter values validated against H713 specifications
- [ ] U-Boot defconfig created and verified for compilation
- [ ] Analysis methodology documented for reproducibility

### Project Progress
- [ ] Phase II U-Boot porting unblocked
- [ ] Foundation established for bootloader compilation
- [ ] Safe hardware testing approach documented
- [ ] Next phase dependencies clearly defined

## Dependencies and Blockers

### Current Dependencies
- boot0.bin available (✅ satisfied by Task 001)
- ARM analysis tools in development environment
- Understanding of Allwinner DRAM parameter format

### Potential Blockers
- **Encrypted/Obfuscated Code:** boot0.bin may use code protection
- **Hardware Variants:** Multiple DRAM configurations may exist
- **Documentation Gaps:** Limited H713 DRAM documentation available
- **Tool Limitations:** Disassembly tools may need additional configuration

### Next Task Dependencies
- **Task 003:** U-Boot compilation (depends on DRAM parameters)
- **Task 004:** FEL mode testing (depends on bootloader binary)
- **Task 005:** Hardware access setup (depends on safe testing procedures)

## Quality Assurance

### Verification Process
1. **Cross-check extracted values** against multiple sources
2. **Validate parameter ranges** for safety
3. **Document analysis methodology** for peer review
4. **Test U-Boot configuration** for compilation errors

### Documentation Standards
- **Complete technical analysis** with step-by-step process
- **Parameter mapping tables** with hex and CONFIG values
- **Risk assessment** for each extracted parameter
- **Future reference guide** for similar analysis tasks

## Notes

This task represents the critical path for the entire U-Boot porting effort. Success here unblocks Phase II and enables hardware testing. The analysis must be thorough and conservative to prevent hardware damage during subsequent testing phases.

**Critical Path Impact:** High - This task blocks all subsequent U-Boot development work.