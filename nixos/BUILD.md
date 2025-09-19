# HY300 NixOS Media Player Build Guide

## Overview
This directory contains a complete NixOS configuration for transforming the HY300 Android projector into a dedicated media center running mainline Linux with Kodi.

## System Components

### Core Features
- **Kodi Media Center**: Auto-starting with hardware acceleration
- **Keystone Correction**: Both automatic (accelerometer-based) and manual adjustment
- **IR Remote Control**: Complete WiFi setup and system control via remote
- **Bluetooth Audio**: Seamless audio device pairing and management
- **WiFi Management**: Remote-controlled network configuration
- **Hardware Drivers**: Complete driver support for HY300-specific hardware

### Architecture
- **Target Platform**: ARM64 (aarch64-linux) for Allwinner H713
- **Base System**: NixOS 24.05 with cross-compilation support
- **Boot Method**: U-Boot with device tree integration
- **Storage**: SD card or eMMC deployment

## Directory Structure

```
nixos/
├── modules/                          # NixOS modules
│   ├── hy300-projector.nix          # Main system configuration
│   ├── hy300-hardware.nix           # Hardware-specific settings
│   └── hy300-services.nix           # System services
├── packages/                        # Custom packages
│   ├── hy300-drivers.nix            # Kernel drivers
│   ├── hy300-keystone.nix           # Keystone correction service
│   ├── hy300-bluetooth.nix          # Bluetooth audio management
│   └── kodi-hy300-plugins.nix       # Kodi plugins
├── hy300-image.nix                  # Complete system image
├── hy300-minimal.nix                # Minimal test configuration
└── BUILD.md                         # This file
```

## Build Requirements

### Development Environment
```bash
# Enter the HY300 development environment
nix develop

# The environment provides:
# - Cross-compilation toolchain (aarch64-unknown-linux-gnu-*)
# - Device tree compiler (dtc)
# - Sunxi tools for hardware flashing
# - Firmware analysis tools
```

### Dependencies
- NixOS or Nix package manager
- Cross-compilation support for ARM64
- At least 8GB free disk space for builds
- Internet connection for package downloads

## Building the System

### Option 1: Complete System Image
```bash
# Build complete SD card image
nix build .#hy300-projector.config.system.build.sdImage

# Result will be in ./result/sd-image/
# Image size: ~2GB compressed, ~8GB uncompressed
```

### Option 2: Individual Components

#### Build Kernel Modules
```bash
# Build HY300-specific drivers
nix build .#hy300-drivers

# Includes:
# - MIPS co-processor driver (sunxi-cpu-comm)
# - MIPS firmware loader (sunxi-mipsloader)
# - Keystone motor driver (hy300-keystone-motor)
# - HDMI input capture (sunxi-tvcap-enhanced)
```

#### Build System Services
```bash
# Build keystone correction service
nix build .#hy300-keystone-service

# Build Bluetooth audio management
nix build .#hy300-bluetooth-audio

# Build Kodi plugins
nix build .#kodi-hy300-plugins
```

#### Build Configuration Packages
```bash
# Build complete configuration
nix build .#hy300-projector.config.system.build.toplevel

# Build specific configurations
nix build .#hy300-projector.config.boot.kernelPackages.kernel
nix build .#hy300-projector.config.system.build.kernel
```

## Deployment Options

### SD Card Deployment (Recommended for Testing)
```bash
# 1. Build SD card image
nix build .#hy300-projector.config.system.build.sdImage

# 2. Flash to SD card (replace /dev/sdX with your SD card)
sudo dd if=./result/sd-image/nixos-sd-image-*-aarch64-linux.img of=/dev/sdX bs=4M status=progress

# 3. Insert SD card and boot HY300 in SD card mode
```

### eMMC Deployment (Production)
```bash
# 1. Boot from SD card first
# 2. Use FEL mode for safe eMMC flashing
sunxi-fel -p spl u-boot-sunxi-with-spl.bin
sunxi-fel -p write 0x4000000 nixos-system.img

# 3. Configure eMMC partitions and install system
# (Detailed instructions in hardware testing phase)
```

### Network Deployment
```bash
# 1. Enable SSH in configuration
# 2. Deploy via NixOS rebuild
nixos-rebuild switch --target-host projector@hy300-projector.local --flake .#hy300-projector
```

