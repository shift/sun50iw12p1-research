#!/usr/bin/env bash
# HDMI Input Detection Validation Test
# Task 019: HDMI Input Driver Implementation - Atomic Task 6.3
#
# This script tests HDMI input detection functionality including hot-plug detection,
# signal presence validation, and format auto-detection capabilities.
#
# Usage: ./test-hdmi-input-detection.sh [--device /dev/videoN] [--monitor] [--verbose]

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
TEST_RESULTS_DIR="/tmp/hdmi-input-test-results"
VERBOSE=false
MONITOR_MODE=false
MONITOR_DURATION=30

# Create test results directory
mkdir -p "$TEST_RESULTS_DIR"
LOG_FILE="$TEST_RESULTS_DIR/hdmi-input-test-$(date +%Y%m%d-%H%M%S).log"

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
            --monitor)
                MONITOR_MODE=true
                shift
                ;;
            --duration)
                MONITOR_DURATION="$2"
                shift 2
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help)
                echo "Usage: $0 [--device /dev/videoN] [--monitor] [--duration N] [--verbose]"
                echo "  --device     Specify V4L2 device node (default: /dev/video0)"
                echo "  --monitor    Enable continuous monitoring mode"
                echo "  --duration   Monitor duration in seconds (default: 30)"
                echo "  --verbose    Enable verbose output"
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo "Usage: $0 [--device /dev/videoN] [--monitor] [--duration N] [--verbose]"
                exit 1
                ;;
        esac
    done
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking HDMI input detection test prerequisites..."
    
    # Check if device exists
    if [[ ! -c "$DEVICE_NODE" ]]; then
        log_error "V4L2 device not found: $DEVICE_NODE"
        log_info "Ensure the TV capture driver is loaded and device is created"
        exit 1
    fi
    
    log_success "V4L2 device found: $DEVICE_NODE"
    
    # Check for required tools
    local required_tools=("v4l2-ctl" "dmesg")
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
}

# Get current kernel message count for monitoring
get_dmesg_line_count() {
    dmesg | wc -l
}

# Monitor kernel messages for HDMI events
monitor_kernel_messages() {
    local start_line="$1"
    local end_line
    end_line=$(get_dmesg_line_count)
    
    if [[ $end_line -gt $start_line ]]; then
        local new_messages
        new_messages=$(dmesg | tail -n $((end_line - start_line)))
        
        # Filter for HDMI/TV capture related messages
        local hdmi_messages
        hdmi_messages=$(echo "$new_messages" | grep -iE "(hdmi|tvcap|tvtop|hotplug|hpd)" || true)
        
        if [[ -n "$hdmi_messages" ]]; then
            log_info "HDMI-related kernel messages:"
            echo "$hdmi_messages" | while read -r line; do
                log_info "  $line"
            done
            return 0
        fi
    fi
    
    return 1
}

# Test 1: Basic HDMI Input Status
test_hdmi_input_status() {
    log_test "Testing basic HDMI input status detection..."
    
    local output_file="$TEST_RESULTS_DIR/input_status.log"
    
    # Get current input status
    if v4l2-ctl -d "$DEVICE_NODE" --get-input >"$output_file" 2>&1; then
        local input_num
        input_num=$(cat "$output_file" | grep -o '[0-9]*' || echo "0")
        log_info "Current input: $input_num"
        pass_test "Input status query successful"
    else
        warn_test "Input status query failed (may not be implemented)"
    fi
    
    # Try to get input information
    if v4l2-ctl -d "$DEVICE_NODE" --list-inputs >"$TEST_RESULTS_DIR/inputs_detailed.log" 2>&1; then
        local input_count
        input_count=$(grep -c "Input" "$TEST_RESULTS_DIR/inputs_detailed.log" || echo "0")
        
        if [[ $input_count -gt 0 ]]; then
            pass_test "Input enumeration successful ($input_count inputs found)"
            
            # Look for HDMI-specific information
            if grep -qi "hdmi" "$TEST_RESULTS_DIR/inputs_detailed.log"; then
                pass_test "HDMI input explicitly identified"
            else
                warn_test "HDMI input not explicitly named"
            fi
            
            # Check for status information
            if grep -qE "(connected|active|signal)" "$TEST_RESULTS_DIR/inputs_detailed.log"; then
                log_info "Input status information available"
            else
                warn_test "No input status information reported"
            fi
            
            if [[ "$VERBOSE" == "true" ]]; then
                log_info "Input details:"
                cat "$TEST_RESULTS_DIR/inputs_detailed.log" | while read -r line; do
                    log_info "  $line"
                done
            fi
        else
            warn_test "No inputs detected or enumeration not supported"
        fi
    else
        warn_test "Input enumeration failed"
    fi
}

