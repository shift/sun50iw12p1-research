# HY300 Android Projector Linux Porting Project

Porting mainline Linux to the HY300 consumer projector with Allwinner H713 SoC (ARM64 Cortex-A53, Mali-Midgard GPU).

## Hardware Overview

- **SoC:** Allwinner H713 (ARM64 Cortex-A53, Mali-Midgard GPU)
- **Target Device:** HY300 consumer projector
- **Architecture:** aarch64 (64-bit ARM)
- **Platform Family:** sun50i (Allwinner H-series)

## Development Environment

This project uses Nix flakes for reproducible development environments:

```bash
# Enter development shell
nix develop

# Or use direnv (recommended)
direnv allow
```

### Available Tools
- aarch64 cross-compilation toolchain
- sunxi-tools for FEL mode operations
- Firmware analysis tools (binwalk, hexdump, strings)
- Serial console tools (minicom, picocom)
- Device tree compiler and Android tools

## Project Phases

### Phase I: Firmware Analysis ✅ COMPLETED
- [x] Extract and analyze factory ROM (`update.img`)
- [x] Identify bootloader and DRAM parameters
- [x] Extract device trees and partition layout
- [x] Document hardware configuration
- [x] Analyze factory DTB files for hardware mapping
- [x] Correct previous analysis errors (GPU identification, etc.)

### Phase II: U-Boot Porting ✅ COMPLETED
- [x] Extract DRAM parameters from boot0.bin
- [x] Configure U-Boot for H713 (using H6 base)
- [x] Build SPL and U-Boot binaries
- [x] Integrate custom DRAM parameters
- [ ] Test FEL mode boot (requires hardware access)
- [ ] Serial console access (requires hardware access)

### Phase III: Additional Firmware Analysis ✅ COMPLETED
- [x] Complete DTB analysis and documentation
- [x] Extract display.bin (MIPS co-processor firmware)
- [x] Extract kernel and initramfs from Android partitions
- [x] Analyze hardware-specific kernel modules
- [x] Document additional drivers needed for mainline
- [x] Complete MIPS co-processor integration analysis

### Phase IV: Mainline Device Tree Creation ✅ COMPLETED
- [x] Device tree creation for mainline Linux (`sun50i-h713-hy300.dts`)
- [x] All hardware components configured and documented
- [x] MIPS co-processor integration with memory regions
- [x] Mali-Midgard GPU support with operating points
- [x] Projector-specific hardware configuration
- [x] Device tree compilation verification (10.5KB DTB)
- [x] Safe testing methodology with FEL recovery

### Phase VI: Extended Research and Analysis ✅ COMPLETED
- [x] Comprehensive WiFi driver analysis (AIC8800 with 3 implementations)
- [x] Mali GPU driver research (Panfrost vs proprietary options)
- [x] MIPS co-processor firmware integration methodology
- [x] Android kernel driver pattern analysis and documentation
- [x] Projector-specific hardware driver requirements
- [x] Complete driver integration roadmap with priorities
- [x] Bootloader and MIPS co-processor complete analysis
- [x] TSE database and panel configuration extraction

### Phase VII: Kernel Module Development 🎯 IN PROGRESS
- [x] Basic kernel module framework (`drivers/misc/sunxi-mipsloader.c`)
- [x] Linux 6.16.7 API compatibility and compilation fixes
- [x] Platform driver with device tree integration design
- [x] Character device interface with IOCTL commands
- [x] 40MB MIPS memory region management (0x4b100000)
- [x] Register interface implementation (0x3061000 base)
- [ ] Device tree binding documentation and integration
- [ ] Module loading and hardware validation testing

### Phase VIII: Hardware Testing and Validation 🎯 NEXT (requires hardware access)
- [ ] Serial console setup and FEL mode recovery validation
- [ ] U-Boot bootloader testing via FEL mode
- [ ] Kernel module loading and MIPS co-processor testing
- [ ] Mainline device tree boot validation
- [ ] Basic kernel functionality with console access
- [ ] Driver testing framework establishment
- [ ] WiFi and GPU driver hardware validation
- [ ] Projector hardware validation (motors, sensors, thermal)
- [ ] Complete hardware enablement testing

