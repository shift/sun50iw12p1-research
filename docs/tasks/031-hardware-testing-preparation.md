# Task 031: Hardware Testing Preparation

## Status
- **Current Status**: pending
- **Priority**: high
- **Estimated Effort**: 3-4 days
- **Dependencies**: Device Tree Completion (Task 030)

## Objective
Prepare complete hardware testing framework using FEL mode for safe validation of U-Boot, kernel, drivers, and complete system integration before production deployment.

## Background
With complete software stack developed (U-Boot, kernel, drivers, services, device tree), this task prepares comprehensive hardware testing procedures to validate the entire system safely using FEL recovery mode.

## Success Criteria
- [ ] FEL mode testing procedures documented and validated
- [ ] Complete hardware testing checklist created
- [ ] Automated testing scripts developed for systematic validation
- [ ] Hardware recovery procedures documented and tested
- [ ] Performance benchmarking framework established
- [ ] Production deployment procedures prepared

## Implementation Steps

### Phase 1: FEL Mode Testing Framework
1. **FEL Testing Infrastructure Setup**
   ```bash
   #!/bin/bash
   # fel-test-framework.sh - Complete FEL testing framework
   
   set -euo pipefail
   
   # Configuration
   UBOOT_BINARY="u-boot-sunxi-with-spl.bin"
   KERNEL_IMAGE="Image"
   DTB_FILE="sun50i-h713-hy300.dtb"
   ROOTFS_IMAGE="rootfs.cpio.gz"
   
   # FEL testing functions
   fel_enter_mode() {
       echo "Checking FEL mode entry..."
       if ! lsusb | grep -qi allwinner; then
           echo "ERROR: Device not in FEL mode"
           echo "Please enter FEL mode and retry"
           exit 1
       fi
       echo "✓ Device in FEL mode"
   }
   
   fel_load_uboot() {
       echo "Loading U-Boot via FEL..."
       sunxi-fel -p uboot "${UBOOT_BINARY}"
       echo "✓ U-Boot loaded successfully"
   }
   
   fel_load_kernel() {
       echo "Loading kernel and device tree..."
       sunxi-fel write 0x40080000 "${KERNEL_IMAGE}"
       sunxi-fel write 0x4FA00000 "${DTB_FILE}"
       sunxi-fel write 0x4FC00000 "${ROOTFS_IMAGE}"
       echo "✓ Kernel, DTB, and rootfs loaded"
   }
   
   fel_execute_kernel() {
       echo "Starting kernel execution..."
       sunxi-fel exe 0x40080000
       echo "✓ Kernel execution initiated"
   }
   ```

2. **Hardware Detection Validation**
   ```bash
   #!/bin/bash
   # hardware-detection-test.sh
   
   test_hardware_detection() {
       echo "=== Hardware Detection Test ==="
       
       # eMMC detection
       echo "Testing eMMC detection..."
       if [ -b /dev/mmcblk2 ]; then
           echo "✓ eMMC detected: /dev/mmcblk2"
           fdisk -l /dev/mmcblk2 | head -10
       else
           echo "✗ eMMC not detected"
       fi
       
       # WiFi hardware detection
       echo "Testing WiFi hardware..."
       if lsusb | grep -i aic; then
           echo "✓ AIC8800 WiFi detected"
       else
           echo "✗ WiFi hardware not detected"
       fi
       
       # GPU detection
       echo "Testing GPU detection..."
       if [ -e /dev/dri/card0 ]; then
           echo "✓ Mali GPU detected"
           cat /sys/class/drm/card0/device/vendor 2>/dev/null || echo "GPU vendor info not available"
       else
           echo "✗ GPU not detected"
       fi
       
       # Audio hardware detection
       echo "Testing audio hardware..."
       if [ -e /proc/asound/cards ]; then
           echo "✓ Audio hardware detected:"
           cat /proc/asound/cards
       else
           echo "✗ Audio hardware not detected"
       fi
   }
   ```

