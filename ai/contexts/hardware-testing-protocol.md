# Hardware Testing Protocol Context

## Purpose
Comprehensive testing protocols for safe hardware validation using FEL recovery mode and systematic hardware deployment procedures.

## Testing Philosophy
- **Safety First**: FEL recovery mode ensures hardware is never permanently damaged
- **Incremental Validation**: Test each component systematically before integration
- **Complete Documentation**: All testing procedures documented for reproducibility
- **Risk Management**: Multiple recovery mechanisms and backup strategies

## FEL Mode Testing Framework

### FEL Mode Overview
**FEL (FELlback)** is Allwinner's emergency boot mode that allows complete system recovery via USB connection.

### FEL Mode Capabilities
- **Emergency Boot**: Boot custom firmware without touching eMMC
- **Memory Access**: Direct RAM and register access for debugging
- **Firmware Loading**: Load and test U-Boot without permanent installation
- **Hardware Probing**: Safe hardware detection and configuration testing
- **Recovery Operations**: Restore factory firmware if needed

### FEL Mode Entry Procedures
1. **Hardware Method**: Hold FEL button during power-on (if available)
2. **Software Method**: U-Boot command `reset fel` (if U-Boot accessible)
3. **Emergency Method**: Short eMMC pins to force FEL mode
4. **Verification**: `lsusb` shows Allwinner device in FEL mode

## Testing Hardware Requirements

### Essential Hardware Access
- **Serial Console**: UART access for boot messages and debugging
  - **Connection**: 3.3V TTL serial adapter
  - **Pinout**: TX/RX/GND pins (typically on debug headers)
  - **Settings**: 115200 baud, 8N1
  - **Safety**: 3.3V levels only, never 5V

- **USB Connection**: USB-C or micro-USB for FEL mode
  - **Cable**: USB data cable (not charge-only)
  - **Host**: Linux development machine with sunxi-tools
  - **Driver**: Proper USB drivers for Allwinner devices

- **Power Management**: Safe power cycling
  - **Power Button**: Hardware power control access
  - **Power Supply**: Stable 12V supply for projector
  - **Cooling**: Projector cooling system operational

### Development Tools Required
- **sunxi-tools**: FEL mode utilities (`sunxi-fel`, `sunxi-bootinfo`)
- **Device Tree Compiler**: `dtc` for device tree validation
- **Cross-compilation**: aarch64 toolchain in Nix environment
- **Serial Terminal**: `minicom`, `screen`, or `picocom`

## Systematic Testing Procedures

### Phase 1: Basic Hardware Validation
**Objective**: Verify basic hardware functionality and FEL recovery

1. **FEL Mode Entry Testing**
   ```bash
   # Verify FEL mode entry
   lsusb | grep -i allwinner
   sunxi-fel version
   sunxi-fel chipid
   ```

2. **DRAM Validation**
   ```bash
   # Test DRAM functionality
   sunxi-fel readl 0x40000000  # Test DRAM access
   sunxi-fel writel 0x40000000 0x12345678
   sunxi-fel readl 0x40000000  # Verify write/read
   ```

3. **U-Boot FEL Loading**
   ```bash
   # Load U-Boot via FEL (no eMMC modification)
   sunxi-fel -p uboot u-boot-sunxi-with-spl.bin
   ```

### Phase 2: U-Boot Functionality Testing
**Objective**: Validate U-Boot functionality without permanent installation

1. **Boot Message Validation**
   - Monitor serial console during U-Boot FEL loading
   - Verify DRAM initialization messages
   - Check device tree loading and parsing
   - Validate eMMC detection and partition access

2. **eMMC Access Testing**
   ```bash
   # U-Boot commands via serial console
   mmc info                    # Verify eMMC detection
   mmc part                    # List partition table
   fatls mmc 2:8              # Test super partition access
   ```

3. **Device Tree Validation**
   ```bash
   # Device tree loading verification
   fdt addr $fdtcontroladdr   # Check loaded device tree
   fdt print /soc             # Verify SoC configuration
   fdt print /memory          # Check memory configuration
   ```

