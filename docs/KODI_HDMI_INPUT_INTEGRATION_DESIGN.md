# Kodi HDMI Input Integration Design

**Date:** September 18, 2025  
**Task:** 026 - HDMI Input Integration System Design  
**Phase:** VIII - Kodi Media Center Development  
**Dependencies:** Task 019 - HDMI Input Driver Implementation

## Executive Summary

This document designs a comprehensive HDMI input integration system for Kodi PVR client that treats HDMI input as a seamless "channel" accessible via the projector's remote control. The design leverages the ongoing V4L2 driver development from Task 019 and integrates with Kodi's PVR subsystem to provide real-time HDMI input viewing with minimal latency.

## System Architecture Overview

### High-Level Integration Flow
```
HDMI Input → TV Capture Hardware → sunxi-tvcap Driver → V4L2 Device → PVR Addon → Kodi Player → Display
                                                     ↓
                                                Audio Capture → ALSA → Kodi Audio → Speakers
```

### Core Components
1. **Hardware Layer**: TV capture hardware with MIPS co-processor enhancement
2. **Driver Layer**: sunxi-tvcap V4L2 driver (Task 019 development)
3. **PVR Client Layer**: Custom Kodi PVR addon (`pvr.hdmi-input`)
4. **Integration Layer**: Kodi's PVR subsystem and video player
5. **User Interface Layer**: Channel switching and remote control integration

## 1. HDMI Input Hardware Integration

### Foundation from Task 019
The HDMI input integration builds upon the comprehensive V4L2 driver development:

| Component | Task 019 Implementation | HDMI Integration Usage |
|-----------|------------------------|----------------------|
| **V4L2 Device** | `/dev/video0` capture interface | Primary video source for PVR addon |
| **HDMI Detection** | Hot-plug detection and EDID reading | Automatic input source detection |
| **Video Formats** | 1080p, 720p, multiple color spaces | Dynamic format adaptation |
| **MIPS Integration** | ARM-MIPS communication protocol | Enhanced video processing |
| **Buffer Management** | videobuf2 with DMA integration | Zero-copy video streaming |

### Hardware Capabilities (H713 SoC)
Based on hardware analysis from Task 023:

| Feature | H713 Capability | HDMI Integration Benefit |
|---------|----------------|-------------------------|
| **VPU Hardware Decode** | H.264/HEVC at 4K@60fps | Real-time format processing |
| **Mali GPU** | OpenGL ES 3.2 acceleration | Hardware-accelerated composition |
| **MIPS Co-processor** | 40MB dedicated video memory | Optimized capture pipeline |
| **Memory Bandwidth** | 12.8 GB/s theoretical | Sufficient for live video streams |

### Video Format Support Strategy
```c
// Supported HDMI Input Formats
enum hdmi_input_format {
    HDMI_1080P_60FPS,    // Primary target format
    HDMI_1080P_30FPS,    // High quality format
    HDMI_720P_60FPS,     // Performance fallback
    HDMI_720P_30FPS,     // Minimum quality
    HDMI_AUTO_DETECT     // EDID-based detection
};
```

## 2. Kodi PVR Client Architecture

### PVR Addon Structure (`pvr.hdmi-input`)
The custom PVR addon implements Kodi's PVR client API to present HDMI input as live TV:

```cpp
class CHdmiInputPVRClient : public CPVRClient
{
public:
    // Core PVR API Implementation
    PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio) override;
    PVR_ERROR OpenLiveStream(const PVR_CHANNEL& channel) override;
    void CloseLiveStream() override;
    bool SwitchChannel(const PVR_CHANNEL& channel) override;
    PVR_ERROR GetSignalStatus(int channelUid, PVR_SIGNAL_STATUS& signalStatus) override;
    
    // HDMI-specific functionality
    bool DetectHdmiInput();
    bool ConfigureVideoFormat();
    void UpdateSignalStatus();
    
private:
    V4L2Device m_captureDevice;      // /dev/video0 interface
    HdmiDetector m_hdmiDetector;     // Hot-plug monitoring
    StreamProcessor m_streamProcessor; // Format conversion
};
```

