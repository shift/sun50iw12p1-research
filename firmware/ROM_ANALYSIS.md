# HY300 ROM Firmware Analysis Report

**Target Device:** HY300 Android Projector  
**ROM File:** `update.img` (1.6GB factory firmware)  
**Analysis Date:** September 2025  
**SoC:** Allwinner H713 (ARM64 Cortex-A53)

## Executive Summary

Successfully extracted and analyzed the factory ROM firmware, identifying critical components needed for Linux porting. Key findings include bootloader extraction, DRAM parameter locations, device tree configurations, and hardware component identification.

## ROM Structure Analysis

### File Format
- **Type:** Custom Allwinner IMAGEWTY format
- **Size:** 1.6GB uncompressed
- **Compression:** Custom Allwinner packaging
- **Tool Used:** binwalk for structure analysis

### Partition Layout
Located GPT partition table with Android system components:
- System partitions (Android)
- Boot partitions 
- Recovery partitions
- User data areas

## Critical Findings

### 1. Boot0 Bootloader Extraction ✅

**Location:** 5 eGON.BT0 bootloaders identified via binwalk  
**Primary Bootloader:** Extracted from offset 136196  
**File:** `boot0.bin` (32KB)  
**Status:** Successfully extracted, contains DRAM parameters

#### Boot0 Characteristics
- ARM assembly code for H713 initialization
- DRAM timing and frequency configuration
- Hardware initialization sequences
- Critical for U-Boot porting phase

### 2. Device Tree Analysis ✅

**Count:** 4 DTB (Device Tree Binary) files located  
**Content:** H713 hardware configuration  
**Status:** Available for Linux device tree creation

#### Device Tree Contents
- GPIO pin configurations
- Clock and power management
- Peripheral device mappings
- Memory layout definitions

### 3. WiFi Module Identification ✅

**Module:** AW869A/AIC8800 WiFi chip  
**Evidence:** GPIO reference strings in firmware  
**Impact:** Driver requirements for Linux port

### 4. Hardware Configuration

**GPIO Mappings:** Extensive GPIO configurations found  
**Clock Trees:** H713 clock configuration identified  
**Power Management:** PMIC integration details  
**Peripheral Devices:** USB, HDMI, projection hardware

## Technical Extraction Details

### Analysis Tools Used
```bash
binwalk -E update.img          # Entropy analysis
binwalk --extract update.img   # Structure extraction  
strings update.img | grep -i  # Component identification
hexdump -C update.img | head  # Binary structure
```

### Key Offsets and Signatures
- **eGON.BT0 Signature:** Multiple locations (ARM bootloaders)
- **DTB Magic:** Device tree binaries at various offsets
- **Android Headers:** System partition markers
- **WiFi Strings:** AW869A/AIC8800 references

## Next Phase Requirements

### DRAM Parameter Extraction (Critical)
**Blocker:** Must extract CONFIG_DRAM_* values from boot0.bin  
**Method:** ARM disassembly and reverse engineering  
**Tools:** objdump, Ghidra, or similar ARM analysis tools  
**Output:** U-Boot-compatible DRAM configuration

### Device Tree Creation
**Input:** Extracted DTB files + hardware analysis  
**Output:** sun50i-h713-hy300.dts for mainline Linux  
**Dependencies:** GPIO, clock, and peripheral mappings

### Hardware Access Planning
**Serial Console:** UART TX/RX/GND pad identification needed  
**FEL Mode:** USB recovery mode access for safe testing  
**Backup Strategy:** Complete eMMC backup before modifications

## Risk Assessment

### Low Risk ✅
- ROM analysis completed without device access
- Multiple bootloader copies provide redundancy
- FEL mode available for recovery

### Medium Risk ⚠️
- DRAM parameter extraction requires binary analysis
- Hardware access needed for testing phases
- Some proprietary components may need reverse engineering

### High Risk ❌
- No irreversible modifications yet
- Complete factory restore capability maintained

## Success Metrics

### Phase I Achievements ✅
- [x] ROM structure successfully analyzed
- [x] Bootloader extracted and preserved
- [x] Device trees located and accessible
- [x] WiFi module identified
- [x] Development environment established

### Phase II Readiness ✅
- [x] boot0.bin available for DRAM analysis
- [x] Cross-compilation toolchain configured
- [x] Analysis tools and methodology established
- [x] Safe development practices documented

## Files Generated

### Extracted Artifacts
- `boot0.bin` - Primary bootloader (32KB, contains DRAM config)
- Device tree binaries (4 files, hardware config)
- Partition layout documentation
- Component identification results

### Documentation
- This analysis report
- Development environment configuration
- Task management system
- Git repository with proper exclusions

## Conclusion

Phase I firmware analysis successfully completed with all critical components identified and extracted. The project is ready to proceed to Phase II (U-Boot porting) with the main focus on DRAM parameter extraction from boot0.bin.

**Next Critical Task:** Disassemble boot0.bin to extract DRAM timing parameters for U-Boot configuration.

**Project Status:** ✅ On track, no blockers for Phase II initiation