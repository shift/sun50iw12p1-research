# Kodi PVR Client API Research for HDMI Input Integration

## Document Information
- **Task**: [Task 026] Kodi PVR client API and addon development framework research
- **Date**: September 23, 2025
- **Purpose**: Research foundation for HDMI input PVR client implementation
- **Target**: HY300 projector HDMI input integration with Kodi media center

## Executive Summary

This research establishes the foundation for developing a Kodi PVR client addon that integrates the HY300 projector's HDMI input capabilities. The PVR (Personal Video Recorder) client API provides the necessary framework for creating custom channel sources and stream handlers within Kodi's media pipeline.

## Kodi PVR Client Architecture

### Core Components Overview

The Kodi PVR system consists of several key architectural components:

1. **PVR Manager** (`PVRManager.cpp`) - Central coordination and lifecycle management
2. **PVR Clients** (`PVRClient.cpp`) - Addon interface and communication layer
3. **Channel Management** (`PVRChannel.cpp`, `PVRChannelGroups.cpp`) - Channel enumeration and organization
4. **Stream Handling** (`PVRStreamProperties.cpp`, `PVRStreamUtils.cpp`) - Video/audio stream management
5. **EPG Integration** (`EpgContainer.cpp`, `EpgInfoTag.cpp`) - Electronic Program Guide support
6. **Recording Support** (`PVRRecording.cpp`, `PVRRecordings.cpp`) - Optional recording functionality

### PVR Client Addon Interface

#### Key Header Files
The PVR client API is defined through several critical header files located in:
`xbmc/addons/kodi-dev-kit/include/kodi/c-api/addon-instance/pvr/`

**Primary API Headers:**
- `pvr_general.h` - Core addon lifecycle and capabilities
- `pvr_channels.h` - Channel enumeration and management
- `pvr_stream.h` - Stream handling and demultiplexing
- `pvr_epg.h` - Electronic Program Guide integration
- `pvr_recordings.h` - Recording functionality (optional)
- `pvr_timers.h` - Timer/scheduling support (optional)
- `pvr_defines.h` - Common constants and data structures

#### Addon Instance Structure
```cpp
class ATTRIBUTE_HIDDEN CInstancePVRClient : public IAddonInstanceHandler
{
public:
  // Core addon lifecycle
  virtual ADDON_STATUS GetStatus() override;
  virtual ADDON_STATUS Initialize() override;
  virtual void Deinitialize() override;
  
  // Channel management
  virtual PVR_ERROR GetChannelsAmount(int& amount);
  virtual PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results);
  virtual PVR_ERROR GetChannelGroups(bool radio, kodi::addon::PVRChannelGroupsResultSet& results);
  
  // Stream handling
  virtual bool OpenLiveStream(const kodi::addon::PVRChannel& channel);
  virtual void CloseLiveStream();
  virtual int ReadLiveStream(unsigned char* buffer, unsigned int size);
  virtual long long SeekLiveStream(long long position, int whence);
  virtual bool CanPauseStream();
  virtual bool CanSeekStream();
  
  // Optional EPG support
  virtual PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, 
                                     kodi::addon::PVREPGTagsResultSet& results);
};
```

## Channel Management Implementation

### Channel Definition Structure
Channels in Kodi PVR are defined with the following key properties:

```cpp
struct PVRChannel {
  unsigned int    iUniqueId;           // Unique channel identifier
  bool            bIsRadio;            // Radio vs TV channel flag
  unsigned int    iChannelNumber;      // Display channel number
  unsigned int    iSubChannelNumber;   // Sub-channel number (optional)
  char            strChannelName[256]; // Display name
  char            strInputFormat[32];  // Input format (e.g., "HDMI", "Component")
  char            strStreamURL[1024];  // Stream URL or identifier
  int             iEncryptionSystem;   // Encryption (0 = none)
  char            strIconPath[1024];   // Channel icon path
  bool            bIsHidden;           // Hidden channel flag
};
```

### HDMI Input Channel Implementation
For the HY300 HDMI input, the channel configuration would be:

```cpp
PVRChannel hdmiChannel = {
  .iUniqueId = 1,
  .bIsRadio = false,
  .iChannelNumber = 1,
  .iSubChannelNumber = 0,
  .strChannelName = "HDMI Input",
  .strInputFormat = "HDMI",
  .strStreamURL = "hdmi://input1", // Custom URL scheme
  .iEncryptionSystem = 0,
  .strIconPath = "/path/to/hdmi-icon.png",
  .bIsHidden = false
};
```

## Stream Handling Architecture

### Stream Lifecycle Management

1. **Stream Opening** (`OpenLiveStream`)
   - Validate channel parameters
   - Initialize hardware capture interface
   - Configure video/audio properties
   - Return success/failure status

