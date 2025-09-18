# Projector-Specific Hardware Driver Analysis - HY300 Project

**Analysis Date:** September 2025  
**Source:** Factory kernel configuration and device tree analysis  
**Analyzed By:** Phase V.5 Driver Integration Research  

## Executive Summary

Analysis of HY300 projector-specific hardware drivers based on factory Android kernel configuration and device tree specifications. This analysis identifies the driver requirements for projector-unique components including motor control, environmental sensors, thermal management, and LED control systems.

## Projector Hardware Component Matrix

### 1. Motor Control System (Electronic Keystone Correction)
**Device Tree Configuration:** `motor_ctr` node in factory DTB
**Hardware Components:**
- **Stepper Motor:** 4-phase stepper motor for keystone adjustment
- **Control GPIOs:** PH4, PH5, PH6, PH7 (motor phase control)
- **Limit Switch:** PH14 (end-of-travel detection)
- **Timing Control:** Phase delays and step delays defined

**Factory Driver Requirements:**
```bash
# Likely part of Sunxi platform drivers
CONFIG_SUNXI_GPIO=y                       # GPIO subsystem support
CONFIG_SUNXI_PINCTRL=y                    # Pin control framework
```

**Mainline Integration Strategy:**
- **Platform Driver:** Custom motor control platform driver
- **GPIO Integration:** Use mainline H713 pinctrl/GPIO drivers
- **Userspace Interface:** `/sys/class/gpio` or custom sysfs interface
- **Motor Control Algorithm:** Implement stepping sequences from DTB data

**Implementation Requirements:**
- Precise timing control for smooth motor operation
- Limit switch monitoring for safe operation
- Bidirectional control (clockwise/counter-clockwise)
- Position tracking and calibration

### 2. Environmental Sensors (Auto-Keystone Correction)
**Device Tree Configuration:** I2C sensors on twi1 (I2C bus 1)
**Hardware Components:**
- **STK8BA58 Accelerometer:** I2C address 0x18, compatible "stk,stk83xx"
- **KXTTJ3 Accelerometer:** I2C address 0x18, compatible "kxtj3"
- **I2C Interface:** twi@2502400 (twi1 bus)

**Factory Driver Requirements:**
```bash
CONFIG_I2C_SUNXI=y                        # Sunxi I2C controller
CONFIG_SENSORS_STK8XXX=m                  # STK accelerometer driver (if available)
CONFIG_SENSORS_KXTJ9=m                    # Kionix accelerometer driver (if available)
```

**Mainline Integration Strategy:**
- **I2C Driver:** Use mainline sunxi-i2c driver
- **Sensor Drivers:** Check for existing mainline drivers for STK8BA58/KXTTJ3
- **IIO Framework:** Integrate with Industrial I/O subsystem
- **Auto-Keystone Algorithm:** Userspace application reading IIO devices

**Implementation Requirements:**
- Dual accelerometer data fusion for improved accuracy
- Tilt angle calculation and keystone correction mapping
- Calibration procedures for sensor alignment
- Integration with motor control system

### 3. Display Control and Power Management
**Device Tree Configuration:** `tvtop@5700000` node
**Hardware Components:**
- **LCD Panel Power:** PH19 (panel_power_en)
- **LED Backlight Power:** PB5 (panel_bl_en)  
- **PWM Brightness Control:** PWM channel 3, 25kHz frequency
- **Power Sequencing:** Multi-stage delays for safe power cycling

**Factory Driver Requirements:**
```bash
CONFIG_SUNXI_TVUTILS=m                    # TV utilities
CONFIG_SUNXI_TVTOP=m                      # TV top-level control
CONFIG_SUNXI_PWM=y                        # PWM subsystem
CONFIG_SUNXI_TVCAP_TVDISP_IOMMU=y         # TV display IOMMU
```

**Mainline Integration Strategy:**
- **Platform Driver:** Custom tvtop-equivalent platform driver
- **PWM Integration:** Use mainline sunxi-pwm driver
- **GPIO Control:** Use H713 GPIO drivers for power control
- **Display Pipeline:** Coordinate with MIPS co-processor display subsystem

**Implementation Requirements:**
- Safe power sequencing to prevent LCD damage
- PWM-based brightness control integration
- Synchronization with MIPS display firmware
- Panel timing and power state management

