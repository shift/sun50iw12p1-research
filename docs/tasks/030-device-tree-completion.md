# Task 030: Device Tree Completion

## Status
- **Current Status**: pending
- **Priority**: medium
- **Estimated Effort**: 2-3 days
- **Dependencies**: Audio/SPDIF Configuration (Task 029)

## Objective
Complete device tree configuration by adding remaining hardware nodes for accelerometer, temperature sensors, additional GPIO configurations, and other hardware components identified through factory firmware analysis.

## Background
Current device tree (`sun50i-h713-hy300.dts`) covers major hardware components but lacks complete coverage of all factory-identified hardware. This task adds remaining components for complete hardware enablement and future development support.

## Success Criteria
- [ ] All factory-identified hardware components have device tree nodes
- [ ] Accelerometer device tree integration completed
- [ ] Temperature monitoring sensors configured
- [ ] Complete GPIO pin configuration documented
- [ ] Additional I2C/SPI device nodes added
- [ ] Device tree compiles without warnings or errors
- [ ] Hardware component documentation updated

## Missing Hardware Components Analysis

### Components Requiring Device Tree Integration
Based on factory firmware analysis and FEX file examination:

1. **Accelerometer**: Motion detection for keystone correction
2. **Temperature Sensors**: Thermal monitoring and fan control
3. **Additional GPIO**: Factory-configured pins not yet mapped
4. **I2C Devices**: Factory I2C bus devices and sensors
5. **SPI Devices**: Additional SPI peripherals (if present)
6. **PWM Controllers**: Fan control and LED dimming
7. **Watchdog Timer**: System reliability monitoring
8. **RTC (Real-Time Clock)**: System time maintenance

## Implementation Steps

### Phase 1: Accelerometer Integration
1. **Accelerometer Device Tree Node**
   ```dts
   &i2c0 {
       status = "okay";
       
       accelerometer@53 {
           compatible = "st,lis3dh-accel";
           reg = <0x53>;
           interrupt-parent = <&pio>;
           interrupts = <7 9 IRQ_TYPE_EDGE_RISING>; /* PH9 */
           
           /* Accelerometer configuration for keystone correction */
           st,drdy-int-pin = <1>;
           st,click-single-x;
           st,click-single-y; 
           st,click-single-z;
           st,min-limit = <120>;
           st,max-limit = <550>;
           
           /* Keystone correction specific settings */
           mount-matrix = "1", "0", "0",
                         "0", "1", "0", 
                         "0", "0", "1";
           
           status = "okay";
       };
   };
   ```

2. **Accelerometer GPIO Configuration**
   ```dts
   &pio {
       accel_pins: accel-pins {
           pins = "PH9";
           function = "gpio_in";
           bias-pull-up;
       };
   };
   ```

### Phase 2: Temperature Monitoring Integration
1. **Thermal Sensor Nodes**
   ```dts
   thermal-zones {
       cpu-thermal {
           polling-delay-passive = <250>;
           polling-delay = <1000>;
           thermal-sensors = <&ths>;
           
           trips {
               cpu_warm: cpu-warm {
                   temperature = <75000>;
                   hysteresis = <2000>;
                   type = "active";
               };
               
               cpu_hot: cpu-hot {
                   temperature = <85000>;
                   hysteresis = <2000>;
                   type = "active";
               };
               
               cpu_critical: cpu-critical {
                   temperature = <105000>;
                   hysteresis = <2000>;
                   type = "critical";
               };
           };
           
           cooling-maps {
               map0 {
                   trip = <&cpu_warm>;
                   cooling-device = <&cpu0 THERMAL_NO_LIMIT 1>;
               };
               
               map1 {
                   trip = <&cpu_hot>;
                   cooling-device = <&cpu0 1 THERMAL_NO_LIMIT>;
               };
           };
       };
       
       projector-thermal {
           polling-delay-passive = <250>;
           polling-delay = <1000>;
           
           trips {
               projector_warm: projector-warm {
                   temperature = <60000>;
                   hysteresis = <2000>;
                   type = "active";
               };
               
               projector_hot: projector-hot {
                   temperature = <75000>;
                   hysteresis = <2000>;
                   type = "critical";
               };
           };
           
           cooling-maps {
               fan_map {
                   trip = <&projector_warm>;
                   cooling-device = <&projector_fan THERMAL_NO_LIMIT THERMAL_NO_LIMIT>;
               };
           };
       };
   };
   ```

