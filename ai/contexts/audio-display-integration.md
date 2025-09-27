# Audio and Display Integration Context

## Purpose
Specialized guidance for integrating projector-specific audio and display systems, including SPDIF output, HDMI processing, and projector control integration.

## Hardware Architecture Overview

### HY300 Projector Audio System
**Primary Audio Output**: SPDIF digital output to projector speakers
**Secondary Output**: 3.5mm analog audio jack for external devices
**Audio Processing**: Integrated H713 audio subsystem with dedicated audio codec

### Display and Projection System
**Display Controller**: H713 integrated display engine with Mali GPU acceleration
**HDMI Input**: Dedicated HDMI input capture via MIPS co-processor
**Projection Engine**: MIPS-controlled lamp, cooling, and optical systems
**Keystone Correction**: Hardware-accelerated geometric correction

## Audio System Integration

### SPDIF Digital Audio Configuration
**Hardware Specifications**:
- **Output Format**: IEC958 SPDIF standard
- **Sample Rates**: 44.1kHz, 48kHz, 96kHz support
- **Bit Depth**: 16-bit and 24-bit audio
- **Connector**: Optical SPDIF output to projector

**Device Tree Configuration**:
```dts
audio_codec: audio-codec@5096000 {
    compatible = "allwinner,sun50i-h713-codec";
    reg = <0x5096000 0x31c>;
    interrupts = <GIC_SPI 28 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&ccu CLK_BUS_AUDIO_CODEC>, 
             <&ccu CLK_AUDIO_CODEC_1X>,
             <&ccu CLK_AUDIO_CODEC_4X>;
    clock-names = "bus", "codec", "codec-4x";
    resets = <&ccu RST_BUS_AUDIO_CODEC>;
    dmas = <&dma 15>, <&dma 15>;
    dma-names = "rx", "tx";
    allwinner,spdif-enabled;
    status = "okay";
};

spdif: spdif@5093000 {
    compatible = "allwinner,sun50i-h6-spdif";
    reg = <0x5093000 0x400>;
    interrupts = <GIC_SPI 21 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&ccu CLK_BUS_SPDIF>, <&ccu CLK_SPDIF>;
    clock-names = "apb", "spdif";
    resets = <&ccu RST_BUS_SPDIF>;
    dmas = <&dma 2>;
    dma-names = "tx";
    pinctrl-names = "default";
    pinctrl-0 = <&spdif_tx_pin>;
    #sound-dai-cells = <0>;
    status = "okay";
};
```

### Audio Routing and ALSA Configuration
**ALSA Card Configuration**:
- **Primary Device**: SPDIF output (card 0, device 0)
- **Secondary Device**: Analog codec (card 0, device 1)
- **Mixer Controls**: Volume, mute, source selection
- **Default Routing**: SPDIF output for Kodi media playback

**PulseAudio/PipeWire Integration**:
```bash
# Default audio sink configuration
default-sink = alsa_output.platform-spdif.iec958-stereo
# Projector-specific audio routing
load-module module-alsa-sink device=hw:0,0 sink_name=projector_spdif
```

## Display System Integration

### HDMI Input Processing
**V4L2 HDMI Capture Integration**:
- **Driver**: `sunxi-tvcap.c` (already implemented)
- **Device Node**: `/dev/video0` for HDMI input capture
- **Resolution Support**: 1080p, 720p, 480p input formats
- **Color Space**: YUV422, RGB888 conversion support

**MIPS Co-processor Communication**:
- **Control Interface**: ARM-MIPS communication protocol
- **HDMI Detection**: Real-time input signal detection
- **Format Negotiation**: Automatic input format detection
- **Signal Processing**: Hardware-accelerated processing pipeline

### Display Output and Projection Control
**Mali GPU Integration**:
- **Graphics Acceleration**: OpenGL ES support for Kodi
- **Video Acceleration**: Hardware video decoding (H.264, H.265, AV1)
- **Render Pipeline**: GPU-accelerated rendering to projector display
- **Memory Management**: CMA and IOMMU integration

**Projector Hardware Control**:
- **Lamp Control**: Power sequencing and brightness control
- **Cooling System**: Fan speed and temperature monitoring
- **Optical System**: Focus and zoom control (if available)
- **Keystone Correction**: Real-time geometric correction

## Kodi Media Center Integration

