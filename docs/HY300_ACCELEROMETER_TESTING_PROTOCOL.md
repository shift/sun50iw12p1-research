# HY300 Accelerometer I2C Detection Testing Protocol

## Overview
This document provides the testing procedure for validating accelerometer hardware detection on the HY300 projector via FEL mode recovery.

## Safety Prerequisites
- **FEL Recovery Setup**: Ensure FEL mode recovery capability before testing
- **Backup Complete**: Maintain complete eMMC backup for recovery
- **Serial Console Access**: Required for debugging output monitoring
- **Power Management**: Safe power cycling procedures established

## Hardware Configuration

### Updated Device Tree
**File**: `sun50i-h713-hy300.dtb` (10.7KB)

#### STK8BA58 Accelerometer
```dts
accelerometer1: stk8ba58@18 {
    compatible = "sensortek,stk8ba58";
    reg = <0x18>;
    interrupt-parent = <&pio>;
    interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */
    stk,direction = <2>;
};
```

#### KXTTJ3 Accelerometer  
```dts
accelerometer2: kxtj3@e {
    compatible = "kionix,kxtj3-1057";
    reg = <0x0e>; /* Factory DTB uses 0x0e */
    interrupt-parent = <&pio>;
    interrupts = <1 0 2>; /* PB0, IRQ_TYPE_EDGE_FALLING */
    status = "disabled"; /* Enable via hardware detection */
};
```

## Testing Procedure

### Phase 1: Boot with Updated Device Tree
1. **Load via FEL**: Boot system with updated `sun50i-h713-hy300.dtb`
2. **Monitor Boot**: Watch for I2C bus initialization and device detection
3. **Kernel Messages**: Check for accelerometer driver binding messages

### Phase 2: I2C Bus Detection  
```bash
# Scan I2C bus 1 for accelerometer devices
i2cdetect -y 1

# Expected results:
# Address 0x18: STK8BA58 accelerometer
# Address 0x0e: KXTTJ3 accelerometer (if present)
```

### Phase 3: GPIO Interrupt Validation
```bash
# Check GPIO configuration
cat /sys/kernel/debug/gpio

# Monitor PB0 (pin 32 = 1*32 + 0) for interrupt activity
echo 32 > /sys/class/gpio/export
cat /sys/class/gpio/gpio32/value

# Test interrupt generation via device movement
# Should see interrupt count changes in /proc/interrupts
grep -i "PB0\|accelerometer" /proc/interrupts
```

### Phase 4: Driver Binding Verification
```bash
# Check device registration in IIO subsystem
ls /sys/bus/iio/devices/

# Verify accelerometer device nodes
ls /dev/iio:device*

# Test basic device access
cat /sys/bus/iio/devices/iio:device*/name
```

## Expected Results

### Successful Detection
- **I2C Scan**: Shows devices at 0x18 and/or 0x0e
- **Kernel Logs**: Driver binding messages for sensortek,stk8ba58 or kionix,kxtj3
- **IIO Devices**: `/dev/iio:device*` nodes created
- **GPIO Interrupts**: PB0 interrupt counter increases with device movement

### Hardware Revision Detection
- **Single Accelerometer**: Only one of STK8BA58 or KXTTJ3 detected
- **Dual Configuration**: Both accelerometers present (unlikely but possible)
- **Alternative Addresses**: Device at different I2C address requiring DTS adjustment

### Failure Cases
- **No I2C Detection**: Bus scan shows no devices (hardware or driver issue)
- **Driver Mismatch**: I2C device detected but wrong compatible string
- **No Interrupts**: Device detected but no interrupt functionality (GPIO configuration issue)

## Troubleshooting Guide

### I2C Detection Failure
1. **Check Bus Configuration**: Verify I2C1 pinmux and clock settings
2. **Alternative Addresses**: Scan full I2C address range (0x08-0x77)
3. **Pull-up Resistors**: Hardware may require external pull-ups

### Driver Binding Issues
1. **Compatible String**: Try factory DTB compatible strings ("stk,stk83xx", "kxtj3")
2. **Kernel Module**: Ensure accelerometer drivers compiled in kernel
3. **Device Tree Path**: Verify I2C bus parent configuration

### Interrupt Configuration
1. **GPIO Pinmux**: Verify PB0 configured for GPIO input mode
2. **Interrupt Polarity**: Try different interrupt types (EDGE_RISING, LEVEL_LOW)
3. **Alternative Pins**: Check if factory uses different GPIO pin

## Integration with Keystone System

### Motor Coordination
Once accelerometer detection is confirmed:
1. **Test Motor Movement**: Verify stepper motor control on PH4-PH7
2. **Combined Operation**: Test accelerometer + motor for auto-correction
3. **MIPS Integration**: Validate display.bin firmware loading and transformation

### Auto-Correction Validation
```bash
# Test keystone correction pipeline
echo "auto" > /sys/class/mips/mipsloader_panelparam/mode
cat /sys/class/mips/mipsloader_panelparam/status

# Monitor accelerometer data during correction
cat /sys/bus/iio/devices/iio:device*/in_accel_*_raw
```

## Documentation Updates Required

### Success Path
- Update `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` with accelerometer status
- Record working I2C addresses and compatible strings
- Document interrupt GPIO validation results

### Issue Path  
- Document specific hardware differences from factory analysis
- Update device tree with confirmed configurations
- Create issue reports for upstream kernel driver improvements

## Next Phase: Motor Integration
Once accelerometer detection is validated:
1. **Task 017**: Motor + Accelerometer integration testing
2. **Task 018**: MIPS co-processor keystone correction validation  
3. **Task 019**: Complete auto-correction system testing

---
**Prerequisites**: FEL mode recovery setup, serial console access
**Hardware Safety**: All testing via FEL recovery - no permanent modifications
**Documentation**: Update all results in hardware enablement matrix
**Created**: 2025-01-18
**Dependencies**: Completed accelerometer GPIO analysis, updated device tree