## Configuration Options

### Hardware Configuration
```nix
services.hy300-projector = {
  enable = true;
  
  hardware = {
    deviceTree = "sun50i-h713-hy300.dtb";
    drivers = {
      enableAll = true;
      mipsCoprocessor = true;    # MIPS display processor
      keystoneMotor = true;      # Physical keystone motor
      hdmiInput = true;          # HDMI input capture
      wifiAIC8800 = true;        # WiFi driver
    };
  };
};
```

### Media Center Configuration
```nix
mediaCenter = {
  enable = true;
  autoStart = true;              # Start Kodi on boot
  hardwareAcceleration = true;   # Mali GPU acceleration
  customPlugins = true;          # HY300-specific plugins
  audioOutput = "auto";          # HDMI/Bluetooth switching
};
```

### Keystone Correction
```nix
keystoneCorrection = {
  enable = true;
  autoCorrection = true;         # Accelerometer-based
  manualInterface = true;        # Manual 4-corner adjustment
  motorControl = true;           # Physical motor adjustment
};
```

### Network Services
```nix
network = {
  wifi = {
    enable = true;
    remoteSetup = true;          # IR remote WiFi configuration
    profileManagement = true;    # Save/load profiles
  };
  bluetooth = {
    enable = true;
    audioDevices = true;         # Audio device support
    hidDevices = true;           # Keyboard/mouse support
    autoReconnect = true;        # Automatic reconnection
  };
};
```

## System Services

### Keystone Correction Service
```bash
# Control keystone correction
hy300-keystone-control auto on          # Enable auto correction
hy300-keystone-control motor 10          # Adjust motor position
hy300-keystone-control digital corners   # Set digital correction
hy300-keystone-control save profile1     # Save current settings
hy300-keystone-control load profile1     # Load saved settings
hy300-keystone-control status            # Show current status
```

### Bluetooth Audio Management
```bash
# Manage Bluetooth audio devices
hy300-bluetooth-control scan 30          # Scan for devices
hy300-bluetooth-control discoverable 60  # Make discoverable
hy300-bluetooth-control pair MAC NAME    # Pair with device
hy300-bluetooth-control connect MAC NAME # Connect to device
hy300-bluetooth-control audio-hdmi       # Switch to HDMI audio
hy300-bluetooth-control status           # Show status
```

### WiFi Configuration
```bash
# WiFi management via IR remote
# Use Kodi WiFi plugin or direct service commands
echo "scan" > /run/wifi-setup-control
echo "connect:SSID:password" > /run/wifi-setup-control
```

## Kodi Plugins

### HY300 Keystone Correction Plugin
- **Location**: Add-ons → Program add-ons → HY300 Keystone Correction
- **Features**:
  - Manual 4-corner adjustment interface
  - Automatic correction enable/disable
  - Profile save/load system
  - Real-time adjustment preview
  - Calibration tools

### HY300 WiFi Setup Plugin  
- **Location**: Add-ons → Program add-ons → HY300 WiFi Setup
- **Features**:
  - Network scanning and selection
  - Password entry via remote control
  - QR code generation for mobile assistance
  - Connection status monitoring
  - Profile management

## Testing and Validation

### Hardware Testing Checklist
- [ ] U-Boot boots successfully
- [ ] Device tree loaded correctly
- [ ] All kernel modules load without errors
- [ ] Kodi starts automatically
- [ ] IR remote responds to input
- [ ] WiFi scanning and connection works
- [ ] Bluetooth audio device pairing works
- [ ] Keystone correction functional
- [ ] HDMI output working at 1080p
- [ ] All system services running

### Performance Validation
- [ ] Video playback smooth (1080p H.264)
- [ ] Audio output clear (HDMI/Bluetooth)
- [ ] Remote control responsive
- [ ] System boot time < 30 seconds
- [ ] Memory usage < 1GB idle
- [ ] Temperature management functional

## Troubleshooting

### Common Issues

#### Boot Problems
```bash
# Check U-Boot configuration
# Verify device tree compilation
dtc -I dtb -O dts sun50i-h713-hy300.dtb > hy300.dts

# Check kernel loading
dmesg | grep -i "hy300\|sunxi\|allwinner"
```