2. **Stream Reading** (`ReadLiveStream`)
   - Provide continuous buffer reading interface
   - Handle real-time data flow from HDMI capture
   - Manage buffer underrun/overrun conditions
   - Return actual bytes read

3. **Stream Closing** (`CloseLiveStream`)
   - Clean up hardware resources
   - Stop capture processes
   - Release allocated buffers

### Stream Properties Configuration
```cpp
struct PVRStreamProperties {
  unsigned int    iStreamCount;        // Number of streams
  PVRStreamInfo*  stream;             // Stream information array
};

struct PVRStreamInfo {
  unsigned int    iCodecType;         // Video/Audio codec
  unsigned int    iCodecId;           // Specific codec identifier
  char            strLanguage[4];     // Language code
  unsigned int    iIdentifier;        // Stream identifier
  unsigned int    iFPSScale;          // Frame rate scale
  unsigned int    iFPSRate;           // Frame rate
  unsigned int    iHeight;            // Video height
  unsigned int    iWidth;             // Video width
  float           fAspect;            // Aspect ratio
  unsigned int    iChannels;          // Audio channels
  unsigned int    iSampleRate;        // Audio sample rate
  unsigned int    iBlockAlign;        // Audio block alignment
  unsigned int    iBitRate;           // Bit rate
  unsigned int    iBitsPerSample;     // Audio bits per sample
};
```

## Integration with HY300 Hardware

### Hardware Interface Layer

The PVR client addon will interface with the HY300's HDMI input through:

1. **Kernel Driver Interface** (`drivers/media/platform/sunxi/sunxi-tvcap-enhanced.c`)
   - V4L2 video capture device (`/dev/video0`)
   - Hardware-accelerated video processing
   - Real-time stream capture capabilities

2. **Stream Processing Pipeline**
   ```
   HDMI Input → Hardware Decoder → V4L2 Buffer → PVR Client → Kodi Pipeline
   ```

3. **Buffer Management**
   - Use V4L2 memory mapping for efficient data transfer
   - Implement circular buffer for continuous streaming
   - Handle frame synchronization and timing

### Video Format Support

The HY300 hardware supports multiple HDMI input formats:
- **Resolutions**: 480p, 720p, 1080i, 1080p
- **Frame Rates**: 23.976, 24, 25, 29.97, 30, 50, 59.94, 60 fps
- **Color Formats**: YUV420, YUV422, RGB

Stream properties must be dynamically detected and configured:

```cpp
PVR_ERROR HY300PVRClient::GetStreamProperties(PVRStreamProperties* props) {
  // Query hardware for current HDMI input format
  v4l2_format fmt;
  if (ioctl(hdmi_fd, VIDIOC_G_FMT, &fmt) < 0) {
    return PVR_ERROR_FAILED;
  }
  
  // Configure video stream
  props->stream[0].iCodecType = XBMC_CODEC_TYPE_VIDEO;
  props->stream[0].iCodecId = fmt.fmt.pix.pixelformat;
  props->stream[0].iWidth = fmt.fmt.pix.width;
  props->stream[0].iHeight = fmt.fmt.pix.height;
  
  // Detect frame rate from hardware
  struct v4l2_streamparm parm;
  ioctl(hdmi_fd, VIDIOC_G_PARM, &parm);
  props->stream[0].iFPSRate = parm.parm.capture.timeperframe.denominator;
  props->stream[0].iFPSScale = parm.parm.capture.timeperframe.numerator;
  
  return PVR_ERROR_NO_ERROR;
}
```

## Build System Integration

### Addon Build Configuration

The PVR addon uses standard Kodi addon build system based on CMake:

```cmake
# CMakeLists.txt for HY300 HDMI PVR addon
cmake_minimum_required(VERSION 3.5)
project(pvr.hy300hdmi)

find_package(Kodi REQUIRED)
find_package(kodiplatform REQUIRED)

set(HY300_SOURCES 
  src/HY300PVRClient.cpp
  src/HY300HDMICapture.cpp
  src/HY300StreamHandler.cpp
)

set(HY300_HEADERS
  src/HY300PVRClient.h
  src/HY300HDMICapture.h
  src/HY300StreamHandler.h
)

add_library(pvr.hy300hdmi ${HY300_SOURCES})
target_link_libraries(pvr.hy300hdmi ${kodiplatform_LIBRARIES})
target_include_directories(pvr.hy300hdmi PRIVATE ${KODI_INCLUDE_DIR})

# Addon metadata
configure_file(addon.xml.in addon.xml @ONLY)
```