### Channel Abstraction Design
HDMI inputs are presented as virtual TV channels:

| Channel ID | Channel Name | Description | Source |
|------------|-------------|-------------|---------|
| **1** | "HDMI Input" | Primary external device input | `/dev/video0` |
| **2** | "HDMI Input 2" | Secondary input (future expansion) | `/dev/video1` |
| **99** | "No Signal" | Placeholder when no input detected | Static content |

### EPG (Electronic Program Guide) Simulation
```cpp
// EPG Entry Structure for HDMI Input
struct HdmiEpgEntry {
    std::string title = "External Device Input";
    std::string description = "Live HDMI input from connected device";
    time_t start_time = current_time();
    time_t end_time = start_time + (24 * 3600); // 24-hour duration
    std::string genre = "Input Source";
};
```

## 3. Video Processing Pipeline

### Real-Time Video Capture Chain
```
1. HDMI Signal → TV Capture Hardware
2. Hardware → sunxi-tvcap Driver (V4L2)
3. Driver → PVR Addon (V4L2 IOCTL)
4. Addon → Kodi Stream (PVR API)
5. Stream → Kodi Player (Hardware Decode)
6. Player → Mali GPU (Composition)
7. GPU → Display Output
```

### Latency Optimization Strategy
**Target:** <100ms end-to-end latency

| Stage | Latency Budget | Optimization Strategy |
|-------|---------------|----------------------|
| **Capture** | 16ms (1 frame @ 60fps) | Minimal hardware buffering |
| **Driver** | 8ms | Direct memory access, no copying |
| **PVR Processing** | 16ms | Zero-copy buffer handoff |
| **Kodi Decode** | 24ms | Hardware VPU acceleration |
| **GPU Composition** | 16ms | OpenGL ES direct rendering |
| **Display** | 16ms | Hardware display controller |
| **Total** | **96ms** | Within 100ms target |

### Format Handling and Conversion
```cpp
class VideoFormatProcessor {
public:
    struct InputFormat {
        uint32_t width, height;
        uint32_t framerate;
        uint32_t fourcc;           // V4L2 pixel format
        bool interlaced;
    };
    
    // Auto-detect input format from EDID
    InputFormat DetectFormat();
    
    // Convert to Kodi-compatible format
    bool ConvertFormat(const InputFormat& input, KodiStream& output);
    
    // Hardware-accelerated scaling
    bool ScaleVideo(uint32_t src_w, uint32_t src_h, 
                   uint32_t dst_w, uint32_t dst_h);
};
```

### Buffer Management Architecture
```cpp
class HdmiBufferManager {
private:
    static constexpr size_t BUFFER_COUNT = 4;  // Minimal latency
    static constexpr size_t MAX_BUFFER_SIZE = 1920 * 1080 * 4; // RGBA
    
    struct VideoBuffer {
        void* data;
        size_t size;
        uint64_t timestamp;
        bool in_use;
    };
    
    VideoBuffer m_buffers[BUFFER_COUNT];
    std::queue<VideoBuffer*> m_free_buffers;
    std::queue<VideoBuffer*> m_ready_buffers;
    
public:
    VideoBuffer* GetFreeBuffer();
    void QueueBuffer(VideoBuffer* buffer);
    VideoBuffer* DequeueBuffer();
    void ReleaseBuffer(VideoBuffer* buffer);
};
```

## 4. Audio Integration Design

### HDMI Audio Capture and Routing
Based on H713 audio capabilities:

| Audio Feature | Implementation | Integration Point |
|---------------|----------------|------------------|
| **HDMI Audio Extraction** | Hardware audio capture | ALSA driver integration |
| **Multi-channel Support** | Up to 7.1 surround | Kodi audio engine |
| **Format Detection** | PCM/compressed detection | Automatic format selection |
| **Passthrough** | DTS/Dolby passthrough | Hardware audio routing |

### Audio Pipeline Architecture
```cpp
class HdmiAudioProcessor {
public:
    struct AudioConfig {
        uint32_t sample_rate;     // 44.1kHz, 48kHz, 96kHz
        uint32_t channels;        // 2, 6, 8 (stereo, 5.1, 7.1)
        uint32_t bit_depth;       // 16, 24, 32 bit
        bool compressed;          // PCM vs compressed stream
    };
    
    // Audio capture integration
    bool InitializeCapture(const AudioConfig& config);
    bool CaptureAudioFrame(AudioFrame& frame);
    
    // Synchronization with video
    bool SynchronizeWithVideo(uint64_t video_timestamp);
    
    // Integration with Kodi audio
    bool SendToKodiAudio(const AudioFrame& frame);
};
```

### Audio/Video Synchronization Strategy
```cpp
class AVSyncManager {
private:
    int64_t m_video_timestamp;
    int64_t m_audio_timestamp;
    int64_t m_sync_threshold = 40000; // 40ms tolerance
    
public:
    void UpdateVideoTimestamp(int64_t timestamp);
    void UpdateAudioTimestamp(int64_t timestamp);
    
    // Calculate drift and apply correction
    SyncAction CalculateSyncAction();
    void ApplyAudioCorrection(int32_t correction_ms);
};
```

## 5. User Interface Design

### Channel Switching Integration
The HDMI input appears as a standard TV channel in Kodi's interface:

#### Channel List Presentation
```xml
<!-- Kodi Channel List Entry -->
<channel>
    <id>1</id>
    <name>HDMI Input</name>
    <icon>/usr/share/kodi/media/hdmi-input-icon.png</icon>
    <preview>true</preview>
    <signal_quality>auto</signal_quality>
</channel>
```

#### Remote Control Mapping
| Remote Button | Function | Implementation |
|---------------|----------|----------------|
| **CH+/CH-** | Switch to/from HDMI input | PVR channel switching |
| **Input/Source** | Direct HDMI input access | Custom key mapping |
| **OK/Select** | Confirm channel selection | Standard Kodi navigation |
| **Info** | Display signal status | Custom OSD overlay |

### Signal Status Interface
```cpp
class HdmiSignalStatus {
public:
    struct SignalInfo {
        bool connected;
        uint32_t width, height;
        uint32_t refresh_rate;
        std::string device_name;    // From EDID
        uint8_t signal_strength;    // 0-100%
        bool audio_present;
    };
    
    SignalInfo GetCurrentStatus();
    void DisplayStatusOSD();
    void UpdateStatusIndicators();
};
```

### User Experience Features
1. **Automatic Detection**:
   - Monitor HDMI hot-plug events
   - Auto-switch to HDMI input when device connected
   - Return to previous content when disconnected

2. **Visual Feedback**:
   - Signal quality indicators in channel list
   - "No Signal" overlay when input unavailable
   - Resolution and refresh rate display

3. **Seamless Integration**:
   - Instant switching between Kodi content and HDMI input
   - Preserve audio/video settings across switches
   - Background content continues when switching away

## 6. Implementation Roadmap

### Development Phases

#### Phase 1: Core PVR Framework (Weeks 1-2)
**Objectives:**
- Create basic PVR addon structure
- Implement Kodi PVR client API
- Add channel enumeration functionality

**Deliverables:**
- `pvr.hdmi-input` addon skeleton
- Channel discovery and listing
- Basic V4L2 device integration
- Initial testing framework

**Dependencies:**
- Task 019 V4L2 driver basic functionality
- Kodi development environment setup

#### Phase 2: Video Stream Integration (Weeks 2-3)
**Objectives:**
- Implement V4L2 video capture
- Add video format detection
- Create video stream packaging

