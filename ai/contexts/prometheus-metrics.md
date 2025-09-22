# Prometheus Metrics Integration for HY300 Kernel Modules

## Overview
Comprehensive metrics collection framework for HY300 hardware monitoring, service performance tracking, and system health validation.

## Metrics Architecture

### Kernel Module Metrics
**Integration Method**: Procfs and sysfs interfaces with Prometheus exposition
**Collection Agent**: Custom Python exporter with hardware-specific metrics
**Update Frequency**: Real-time hardware metrics, 1-second service metrics

#### HDMI Input Driver Metrics
**Source**: `drivers/media/platform/sunxi/sunxi-tvcap-enhanced.c`
**Metrics Collected**:
```
# Hardware status metrics
hy300_hdmi_input_connected{port="hdmi1"} 1                    # Input connection status
hy300_hdmi_input_format{port="hdmi1",format="1080p60"} 1      # Current input format
hy300_hdmi_input_signal_quality{port="hdmi1"} 0.95            # Signal quality (0-1)

# Performance metrics  
hy300_hdmi_frames_captured_total{port="hdmi1"} 1234567        # Total frames captured
hy300_hdmi_frame_drops_total{port="hdmi1"} 42                 # Dropped frames
hy300_hdmi_buffer_underruns_total{port="hdmi1"} 3             # Buffer underruns
hy300_hdmi_processing_latency_seconds{port="hdmi1"} 0.016     # Frame processing latency

# Error metrics
hy300_hdmi_errors_total{port="hdmi1",type="format_change"} 5  # Format change errors
hy300_hdmi_errors_total{port="hdmi1",type="sync_loss"} 2      # Sync loss errors
```

**Implementation**: Procfs entries in `/proc/hy300/hdmi/`
```c
// In sunxi-tvcap-enhanced.c
static struct proc_dir_entry *hdmi_metrics_dir;

static int hdmi_metrics_show(struct seq_file *m, void *v) {
    struct tvcap_dev *dev = m->private;
    
    seq_printf(m, "frames_captured %llu\n", dev->frames_captured);
    seq_printf(m, "frame_drops %llu\n", dev->frame_drops);
    seq_printf(m, "buffer_underruns %u\n", dev->buffer_underruns);
    seq_printf(m, "signal_quality %u\n", dev->signal_quality);
    
    return 0;
}
```

#### Keystone Motor Driver Metrics
**Source**: `drivers/misc/hy300-keystone-motor.c`
**Metrics Collected**:
```
# Motor position and status
hy300_keystone_position{axis="horizontal"} 50                 # Current position (0-100)
hy300_keystone_position{axis="vertical"} 30                   # Current position (0-100)
hy300_keystone_motor_status{axis="horizontal"} 1              # Motor status (0=off, 1=active)
hy300_keystone_calibrated{axis="horizontal"} 1                # Calibration status

# Movement metrics
hy300_keystone_movements_total{axis="horizontal"} 156         # Total movements
hy300_keystone_movement_duration_seconds{axis="horizontal"} 0.8 # Last movement duration
hy300_keystone_power_usage_watts{axis="horizontal"} 1.2       # Current power usage

# Error and maintenance metrics
hy300_keystone_errors_total{axis="horizontal",type="overcurrent"} 0 # Overcurrent errors
hy300_keystone_errors_total{axis="horizontal",type="stall"} 1        # Motor stall errors
hy300_keystone_maintenance_cycles{axis="horizontal"} 1500           # Maintenance cycle count
```

**Implementation**: Extended sysfs interface with metrics
```c
// In hy300-keystone-motor.c
static ssize_t metrics_show(struct device *dev, struct device_attribute *attr, char *buf) {
    struct motor_device *motor = dev_get_drvdata(dev);
    
    return sprintf(buf, 
        "movements_total %lu\n"
        "movement_duration_ms %u\n" 
        "power_usage_mw %u\n"
        "error_count %u\n",
        motor->movement_count,
        motor->last_movement_duration,
        motor->power_usage,
        motor->error_count);
}
static DEVICE_ATTR_RO(metrics);
```

