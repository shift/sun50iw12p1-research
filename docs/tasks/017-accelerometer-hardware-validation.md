# Task 017: HY300 Accelerometer Hardware Validation

## Task Metadata
- **Task ID**: 017
- **Type**: Hardware Validation
- **Priority**: High
- **Phase**: V (Driver Integration)
- **Status**: pending
- **Created**: 2025-01-18
- **Dependencies**: Task 016 (Accelerometer GPIO Analysis), Updated DTS with interrupt configurations

## Objective
Validate accelerometer hardware detection and GPIO interrupt functionality on HY300 projector hardware via FEL mode testing.

## Prerequisites
- **Hardware Access**: Physical HY300 projector device
- **FEL Recovery**: Confirmed FEL mode access and recovery capability
- **Serial Console**: Working serial console connection for debugging
- **Safety Backup**: Complete eMMC backup for recovery
- **Updated DTS**: `sun50i-h713-hy300.dtb` with accelerometer interrupt GPIO configurations

## Success Criteria
1. **I2C Detection**: Accelerometers detected at expected addresses (0x18, 0x0e)
2. **Driver Binding**: Kernel drivers successfully bind to accelerometer devices
3. **Interrupt Functionality**: PB0 GPIO interrupts trigger on device movement
4. **IIO Integration**: `/dev/iio:device*` nodes created and accessible
5. **Data Validation**: Accelerometer data changes with physical device orientation

## Implementation Plan

### Phase 1: Device Tree Validation
**Objective**: Confirm updated device tree boots successfully
```bash
# Boot with updated DTB via FEL mode
sunxi-fel -v uboot u-boot-sunxi-with-spl.bin write 0x4a000000 sun50i-h713-hy300.dtb
# Monitor boot process via serial console
```

### Phase 2: I2C Hardware Detection  
**Objective**: Confirm accelerometers present on I2C bus
```bash
# Scan I2C bus 1 for accelerometer devices
i2cdetect -y 1

# Expected results:
# 0x18: STK8BA58 accelerometer
# 0x0e: KXTTJ3 accelerometer (alternative sensor)
```

### Phase 3: Driver Binding Verification
**Objective**: Validate kernel driver integration
```bash
# Check IIO device registration
ls -la /sys/bus/iio/devices/
cat /sys/bus/iio/devices/iio:device*/name

# Verify device nodes created
ls -la /dev/iio:device*

# Check kernel driver messages
dmesg | grep -i "stk8ba58\|kxtj3\|accelerometer"
```

### Phase 4: GPIO Interrupt Testing
**Objective**: Validate PB0 interrupt functionality
```bash
# Check GPIO configuration
cat /sys/kernel/debug/gpio | grep -A5 -B5 "32\|PB0"

# Monitor interrupt activity
grep -i "PB0\|32:" /proc/interrupts

# Test physical movement detection
# Move device and check for interrupt count changes
watch -n1 'grep -i "PB0\|32:" /proc/interrupts'
```

### Phase 5: Data Acquisition Testing
**Objective**: Validate accelerometer data collection
```bash
# Read raw accelerometer values
cat /sys/bus/iio/devices/iio:device*/in_accel_x_raw
cat /sys/bus/iio/devices/iio:device*/in_accel_y_raw  
cat /sys/bus/iio/devices/iio:device*/in_accel_z_raw

# Test orientation detection by tilting device
# Values should change with device orientation
```

## Hardware Configuration Details

### Expected I2C Configuration
- **Bus**: I2C1 at 0x5002400
- **Clock**: 100kHz (factory configuration)
- **Devices**: STK8BA58 @ 0x18, KXTTJ3 @ 0x0e

### GPIO Interrupt Configuration  
- **Pin**: PB0 (GPIO Bank B, Pin 0)
- **Type**: Edge falling (IRQ_TYPE_EDGE_FALLING = 2)
- **Function**: Motion detection for auto-keystone correction

