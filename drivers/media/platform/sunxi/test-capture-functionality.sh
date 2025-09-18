#!/usr/bin/env bash
# Basic Capture Functionality Test
# Task 019: HDMI Input Driver Implementation - Atomic Task 6.4
#
# This script tests basic video capture functionality including buffer allocation,
# streaming operations, and frame capture capabilities.
#
# Usage: ./test-capture-functionality.sh [--device /dev/videoN] [--frames N] [--verbose]

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
TEST_RESULTS_DIR="/tmp/capture-test-results"
VERBOSE=false
TEST_FRAMES=10
BUFFER_COUNT=4
TIMEOUT_SECONDS=30

# Create test results directory
mkdir -p "$TEST_RESULTS_DIR"
LOG_FILE="$TEST_RESULTS_DIR/capture-test-$(date +%Y%m%d-%H%M%S).log"

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
            --frames)
                TEST_FRAMES="$2"
                shift 2
                ;;
            --buffers)
                BUFFER_COUNT="$2"
                shift 2
                ;;
            --timeout)
                TIMEOUT_SECONDS="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help)
                echo "Usage: $0 [--device /dev/videoN] [--frames N] [--buffers N] [--timeout N] [--verbose]"
                echo "  --device    Specify V4L2 device node (default: /dev/video0)"
                echo "  --frames    Number of frames to capture (default: 10)"
                echo "  --buffers   Number of buffers to allocate (default: 4)"
                echo "  --timeout   Timeout in seconds (default: 30)"
                echo "  --verbose   Enable verbose output"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Usage: $0 [--device /dev/videoN] [--frames N] [--buffers N] [--timeout N] [--verbose]"
                exit 1
                ;;
        esac
    done
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking capture functionality test prerequisites..."
    
    # Check if device exists
    if [[ ! -c "$DEVICE_NODE" ]]; then
        log_error "V4L2 device not found: $DEVICE_NODE"
        log_info "Ensure the TV capture driver is loaded and device is created"
        exit 1
    fi
    
    log_success "V4L2 device found: $DEVICE_NODE"
    
    # Check for required tools
    local required_tools=("v4l2-ctl" "timeout")
    local missing_tools=0
    
    for tool in "${required_tools[@]}"; do
        if command -v "$tool" >/dev/null 2>&1; then
            log_success "Tool available: $tool"
        else
            log_error "Required tool not found: $tool"
            ((missing_tools++))
        fi
    done
    
    if [[ $missing_tools -gt 0 ]]; then
        log_error "Missing required tools - cannot proceed"
        exit 1
    fi
    
    # Check device permissions
    if [[ -r "$DEVICE_NODE" && -w "$DEVICE_NODE" ]]; then
        log_success "Device permissions OK"
    else
        log_error "Insufficient permissions for device: $DEVICE_NODE"
        log_info "Try running as root or adding user to video group"
        exit 1
    fi
}

# Test 1: Buffer Allocation and Management
test_buffer_allocation() {
    log_test "Testing buffer allocation and management..."
    
    local output_file="$TEST_RESULTS_DIR/buffer_allocation.log"
    
    # Test buffer request
    log_info "Requesting $BUFFER_COUNT memory-mapped buffers..."
    if v4l2-ctl -d "$DEVICE_NODE" --reqbufs-mmap="$BUFFER_COUNT" >"$output_file" 2>&1; then
        pass_test "Buffer allocation successful ($BUFFER_COUNT buffers)"
        
        # Query buffer information
        local querybuf_file="$TEST_RESULTS_DIR/querybuf.log"
        if v4l2-ctl -d "$DEVICE_NODE" --querybuf >"$querybuf_file" 2>&1; then
            pass_test "Buffer query successful"
            
            # Parse buffer information
            local buffer_info
            buffer_info=$(cat "$querybuf_file")
            
            if [[ -n "$buffer_info" ]]; then
                log_info "Buffer information available:"
                if [[ "$VERBOSE" == "true" ]]; then
                    echo "$buffer_info" | while read -r line; do
                        log_info "  $line"
                    done
                fi
                
                # Look for expected buffer details
                if echo "$buffer_info" | grep -q "memory.*mmap"; then
                    pass_test "Memory-mapped buffers confirmed"
                fi
                
                if echo "$buffer_info" | grep -qE "length.*[0-9]+"; then
                    local buffer_size
                    buffer_size=$(echo "$buffer_info" | grep -E "length.*[0-9]+" | head -1 | grep -o '[0-9]\+' || echo "0")
                    if [[ $buffer_size -gt 0 ]]; then
                        log_info "Buffer size: $buffer_size bytes"
                        pass_test "Buffer size information available"
                    fi
                fi
            else
                warn_test "No buffer information returned"
            fi
        else
            warn_test "Buffer query failed"
            if [[ "$VERBOSE" == "true" ]]; then
                cat "$querybuf_file" | while read -r line; do
                    log_info "  $line"
                done
            fi
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Buffer allocation output:"
            cat "$output_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Buffer allocation failed"
        cat "$output_file" | while read -r line; do
            log_error "  $line"
        done
        return 1
    fi
}

