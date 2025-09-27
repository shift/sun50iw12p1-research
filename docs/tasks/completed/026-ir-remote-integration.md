# Task 026: IR Remote Integration

## Status
- **Current Status**: pending
- **Priority**: high
- **Estimated Effort**: 2-3 days
- **Dependencies**: U-Boot eMMC configuration (Task 019)

## Objective
Implement complete IR remote control integration including U-Boot early initialization, kernel driver integration, and Kodi remote control functionality.

## Background
Factory firmware analysis confirms IR remote support (`irkey_used = 1` in FEX files) with Consumer IR (CIR) protocol. This task implements the complete IR remote control chain for user interaction with the projector.

## Success Criteria
- [ ] U-Boot IR commands functional for boot menu navigation
- [ ] Linux kernel IR subsystem integrated and functional
- [ ] LIRC daemon configured for remote protocol decoding
- [ ] Kodi remote keymap configured and responsive
- [ ] Remote control tested with all major functions
- [ ] IR receiver device tree node properly configured

## Implementation Steps

### Phase 1: U-Boot IR Support
1. **U-Boot IR Command Integration**
   - Add IR input support to U-Boot configuration
   - Configure IR receiver GPIO pin (CIR_RX)
   - Implement basic IR command handling for boot menu
   - Test IR functionality during boot sequence

2. **Boot Menu Navigation**
   - Configure U-Boot boot menu with IR navigation
   - Map basic remote keys (up, down, select, back)
   - Test boot menu control via IR remote
   - Document IR boot procedures

### Phase 2: Kernel Driver Integration
1. **IR Receiver Device Tree Configuration**
   ```dts
   ir: ir-receiver@5000000 {
       compatible = "allwinner,sun50i-h6-ir", "allwinner,sun6i-a31-ir";
       reg = <0x5000000 0x400>;
       clocks = <&r_ccu CLK_R_APB1_IR>, <&r_ccu CLK_IR>;
       clock-names = "apb", "ir";
       resets = <&r_ccu RST_R_APB1_IR>;
       interrupts = <GIC_SPI 37 IRQ_TYPE_LEVEL_HIGH>;
       pinctrl-names = "default";
       pinctrl-0 = <&r_ir_rx_pin>;
       status = "okay";
   };
   ```

2. **Linux IR Subsystem Integration**
   - Enable Consumer IR support in kernel configuration
   - Configure Allwinner IR driver for H713
   - Test IR signal reception and decoding
   - Validate IR protocol detection

### Phase 3: LIRC Configuration
1. **LIRC Daemon Setup**
   ```conf
   # /etc/lirc/lirc_options.conf
   [lircd]
   nodaemon        = False
   driver          = devinput
   device          = /dev/lirc0
   output          = /var/run/lirc/lircd
   pidfile         = /var/run/lirc/lircd.pid
   plugindir       = /usr/lib/x86_64-linux-gnu/lirc/plugins
   permission      = 666
   allow-simulate  = No
   repeat-max      = 600
   ```

2. **Remote Control Configuration**
   - Configure LIRC for HY300 remote protocol
   - Map remote key codes to Linux input events
   - Test remote key detection and mapping
   - Create remote control profile for common functions

### Phase 4: Kodi Integration
1. **Kodi Remote Keymap Configuration**
   ```xml
   <!-- /storage/.kodi/userdata/keymaps/remote.xml -->
   <keymap>
     <global>
       <remote>
         <power>XBMC.Quit()</power>
         <menu>ActivateWindow(Home)</menu>
         <up>Up</up>
         <down>Down</down>
         <left>Left</left>
         <right>Right</right>
         <select>Select</select>
         <back>Back</back>
         <info>Info</info>
       </remote>
     </global>
   </keymap>
   ```

2. **Remote Control Service Integration**
   - Integrate IR control with HY300 services
   - Map projector-specific functions (keystone, brightness)
   - Test remote control responsiveness
   - Configure power management via remote

## Technical Requirements

