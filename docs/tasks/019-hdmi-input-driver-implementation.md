# Task 019: HDMI Input Driver Implementation

**Status:** in_progress  
**Priority:** high  
**Phase:** VII - Kernel Module Development  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Context:** Based on comprehensive analysis from Task 022 - HDMI Input Analysis and Implementation

## Objective

Implement the V4L2 TV capture driver (`sunxi-tvcap`) for HDMI input functionality on the HY300 projector, including device tree integration, MIPS co-processor coordination, and user-space V4L2 API support.

## Prerequisites

- [x] **Task 022**: HDMI Input Analysis and Implementation - COMPLETED
- [x] **Factory TV Capture Analysis**: Complete hardware interface specification from Task 022
- [x] **MIPS Co-processor Analysis**: ARM-MIPS communication protocol documented
- [x] **Device Tree Foundation**: TV capture nodes added to `sun50i-h713-hy300.dts`
- [x] **Android Driver Patterns**: Factory driver interfaces documented and analyzed
- [x] **Development Environment**: Nix devShell with cross-compilation toolchain

## Acceptance Criteria

- [ ] **V4L2 Capture Driver**: Complete `sunxi-tvcap.ko` kernel module implementation
- [ ] **Device Tree Integration**: TV capture hardware properly initialized from device tree
- [ ] **HDMI Input Detection**: Hot-plug detection and EDID reading functional
- [ ] **Video Format Support**: Multiple capture formats (1080p, 720p, YUV/RGB)
- [ ] **MIPS Coordination**: Shared memory communication with MIPS co-processor
- [ ] **Buffer Management**: V4L2 videobuf2 integration with DMA buffers
- [ ] **User-Space API**: Standard V4L2 IOCTL interface for applications
- [ ] **Testing Validation**: Driver loads and initializes without errors
- [ ] **Documentation Complete**: Driver architecture and usage documented

## Implementation Steps

### 1. V4L2 Driver Framework Implementation
**Objective**: Create basic V4L2 capture driver structure
**Atomic Tasks**:
- [x] **1.1**: Create `drivers/media/platform/sunxi/sunxi-tvcap.c` skeleton
- [x] **1.2**: Implement V4L2 device registration and basic IOCTL handlers
- [x] **1.3**: Add device tree probing and hardware resource management
- [x] **1.4**: Implement clock and reset control based on Task 022 analysis
- [x] **1.5**: Add Makefile and Kconfig integration

### 2. Hardware Interface Implementation
**Objective**: Implement direct TV capture hardware control
**Atomic Tasks**:
- [x] **2.1**: Implement `tvtop@5700000` register interface based on factory analysis
- [x] **2.2**: Add TV capture clock management (7 clocks from Task 022)
- [x] **2.3**: Implement reset line control (3 reset lines from Task 022)
- [x] **2.4**: Add interrupt handling for IRQ 110 (shared interrupt)
- [ ] **2.5**: Implement basic hardware initialization sequence

### 3. MIPS Co-processor Integration
**Objective**: Implement ARM-MIPS communication for TV capture coordination
**Status**: ⚠️ **BLOCKED** - Missing critical platform drivers identified
**Atomic Tasks**:
- [ ] **3.1**: Extend `sunxi-mipsloader.ko` with TV capture operations
- [ ] **3.2**: Implement shared memory interface at 0x4ba00000
- [ ] **3.3**: Add MIPS command/response protocol for TV capture
- [ ] **3.4**: Implement HDMI input switching via MIPS coordination
- [ ] **3.5**: Add EDID reading through MIPS communication

**Research Finding**: Missing critical platform drivers required for MIPS integration:
- **SUNXI_NSI** - Network Service Interface for inter-processor communication
- **SUNXI_CPU_COMM** - ARM-MIPS communication framework
- **SUNXI_TVTOP** - Top-level TV subsystem controller
- **SUNXI_TVUTILS** - TV utilities and display processing

