# Task 025: Kodi Remote Input System Design

**Status:** blocked  
**Priority:** high  
**Phase:** Phase VIII - Kodi Media Center Development  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Context:** Remote control integration design for 10-foot UI experience

## Objective

Design and implement a complete remote control input system for Kodi on the HY300 projector, ensuring 100% remote-only navigation capability with proper key mapping, on-screen keyboards, and context-sensitive controls as specified in the Kodi requirements.

## Prerequisites

- [x] **Task 008**: Phase IV - Mainline Device Tree Creation - COMPLETED
- [ ] **Task 023**: Kodi Hardware Compatibility Research - PENDING
- [x] **IR Receiver Analysis**: IR hardware component identification and drivers
- [x] **HY300 Remote Analysis**: Factory remote control button mapping and protocols
- [x] **Input System Research**: Linux input subsystem and IR-to-input event pipeline

## Acceptance Criteria

- [ ] **Complete Remote Navigation**: 100% Kodi functionality accessible via D-pad remote
- [ ] **Key Mapping Configuration**: Custom keymap for HY300 remote control
- [ ] **On-Screen Keyboard**: Remote-friendly text input system
- [ ] **Context Menu Integration**: Menu button and long-press context actions
- [ ] **Volume Control Integration**: Hardware volume control with OSD feedback
- [ ] **Power Management**: Remote power controls integrated with system power management
- [ ] **Visual Focus System**: Clear visual indicators for current selection
- [ ] **Input Response Testing**: <500ms response time validation

## Implementation Steps

### 1. IR Hardware Integration Analysis
**Objective**: Understand and document IR receiver hardware and driver requirements
**Atomic Tasks**:
- [ ] **1.1**: Analyze HY300 IR receiver hardware from device tree and factory analysis
- [ ] **1.2**: Research IR receiver driver options (rc-core, lirc, custom)
- [ ] **1.3**: Document factory remote control protocol and button codes
- [ ] **1.4**: Test IR receiver functionality and signal decoding
- [ ] **1.5**: Establish IR-to-Linux input event pipeline

### 2. Linux Input System Configuration
**Objective**: Configure Linux input subsystem for remote control integration
**Atomic Tasks**:
- [ ] **2.1**: Configure kernel input subsystem with IR receiver support
- [ ] **2.2**: Set up input event device mapping (/dev/input/eventX)
- [ ] **2.3**: Configure udev rules for consistent device naming
- [ ] **2.4**: Test input event generation from remote control presses
- [ ] **2.5**: Implement input event debugging and monitoring tools

### 3. Kodi Keymap Design and Implementation
**Objective**: Create comprehensive Kodi keymap for HY300 remote control
**Atomic Tasks**:
- [ ] **3.1**: Design key mapping strategy based on Kodi requirements document
- [ ] **3.2**: Create custom Kodi keymap XML configuration
- [ ] **3.3**: Implement D-pad navigation for all Kodi interface elements
- [ ] **3.4**: Configure context menu activation (Menu button/long-press)
- [ ] **3.5**: Test navigation completeness across all Kodi screens

### 4. On-Screen Keyboard Implementation
**Objective**: Implement remote-friendly text input system
**Atomic Tasks**:
- [ ] **4.1**: Research Kodi on-screen keyboard options and configurations
- [ ] **4.2**: Design optimal keyboard layout for D-pad navigation
- [ ] **4.3**: Configure automatic keyboard activation for text input fields
- [ ] **4.4**: Implement efficient character selection with D-pad
- [ ] **4.5**: Test keyboard functionality across search, settings, and password fields

### 5. Media and System Control Integration
**Objective**: Integrate playback and system controls with remote buttons
**Atomic Tasks**:
- [ ] **5.1**: Map media control buttons (Play/Pause, Stop, FFWD, RWD)
- [ ] **5.2**: Implement volume control with OSD feedback integration
- [ ] **5.3**: Configure Home and Back button navigation logic
- [ ] **5.4**: Implement power button long-press menu (Power Off, Reboot, Quit)
- [ ] **5.5**: Test all media control functions during video playback

### 6. Visual Focus and Accessibility Enhancement
**Objective**: Ensure clear visual feedback for remote navigation
**Atomic Tasks**:
- [ ] **6.1**: Configure Kodi skin for high-contrast focus indicators
- [ ] **6.2**: Test focus visibility across different Kodi interface elements
- [ ] **6.3**: Optimize focus indicator size and style for 10-foot viewing
- [ ] **6.4**: Implement focus wrap-around and navigation logic optimization
- [ ] **6.5**: Test accessibility with different Kodi skins and themes

## Quality Validation