#### MIPS Co-processor Communication Metrics
**Source**: `drivers/misc/sunxi-cpu-comm.c`
**Metrics Collected**:
```
# Communication statistics
hy300_mips_messages_sent_total{type="display_command"} 2345    # Messages sent to MIPS
hy300_mips_messages_received_total{type="status_response"} 2340 # Messages received
hy300_mips_message_latency_seconds{type="display_command"} 0.003 # Message latency

# Protocol health
hy300_mips_protocol_errors_total{type="timeout"} 12           # Protocol errors
hy300_mips_protocol_errors_total{type="checksum"} 3           # Checksum errors
hy300_mips_connection_status 1                                # Connection status (0/1)

# Buffer and queue metrics
hy300_mips_tx_queue_depth 5                                   # Transmit queue depth
hy300_mips_rx_queue_depth 2                                   # Receive queue depth
hy300_mips_buffer_utilization 0.15                            # Buffer utilization (0-1)
```

### System Service Metrics

#### HY300 Service Framework
**Services Monitored**: keystone, wifi-manager, remote-input, hdmi-input
**Common Metrics Pattern**:
```
# Service health
hy300_service_status{service="keystone"} 1                    # Service status (0/1)
hy300_service_uptime_seconds{service="keystone"} 3600         # Service uptime
hy300_service_restarts_total{service="keystone"} 2            # Service restart count

# Performance metrics
hy300_service_cpu_usage{service="keystone"} 0.05              # CPU usage (0-1)
hy300_service_memory_bytes{service="keystone"} 52428800       # Memory usage
hy300_service_api_requests_total{service="keystone",method="POST"} 1500 # API requests

# Error metrics
hy300_service_errors_total{service="keystone",type="hardware"} 5 # Service errors
hy300_service_response_time_seconds{service="keystone"} 0.05   # API response time
```

#### Kodi Integration Metrics
**Kodi Performance Monitoring**:
```
# Playback metrics
hy300_kodi_playback_active 1                                  # Playback status
hy300_kodi_video_fps{source="hdmi1"} 59.94                   # Video framerate
hy300_kodi_audio_dropouts_total{source="hdmi1"} 3            # Audio dropouts

# Hardware integration
hy300_kodi_keystone_adjustments_total 45                      # Keystone adjustments
hy300_kodi_input_switches_total{from="internal",to="hdmi1"} 12 # Input switches
hy300_kodi_remote_commands_total{command="play"} 234          # Remote commands

# Resource usage
hy300_kodi_gpu_usage 0.25                                     # GPU utilization
hy300_kodi_decode_errors_total{codec="h264"} 2                # Decode errors
```

## Metrics Collection Implementation

### Prometheus Exporter Service
**Service**: `hy300-metrics-exporter`
**Language**: Python with prometheus_client library
**Port**: 9100 (node_exporter compatible)
**Update Interval**: 1 second for critical metrics, 5 seconds for system metrics

