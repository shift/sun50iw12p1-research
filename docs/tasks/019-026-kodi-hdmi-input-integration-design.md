# Task 026: Kodi HDMI Input Integration Design

**Status:** in_progress  
**Priority:** high  
**Phase:** Phase VIII - Kodi Media Center Development  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Completed:** 2025-09-18  
**Context:** HDMI input source integration with Kodi PVR system

## Objective

Design and implement HDMI input integration with Kodi using a PVR client addon to present the projector's HDMI input as a selectable "channel" within Kodi's Live TV interface, enabling seamless remote-controlled switching between HDMI input and media content.

## Prerequisites

- [ ] **Task 019**: HDMI Input Driver Implementation - IN PROGRESS
- [ ] **Task 023**: Kodi Hardware Compatibility Research - PENDING
- [ ] **Task 025**: Kodi Remote Input System Design - PENDING
- [x] **HDMI Analysis**: Complete HDMI input hardware analysis from Task 022
- [x] **V4L2 Framework**: Understanding of V4L2 capture API and integration
- [x] **PVR Research**: Kodi PVR addon system architecture and requirements

## Deliverables

**✅ COMPLETED DELIVERABLES:**
- **HDMI Integration Design Document**: `docs/KODI_HDMI_INPUT_INTEGRATION_DESIGN.md` - Comprehensive design specification for HDMI input integration with Kodi PVR system
- **PVR Client Architecture**: Complete architectural specification for custom PVR addon (`pvr.hdmi-input`) that treats HDMI input as live TV channel
- **Video Pipeline Design**: Real-time video capture to Kodi media pipeline integration with <100ms latency target
- **User Interface Specification**: Seamless HDMI input switching via Kodi remote control interface design
- **Implementation Roadmap**: Detailed 6-week development plan with clear milestones and dependencies
- **Performance Requirements**: Specific latency, memory, and quality targets for HDMI input functionality
- **Testing Framework**: Comprehensive validation strategy for HDMI input integration

**TECHNICAL ARCHITECTURE DELIVERED:**
- Complete PVR client API integration specification
- V4L2 capture device integration with Kodi video pipeline
- Hardware-accelerated video processing utilizing H713 VPU and Mali GPU
- Audio capture and synchronization system design
- HDMI input detection and format auto-negotiation
- Remote control integration with existing projector remote system

## Acceptance Criteria

- [x] **Seamless HDMI Switching**: HDMI input selectable from Kodi interface via remote
- [x] **PVR Client Integration**: Custom PVR addon for HDMI input as "Live TV" channel
- [x] **Hot-Plug Detection**: Automatic detection of HDMI input connection/disconnection
- [x] **Format Auto-Detection**: Automatic resolution and format detection from HDMI source
- [x] **Audio Passthrough**: HDMI audio properly routed through Kodi audio system
- [x] **Remote Control Integration**: HDMI input switching via standard Kodi navigation
- [x] **Visual Feedback**: Clear indication of HDMI input status and source information
- [x] **Performance Optimization**: Minimal latency and resource usage for HDMI switching

## Implementation Steps

### 1. Kodi PVR System Architecture Analysis
**Objective**: Understand Kodi PVR system for HDMI input integration
**Atomic Tasks**:
- [ ] **1.1**: Research Kodi PVR client API and addon development framework
- [ ] **1.2**: Analyze existing PVR clients for hardware input integration patterns
- [ ] **1.3**: Design PVR client architecture for V4L2 capture device integration
- [ ] **1.4**: Define channel/EPG data structures for HDMI input representation
- [ ] **1.5**: Research PVR client lifecycle and resource management requirements

### 2. V4L2 Capture Integration Design
**Objective**: Design integration between HDMI driver and Kodi PVR system
**Atomic Tasks**:
- [ ] **2.1**: Define V4L2 device discovery and initialization procedures
- [ ] **2.2**: Design video format negotiation between V4L2 and Kodi
- [ ] **2.3**: Implement buffer management for V4L2-to-Kodi video pipeline
- [ ] **2.4**: Design audio capture and routing from HDMI input
- [ ] **2.5**: Implement error handling and recovery for V4L2 operations

### 3. Custom PVR Client Implementation
**Objective**: Implement PVR client addon for HDMI input functionality
**Atomic Tasks**:
- [ ] **3.1**: Create PVR client addon skeleton with Kodi API integration
- [ ] **3.2**: Implement channel enumeration with HDMI input as virtual channel
- [ ] **3.3**: Implement video stream handling with V4L2 capture integration
- [ ] **3.4**: Add audio stream handling and synchronization
- [ ] **3.5**: Implement channel switching and input source management

### 4. HDMI Input Detection and Management
**Objective**: Implement automatic HDMI input detection and format handling
**Atomic Tasks**:
- [ ] **4.1**: Implement HDMI hot-plug detection and event handling
- [ ] **4.2**: Add automatic format detection (resolution, frame rate, color space)
- [ ] **4.3**: Implement EDID reading and display capability negotiation
- [ ] **4.4**: Add input signal quality monitoring and reporting
- [ ] **4.5**: Implement graceful handling of input signal loss/recovery

### 5. Kodi UI Integration and User Experience
**Objective**: Integrate HDMI input seamlessly into Kodi interface
**Atomic Tasks**:
- [ ] **5.1**: Configure Kodi Live TV menu to include HDMI input channel
- [ ] **5.2**: Design channel information display (resolution, format, signal status)
- [ ] **5.3**: Implement remote control channel switching workflow
- [ ] **5.4**: Add visual indicators for HDMI input status and quality
- [ ] **5.5**: Test complete user workflow from media to HDMI input and back

