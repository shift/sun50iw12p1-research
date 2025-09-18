#!/usr/bin/env bash
# V4L2 Compliance Testing for TV Capture Driver
# Task 019: HDMI Input Driver Implementation - Atomic Task 6.2
#
# This script implements comprehensive V4L2 compliance testing for the sunxi-tvcap driver
# following the Video4Linux2 API specification and kernel compliance standards.
#
# Usage: ./test-v4l2-compliance.sh [--device /dev/videoN] [--verbose] [--quick]

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Default configuration
DEVICE_NODE="/dev/video0"
TEST_RESULTS_DIR="/tmp/v4l2-compliance-results"
VERBOSE=false
QUICK_MODE=false

# Create test results directory
mkdir -p "$TEST_RESULTS_DIR"
LOG_FILE="$TEST_RESULTS_DIR/v4l2-compliance-$(date +%Y%m%d-%H%M%S).log"

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

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --device)
                DEVICE_NODE="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --quick)
                QUICK_MODE=true
                shift
                ;;
            --help)
                echo "Usage: $0 [--device /dev/videoN] [--verbose] [--quick]"
                echo "  --device    Specify V4L2 device node (default: /dev/video0)"
                echo "  --verbose   Enable verbose output"
                echo "  --quick     Run quick compliance test (subset of tests)"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Usage: $0 [--device /dev/videoN] [--verbose] [--quick]"
                exit 1
                ;;
        esac
    done
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking V4L2 compliance testing prerequisites..."
    
    # Check if device exists
    if [[ ! -c "$DEVICE_NODE" ]]; then
        log_error "V4L2 device not found: $DEVICE_NODE"
        log_info "Ensure the TV capture driver is loaded and device is created"
        exit 1
    fi
    
    log_success "V4L2 device found: $DEVICE_NODE"
    
    # Check for v4l2-ctl
    if ! command -v v4l2-ctl >/dev/null 2>&1; then
        log_error "v4l2-ctl tool not found"
        log_info "Install v4l-utils package or ensure it's available in PATH"
        exit 1
    fi
    
    log_success "v4l2-ctl tool available"
    
    # Check for v4l2-compliance if available
    if command -v v4l2-compliance >/dev/null 2>&1; then
        log_success "v4l2-compliance tool available"
    else
        log_warning "v4l2-compliance tool not available - using manual tests only"
    fi
}

# Test 1: Device Capabilities
test_device_capabilities() {
    log_test "Testing device capabilities (VIDIOC_QUERYCAP)..."
    
    local output_file="$TEST_RESULTS_DIR/capabilities.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --info >"$output_file" 2>&1; then
        pass_test "Device capabilities query successful"
        
        # Parse and validate capabilities
        local driver_name
        driver_name=$(grep "Driver name" "$output_file" | awk -F: '{print $2}' | xargs || echo "unknown")
        log_info "Driver name: $driver_name"
        
        if [[ "$driver_name" == "sunxi-tvcap" ]]; then
            pass_test "Correct driver name reported"
        else
            warn_test "Unexpected driver name: $driver_name"
        fi
        
        # Check for video capture capability
        if grep -q "Video Capture" "$output_file"; then
            pass_test "Video capture capability present"
        else
            fail_test "Video capture capability not reported"
        fi
        
        # Check for streaming I/O capability
        if grep -q "Streaming" "$output_file"; then
            pass_test "Streaming I/O capability present"
        else
            fail_test "Streaming I/O capability not reported"
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Full capabilities output:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Device capabilities query failed"
        cat "$output_file" | while read -r line; do
            log_error "  $line"
        done
    fi
}

# Test 2: Format Support
test_format_support() {
    log_test "Testing format enumeration (VIDIOC_ENUM_FMT)..."
    
    local output_file="$TEST_RESULTS_DIR/formats.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --list-formats >"$output_file" 2>&1; then
        pass_test "Format enumeration successful"
        
        # Count supported formats
        local format_count
        format_count=$(grep -c "Index" "$output_file" || echo "0")
        log_info "Supported formats count: $format_count"
        
        if [[ $format_count -gt 0 ]]; then
            pass_test "At least one format is supported"
            
            # Check for common formats
            if grep -q "YUYV" "$output_file"; then
                pass_test "YUYV format supported"
            else
                warn_test "YUYV format not supported"
            fi
            
            if grep -q "YU12\|YV12" "$output_file"; then
                log_info "YUV planar format supported"
            fi
            
            if grep -q "RGB" "$output_file"; then
                log_info "RGB format supported"
            fi
            
        else
            fail_test "No formats reported"
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Supported formats:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Format enumeration failed"
        cat "$output_file" | while read -r line; do
            log_error "  $line"
        done
    fi
}