# Test 2: Signal Detection Capabilities
test_signal_detection() {
    log_test "Testing HDMI signal detection capabilities..."
    
    # Check device capabilities for signal detection
    local caps_file="$TEST_RESULTS_DIR/signal_caps.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --info >"$caps_file" 2>&1; then
        # Look for relevant capabilities
        if grep -qE "(Video Capture|Streaming)" "$caps_file"; then
            pass_test "Basic capture capabilities present"
        else
            fail_test "Video capture capability not reported"
        fi
        
        # Check for additional signal-related information
        if v4l2-ctl -d "$DEVICE_NODE" --get-dv-timings >"$TEST_RESULTS_DIR/dv_timings.log" 2>&1; then
            log_info "DV timings query supported - checking for signal timing detection"
            
            local timing_info
            timing_info=$(cat "$TEST_RESULTS_DIR/dv_timings.log")
            
            if [[ -n "$timing_info" ]]; then
                log_info "Timing information available:"
                echo "$timing_info" | while read -r line; do
                    log_info "  $line"
                done
                pass_test "Signal timing detection supported"
            else
                log_info "No timing information currently available (normal with no signal)"
            fi
        else
            log_info "DV timings not supported (normal for basic drivers)"
        fi
    else
        fail_test "Device capabilities query failed"
    fi
}

# Test 3: Format Auto-Detection
test_format_autodetection() {
    log_test "Testing format auto-detection capabilities..."
    
    # Get current format
    local format_file="$TEST_RESULTS_DIR/current_format.log"
    
    if v4l2-ctl -d "$DEVICE_NODE" --get-fmt-video >"$format_file" 2>&1; then
        # Parse format information
        local width height pixelformat
        width=$(grep "Width/Height" "$format_file" | awk '{print $3}' | tr -d '/' || echo "0")
        height=$(grep "Width/Height" "$format_file" | awk '{print $4}' || echo "0")
        pixelformat=$(grep "Pixel Format" "$format_file" | awk -F"'" '{print $2}' || echo "unknown")
        
        log_info "Current detected format: ${width}x${height} $pixelformat"
        
        # Check if format looks reasonable
        if [[ $width -ge 640 && $height -ge 480 ]]; then
            pass_test "Reasonable video format detected: ${width}x${height}"
        elif [[ $width -eq 0 || $height -eq 0 ]]; then
            log_info "No signal detected (0x0 resolution is normal)"
        else
            warn_test "Unusual video format detected: ${width}x${height}"
        fi
        
        # Check for common formats
        case "$pixelformat" in
            "YUYV"|"YU12"|"YV12"|"RGB3"|"RGB4")
                pass_test "Standard pixel format detected: $pixelformat"
                ;;
            "unknown"|"")
                warn_test "Pixel format not detected or invalid"
                ;;
            *)
                log_info "Custom or unusual pixel format: $pixelformat"
                ;;
        esac
        
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Complete format information:"
            cat "$format_file" | while read -r line; do
                log_info "  $line"
            done
        fi
        
    else
        fail_test "Format query failed"
        cat "$format_file" | while read -r line; do
            log_error "  $line"
        done
    fi
}

# Test 4: Hot-Plug Event Detection
test_hotplug_detection() {
    log_test "Testing HDMI hot-plug event detection..."
    
    log_info "Monitoring for hot-plug events (requires manual cable connection/disconnection)"
    log_info "This test monitors kernel messages for hot-plug related events"
    
    local start_dmesg_lines
    start_dmesg_lines=$(get_dmesg_line_count)
    
    # Monitor for a short period to see if there are any existing events
    log_info "Monitoring kernel messages for 10 seconds..."
    
    for i in {1..10}; do
        sleep 1
        if monitor_kernel_messages "$start_dmesg_lines"; then
            pass_test "HDMI-related kernel activity detected"
            return
        fi
        start_dmesg_lines=$(get_dmesg_line_count)
    done
    
    log_info "No HDMI events detected during monitoring period"
    log_info "Note: Hot-plug detection requires physical cable connection/disconnection"
    warn_test "Hot-plug event monitoring inconclusive (no events during test period)"
}

# Test 5: V4L2 Event System
test_v4l2_events() {
    log_test "Testing V4L2 event system for input changes..."
    
    # Check if device supports V4L2 events
    if v4l2-ctl -d "$DEVICE_NODE" --list-devices >"$TEST_RESULTS_DIR/events.log" 2>&1; then
        log_info "Device enumeration successful"
    fi
    
    # Try to subscribe to events (this may not be supported)
    local event_file="$TEST_RESULTS_DIR/event_test.log"
    
    # Use timeout to prevent hanging
    if timeout 5 v4l2-ctl -d "$DEVICE_NODE" --wait-for-event=5 >"$event_file" 2>&1; then
        local event_output
        event_output=$(cat "$event_file")
        
        if [[ -n "$event_output" ]]; then
            log_info "V4L2 event output:"
            echo "$event_output" | while read -r line; do
                log_info "  $line"
            done
            pass_test "V4L2 event system responsive"
        else
            log_info "No events received during test period"
        fi
    else
        log_info "V4L2 event monitoring timed out or not supported"
        log_info "(This is normal for drivers that don't implement event system)"
    fi
}