### Phase 2: Systematic Component Testing
1. **U-Boot Functionality Testing**
   ```bash
   #!/bin/bash
   # uboot-test-suite.sh
   
   test_uboot_functionality() {
       echo "=== U-Boot Functionality Test ==="
       
       # Serial console commands for U-Boot testing
       cat > uboot_test_commands.txt << 'EOF'
   # U-Boot test command sequence
   echo "=== U-Boot Test Started ==="
   version
   bdinfo
   mmc info
   mmc part
   fatls mmc 2:8
   fdt addr $fdtcontroladdr
   fdt print / | head -20
   env print | grep -E "(boot|fdt|kernel)"
   echo "=== U-Boot Test Complete ==="
   EOF
       
       echo "✓ U-Boot test commands prepared"
       echo "Execute these commands via serial console during U-Boot FEL test"
   }
   ```

2. **Kernel and Driver Testing**
   ```bash
   #!/bin/bash
   # kernel-driver-test.sh
   
   test_kernel_drivers() {
       echo "=== Kernel and Driver Test ==="
       
       # Test MIPS co-processor driver
       echo "Testing MIPS co-processor driver..."
       if lsmod | grep sunxi-mipsloader; then
           echo "✓ MIPS loader driver loaded"
           ls -la /dev/mips* 2>/dev/null || echo "MIPS device nodes not found"
       else
           echo "Loading MIPS driver..."
           modprobe sunxi-mipsloader || echo "✗ Failed to load MIPS driver"
       fi
       
       # Test keystone motor driver
       echo "Testing keystone motor driver..."
       if lsmod | grep hy300-keystone-motor; then
           echo "✓ Keystone motor driver loaded"
           ls -la /sys/class/hy300-keystone/ 2>/dev/null || echo "Keystone sysfs not found"
       else
           echo "Loading keystone driver..."
           modprobe hy300-keystone-motor || echo "✗ Failed to load keystone driver"
       fi
       
       # Test V4L2 HDMI input driver
       echo "Testing V4L2 HDMI input..."
       if ls /dev/video* >/dev/null 2>&1; then
           echo "✓ V4L2 devices detected:"
           ls -la /dev/video*
           v4l2-ctl --list-devices 2>/dev/null || echo "v4l2-ctl not available"
       else
           echo "✗ No V4L2 devices detected"
       fi
   }
   ```

### Phase 3: Service Integration Testing
1. **HY300 Services Testing**
   ```bash
   #!/bin/bash
   # services-integration-test.sh
   
   test_hy300_services() {
       echo "=== HY300 Services Integration Test ==="
       
       # Test service startup
       echo "Testing service startup..."
       systemctl status hy300-system || echo "HY300 system service not running"
       systemctl status hy300-keystone || echo "Keystone service not running"
       systemctl status hy300-wifi || echo "WiFi service not running"
       
       # Test service APIs
       echo "Testing service APIs..."
       curl -s http://localhost:8080/api/status | jq . 2>/dev/null || echo "Service API not responding"
       
       # Test hardware mode activation
       echo "Testing hardware mode activation..."
       curl -s -X POST http://localhost:8080/api/mode/hardware || echo "Hardware mode activation failed"
   }
   ```

2. **Kodi Integration Testing**
   ```bash
   #!/bin/bash
   # kodi-integration-test.sh
   
   test_kodi_integration() {
       echo "=== Kodi Integration Test ==="
       
       # Test Kodi startup
       echo "Testing Kodi startup..."
       systemctl status kodi || echo "Kodi service not running"
       
       # Test Kodi hardware acceleration
       echo "Testing Kodi hardware acceleration..."
       kodi-send --action="Info" 2>/dev/null || echo "Kodi remote control not available"
       
       # Test PVR HDMI input plugin
       echo "Testing PVR HDMI input..."
       ls ~/.kodi/addons/pvr.hdmi-input/ 2>/dev/null && echo "✓ PVR HDMI plugin installed" || echo "✗ PVR HDMI plugin not found"
   }
   ```