### 4. V4L2 Capture Operations  
**Objective**: Implement video capture pipeline with V4L2 API
**Status**: ⚠️ **LIMITED** - Missing display pipeline components
**Atomic Tasks**:
- [ ] **4.1**: Implement videobuf2 queue management for capture buffers
- [ ] **4.2**: Add video format enumeration and negotiation
- [ ] **4.3**: Implement stream start/stop operations (`VIDIOC_STREAMON/OFF`)
- [ ] **4.4**: Add buffer queueing and dequeuing operations
- [ ] **4.5**: Implement frame capture completion and timestamping

**Research Finding**: Display output functionality requires missing drivers:
- **SUNXI_TVUTILS** - Video processing and format conversion
- **SUNXI_TVTOP** - Display subsystem control for output routing
- **Display IOMMU** - Advanced memory management for display pipeline

**Current Limitation**: Can capture HDMI input but cannot display output without complete driver stack

### 5. Input Management and Format Detection
**Objective**: Implement HDMI input detection and format negotiation
**Atomic Tasks**:
- [ ] **5.1**: Implement HDMI hot-plug detection and monitoring
- [ ] **5.2**: Add EDID parsing and display timing detection
- [ ] **5.3**: Implement video format auto-detection (resolution, color space)
- [ ] **5.4**: Add V4L2 input enumeration (`VIDIOC_ENUM_INPUT`)
- [ ] **5.5**: Implement input selection and switching (`VIDIOC_S_INPUT`)

### 6. Testing and Validation Framework
**Objective**: Establish testing procedures for safe driver validation
**Atomic Tasks**:
- [ ] **6.1**: Create driver loading and initialization tests
- [ ] **6.2**: Implement V4L2 compliance testing procedures
- [ ] **6.3**: Add HDMI input detection validation tests
- [ ] **6.4**: Create basic capture functionality tests
- [ ] **6.5**: Document testing procedures and expected results

## Quality Validation

- [ ] **Compilation Success**: Driver compiles without errors or warnings
- [ ] **Module Loading**: Driver loads and initializes hardware correctly
- [ ] **Device Registration**: V4L2 device appears as `/dev/video0`
- [ ] **IOCTL Interface**: Basic V4L2 IOCTLs respond correctly
- [ ] **Hardware Integration**: TV capture hardware initializes successfully
- [ ] **MIPS Communication**: Shared memory interface functions correctly
- [ ] **Code Quality**: Driver follows Linux kernel coding standards
- [ ] **Documentation**: Driver architecture and usage fully documented

## Next Task Dependencies

**Immediate Dependencies** (require Task 019 completion):
- **Task 020**: HDMI Input Hardware Testing and Validation
- **Task 021**: Video Capture Pipeline Performance Optimization
- **Task 024**: MIPS Co-processor Integration Testing

**Related Tasks** (enhanced by Task 019):
- **Task 018**: NixOS Kodi Module (can utilize HDMI input after completion)
- **Task 010**: Phase VI Hardware Testing (HDMI driver testing phase)

## Notes

### **CRITICAL RESEARCH FINDING - Missing Platform Drivers**
**Date:** September 18, 2025  
**Source:** Factory kernel missing components analysis

**10 CRITICAL MISSING DRIVERS** identified that affect Task 019 implementation:

#### **P1 CRITICAL (Display Functionality)**
- **SUNXI_TVTOP** - Top-level TV controller ⚠️ REQUIRED for display output
- **SUNXI_TVUTILS** - Display utilities ⚠️ REQUIRED for video processing  
- **Enhanced SUNXI_TVCAP** - Current driver needs IOMMU integration ⚠️ PARTIAL

#### **P2 HIGH (MIPS Communication)**  
- **SUNXI_NSI** - Network Service Interface ⚠️ REQUIRED for MIPS communication
- **SUNXI_CPU_COMM** - ARM-MIPS communication ⚠️ REQUIRED for co-processor control
- **Display IOMMU** - Display memory management ⚠️ REQUIRED for advanced graphics