# Test 6: Hardware Register Status (Kernel Messages)
test_hardware_status() {
    log_test "Testing hardware status from kernel messages..."
    
    # Look for recent HDMI/TV capture related kernel messages
    local recent_messages
    recent_messages=$(dmesg | grep -iE "(hdmi|tvcap|tvtop)" | tail -20 || true)
    
    if [[ -n "$recent_messages" ]]; then
        log_info "Recent HDMI/TV capture kernel messages:"
        echo "$recent_messages" | while read -r line; do
            log_info "  $line"
        done
        
        # Analyze messages for status indicators
        if echo "$recent_messages" | grep -qE "(connected|detected|ready|initialized)"; then
            pass_test "Positive hardware status indicators found"
        elif echo "$recent_messages" | grep -qE "(error|failed|timeout|disconnected)"; then
            warn_test "Negative hardware status indicators found"
        else
            log_info "Hardware status indicators neutral or unclear"
        fi
        
        # Look for initialization messages
        if echo "$recent_messages" | grep -q "initialized successfully"; then
            pass_test "Hardware initialization success confirmed"
        fi
        
    else
        warn_test "No recent HDMI/TV capture kernel messages found"
        log_info "This may indicate the driver is not generating status messages"
    fi
}

# Continuous Monitoring Mode
continuous_monitoring() {
    log_info "Starting continuous HDMI monitoring mode for $MONITOR_DURATION seconds..."
    log_info "Connect/disconnect HDMI cable to test hot-plug detection"
    
    local start_time
    start_time=$(date +%s)
    local start_dmesg_lines
    start_dmesg_lines=$(get_dmesg_line_count)
    local events_detected=0
    
    while true; do
        local current_time
        current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        
        if [[ $elapsed -ge $MONITOR_DURATION ]]; then
            break
        fi
        
        # Check for kernel messages
        if monitor_kernel_messages "$start_dmesg_lines"; then
            ((events_detected++))
            log_success "HDMI event detected! (Event #$events_detected)"
        fi
        
        start_dmesg_lines=$(get_dmesg_line_count)
        
        # Show progress every 5 seconds
        if [[ $((elapsed % 5)) -eq 0 ]]; then
            local remaining=$((MONITOR_DURATION - elapsed))
            log_info "Monitoring... ${remaining}s remaining (${events_detected} events detected)"
        fi
        
        sleep 1
    done
    
    log_info "Monitoring completed. Total events detected: $events_detected"
    
    if [[ $events_detected -gt 0 ]]; then
        pass_test "Hot-plug events successfully detected during monitoring"
    else
        warn_test "No hot-plug events detected during monitoring period"
        log_info "This may be normal if no cable connections/disconnections occurred"
    fi
}

# Summary and recommendations
print_summary() {
    echo
    log_info "=============================================="
    log_info "HDMI Input Detection Test Summary"
    log_info "=============================================="
    log_info "Device tested: $DEVICE_NODE"
    log_info "Total tests: $TOTAL_TESTS"
    log_info "Passed: $PASSED_TESTS"
    log_info "Failed: $FAILED_TESTS"
    log_info "Warnings: $WARNINGS"
    log_info "Test log: $LOG_FILE"
    log_info "Results directory: $TEST_RESULTS_DIR"
    echo
    
    # Provide specific recommendations for HDMI functionality
    if [[ $FAILED_TESTS -eq 0 ]]; then
        log_success "HDMI input detection tests PASSED! ✅"
        log_info "Basic HDMI input functionality is working correctly"
    elif [[ $FAILED_TESTS -le 1 ]]; then
        log_warning "HDMI input detection tests MOSTLY PASSED ⚠️"
        log_info "Core functionality working with some limitations"
    else
        log_error "HDMI input detection tests FAILED ❌"
        log_info "Significant HDMI input functionality issues detected"
    fi
    
    # Specific recommendations
    echo
    log_info "Recommendations:"
    
    if [[ $WARNINGS -gt 0 ]]; then
        log_info "- Review warning messages for missing HDMI features"
        log_info "- Consider implementing V4L2 event system for hot-plug detection"
        log_info "- Verify signal detection and format auto-detection capabilities"
    fi
    
    log_info "- Test with actual HDMI sources for full validation"
    log_info "- Use continuous monitoring mode to test hot-plug detection"
    log_info "- Verify EDID reading capabilities when MIPS communication is available"
    
    if [[ $FAILED_TESTS -eq 0 && $WARNINGS -le 2 ]]; then
        return 0
    else
        return 1
    fi
}

# Main execution
main() {
    echo -e "${BLUE}HDMI Input Detection Validation Test${NC}"
    echo "===================================="
    echo
    
    parse_args "$@"
    
    log_info "Starting HDMI input detection tests for device: $DEVICE_NODE"
    
    check_prerequisites
    
    # Run core tests
    test_hdmi_input_status
    test_signal_detection
    test_format_autodetection
    test_v4l2_events
    test_hardware_status
    
    if [[ "$MONITOR_MODE" != "true" ]]; then
        test_hotplug_detection
    fi
    
    # Run continuous monitoring if requested
    if [[ "$MONITOR_MODE" == "true" ]]; then
        continuous_monitoring
    fi
    
    print_summary
}

# Execute main function
main "$@"