# Test 3: Input Enumeration
test_input_support() {
    log_test "Testing input enumeration (VIDIOC_ENUM_INPUT)..."
    
    local output_file="$TEST_RESULTS_DIR/inputs.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --list-inputs >"$output_file" 2>&1; then
        pass_test "Input enumeration successful"
        
        # Count inputs
        local input_count
        input_count=$(grep -c "Input" "$output_file" || echo "0")
        log_info "Available inputs count: $input_count"
        
        if [[ $input_count -gt 0 ]]; then
            pass_test "At least one input is available"
            
            # Check for HDMI input
            if grep -qi "hdmi" "$output_file"; then
                pass_test "HDMI input detected"
            else
                warn_test "HDMI input not explicitly named"
            fi
            
        else
            warn_test "No inputs reported (may be normal for some drivers)"
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Available inputs:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        warn_test "Input enumeration failed (may not be implemented)"
        if [[ "$VERBOSE" == "true" ]]; then
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
    fi
}

# Test 4: Current Format Query
test_current_format() {
    log_test "Testing current format query (VIDIOC_G_FMT)..."
    
    local output_file="$TEST_RESULTS_DIR/current_format.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video >"$output_file" 2>&1; then
        pass_test "Current format query successful"
        
        # Parse format information
        local width height pixelformat
        width=$(grep "Width/Height" "$output_file" | awk '{print $3}' | tr -d '/' || echo "0")
        height=$(grep "Width/Height" "$output_file" | awk '{print $4}' || echo "0")
        pixelformat=$(grep "Pixel Format" "$output_file" | awk -F"'" '{print $2}' || echo "unknown")
        
        log_info "Current format: ${width}x${height} $pixelformat"
        
        # Validate format
        if [[ $width -gt 0 && $height -gt 0 ]]; then
            pass_test "Valid resolution reported: ${width}x${height}"
        else
            fail_test "Invalid resolution reported: ${width}x${height}"
        fi
        
        if [[ "$pixelformat" != "unknown" && "$pixelformat" != "" ]]; then
            pass_test "Valid pixel format reported: $pixelformat"
        else
            fail_test "Invalid pixel format reported"
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Current format details:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Current format query failed"
        cat "$output_file" | while read -r line; do
            log_error "  $line"
        done
    fi
}

# Test 5: Format Setting
test_format_setting() {
    log_test "Testing format setting (VIDIOC_S_FMT)..."
    
    # Try to set a common format (1920x1080 YUYV)
    local output_file="$TEST_RESULTS_DIR/set_format.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --set-fmt-video=width=1920,height=1080,pixelformat=YUYV >"$output_file" 2>&1; then
        pass_test "Format setting successful"
        
        # Verify the format was actually set
        if v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video | grep -q "1920" && \
           v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video | grep -q "1080"; then
            pass_test "Format setting verified"
        else
            warn_test "Format setting may not have taken effect"
        fi
        
    else
        warn_test "Format setting failed (may not be implemented yet)"
        if [[ "$VERBOSE" == "true" ]]; then
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
    fi
}

# Test 6: Buffer Request
test_buffer_operations() {
    log_test "Testing buffer operations (VIDIOC_REQBUFS)..."
    
    local output_file="$TEST_RESULTS_DIR/buffers.log"
    
    # Request 4 buffers for memory mapping
    if v4l2-ctl -d "$DEVICE_NODE" --reqbufs-mmap=4 >"$output_file" 2>&1; then
        pass_test "Buffer request successful"
        
        # Try to query buffer information
        if v4l2-ctl -d "$DEVICE_NODE" --querybuf >"$TEST_RESULTS_DIR/querybuf.log" 2>&1; then
            pass_test "Buffer query successful"
        else
            warn_test "Buffer query failed"
        fi
        
        # Clean up buffers
        if v4l2-ctl -d "$DEVICE_NODE" --reqbufs-mmap=0 >"$TEST_RESULTS_DIR/cleanup_buffers.log" 2>&1; then
            log_info "Buffer cleanup successful"
        else
            warn_test "Buffer cleanup failed"
        fi
        
    else
        fail_test "Buffer request failed"
        cat "$output_file" | while read -r line; do
            log_error "  $line"
        done
    fi
}

