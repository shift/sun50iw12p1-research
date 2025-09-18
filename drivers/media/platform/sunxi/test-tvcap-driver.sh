#!/usr/bin/env bash
# TV Capture Driver Testing and Validation Framework
# Task 019: HDMI Input Driver Implementation - Atomic Task 6.1
#
# This script provides comprehensive testing for the sunxi-tvcap driver
# including initialization tests, V4L2 compliance testing, and hardware validation.
#
# Usage: ./test-tvcap-driver.sh [--load-only|--unload-only|--quick|--full]

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test configuration
DRIVER_NAME="sunxi-tvcap"
MODULE_FILE="sunxi-tvcap.ko"
DEVICE_NODE="/dev/video0"
TEST_RESULTS_DIR="/tmp/tvcap-test-results"

# Create test results directory
mkdir -p "$TEST_RESULTS_DIR"
LOG_FILE="$TEST_RESULTS_DIR/test-$(date +%Y%m%d-%H%M%S).log"

# Logging functions
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $*" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[PASS]${NC} $*" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[WARN]${NC} $*" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[FAIL]${NC} $*" | tee -a "$LOG_FILE"
}

log_test() {
    echo -e "${CYAN}[TEST]${NC} $*" | tee -a "$LOG_FILE"
}

# Global test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
WARNINGS=0

# Test result tracking
pass_test() {
    ((PASSED_TESTS++))
    ((TOTAL_TESTS++))
    log_success "$1"
}

fail_test() {
    ((FAILED_TESTS++))
    ((TOTAL_TESTS++))
    log_error "$1"
}

warn_test() {
    ((WARNINGS++))
    log_warning "$1"
}

# Check if running in Nix devShell
check_environment() {
    log_info "Checking development environment..."
    
    if [[ -z "${IN_NIX_SHELL:-}" ]]; then
        log_warning "Not running in Nix devShell - tools may not be available"
        log_info "Consider running: nix develop -c -- $0"
    else
        log_success "Running in Nix devShell environment"
    fi
    
    # Check for required tools
    local required_tools=("lsmod" "insmod" "rmmod" "dmesg" "v4l2-ctl")
    local missing_tools=0
    
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "Tool available: $tool"
        else
            log_warning "Tool not available: $tool"
            ((missing_tools++))
        fi
    done
    
    if [[ $missing_tools -gt 0 ]]; then
        warn_test "Some testing tools are not available - test coverage will be limited"
    fi
}

# Pre-test safety checks
safety_checks() {
    log_info "Performing safety checks..."
    
    # Check if running as root (required for module operations)
    if [[ $EUID -ne 0 ]]; then
        log_error "This script must be run as root for module loading/unloading"
        log_info "Consider running: sudo $0"
        exit 1
    fi
    
    # Check if module file exists
    if [[ ! -f "$MODULE_FILE" ]]; then
        log_error "Module file not found: $MODULE_FILE"
        log_info "Build the module first with appropriate kernel build system"
        exit 1
    fi
    
    # Check module file integrity
    if file "$MODULE_FILE" | grep -q "ELF.*ARM aarch64"; then
        pass_test "Module file is valid ARM64 ELF object"
    else
        fail_test "Module file is not a valid ARM64 kernel module"
        exit 1
    fi
    
    log_success "Safety checks completed"
}

# Test 1: Module Compilation and Loading
test_module_loading() {
    log_test "Testing module loading and initialization..."
    
    # Check if module is already loaded
    if lsmod | grep -q "^$DRIVER_NAME"; then
        log_info "Module already loaded - unloading first"
        if ! rmmod "$DRIVER_NAME" 2>/dev/null; then
            warn_test "Failed to unload existing module"
        fi
    fi
    
    # Capture kernel messages before loading
    local pre_dmesg_lines
    pre_dmesg_lines=$(dmesg | wc -l)
    
    # Load the module
    if insmod "$MODULE_FILE" 2>"$TEST_RESULTS_DIR/insmod_error.log"; then
        pass_test "Module loaded successfully"
        
        # Check if module appears in lsmod
        if lsmod | grep -q "^$DRIVER_NAME"; then
            pass_test "Module appears in lsmod output"
        else
            fail_test "Module not found in lsmod after loading"
        fi
        
        # Capture kernel messages after loading
        local post_dmesg_lines
        post_dmesg_lines=$(dmesg | wc -l)
        
        if [[ $post_dmesg_lines -gt $pre_dmesg_lines ]]; then
            log_info "Kernel messages generated during module loading:"
            dmesg | tail -n $((post_dmesg_lines - pre_dmesg_lines)) | while read -r line; do
                log_info "  $line"
            done
            
            # Check for initialization success messages
            if dmesg | tail -n $((post_dmesg_lines - pre_dmesg_lines)) | grep -q "initialized successfully"; then
                pass_test "Driver reported successful initialization"
            else
                warn_test "No explicit success message found in kernel log"
            fi
            
            # Check for error messages
            if dmesg | tail -n $((post_dmesg_lines - pre_dmesg_lines)) | grep -qE "(error|Error|ERROR|failed|Failed|FAILED)"; then
                warn_test "Error messages detected in kernel log during loading"
            fi
        else
            warn_test "No kernel messages generated during module loading"
        fi
        
    else
        fail_test "Module loading failed"
        if [[ -f "$TEST_RESULTS_DIR/insmod_error.log" ]]; then
            log_error "insmod error output:"
            cat "$TEST_RESULTS_DIR/insmod_error.log" | while read -r line; do
                log_error "  $line"
            done
        fi
        return 1
    fi
}