### 4. Thermal Management System
**Device Tree Configuration:** `fan` and `fan_ctrl` nodes
**Hardware Components:**
- **PWM Fan Control:** Compatible "pwm-fan" for variable speed control
- **GPIO Fan Control:** PH17 (additional fan control, on/off)
- **Temperature Monitoring:** Integration with SoC thermal sensors

**Factory Driver Requirements:**
```bash
CONFIG_PWM_FAN=m                          # PWM fan driver
CONFIG_THERMAL_SUNXI=y                    # Sunxi thermal framework
CONFIG_SUNXI_THERMAL=y                    # Thermal sensor support
```

**Mainline Integration Strategy:**
- **PWM Fan Driver:** Use mainline pwm-fan driver
- **GPIO Control:** Additional fan control via GPIO
- **Thermal Framework:** Integrate with Linux thermal subsystem
- **Cooling Policy:** Temperature-based fan speed control

**Implementation Requirements:**
- Thermal zone configuration for H713 temperature sensors
- Fan speed curves based on SoC temperature
- Emergency shutdown protection for overheating
- Integration with projector-specific thermal characteristics

### 5. Status LED Control System
**Device Tree Configuration:** `leds` node
**Hardware Components:**
- **Red LED:** PL0 (pwr-r, power status)
- **Green LED:** PL1 (status-g, general status)
- **Blue LED:** PL5 (led3-b, additional status)

**Factory Driver Requirements:**
```bash
CONFIG_LEDS_GPIO=m                        # GPIO-based LED driver
CONFIG_LEDS_CLASS=y                       # LED class framework
```

**Mainline Integration Strategy:**
- **LED Class Driver:** Use mainline leds-gpio driver
- **GPIO Integration:** H713 GPIO subsystem
- **LED Triggers:** Heartbeat, network activity, custom triggers
- **Userspace Control:** Standard `/sys/class/leds/` interface

**Implementation Requirements:**
- Multi-color status indication patterns
- System state visualization (boot, running, error states)
- Network connectivity status indication
- Low-power operation and sleep state handling

## Sunxi Platform Driver Ecosystem Analysis

### Core Platform Drivers (Factory Kernel)
**Essential for Projector Operation:**
```bash
CONFIG_SUNXI_NSI=y                        # Network Service Interface
CONFIG_SUNXI_CPU_COMM=y                   # CPU communication framework
CONFIG_SUNXI_ARISC_RPM=y                  # ARISC Runtime Power Management
CONFIG_SUNXI_BOOTEVENT=y                  # Boot event handling
```

**Projector-Specific Drivers:**
```bash
CONFIG_SUNXI_RFKILL=m                     # RF kill switch (WiFi control)
CONFIG_SUNXI_ADDR_MGT=m                   # Address management
CONFIG_SUNXI_TVUTILS=m                    # TV/display utilities
CONFIG_SUNXI_TVTOP=m                      # TV top-level control
CONFIG_SUNXI_TVCAP_TVDISP_IOMMU=y         # Display IOMMU support
```

### Mainline Integration Strategy

#### Phase 1: Core Platform Support
1. **ARISC Integration:** Port ARISC runtime power management
   - **Source:** Allwinner kernel tree or community ports
   - **Function:** Low-power state management, wake-up handling
   - **Critical:** Required for proper power management

2. **GPIO/Pinctrl:** Enable H713 GPIO and pinctrl support
   - **Source:** Mainline kernel (sun50i-h6 compatibility)
   - **Function:** All projector hardware control
   - **Status:** Should work with H6 drivers

#### Phase 2: Projector Hardware Drivers
1. **Motor Control Platform Driver:** Custom implementation
   - **Approach:** New platform driver based on DTB specifications
   - **Interface:** Sysfs attributes for position control
   - **Dependencies:** GPIO, precise timing

2. **Sensor Integration:** I2C accelerometer drivers
   - **Approach:** Use existing mainline drivers or port vendor drivers
   - **Interface:** IIO framework for standardized access
   - **Dependencies:** I2C subsystem, IIO core

3. **Display Power Management:** TV/display control equivalent
   - **Approach:** Custom platform driver for power sequencing
   - **Interface:** DRM integration or custom sysfs interface
   - **Dependencies:** PWM, GPIO, coordination with MIPS firmware

