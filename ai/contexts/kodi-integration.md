# Kodi Integration Standards and Procedures

## Kodi for HY300 Projector

### Project Integration Goals
**Purpose**: Complete media center solution optimized for HY300 projector hardware
**Focus**: HDMI input integration, keystone correction, remote input, and hardware acceleration

### Kodi Configuration Architecture

#### Base Configuration
**Installation**: NixOS module with embedded packages
**User**: Dedicated `kodi` system user with hardware access
**Configuration Path**: `/home/kodi/.kodi/userdata/`
**Hardware Access**: V4L2 devices, input devices, system controls

#### HY300-Specific Components
**Keymap Configuration**: `hy300-keymap.xml`
- Custom IR remote button mappings
- Keystone adjustment controls (arrow keys)
- HDMI input switching commands
- Projector-specific shortcuts

**Addons and Plugins**:
- **plugin.video.hy300-hdmi-input**: HDMI input source management
- **script.module.hy300-keystone**: Keystone correction interface
- **plugin.program.hy300-system**: System control and monitoring

#### Graphics and Display
**Display Configuration**: 1920x1080 projector output
**GPU Acceleration**: Mali-G57 hardware acceleration when available
**Video Decoding**: AV1 hardware decoder integration
**Keystone Correction**: Real-time geometry correction

### HDMI Input Integration Design

#### V4L2 Integration
**Device Path**: `/dev/video0` (HDMI input capture)
**Format Support**: 1080p60, 1080p30, 720p60, 720p30
**Buffer Management**: Hardware-accelerated capture buffers
**Format Detection**: Automatic input format detection

#### Kodi PVR Integration
**Backend**: Custom PVR addon for HDMI input
**Channel Mapping**: HDMI inputs as "channels"
- Channel 1: HDMI Input 1
- Channel 2: HDMI Input 2 (if available)
- Channel 3: Internal Android system (future)

**Live TV Interface**: 
- HDMI inputs appear as live TV channels
- Channel switching = HDMI input switching
- No recording functionality (live input only)

#### Implementation Approach
**Plugin Architecture**: `plugin.video.hy300-hdmi-input`
```python
# Addon structure
addon.xml              # Kodi addon manifest
resources/
  lib/
    hdmi_input.py      # Core HDMI input management
    v4l2_interface.py  # V4L2 device interaction
    display_manager.py # Format detection and switching
  settings.xml         # User configuration options
```

**Integration Points**:
1. **V4L2 Device Management**: Direct interaction with `/dev/video0`
2. **Kodi Player Integration**: Feed HDMI stream to Kodi's video player
3. **OSD Integration**: Display input status and format information
4. **Remote Control**: HDMI input switching via IR remote
5. **Keystone Integration**: Real-time correction during HDMI playback

### Keystone Correction Integration

#### Sysfs Interface Integration
**Kernel Interface**: `/sys/class/hy300/keystone/`
**Parameters**: h_keystone, v_keystone, rotation, zoom
**Real-time Updates**: Parameter changes during video playback
**Bounds Checking**: Validate parameter ranges in userspace

#### Kodi Integration Methods
**Script Module**: `script.module.hy300-keystone`
- Python module for keystone control
- Integration with Kodi's settings system
- Real-time parameter adjustment during playback

**User Interface Options**:
1. **Settings Menu**: Persistent keystone configuration
2. **OSD Controls**: Real-time adjustment during playback
3. **Remote Control**: Arrow keys for quick adjustments
4. **Auto-correction**: Accelerometer-based automatic adjustment (future)

#### Implementation Design
**Configuration Storage**: 
- Kodi settings database for persistence
- Kernel sysfs for real-time hardware control
- Backup configuration files for system recovery

**Update Mechanisms**:
```python
# Real-time keystone updates
def update_keystone(h_keystone, v_keystone):
    with open('/sys/class/hy300/keystone/h_keystone', 'w') as f:
        f.write(str(h_keystone))
    with open('/sys/class/hy300/keystone/v_keystone', 'w') as f:
        f.write(str(v_keystone))
    
    # Update Kodi settings for persistence
    kodi.settings.setSetting('hy300.keystone.h', str(h_keystone))
    kodi.settings.setSetting('hy300.keystone.v', str(v_keystone))
```

### Remote Input System Design

#### IR Remote Integration
**LIRC Configuration**: `/etc/lirc/hy300-remote.conf`
**Input Device**: `/dev/input/by-id/hy300-remote`
**Protocol Support**: Multiple IR protocols (NEC, Samsung, etc.)
**Key Mapping**: Custom keymap for projector-specific functions

#### Kodi Keymap Configuration
**Keymap File**: `hy300-keymap.xml`
```xml
<keymap>
  <global>
    <!-- Keystone adjustment -->
    <key id="61448">RunScript(script.module.hy300-keystone,up)</key>
    <key id="61449">RunScript(script.module.hy300-keystone,down)</key>
    
    <!-- HDMI input switching -->
    <key id="61450">RunAddon(plugin.video.hy300-hdmi-input,switch)</key>
    
    <!-- System controls -->
    <key id="61451">System.Shutdown</key>
  </global>
</keymap>
```

#### Input Processing Pipeline
1. **IR Signal Reception**: Hardware IR receiver
2. **LIRC Processing**: Convert IR signals to key codes
3. **Input Device**: Linux input subsystem processing
4. **Kodi Integration**: Custom keymap handling
5. **Action Execution**: Call appropriate addon or system function

### System Integration Standards

#### Service Coordination
**Startup Sequence**:
1. Hardware services (keystone, HDMI input drivers)
2. System services (LIRC, network management)
3. Kodi with HY300 configuration
4. Monitoring and metrics collection

**Dependency Management**:
- Kodi waits for hardware services
- Addons wait for Kodi API availability
- Graceful degradation if hardware unavailable

#### Configuration Management
**Centralized Configuration**: NixOS module system
**User Settings**: Kodi settings database
**Hardware Configuration**: Device tree and kernel modules
**Runtime Configuration**: Sysfs interfaces for real-time control

#### Error Handling and Recovery
**Service Failures**:
- Individual service restart capabilities
- Fallback to software-only mode if hardware fails
- User notification of hardware unavailability
- Automatic service recovery attempts

**Hardware Errors**:
- HDMI input disconnection handling
- Keystone motor error recovery
- Remote input device failures
- GPU acceleration fallback to software

### Testing and Validation

#### VM Testing (Phase VIII)
**Simulation Environment**: Mock hardware interfaces
**Functional Testing**: All addons load and respond correctly
**Integration Testing**: Service coordination and API communication
**Performance Testing**: Resource usage and response times

#### Hardware Testing (Future)
**HDMI Input Testing**: Real video sources and format detection
**Keystone Testing**: Physical motor movement and correction
**Remote Testing**: Actual IR remote functionality
**Integration Testing**: Complete end-to-end projector operation

### Performance Requirements

#### Resource Usage
**Memory**: Kodi + addons under 256MB
**CPU**: Smooth 1080p playback with keystone correction
**Storage**: Complete configuration under 100MB
**Network**: Streaming performance not degraded by system services

#### Response Times
**HDMI Input Switch**: Under 500ms source change
**Keystone Adjustment**: Under 100ms parameter update
**Remote Input**: Under 50ms from IR signal to action
**Menu Navigation**: Fluid UI response (60fps target)

This Kodi integration framework provides a complete media center solution optimized for the HY300 projector's unique hardware capabilities while maintaining compatibility with standard Kodi functionality.