2. **PWM Fan Control**
   ```dts
   &pwm {
       status = "okay";
   };
   
   projector_fan: projector-fan {
       compatible = "pwm-fan";
       pwms = <&pwm 0 40000 0>; /* 25kHz */
       cooling-min-state = <0>;
       cooling-max-state = <3>;
       #cooling-cells = <2>;
       cooling-levels = <0 102 170 230>;
   };
   ```

### Phase 3: Additional I2C and SPI Devices
1. **Complete I2C Bus Configuration**
   ```dts
   &i2c1 {
       status = "okay";
       
       /* Power management IC (if present) */
       pmic@34 {
           compatible = "x-powers,axp803";
           reg = <0x34>;
           interrupt-parent = <&r_intc>;
           interrupts = <0 IRQ_TYPE_LEVEL_LOW>;
           
           regulators {
               /* Regulator configuration for additional devices */
               reg_aldo1: aldo1 {
                   regulator-min-microvolt = <1800000>;
                   regulator-max-microvolt = <3300000>;
                   regulator-name = "vdd-sensor";
               };
           };
       };
   };
   
   &i2c2 {
       status = "okay";
       
       /* Additional sensors or devices from factory analysis */
       eeprom@50 {
           compatible = "atmel,24c02";
           reg = <0x50>;
           pagesize = <16>;
       };
   };
   ```

2. **SPI Device Configuration (if applicable)**
   ```dts
   &spi0 {
       status = "okay";
       
       /* SPI flash or other SPI devices if identified */
       spiflash@0 {
           compatible = "jedec,spi-nor";
           reg = <0>;
           spi-max-frequency = <50000000>;
           
           partitions {
               compatible = "fixed-partitions";
               #address-cells = <1>;
               #size-cells = <1>;
               
               partition@0 {
                   label = "spi-config";
                   reg = <0x0 0x100000>;
                   read-only;
               };
           };
       };
   };
   ```

### Phase 4: GPIO and Pin Configuration Completion
1. **Complete GPIO Pin Mapping**
   ```dts
   &pio {
       /* Factory-analyzed GPIO configurations */
       projector_control_pins: projector-control-pins {
           pins = "PC7", "PC8", "PC9"; /* Lamp control pins */
           function = "gpio_out";
           drive-strength = <20>;
       };
       
       status_led_pins: status-led-pins {
           pins = "PC10"; /* Status LED */
           function = "gpio_out";
           drive-strength = <20>;
       };
       
       power_button_pins: power-button-pins {
           pins = "PL3"; /* Power button */
           function = "gpio_in";
           bias-pull-up;
       };
   };
   ```

2. **LED Configuration**
   ```dts
   leds {
       compatible = "gpio-leds";
       
       status {
           label = "hy300:blue:status";
           gpios = <&pio 2 10 GPIO_ACTIVE_HIGH>; /* PC10 */
           linux,default-trigger = "heartbeat";
       };
       
       power {
           label = "hy300:red:power";
           gpios = <&pio 2 11 GPIO_ACTIVE_HIGH>; /* PC11 */
           default-state = "on";
       };
   };
   ```

### Phase 5: System Integration Devices
1. **Watchdog Timer Configuration**
   ```dts
   &wdt {
       status = "okay";
   };
   ```

2. **RTC Configuration (if external RTC present)**
   ```dts
   &i2c0 {
       rtc@68 {
           compatible = "dallas,ds1307";
           reg = <0x68>;
       };
   };
   ```

3. **Additional MMC/Storage**
   ```dts
   &mmc1 {
       pinctrl-names = "default";
       pinctrl-0 = <&mmc1_pins>;
       vmmc-supply = <&reg_cldo1>;
       vqmmc-supply = <&reg_bldo1>;
       cd-gpios = <&pio 5 6 GPIO_ACTIVE_LOW>; /* PF6 */
       bus-width = <4>;
       status = "okay";
   };
   ```

## Technical Requirements