#### Phase 3: System Integration
1. **Thermal Management:** Complete cooling system
   - **Approach:** Thermal zone + PWM fan + GPIO control
   - **Interface:** Standard thermal framework
   - **Dependencies:** Temperature sensors, PWM fan driver

2. **LED Status System:** Multi-color status indication
   - **Approach:** LED class driver with custom triggers
   - **Interface:** Standard LED framework
   - **Dependencies:** GPIO LEDs

## Driver Development Priorities

### Priority 1: Essential Platform Support
1. **GPIO/Pinctrl:** Foundation for all hardware control
2. **PWM Subsystem:** Required for fan and display brightness
3. **I2C Support:** Sensor communication
4. **ARISC/Power Management:** System stability

### Priority 2: Basic Projector Functionality  
1. **LED Status System:** User feedback and debugging
2. **Thermal Management:** Hardware protection
3. **Display Power Control:** Basic display functionality

### Priority 3: Advanced Features
1. **Motor Control System:** Keystone correction
2. **Environmental Sensors:** Auto-keystone functionality
3. **RF Kill Integration:** WiFi power management

## Implementation Roadmap

### Phase V.5 Completion Tasks
1. **Driver Source Analysis:** Identify available Sunxi drivers
2. **Mainline Compatibility:** Check H6/H713 driver compatibility
3. **Custom Driver Requirements:** List drivers needing custom implementation
4. **Integration Dependencies:** Map driver interdependencies

### Hardware Testing Phase Preparation
1. **GPIO Testing:** Verify pin assignments and basic control
2. **I2C Bus Testing:** Sensor communication validation
3. **PWM Testing:** Fan and brightness control validation
4. **Platform Driver Loading:** Test basic driver framework

### Full Integration Testing
1. **Motor Control:** Keystone adjustment functionality
2. **Auto-Keystone:** Complete sensor-to-motor pipeline
3. **Thermal Protection:** Temperature-based cooling
4. **Status Display:** LED indication system

## Technical Challenges and Solutions

### Challenge 1: Sunxi Driver Availability
**Problem:** Many Sunxi-specific drivers not in mainline
**Solution:** 
- Port from Allwinner kernel tree
- Implement simplified equivalents
- Use generic subsystem drivers where possible

### Challenge 2: MIPS Co-processor Coordination
**Problem:** Display control requires MIPS firmware coordination
**Solution:**
- Start with basic power control
- Implement communication protocol with MIPS firmware
- Coordinate timing with display pipeline

### Challenge 3: Motor Control Precision
**Problem:** Stepper motor requires precise timing
**Solution:**
- Use high-resolution timers
- Implement interrupt-driven stepping
- Add position feedback and calibration

### Challenge 4: Dual Accelerometer Integration  
**Problem:** Two sensors at same I2C address
**Solution:**
- Investigate I2C address configuration
- Implement sensor multiplexing if needed
- Use sensor fusion algorithms for accuracy

## Success Criteria

### Sub-task 5 Completion
- ✅ Projector-specific hardware components catalogued
- ✅ Driver requirements mapped to factory kernel configuration
- ✅ Mainline integration strategy defined
- ✅ Implementation priority matrix established

### Phase V Overall Success
- Complete driver integration roadmap
- All hardware components have defined implementation paths
- Testing methodology for each hardware subsystem
- Ready for driver compilation and FEL mode testing

## Documentation Cross-References

- **Hardware Specifications:** `docs/HY300_SPECIFIC_HARDWARE.md`
- **Factory Kernel Analysis:** `docs/ANDROID_KERNEL_DRIVER_ANALYSIS.md`
- **Device Tree:** `sun50i-h713-hy300.dts` (hardware configuration)
- **Hardware Status:** `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md`
- **Testing Methodology:** `docs/HY300_TESTING_METHODOLOGY.md`
- **Task Management:** `docs/tasks/009-phase5-driver-research.md`

---

**Analysis Completed:** Phase V.5 - Projector-specific hardware driver requirements documented  
**Next:** Complete driver integration roadmap (Phase V.6)  
**Status:** Ready for roadmap creation and hardware testing preparation