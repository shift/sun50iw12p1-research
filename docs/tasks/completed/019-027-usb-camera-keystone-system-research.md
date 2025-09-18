# Task 027: USB Camera Keystone System Research

**Status:** completed  
**Priority:** medium  
**Phase:** Phase VIII - Kodi Media Center Development  
**Assigned:** AI Agent  
**Created:** 2025-09-18  
**Context:** Automatic keystone correction using USB camera and computer vision

## Objective

Research and design an automatic keystone correction system using a USB UVC camera to detect projected image distortion and automatically apply geometric correction, with integration into Kodi's settings interface for remote-controlled operation.

## Prerequisites

- [x] **Task 008**: Phase IV - Mainline Device Tree Creation - COMPLETED
- [ ] **Task 023**: Kodi Hardware Compatibility Research - PENDING
- [ ] **Task 024**: Kodi Graphics Driver Evaluation - PENDING (for display pipeline integration)
- [ ] **Task 025**: Kodi Remote Input System Design - PENDING (for UI integration)
- [x] **USB Hardware Analysis**: USB port capabilities and UVC camera support
- [x] **Display Pipeline**: Understanding of HY300 display output and projection system

## Acceptance Criteria

- [ ] **UVC Camera Detection**: Automatic detection and initialization of USB Video Class cameras
- [ ] **Computer Vision Pipeline**: OpenCV-based image analysis for keystone detection
- [ ] **Calibration Pattern System**: Projection and capture of calibration patterns
- [ ] **Geometric Correction**: Real-time perspective transformation calculation and application
- [ ] **Kodi UI Integration**: Remote-controlled calibration process through Kodi interface
- [ ] **Display Pipeline Integration**: Real-time geometric correction applied to video output
- [ ] **Persistent Configuration**: Keystone correction settings saved across reboots
- [ ] **Fallback Manual Control**: Existing manual keystone controls retained as backup

## Implementation Steps

### 1. USB Camera System Analysis
**Objective**: Research UVC camera support and integration requirements
**Atomic Tasks**:
- [ ] **1.1**: Research UVC (USB Video Class) camera compatibility and drivers
- [ ] **1.2**: Analyze HY300 USB port capabilities and power requirements
- [ ] **1.3**: Test USB camera detection and V4L2 device enumeration
- [ ] **1.4**: Research camera resolution and frame rate requirements for keystone detection
- [ ] **1.5**: Document camera positioning and mounting considerations

### 2. Computer Vision Framework Research
**Objective**: Evaluate computer vision libraries and algorithms for keystone detection
**Atomic Tasks**:
- [ ] **2.1**: Research OpenCV ARM64 compatibility and performance on H713
- [ ] **2.2**: Analyze alternative computer vision libraries (OpenVX, custom implementations)
- [ ] **2.3**: Research keystone detection algorithms (corner detection, edge detection)
- [ ] **2.4**: Evaluate perspective transformation calculation methods
- [ ] **2.5**: Assess computational requirements and optimization strategies

### 3. Calibration Pattern Design and Projection
**Objective**: Design calibration patterns and projection system
**Atomic Tasks**:
- [ ] **3.1**: Design optimal calibration patterns (grid, checkerboard, circles)
- [ ] **3.2**: Research pattern projection methods through display pipeline
- [ ] **3.3**: Analyze pattern visibility and contrast requirements
- [ ] **3.4**: Design pattern capture and image preprocessing procedures
- [ ] **3.5**: Test pattern recognition robustness under different lighting conditions

### 4. Geometric Correction Implementation
**Objective**: Implement perspective transformation and display pipeline integration
**Atomic Tasks**:
- [ ] **4.1**: Research display pipeline modification points (DRM/KMS, compositor)
- [ ] **4.2**: Implement homography matrix calculation from detected corners
- [ ] **4.3**: Design real-time geometric transformation application
- [ ] **4.4**: Integrate transformation with Mali GPU hardware acceleration
- [ ] **4.5**: Optimize transformation performance for minimal display latency

### 5. Kodi Interface Integration
**Objective**: Integrate keystone correction into Kodi settings and remote control
**Atomic Tasks**:
- [ ] **5.1**: Design Kodi settings addon for keystone correction control
- [ ] **5.2**: Implement remote-controlled calibration workflow within Kodi
- [ ] **5.3**: Create user interface for calibration process (instructions, confirmation)
- [ ] **5.4**: Integrate with Kodi's display settings and configuration system
- [ ] **5.5**: Implement status indicators and error reporting within Kodi interface

### 6. System Integration and Persistence
**Objective**: Implement complete system integration with persistent configuration
**Atomic Tasks**:
- [ ] **6.1**: Design configuration file format for keystone correction parameters
- [ ] **6.2**: Implement automatic correction application at system startup
- [ ] **6.3**: Integrate with existing manual keystone correction fallback
- [ ] **6.4**: Implement system service for keystone correction management
- [ ] **6.5**: Test complete workflow from calibration to persistent correction

## Quality Validation

- [ ] **Camera Compatibility**: Multiple UVC cameras detected and function correctly
- [ ] **Detection Accuracy**: >95% success rate for keystone pattern detection
- [ ] **Correction Quality**: Geometric correction produces visually straight rectangular image
- [ ] **Performance**: Real-time correction with <50ms additional display latency
- [ ] **UI Integration**: Complete remote control operation without keyboard/mouse
- [ ] **Persistence**: Correction settings properly saved and restored across reboots
- [ ] **Robustness**: System handles camera disconnection and reconnection gracefully
- [ ] **Manual Fallback**: Existing manual keystone controls remain functional