# Test 2: Device Node Creation
test_device_registration() {
    log_test "Testing V4L2 device registration..."
    
    # Wait a moment for device creation
    sleep 2
    
    # Check if device node exists
    if [[ -c "$DEVICE_NODE" ]]; then
        pass_test "V4L2 device node created: $DEVICE_NODE"
        
        # Check device permissions
        local perms
        perms=$(stat -c "%a" "$DEVICE_NODE")
        log_info "Device permissions: $perms"
        
        # Check device ownership
        local owner
        owner=$(stat -c "%U:%G" "$DEVICE_NODE")
        log_info "Device ownership: $owner"
        
    else
        fail_test "V4L2 device node not created: $DEVICE_NODE"
        
        # Check for alternative video devices
        local video_devices
        video_devices=$(find /dev -name "video*" 2>/dev/null || true)
        if [[ -n "$video_devices" ]]; then
            log_info "Available video devices:"
            echo "$video_devices" | while read -r device; do
                log_info "  $device"
            done
        else
            log_info "No video devices found in /dev"
        fi
        return 1
    fi
}

# Test 3: Basic V4L2 IOCTL Interface
test_v4l2_interface() {
    log_test "Testing basic V4L2 IOCTL interface..."
    
    if [[ ! -c "$DEVICE_NODE" ]]; then
        fail_test "Device node not available for V4L2 testing"
        return 1
    fi
    
    # Test device capabilities query
    if command -v v4l2-ctl >/dev/null 2>&1; then
        log_info "Testing device capabilities with v4l2-ctl..."
        
        if v4l2-ctl -d "$DEVICE_NODE" --info >"$TEST_RESULTS_DIR/v4l2_info.log" 2>&1; then
            pass_test "V4L2 device info query successful"
            log_info "Device capabilities:"
            cat "$TEST_RESULTS_DIR/v4l2_info.log" | while read -r line; do
                log_info "  $line"
            done
        else
            fail_test "V4L2 device info query failed"
            if [[ -f "$TEST_RESULTS_DIR/v4l2_info.log" ]]; then
                cat "$TEST_RESULTS_DIR/v4l2_info.log" | while read -r line; do
                    log_error "  $line"
                done
            fi
        fi
        
        # Test format enumeration
        if v4l2-ctl -d "$DEVICE_NODE" --list-formats >"$TEST_RESULTS_DIR/v4l2_formats.log" 2>&1; then
            pass_test "V4L2 format enumeration successful"
            log_info "Supported formats:"
            cat "$TEST_RESULTS_DIR/v4l2_formats.log" | while read -r line; do
                log_info "  $line"
            done
        else
            warn_test "V4L2 format enumeration failed"
        fi
        
        # Test input enumeration
        if v4l2-ctl -d "$DEVICE_NODE" --list-inputs >"$TEST_RESULTS_DIR/v4l2_inputs.log" 2>&1; then
            pass_test "V4L2 input enumeration successful"
            log_info "Available inputs:"
            cat "$TEST_RESULTS_DIR/v4l2_inputs.log" | while read -r line; do
                log_info "  $line"
            done
        else
            warn_test "V4L2 input enumeration failed"
        fi
        
    else
        warn_test "v4l2-ctl not available - skipping V4L2 interface tests"
    fi
}

# Test 4: Hardware Register Access
test_hardware_initialization() {
    log_test "Testing hardware initialization verification..."
    
    # Check for hardware-related kernel messages
    local hw_messages
    hw_messages=$(dmesg | grep -i "tvcap\|tvtop\|hdmi" | tail -20 || true)
    
    if [[ -n "$hw_messages" ]]; then
        log_info "Hardware-related kernel messages:"
        echo "$hw_messages" | while read -r line; do
            log_info "  $line"
        done
        
        # Look for initialization success indicators
        if echo "$hw_messages" | grep -q "initialized successfully\|hardware ready\|initialization complete"; then
            pass_test "Hardware initialization appears successful"
        elif echo "$hw_messages" | grep -qE "error|failed|timeout"; then
            warn_test "Hardware initialization may have encountered issues"
        else
            warn_test "Hardware initialization status unclear from kernel messages"
        fi
    else
        warn_test "No hardware-related kernel messages found"
    fi
    
    # Check /proc/devices for our driver
    if grep -q "$DRIVER_NAME" /proc/devices 2>/dev/null; then
        pass_test "Driver registered in /proc/devices"
    else
        # This is not necessarily an error for V4L2 drivers
        log_info "Driver not found in /proc/devices (normal for V4L2 drivers)"
    fi
}

