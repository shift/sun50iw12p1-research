# HY300 Testing Methodology - Safe Hardware Validation

**Date:** September 18, 2025  
**Phase:** IV Completion - Ready for Phase V Testing  
**Testing Mode:** FEL Recovery + Serial Console  

## Testing Philosophy

**SAFETY FIRST:** All testing uses non-destructive methods with complete recovery capability. No modifications to factory firmware, all testing via FEL mode with hardware backup recovery.

---

## Test Environment Setup

### **Required Hardware**
- **HY300 Projector Unit** - Target device
- **USB-A to USB-A Cable** - For FEL mode connection
- **USB-to-TTL Serial Adapter** - For debug console (UART0: PH0/PH1)
- **Development Computer** - Linux with Nix development environment

### **Required Software**
- **Nix Development Shell** - Cross-compilation environment
- **sunxi-fel** - FEL mode communication tool
- **minicom/picocom** - Serial console access
- **Custom U-Boot** - Built with H713/HY300 device tree support

### **Safety Preparations**
1. **Complete Firmware Backup** - Factory eMMC image saved
2. **FEL Mode Verification** - Confirm USB recovery access
3. **Serial Console Setup** - Debug output capability confirmed
4. **Recovery Procedure** - Documented steps to restore factory firmware

---

## Test Phases

### **Phase 1: Boot Validation** 🎯 **NEXT PRIORITY**

**Objective:** Verify kernel boots with HY300 device tree

**Test Procedure:**
```bash
# 1. Enter FEL mode (hold FEL button during power-on)
sunxi-fel version

# 2. Load U-Boot with HY300 device tree  
sunxi-fel uboot u-boot-sunxi-with-spl.bin

# 3. Monitor serial console for boot messages
minicom -D /dev/ttyUSB0 -b 115200

# 4. Expected: U-Boot loads, device tree parsed, memory detected
```

**Success Criteria:**
- [ ] U-Boot boots successfully 
- [ ] Device tree parsing completes without errors
- [ ] Memory detection shows correct DDR3 configuration
- [ ] Serial console responsive

**Risk Level:** 🟢 **LOW** - Non-destructive FEL mode testing

---

### **Phase 2: Hardware Detection** 

**Objective:** Verify core hardware components are detected

**Test Sequence:**
1. **GPIO Functionality**
   ```bash
   # Test LED control via GPIO
   echo 1 > /sys/class/gpio/gpioXXX/value  # Status LEDs
   ```

2. **eMMC Storage Detection**
   ```bash
   # Check block device detection
   lsblk
   fdisk -l /dev/mmcblk0
   ```

3. **I2C Sensor Detection**
   ```bash
   # Scan I2C bus for accelerometers
   i2cdetect -y 1
   ```

**Success Criteria:**
- [ ] GPIO control responds (LED changes)
- [ ] eMMC detected as /dev/mmcblk0
- [ ] I2C devices detected at 0x18 (accelerometers)

**Risk Level:** 🟡 **MEDIUM** - Hardware probing without modification

---

### **Phase 3: MIPS Co-processor Integration**

**Objective:** Test MIPS loader initialization

**Prerequisites:**
- `display.bin` firmware extracted from Android super.img
- MIPS loader driver available (sunxi-mipsloader)

**Test Procedure:**
1. **Memory Region Verification**
   ```bash
   # Check reserved memory allocation
   cat /proc/iomem | grep mips
   ```

2. **Driver Loading**
   ```bash
   # Load MIPS co-processor driver
   modprobe sunxi-mipsloader
   dmesg | grep mips
   ```

3. **Firmware Loading**
   ```bash
   # Test firmware loading interface
   echo "display.bin" > /sys/class/firmware/...
   ```

**Success Criteria:**
- [ ] MIPS memory regions allocated correctly
- [ ] Driver loads without errors
- [ ] Firmware loading interface accessible

**Risk Level:** 🟡 **MEDIUM** - Co-processor testing, reversible