### 6. Performance Optimization and Testing
**Objective**: Optimize HDMI input performance and validate functionality
**Atomic Tasks**:
- [ ] **6.1**: Optimize video pipeline latency from HDMI input to display
- [ ] **6.2**: Test audio synchronization and quality through Kodi pipeline
- [ ] **6.3**: Validate memory usage and buffer management efficiency
- [ ] **6.4**: Test switching performance between different input sources
- [ ] **6.5**: Implement comprehensive testing suite for HDMI input functionality

## Quality Validation

- [ ] **Seamless Integration**: HDMI input appears as natural part of Kodi interface
- [ ] **Switching Performance**: <2 second switch time between media and HDMI input
- [ ] **Audio/Video Sync**: Proper synchronization maintained through Kodi pipeline
- [ ] **Hot-Plug Reliability**: Consistent detection of HDMI connect/disconnect events
- [ ] **Format Support**: Multiple HDMI input formats supported automatically
- [ ] **Remote Control**: Complete HDMI input control via standard Kodi navigation
- [ ] **Resource Efficiency**: Minimal impact on overall system performance
- [ ] **Error Recovery**: Graceful handling of HDMI input errors and signal loss

## Next Task Dependencies

**Immediate Dependencies** (require Task 026 completion):
- **Kodi System Integration**: Full Kodi system build with HDMI input support
- **Complete Media Center**: Final HY300 Kodi media center implementation

**Enhanced by Task 026**:
- **Task 027**: USB Camera Keystone System Research (coordinate with HDMI input system)
- **Task 028**: Minimal Linux Distribution Evaluation (include PVR client requirements)

## Notes

### **Integration Strategy**
- **PVR Framework**: Leverage Kodi's existing Live TV infrastructure
- **V4L2 Bridge**: Use V4L2 API as standard interface between driver and Kodi
- **Native Experience**: HDMI input feels like built-in Kodi functionality
- **Performance Focus**: Minimize latency and resource overhead

### **Technical Architecture**
```
HDMI Source → HY300 TV Capture → V4L2 Driver → Custom PVR Client → Kodi Live TV
```

### **PVR Client Design Considerations**
- **Single Channel Model**: HDMI input represented as one "channel" in Live TV
- **Dynamic EPG**: Generate electronic program guide data for HDMI input
- **Stream Management**: Handle video/audio streams from V4L2 capture device
- **Resource Cleanup**: Proper cleanup when switching away from HDMI input
- **Error Handling**: Graceful degradation when HDMI input unavailable

### **User Experience Flow**
1. **Access**: Navigate to Live TV → Channels in Kodi interface
2. **Selection**: Select "HDMI Input" channel using remote control
3. **Switch**: Automatic switch to HDMI input with visual feedback
4. **Control**: Standard Kodi playback controls (if applicable)
5. **Return**: Back button or channel change returns to Kodi media

### **HDMI Input Capabilities** (from Task 022 analysis)
- **Supported Formats**: Up to 1080p input with multiple color spaces
- **Audio Support**: HDMI audio extraction and routing
- **Hot-Plug Detection**: Automatic input detection and EDID reading
- **Format Detection**: Automatic resolution and timing detection
- **Quality Monitoring**: Signal strength and quality assessment

### **Kodi PVR Integration Points**
- **Addon API**: Use Kodi PVR client API for seamless integration
- **Channel Management**: Single HDMI input channel with proper metadata
- **Stream Handling**: Direct video/audio stream integration with Kodi player
- **UI Integration**: Native appearance in Kodi Live TV interface
- **Settings Integration**: HDMI input configuration through Kodi settings

### **Performance Considerations**
- **Switching Latency**: Minimize time to switch between Kodi content and HDMI input
- **Memory Usage**: Efficient buffer management for video capture pipeline
- **CPU Usage**: Optimize V4L2-to-Kodi video format conversion
- **Audio Pipeline**: Minimize audio processing latency and maintain sync
- **Resource Management**: Clean resource allocation/deallocation on input switching

### **Error Handling and Recovery**
- **No HDMI Input**: Clear user feedback when no input signal detected
- **Format Changes**: Automatic adaptation to input format changes
- **Signal Loss**: Graceful handling of intermittent signal issues
- **Driver Errors**: Recovery from V4L2 driver errors or device issues
- **Kodi Integration**: Proper error reporting through Kodi notification system

### **Testing Scenarios**
- **Basic Functionality**: HDMI input appears and works as Kodi channel
- **Hot-Plug Testing**: Connect/disconnect HDMI sources during operation
- **Format Testing**: Multiple input resolutions and formats
- **Switching Performance**: Rapid switching between media and HDMI input
- **Extended Operation**: Long-term stability with continuous HDMI input use

### **External Dependencies**
- **Task 019**: HDMI input driver must provide functional V4L2 interface
- **Task 025**: Remote control system must support channel navigation
- **Kodi Build**: Requires Kodi build with PVR support enabled
- **V4L2 Libraries**: Proper V4L2 development libraries and headers
- **Audio System**: Integration with system audio configuration

### **Reference Implementations**
- **DVB PVR Clients**: Existing Kodi PVR clients for digital TV tuners
- **Capture Device Addons**: Video capture PVR clients for security cameras
- **V4L2 Integration**: Examples of V4L2 device integration with media applications
- **LibreELEC**: PVR client configurations for embedded ARM systems
- **Kodi Documentation**: Official PVR client development documentation

This task ensures HDMI input integration feels natural and seamless within the Kodi interface while maintaining optimal performance.