### Phase 4: Performance Benchmarking
1. **System Performance Testing**
   ```bash
   #!/bin/bash
   # performance-benchmark.sh
   
   run_performance_benchmarks() {
       echo "=== Performance Benchmark Suite ==="
       
       # CPU performance
       echo "Testing CPU performance..."
       if command -v sysbench >/dev/null; then
           sysbench cpu --threads=4 run | grep "events per second"
       else
           echo "sysbench not available, using alternative CPU test"
           time dd if=/dev/zero of=/dev/null bs=1M count=1000
       fi
       
       # Memory performance
       echo "Testing memory performance..."
       if command -v memtester >/dev/null; then
           memtester 100M 1 | grep -E "(Loop|ok)"
       else
           echo "Testing memory with dd..."
           time dd if=/dev/zero of=/tmp/memtest bs=1M count=500
           rm -f /tmp/memtest
       fi
       
       # Storage performance
       echo "Testing eMMC performance..."
       time dd if=/dev/zero of=/tmp/storagetest bs=1M count=100 conv=fsync
       time dd if=/tmp/storagetest of=/dev/null bs=1M
       rm -f /tmp/storagetest
       
       # GPU performance (if available)
       echo "Testing GPU performance..."
       if command -v glmark2-es2-drm >/dev/null; then
           timeout 60 glmark2-es2-drm --annotate 2>/dev/null | grep "Score:" || echo "GPU benchmark timed out"
       else
           echo "GPU benchmark not available"
       fi
   }
   ```

2. **Network Performance Testing**
   ```bash
   #!/bin/bash
   # network-performance-test.sh
   
   test_network_performance() {
       echo "=== Network Performance Test ==="
       
       # WiFi connection test
       echo "Testing WiFi connectivity..."
       if ping -c 5 8.8.8.8 >/dev/null 2>&1; then
           echo "✓ Internet connectivity confirmed"
           
           # Network throughput test
           echo "Testing network throughput..."
           if command -v iperf3 >/dev/null; then
               iperf3 -c speedtest.net -t 10 2>/dev/null | grep sender || echo "iperf3 test failed"
           else
               echo "Testing with wget..."
               timeout 30 wget -O /dev/null http://speedtest.tele2.net/10MB.zip 2>&1 | grep -E "saved|speed"
           fi
       else
           echo "✗ No internet connectivity"
       fi
   }
   ```

### Phase 5: Recovery and Safety Procedures
1. **Emergency Recovery Procedures**
   ```bash
   #!/bin/bash
   # emergency-recovery.sh
   
   emergency_fel_recovery() {
       echo "=== Emergency FEL Recovery ==="
       
       # Check FEL mode availability
       if lsusb | grep -qi allwinner; then
           echo "✓ Device in FEL mode - recovery possible"
           
           # Basic system recovery
           echo "Performing basic recovery..."
           sunxi-fel version
           sunxi-fel chipid
           
           # Memory test
           echo "Testing DRAM..."
           sunxi-fel readl 0x40000000
           sunxi-fel writel 0x40000000 0x12345678
           if [ "$(sunxi-fel readl 0x40000000)" = "0x12345678" ]; then
               echo "✓ DRAM functional"
           else
               echo "✗ DRAM test failed"
           fi
           
           echo "Device ready for recovery operations"
       else
           echo "✗ Device not in FEL mode"
           echo "Recovery procedures:"
           echo "1. Power off device completely"
           echo "2. Hold FEL button (if available)"
           echo "3. Connect USB cable"
           echo "4. Power on while holding FEL button"
           echo "5. Retry recovery"
       fi
   }
   ```

2. **Factory Reset Procedures**
   ```bash
   #!/bin/bash
   # factory-reset.sh
   
   factory_reset_procedure() {
       echo "=== Factory Reset Procedure ==="
       echo "WARNING: This will restore factory firmware"
       echo "Ensure factory backup images are available"
       
       read -p "Continue with factory reset? (yes/no): " confirm
       if [ "$confirm" = "yes" ]; then
           echo "Restoring factory partitions..."
           # Factory restore commands would go here
           echo "Factory restore procedures documented but not automated for safety"
       else
           echo "Factory reset cancelled"
       fi
   }
   ```

