# VM Testing and Validation Context

## Overview
Phase VIII focuses on complete software stack validation using NixOS VMs, enabling development and testing without hardware access requirements.

## VM Testing Framework Architecture

### NixOS VM Configuration
**Base Configuration**: `/nixos/hy300-vm.nix`
- **Purpose**: Complete HY300 software stack simulation
- **Components**: Kodi, services, drivers (simulation mode)
- **Network**: Simulated hardware interfaces for testing

### Service Integration
**Python Services**: Real implementations, not mocks
- **Keystone Service**: Motor control simulation with parameter validation
- **WiFi Management**: Network configuration with simulated hardware responses
- **Remote Input**: IR code processing with virtual event generation
- **HDMI Input**: V4L2 simulation for input source management

### Testing Methodology
**Validation Approach**: 
1. **Boot Testing**: NixOS VM starts successfully with all services
2. **Service Coordination**: Proper startup sequence and dependencies
3. **Interface Testing**: All APIs respond correctly to test inputs
4. **Integration Testing**: Services communicate correctly with each other
5. **Resource Testing**: Memory, CPU, and storage usage within acceptable limits

## VM Testing Procedures

### Initial Setup
```bash
# Build HY300 VM image
nix build .#hy300-vm

# Start VM with graphics
./result/bin/run-hy300-vm

# Monitor service status
systemctl status hy300-services
journalctl -f -u hy300-*
```

### Service Testing Protocol
**Keystone Service Testing**:
```bash
# Test keystone parameter updates
echo "h_keystone=50" > /sys/class/hy300/keystone/parameters
cat /sys/class/hy300/keystone/status

# Monitor motor simulation
journalctl -f -u hy300-keystone
```

**Kodi Integration Testing**:
```bash
# Verify Kodi starts with HY300 configuration
systemctl status kodi
ls -la /home/kodi/.kodi/userdata/keymaps/hy300-keymap.xml

# Test HDMI input addon
kodi-send --action="RunAddon(plugin.video.hy300-hdmi-input)"
```

**Network Service Testing**:
```bash
# Test WiFi management
systemctl status hy300-wifi-manager
curl -X POST http://localhost:8080/api/wifi/scan

# Test remote input processing
echo "0x40BF40BF" > /dev/input/by-id/hy300-remote
journalctl -f -u hy300-remote-input
```

### Performance Validation
**Resource Monitoring**:
- **Memory Usage**: All services under 512MB combined
- **CPU Usage**: Idle system under 10% CPU utilization
- **Boot Time**: Complete service startup under 30 seconds
- **Storage**: Total system under 2GB compressed

**Service Response Times**:
- **Keystone Updates**: Parameter changes under 100ms
- **HDMI Input Switch**: Source changes under 500ms
- **WiFi Operations**: Network scans under 5 seconds
- **Remote Input**: IR code processing under 50ms

## Integration Testing Scenarios

### Startup Sequence Validation
1. **System Boot**: NixOS VM boots to login prompt
2. **Service Initialization**: All hy300-* services start successfully  
3. **Kodi Startup**: Media center loads with HY300 configuration
4. **Network Services**: WiFi manager and remote input become responsive
5. **Metrics Collection**: Prometheus begins collecting service metrics

### Functional Testing Scenarios
**Keystone Correction Testing**:
- Parameter updates via sysfs interface
- Motor position simulation responses
- Boundary condition handling (min/max values)
- Error handling for invalid parameters

**HDMI Input Testing**:
- Source detection simulation
- Video format reporting
- Input switching operations
- Error handling for disconnected sources

**Remote Input Testing**:
- IR code recognition and mapping
- Kodi navigation integration
- Custom command handling
- Multiple remote protocol support

### Error Handling Validation
**Service Failure Recovery**:
- Individual service restart capabilities
- Dependency handling during failures
- Clean shutdown procedures
- Log generation during error conditions

**Resource Limit Testing**:
- Memory exhaustion handling
- CPU spike recovery
- Storage space management
- Network connectivity loss simulation

## Success Criteria

### Functional Requirements
- [ ] VM boots successfully with all HY300 services
- [ ] Kodi loads and displays HY300-specific interface
- [ ] All Python services respond to API requests
- [ ] Simulated hardware interfaces accept commands
- [ ] Service coordination maintains proper dependencies
- [ ] Metrics collection functions without errors

### Performance Requirements  
- [ ] Boot time under 30 seconds from VM start to service ready
- [ ] Memory usage under 512MB for all services combined
- [ ] CPU usage under 10% during idle operation
- [ ] All API responses under documented time limits
- [ ] Clean shutdown under 10 seconds

### Integration Requirements
- [ ] Keystone service accepts parameter updates
- [ ] HDMI input service simulates source switching
- [ ] WiFi manager handles network operations
- [ ] Remote input processes IR codes correctly
- [ ] Prometheus collects metrics from all services
- [ ] All services log appropriately to journald

## Validation Tools

### Automated Testing
**Test Scripts**: `/nixos/tests/`
- `hy300-vm-boot-test.nix` - VM startup validation
- `hy300-services-test.nix` - Service functionality testing
- `hy300-integration-test.nix` - Cross-service integration
- `hy300-performance-test.nix` - Resource usage validation

### Manual Testing Procedures
**Service Verification**: `docs/HY300_TESTING_METHODOLOGY.md`
- Step-by-step service testing procedures
- Expected output examples for each test
- Troubleshooting guides for common issues
- Performance benchmarking procedures

### Monitoring and Debugging
**VM Monitoring**:
```bash
# Real-time service monitoring
watch -n 1 'systemctl status hy300-* --no-pager -l'

# Performance monitoring
htop
iotop
nethogs

# Service-specific debugging
journalctl -f -u hy300-keystone -u hy300-wifi-manager
```

## Troubleshooting Common Issues

### Service Startup Failures
**Symptoms**: Services fail to start or report errors
**Investigation**:
1. Check service logs: `journalctl -u <service-name>`
2. Verify configuration files exist and are readable
3. Check dependency services are running
4. Validate file permissions and ownership

### Simulation Interface Errors
**Symptoms**: Hardware simulation not responding
**Investigation**:
1. Verify mock device files exist in `/sys/class/hy300/`
2. Check service can write to simulated interfaces
3. Validate parameter formats and ranges
4. Test with minimal parameter sets

### VM Performance Issues
**Symptoms**: Slow response or high resource usage
**Investigation**:
1. Monitor resource usage with `htop`, `iotop`
2. Check for memory leaks in Python services
3. Validate VM memory and CPU allocation
4. Review service polling intervals and optimization

This VM testing framework ensures complete software validation before hardware deployment, reducing risk and enabling rapid development cycles.