**Deliverables:**
- Video capture functionality
- Format auto-detection
- Basic video playback integration
- Performance baseline measurements

**Dependencies:**
- Task 019 video capture implementation
- Hardware video format support

#### Phase 3: Audio Integration (Weeks 3-4)
**Objectives:**
- Add HDMI audio capture
- Implement audio/video synchronization
- Integrate with Kodi audio subsystem

**Deliverables:**
- Audio capture implementation
- Synchronization algorithms
- Audio format support
- Audio quality validation

**Dependencies:**
- Task 019 audio capture capabilities
- ALSA driver integration

#### Phase 4: User Interface Integration (Weeks 4-5)
**Objectives:**
- Implement channel switching interface
- Add signal status monitoring
- Create EPG entries

**Deliverables:**
- Complete channel switching
- Signal status displays
- Remote control integration
- User experience testing

**Dependencies:**
- Remote input system integration
- Kodi UI framework understanding

#### Phase 5: Performance Optimization (Weeks 5-6)
**Objectives:**
- Optimize latency to <100ms
- Implement hardware acceleration
- Finalize buffer management

**Deliverables:**
- Latency optimization results
- Hardware acceleration integration
- Memory usage optimization
- Stability validation

**Dependencies:**
- Complete hardware driver integration
- Performance profiling tools

### Critical Dependencies from Task 019

| Task 019 Component | Required for HDMI Integration | Status |
|-------------------|-------------------------------|---------|
| **V4L2 Device Registration** | PVR addon device access | In Progress |
| **HDMI Input Detection** | Channel availability | Planned |
| **Video Format Support** | Format handling | Planned |
| **MIPS Communication** | Enhanced processing | Planned |
| **Buffer Management** | Stream performance | Planned |

## 7. Testing and Validation Strategy

### Functional Testing Requirements
1. **HDMI Input Detection**:
   - Hot-plug event detection
   - EDID reading and parsing
   - Multi-format input support
   - Signal quality monitoring

2. **Video Streaming**:
   - Real-time capture functionality
   - Format conversion accuracy
   - Frame rate stability
   - Image quality validation

3. **Audio Integration**:
   - Audio capture synchronization
   - Multi-channel audio support
   - Format detection accuracy
   - Audio/video sync validation

4. **User Interface**:
   - Channel switching responsiveness
   - Remote control integration
   - Signal status accuracy
   - Error handling robustness

### Performance Testing Criteria
| Metric | Target | Measurement Method |
|--------|--------|--------------------|
| **End-to-End Latency** | <100ms | Video timestamp analysis |
| **CPU Usage** | <10% during playback | System monitoring |
| **Memory Usage** | <200MB additional | Memory profiling |
| **Frame Drops** | <0.1% @ 60fps | Frame analysis |
| **Audio Sync** | ±40ms tolerance | A/V sync measurement |

### Integration Testing Scenarios
1. **Input Source Variety**:
   - Gaming consoles (PS5, Xbox, Switch)
   - Set-top boxes and streaming devices
   - Laptop/computer HDMI output
   - Various video formats and resolutions

2. **User Workflow Testing**:
   - Boot-to-HDMI-input time
   - Channel switching from any Kodi screen
   - Remote control responsiveness
   - Multi-user scenario testing

3. **Error Condition Handling**:
   - Cable disconnection during playback
   - Format changes during streaming
   - Device power cycling
   - Driver reload scenarios

### Validation Acceptance Criteria
- [ ] **Seamless Integration**: HDMI input appears as standard Kodi channel
- [ ] **Single Remote Operation**: All functions accessible via projector remote
- [ ] **Low Latency**: <100ms end-to-end latency confirmed
- [ ] **Audio Passthrough**: HDMI audio properly routed to projector speakers
- [ ] **Format Support**: 1080p and 720p inputs at 30/60fps
- [ ] **Signal Monitoring**: Accurate status reporting and user feedback
- [ ] **Stability**: 24+ hour continuous operation without issues
- [ ] **Resource Efficiency**: Minimal impact on system performance