## Testing Documentation and Procedures

### Complete Testing Checklist
```markdown
# HY300 Hardware Testing Checklist

## Pre-Testing Requirements
- [ ] Serial console access configured (115200 baud)
- [ ] USB connection for FEL mode available
- [ ] Factory firmware backup completed
- [ ] Testing environment prepared (Nix devShell)
- [ ] All testing scripts executable and validated

## FEL Mode Testing Phase
- [ ] FEL mode entry successful
- [ ] U-Boot loading via FEL successful  
- [ ] U-Boot functionality validation complete
- [ ] Memory (DRAM) testing passed
- [ ] eMMC detection and access confirmed

## Kernel and Driver Testing Phase
- [ ] Kernel loading and boot successful
- [ ] Device tree loading and parsing successful
- [ ] MIPS co-processor driver loaded and functional
- [ ] Keystone motor driver loaded and functional
- [ ] V4L2 HDMI input driver loaded and functional
- [ ] WiFi driver loaded and hardware detected
- [ ] GPU driver loaded and hardware detected
- [ ] Audio driver loaded and hardware detected

## Service Integration Testing Phase
- [ ] HY300 system service startup successful
- [ ] Keystone service functional in hardware mode
- [ ] WiFi service functional with real hardware
- [ ] Service APIs responding correctly
- [ ] Hardware mode activation successful

## Kodi Integration Testing Phase
- [ ] Kodi startup successful with hardware acceleration
- [ ] Graphics acceleration functional (Panfrost)
- [ ] Audio output functional (SPDIF and analog)
- [ ] PVR HDMI input plugin functional
- [ ] Remote control (IR) functional
- [ ] Video playback with hardware acceleration

## Performance Validation Phase
- [ ] CPU performance meets expectations
- [ ] Memory performance adequate
- [ ] eMMC storage performance acceptable
- [ ] GPU performance sufficient for media center
- [ ] Network throughput adequate for streaming
- [ ] Overall system responsiveness acceptable

## Safety and Recovery Testing Phase
- [ ] FEL mode recovery procedures validated
- [ ] Emergency recovery procedures tested
- [ ] Factory reset procedures documented
- [ ] All recovery mechanisms functional
```

## Risk Management and Safety

### Hardware Safety Protocols
1. **Never Skip FEL Testing**: Always test via FEL mode before permanent installation
2. **Maintain Recovery Access**: Ensure FEL mode access always available
3. **Complete Backups**: Factory firmware and partition backups maintained
4. **Incremental Testing**: Test one component at a time with validation
5. **Documentation**: Complete documentation of all procedures and results

### Testing Risk Assessment
- **Hardware Damage**: FEL mode eliminates permanent hardware damage risk
- **Data Loss**: Factory backups prevent data loss scenarios
- **Testing Failures**: Comprehensive recovery procedures handle all failure modes
- **Performance Issues**: Benchmarking identifies performance bottlenecks early

## Success Metrics
- **Complete Testing Coverage**: All hardware components tested successfully
- **Performance Validation**: All performance benchmarks meet requirements
- **Recovery Validation**: All recovery procedures tested and functional
- **Documentation Quality**: Complete testing documentation for future reference
- **Production Readiness**: System ready for production deployment

## Documentation Updates Required

### Testing Documentation
- Create comprehensive hardware testing guide
- Document all testing procedures and scripts
- Create troubleshooting guide for common issues
- Document performance benchmarking results

### Safety Documentation
- Update hardware safety protocols with testing procedures
- Document emergency recovery procedures
- Create production deployment safety checklist

## Next Steps After Completion
- **Production Deployment**: Deploy tested system to production hardware
- **Performance Monitoring**: Implement continuous performance monitoring
- **User Documentation**: Create user guides and operation manuals
- **Maintenance Procedures**: Establish ongoing maintenance and update procedures

This task establishes comprehensive, safe hardware testing procedures that validate the complete HY300 Linux system before production deployment, ensuring reliability and maintainability.