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

### Phase III: Additional Firmware Analysis 🔄 IN PROGRESS
- [x] Complete DTB analysis and documentation
- [ ] Extract display.bin (MIPS co-processor firmware)
- [ ] Extract kernel and initramfs from Android partitions
- [ ] Analyze hardware-specific kernel modules
- [ ] Document additional drivers needed for mainline

### Phase IV: Kernel Boot ⏳ PENDING
- [ ] Device tree creation for mainline Linux
- [ ] Basic kernel boot with hardware support
- [ ] Serial output verification

### Phase V: Driver Enablement ⏳ PENDING
- [ ] Display/GPU drivers (Mali-Midgard)
- [ ] MIPS co-processor integration
- [ ] WiFi module support (AW869A/AIC8800)
- [ ] Audio/projection hardware
- [ ] Input devices

## Current Status

**Phase I & II Complete:** Firmware analysis and U-Boot porting completed successfully. Ready for hardware testing.

**Phase III In Progress:** Additional firmware component extraction and analysis underway.

**Next Priority:** Extract display.bin MIPS firmware, kernel, initramfs, and analyze additional hardware-specific components.

### Key Documents
- `firmware/ROM_ANALYSIS.md` - Complete ROM analysis results
- `docs/FACTORY_DTB_ANALYSIS.md` - Detailed DTB hardware analysis
- `docs/DTB_ANALYSIS_COMPARISON.md` - Error corrections from previous analysis
- `firmware/boot0.bin` - Extracted bootloader with DRAM parameters
- `docs/tasks/` - Detailed task tracking and project history
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

### Configuration Used
- Platform: sun50i-H6 (compatible with H713)
- DRAM: DDR3-1600 configuration extracted from boot0.bin
- Device Tree: Custom HY300 configuration based on H6 reference

## Known Challenges

- **Hardware Access:** Physical serial console and FEL mode access needed for testing
- **ARM Trusted Firmware:** BL31 missing (but not required for basic boot)
- **Proprietary Blobs:** Some advanced features may require reverse engineering

## Contributing

This is a hardware porting project. All development should happen through the Nix environment and follow the established task management system.

See `docs/instructions/README.md` for detailed development guidelines.