# Test 5: Module Resource Cleanup
test_module_unloading() {
    log_test "Testing module unloading and cleanup..."
    
    # Capture kernel messages before unloading
    local pre_dmesg_lines
    pre_dmesg_lines=$(dmesg | wc -l)
    
    # Unload the module
    if rmmod "$DRIVER_NAME" 2>"$TEST_RESULTS_DIR/rmmod_error.log"; then
        pass_test "Module unloaded successfully"
        
        # Check if module is removed from lsmod
        if ! lsmod | grep -q "^$DRIVER_NAME"; then
            pass_test "Module removed from lsmod"
        else
            fail_test "Module still appears in lsmod after unloading"
        fi
        
        # Check if device node is cleaned up
        if [[ ! -e "$DEVICE_NODE" ]]; then
            pass_test "Device node cleaned up"
        else
            warn_test "Device node still exists after module unload"
        fi
        
        # Capture cleanup messages
        local post_dmesg_lines
        post_dmesg_lines=$(dmesg | wc -l)
        
        if [[ $post_dmesg_lines -gt $pre_dmesg_lines ]]; then
            log_info "Kernel messages during module unloading:"
            dmesg | tail -n $((post_dmesg_lines - pre_dmesg_lines)) | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Module unloading failed"
        if [[ -f "$TEST_RESULTS_DIR/rmmod_error.log" ]]; then
            log_error "rmmod error output:"
            cat "$TEST_RESULTS_DIR/rmmod_error.log" | while read -r line; do
                log_error "  $line"
            done
        fi
        return 1
    fi
}

# Test 6: Memory Leak Detection
test_memory_leaks() {
    log_test "Testing for memory leaks and resource cleanup..."
    
    local initial_memory
    initial_memory=$(grep MemAvailable /proc/meminfo | awk '{print $2}' || echo "0")
    
    # Load and unload module multiple times
    for i in {1..3}; do
        log_info "Load/unload cycle $i..."
        
        if insmod "$MODULE_FILE" 2>/dev/null; then
            sleep 1
            if rmmod "$DRIVER_NAME" 2>/dev/null; then
                log_info "Cycle $i completed successfully"
            else
                warn_test "Failed to unload module in cycle $i"
                break
            fi
        else
            warn_test "Failed to load module in cycle $i"
            break
        fi
    done
    
    local final_memory
    final_memory=$(grep MemAvailable /proc/meminfo | awk '{print $2}' || echo "0")
    
    local memory_diff=$((initial_memory - final_memory))
    log_info "Memory difference: ${memory_diff}KB"
    
    # Allow for some variation (less than 1MB indicates no major leaks)
    if [[ $memory_diff -lt 1024 ]]; then
        pass_test "No significant memory leaks detected"
    else
        warn_test "Potential memory leak detected: ${memory_diff}KB"
    fi
}

# Summary and cleanup
print_summary() {
    echo
    log_info "======================================"
    log_info "TV Capture Driver Test Summary"
    log_info "======================================"
    log_info "Total tests: $TOTAL_TESTS"
    log_info "Passed: $PASSED_TESTS"
    log_info "Failed: $FAILED_TESTS"
    log_info "Warnings: $WARNINGS"
    log_info "Test log: $LOG_FILE"
    log_info "Results directory: $TEST_RESULTS_DIR"
    echo
    
    if [[ $FAILED_TESTS -eq 0 ]]; then
        log_success "All tests passed! ✅"
        if [[ $WARNINGS -gt 0 ]]; then
            log_warning "Note: $WARNINGS warnings were reported"
        fi
        return 0
    else
        log_error "$FAILED_TESTS tests failed ❌"
        return 1
    fi
}

# Main test execution
main() {
    echo -e "${BLUE}TV Capture Driver Testing Framework${NC}"
    echo "==================================="
    echo
    
    local test_mode="full"
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            --load-only)
                test_mode="load-only"
                shift
                ;;
            --unload-only)
                test_mode="unload-only"
                shift
                ;;
            --quick)
                test_mode="quick"
                shift
                ;;
            --full)
                test_mode="full"
                shift
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Usage: $0 [--load-only|--unload-only|--quick|--full]"
                exit 1
                ;;
        esac
    done
    
    log_info "Starting TV capture driver tests (mode: $test_mode)..."
    log_info "Test results will be saved to: $TEST_RESULTS_DIR"
    
    check_environment
    
    case $test_mode in
        "load-only")
            safety_checks
            test_module_loading
            test_device_registration
            ;;
        "unload-only")
            test_module_unloading
            ;;
        "quick")
            safety_checks
            test_module_loading
            test_device_registration
            test_v4l2_interface
            test_module_unloading
            ;;
        "full")
            safety_checks
            test_module_loading
            test_device_registration
            test_v4l2_interface
            test_hardware_initialization
            test_module_unloading
            test_memory_leaks
            ;;
    esac
    
    print_summary
}

# Cleanup on exit
cleanup() {
    # Try to unload module if it's still loaded
    if lsmod | grep -q "^$DRIVER_NAME" 2>/dev/null; then
        log_info "Cleaning up: unloading module..."
        rmmod "$DRIVER_NAME" 2>/dev/null || true
    fi
}

trap cleanup EXIT

# Execute main function
main "$@"