### Hardware Specifications
- **IR Receiver**: Consumer IR (CIR) protocol support
- **GPIO Pin**: Dedicated CIR_RX pin configuration
- **Protocol**: NEC or RC5 IR protocol (determined from factory analysis)
- **Range**: Standard IR remote control range (~10 meters)

### Software Dependencies
- **U-Boot**: IR input command support
- **Kernel**: Allwinner IR driver and Linux IR subsystem
- **LIRC**: Remote control daemon and protocol decoding
- **Kodi**: Remote keymap configuration and input handling

### Integration Points
- **Device Tree**: IR receiver hardware configuration
- **Service Integration**: Remote control for HY300 services
- **Power Management**: Remote power control integration
- **User Interface**: Seamless remote control experience

## Testing Procedures

### Phase 1 Testing: U-Boot IR
1. **FEL Mode Testing**
   ```bash
   # Load U-Boot with IR support via FEL
   sunxi-fel -p uboot u-boot-sunxi-with-spl.bin
   # Test IR boot menu navigation via serial console
   ```

2. **Boot Menu Validation**
   - Test IR remote responsiveness during boot
   - Validate boot menu navigation
   - Check IR command recognition

### Phase 2 Testing: Kernel Integration
1. **IR Hardware Detection**
   ```bash
   # Check IR device detection
   dmesg | grep -i ir
   ls /dev/lirc*
   cat /proc/interrupts | grep ir
   ```

2. **Signal Reception Testing**
   ```bash
   # Test raw IR signal reception
   ir-ctl -r /dev/lirc0
   # Test IR protocol decoding
   irw /var/run/lirc/lircd
   ```

### Phase 3 Testing: LIRC Integration
1. **LIRC Service Testing**
   ```bash
   # Test LIRC daemon functionality
   systemctl status lircd
   # Test remote key mapping
   irw | head -20
   ```

2. **Input Event Testing**
   ```bash
   # Test Linux input events
   evtest /dev/input/eventX
   # Monitor input events from IR
   ```

### Phase 4 Testing: Kodi Integration
1. **Remote Control Validation**
   - Test all remote control functions in Kodi
   - Validate navigation responsiveness
   - Check projector-specific function mapping
   - Test power management integration

2. **User Experience Testing**
   - Remote control range testing
   - Button response time validation
   - Menu navigation smoothness
   - Overall user experience evaluation

## Quality Assurance

### Integration Validation
- **Complete Implementation**: No partial or mock IR functionality
- **Hardware Safety**: Safe GPIO configuration and power management
- **User Experience**: Responsive and intuitive remote control
- **Service Integration**: Seamless integration with HY300 services

### Performance Requirements
- **Response Time**: < 100ms from remote button press to action
- **Range**: Minimum 5 meters effective range
- **Reliability**: 99%+ button press recognition rate
- **Power Efficiency**: Minimal power consumption impact

## Risk Assessment

### Technical Risks
- **Hardware Compatibility**: IR receiver protocol compatibility
- **Timing Issues**: IR signal timing and kernel interrupt handling
- **Integration Complexity**: Multiple system layer integration
- **User Experience**: Remote responsiveness and reliability

### Mitigation Strategies
- **Factory Analysis**: Use factory IR configuration as baseline
- **Incremental Testing**: Test each integration layer independently
- **FEL Mode Safety**: All testing via FEL mode first
- **Documentation**: Complete documentation for troubleshooting

## Success Metrics
- **Functionality**: All remote control functions working correctly
- **Performance**: Responsive remote control with < 100ms latency
- **Reliability**: Consistent remote control operation
- **Integration**: Seamless integration with Kodi and HY300 services
- **Documentation**: Complete IR integration documentation

## Next Steps After Completion
- **WiFi Driver Integration** (Task 021): Network connectivity implementation
- **Mali GPU Driver Selection** (Task 022): Graphics acceleration decision
- **Hardware Testing Preparation** (Task 025): FEL mode testing preparation

This task establishes complete IR remote control functionality as the primary user interface for the HY300 projector system.