## 8. Technical Implementation Details

### V4L2 Device Integration
```cpp
class V4L2CaptureDevice {
private:
    int m_fd;
    struct v4l2_buffer m_buffers[BUFFER_COUNT];
    void* m_mapped_buffers[BUFFER_COUNT];
    
public:
    bool Initialize(const std::string& device_path);
    bool SetFormat(uint32_t width, uint32_t height, uint32_t fourcc);
    bool StartCapture();
    bool CaptureFrame(VideoFrame& frame);
    void StopCapture();
    void Cleanup();
    
    // HDMI-specific functionality
    bool DetectInputSignal();
    EdidInfo ReadEdidData();
    bool SetInputSource(uint32_t input_id);
};
```

### Kodi PVR API Integration
```cpp
// Main PVR addon entry points
extern "C" {
    ADDON_STATUS ADDON_Create(void* hdl, void* props);
    void ADDON_Stop();
    void ADDON_Destroy();
    
    // PVR capability functions
    PVR_ERROR GetCapabilities(PVR_ADDON_CAPABILITIES* capabilities);
    PVR_ERROR GetChannels(ADDON_HANDLE handle, bool bRadio);
    PVR_ERROR OpenLiveStream(const PVR_CHANNEL& channel);
    void CloseLiveStream();
    
    // Stream functions
    int ReadLiveStream(unsigned char* buffer, unsigned int size);
    bool SwitchChannel(const PVR_CHANNEL& channel);
    PVR_ERROR GetSignalStatus(int channelUid, PVR_SIGNAL_STATUS* signalStatus);
}
```

### Memory Management Strategy
```cpp
class HdmiMemoryManager {
private:
    // Use H713 unified memory architecture
    struct MemoryPool {
        void* base_address;
        size_t total_size;
        size_t used_size;
        std::vector<MemoryBlock> free_blocks;
    };
    
    MemoryPool m_video_pool;
    MemoryPool m_audio_pool;
    
public:
    // Zero-copy buffer allocation
    VideoBuffer* AllocateVideoBuffer(size_t size);
    AudioBuffer* AllocateAudioBuffer(size_t size);
    
    // Direct memory sharing with hardware
    bool MapHardwareBuffer(void* hw_buffer, size_t size);
    void UnmapHardwareBuffer(void* buffer);
};
```

## 9. Integration with Existing Project Components

### Device Tree Integration
The HDMI input integration leverages device tree configuration from Task 019:
```dts
// TV capture hardware configuration
tvcap: tv-capture@5700000 {
    compatible = "allwinner,sun50i-h713-tvcap";
    reg = <0x5700000 0x400>;
    interrupts = <GIC_SPI 110 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&ccu CLK_BUS_TVCAP>, <&ccu CLK_CAP_300M>;
    clock-names = "bus", "capture";
    resets = <&ccu RST_BUS_TVCAP>;
    status = "okay";
};
```

### MIPS Co-processor Coordination
```cpp
class MipsVideoProcessor {
public:
    // Enhanced video processing via MIPS
    bool InitializeMipsProcessor();
    bool ConfigureVideoProcessing(const VideoFormat& format);
    bool ProcessVideoFrame(VideoFrame& input, VideoFrame& output);
    
    // Keystone correction for projector
    bool ApplyKeystoneCorrection(const KeystoneParams& params);
    
    // Memory sharing with ARM processor
    bool SetupSharedMemory(void* arm_buffer, size_t size);
};
```

### Remote Input System Integration
Based on project's existing remote control implementation:
```cpp
class HdmiRemoteHandler {
public:
    // Map remote buttons to HDMI functions
    void RegisterButtonMappings();
    void HandleChannelUp();
    void HandleChannelDown();
    void HandleInputButton();
    void HandleInfoButton();
    
    // Integration with existing remote system
    bool ProcessRemoteEvent(const RemoteEvent& event);
};
```