### Device Tree Standards
- **Mainline Compatibility**: Follow upstream device tree bindings
- **Documentation**: All custom bindings properly documented
- **Validation**: Device tree compilation without warnings
- **Testing**: Hardware detection validation where possible

### Integration Requirements
- **Service Compatibility**: Device tree supports HY300 service integration
- **Driver Requirements**: All nodes have corresponding driver support
- **Power Management**: Proper power domain and clock integration
- **Thermal Management**: Complete thermal monitoring coverage

## Testing Procedures

### Phase 1 Testing: Device Tree Compilation
1. **Compilation Validation**
   ```bash
   # Compile device tree with all additions
   nix develop -c -- dtc -I dts -O dtb sun50i-h713-hy300.dts -o sun50i-h713-hy300.dtb
   # Check for warnings or errors
   dtc -I dts -O dts sun50i-h713-hy300.dts | grep -i warning
   ```

2. **Size and Structure Validation**
   ```bash
   # Check compiled device tree size
   ls -lah sun50i-h713-hy300.dtb
   # Validate device tree structure
   fdtdump sun50i-h713-hy300.dtb | head -50
   ```

### Phase 2 Testing: Hardware Detection Simulation
1. **Device Node Validation**
   ```bash
   # Extract device tree and check nodes (via FEL mode when available)
   # fdtget sun50i-h713-hy300.dtb / model
   # fdtget sun50i-h713-hy300.dtb /soc compatible
   ```

2. **GPIO Configuration Testing**
   ```bash
   # Test GPIO configuration (when hardware available)
   # cat /sys/kernel/debug/pinctrl/1c20800.pinctrl/pins
   # grep -r "projector" /sys/class/gpio/
   ```

### Phase 3 Testing: Service Integration
1. **HY300 Service Compatibility**
   ```bash
   # Test service device tree parsing
   python3 -c "import hy300_services; hy300_services.parse_device_tree()"
   # Validate service hardware expectations
   ```

2. **Kodi Hardware Detection**
   ```bash
   # Test Kodi hardware detection with complete device tree
   kodi --debug --log-level=2 | grep -i hardware
   ```

## Quality Assurance

### Device Tree Standards
- **Binding Compliance**: All device tree bindings follow mainline standards
- **Documentation**: Complete documentation for custom or projector-specific nodes
- **Validation**: No compilation warnings or errors
- **Future Compatibility**: Device tree structure supports future hardware additions

### Hardware Coverage
- **Complete Mapping**: All factory-identified hardware has device tree representation
- **Service Integration**: Device tree supports all HY300 service requirements  
- **Driver Support**: All device tree nodes have corresponding driver implementations
- **Testing Capability**: Hardware detection testable via FEL mode

## Risk Assessment

### Technical Risks
- **Hardware Assumptions**: Device tree assumptions may not match actual hardware
- **Driver Compatibility**: Device tree nodes may not have working drivers
- **Integration Issues**: Complex device tree integration issues
- **Performance Impact**: Large device tree compilation and boot time impact

### Mitigation Strategies
- **Factory Analysis**: Base all nodes on factory firmware analysis
- **Incremental Addition**: Add device tree nodes incrementally with testing
- **Driver Validation**: Verify driver support before adding device tree nodes
- **FEL Testing**: Test device tree changes via FEL mode first

## Success Metrics
- **Complete Coverage**: All factory hardware components represented
- **Compilation Success**: Clean device tree compilation without warnings
- **Hardware Detection**: Successful hardware detection for testable components
- **Service Integration**: Complete HY300 service device tree compatibility
- **Documentation**: Comprehensive device tree documentation

## Documentation Updates Required

### Hardware Documentation
- Update `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` with complete device tree coverage
- Document all device tree nodes and their hardware mapping
- Create device tree troubleshooting and validation guide

### Technical References
- Update device tree binding documentation
- Document custom projector-specific device tree bindings
- Create hardware detection and testing procedures

## Next Steps After Completion
- **Hardware Testing Preparation** (Task 025): FEL mode testing with complete device tree
- **Motor Driver Integration** (Task 020): Hardware keystone control integration
- **Hardware Validation** (Task 010): Complete hardware testing with full device tree

This task completes the comprehensive hardware representation in the device tree, providing the foundation for complete hardware enablement and testing.