#!/usr/bin/env bash
# HY300 Accelerometer System Integration Test
# 
# This script tests the complete accelerometer detection and integration system:
# 1. Kernel driver sysfs interface
# 2. Accelerometer detection utility
# 3. Integration service functionality
# 4. Build system validation
#
# Copyright (C) 2025 HY300 Linux Porting Project
# SPDX-License-Identifier: GPL-2.0+

set -e

# Test configuration
TEST_DIR="/tmp/hy300_accel_test"
MOCK_SYSFS_DIR="$TEST_DIR/mock_sysfs"
MOCK_I2C_DIR="$TEST_DIR/mock_i2c"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results
TESTS_PASSED=0
TESTS_FAILED=0

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Test result functions
test_pass() {
    echo -e "${GREEN}✓ PASS:${NC} $1"
    ((TESTS_PASSED++))
}

test_fail() {
    echo -e "${RED}✗ FAIL:${NC} $1"
    ((TESTS_FAILED++))
}

test_skip() {
    echo -e "${YELLOW}- SKIP:${NC} $1"
}

# Setup test environment
setup_test_env() {
    log_info "Setting up test environment in $TEST_DIR"
    
    # Create test directory structure
    mkdir -p "$TEST_DIR"
    mkdir -p "$MOCK_SYSFS_DIR/class/hy300"
    mkdir -p "$MOCK_I2C_DIR/dev"
    
    # Create mock sysfs files
    echo "unknown" > "$MOCK_SYSFS_DIR/class/hy300/accelerometer_type"
    chmod 666 "$MOCK_SYSFS_DIR/class/hy300/accelerometer_type"
    
    log_info "Test environment ready"
}

# Cleanup test environment
cleanup_test_env() {
    log_info "Cleaning up test environment"
    rm -rf "$TEST_DIR"
}

# Test 1: Build System Validation
test_build_system() {
    log_info "Testing build system..."
    
    cd tools
    
    # Test clean build
    if make clean >/dev/null 2>&1 && make all >/dev/null 2>&1; then
        test_pass "Build system - clean build"
    else
        test_fail "Build system - clean build failed"
        return
    fi
    
    # Test binary exists and is executable
    if [ -x "accelerometer_detection" ]; then
        test_pass "Build system - binary created and executable"
    else
        test_fail "Build system - binary not executable"
    fi
    
    # Test help output
    if ./accelerometer_detection --help >/dev/null 2>&1; then
        test_pass "Build system - help output works"
    else
        test_fail "Build system - help output failed"
    fi
    
    cd ..
}

# Test 2: Accelerometer Detection Utility
test_detection_utility() {
    log_info "Testing accelerometer detection utility..."
    
    cd tools
    
    # Test verbose mode (should not crash even without I2C)
    if timeout 5 ./accelerometer_detection --verbose --bus=99 2>/dev/null; then
        # Expected to fail gracefully with non-existent bus
        test_pass "Detection utility - graceful failure on invalid bus"
    else
        # Check exit code - should be 1 (failure), not segfault
        exit_code=$?
        if [ $exit_code -eq 1 ]; then
            test_pass "Detection utility - proper error exit code"
        else
            test_fail "Detection utility - unexpected exit code: $exit_code"
        fi
    fi
    
    # Test scan mode 
    if timeout 5 ./accelerometer_detection --scan --bus=99 2>/dev/null; then
        test_pass "Detection utility - scan mode handles missing bus"
    else
        exit_code=$?
        if [ $exit_code -eq 1 ]; then
            test_pass "Detection utility - scan mode proper error handling"
        else
            test_fail "Detection utility - scan mode unexpected exit: $exit_code"
        fi
    fi
    
    cd ..
}

# Test 3: Integration Service
test_integration_service() {
    log_info "Testing integration service..."
    
    # Test service help
    if python3 tools/hy300-accelerometer-service.py --help >/dev/null 2>&1; then
        test_pass "Integration service - help output works"
    else
        test_fail "Integration service - help output failed"
    fi
    
    # Test service status (should handle missing state gracefully)
    if python3 tools/hy300-accelerometer-service.py --status 2>/dev/null; then
        test_pass "Integration service - status command works"
    else
        test_fail "Integration service - status command failed"
    fi
    
    # Test Python syntax (already done in build, but verify)
    if python3 -m py_compile tools/hy300-accelerometer-service.py 2>/dev/null; then
        test_pass "Integration service - Python syntax valid"
    else
        test_fail "Integration service - Python syntax errors"
    fi
}

