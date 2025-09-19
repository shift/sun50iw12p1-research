# Task 018: NixOS Kodi Media Player Module for HY300 Projector

## Task Metadata
- **Task ID**: 018
- **Type**: System Integration
- **Priority**: High
- **Phase**: VIII (Complete OS Integration)
- **Status**: pending
- **Created**: 2025-01-18
- **Dependencies**: Tasks 015 (Kernel Module), 016 (Keystone Research)

## Objective
Create a comprehensive NixOS module that builds a complete Kodi-based media player OS image for the HY300 projector, including:
1. **Kodi Media Center**: Optimized for projector use
2. **WiFi Configuration**: IR remote-friendly setup interface
3. **Keystone Correction**: Both automatic and manual adjustment integration
4. **Bluetooth Support**: For headphones, speakers, and external devices
5. **IR Remote Interface**: Complete remote control integration
6. **Projector-Specific Features**: Hardware acceleration and optimization

## Context

### Hardware Platform
Based on comprehensive project analysis:
- **SoC**: Allwinner H713 (ARM64 Cortex-A53) with H6 compatibility
- **GPU**: Mali-Midgard with dedicated display processing
- **MIPS Co-processor**: 40MB dedicated display engine for keystone correction
- **Network**: AIC8800 WiFi module (802.11ac)
- **Input**: IR receiver for remote control
- **Storage**: eMMC with custom partition layout
- **Audio**: HDMI audio output + Bluetooth support needed

### Existing Project Assets
- ✅ **U-Boot Bootloader**: Complete with DRAM parameters (`u-boot-sunxi-with-spl.bin`)
- ✅ **Device Tree**: Mainline Linux DTS with all hardware components (`sun50i-h713-hy300.dts`)
- ✅ **Kernel Modules**: MIPS co-processor driver and keystone motor control
- ✅ **Cross-compilation**: Nix-based aarch64 toolchain ready
- ✅ **Hardware Documentation**: Complete component analysis and enablement status

### Keystone Correction Integration
From Task 016 research:
- **Physical Motor**: Single-axis stepper motor for baseline adjustment
- **Digital Processing**: MIPS co-processor performs 4-corner transformation
- **Auto-correction**: Dual accelerometers for orientation detection
- **User Interface**: 4-corner coordinate input system

## Prerequisites

- [x] Task 015: Kernel module development completed
- [x] Task 016: Keystone correction research completed
- [x] U-Boot bootloader built and ready
- [x] Device tree compiled and validated
- [x] Cross-compilation environment functional
- [ ] NixOS aarch64 development environment setup
- [ ] Kodi cross-compilation dependencies identified

## Acceptance Criteria

### Phase 1: NixOS Module Framework
- [ ] **Complete NixOS module structure** with all configuration options
- [ ] **Cross-compilation support** builds successfully for aarch64
- [ ] **U-Boot integration** working with custom bootloader
- [ ] **Device tree integration** using `sun50i-h713-hy300.dts`
- [ ] **Kernel module loading** automatic for HY300 drivers

### Phase 2: Kodi Integration  
- [ ] **Kodi auto-start** boots directly to media center interface
- [ ] **Hardware video acceleration** using Mali GPU (1080p smooth playback)
- [ ] **Custom projector plugins** for keystone and hardware control
- [ ] **Audio output configuration** HDMI audio working
- [ ] **IR remote integration** basic navigation functional

### Phase 3: WiFi and Network
- [ ] **IR remote WiFi setup** complete interface for network configuration
- [ ] **Network profile management** save/load WiFi configurations
- [ ] **Automatic reconnection** stable WiFi with recovery
- [ ] **QR code setup option** for mobile-assisted configuration

### Phase 4: Keystone Correction
- [ ] **Manual 4-corner adjustment** Kodi plugin for precise control
- [ ] **Auto-correction system** accelerometer-based orientation detection
- [ ] **Motor control integration** physical adjustment via stepper motor
- [ ] **MIPS digital correction** real-time image transformation
- [ ] **Correction profile system** save/load keystone settings

### Phase 5: Bluetooth and Audio
- [ ] **Bluetooth device pairing** using IR remote interface
- [ ] **Audio device support** headphones, speakers streaming
- [ ] **Automatic audio routing** HDMI/Bluetooth switching
- [ ] **HID input support** Bluetooth keyboards for text entry

