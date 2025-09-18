# HY300 Hardware Enablement Status Matrix

**Date:** September 18, 2025  
**Phase:** IV - Mainline Device Tree Creation  
**Status:** COMPLETED  

## Device Tree Components Status

### ✅ **Core SoC Components (IMPLEMENTED)**

| Component | Status | Driver Compatibility | Notes |
|-----------|--------|---------------------|-------|
| **CPU** | ✅ Complete | ARM Cortex-A53 (mainline) | Quad-core ARM64, PSCI power management |
| **Memory** | ✅ Complete | Standard DDR3 | 2GB default, DDR3-1600 from boot0.bin analysis |
| **Clocks** | ✅ Complete | sun50i-h6-ccu | H6-compatible CCU with tv303 extensions |
| **Interrupts** | ✅ Complete | ARM GIC-400 | Standard ARM Generic Interrupt Controller |
| **GPIO/Pinctrl** | ✅ Complete | sun50i-h6-pinctrl | H6-compatible pin controller |
| **DMA** | ✅ Complete | sun50i-h6-dma | H6-compatible DMA controller |
| **Timers** | ✅ Complete | ARM architected timer | Standard ARM64 timer support |
| **Watchdog** | ✅ Complete | sun50i-h6-wdt | H6-compatible watchdog timer |

### ✅ **Storage and Connectivity (IMPLEMENTED)**

| Component | Status | Driver Compatibility | Notes |
|-----------|--------|---------------------|-------|
| **eMMC** | ✅ Complete | sun50i-h6-emmc | Primary storage, 8-bit bus, HS200 support |
| **UART0** | ✅ Complete | snps,dw-apb-uart | Debug console (115200n8) |
| **I2C1** | ✅ Complete | sun50i-h6-i2c | Sensor bus for accelerometers |

### ✅ **Graphics and Display (IMPLEMENTED)**

| Component | Status | Driver Compatibility | Notes |
|-----------|--------|---------------------|-------|
| **GPU** | ✅ Complete | arm,mali-midgard | Mali-Midgard GPU with power management |
| **MIPS Co-processor** | ✅ Complete | allwinner,sunxi-mipsloader | Display subsystem co-processor |
| **Reserved Memory** | ✅ Complete | Standard DT reservation | 40.3MB MIPS region + 128KB decoder |

### ✅ **Projector-Specific Hardware (IMPLEMENTED)**

| Component | Status | Driver Compatibility | Notes |
|-----------|--------|---------------------|-------|
| **Keystone Motor** | ✅ Complete | Custom hy300,keystone-motor | 4-phase stepper motor with limit switch |
| **Accelerometers** | ✅ Complete | stk8ba58 / kxtj3-1057 | Auto-keystone correction sensors |
| **Status LEDs** | ✅ Complete | gpio-leds | RGB status indicators |
| **Thermal Management** | ✅ Complete | pwm-fan + sun50i-h6-ths | PWM fan control with thermal sensor |
| **Panel Power** | ✅ Complete | regulator-fixed | LCD panel and backlight control |

### 🔄 **Network Connectivity (BASIC IMPLEMENTATION)**

| Component | Status | Driver Compatibility | Notes |
|-----------|--------|---------------------|-------|
| **WiFi Module** | 🔄 Basic | aicsemi,aic8800 | **Phase V**: Requires proprietary driver integration |

## Memory Map Configuration

### **Reserved Memory Regions**
- **MIPS Loader:** `0x4b100000` - `0x4d941000` (40.3MB)
- **Video Decoder:** `0x4d941000` - `0x4d961000` (128KB)

### **Hardware Register Mappings**
- **GPU:** `0x01800000` (64KB register space)
- **MIPS Controller:** `0x03061000` (256 bytes)
- **System Control:** `0x03000000` (H6-compatible)
- **CCU:** `0x03001000` (H6-compatible)

## GPIO Assignments (Factory DTB Verified)

### **Motor Control System**
- **Phase Control:** PH4, PH5, PH6, PH7 (stepper motor phases)
- **Limit Switch:** PH14 (end-of-travel detection)

### **Power and Status**
- **Panel Power:** PH19 (LCD panel enable)
- **Backlight:** PB5 (LED light source enable)
- **Fan Control:** PH17 (cooling fan enable)
- **Status LEDs:** PL0 (red), PL1 (green), PL5 (blue)

### **Communication Interfaces**
- **Debug UART:** PH0, PH1 (console access)
- **Sensor I2C:** PB18, PB19 (accelerometer bus)

## Integration Requirements

### **U-Boot Integration**
- Device tree path: `arch/arm64/boot/dts/allwinner/sun50i-h713-hy300.dts`
- Compatible with existing H6 U-Boot configuration
- Memory regions configured for MIPS co-processor

### **Kernel Requirements**
- **Base Support:** CONFIG_ARCH_SUNXI, CONFIG_ARM64
- **MIPS Loader:** CONFIG_SUNXI_MIPSLOADER (if available)
- **Mali GPU:** CONFIG_DRM_PANFROST or Mali Midgard driver
- **Sensors:** CONFIG_IIO for accelerometer support

### **Firmware Dependencies**
- **MIPS Firmware:** `display.bin` (Phase III: Located in Android super.img)
- **GPU Firmware:** Mali Midgard firmware files

## Testing Readiness Assessment

### ✅ **Safe Testing Environment**
- **Recovery Method:** FEL mode via USB (sunxi-fel tools)
- **Bootloader:** U-Boot with device tree support ready
- **Serial Console:** UART0 configured for debug access
- **Non-destructive:** No modification to factory firmware

### 🎯 **Phase V Testing Priorities**
1. **Basic Boot:** Serial console output and kernel panic resolution
2. **Storage Access:** eMMC detection and filesystem mounting  
3. **Hardware Detection:** GPIO functionality and sensor access
4. **MIPS Integration:** Co-processor initialization and firmware loading
5. **Display Output:** GPU driver integration and projection testing

## Risk Assessment

### **Low Risk Components** ✅
- All core SoC functionality (CPU, memory, clocks, GPIO)
- Standard peripherals (UART, I2C, eMMC)
- Status LEDs and basic power management

### **Medium Risk Components** ⚠️
- MIPS co-processor integration (custom hardware)
- Mali GPU driver compatibility
- Accelerometer sensor detection
- Motor control system functionality

### **Controlled Testing** 🛡️
- FEL mode provides complete recovery capability
- Factory firmware backup maintained
- Incremental hardware enablement approach
- Serial console for debugging throughout

## Phase V Integration Roadmap

### **Driver Development Priorities**
1. **MIPS Loader Driver** - Port sunxi-mipsloader for mainline
2. **Mali GPU Driver** - Integrate Midgard support or Panfrost
3. **WiFi Driver** - AIC8800 proprietary driver integration
4. **Custom Drivers** - Keystone motor control driver

### **Hardware Validation Plan**
1. **Boot Testing** - Kernel boot via FEL mode with serial console
2. **Storage Testing** - eMMC detection and filesystem access
3. **Sensor Testing** - I2C accelerometer detection and data reading
4. **Display Testing** - MIPS co-processor initialization and display.bin loading
5. **Integration Testing** - Full projector functionality validation

---

## Summary

**Phase IV Status: ✅ COMPLETED**

- **Device Tree Created:** Complete `sun50i-h713-hy300.dts` with all hardware components
- **Compilation Verified:** DTB compiles successfully (10.5KB output)
- **Hardware Mapped:** All projector-specific components configured
- **Testing Ready:** Safe testing methodology established with FEL recovery

**Next Phase Ready:** Phase V driver integration can begin with complete hardware specification.