#### Core Exporter Structure
```python
# /nixos/packages/hy300-metrics-exporter/exporter.py
from prometheus_client import start_http_server, Gauge, Counter, Histogram
import time
import threading

class HY300MetricsCollector:
    def __init__(self):
        # HDMI Input metrics
        self.hdmi_connected = Gauge('hy300_hdmi_input_connected', 
                                   'HDMI input connection status', ['port'])
        self.hdmi_frames = Counter('hy300_hdmi_frames_captured_total',
                                  'Total frames captured', ['port'])
        
        # Keystone metrics  
        self.keystone_position = Gauge('hy300_keystone_position',
                                      'Keystone position', ['axis'])
        self.keystone_movements = Counter('hy300_keystone_movements_total',
                                         'Total keystone movements', ['axis'])
        
        # Service metrics
        self.service_status = Gauge('hy300_service_status',
                                   'Service status', ['service'])
        self.service_uptime = Gauge('hy300_service_uptime_seconds',
                                   'Service uptime', ['service'])

    def collect_hdmi_metrics(self):
        """Collect HDMI input metrics from procfs"""
        try:
            with open('/proc/hy300/hdmi/metrics', 'r') as f:
                for line in f:
                    key, value = line.strip().split()
                    if key == 'frames_captured':
                        self.hdmi_frames.labels(port='hdmi1')._value._value = int(value)
                    elif key == 'connected':
                        self.hdmi_connected.labels(port='hdmi1').set(int(value))
        except FileNotFoundError:
            # Hardware not available (VM mode)
            self.hdmi_connected.labels(port='hdmi1').set(0)

    def collect_keystone_metrics(self):
        """Collect keystone metrics from sysfs"""
        try:
            with open('/sys/class/hy300/keystone/h_position', 'r') as f:
                h_pos = int(f.read().strip())
                self.keystone_position.labels(axis='horizontal').set(h_pos)
                
            with open('/sys/class/hy300/keystone/metrics', 'r') as f:
                for line in f:
                    key, value = line.strip().split()
                    if key == 'movements_total':
                        self.keystone_movements.labels(axis='horizontal')._value._value = int(value)
        except FileNotFoundError:
            # Hardware not available (VM mode) - use simulation values
            pass

    def collect_service_metrics(self):
        """Collect service health metrics"""
        services = ['hy300-keystone', 'hy300-wifi-manager', 'hy300-remote-input']
        for service in services:
            try:
                # Check systemd service status
                result = subprocess.run(['systemctl', 'is-active', service], 
                                      capture_output=True, text=True)
                status = 1 if result.stdout.strip() == 'active' else 0
                self.service_status.labels(service=service).set(status)
                
                # Get service uptime
                if status:
                    uptime_result = subprocess.run(['systemctl', 'show', service, 
                                                  '--property=ActiveEnterTimestamp'], 
                                                 capture_output=True, text=True)
                    # Parse timestamp and calculate uptime
                    # Implementation details...
                    
            except Exception as e:
                self.service_status.labels(service=service).set(0)

    def start_collection(self):
        """Start metrics collection loop"""
        def collection_loop():
            while True:
                self.collect_hdmi_metrics()
                self.collect_keystone_metrics() 
                self.collect_service_metrics()
                time.sleep(1)
        
        thread = threading.Thread(target=collection_loop, daemon=True)
        thread.start()

if __name__ == '__main__':
    collector = HY300MetricsCollector()
    collector.start_collection()
    start_http_server(9100)
    
    # Keep the exporter running
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Shutting down metrics exporter")
```

### Grafana Dashboard Configuration
**Dashboard**: HY300 Projector Monitoring
**Panels**: Hardware status, performance metrics, error tracking
**Alerts**: Service failures, hardware errors, performance degradation

#### Key Dashboard Panels
1. **System Overview**: Service status, uptime, basic health
2. **HDMI Input**: Connection status, signal quality, performance  
3. **Keystone System**: Motor position, movement tracking, errors
4. **MIPS Communication**: Message rates, latency, protocol health
5. **Kodi Performance**: Playback status, resource usage, errors
6. **Hardware Health**: Temperature, power usage, error rates

## VM Testing Integration

### Simulation Mode Metrics
**Purpose**: Validate metrics collection without hardware
**Implementation**: Mock interfaces that provide realistic metric values
**Testing**: Ensure Prometheus scraping and Grafana display work correctly

#### Mock Hardware Interfaces
```bash
# Create mock procfs entries for VM testing
mkdir -p /proc/hy300/hdmi
echo "frames_captured 1234567" > /proc/hy300/hdmi/metrics
echo "connected 1" >> /proc/hy300/hdmi/metrics
echo "signal_quality 95" >> /proc/hy300/hdmi/metrics

# Create mock sysfs entries
mkdir -p /sys/class/hy300/keystone
echo "50" > /sys/class/hy300/keystone/h_position
echo "30" > /sys/class/hy300/keystone/v_position
echo "movements_total 156" > /sys/class/hy300/keystone/metrics
```

### Performance Validation
**Metrics Collection Overhead**: Under 1% CPU usage
**Memory Usage**: Under 50MB for complete metrics exporter
**Network Traffic**: Under 10KB/second for metrics scraping
**Storage**: Metrics retention configurable (default 30 days)

## Deployment and Configuration

### NixOS Integration
**Service Configuration**: Systemd service with proper dependencies
**Firewall**: Prometheus port (9100) accessible for monitoring
**Persistence**: Metrics configuration survives system updates
**Security**: Metrics access restricted to monitoring network

### Production Deployment
**High Availability**: Service restart on failure
**Log Management**: Structured logging for troubleshooting
**Performance Monitoring**: Self-monitoring of metrics collector
**Update Procedures**: Rolling updates without metrics loss

This comprehensive metrics framework enables complete monitoring of the HY300 projector system, providing visibility into hardware performance, service health, and user experience metrics.