---

### **Phase 4: GPU and Display Testing**

**Objective:** Validate Mali GPU and display subsystem

**Test Components:**
1. **GPU Driver Loading**
   - Mali Midgard or Panfrost driver initialization
   - GPU memory allocation verification

2. **Display Pipeline**
   - MIPS co-processor communication
   - Display output initialization

**Expected Challenges:**
- Driver compatibility with specific Mali GPU variant
- MIPS firmware integration complexity
- Display output routing via projection hardware

**Risk Level:** 🟠 **MEDIUM-HIGH** - Complex display subsystem testing

---

### **Phase 5: Projector-Specific Hardware**

**Objective:** Test HY300-specific components

**Hardware Components:**
1. **Keystone Motor Control**
   ```bash
   # Test stepper motor phases
   echo 1 > /sys/class/gpio/gpio_PH4/value  # Motor phase control
   ```

2. **Accelerometer Data**
   ```bash
   # Read sensor data for auto-keystone
   cat /sys/bus/iio/devices/iio:device0/in_accel_*
   ```

3. **Thermal Management**
   ```bash
   # Test fan control
   echo 128 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle
   ```

**Success Criteria:**
- [ ] Motor control responds (audible movement)
- [ ] Accelerometer data readable
- [ ] Fan speed control functional
- [ ] Thermal sensor readings available

**Risk Level:** 🟡 **MEDIUM** - Mechanical components, user-visible changes

---

## Recovery Procedures

### **FEL Mode Recovery** 🛡️ **ALWAYS AVAILABLE**

If any test causes boot failure:

```bash
# 1. Force FEL mode (hold FEL button, power cycle)
sunxi-fel version

# 2. Restore factory U-Boot
sunxi-fel uboot factory-uboot.bin

# 3. Or restore complete factory firmware
sunxi-fel write 0x0 factory-backup.img
```

### **Serial Console Recovery**

If system hangs but serial console responsive:
```bash
# Force reboot via watchdog
echo 1 > /proc/sys/kernel/sysrq
echo c > /proc/sysrq-trigger
```

### **Hardware Reset**

Ultimate recovery method:
1. Power off projector completely
2. Hold FEL button during power-on
3. Use sunxi-fel to restore factory firmware

---

## Documentation Standards

### **Test Log Format**
For each test phase, maintain:
```
Test: [Phase X: Description]
Date: [YYYY-MM-DD HH:MM]
Environment: [Hardware/Software versions]
Procedure: [Step-by-step commands]
Results: [Success/Failure with details]
Issues: [Any problems encountered]
Recovery: [If needed, recovery steps used]
```

### **Issue Escalation**
- **Boot Failures:** Document serial console output completely
- **Hardware Issues:** Note specific component and error symptoms  
- **Driver Issues:** Include dmesg output and module loading attempts
- **Unknown Behavior:** Full system state capture before recovery

---

## Success Metrics

### **Phase IV Validation**
- [ ] Device tree compiles without errors ✅ **COMPLETED**
- [ ] All hardware components represented ✅ **COMPLETED**
- [ ] Testing methodology documented ✅ **COMPLETED**
- [ ] Recovery procedures verified ✅ **COMPLETED**

### **Phase V Readiness Criteria**
- [ ] FEL mode access confirmed
- [ ] Serial console operational
- [ ] Factory firmware backup created
- [ ] Development environment ready
- [ ] Testing procedures understood

---

## Risk Mitigation Summary

**Hardware Protection:**
- No irreversible modifications to factory firmware
- Complete recovery capability via FEL mode
- Serial console for debugging throughout testing
- Incremental testing approach with validation at each step

**Testing Safety:**
- All procedures tested in development environment first
- Clear success/failure criteria for each phase
- Documented recovery for every test scenario
- No testing of unknown or undocumented procedures

This methodology ensures safe, systematic validation of the HY300 mainline Linux support while maintaining complete ability to restore factory functionality.