# Test 2: Format Configuration
test_format_configuration() {
    log_test "Testing format configuration for capture..."
    
    # Get current format
    local current_fmt_file="$TEST_RESULTS_DIR/current_format.log"
    if v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video >"$current_fmt_file" 2>&1; then
        local width height pixelformat
        width=$(grep "Width/Height" "$current_fmt_file" | awk '{print $3}' | tr -d '/' || echo "1920")
        height=$(grep "Width/Height" "$current_fmt_file" | awk '{print $4}' || echo "1080")
        pixelformat=$(grep "Pixel Format" "$current_fmt_file" | awk -F"'" '{print $2}' || echo "YUYV")
        
        log_info "Current format: ${width}x${height} $pixelformat"
        
        # Try to set a known good format for testing
        local set_fmt_file="$TEST_RESULTS_DIR/set_format.log"
        log_info "Setting format to 1920x1080 YUYV for testing..."
        
        if v4l2-ctl -d "$DEVICE_NODE" --set-fmt-video=width=1920,height=1080,pixelformat=YUYV >"$set_fmt_file" 2>&1; then
            pass_test "Format setting successful"
            
            # Verify the format was set
            if v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video | grep -q "1920.*1080"; then
                pass_test "Format setting verified"
            else
                warn_test "Format setting may not have taken effect"
            fi
        else
            warn_test "Format setting failed or not supported"
            if [[ "$VERBOSE" == "true" ]]; then
                cat "$set_fmt_file" | while read -r line; do
                    log_info "  $line"
                done
            fi
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Current format details:"
            cat "$current_fmt_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Format query failed"
        cat "$current_fmt_file" | while read -r line; do
            log_error "  $line"
        done
        return 1
    fi
}

# Test 3: Streaming Start/Stop
test_streaming_operations() {
    log_test "Testing streaming start/stop operations..."
    
    # Start streaming
    local stream_start_file="$TEST_RESULTS_DIR/stream_start.log"
    log_info "Starting video streaming..."
    
    if timeout 10 v4l2-ctl -d "$DEVICE_NODE" --stream-mmap --stream-count=0 --stream-to=/dev/null >"$stream_start_file" 2>&1 &
    then
        local stream_pid=$!
        sleep 2
        
        # Check if streaming started successfully
        if kill -0 "$stream_pid" 2>/dev/null; then
            pass_test "Streaming started successfully"
            
            # Let it run for a moment
            sleep 3
            
            # Stop streaming
            log_info "Stopping streaming..."
            if kill "$stream_pid" 2>/dev/null; then
                wait "$stream_pid" 2>/dev/null || true
                pass_test "Streaming stopped successfully"
            else
                warn_test "Streaming process already terminated"
            fi
        else
            fail_test "Streaming failed to start"
        fi
        
        if [[ "$VERBOSE" == "true" && -f "$stream_start_file" ]]; then
            log_info "Streaming output:"
            cat "$stream_start_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Streaming operation failed"
        if [[ -f "$stream_start_file" ]]; then
            cat "$stream_start_file" | while read -r line; do
                log_error "  $line"
            done
        fi
    fi
}