## Next Task Dependencies

**Immediate Dependencies** (require Task 027 completion):
- **Complete Kodi System**: Integration with final Kodi media center implementation
- **User Documentation**: Keystone correction user guide and troubleshooting

**Enhanced by Task 027**:
- **Task 028**: Minimal Linux Distribution Evaluation (include computer vision requirements)

## Notes

### **Technical Approach Overview**
1. **Detection Phase**: USB camera captures projected calibration pattern
2. **Analysis Phase**: Computer vision detects pattern corners and calculates distortion
3. **Correction Phase**: Perspective transformation applied to display output pipeline
4. **Integration Phase**: Kodi interface provides user-friendly calibration workflow

### **Computer Vision Pipeline**
```
USB Camera → V4L2 Capture → OpenCV Analysis → Corner Detection → 
Homography Calculation → Display Pipeline Transform → Corrected Output
```

### **Calibration Pattern Considerations**
- **High Contrast**: Black and white patterns for reliable detection
- **Corner Detection**: Grid or checkerboard patterns with clear corners
- **Size Optimization**: Large enough for camera detection, small enough for processing
- **Lighting Independence**: Patterns visible under various ambient lighting
- **False Positive Reduction**: Unique patterns to avoid environmental interference

### **Display Pipeline Integration Options**
1. **DRM/KMS Level**: Hardware-level geometric transformation
2. **Compositor Level**: Wayland/X11 compositor transformation
3. **GPU Shader**: Mali GPU shader-based real-time transformation
4. **Application Level**: Kodi-level video output transformation

### **Camera Requirements and Compatibility**
- **UVC Standard**: Standard USB Video Class for broad compatibility
- **Resolution**: Minimum 640x480, preferred 1280x720 for accuracy
- **Frame Rate**: 15fps minimum for responsive calibration
- **Auto-Exposure**: Automatic exposure control for varying projection brightness
- **Focus**: Fixed focus or auto-focus capability for pattern detection

### **User Experience Design**
Following Kodi requirements document:
1. **Initiation**: Settings → System → Display → Run Auto Keystone Calibration
2. **Instruction**: Clear dialog with camera positioning instructions
3. **Calibration**: Automatic pattern projection and capture
4. **Confirmation**: "Is the image correctly aligned? [Save] [Retry] [Cancel]"
5. **Persistence**: Correction settings saved for future boot cycles

### **Performance Considerations**
- **CPU Usage**: Optimize computer vision processing for H713 ARM cores
- **Memory Usage**: Efficient image processing with limited 2GB system RAM
- **GPU Acceleration**: Leverage Mali-G31 for transformation calculations
- **Thermal Management**: Monitor processing load to prevent thermal throttling
- **Real-time Constraints**: Minimal impact on video playback performance

### **Error Handling and Recovery**
- **Camera Not Found**: Clear error message and fallback to manual adjustment
- **Pattern Detection Failure**: Retry mechanism and improved lighting guidance
- **Calculation Errors**: Validation of transformation matrix before application
- **Display Pipeline Errors**: Graceful fallback to uncorrected output
- **Configuration Corruption**: Reset to default settings and manual override

### **Integration with Existing Systems**
- **Manual Keystone**: Preserve existing manual controls as backup
- **Display Settings**: Integration with Kodi's display configuration
- **Remote Control**: Full operation via HY300 remote control
- **Boot Process**: Automatic application of saved correction at startup
- **System Services**: Clean integration with systemd service management

### **External Resources and Libraries**
- **OpenCV**: Primary computer vision library for ARM64 platforms
- **V4L2**: Standard Linux camera interface for UVC devices
- **DRM/KMS**: Direct Rendering Manager for display pipeline access
- **Mesa**: OpenGL/Mali GPU integration for hardware acceleration
- **Kodi Addons**: Kodi settings addon development framework

### **Testing Methodology**
- **Camera Compatibility**: Test multiple UVC camera models and configurations
- **Pattern Detection**: Various lighting conditions and projection surfaces
- **Accuracy Testing**: Measurement of correction accuracy and consistency
- **Performance Testing**: Impact on system performance and video playback
- **User Experience**: Complete workflow testing with remote control only

### **Limitations and Constraints**
- **Camera Requirement**: System requires external USB camera for automatic operation
- **Lighting Dependency**: May require adequate lighting for pattern detection
- **Processing Power**: Computer vision processing limited by H713 CPU capabilities
- **Camera Positioning**: User must position camera to see projected image
- **Projection Surface**: Works best with flat, uniform projection surfaces

### **Future Enhancement Possibilities**
- **Multi-Point Correction**: Advanced correction for curved surfaces
- **Automatic Adjustment**: Periodic automatic recalibration
- **Camera Integration**: Integration with webcam for video calls
- **Advanced Patterns**: 3D calibration patterns for enhanced accuracy
- **Machine Learning**: Pattern detection improvement through training

This task provides an innovative automatic keystone correction system that enhances the HY300 projector's usability while maintaining the remote-only operation philosophy.