## 10. Risk Assessment and Mitigation

### Technical Risks
| Risk | Impact | Probability | Mitigation Strategy |
|------|--------|-------------|-------------------|
| **V4L2 Driver Delays** | High | Medium | Incremental testing, fallback implementation |
| **Latency Exceeds Target** | Medium | Medium | Hardware optimization, buffer tuning |
| **Audio Sync Issues** | Medium | Low | Robust sync algorithms, adaptive correction |
| **Memory Constraints** | Low | Low | Efficient buffer management, profiling |

### Integration Risks
| Risk | Impact | Probability | Mitigation Strategy |
|------|--------|-------------|-------------------|
| **Kodi API Changes** | Medium | Low | Version compatibility testing |
| **Hardware Incompatibility** | High | Low | Comprehensive hardware validation |
| **Performance Degradation** | Medium | Medium | Performance monitoring, optimization |

### User Experience Risks
| Risk | Impact | Probability | Mitigation Strategy |
|------|--------|-------------|-------------------|
| **Remote Control Conflicts** | Medium | Medium | Careful button mapping, user testing |
| **Complex Setup** | Low | Low | Automated configuration, documentation |
| **Reliability Issues** | High | Low | Extensive testing, error handling |

## 11. Success Metrics and Validation

### Key Performance Indicators
1. **Latency Performance**: <100ms end-to-end confirmed
2. **Resource Efficiency**: <10% CPU usage during streaming
3. **User Experience**: Single remote operation confirmed
4. **Reliability**: 99%+ uptime during continuous operation
5. **Quality**: No visible artifacts or audio sync issues

### Acceptance Testing Checklist
- [ ] HDMI input appears in Kodi channel list
- [ ] Channel switching via remote control functional
- [ ] Video quality matches input source capabilities
- [ ] Audio synchronized and properly routed
- [ ] Signal status accurately reported
- [ ] Hot-plug detection working reliably
- [ ] Performance targets met under load testing
- [ ] Integration with existing Kodi features confirmed
- [ ] Error handling robust across scenarios
- [ ] Documentation complete and accurate

## 12. Future Enhancement Opportunities

### Potential Expansions
1. **Multiple HDMI Inputs**: Support for additional input sources
2. **Picture-in-Picture**: HDMI input as overlay during Kodi content
3. **Recording Capability**: DVR functionality for HDMI input streams
4. **Enhanced Processing**: Advanced video filters and enhancements
5. **Streaming Output**: Network streaming of HDMI input content

### Integration Possibilities
1. **CEC Control**: Device control via HDMI-CEC protocol
2. **Auto-switching**: Intelligent input source detection
3. **Custom EPG**: Rich program guide information
4. **Voice Control**: Integration with voice assistant systems
5. **Mobile App**: Smartphone control interface

## Conclusion

This comprehensive design provides a complete framework for integrating HDMI input functionality into Kodi as a seamless channel experience. By leveraging the V4L2 driver development from Task 019 and building upon the H713 hardware capabilities identified in previous analysis, the system enables users to access external HDMI devices using only their projector remote control.

The phased implementation approach ensures systematic development with clear validation criteria at each stage. The design prioritizes low latency, seamless user experience, and robust performance while maintaining integration with existing project components and standards.

**Key Success Factors:**
- **Single Remote Operation**: All functionality accessible via existing projector remote
- **Seamless Integration**: HDMI input appears as standard Kodi TV channel
- **Low Latency**: <100ms real-time performance for live viewing
- **Hardware Optimization**: Full utilization of H713 video processing capabilities

This design positions the HY300 projector as a comprehensive media center capable of seamlessly integrating external HDMI devices alongside traditional Kodi media content.