### Phase 6: Complete System Image
- [ ] **Bootable NixOS image** complete OS for HY300 deployment
- [ ] **FEL mode deployment** safe testing and installation
- [ ] **eMMC installation** production deployment support
- [ ] **System optimization** thermal management, power efficiency

## Implementation Steps

### 1. NixOS Module Framework Development
**Objective**: Create foundational module structure

**Files to Create**:
```
nixos/
├── modules/
│   ├── hy300-projector.nix      # Main module
│   ├── hy300-hardware.nix       # Hardware configuration
│   └── hy300-services.nix       # System services
├── packages/
│   ├── hy300-drivers.nix        # Kernel modules
│   ├── hy300-keystone.nix       # Keystone service
│   ├── hy300-wifi-setup.nix     # WiFi configuration
│   └── kodi-hy300-plugins.nix   # Kodi extensions
└── hy300-image.nix              # System image configuration
```

**Integration Points**:
- Cross-compilation support for aarch64
- U-Boot bootloader integration
- Device tree and kernel module loading
- Service configuration and startup

### 2. Kodi Media Center Integration
**Objective**: Optimize Kodi for projector use

**Components**:
- Hardware acceleration configuration (Mali GPU)
- Audio system setup (HDMI + preparation for Bluetooth)
- Auto-start configuration with kiosk mode
- Custom skin optimization for projector display

**Custom Plugins Development**:
- Keystone correction interface
- Hardware status monitoring
- IR remote configuration
- WiFi setup integration

### 3. IR Remote and WiFi Setup System
**Objective**: Create user-friendly network configuration

**IR Remote Protocol**: 
- Reverse engineer HY300 remote control protocol
- Implement LIRC configuration
- Create navigation interface for text entry

**WiFi Setup Interface**:
- Visual network selection with large text
- Virtual keyboard for password entry
- QR code generation for mobile assistance
- Network profile management

### 4. Keystone Correction Integration
**Objective**: Complete keystone correction system

**Integration Components**:
- Kodi plugin for manual adjustment interface
- Systemd service for keystone daemon
- Kernel driver communication (motor + MIPS)
- Auto-correction with accelerometer input

**User Interface**:
- 4-corner coordinate adjustment
- Test pattern overlays
- Preset profile management
- Real-time adjustment preview

### 5. Bluetooth and Audio System
**Objective**: Complete audio device support

**Audio Pipeline**:
- PipeWire/PulseAudio configuration
- Bluetooth A2DP streaming
- Automatic device switching
- Audio latency optimization

**Bluetooth Management**:
- Device discovery and pairing
- Profile management and auto-reconnection
- HID input device support
- Integration with Kodi interface

### 6. System Image Building and Testing
**Objective**: Create complete deployable system

**Build Configuration**:
- NixOS system image with all components
- Cross-compilation for ARM64
- Partition layout for eMMC deployment
- Boot configuration with U-Boot

**Testing Strategy**:
- Mock interfaces for development without hardware
- FEL mode testing for safe hardware validation
- Performance optimization and thermal management
- User experience testing with IR remote

## Quality Validation

### Software Testing
- [ ] **Module compilation** - All NixOS modules build without errors
- [ ] **Cross-compilation** - aarch64 builds complete successfully  
- [ ] **Service integration** - All systemd services start correctly
- [ ] **Dependency resolution** - All package dependencies satisfied
- [ ] **Configuration validation** - Module options work as expected

### Integration Testing  
- [ ] **Boot sequence** - System boots to Kodi automatically
- [ ] **Hardware driver loading** - All kernel modules load correctly
- [ ] **Service communication** - Inter-service communication functional
- [ ] **Plugin integration** - Kodi plugins communicate with system services
- [ ] **Configuration persistence** - Settings saved and restored correctly

### Hardware Testing (Future Phase)
- [ ] **FEL mode deployment** - Safe installation via USB recovery
- [ ] **Hardware functionality** - All drivers and hardware working
- [ ] **Performance validation** - Smooth video playback and system response
- [ ] **Thermal management** - System operates within temperature limits
- [ ] **Long-term stability** - Extended operation without issues

### User Experience Testing
- [ ] **IR remote workflow** - Complete setup using only remote control
- [ ] **WiFi configuration** - Network setup and profile management
- [ ] **Keystone adjustment** - Manual and automatic correction working
- [ ] **Bluetooth pairing** - Audio device setup and operation
- [ ] **Media playback** - Common formats and use cases working

