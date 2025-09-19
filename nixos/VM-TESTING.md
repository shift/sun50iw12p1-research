# HY300 VM Testing Guide

This guide explains how to build and test the HY300 NixOS configuration using a local VM.

## Building the VM

### Prerequisites
- NixOS system or Nix package manager installed
- At least 4GB RAM available for the VM
- QEMU/KVM support (optional but recommended for performance)

### Build Commands

1. **Build the VM image:**
```bash
nix build .#hy300-vm
```

2. **Run the VM:**
```bash
nix run .#vm
```

Alternatively, you can run the VM directly:
```bash
./result/bin/run-hy300-vm
```

### VM Configuration

The VM is configured with:
- **Memory**: 2GB RAM
- **Disk**: 8GB virtual disk
- **CPU**: 2 cores
- **Graphics**: Software rendering (no GPU acceleration)
- **Network**: NAT with port forwarding

### Port Forwarding

The VM forwards these ports to the host:
- **SSH**: `localhost:2222` → VM port 22
- **HTTP**: `localhost:8080` → VM port 80  
- **Kodi Web Interface**: `localhost:8888` → VM port 8080

### Accessing the VM

#### Via Console
The VM boots to an auto-login console as user `hy300` with password `test123`.

#### Via SSH
```bash
ssh -p 2222 hy300@localhost
# Password: test123
```

#### Via Web Browser
- **Kodi Web Interface**: http://localhost:8888
- **System HTTP**: http://localhost:8080

## Testing Features

### Kodi Media Center
- Automatically starts on boot
- Accessible via web interface at http://localhost:8888
- Custom HY300 plugins are loaded but in simulation mode

### HY300-Specific Features

#### Keystone Correction
- Plugin loads in simulation mode
- UI elements are functional
- Motor control commands are logged but don't affect hardware

#### WiFi Setup
- Plugin available through Kodi interface
- Uses VM's ethernet connection for testing
- Network scanning simulated

#### System Information
- Displays actual VM system stats
- Shows HY300 configuration status
- Hardware status shows "simulation mode"

### System Services

Check service status inside the VM:
```bash
# Check Kodi service
systemctl status kodi

# Check keystone service (simulation)
systemctl status hy300-keystone

# Check VM simulation service
systemctl status hy300-vm-simulation
```

## Development Testing

### Modifying Configuration
1. Edit configuration files in `nixos/`
2. Rebuild: `nix build .#hy300-vm`
3. Run updated VM: `./result/bin/run-hy300-vm`

### Testing Plugin Changes
1. Modify plugin configurations in `nixos/modules/hy300-projector.nix`
2. Rebuild and test in VM
3. Validate web interface functionality

### Performance Testing
The VM provides a realistic testing environment for:
- Kodi startup time and responsiveness
- Plugin loading and UI performance
- Service integration and dependencies
- Network configuration workflows

## Limitations

### Hardware Features Not Available
- **Motor Control**: Keystone motors are simulated
- **GPIO**: Hardware GPIO pins not accessible
- **MIPS Co-processor**: Display processing simulation only
- **Mali GPU**: Software rendering only
- **IR Remote**: No physical remote input

### VM-Specific Behaviors
- **Network**: Uses ethernet instead of WiFi
- **Audio**: Host audio system dependent
- **Graphics**: Software rendering may be slower
- **Storage**: Virtual disk, not SD card

## Troubleshooting

### VM Won't Start
- Check available RAM (needs 2GB+)
- Verify QEMU is installed
- Try building with `--show-trace` for detailed errors

### Kodi Not Loading
- Check VM console for error messages
- Verify auto-login is working
- SSH into VM and check `systemctl status kodi`

### Web Interface Not Accessible
- Verify port forwarding: `netstat -ln | grep 8888`
- Check VM firewall settings
- Ensure Kodi web interface is enabled in configuration

### Performance Issues
- Increase VM memory in `nixos/hy300-vm.nix`
- Enable KVM acceleration if available
- Reduce graphics quality in Kodi settings

## Deployment Testing

The VM environment closely mirrors the target hardware configuration:
- Same NixOS modules and services
- Identical Kodi configuration
- Compatible system layout and user setup

Successfully tested features in VM should work on real hardware with minimal changes.

## Next Steps

After VM testing validates the software stack:
1. Build ARM64 image for target hardware
2. Deploy to SD card for HY300 testing
3. Validate hardware-specific features
4. Performance tune for ARM64 platform