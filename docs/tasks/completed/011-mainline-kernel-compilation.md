# Task 011: Mainline Linux Kernel Compilation

## Task Overview
**Objective:** Compile mainline Linux kernel with our H713 device tree and driver configuration, creating a complete bootable kernel ready for hardware testing.

**Status:** completed  
**Priority:** high  
**Phase:** V.5 (Software Preparation - No Hardware Required)  
**Dependencies:** Phase V completion (Task 009)

## Background
With comprehensive driver analysis complete and device tree created, we can now compile a mainline Linux kernel that incorporates our research findings. This provides a complete software stack ready for FEL mode testing when hardware access becomes available.

## Success Criteria
- [ ] Download and configure mainline Linux kernel source
- [ ] Configure kernel for Allwinner H713/H6 with our hardware requirements
- [ ] Integrate our device tree into kernel build system
- [ ] Enable drivers based on Phase V research findings
- [ ] Compile complete kernel image (Image, dtb, modules)
- [ ] Validate kernel configuration against our hardware analysis
- [ ] Create kernel ready for FEL mode testing

## Implementation Plan

### Sub-task 1: Kernel Source Setup
**Goal:** Download and prepare mainline kernel for H713 configuration
- Download latest stable mainline kernel (6.1+ LTS or 6.6+ LTS)
- Verify kernel has Allwinner H6 support (our H713 compatibility base)
- Set up kernel build environment in Nix devShell
- Configure cross-compilation for aarch64

### Sub-task 2: Device Tree Integration
**Goal:** Integrate our device tree into kernel build system
- Add `sun50i-h713-hy300.dts` to kernel device tree sources
- Update Makefile to include our device tree in build
- Verify device tree compiles correctly with kernel headers
- Test device tree validation against kernel bindings

### Sub-task 3: Kernel Configuration for H713
**Goal:** Configure kernel with appropriate drivers and features
Based on Phase V driver research, enable:
- **Core Platform:** Allwinner H6 platform support (sun50i)
- **GPU:** Mali-Midgard support (Panfrost driver)
- **WiFi:** SDIO subsystem + community AIC8800 driver preparation
- **MIPS:** Firmware loading support for display co-processor
- **Projector Hardware:** I2C, SPI, GPIO drivers for sensors/motors
- **Standard Peripherals:** UART, MMC, USB, audio subsystem

### Sub-task 4: Driver Configuration Optimization
**Goal:** Configure kernel based on our hardware analysis
- Enable only required drivers to minimize kernel size
- Configure power management for projector use case
- Set up module loading for AIC8800 WiFi (if available)
- Configure Mali GPU memory management
- Set appropriate DMA and CMA settings for video processing

### Sub-task 5: Kernel Compilation
**Goal:** Build complete kernel image and modules
- Compile kernel with our device tree included
- Build kernel modules for peripheral drivers
- Create compressed kernel image (Image.gz)
- Verify all components build without errors
- Generate complete kernel package ready for testing

### Sub-task 6: Validation and Testing Preparation
**Goal:** Validate kernel build and prepare for hardware testing
- Verify kernel size and module count are reasonable
- Check device tree integration is correct in built kernel
- Create FEL mode testing scripts for kernel loading
- Document kernel boot command line parameters
- Prepare kernel for safe FEL mode testing

## Technical Implementation

### Kernel Configuration Highlights
```bash
# Core platform support
CONFIG_ARCH_SUNXI=y
CONFIG_MACH_SUN50I=y

# Mali GPU (Panfrost)
CONFIG_DRM_PANFROST=m

# WiFi subsystem
CONFIG_CFG80211=m
CONFIG_MAC80211=m
CONFIG_MMC=y
CONFIG_MMC_SUNXI=y

# Power management
CONFIG_CPUFREQ=y
CONFIG_CPU_FREQ_GOV_ONDEMAND=y

# Projector hardware
CONFIG_I2C=y
CONFIG_I2C_SUN6I_P2WI=y
CONFIG_SPI=y
CONFIG_SPI_SUN6I=y
```

### Build Process
1. Configure kernel with `make ARCH=arm64 defconfig`
2. Customize configuration with `make ARCH=arm64 menuconfig`
3. Add our device tree to appropriate Makefile
4. Build with `make ARCH=arm64 CROSS_COMPILE=aarch64-unknown-linux-gnu-`
5. Verify build produces complete kernel image

## Quality Validation
- [ ] Kernel builds without errors or warnings
- [ ] Device tree integration verified
- [ ] Kernel configuration reviewed against hardware requirements
- [ ] Module dependencies satisfied
- [ ] FEL testing preparation completed

## Expected Outcomes
- Complete mainline kernel ready for H713 hardware
- Kernel image optimized for projector hardware
- All drivers configured based on comprehensive analysis
- Ready for immediate hardware testing when access becomes available
- Reduced hardware testing time through thorough software preparation

## Files Created
- `linux/` - Kernel source directory
- `linux-hy300-config` - Our kernel configuration
- `Image` - Compiled kernel image
- `sun50i-h713-hy300.dtb` - Integrated device tree
- `modules/` - Compiled kernel modules
- FEL testing scripts

## Risk Assessment
- **Low Risk:** Pure software compilation task
- **No Hardware Required:** Complete software validation possible
- **High Value:** Critical component for hardware testing phase

## Next Steps After Completion
- Ready for Task 010: Hardware testing with complete software stack
- FEL mode validation of U-Boot + device tree + kernel
- Driver testing with compiled kernel modules

## Time Estimate
2-3 hours for kernel download, configuration, and compilation

## Documentation Updates Required
After completion, update:
- README.md with kernel compilation status
- PROJECT_OVERVIEW.md with software stack completion
- Task 010 dependencies (hardware testing now has complete software stack)