- [ ] **Navigation Completeness**: Every Kodi function accessible via remote only
- [ ] **Response Time**: <500ms input response across all interface elements
- [ ] **Focus Visibility**: Clear visual focus indicators on all selectable elements
- [ ] **Text Input Functionality**: On-screen keyboard works for all text fields
- [ ] **Media Control Integration**: All playback controls function correctly
- [ ] **System Integration**: Volume, power, and navigation controls work reliably
- [ ] **Extended Testing**: 24+ hour usage without input system failures
- [ ] **User Experience**: Smooth, intuitive remote-only operation

## Next Task Dependencies

**Immediate Dependencies** (require Task 025 completion):
- **Task 026**: Kodi HDMI Input Integration Design (benefits from input system framework)
- **Kodi Implementation**: Full Kodi system build and configuration

**Enhanced by Task 025**:
- **Task 027**: USB Camera Keystone System Research (requires remote control integration)
- **Task 028**: Minimal Linux Distribution Evaluation (requires input system requirements)

## Notes

### **Design Philosophy**
- **Remote-First Design**: Every feature accessible without mouse/keyboard
- **10-Foot UI Optimization**: Interface elements sized for living room viewing
- **Intuitive Navigation**: Logical, predictable navigation patterns
- **Accessibility Focus**: Clear visual feedback and consistent interaction model

### **HY300 Remote Control Analysis**
Based on factory analysis, the HY300 remote includes:
- **D-Pad**: Up, Down, Left, Right, OK/Select
- **Navigation**: Home, Back, Menu
- **Media Controls**: Play/Pause, Stop, Fast Forward, Rewind
- **System Controls**: Power, Volume Up/Down
- **Additional**: Number keys (0-9), possibly mute and input selection

### **Kodi Keymap Strategy**
Following Kodi requirements document:
1. **D-Pad Navigation**: Primary interface navigation method
2. **OK/Select**: Primary action button for all selections
3. **Back Button**: Logical hierarchy navigation, no effect from home screen
4. **Home Button**: Always returns to Kodi main menu
5. **Menu Button**: Context-sensitive menus for current item
6. **Long-Press OK**: Alternative context menu activation

### **Critical Integration Points**
- **IR Receiver Hardware**: Device tree configuration and driver integration
- **Input Event Pipeline**: IR → Linux input events → Kodi input processing
- **Volume Integration**: System volume control with Kodi OSD integration
- **Power Management**: Integration with system shutdown/reboot procedures
- **Focus Management**: Kodi skin configuration for optimal focus visibility

### **Technical Implementation Approaches**
1. **IR Driver**: Use rc-core framework for robust IR signal decoding
2. **Input Events**: Standard Linux input subsystem for clean integration
3. **Kodi Configuration**: Custom keymap XML for HY300-specific layout
4. **OSD Integration**: Kodi's built-in volume OSD and notification system
5. **Power Integration**: systemd integration for clean power management

### **Testing Methodology**
- **Functionality Testing**: Complete navigation testing across all Kodi screens
- **Performance Testing**: Input response time measurement and optimization
- **Usability Testing**: Real-world usage scenarios with remote-only operation
- **Reliability Testing**: Extended operation testing for input system stability
- **Edge Case Testing**: Error conditions, rapid input, simultaneous presses

### **External Resources**
- **Kodi Input Documentation**: Official Kodi keymap and input configuration guides
- **rc-core Documentation**: Linux IR subsystem documentation and examples
- **LibreELEC Configurations**: Production remote control configurations for ARM devices
- **IR Protocol Databases**: Common remote control protocols and code tables
- **Accessibility Guidelines**: Best practices for 10-foot UI design

### **Performance Requirements**
- **Input Latency**: <100ms from button press to visual response
- **Navigation Speed**: Smooth scrolling through large media libraries
- **Memory Usage**: Minimal impact on overall system memory usage
- **CPU Usage**: Input processing should not impact video playback performance
- **Reliability**: 99.9% successful button press recognition

### **Integration with Other Components**
- **Graphics System**: Coordinate with Task 024 for focus rendering performance
- **HDMI Input**: Integrate with Task 026 for seamless input source switching
- **Audio System**: Volume control integration with audio output management
- **Power Management**: Clean integration with system power state management
- **Boot System**: Ensure input system available immediately after Kodi launch

### **Fallback and Error Handling**
- **IR Receiver Failure**: Graceful degradation and error reporting
- **Input Event Corruption**: Robust event processing and filtering
- **Focus Loss**: Automatic focus recovery and visual feedback
- **Keymap Loading**: Fallback to default keymap if custom configuration fails
- **System Integration**: Fallback controls if Kodi input system fails

This task ensures the HY300 projector provides an optimal 10-foot UI experience with complete remote control functionality.

**Blocked Reason:** Waiting for Task 019 completion
