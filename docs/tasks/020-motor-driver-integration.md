# Task 020: Motor Driver Kernel Integration

## Task Metadata
- **Task ID**: 020
- **Type**: Kernel Driver Integration
- **Priority**: Medium
- **Phase**: VII
- **Status**: pending
- **Created**: 2025-09-18
- **Dependencies**: Task 019 (Keystone sysfs interface)

## Objective
Integrate the HY300 keystone motor control driver into the mainline kernel build system and ensure proper device tree binding.

## Context
The motor driver (`drivers/misc/hy300-keystone-motor.c`) is complete and functional, but needs integration into the kernel build system and device tree for production use.

## Current Driver Status
- ✅ Complete driver implementation (420 lines)
- ✅ Sysfs interface for manual control
- ✅ Interrupt-driven limit switch handling
- ✅ Factory-analyzed stepping sequences
- ❌ Not integrated into kernel build system
- ❌ Device tree node not active

## Implementation Plan

### 1. Kernel Configuration Integration
Add to `drivers/misc/Kconfig`:
```kconfig
config HY300_KEYSTONE_MOTOR
    tristate "HY300 Keystone Motor Control Driver"
    depends on OF && GPIOLIB
    help
      Driver for the HY300 projector keystone correction stepper motor.
      
      This driver provides control interface for the 4-phase stepper motor
      used for physical lens keystone correction in HY300 Android projectors.
      
      The driver exposes sysfs interfaces for position control, homing, and
      status monitoring.
      
      Say Y here if you have an HY300 projector and want keystone motor
      control support.
      
      To compile this driver as a module, choose M here: the module will
      be called hy300-keystone-motor.
```

### 2. Makefile Integration
Update `drivers/misc/Makefile`:
```makefile
obj-$(CONFIG_HY300_KEYSTONE_MOTOR) += hy300-keystone-motor.o
```

### 3. Device Tree Node Activation
Update `sun50i-h713-hy300.dts` to activate motor node:
```dts
&motor_ctr {
    compatible = "hy300,keystone-motor";
    status = "okay";
    
    phase-gpios = <&pio 7 4 GPIO_ACTIVE_HIGH>,   /* PH4 */
                  <&pio 7 5 GPIO_ACTIVE_HIGH>,   /* PH5 */
                  <&pio 7 6 GPIO_ACTIVE_HIGH>,   /* PH6 */
                  <&pio 7 7 GPIO_ACTIVE_HIGH>;   /* PH7 */
    
    limit-gpio = <&pio 7 14 GPIO_ACTIVE_LOW>;    /* PH14 */
    
    phase-delay-us = <1000>;
    step-delay-ms = <2>;
    max-position = <1000>;
};
```

### 4. Device Tree Binding Documentation
Create `Documentation/devicetree/bindings/misc/hy300,keystone-motor.yaml`:
```yaml
# SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)
%YAML 1.2
---
$id: http://devicetree.org/schemas/misc/hy300,keystone-motor.yaml#
$schema: http://devicetree.org/meta-schemas/core.yaml#

title: HY300 Keystone Motor Control

maintainers:
  - HY300 Linux Porting Project <hy300@example.com>

description: |
  The HY300 projector uses a 4-phase stepper motor for keystone correction.
  This binding describes the motor control interface including GPIO pins
  for motor phases and limit switch.

properties:
  compatible:
    const: hy300,keystone-motor

  phase-gpios:
    description: |
      GPIO pins for the 4 motor phases in order (phase0-phase3).
      These control the stepper motor windings.
    minItems: 4
    maxItems: 4

  limit-gpio:
    description: |
      GPIO pin for the limit switch that detects home position.
      Active low signal indicates motor is at home position.
    maxItems: 1

  phase-delay-us:
    description: Delay in microseconds between phase changes
    default: 1000

  step-delay-ms:
    description: Delay in milliseconds between full steps
    default: 2

  max-position:
    description: Maximum motor position in steps from home
    default: 1000

required:
  - compatible
  - phase-gpios
  - limit-gpio

additionalProperties: false

examples:
  - |
    #include <dt-bindings/gpio/gpio.h>
    
    keystone-motor {
        compatible = "hy300,keystone-motor";
        phase-gpios = <&pio 7 4 GPIO_ACTIVE_HIGH>,
                      <&pio 7 5 GPIO_ACTIVE_HIGH>,
                      <&pio 7 6 GPIO_ACTIVE_HIGH>,
                      <&pio 7 7 GPIO_ACTIVE_HIGH>;
        limit-gpio = <&pio 7 14 GPIO_ACTIVE_LOW>;
        phase-delay-us = <1000>;
        step-delay-ms = <2>;
        max-position = <1000>;
    };
```

### 5. Kernel Defconfig Update
Add to `configs/sun50i-h713-hy300_defconfig`:
```
CONFIG_HY300_KEYSTONE_MOTOR=m
```

### 6. Module Loading Integration
Create udev rule for automatic loading:
```udev
# /etc/udev/rules.d/99-hy300-keystone.rules
SUBSYSTEM=="platform", DRIVER=="hy300-keystone-motor", TAG+="systemd", ENV{SYSTEMD_WANTS}+="hy300-keystone-setup.service"
```

## Acceptance Criteria
- [ ] Kconfig entry created and functional
- [ ] Makefile integration working
- [ ] Device tree node properly activated
- [ ] DT binding documentation complete
- [ ] Driver compiles as module and built-in
- [ ] Device node created: `/dev/keystone-motor`
- [ ] Sysfs interface accessible: `/sys/class/platform/keystone-motor/`
- [ ] Module loads without errors
- [ ] GPIO reservation successful
- [ ] Interrupt registration working

## Files to Create/Modify
- `drivers/misc/Kconfig` - Add configuration option
- `drivers/misc/Makefile` - Add build target
- `sun50i-h713-hy300.dts` - Activate motor node
- `Documentation/devicetree/bindings/misc/hy300,keystone-motor.yaml` - DT bindings
- `configs/sun50i-h713-hy300_defconfig` - Default configuration

## Testing Plan
1. **Compilation Testing**
   - Module compilation (CONFIG_HY300_KEYSTONE_MOTOR=m)
   - Built-in compilation (CONFIG_HY300_KEYSTONE_MOTOR=y)
   - Device tree compilation with new node

2. **Integration Testing**
   - Module loading: `modprobe hy300-keystone-motor`
   - Device creation verification
   - GPIO reservation confirmation
   - Sysfs interface availability

3. **Functional Testing**
   - Motor position control via sysfs
   - Homing operation functionality
   - Limit switch interrupt handling
   - Error condition management

## Implementation Steps
1. **Kconfig Integration**: Add configuration option
2. **Build System**: Update Makefile and defconfig
3. **Device Tree**: Activate motor node and create bindings
4. **Documentation**: Create DT binding documentation
5. **Testing**: Verify compilation and integration
6. **Validation**: Test motor functionality

## Notes
- Driver is already complete and functional
- Focus on build system and device tree integration
- Must maintain compatibility with existing device tree
- GPIO pins match factory analysis (PH4-PH7, PH14)
- Default timing parameters from factory firmware analysis