#### **P3 MEDIUM (System Integration)**
- **SUNXI_ARISC_RPM** - Runtime power management ⚠️ REQUIRED for system stability
- **SUNXI_ADDR_MGT** - Address management ⚠️ REQUIRED for shared memory
- **SUNXI_RFKILL** - RF kill switch ⚠️ REQUIRED for WiFi power control
- **SUNXI_BOOTEVENT** - Boot event handling ⚠️ REQUIRED for initialization

**Impact on Task 019:**
- **Current Status**: HDMI input capture possible, **NO display output**
- **MIPS Integration**: **BLOCKED** - Cannot communicate with display co-processor
- **Display Pipeline**: **INCOMPLETE** - Missing processing and control layers

**Minimum Viable Solution for Task 019:**
1. **Enhanced SUNXI_TVCAP** (1-2 weeks) - Add IOMMU and display output
2. **SUNXI_TVUTILS** (2-3 weeks) - Basic video processing capability  
3. **SUNXI_TVTOP stub** (1 week) - Minimal display control interface

**Complete Solution Timeline:** 4-6 weeks additional development

**Reference Documents:**
- `docs/FACTORY_KERNEL_MISSING_ANALYSIS.md` - Complete analysis
- `docs/DRIVER_PRIORITY_MATRIX.md` - Implementation priorities  
- `docs/MISSING_COMPONENTS_IMPLEMENTATION_ROADMAP.md` - Detailed timeline
- `docs/INTEGRATION_DEPENDENCIES_ANALYSIS.md` - Task 019 specific impact

### **Implementation Strategy**
- **Phase-based development**: Build incrementally from basic framework to full functionality
- **Safety-first approach**: Extensive software testing before hardware validation
- **Factory analysis foundation**: Leverage comprehensive Task 022 analysis results
- **Mainline compatibility**: Ensure driver meets upstream Linux kernel standards

### **Critical Hardware Dependencies**
Based on Task 022 analysis, the driver requires:
- **7 TV-specific clocks**: `CLK_BUS_TVCAP`, `CLK_CAP_300M`, `CLK_VINCAP_DMA`, etc.
- **3 reset lines**: `RST_BUS_TVCAP`, `RST_BUS_DISP`, `RST_BUS_DEMOD`
- **5 memory regions**: TV system control registers and buffers
- **IRQ 110**: Shared interrupt with decoder subsystem
- **MIPS coordination**: Shared memory at 0x4ba00000 for ARM-MIPS communication

### **Factory Driver Patterns**
From Task 022 Android driver analysis:
- **Device node**: `/dev/disp` with extended IOCTL commands
- **Capture workflow**: Format setup → Buffer init → Start → Acquire → Release → Stop
- **HDMI integration**: HPD detection, EDID reading, format auto-detection
- **Multi-format support**: 56+ pixel formats (RGB, YUV, 10-bit variants)

### **Development Environment**
- **Cross-compilation**: aarch64-unknown-linux-gnu toolchain via Nix
- **Kernel version**: Target Linux 6.16.7 (current mainline compatibility)
- **Testing framework**: V4L2 utilities and compliance testing
- **Documentation**: Driver implementation and usage guides

### **Risk Mitigation**
- **Hardware safety**: All testing via FEL mode with recovery procedures
- **Incremental development**: Each atomic task has validation criteria
- **Factory reference**: Constant validation against working Android implementation
- **Fallback procedures**: Error handling and recovery mechanisms

### **External Resources**
- **V4L2 framework documentation**: https://www.kernel.org/doc/html/latest/media/
- **Allwinner platform drivers**: Reference implementations in mainline kernel
- **Factory analysis results**: Complete hardware specification from Task 022
- **MIPS firmware structure**: Documented communication protocols