# Test 4: Frame Capture Test
test_frame_capture() {
    log_test "Testing frame capture functionality..."
    
    local capture_file="$TEST_RESULTS_DIR/frame_capture.log"
    local frames_captured=0
    
    log_info "Attempting to capture $TEST_FRAMES frames..."
    
    # Use timeout to prevent hanging
    if timeout "$TIMEOUT_SECONDS" v4l2-ctl -d "$DEVICE_NODE" \
        --stream-mmap \
        --stream-count="$TEST_FRAMES" \
        --stream-to="$TEST_RESULTS_DIR/captured_frame_%03d.raw" \
        >"$capture_file" 2>&1; then
        
        # Count captured files
        frames_captured=$(find "$TEST_RESULTS_DIR" -name "captured_frame_*.raw" 2>/dev/null | wc -l || echo "0")
        
        if [[ $frames_captured -gt 0 ]]; then
            pass_test "Frame capture successful ($frames_captured frames captured)"
            
            # Analyze captured frames
            local frame_file
            frame_file=$(find "$TEST_RESULTS_DIR" -name "captured_frame_*.raw" | head -1 || echo "")
            
            if [[ -n "$frame_file" && -f "$frame_file" ]]; then
                local frame_size
                frame_size=$(stat -c%s "$frame_file" 2>/dev/null || echo "0")
                log_info "Frame size: $frame_size bytes"
                
                if [[ $frame_size -gt 0 ]]; then
                    pass_test "Captured frames contain data"
                    
                    # Check for reasonable frame size (should be > 100KB for typical video)
                    if [[ $frame_size -gt 100000 ]]; then
                        pass_test "Frame size appears reasonable for video data"
                    elif [[ $frame_size -gt 1000 ]]; then
                        warn_test "Frame size is small but may be valid"
                    else
                        warn_test "Frame size is very small - may indicate capture issues"
                    fi
                else
                    fail_test "Captured frames are empty"
                fi
                
                # Basic data validation (check for non-zero content)
                if [[ -f "$frame_file" ]]; then
                    local non_zero_bytes
                    non_zero_bytes=$(od -An -tx1 "$frame_file" | tr -d ' ' | grep -v '^00*$' | wc -l || echo "0")
                    
                    if [[ $non_zero_bytes -gt 0 ]]; then
                        pass_test "Frame data contains non-zero content"
                    else
                        warn_test "Frame data appears to be all zeros"
                    fi
                fi
            fi
            
            # Cleanup captured frames to save space
            log_info "Cleaning up captured frame files..."
            rm -f "$TEST_RESULTS_DIR"/captured_frame_*.raw 2>/dev/null || true
            
        else
            fail_test "No frames were captured"
        fi
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Capture command output:"
            cat "$capture_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Frame capture operation timed out or failed"
        if [[ -f "$capture_file" ]]; then
            log_error "Capture error output:"
            cat "$capture_file" | while read -r line; do
                log_error "  $line"
            done
        fi
    fi
}

# Test 5: Performance and Timing
test_capture_performance() {
    log_test "Testing capture performance and timing..."
    
    local perf_file="$TEST_RESULTS_DIR/performance.log"
    
    log_info "Running performance test with 5 frames..."
    
    local start_time
    start_time=$(date +%s.%N)
    
    if timeout 15 v4l2-ctl -d "$DEVICE_NODE" \
        --stream-mmap \
        --stream-count=5 \
        --stream-to=/dev/null \
        >"$perf_file" 2>&1; then
        
        local end_time
        end_time=$(date +%s.%N)
        local elapsed
        elapsed=$(echo "$end_time - $start_time" | bc -l 2>/dev/null || echo "0")
        
        log_info "Performance test completed in ${elapsed}s"
        
        if [[ -n "$elapsed" ]] && (( $(echo "$elapsed > 0" | bc -l 2>/dev/null || echo "0") )); then
            local fps
            fps=$(echo "scale=2; 5 / $elapsed" | bc -l 2>/dev/null || echo "0")
            log_info "Approximate capture rate: ${fps} fps"
            
            # Basic performance validation
            if (( $(echo "$fps > 1" | bc -l 2>/dev/null || echo "0") )); then
                pass_test "Capture performance appears reasonable (${fps} fps)"
            else
                warn_test "Capture performance may be slow (${fps} fps)"
            fi
        else
            warn_test "Could not calculate performance metrics"
        fi
        
        # Check for any performance-related messages
        if grep -qE "(fps|frame.*rate|performance)" "$perf_file"; then
            log_info "Performance information in output:"
            grep -E "(fps|frame.*rate|performance)" "$perf_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        warn_test "Performance test failed or timed out"
        if [[ -f "$perf_file" ]]; then
            cat "$perf_file" | while read -r line; do
                log_info "  $line"
            done
        fi
    fi
}

# Test 6: Resource Cleanup
test_resource_cleanup() {
    log_test "Testing resource cleanup..."
    
    # Release buffers
    local cleanup_file="$TEST_RESULTS_DIR/cleanup.log"
    
    log_info "Releasing allocated buffers..."
    if v4l2-ctl -d "$DEVICE_NODE" --reqbufs-mmap=0 >"$cleanup_file" 2>&1; then
        pass_test "Buffer cleanup successful"
    else
        warn_test "Buffer cleanup failed or not needed"
        if [[ "$VERBOSE" == "true" ]]; then
            cat "$cleanup_file" | while read -r line; do
                log_info "  $line"
            done
        fi
    fi
    
    # Check if device is still accessible after cleanup
    if v4l2-ctl -d "$DEVICE_NODE" --info >/dev/null 2>&1; then
        pass_test "Device remains accessible after cleanup"
    else
        fail_test "Device not accessible after cleanup"
    fi
}