### Audio Output Configuration
**Kodi Audio Settings**:
```xml
<audiooutput>
    <audiodevice>ALSA:iec958:CARD=0,DEV=0</audiodevice>
    <passthroughdevice>iec958:CARD=0,DEV=0</passthroughdevice>
    <streamsilence>1</streamsilence>
    <ac3passthrough>true</ac3passthrough>
    <dtspassthrough>true</dtspassthrough>
</audiooutput>
```

**Digital Audio Passthrough**:
- **AC3/DTS Passthrough**: Direct digital audio to projector
- **Multi-channel Audio**: 5.1/7.1 surround sound support
- **Sample Rate Management**: Automatic sample rate switching
- **Volume Control**: SPDIF volume control integration

### Video Output and Hardware Acceleration
**Kodi Video Configuration**:
```xml
<videoplayer>
    <usevaapi>true</usevaapi>
    <usevdpau>false</usevdpau>
    <render_method>17</render_method> <!-- Mali GPU -->
    <adjustrefreshrate>true</adjustrefreshrate>
</videoplayer>
```

**Hardware Decoding Pipeline**:
- **AV1 Hardware**: Dedicated AV1 decoder utilization
- **H.264/H.265**: VPU hardware acceleration
- **Mali GPU**: OpenGL ES rendering acceleration
- **Memory Optimization**: Zero-copy video pipeline

### HDMI Input Integration
**Kodi PVR HDMI Input**:
- **Live TV Source**: HDMI input as live TV channel
- **V4L2 Integration**: Direct integration with sunxi-tvcap driver
- **Resolution Handling**: Automatic input resolution detection
- **Audio Extraction**: HDMI audio extraction and routing

## Service Integration Architecture

### Audio Service Integration
**HY300 Audio Service**:
```python
class HY300AudioService:
    def __init__(self):
        self.spdif_device = "/dev/snd/pcmC0D0p"
        self.analog_device = "/dev/snd/pcmC0D1p"
        
    def configure_audio_routing(self):
        # Configure ALSA routing for projector
        # Set default SPDIF output
        # Configure volume controls
        
    def test_audio_output(self):
        # Test SPDIF output functionality
        # Validate audio routing
        # Check projector audio reception
```

### Display Service Integration
**HY300 Display Service**:
```python
class HY300DisplayService:
    def __init__(self):
        self.mips_comm = MIPSCommunication()
        self.v4l2_hdmi = V4L2HDMIDevice()
        
    def configure_hdmi_input(self):
        # Configure HDMI input capture
        # Set up MIPS communication
        # Initialize format detection
        
    def handle_projector_control(self):
        # Lamp power management
        # Cooling system control
        # Keystone correction interface
```

## Testing and Validation Procedures

### Audio System Testing
1. **SPDIF Output Validation**
   ```bash
   # Test SPDIF output
   aplay -D hw:0,0 test_audio.wav
   # Check digital output signal
   ```

2. **Kodi Audio Integration**
   ```bash
   # Test Kodi audio output
   kodi --debug --audio-backend=alsa
   # Validate AC3/DTS passthrough
   ```

### Display System Testing
1. **HDMI Input Capture**
   ```bash
   # Test HDMI input capture
   v4l2-ctl --device=/dev/video0 --stream-mmap
   # Validate input signal detection
   ```

2. **GPU Acceleration Validation**
   ```bash
   # Test Mali GPU functionality
   glxgears -info
   # Kodi hardware acceleration test
   ```

## Performance Optimization

### Audio Latency Optimization
- **Buffer Management**: Optimized audio buffer sizes for projector
- **Real-time Scheduling**: Priority scheduling for audio threads
- **Interrupt Handling**: Low-latency audio interrupt processing
- **Sample Rate Sync**: Synchronized sample rate conversion

### Display Performance Optimization
- **Zero-copy Pipeline**: Direct memory access for video frames
- **GPU Memory Management**: Optimized GPU memory allocation
- **HDMI Processing**: Low-latency HDMI input processing
- **Render Optimization**: Optimized rendering pipeline for projector

## Integration Dependencies

### Hardware Dependencies
- **Power Management**: Proper audio/display power sequencing
- **Clock Management**: Synchronized audio/video clock domains
- **Thermal Management**: Audio/display thermal considerations
- **GPIO Configuration**: Proper pin configurations for audio/display

### Service Dependencies
- **MIPS Communication**: Audio/display coordination with MIPS processor
- **Keystone Service**: Display geometry correction integration
- **Network Service**: Remote control and streaming capabilities
- **System Service**: Overall system coordination and monitoring

This specialized context ensures proper integration of projector-specific audio and display systems with comprehensive hardware acceleration and service coordination.