### Phase 3: Kernel and Driver Testing
**Objective**: Test Linux kernel and driver loading in controlled environment

1. **Kernel Loading via FEL**
   ```bash
   # Load kernel and device tree via FEL
   sunxi-fel write 0x40080000 Image
   sunxi-fel write 0x4FA00000 sun50i-h713-hy300.dtb
   sunxi-fel exe 0x40080000
   ```

2. **Driver Module Testing**
   - Load MIPS co-processor driver
   - Test keystone motor control driver
   - Validate V4L2 HDMI input driver
   - Test new drivers (WiFi, GPU) incrementally

3. **Hardware Detection Validation**
   ```bash
   # Kernel hardware detection verification
   dmesg | grep -i "hy300"    # HY300-specific messages
   lsmod                      # Loaded kernel modules
   ls /dev/v4l/by-path/       # V4L2 device detection
   ```

## Hardware Component Testing

### WiFi Hardware Testing
1. **AIC8800 Detection**
   ```bash
   lsusb | grep -i aic        # USB device detection
   dmesg | grep -i wifi       # WiFi driver messages
   iw dev                     # WiFi interface detection
   ```

2. **Network Functionality**
   ```bash
   nmcli dev status           # NetworkManager status
   iwlist scan               # WiFi network scanning
   ping 8.8.8.8              # Basic connectivity test
   ```

### GPU Hardware Testing
1. **Mali GPU Detection**
   ```bash
   dmesg | grep -i mali       # GPU driver messages
   ls /dev/mali*             # GPU device nodes
   glxinfo | head -20        # OpenGL capability
   ```

2. **Graphics Performance**
   ```bash
   # Basic graphics testing
   glxgears                   # Simple graphics test
   kodi --debug              # Kodi with debug output
   ```

### IR Remote Testing
1. **IR Hardware Detection**
   ```bash
   dmesg | grep -i ir         # IR receiver messages
   ls /dev/lirc*             # LIRC device nodes
   ir-keytable               # IR protocol detection
   ```

2. **Remote Functionality**
   ```bash
   ir-ctl -r                 # Raw IR signal capture
   irw                       # Decoded IR events
   ```

## Safety and Recovery Procedures

### Emergency Recovery
1. **FEL Mode Recovery**
   - Always possible via USB connection
   - Does not modify eMMC content
   - Can restore factory firmware if needed
   - Hardware reset capabilities maintained

2. **Factory Firmware Restoration**
   ```bash
   # If needed, restore factory partitions
   sunxi-fel write 0x40000000 factory_backup.img
   # Restore via fastboot or dd commands
   ```

3. **Backup Strategy**
   - Complete eMMC backup before any modifications
   - Critical partition backups (boot, system)
   - U-Boot environment backup
   - Device tree backup

### Risk Mitigation
- **Never modify bootloader permanently** until thoroughly tested
- **Always test via FEL first** before eMMC installation
- **Maintain serial console access** for debugging
- **Keep factory recovery images** easily accessible
- **Document all modifications** for future reference

## Testing Documentation Requirements

### Test Planning
- **Test Objectives**: Clear goals for each testing phase
- **Success Criteria**: Measurable outcomes for validation
- **Risk Assessment**: Potential issues and mitigation strategies
- **Recovery Procedures**: Documented recovery paths

### Test Execution Documentation
- **Test Logs**: Complete serial console output capture
- **Hardware Observations**: Physical hardware behavior notes
- **Performance Metrics**: Boot times, functionality measurements
- **Issue Documentation**: Problems encountered and resolution

### Results Documentation
- **Validation Summary**: Pass/fail status for each test
- **Integration Status**: Components ready for production
- **Known Issues**: Documented limitations or workarounds
- **Next Steps**: Recommendations for further development

This comprehensive testing protocol ensures safe, systematic hardware validation while maintaining multiple recovery mechanisms and complete documentation of the testing process.