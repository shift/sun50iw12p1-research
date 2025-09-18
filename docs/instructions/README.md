# HY300 Android Projector Linux Porting

## Project Goal
Port mainline Linux to the HY300 consumer projector, replacing the Android-based firmware with a custom Linux distribution optimized for projection hardware.

## Hardware Specifications

### Target Device: HY300 Projector
- **SoC:** Allwinner H713
- **CPU:** ARM64 Cortex-A53 quad-core
- **GPU:** Mali-Midgard (corrected from previous Mali-G31 misidentification) 
- **Architecture:** aarch64 (64-bit ARM)
- **Platform:** sun50i (Allwinner H-series)

### Key Components
- **Projection Engine:** Custom optics hardware
- **WiFi Module:** AW869A/AIC8800 (found in ROM analysis)
- **Storage:** eMMC flash storage
- **Connectivity:** USB, HDMI, WiFi

## Technical Approach

### Phase-Based Development
1. **Firmware Analysis** - Extract and analyze factory ROM
2. **U-Boot Porting** - Custom bootloader for H713
3. **Kernel Boot** - Basic Linux kernel with device tree
4. **Driver Enablement** - Hardware-specific drivers

### Development Strategy
- **Reverse Engineering:** Extract hardware configuration from factory ROM
- **Cross-Compilation:** aarch64 toolchain via Nix
- **Safe Testing:** FEL mode for recovery
- **Incremental Progress:** Boot stages before full system

## Current Progress

### Completed: Phase I - Firmware Analysis ✅
- ✅ Factory ROM (`update.img`) extracted and analyzed
- ✅ Boot0 bootloader extracted with DRAM parameters
- ✅ Device trees and partition layout documented
- ✅ WiFi module and GPIO configuration identified
- ✅ Complete DTB analysis with hardware mapping
- ✅ Error correction of previous analysis (GPU, file formats)

### Completed: Phase II - U-Boot Porting ✅
- ✅ DRAM parameter extraction from boot0.bin
- ✅ U-Boot configuration for H713 (using H6 base)
- ✅ Successfully built bootloader binaries
- ✅ Cross-compilation environment validated

### In Progress: Phase III - Additional Firmware Analysis 🔄
- 🔄 MIPS co-processor firmware (display.bin) extraction
- ⏳ Kernel and initramfs extraction from Android partitions
- ⏳ Hardware-specific driver analysis
- ⏳ Additional component inventory for mainline integration

### Results Documentation
- `firmware/ROM_ANALYSIS.md` - Complete ROM analysis
- `docs/FACTORY_DTB_ANALYSIS.md` - Detailed DTB hardware analysis  
- `docs/DTB_ANALYSIS_COMPARISON.md` - Previous analysis error corrections
- `docs/tasks/completed/` - All completed task documentation

## Technical Challenges

### Primary Challenge: Additional Firmware Components
With U-Boot porting complete, the current focus is extracting and analyzing additional firmware components:
- **MIPS co-processor firmware:** display.bin for projection hardware
- **Kernel analysis:** Android kernel with H713-specific drivers
- **Initramfs extraction:** Hardware initialization and driver loading
- **Module inventory:** Proprietary drivers needed for mainline integration

### Hardware Dependencies
- **Serial Console Access:** Physical UART connection required
- **FEL Mode Access:** USB recovery mode for safe testing
- **Complete Backup:** eMMC backup before modifications

## Development Environment

Uses Nix flakes for reproducible cross-compilation:

```bash
# Enter development shell
nix develop

# Or use direnv
direnv allow
```

### Toolchain Includes
- aarch64-unknown-linux-gnu-gcc
- sunxi-tools (FEL mode utilities)
- Device tree compiler
- Firmware analysis tools
- Serial console tools

## Risk Mitigation

### Safe Development Practices
- **FEL Mode Recovery:** Always test via USB recovery mode first
- **Complete Backups:** Full eMMC backup before any modifications
- **Incremental Testing:** Boot stages verified independently
- **Hardware Preservation:** No irreversible modifications

### Fallback Strategy
- Factory ROM backup for full restore
- FEL mode allows recovery without hardware tools
- Staged approach allows rollback at any phase

## Expected Outcomes

### Successful Port Delivers
- **Custom Linux Distribution:** Optimized for projection hardware
- **Open Source Drivers:** Community-maintainable codebase  
- **Enhanced Features:** Potential improvements over stock Android
- **Development Platform:** Foundation for further hardware projects

### Learning Outcomes
- ARM64 bootloader porting techniques
- Allwinner SoC development experience
- Reverse engineering methodologies
- Embedded Linux system integration

This project represents a comprehensive embedded systems development effort, combining reverse engineering, cross-platform development, and hardware-specific optimization.