### Device Tree Configuration
```dts
&i2c1 {
    accelerometer1: stk8ba58@18 {
        compatible = "sensortek,stk8ba58";
        reg = <0x18>;
        interrupt-parent = <&pio>;
        interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */
        stk,direction = <2>;
    };

    accelerometer2: kxtj3@e {
        compatible = "kionix,kxtj3-1057";
        reg = <0x0e>;
        interrupt-parent = <&pio>;
        interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */
        status = "disabled"; /* Enable via hardware detection */
    };
};
```

## Validation Procedures

### Success Indicators
1. **Boot Success**: System boots with updated DTB without errors
2. **I2C Scan Results**: Devices detected at expected addresses
3. **Driver Messages**: Positive kernel driver binding messages
4. **IIO Device Creation**: `/dev/iio:device*` nodes accessible
5. **Interrupt Activity**: PB0 interrupt count increases with movement
6. **Data Variation**: Accelerometer readings change with orientation

### Common Issues and Solutions

#### Issue: No I2C Device Detection
**Symptoms**: `i2cdetect` shows no devices at 0x18 or 0x0e
**Solutions**:
1. Check I2C bus configuration in device tree
2. Verify I2C pinmux settings (SDA/SCL pins)
3. Test alternative I2C addresses (full bus scan)
4. Check for hardware pull-up resistor requirements

#### Issue: Driver Binding Failure
**Symptoms**: I2C device detected but no IIO device created
**Solutions**:
1. Try factory compatible strings ("stk,stk83xx", "kxtj3")
2. Verify kernel accelerometer drivers enabled
3. Check device tree path and parent configuration
4. Load kernel modules manually if built as modules

#### Issue: No GPIO Interrupts
**Symptoms**: Accelerometer detected but no interrupt activity
**Solutions**:
1. Verify PB0 GPIO configuration in pinctrl
2. Test different interrupt types (EDGE_RISING, LEVEL_LOW)
3. Check for GPIO conflicts with other devices
4. Validate interrupt parent reference in device tree

#### Issue: Hardware Revision Differences
**Symptoms**: Different accelerometer model than expected
**Solutions**:
1. Document actual hardware configuration
2. Update device tree with confirmed compatible strings
3. Test both accelerometer configurations
4. Create hardware detection logic

## Integration Testing

### Motor Coordination
Once accelerometer validation is complete:
1. **Test Motor Control**: Verify stepper motor on PH4-PH7 GPIOs
2. **Combined Operation**: Accelerometer + motor auto-correction
3. **Limit Switch**: Validate PH14 limit switch integration

### MIPS Co-processor Integration
1. **Firmware Loading**: Test `display.bin` loading to MIPS
2. **Transform Pipeline**: Validate keystone correction processing
3. **User Interface**: Test sysfs interface for keystone parameters

## Documentation Requirements

### Success Documentation
- Update `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` with validation results
- Record confirmed I2C addresses and compatible strings  
- Document working interrupt GPIO configuration
- Add accelerometer testing protocol to hardware methodology

### Issue Documentation
- Document hardware differences from factory analysis
- Create device tree patches for confirmed configurations
- Report upstream kernel driver compatibility issues
- Update testing methodology with lessons learned

## Next Tasks
Upon successful completion:
- **Task 018**: Motor + Accelerometer integration testing
- **Task 019**: MIPS keystone correction validation
- **Task 020**: Complete auto-correction system testing

## Risk Mitigation
- **FEL Recovery**: All testing via FEL mode - no permanent eMMC modifications
- **Serial Console**: Maintain debugging access throughout testing
- **Incremental Testing**: Test each component individually before integration
- **Backup Strategy**: Maintain multiple recovery paths and backups

---
**Hardware Dependencies**: Physical HY300 device, FEL mode access, serial console
**Safety Level**: Medium - FEL recovery available but requires hardware access
**Expected Duration**: 4-6 hours for complete validation
**Critical Path**: Required for keystone correction functionality validation