#### Driver Issues
```bash
# Check loaded modules
lsmod | grep -E "sunxi|hy300"

# Check hardware devices
ls -la /dev/mips-loader /dev/keystone-motor /dev/video0

# Check system logs
journalctl -u hy300-keystone.service
journalctl -u hy300-bluetooth-audio.service
```

#### Network Problems
```bash
# Check WiFi status
nmcli device status
nmcli connection show

# Check Bluetooth status
bluetoothctl show
```

#### Kodi Issues
```bash
# Check Kodi service
systemctl status kodi-hy300.service

# Check Kodi logs
journalctl -u kodi-hy300.service

# Check hardware acceleration
kodi-send --action="ActivateWindow(systeminfo)"
```

### Recovery Procedures

#### SD Card Recovery
1. Flash new SD card image
2. Boot from SD card
3. Fix configuration issues
4. Re-deploy to eMMC

#### FEL Mode Recovery
1. Enter FEL mode (USB recovery)
2. Flash U-Boot via sunxi-fel
3. Boot minimal system
4. Restore full configuration

#### Configuration Reset
```bash
# Reset to defaults
rm -rf /var/lib/hy300/*
systemctl restart hy300-keystone.service
systemctl restart hy300-bluetooth-audio.service
```

## Integration with Project

### Existing Components
This NixOS configuration integrates with:
- **U-Boot bootloader**: Built in Phase II (`u-boot-sunxi-with-spl.bin`)
- **Device tree**: Created in Phase IV (`sun50i-h713-hy300.dts`)
- **Kernel drivers**: Developed in Phase V (`drivers/` directory)
- **Firmware files**: Extracted in Phase III (`firmware/extracted_components/`)

### Hardware Dependencies
- **Required for full functionality**: Physical HY300 hardware
- **Testing without hardware**: Limited to software component validation
- **FEL mode access**: Required for safe hardware deployment

### Future Enhancements
- **Mobile app integration**: Smartphone companion app
- **Voice control**: Integration with voice assistants
- **Smart home**: IoT device control features
- **Screen mirroring**: Wireless display capabilities

## Performance Characteristics

### System Requirements
- **RAM**: 2GB (1GB minimum)
- **Storage**: 8GB (4GB minimum)
- **CPU**: ARM64 Cortex-A53 (Allwinner H713)
- **GPU**: Mali-T720MP2 with hardware acceleration

### Expected Performance
- **Boot time**: 20-30 seconds to Kodi interface
- **Video playback**: 1080p H.264/H.265 smooth
- **Audio latency**: <100ms (Bluetooth), <50ms (HDMI)
- **Remote responsiveness**: <200ms input lag
- **WiFi performance**: 802.11ac speeds (device dependent)

## Security Considerations

### Network Security
- SSH access restricted by default
- WiFi WPA2/WPA3 support
- Bluetooth authentication required

### System Security
- Read-only root filesystem (NixOS immutability)
- Service isolation via systemd
- Minimal attack surface (embedded configuration)

### User Data
- Configuration stored in `/var/lib/hy300/`
- Kodi library in `/home/projector/.kodi/`
- Network profiles encrypted storage

## License and Attribution

This NixOS configuration is part of the HY300 Linux porting project:
- **License**: GPL-3.0 (compatible with Linux kernel)
- **Attribution**: HY300 Project contributors
- **Dependencies**: NixOS, Linux kernel, Kodi, various FOSS projects

## Support and Development

### Development Environment
```bash
# Clone project
git clone <repository>
cd android_projector

# Enter development environment
nix develop

# Make changes to nixos/ directory
# Test changes with minimal configuration
```

### Contributing Guidelines
1. Test all changes with minimal configuration first
2. Document new features in module comments
3. Update this BUILD.md with any new procedures
4. Follow NixOS module conventions
5. Ensure cross-compilation compatibility

### Reporting Issues
- **Hardware-specific issues**: Require physical device access
- **Software issues**: Can be reproduced in development environment
- **Configuration issues**: Include full system logs and configuration

---

**Status**: ✅ **Software Development Complete**  
**Next Phase**: Hardware testing and validation with physical HY300 device  
**Ready for**: SD card deployment and hardware testing phase