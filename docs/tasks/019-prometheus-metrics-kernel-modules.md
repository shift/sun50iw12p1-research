# Task 019: Prometheus Metrics Implementation for Kernel Modules

**Status:** in_progress
**Priority:** high  
**Dependencies:** Phase VII kernel modules completed
**Phase:** VIII - Monitoring Integration

## Objective
Implement Prometheus-formatted metrics export via sysfs interface for all HY300 kernel modules to enable comprehensive hardware monitoring and observability.

## Success Criteria
- [ ] MIPS co-processor metrics exported via sysfs
- [ ] HDMI capture metrics exported via sysfs  
- [ ] Motor control metrics exported via sysfs
- [ ] Prometheus text format compliance
- [ ] Integration with NixOS monitoring stack
- [ ] VM testing validation

## Target Kernel Modules
1. **drivers/misc/sunxi-mipsloader.c** (441 lines)
   - Memory region status and utilization
   - MIPS register access counters
   - Firmware loading status and errors
   - Communication protocol metrics

2. **drivers/media/platform/sunxi/sunxi-tvcap.c** (1,760 lines)
   - V4L2 capture frame rates and resolution
   - Buffer allocation and queue status
   - HDMI input signal detection metrics
   - TVTOP/TVCAP register status

3. **drivers/misc/hy300-keystone-motor.c** (420 lines)
   - Motor position and movement counters
   - Calibration status and homing state
   - Step timing and error metrics
   - GPIO state monitoring

## Sysfs Architecture Design
**Target Structure:** `/sys/class/hy300/`
```
/sys/class/hy300/
├── mips/
│   ├── memory_stats
│   ├── register_access_count
│   ├── firmware_status
│   └── communication_errors
├── tvcap/
│   ├── capture_stats
│   ├── buffer_status
│   ├── signal_detection
│   └── frame_metrics
└── motor/
    ├── position_status
    ├── calibration_state
    ├── movement_counters
    └── gpio_status
```

## Implementation Plan
1. **Design metrics architecture and sysfs structure** ✅
2. **Implement MIPS loader metrics**
   - Add sysfs class registration
   - Memory region status attributes
   - Register access counters
   - Communication error tracking
3. **Implement TVCAP metrics**
   - V4L2 capture statistics
   - Buffer management metrics
   - HDMI signal detection status
4. **Implement motor control metrics** 
   - Position and movement tracking
   - Calibration status reporting
   - Timing and error metrics
5. **Test metrics in VM environment**
6. **Integrate with NixOS Prometheus configuration**

## Metrics Schema (Prometheus Format)

### MIPS Co-processor Metrics
```
# HELP hy300_mips_memory_usage_bytes Memory usage in MIPS regions
# TYPE hy300_mips_memory_usage_bytes gauge
hy300_mips_memory_usage_bytes{region="boot_code"} 4096
hy300_mips_memory_usage_bytes{region="firmware"} 12582912
hy300_mips_memory_usage_bytes{region="debug"} 1048576

# HELP hy300_mips_register_access_total Register access count
# TYPE hy300_mips_register_access_total counter
hy300_mips_register_access_total{register="cmd"} 1547
hy300_mips_register_access_total{register="status"} 3094

# HELP hy300_mips_firmware_loaded Firmware loading status
# TYPE hy300_mips_firmware_loaded gauge
hy300_mips_firmware_loaded 1
```

### HDMI Capture Metrics
```
# HELP hy300_tvcap_frames_captured_total Total captured frames
# TYPE hy300_tvcap_frames_captured_total counter
hy300_tvcap_frames_captured_total 15678

# HELP hy300_tvcap_buffer_usage_ratio Buffer usage ratio
# TYPE hy300_tvcap_buffer_usage_ratio gauge
hy300_tvcap_buffer_usage_ratio 0.75

# HELP hy300_tvcap_signal_detected HDMI signal detection status
# TYPE hy300_tvcap_signal_detected gauge
hy300_tvcap_signal_detected 1
```

### Motor Control Metrics
```
# HELP hy300_motor_position_steps Current motor position in steps
# TYPE hy300_motor_position_steps gauge
hy300_motor_position_steps 127

# HELP hy300_motor_movements_total Total motor movements
# TYPE hy300_motor_movements_total counter
hy300_motor_movements_total 45

# HELP hy300_motor_homed Motor homing status
# TYPE hy300_motor_homed gauge
hy300_motor_homed 1
```

## Implementation Notes
- **Session Safety**: Use patch-based editing for all .c file modifications
- **Kernel Standards**: Follow Linux kernel sysfs attribute conventions
- **Performance**: Minimal overhead for metrics collection
- **Error Handling**: Graceful degradation if sysfs creation fails
- **Prometheus Compliance**: Proper metric naming and help text

## Testing Strategy
1. **VM Environment**: Test sysfs attribute creation and reading
2. **Metrics Format**: Validate Prometheus text format compliance
3. **Performance**: Ensure minimal impact on driver functionality
4. **Integration**: Test with NixOS Prometheus scraping

## External References
- Linux kernel sysfs documentation
- Prometheus metric naming conventions
- V4L2 statistics interfaces
- Platform device attribute management
