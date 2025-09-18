# HY300 Accelerometer GPIO Interrupt Analysis

## Factory vs Mainline Configuration Comparison

### Factory DTB Configuration
**File**: `firmware/update.img.extracted/FC00/system.dtb`

#### STK8BA58 Accelerometer
```dts
stk8ba58@18 {
    compatible = "stk,stk83xx";
    reg = <0x18>;
    stk,direction = <0x02>;
    stk83xx,irq-gpio = <0x02 0x01 0x00 0x01>;
    status = "okay";
};
```

#### KXTTJ3 Accelerometer  
```dts
kxttj3@18 {
    compatible = "kxtj3";
    reg = <0x0e>;              /* Note: Different I2C address! */
    stk,direction = <0x02>;
    irq-gpio = <0x02 0x01 0x00 0x01>;
    status = "okay";
};
```

### Current Mainline DTS Configuration
**File**: `sun50i-h713-hy300.dts`

#### STK8BA58 Accelerometer
```dts
accelerometer1: stk8ba58@18 {
    compatible = "sensortek,stk8ba58";
    reg = <0x18>;
    /* Missing: interrupt GPIO configuration */
};
```

#### KXTTJ3 Accelerometer
```dts
accelerometer2: kxtj3@18 {
    compatible = "kionix,kxtj3-1057";
    reg = <0x18>;              /* Inconsistent with factory DTB! */
    status = "disabled";
    /* Missing: interrupt GPIO configuration */
};
```

## GPIO Interrupt Pin Analysis

### GPIO Controller Reference
```dts
pinctrl@2000000 {
    compatible = "allwinner,sun50iw12-pinctrl";
    gpio-controller;
    #gpio-cells = <0x03>;
    phandle = <0x02>;          /* Referenced by accelerometer irq-gpio */
    /* ... */
};
```

### Interrupt GPIO Decoding
**Factory DTB pattern**: `irq-gpio = <0x02 0x01 0x00 0x01>`

- `0x02`: phandle to main GPIO controller (pinctrl@2000000)
- `0x01`: GPIO bank B (0x01 = PB bank)
- `0x00`: GPIO pin 0 (PB0)
- `0x01`: GPIO flags (likely GPIO_ACTIVE_LOW or similar)

**Decoded as**: PB0 (Pin Bank B, Pin 0)

## Key Findings

### 1. Missing Interrupt Configurations
Both accelerometers in mainline DTS lack interrupt GPIO configurations that are present in factory DTB.

### 2. I2C Address Inconsistency
- **STK8BA58**: Consistent at `0x18`
- **KXTTJ3**: Factory uses `0x0e`, mainline uses `0x18` 

### 3. Compatible String Differences
- **STK8BA58**: Factory `"stk,stk83xx"` vs mainline `"sensortek,stk8ba58"`
- **KXTTJ3**: Factory `"kxtj3"` vs mainline `"kionix,kxtj3-1057"`

### 4. Both Use Same Interrupt Pin
Both accelerometers reference the same GPIO pin (PB0), suggesting they may be:
- Alternative sensors (different hardware revisions)
- Shared interrupt handling
- Hardware detection required to enable correct sensor

## Recommended Actions

### Immediate Fixes Needed
1. **Add interrupt GPIO**: Both accelerometers need `interrupt-parent` and `interrupts` properties
2. **Fix KXTTJ3 address**: Change from `0x18` to `0x0e` to match factory configuration
3. **Verify compatible strings**: Research upstream kernel drivers for correct compatible strings

### Device Tree Updates Required
```dts
&i2c1 {
    accelerometer1: stk8ba58@18 {
        compatible = "sensortek,stk8ba58";
        reg = <0x18>;
        interrupt-parent = <&pio>;
        interrupts = <1 0 IRQ_TYPE_EDGE_FALLING>; /* PB0 */
        stk,direction = <2>;
    };

    accelerometer2: kxtj3@0e {
        compatible = "kionix,kxtj3-1057";
        reg = <0x0e>;                              /* Corrected address */
        interrupt-parent = <&pio>;
        interrupts = <1 0 IRQ_TYPE_EDGE_FALLING>; /* PB0 */
        status = "disabled"; /* Enable via hardware detection */
    };
};
```

## Hardware Testing Requirements

### Safe Testing via FEL Mode
1. Test accelerometer detection on I2C bus
2. Verify interrupt functionality with GPIO monitoring
3. Validate keystone motor integration with accelerometer data
4. Test hardware revision detection logic

### Validation Steps
1. `i2cdetect -y 1` - Should show devices at 0x18 and/or 0x0e
2. GPIO interrupt monitoring for motion detection
3. Integration with keystone correction system testing

## Next Steps
1. Research STK8BA58 and KXTTJ3 datasheets for interrupt requirements
2. Update mainline DTS with interrupt configurations
3. Test hardware detection and proper sensor activation
4. Integrate with keystone correction motor control system

---
**Status**: Analysis complete, ready for DTS updates and hardware validation
**Created**: 2025-01-18
**Related Tasks**: 016 (Keystone Correction Research)