## Development Approach

### Software-First Development (Current Phase)
**Advantages**: Can be completed without hardware access

1. **Mock Hardware Interfaces**: Create abstraction layer for hardware
2. **Cross-compilation Testing**: Validate builds on development machine
3. **Component Testing**: Test each module independently
4. **Integration Simulation**: Use virtualization for system testing

### Hardware Validation Phase (Future)
**Requirements**: Access to HY300 hardware

1. **FEL Mode Testing**: Safe deployment using USB recovery
2. **Incremental Validation**: Test components individually
3. **Performance Optimization**: Hardware-specific tuning
4. **User Experience Validation**: Real-world usage testing

### Risk Mitigation
1. **Modular Design**: Each component independently testable
2. **Hardware Abstraction**: Clean interfaces for mock testing
3. **FEL Recovery**: Safe hardware testing with fallback
4. **Incremental Deployment**: Gradual feature integration

## External Resource Integration

### Kodi Ecosystem Research
- **Keystone Plugins**: Existing Kodi keystone correction implementations
- **IR Remote Protocols**: LIRC configurations for media center remotes
- **Hardware Acceleration**: Mali GPU optimization for video playback
- **Audio Systems**: Bluetooth integration patterns for media centers

### NixOS Community Resources  
- **ARM64 Deployments**: Existing NixOS embedded system patterns
- **Cross-compilation**: Best practices for aarch64 builds
- **Media Center Modules**: Home theater NixOS configurations
- **Hardware Integration**: GPIO, I2C, and custom driver examples

### Hardware Driver Resources
- **Mali GPU Drivers**: Panfrost vs proprietary driver options
- **Bluetooth Audio**: A2DP streaming configuration and optimization
- **IR Receivers**: LIRC protocol development and testing
- **Accelerometer Integration**: Motion detection and calibration algorithms

## Next Task Dependencies

### Immediate Follow-up Tasks
- **019**: Hardware testing and validation with physical HY300 device
- **020**: Performance optimization and thermal management
- **021**: User experience testing and refinement

### Future Enhancement Tasks
- **Mobile App Development**: Smartphone companion for advanced control
- **Screen Mirroring**: Wireless display capabilities
- **Voice Control**: Integration with voice assistants
- **Smart Home Integration**: IoT device control features

## Integration with Existing Project

### Build System Updates
```nix
# flake.nix additions
{
  outputs = { self, nixpkgs }: {
    nixosConfigurations.hy300-projector = nixpkgs.lib.nixosSystem {
      system = "aarch64-linux";
      modules = [ ./nixos/hy300-image.nix ];
    };
    
    packages.aarch64-linux.hy300-image = 
      self.nixosConfigurations.hy300-projector.config.system.build.sdImage;
  };
}
```

### Documentation Updates
- **README.md**: Add NixOS media player instructions
- **PROJECT_OVERVIEW.md**: Update with Phase VIII completion
- **User Guide**: Complete setup and operation manual
- **Developer Guide**: Module customization and development

## Notes

### Project Significance
This task represents the **culmination of the entire HY300 Linux porting project**:
- Integrates all previous hardware analysis and driver development
- Creates a complete, user-ready operating system
- Demonstrates successful mainline Linux porting to proprietary hardware
- Provides foundation for future projector hardware projects

### Technical Innovation
- **IR Remote Interface**: Unique approach to headless device configuration
- **Hybrid Keystone Correction**: Physical motor + digital transformation
- **Embedded Media Center**: Complete NixOS-based entertainment system
- **Cross-platform Development**: Software-first approach with hardware validation

### Real-world Application
- **Home Theater Integration**: Professional-quality projector media center
- **Educational Use**: Portable presentation and teaching tool
- **Commercial Deployment**: Digital signage and presentation systems
- **Open Source Achievement**: Complete proprietary hardware liberation

### Development Methodology
- **Comprehensive Planning**: Detailed implementation roadmap
- **Risk Mitigation**: Safe development without hardware dependency
- **Quality Focus**: No shortcuts, complete implementation required
- **Documentation Excellence**: Complete user and developer resources

---

**Status**: 🔄 **Ready to Start** - Comprehensive planning complete, implementation roadmap defined
**Priority**: High - Represents final project integration milestone
**Complexity**: High - Multi-component system integration with hardware dependencies
**Timeline**: Estimated 2-3 weeks for software development, additional time for hardware testing