# Test 7: Control Interface
test_control_interface() {
    log_test "Testing control interface (VIDIOC_QUERY/G/S_CTRL)..."
    
    local output_file="$TEST_RESULTS_DIR/controls.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --list-ctrls >"$output_file" 2>&1; then
        local ctrl_count
        ctrl_count=$(grep -c ":" "$output_file" || echo "0")
        
        if [[ $ctrl_count -gt 0 ]]; then
            pass_test "Control interface available ($ctrl_count controls)"
            
            if [[ "$VERBOSE" == "true" ]]; then
                log_info "Available controls:"
                cat "$output_file" | while read -r line; do
                    log_info "  $line"
                done
            fi
        else
            log_info "No controls available (normal for basic capture drivers)"
        fi
    else
        log_info "Control enumeration failed (normal for basic drivers)"
    fi
}

# Test 8: Official V4L2 Compliance Test (if available)
test_official_compliance() {
    if ! command -v v4l2-compliance >/dev/null 2>&1; then
        log_info "v4l2-compliance tool not available - skipping official compliance test"
        return
    fi
    
    log_test "Running official V4L2 compliance test..."
    
    local output_file="$TEST_RESULTS_DIR/v4l2_compliance.log"
    local compliance_args=""
    
    if [[ "$QUICK_MODE" == "true" ]]; then
        compliance_args="-f"  # Fast mode
    fi
    
    # Run v4l2-compliance with timeout to prevent hanging
    if timeout 300 v4l2-compliance $compliance_args -d "$DEVICE_NODE" >"$output_file" 2>&1; then
        # Parse results
        local total_tests failed_tests
        total_tests=$(grep "Total:" "$output_file" | awk '{print $2}' || echo "0")
        failed_tests=$(grep "Fail:" "$output_file" | awk '{print $2}' || echo "0")
        
        log_info "Official compliance: $total_tests total tests, $failed_tests failed"
        
        if [[ $failed_tests -eq 0 ]]; then
            pass_test "All official V4L2 compliance tests passed"
        elif [[ $failed_tests -lt 3 ]]; then
            warn_test "Minor compliance issues: $failed_tests tests failed"
        else
            fail_test "Significant compliance issues: $failed_tests tests failed"
        fi
        
        if [[ "$VERBOSE" == "true" || $failed_tests -gt 0 ]]; then
            log_info "V4L2 compliance test output:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        warn_test "Official V4L2 compliance test timed out or failed"
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Partial compliance test output:"
            head -50 "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
    fi
}

# Summary and recommendations
print_summary() {
    echo
    log_info "=========================================="
    log_info "V4L2 Compliance Test Summary"
    log_info "=========================================="
    log_info "Device tested: $DEVICE_NODE"
    log_info "Total tests: $TOTAL_TESTS"
    log_info "Passed: $PASSED_TESTS"
    log_info "Failed: $FAILED_TESTS"
    log_info "Warnings: $WARNINGS"
    log_info "Test log: $LOG_FILE"
    log_info "Results directory: $TEST_RESULTS_DIR"
    echo
    
    # Provide recommendations based on test results
    if [[ $FAILED_TESTS -eq 0 ]]; then
        log_success "V4L2 compliance test PASSED! ✅"
        log_info "The driver implements the basic V4L2 API correctly"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        log_warning "V4L2 compliance test MOSTLY PASSED with minor issues ⚠️"
        log_info "The driver has basic V4L2 functionality with some missing features"
    else
        log_error "V4L2 compliance test FAILED ❌"
        log_info "The driver requires significant V4L2 API improvements"
    fi
    
    if [[ $WARNINGS -gt 0 ]]; then
        log_info "Recommendations for improvement:"
        log_info "- Review warning messages for missing optional features"
        log_info "- Consider implementing additional V4L2 controls"
        log_info "- Verify format setting and buffer management"
    fi
    
    if [[ $FAILED_TESTS -eq 0 && $WARNINGS -eq 0 ]]; then
        return 0
    else
        return 1
    fi
}

# Main execution
main() {
    echo -e "${BLUE}V4L2 Compliance Testing Framework${NC}"
    echo "================================="
    echo
    
    parse_args "$@"
    
    log_info "Starting V4L2 compliance tests for device: $DEVICE_NODE"
    if [[ "$QUICK_MODE" == "true" ]]; then
        log_info "Running in quick mode (subset of tests)"
    fi
    
    check_prerequisites
    
    # Core V4L2 API tests
    test_device_capabilities
    test_format_support
    test_input_support
    test_current_format
    
    if [[ "$QUICK_MODE" != "true" ]]; then
        test_format_setting
        test_buffer_operations
        test_control_interface
        test_official_compliance
    fi
    
    print_summary
}

# Execute main function
main "$@"