### Addon Metadata Configuration
```xml
<?xml version="1.0" encoding="UTF-8"?>
<addon id="pvr.hy300hdmi" 
       version="1.0.0"
       name="HY300 HDMI Input PVR Client"
       provider-name="HY300 Project">
  <requires>
    <import addon="xbmc.pvr" version="8.2.0"/>
  </requires>
  <extension point="xbmc.pvrclient" library_linux="pvr.hy300hdmi.so">
    <supports_tv>true</supports_tv>
    <supports_radio>false</supports_radio>
    <supports_channelgroups>true</supports_channelgroups>
    <supports_epg>false</supports_epg>
    <supports_recordings>false</supports_recordings>
    <supports_timers>false</supports_timers>
  </extension>
  <extension point="xbmc.addon.metadata">
    <summary lang="en">HY300 Projector HDMI Input PVR Client</summary>
    <description lang="en">
      Provides access to HY300 projector HDMI input as a TV channel in Kodi.
      Supports live viewing of HDMI sources through the projector's hardware.
    </description>
    <platform>linux</platform>
  </extension>
</addon>
```

## Resource Management and Cleanup

### Memory Management Patterns
```cpp
class HY300PVRClient {
private:
  int hdmi_fd;
  void* mapped_buffers[NUM_BUFFERS];
  bool streaming_active;
  std::thread capture_thread;
  
public:
  ~HY300PVRClient() {
    // Ensure cleanup on destruction
    CloseLiveStream();
    CleanupResources();
  }
  
  void CleanupResources() {
    if (hdmi_fd >= 0) {
      // Unmap V4L2 buffers
      for (int i = 0; i < NUM_BUFFERS; i++) {
        if (mapped_buffers[i] != MAP_FAILED) {
          munmap(mapped_buffers[i], buffer_size);
          mapped_buffers[i] = MAP_FAILED;
        }
      }
      close(hdmi_fd);
      hdmi_fd = -1;
    }
  }
};
```

### Error Handling Strategy
```cpp
PVR_ERROR HY300PVRClient::OpenLiveStream(const PVRChannel& channel) {
  try {
    // Validate channel
    if (channel.iUniqueId != HDMI_CHANNEL_ID) {
      kodi::Log(ADDON_LOG_ERROR, "Invalid channel ID: %d", channel.iUniqueId);
      return PVR_ERROR_INVALID_PARAMETERS;
    }
    
    // Open hardware device
    hdmi_fd = open("/dev/video0", O_RDWR);
    if (hdmi_fd < 0) {
      kodi::Log(ADDON_LOG_ERROR, "Failed to open HDMI capture device: %s", 
                strerror(errno));
      return PVR_ERROR_SERVER_ERROR;
    }
    
    // Configure capture format
    if (ConfigureCapture() != PVR_ERROR_NO_ERROR) {
      close(hdmi_fd);
      hdmi_fd = -1;
      return PVR_ERROR_SERVER_ERROR;
    }
    
    streaming_active = true;
    return PVR_ERROR_NO_ERROR;
    
  } catch (const std::exception& e) {
    kodi::Log(ADDON_LOG_ERROR, "Exception in OpenLiveStream: %s", e.what());
    CleanupResources();
    return PVR_ERROR_SERVER_ERROR;
  }
}
```

## Development Implementation Template

### Basic PVR Client Skeleton
```cpp
// HY300PVRClient.h
#include <kodi/addon-instance/PVR.h>
#include <kodi/General.h>

class ATTRIBUTE_HIDDEN HY300PVRClient : public kodi::addon::CInstancePVRClient
{
public:
  HY300PVRClient(KODI_HANDLE instance, const std::string& kodiVersion);
  virtual ~HY300PVRClient();

  // Inherited from CInstancePVRClient
  PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override;
  PVR_ERROR GetBackendName(std::string& name) override;
  PVR_ERROR GetChannelsAmount(int& amount) override;
  PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override;
  
  bool OpenLiveStream(const kodi::addon::PVRChannel& channel) override;
  void CloseLiveStream() override;
  int ReadLiveStream(unsigned char* buffer, unsigned int size) override;
  long long SeekLiveStream(long long position, int whence) override;
  
private:
  bool InitializeHardware();
  void CleanupHardware();
  PVR_ERROR ConfigureCapture();
  
  int hdmi_device_fd;
  bool stream_active;
  std::mutex stream_mutex;
};

// Entry point
class ATTRIBUTE_HIDDEN CHY300Addon : public kodi::addon::CAddonBase
{
public:
  CHY300Addon() = default;
  ADDON_STATUS CreateInstance(int instanceType, const std::string& instanceID, 
                              KODI_HANDLE instance, const std::string& version, 
                              KODI_HANDLE& addonInstance) override;
};

ADDONCREATOR(CHY300Addon)
```

## Integration Points with Existing Codebase

### Connection to HY300 Driver Infrastructure

The PVR client will integrate with existing HY300 driver components:

1. **HDMI Capture Driver** (`drivers/media/platform/sunxi/sunxi-tvcap-enhanced.c`)
   - Provides V4L2 interface for video capture
   - Handles hardware video processing
   - Manages capture buffers and timing

2. **Motor Control Integration** (`drivers/misc/hy300-keystone-motor.c`)
   - PVR client can trigger keystone adjustments
   - Automatic image correction based on input source

3. **Service Layer** (`nixos/modules/hy300-services.nix`)
   - System service management for PVR addon
   - Hardware resource coordination
   - Service dependency management

### Kodi Integration Architecture
```
User Interface
     ↓
Kodi PVR Manager
     ↓
HY300 PVR Client Addon
     ↓
V4L2 Video Capture API
     ↓
HY300 HDMI Capture Driver
     ↓
Hardware HDMI Input
```

## Quality Assurance and Testing Strategy

### Unit Testing Framework
```cpp
// Test framework for PVR client validation
class HY300PVRClientTest : public testing::Test {
protected:
  void SetUp() override {
    client = std::make_unique<HY300PVRClient>(nullptr, "19.0.0");
  }
  
  void TearDown() override {
    client.reset();
  }
  
  std::unique_ptr<HY300PVRClient> client;
};

TEST_F(HY300PVRClientTest, ChannelEnumeration) {
  int channelCount = 0;
  EXPECT_EQ(PVR_ERROR_NO_ERROR, client->GetChannelsAmount(channelCount));
  EXPECT_EQ(1, channelCount); // HDMI input channel
}

TEST_F(HY300PVRClientTest, StreamLifecycle) {
  kodi::addon::PVRChannel channel;
  channel.SetUniqueId(1);
  channel.SetChannelName("HDMI Input");
  
  EXPECT_TRUE(client->OpenLiveStream(channel));
  
  unsigned char buffer[1024];
  int bytesRead = client->ReadLiveStream(buffer, sizeof(buffer));
  EXPECT_GT(bytesRead, 0);
  
  client->CloseLiveStream();
}
```

### Integration Testing
1. **Hardware Validation**: Test with various HDMI input sources
2. **Format Support**: Verify multiple resolution and frame rate handling
3. **Resource Management**: Validate proper cleanup under error conditions
4. **Performance Testing**: Measure streaming latency and throughput
5. **Stability Testing**: Long-duration streaming tests

## Documentation and Examples

### Reference Implementations
Study existing PVR clients for implementation patterns:
- `pvr.demo` - Basic PVR client structure and API usage
- `pvr.iptvsimple` - Network streaming implementation
- `pvr.hdhomerun` - Hardware capture device integration

### API Documentation Resources
- **Kodi PVR API Documentation**: https://github.com/xbmc/xbmc/tree/master/xbmc/addons/kodi-dev-kit/include/kodi/addon-instance/pvr
- **V4L2 Documentation**: https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/
- **Kodi Addon Development**: https://kodi.wiki/view/Add-on_development

## Next Steps and Implementation Roadmap

### Phase 1: Basic PVR Client Framework
1. Create addon directory structure and build system
2. Implement basic PVR client skeleton with channel enumeration
3. Add HDMI channel definition and basic stream interface
4. Test addon loading and channel listing in Kodi

### Phase 2: Hardware Integration
1. Integrate with V4L2 HDMI capture interface
2. Implement stream opening and buffer reading
3. Add format detection and stream property configuration
4. Test basic video streaming functionality

### Phase 3: Enhanced Features
1. Add error handling and resource management
2. Implement stream seeking and playback controls (if supported)
3. Add logging and debugging capabilities
4. Performance optimization and stability testing

### Phase 4: System Integration
1. Integration with NixOS packaging system
2. Service management and dependency handling
3. User interface refinements and configuration options
4. Comprehensive testing and validation

## Conclusion

The Kodi PVR client API provides a robust framework for integrating the HY300 projector's HDMI input as a live TV source. The implementation will leverage the existing V4L2 capture driver infrastructure while providing a seamless user experience through Kodi's media interface.

Key success factors:
- **Clean API Implementation**: Follow Kodi PVR patterns for reliability
- **Hardware Integration**: Efficient V4L2 buffer management for real-time streaming
- **Error Handling**: Robust error management for hardware interactions
- **Resource Management**: Proper cleanup and resource lifecycle management
- **Performance**: Low-latency streaming suitable for interactive use

This research provides the foundation for developing a production-quality PVR client that integrates seamlessly with the HY300 hardware platform and Kodi media center.

---
*Research completed for Task 026 - Kodi PVR API and addon development framework research*
*Cross-references: HY300_HDMI_INPUT_INTEGRATION_DESIGN.md, KODI_H713_HARDWARE_ANALYSIS.md*