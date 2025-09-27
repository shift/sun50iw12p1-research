# Task 019: kodi-pvr-hdmi-input-implementation

**Status:** in_progress  
**Priority:** high  
**Phase:** VIII - VM Testing and Integration  
**Assigned:** AI Agent  
**Created:** 2025-09-23  
**Context:** HY300 Kodi PVR client implementation

## Objective

Implement the complete Kodi PVR client for HY300 HDMI input integration. The addon provides HDMI input as a live TV channel in Kodi, integrating with the hardware V4L2 capture driver for real-time streaming.

## Prerequisites

- [x] PVR addon structure and headers defined (`pvr.hdmi-input/`)
- [x] V4L2 HDMI capture driver implemented (`drivers/media/platform/sunxi/sunxi-tvcap-enhanced.c`)
- [x] Kodi PVR API research completed (`docs/KODI_PVR_API_RESEARCH.md`)
- [x] Hardware integration design finalized (`docs/KODI_HDMI_INPUT_INTEGRATION_DESIGN.md`)
- [ ] Build system configuration for Kodi addon compilation

## Acceptance Criteria

- [ ] Complete PVR client implementation with all missing source files
- [ ] V4L2 device integration for HDMI capture hardware
- [ ] Channel manager providing HDMI input as TV channel
- [ ] Stream processor for real-time video/audio handling
- [ ] Signal monitor for HDMI connection status detection
- [ ] EPG provider for basic program guide functionality
- [ ] Build system integration with NixOS flake
- [ ] Error handling and resource management
- [ ] Memory-safe buffer management for high-throughput streaming
- [ ] Hardware-accelerated demuxing support
- [ ] Settings interface for user configuration

## Implementation Steps

### 1. V4L2 Device Implementation (`src/v4l2_device.cpp`)
- Implement V4L2 device management and capabilities query
- Add format detection and configuration
- Implement buffer allocation and memory mapping
- Add streaming control and frame capture
- Implement signal detection and status monitoring

### 2. Channel Manager Implementation (`src/channel_manager.cpp`)
- Create HDMI input channel definition
- Implement channel enumeration for Kodi
- Add channel groups and organization
- Handle channel settings and configuration

### 3. Stream Processor Implementation (`src/stream_processor.cpp`)
- Implement real-time video/audio stream processing
- Add hardware-accelerated demuxing
- Handle stream properties detection and configuration
- Implement buffer management for continuous streaming

### 4. Signal Monitor Implementation (`src/signal_monitor.cpp`)
- Implement HDMI signal detection and monitoring
- Add hot-plug detection capabilities
- Provide signal strength and quality reporting
- Handle connection state changes

### 5. HDMI Client Integration (`src/hdmi_client.cpp`)
- Implement main client class coordinating all components
- Add lifecycle management and initialization
- Implement PVR API interface methods
- Add settings management and configuration

### 6. Build System Integration
- Configure CMakeLists.txt for NixOS cross-compilation
- Add Kodi development dependencies to flake.nix
- Implement addon packaging and installation
- Add build targets and validation

## Quality Validation

- [ ] V4L2 device operations tested with `/dev/video0`
- [ ] Channel enumeration working in Kodi interface
- [ ] Live streaming functional with HDMI input sources
- [ ] Signal detection and status reporting accurate
- [ ] Memory management validated with no leaks
- [ ] Error handling tested under various failure conditions
- [ ] Build system produces functional addon package
- [ ] Integration testing with full Kodi installation

## Architecture Integration

### V4L2 Hardware Interface
```cpp
// V4L2 capture flow
HDMI Input → Hardware Decoder → V4L2 Buffer → PVR Client → Kodi Pipeline
```

### Component Dependencies
```cpp
HdmiClient
├── V4L2Device (hardware interface)
├── ChannelManager (channel enumeration)
├── StreamProcessor (stream handling)
└── SignalMonitor (connection status)
```

### Kodi Integration Points
- PVR Manager API for addon lifecycle
- Channel management for TV guide integration
- Stream handling for live video playback
- EPG system for program information
- Settings interface for user configuration

## Next Task Dependencies

- **VM Testing**: PVR addon must be complete for Kodi testing in VM environment
- **Hardware Validation**: Addon ready for real hardware deployment testing
- **Service Integration**: PVR client integrates with broader HY300 service stack

## Notes

**Critical Implementation Details:**
- **Memory Management**: Use RAII and smart pointers for V4L2 buffer management
- **Threading**: Implement thread-safe stream processing with proper synchronization
- **Error Recovery**: Handle device disconnection and reconnection gracefully
- **Performance**: Optimize for real-time streaming with minimal latency
- **Hardware Integration**: Leverage existing V4L2 driver capabilities fully

**Build System Requirements:**
- Cross-compilation support for ARM64 target
- Kodi development headers and libraries
- V4L2 library dependencies
- NixOS packaging integration

**Testing Strategy:**
- Unit tests for individual components
- Integration tests with V4L2 mock devices
- End-to-end testing with Kodi in VM environment
- Hardware validation with real HDMI sources

**External References:**
- Kodi PVR API: `docs/KODI_PVR_API_RESEARCH.md`
- Hardware Design: `docs/KODI_HDMI_INPUT_INTEGRATION_DESIGN.md`
- V4L2 Driver: `drivers/media/platform/sunxi/sunxi-tvcap-enhanced.c`