## Current Status

**Phases I-V Complete:** All software analysis phases completed successfully - firmware analysis, U-Boot porting, additional firmware analysis, mainline device tree creation, and comprehensive driver integration research.

**Phase VI Ready:** Hardware testing and validation phase ready to begin, requiring serial console access and FEL mode testing capabilities.

**Key Achievements:** 
- Complete mainline Linux device tree (`sun50i-h713-hy300.dts`) with all hardware components
- U-Boot bootloader ready for testing (`u-boot-sunxi-with-spl.bin`)
- Comprehensive driver integration roadmap with multiple implementation options
- Complete firmware component analysis and driver patterns documented

**Next Priority:** Phase V driver integration requires hardware access for FEL mode testing, serial console access, and driver validation.

### Key Documents
- `firmware/ROM_ANALYSIS.md` - Complete ROM analysis results
- `docs/FACTORY_DTB_ANALYSIS.md` - Detailed DTB hardware analysis
- `docs/DTB_ANALYSIS_COMPARISON.md` - Error corrections from previous analysis
- `sun50i-h713-hy300.dts` - **Main deliverable**: Complete mainline device tree
- `drivers/misc/sunxi-mipsloader.c` - **NEW**: MIPS co-processor kernel module (441 lines)
- `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` - Hardware component status matrix
- `docs/HY300_TESTING_METHODOLOGY.md` - Safe testing procedures with FEL recovery
- `docs/HY300_SPECIFIC_HARDWARE.md` - Projector-specific hardware documentation
- `firmware/boot0.bin` - Extracted bootloader with DRAM parameters
- `docs/tasks/completed/` - All completed task documentation
- `flake.nix` - Development environment configuration

## Development Workflow

1. **Task Management:** Create numbered tasks in `docs/tasks/`, move to `completed/` when done
2. **Git Commits:** Reference task numbers, atomic commits with descriptive messages  
3. **Documentation:** Update analysis reports as discoveries are made
4. **Testing:** Use FEL mode for safe bootloader testing

## Hardware Access Requirements

- **Serial Console:** UART access via TX/RX/GND pads
- **FEL Mode:** USB recovery mode for bootloader testing
- **Complete Backup:** Full eMMC backup before modifications

## Build Artifacts

### Successfully Built
- **u-boot-sunxi-with-spl.bin** - Complete U-Boot binary for H713 (657.5 KB)
- **sunxi-spl.bin** - Secondary Program Loader (32 KB)
- **u-boot.bin** - Main U-Boot binary (638 KB)
- **u-boot.dtb** - Device tree blob for HY300 hardware
- **sun50i-h713-hy300.dtb** - **Main deliverable**: Mainline device tree blob (10.5 KB)
- **sunxi-mipsloader.ko** - **NEW**: MIPS co-processor kernel module (compiles cleanly)

### Configuration Used
- Platform: sun50i-H6 (compatible with H713)
- DRAM: DDR3-1600 configuration extracted from boot0.bin
- Device Tree: Complete HY300 mainline configuration with all hardware support
- MIPS: Co-processor integration with 40.3MB memory region

## Known Challenges

- **Hardware Access Required:** Phase V driver integration requires physical serial console and FEL mode access for testing
- **MIPS Co-processor:** Custom driver integration needed for display.bin firmware loading
- **Proprietary Components:** WiFi (AIC8800) and some projection hardware may require reverse engineering
- **Mali-Midgard GPU:** Driver selection between Midgard and Panfrost compatibility

## Contributing

This is a hardware porting project. All development should happen through the Nix environment and follow the established task management system.

See `docs/instructions/README.md` for detailed development guidelines.