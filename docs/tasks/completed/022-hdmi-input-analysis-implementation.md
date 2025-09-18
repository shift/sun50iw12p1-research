# Task 022: HDMI Input Analysis and Implementation

**Status:** completed  
**Priority:** high  
**Phase:** VI - Hardware Testing and Integration  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Context:** HDMI input capability is missing from mainline device tree and driver stack

## Objective

Analyze and implement HDMI input functionality for the HY300 projector, including TV capture interface configuration, driver development, and MIPS co-processor coordination for seamless input switching and processing.

## Prerequisites

- [x] Factory firmware extracted and analyzed (`firmware/update.img.extracted/`)
- [x] MIPS co-processor firmware analyzed (`firmware/FIRMWARE_COMPONENTS_ANALYSIS.md`)
- [x] Device tree framework established (`sun50i-h713-hy300.dts`)
- [x] Factory DTB files available for reference (`FC00/system.dtb`)
- [x] Development environment configured (Nix devShell)

## Acceptance Criteria

- [ ] **TV Capture Interface Identified**: Complete analysis of `tvcap@6800000` hardware configuration
- [ ] **Factory Driver Analysis**: Understanding of Android TV capture driver implementation
- [ ] **Device Tree Integration**: `tvcap@6800000` properly configured in mainline DTS
- [ ] **MIPS Coordination Protocol**: Understanding of display processing handoff mechanisms
- [ ] **Driver Implementation Plan**: Roadmap for mainline Linux TV capture driver
- [ ] **Testing Framework**: Safe FEL-mode testing procedures for HDMI input validation
- [ ] **Documentation Complete**: Full analysis documented for future development

## Implementation Steps

### 1. Factory TV Capture Configuration Analysis
**Objective**: Extract complete TV capture interface configuration from factory firmware
**Atomic Tasks**:
- [ ] **1.1**: Extract and decompile factory system.dtb (`firmware/update.img.extracted/FC00/system.dtb`)
- [ ] **1.2**: Identify `tvcap@6800000` node configuration and all related properties
- [ ] **1.3**: Document memory ranges, interrupts, clocks, and power domains
- [ ] **1.4**: Compare with existing mainline Allwinner TV capture implementations
- [ ] **1.5**: Create detailed hardware interface specification document

### 2. Factory Android Driver Analysis  
**Objective**: Understand existing HDMI input driver implementation patterns
**Atomic Tasks**:
- [ ] **2.1**: Extract Android kernel TV capture drivers from factory firmware
- [ ] **2.2**: Analyze driver initialization sequences and hardware setup
- [ ] **2.3**: Document IOCTL interfaces and user-space interaction patterns
- [ ] **2.4**: Identify video format support and capture pipeline configuration
- [ ] **2.5**: Map driver dependencies and required kernel subsystems

### 3. MIPS Co-processor Integration Analysis
**Objective**: Understand coordination between TV capture and MIPS display processing
**Atomic Tasks**:
- [ ] **3.1**: Analyze MIPS firmware for TV capture coordination protocols
- [ ] **3.2**: Identify shared memory regions and communication mechanisms
- [ ] **3.3**: Document input source switching and processing handoff procedures
- [ ] **3.4**: Map MIPS display transformation pipeline for captured input
- [ ] **3.5**: Create integration specification for Linux driver coordination

### 4. Mainline Device Tree Integration
**Objective**: Add TV capture interface to mainline device tree
**Atomic Tasks**:
- [ ] **4.1**: Add `tvcap@6800000` node to `sun50i-h713-hy300.dts`
- [ ] **4.2**: Configure memory ranges, interrupts, and clock references
- [ ] **4.3**: Add power domain and reset line configurations
- [ ] **4.4**: Verify device tree compilation and validate against factory config
- [ ] **4.5**: Update hardware enablement status documentation

### 5. Driver Development Planning
**Objective**: Create implementation roadmap for mainline Linux TV capture driver
**Atomic Tasks**:
- [ ] **5.1**: Research existing V4L2 TV capture driver implementations
- [ ] **5.2**: Design driver architecture for Allwinner TV capture integration
- [ ] **5.3**: Plan MIPS co-processor coordination interfaces
- [ ] **5.4**: Create driver development milestones and testing procedures
- [ ] **5.5**: Document driver implementation dependencies and timeline

### 6. Testing Framework Development
**Objective**: Establish safe testing procedures for HDMI input validation
**Atomic Tasks**:
- [ ] **6.1**: Design FEL-mode testing procedures for TV capture drivers
- [ ] **6.2**: Create HDMI input signal testing protocols
- [ ] **6.3**: Plan capture pipeline validation procedures
- [ ] **6.4**: Develop MIPS coordination testing methodology
- [ ] **6.5**: Document hardware safety procedures and recovery protocols

## Quality Validation

- [ ] **Factory Configuration Extraction**: All TV capture interface parameters documented
- [ ] **Driver Analysis Complete**: Android driver patterns fully understood and documented
- [ ] **Device Tree Validation**: TV capture node compiles and validates against factory reference
- [ ] **MIPS Integration Specification**: Complete coordination protocol documented
- [ ] **Testing Framework Ready**: Safe validation procedures established for hardware testing
- [ ] **Documentation Standards**: All analysis follows project documentation standards
- [ ] **Code Quality**: Device tree changes compile without errors or warnings

## Next Task Dependencies

**Immediate Dependencies** (require Task 022 completion):
- **Task 023**: HDMI Input Driver Implementation
- **Task 024**: MIPS Co-processor Integration Testing
- **Task 025**: Video Input Pipeline Validation

**Related Tasks** (can proceed in parallel):
- **Task 018**: NixOS Kodi Module (enhanced with HDMI input after Task 022)
- **Task 010**: Phase VI Hardware Testing (HDMI input testing phase)

## Notes

### **Critical Discovery Context**
Current mainline device tree **missing TV capture interface**:
- ✅ MIPS co-processor implemented (`mips-loader@6000000`)
- ✅ GPU implemented (`gpu@7000000`)  
- ❌ **TV capture hardware (`tvcap@6800000`) - NOT IMPLEMENTED**

### **Hardware Architecture**
```
HDMI Input Signal → TV Capture (tvcap@6800000) → MIPS Processing → Display Output
```

### **Factory Kernel Evidence**
- `CONFIG_SUNXI_TVCAP_TVDISP_IOMMU=y`
- `CONFIG_SUNXI_TVTOP=m`
- Custom capture drivers in Android kernel

### **Development Approach**
- **Software-first analysis**: Maximum extraction from factory firmware before hardware testing
- **Safety protocols**: All testing via FEL mode with recovery procedures
- **Incremental validation**: Each atomic task has specific validation criteria
- **Documentation completeness**: Full specification before driver implementation

### **External Resources Required**
- Factory system.dtb analysis tools
- V4L2 driver development references  
- Allwinner TV capture subsystem documentation
- MIPS firmware analysis utilities

### **Risk Mitigation**
- Complete factory analysis before any hardware modifications
- FEL-mode testing prevents brick risk
- Incremental device tree validation
- MIPS coordination protocols established before driver development