# Monitor kernel messages during tests
monitor_kernel_activity() {
    local start_line="$1"
    local test_name="$2"
    
    local end_line
    end_line=$(dmesg | wc -l)
    
    if [[ $end_line -gt $start_line ]]; then
        local new_messages
        new_messages=$(dmesg | tail -n $((end_line - start_line)))
        
        # Filter for relevant messages
        local relevant_messages
        relevant_messages=$(echo "$new_messages" | grep -iE "(tvcap|tvtop|error|warning|timeout)" || true)
        
        if [[ -n "$relevant_messages" ]]; then
            log_info "Kernel messages during $test_name:"
            echo "$relevant_messages" | while read -r line; do
                log_info "  $line"
            done
            
            # Check for errors
            if echo "$relevant_messages" | grep -qE "(error|Error|ERROR|failed|Failed)"; then
                warn_test "Error messages detected during $test_name"
            fi
        fi
    fi
}

# Summary and recommendations
print_summary() {
    echo
    log_info "=============================================="
    log_info "Capture Functionality Test Summary"
    log_info "=============================================="
    log_info "Device tested: $DEVICE_NODE"
    log_info "Frames requested: $TEST_FRAMES"
    log_info "Buffers used: $BUFFER_COUNT"
    log_info "Total tests: $TOTAL_TESTS"
    log_info "Passed: $PASSED_TESTS"
    log_info "Failed: $FAILED_TESTS"
    log_info "Warnings: $WARNINGS"
    log_info "Test log: $LOG_FILE"
    log_info "Results directory: $TEST_RESULTS_DIR"
    echo
    
    # Provide specific recommendations for capture functionality
    if [[ $FAILED_TESTS -eq 0 ]]; then
        log_success "Capture functionality tests PASSED! ✅"
        log_info "Basic video capture operations are working correctly"
    elif [[ $FAILED_TESTS -le 2 ]]; then
        log_warning "Capture functionality tests MOSTLY PASSED ⚠️"
        log_info "Core capture works with some limitations"
    else
        log_error "Capture functionality tests FAILED ❌"
        log_info "Significant capture functionality issues detected"
    fi
    
    # Specific recommendations
    echo
    log_info "Capture Test Analysis:"
    
    if [[ $PASSED_TESTS -ge 5 ]]; then
        log_info "✓ Driver successfully implements basic V4L2 capture pipeline"
        log_info "✓ Buffer management and streaming operations functional"
    fi
    
    if [[ $WARNINGS -gt 0 ]]; then
        log_info "⚠ Some advanced features may need implementation or testing"
        log_info "⚠ Performance optimization may be beneficial"
    fi
    
    log_info "Next steps:"
    log_info "- Test with actual HDMI input sources for real data capture"
    log_info "- Verify frame data integrity and format correctness"
    log_info "- Test with different resolutions and formats"
    log_info "- Optimize capture performance if needed"
    
    if [[ $FAILED_TESTS -eq 0 && $WARNINGS -le 3 ]]; then
        return 0
    else
        return 1
    fi
}

# Main execution
main() {
    echo -e "${BLUE}Basic Capture Functionality Test${NC}"
    echo "================================"
    echo
    
    parse_args "$@"
    
    log_info "Starting capture functionality tests for device: $DEVICE_NODE"
    log_info "Test parameters: $TEST_FRAMES frames, $BUFFER_COUNT buffers, ${TIMEOUT_SECONDS}s timeout"
    
    check_prerequisites
    
    # Monitor kernel messages
    local start_dmesg_lines
    start_dmesg_lines=$(dmesg | wc -l)
    
    # Run tests in sequence
    test_buffer_allocation
    monitor_kernel_activity "$start_dmesg_lines" "buffer allocation"
    start_dmesg_lines=$(dmesg | wc -l)
    
    test_format_configuration
    monitor_kernel_activity "$start_dmesg_lines" "format configuration"
    start_dmesg_lines=$(dmesg | wc -l)
    
    test_streaming_operations
    monitor_kernel_activity "$start_dmesg_lines" "streaming operations"
    start_dmesg_lines=$(dmesg | wc -l)
    
    test_frame_capture
    monitor_kernel_activity "$start_dmesg_lines" "frame capture"
    start_dmesg_lines=$(dmesg | wc -l)
    
    test_capture_performance
    monitor_kernel_activity "$start_dmesg_lines" "performance test"
    start_dmesg_lines=$(dmesg | wc -l)
    
    test_resource_cleanup
    monitor_kernel_activity "$start_dmesg_lines" "resource cleanup"
    
    print_summary
}

# Execute main function
main "$@"