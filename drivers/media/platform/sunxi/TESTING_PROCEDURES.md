# TV Capture Driver Testing Procedures and Expected Results
# Task 019: HDMI Input Driver Implementation - Atomic Task 6.5

## Overview

This document provides comprehensive testing procedures for the sunxi-tvcap TV capture driver for the HY300 projector. The testing framework validates driver functionality from basic initialization through advanced capture operations.

## Testing Environment Requirements

### Hardware Requirements
- **Target Device**: HY300 Android Projector with H713 SoC
- **Host System**: Development machine with cross-compilation environment
- **HDMI Source**: External HDMI device for input testing (optional but recommended)
- **Serial Console**: For kernel message monitoring (recommended)
- **Recovery Access**: FEL mode capability for safe testing

### Software Requirements
- **Nix Development Environment**: All tools available via `nix develop`
- **Cross-Compilation Toolchain**: aarch64-unknown-linux-gnu-*
- **V4L2 Utilities**: v4l2-ctl, v4l2-compliance (if available)
- **Root Access**: Required for module loading/unloading operations
- **Sufficient Storage**: ~100MB for test results and logs

### Network Requirements
- **File Transfer**: Method to transfer test scripts to target device
- **Results Retrieval**: Method to retrieve test logs from target device

## Test Suite Components

### 1. Driver Loading and Initialization Test
**Script**: `test-tvcap-driver.sh`
**Purpose**: Validates basic driver loading, initialization, and cleanup

#### Usage
```bash
# Basic test
sudo ./test-tvcap-driver.sh

# Quick test (load/unload only)
sudo ./test-tvcap-driver.sh --quick

# Load only (for subsequent tests)
sudo ./test-tvcap-driver.sh --load-only

# Unload only (cleanup)
sudo ./test-tvcap-driver.sh --unload-only

# Full test with memory leak detection
sudo ./test-tvcap-driver.sh --full
```

#### Expected Results
- **Module Loading**: Driver loads without errors
- **Device Creation**: `/dev/video0` appears with correct permissions
- **Hardware Initialization**: TVTOP subsystem initializes successfully
- **Kernel Messages**: Positive initialization messages in dmesg
- **Module Unloading**: Clean unload with proper resource cleanup

#### Success Criteria
- ✅ All tests pass (0 failures)
- ⚠️ Warnings acceptable if < 3 and related to missing optional features
- ❌ Any failures in core loading/unloading indicate driver issues

### 2. V4L2 Compliance Testing
**Script**: `test-v4l2-compliance.sh`
**Purpose**: Validates Video4Linux2 API compliance and interface correctness

#### Usage
```bash
# Basic compliance test
./test-v4l2-compliance.sh

# Quick compliance test
./test-v4l2-compliance.sh --quick

# Verbose output
./test-v4l2-compliance.sh --verbose

# Custom device
./test-v4l2-compliance.sh --device /dev/video1
```

#### Expected Results
- **Device Capabilities**: Correct driver name (sunxi-tvcap) and capabilities
- **Format Support**: At least YUYV format available
- **Input Enumeration**: HDMI input detected and enumerable
- **IOCTL Interface**: Basic V4L2 IOCTLs respond correctly
- **Buffer Management**: Memory mapping buffers work correctly

#### Success Criteria
- ✅ Core V4L2 functionality working (capability query, format enum, buffer ops)
- ⚠️ Minor compliance issues acceptable for development driver
- ❌ Major IOCTL failures indicate API implementation problems

### 3. HDMI Input Detection Validation
**Script**: `test-hdmi-input-detection.sh`
**Purpose**: Tests HDMI input detection, hot-plug events, and signal detection

#### Usage
```bash
# Basic input detection test
./test-hdmi-input-detection.sh

# Continuous monitoring mode
./test-hdmi-input-detection.sh --monitor --duration 60

# Verbose output
./test-hdmi-input-detection.sh --verbose
```

#### Expected Results
- **Input Status**: HDMI input properly enumerated
- **Signal Detection**: Current input status available
- **Format Detection**: Auto-detection of connected source format
- **Hot-plug Events**: Kernel messages when cables connected/disconnected
- **Hardware Status**: Positive hardware initialization indicators

#### Success Criteria
- ✅ Input enumeration works, hardware reports ready status
- ⚠️ Hot-plug detection may require physical cable manipulation
- ❌ No input detection or hardware errors indicate hardware integration issues