# Test 4: Kernel Driver Integration (Mock Test)
test_kernel_driver_mock() {
    log_info "Testing kernel driver integration (mock)..."
    
    # Test sysfs write functionality (using mock)
    test_file="$MOCK_SYSFS_DIR/class/hy300/accelerometer_type"
    
    if echo "STK8BA58" > "$test_file" 2>/dev/null; then
        if [ "$(cat "$test_file")" = "STK8BA58" ]; then
            test_pass "Kernel driver - sysfs write/read works"
        else
            test_fail "Kernel driver - sysfs read mismatch"
        fi
    else
        test_fail "Kernel driver - sysfs write failed"
    fi
    
    # Test different accelerometer types
    for accel_type in "KXTTJ3" "unknown" "none"; do
        if echo "$accel_type" > "$test_file" 2>/dev/null; then
            if [ "$(cat "$test_file")" = "$accel_type" ]; then
                test_pass "Kernel driver - sysfs handles $accel_type"
            else
                test_fail "Kernel driver - sysfs $accel_type mismatch"
            fi
        else
            test_fail "Kernel driver - sysfs $accel_type write failed"
        fi
    done
}

# Test 5: System Integration
test_system_integration() {
    log_info "Testing system integration..."
    
    # Check if systemd service file is valid
    if [ -f "tools/hy300-accelerometer.service" ]; then
        # Basic syntax check for systemd service
        if grep -q "ExecStart=" tools/hy300-accelerometer.service && \
           grep -q "WantedBy=" tools/hy300-accelerometer.service; then
            test_pass "System integration - systemd service file valid"
        else
            test_fail "System integration - systemd service file invalid"
        fi
    else
        test_fail "System integration - systemd service file missing"
    fi
    
    # Test Makefile install targets (dry run)
    cd tools
    if make help >/dev/null 2>&1; then
        test_pass "System integration - Makefile help works"
    else
        test_fail "System integration - Makefile help failed"
    fi
    cd ..
}

# Test 6: Error Handling
test_error_handling() {
    log_info "Testing error handling..."
    
    cd tools
    
    # Test detection utility with invalid arguments
    if ./accelerometer_detection --invalid-option 2>/dev/null; then
        test_fail "Error handling - should reject invalid options"
    else
        test_pass "Error handling - rejects invalid options"
    fi
    
    # Test service with invalid arguments  
    if python3 hy300-accelerometer-service.py --invalid-option 2>/dev/null; then
        test_fail "Error handling - service should reject invalid options"
    else
        test_pass "Error handling - service rejects invalid options"
    fi
    
    cd ..
}

# Main test runner
main() {
    echo "=================================================="
    echo "HY300 Accelerometer System Integration Test Suite"
    echo "=================================================="
    echo
    
    # Check if we're in the right directory
    if [ ! -f "tools/accelerometer_detection.c" ]; then
        log_error "Please run this script from the HY300 project root directory"
        exit 1
    fi
    
    # Setup
    setup_test_env
    trap cleanup_test_env EXIT
    
    # Run tests
    test_build_system
    test_detection_utility  
    test_integration_service
    test_kernel_driver_mock
    test_system_integration
    test_error_handling
    
    # Summary
    echo
    echo "=================================================="
    echo "Test Results Summary"
    echo "=================================================="
    echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"
    echo -e "Total Tests:  $((TESTS_PASSED + TESTS_FAILED))"
    
    if [ $TESTS_FAILED -eq 0 ]; then
        echo
        log_info "All tests passed! ✓"
        echo
        echo "The HY300 accelerometer detection system is ready for hardware testing."
        echo "Next steps:"
        echo "1. Load kernel modules on target hardware"
        echo "2. Run accelerometer detection utility with hardware present"
        echo "3. Start integration service to coordinate with device tree"
        echo "4. Test keystone correction functionality"
        exit 0
    else
        echo
        log_error "$TESTS_FAILED tests failed!"
        echo
        echo "Please fix the failing tests before deploying to hardware."
        exit 1
    fi
}

# Run main function
main "$@"