### 4. Basic Capture Functionality Test
**Script**: `test-capture-functionality.sh`
**Purpose**: Validates complete capture pipeline from buffer allocation to frame capture

#### Usage
```bash
# Basic capture test
./test-capture-functionality.sh

# Extended capture test
./test-capture-functionality.sh --frames 20 --buffers 8

# Performance testing
./test-capture-functionality.sh --frames 50 --timeout 60

# Verbose output
./test-capture-functionality.sh --verbose
```

#### Expected Results
- **Buffer Allocation**: Memory-mapped buffers allocate successfully
- **Format Configuration**: Video format setting and querying works
- **Streaming Operations**: Stream start/stop operations function
- **Frame Capture**: Actual frame data captured to files
- **Performance**: Reasonable capture rates achieved
- **Resource Cleanup**: Proper cleanup after operations

#### Success Criteria
- ✅ Complete capture pipeline functional (all tests pass)
- ⚠️ Performance issues acceptable if core functionality works
- ❌ Buffer allocation or streaming failures indicate serious driver issues

## Testing Workflow

### Phase 1: Basic Validation
1. **Environment Setup**
   ```bash
   # Ensure Nix environment
   nix develop
   
   # Transfer test scripts to target device
   scp test-*.sh root@target-device:/tmp/
   ```

2. **Initial Driver Test**
   ```bash
   # Run basic driver test first
   sudo ./test-tvcap-driver.sh --quick
   ```

3. **V4L2 Interface Test**
   ```bash
   # Test V4L2 API compliance
   ./test-v4l2-compliance.sh --quick
   ```

### Phase 2: Functional Validation
1. **HDMI Input Testing**
   ```bash
   # Test input detection
   ./test-hdmi-input-detection.sh
   
   # Optional: Monitor hot-plug events
   ./test-hdmi-input-detection.sh --monitor --duration 30
   ```

2. **Capture Pipeline Testing**
   ```bash
   # Test basic capture functionality
   ./test-capture-functionality.sh --frames 10
   ```

### Phase 3: Comprehensive Testing
1. **Full Test Suite**
   ```bash
   # Run complete driver test with memory leak detection
   sudo ./test-tvcap-driver.sh --full
   
   # Run detailed V4L2 compliance
   ./test-v4l2-compliance.sh --verbose
   
   # Run extended capture test
   ./test-capture-functionality.sh --frames 30 --verbose
   ```

2. **Performance and Stress Testing**
   ```bash
   # Extended capture performance test
   ./test-capture-functionality.sh --frames 100 --timeout 120
   
   # Multiple load/unload cycles
   for i in {1..5}; do
       echo "Cycle $i"
       sudo ./test-tvcap-driver.sh --quick
   done
   ```

## Expected Test Results Analysis

### Normal Operation Indicators
- **Kernel Messages**: Look for "initialized successfully" messages
- **Device Creation**: `/dev/video0` appears with 660 permissions
- **V4L2 Capabilities**: Driver reports Video Capture + Streaming capabilities
- **Buffer Operations**: Memory-mapped buffers allocate and function correctly
- **Format Support**: At least YUYV (and ideally YU12, RGB formats) available

### Warning Conditions (Acceptable)
- **Missing Optional Features**: Some V4L2 controls not implemented
- **Performance Limitations**: Slower than optimal capture rates
- **Limited Format Support**: Fewer formats than full implementation
- **Event System**: V4L2 events not implemented (normal for basic driver)

### Error Conditions (Require Investigation)
- **Module Load Failures**: Driver fails to load or initialize
- **Hardware Errors**: TVTOP hardware not responding
- **IOCTL Failures**: Basic V4L2 operations fail
- **Memory Issues**: Buffer allocation fails or leaks memory
- **System Instability**: Kernel panics or system freezes

## Troubleshooting Guide

### Driver Load Issues
```bash
# Check kernel log for specific errors
dmesg | tail -20

# Verify module dependencies
lsmod | grep sunxi

# Check hardware availability
cat /proc/iomem | grep 57000000
```

### V4L2 Interface Issues
```bash
# Check device node permissions
ls -la /dev/video*

# Verify device capabilities
v4l2-ctl -d /dev/video0 --info

# Test basic IOCTL
v4l2-ctl -d /dev/video0 --list-formats
```

### Hardware Integration Issues
```bash
# Check TVTOP register access
# (This requires specialized tools or custom test code)

# Monitor interrupt activity
cat /proc/interrupts | grep 110

# Check clock and reset status
# (Driver should report this in initialization)
```

## Test Result Interpretation

### Success Metrics
- **Basic Functionality**: Driver loads, creates device, basic IOCTLs work
- **Capture Pipeline**: Buffers allocate, streaming starts, frames captured
- **API Compliance**: V4L2 interface responds correctly to standard operations
- **Stability**: Multiple load/unload cycles work without issues

### Performance Metrics
- **Load Time**: Driver initialization < 5 seconds
- **Capture Rate**: At least 1 fps for basic validation
- **Memory Usage**: No significant memory leaks over multiple operations
- **CPU Usage**: Reasonable overhead for capture operations

### Quality Metrics
- **Code Standards**: Driver follows Linux kernel coding conventions
- **Error Handling**: Graceful handling of error conditions
- **Resource Management**: Proper cleanup of allocated resources
- **Documentation**: Clear error messages and status reporting

## Test Data and Logs

### Log File Locations
- **Driver Tests**: `/tmp/tvcap-test-results/test-YYYYMMDD-HHMMSS.log`
- **V4L2 Compliance**: `/tmp/v4l2-compliance-results/v4l2-compliance-YYYYMMDD-HHMMSS.log`
- **HDMI Detection**: `/tmp/hdmi-input-test-results/hdmi-input-test-YYYYMMDD-HHMMSS.log`
- **Capture Tests**: `/tmp/capture-test-results/capture-test-YYYYMMDD-HHMMSS.log`

### Log Analysis
```bash
# Check for error patterns
grep -i error /tmp/*/test-*.log

# Look for performance data
grep -i "fps\|rate\|performance" /tmp/*/test-*.log

# Find hardware status messages
grep -i "hardware\|tvtop\|initialized" /tmp/*/test-*.log
```

### Result Archival
```bash
# Create test archive
tar -czf tvcap-test-results-$(date +%Y%m%d).tar.gz /tmp/*-test-results/

# Transfer results to development machine
scp tvcap-test-results-*.tar.gz user@dev-machine:/path/to/results/
```

## Continuous Integration Integration

### Automated Testing Script
```bash
#!/bin/bash
# automated-test-runner.sh

set -e

echo "Starting automated TV capture driver test suite..."

# Phase 1: Basic validation
sudo ./test-tvcap-driver.sh --quick
./test-v4l2-compliance.sh --quick

# Phase 2: Functional validation  
./test-hdmi-input-detection.sh
./test-capture-functionality.sh --frames 5

# Phase 3: Extended validation (if basic tests pass)
sudo ./test-tvcap-driver.sh --full
./test-v4l2-compliance.sh
./test-capture-functionality.sh --frames 20

echo "Automated test suite completed successfully"
```

### Exit Codes
- **0**: All tests passed
- **1**: Minor issues (warnings present)
- **2**: Major functionality failures
- **3**: Critical driver issues (load/unload failures)

## Hardware Testing Considerations

### Safe Testing Practices
- **FEL Recovery**: Ensure FEL mode access available for recovery
- **Incremental Testing**: Test basic functionality before complex operations
- **Monitoring**: Watch kernel messages and system stability during tests
- **Backup**: Maintain known-good kernel/driver versions

### Production Testing
- **Real HDMI Sources**: Test with actual HDMI devices (cameras, computers, etc.)
- **Multiple Formats**: Test various resolutions and formats
- **Extended Operation**: Long-running capture tests
- **Thermal Testing**: Monitor temperature during sustained operation

## Future Testing Enhancements

### Additional Test Cases
- **Multiple Format Testing**: Test all supported pixel formats and resolutions
- **Stress Testing**: Extended capture sessions, memory pressure scenarios
- **Error Injection**: Test error handling and recovery mechanisms
- **Performance Profiling**: Detailed CPU and memory usage analysis

### Integration Testing
- **MIPS Communication**: Test ARM-MIPS coordination (when MIPS drivers available)
- **Display Pipeline**: Test capture → display path (when display drivers available)
- **Application Integration**: Test with real applications (GStreamer, FFmpeg, etc.)

This testing framework provides comprehensive validation of the TV capture driver functionality and serves as the foundation for ongoing development and quality assurance.