# Task 029: Audio and SPDIF Configuration

## Status
- **Current Status**: pending
- **Priority**: high
- **Estimated Effort**: 2-3 days
- **Dependencies**: Mali GPU Driver Selection (Task 028)

## Objective
Implement complete audio system integration including SPDIF digital output to projector speakers, analog audio output, and Kodi audio configuration with hardware acceleration support.

## Background
HY300 projector features integrated audio system with SPDIF digital output and analog audio jack. Factory analysis confirms H713 audio subsystem with dedicated audio codec. This task implements complete audio functionality for media center usage.

## Success Criteria
- [ ] H713 audio codec driver integrated and functional
- [ ] SPDIF digital output working correctly to projector speakers
- [ ] Analog audio output functional via 3.5mm jack
- [ ] ALSA and PulseAudio/PipeWire configuration complete
- [ ] Kodi audio output and AC3/DTS passthrough working
- [ ] Audio routing and volume control integrated with HY300 services

## Implementation Steps

### Phase 1: H713 Audio Codec Integration
1. **Audio Codec Device Tree Configuration**
   ```dts
   audio_codec: audio-codec@5096000 {
       compatible = "allwinner,sun50i-h6-codec";
       reg = <0x5096000 0x31c>;
       interrupts = <GIC_SPI 28 IRQ_TYPE_LEVEL_HIGH>;
       clocks = <&ccu CLK_BUS_AUDIO_CODEC>, 
                <&ccu CLK_AUDIO_CODEC_1X>,
                <&ccu CLK_AUDIO_CODEC_4X>;
       clock-names = "bus", "codec", "codec-4x";
       resets = <&ccu RST_BUS_AUDIO_CODEC>;
       dmas = <&dma 15>, <&dma 15>;
       dma-names = "rx", "tx";
       status = "okay";
   };
   ```

2. **SPDIF Digital Output Configuration**
   ```dts
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

3. **Audio Pin Configuration**
   ```dts
   &pio {
       spdif_tx_pin: spdif-tx-pin {
           pins = "PH8";
           function = "spdif";
       };
       
       codec_pins: codec-pins {
           pins = "PH6", "PH7";
           function = "codec";
       };
   };
   ```

### Phase 2: ALSA Sound Card Configuration
1. **Sound Card Device Tree**
   ```dts
   sound: sound {
       compatible = "simple-audio-card";
       simple-audio-card,name = "HY300-Audio";
       simple-audio-card,mclk-fs = <512>;
       status = "okay";

       simple-audio-card,dai-link@0 {
           format = "i2s";
           frame-master = <&link0_cpu>;
           bitclock-master = <&link0_cpu>;
           
           link0_cpu: cpu {
               sound-dai = <&audio_codec>;
           };
           
           link0_codec: codec {
               sound-dai = <&audio_codec>;
           };
       };
       
       simple-audio-card,dai-link@1 {
           format = "iec958";
           
           spdif_cpu: cpu {
               sound-dai = <&spdif>;
           };
           
           spdif_codec: codec {
               sound-dai = <&spdif_out>;
           };
       };
   };

   spdif_out: spdif-out {
       compatible = "linux,spdif-dit";
       #sound-dai-cells = <0>;
   };
   ```

2. **ALSA Configuration**
   ```conf
   # /etc/asound.conf
   pcm.!default {
       type pulse
   }
   ctl.!default {
       type pulse
   }

   pcm.spdif {
       type hw
       card 0
       device 1
   }

   pcm.analog {
       type hw
       card 0
       device 0
   }

   # Hardware-specific mixer controls
   pcm.projector {
       type plug
       slave.pcm "spdif"
       slave.format S16_LE
       slave.rate 48000
   }
   ```

### Phase 3: PulseAudio/PipeWire Integration
1. **PulseAudio Configuration**
   ```conf
   # /etc/pulse/default.pa additions
   
   # Load SPDIF module
   load-module module-alsa-sink device=hw:0,1 sink_name=spdif_out \
       sink_properties=device.description="HY300 SPDIF Output"
   
   # Load analog output module  
   load-module module-alsa-sink device=hw:0,0 sink_name=analog_out \
       sink_properties=device.description="HY300 Analog Output"
   
   # Set SPDIF as default for projector
   set-default-sink spdif_out
   
   # Load module for digital passthrough
   load-module module-alsa-sink device=hw:0,1 sink_name=passthrough \
       format=s16le rate=48000 channels=2 \
       sink_properties=device.description="HY300 Digital Passthrough"
   ```

2. **Audio Routing Service**
   ```python
   class HY300AudioService:
       def __init__(self, simulation_mode=False):
           self.simulation_mode = simulation_mode
           self.pulse_client = pulsectl.Pulse() if not simulation_mode else None
           
       def set_audio_output(self, output_type="spdif"):
           """Set audio output routing"""
           if self.simulation_mode:
               return self._simulate_audio_routing(output_type)
           
           sinks = {
               "spdif": "spdif_out",
               "analog": "analog_out", 
               "passthrough": "passthrough"
           }
           
           if output_type in sinks:
               self.pulse_client.sink_default_set(sinks[output_type])
               
       def configure_volume(self, volume_percent):
           """Configure audio volume"""
           if self.simulation_mode:
               return self._simulate_volume_control(volume_percent)
           
           volume = pulsectl.PulseVolumeInfo(volume_percent / 100.0)
           default_sink = self.pulse_client.server_info().default_sink_name
           sink = self.pulse_client.get_sink_by_name(default_sink)
           self.pulse_client.volume_set(sink, volume)
   ```

### Phase 4: Kodi Audio Integration
1. **Kodi Audio Configuration**
   ```xml
   <!-- ~/.kodi/userdata/advancedsettings.xml -->
   <advancedsettings>
       <audio>
           <audiodevice>PULSE:spdif_out</audiodevice>
           <passthroughdevice>PULSE:passthrough</passthroughdevice>
           <streamsilence>1</streamsilence>
           <ac3passthrough>true</ac3passthrough>
           <dtspassthrough>true</dtspassthrough>
           <truehdpassthrough>true</truehdpassthrough>
           <dtshdpassthrough>true</dtshdpassthrough>
           <audiolatency>0.0</audiolatency>
           <applydrc>false</applydrc>
       </audio>
   </advancedsettings>
   ```

2. **Audio Codec Support**
   ```xml
   <!-- Kodi codec configuration -->
   <videoplayer>
       <usevaapi>true</usevaapi>
       <vaapideintmethod>1</vaapideintmethod>
       <prefervaapirender>true</prefervaapirender>
   </videoplayer>
   
   <audiooutput>
       <mode>2</mode> <!-- Digital output -->
       <ac3passthrough>true</ac3passthrough>
       <dtspassthrough>true</dtspassthrough>
       <channels>2</channels>
       <samplingrate>48000</samplingrate>
       <bitspersample>16</bitspersample>
   </audiooutput>
   ```

3. **Audio Processing Pipeline**
   - Configure hardware audio decoding integration
   - Set up zero-copy audio pipeline from GPU to SPDIF
   - Integrate with AV1 hardware decoder audio output
   - Optimize audio latency for video synchronization

## Technical Requirements

### Hardware Specifications
- **Audio Codec**: H713 integrated audio subsystem
- **SPDIF Output**: IEC958 standard digital audio output
- **Analog Output**: 3.5mm stereo audio jack
- **Sample Rates**: 44.1kHz, 48kHz, 96kHz support
- **Bit Depth**: 16-bit and 24-bit audio processing

### Software Dependencies
- **ALSA**: Advanced Linux Sound Architecture
- **PulseAudio/PipeWire**: Audio server and routing
- **Kodi**: Media center audio integration
- **Audio Libraries**: libavcodec, libasound, libpulse

### Performance Requirements
- **Latency**: < 40ms audio latency for lip-sync
- **Quality**: Lossless digital passthrough for AC3/DTS
- **Stability**: No audio dropouts or glitches during playback
- **Volume Range**: Full dynamic range with proper volume control

## Testing Procedures

### Phase 1 Testing: Hardware Detection
1. **Audio Hardware Detection**
   ```bash
   # Check audio codec detection
   dmesg | grep -i audio
   cat /proc/asound/cards
   aplay -l
   ```

2. **SPDIF Output Testing**
   ```bash
   # Test SPDIF functionality
   aplay -D hw:0,1 /usr/share/sounds/alsa/Front_Left.wav
   # Check SPDIF signal output
   cat /proc/asound/card0/pcm1p/sub0/status
   ```

### Phase 2 Testing: ALSA Configuration
1. **ALSA Functionality**
   ```bash
   # Test analog output
   aplay -D hw:0,0 test_audio.wav
   # Test ALSA mixer controls
   amixer scontrols
   amixer set Master 80%
   ```

2. **Audio Format Testing**
   ```bash
   # Test different sample rates
   aplay -D hw:0,1 -r 48000 -f S16_LE test_48k.wav
   # Test stereo output
   speaker-test -D hw:0,1 -c 2 -t sine
   ```

### Phase 3 Testing: PulseAudio Integration
1. **PulseAudio Functionality**
   ```bash
   # Test PulseAudio sinks
   pactl list sinks
   # Test audio routing
   pactl set-default-sink spdif_out
   pacmd play-sample 0
   ```

2. **Audio Service Testing**
   ```bash
   # Test HY300 audio service
   systemctl status hy300-audio
   # Test service audio routing
   curl -X POST localhost:8080/api/audio/output/spdif
   ```

### Phase 4 Testing: Kodi Integration
1. **Kodi Audio Validation**
   - Test Kodi audio output selection
   - Validate AC3/DTS passthrough functionality
   - Check audio-video synchronization
   - Test volume control integration

2. **Media Playback Testing**
   - Test various audio formats (MP3, FLAC, AC3, DTS)
   - Validate surround sound output via SPDIF
   - Check audio quality with different sample rates
   - Test audio during video playback

## Quality Assurance

### Audio Quality Standards
- **Bit-perfect Output**: Lossless digital audio passthrough
- **Low Latency**: Minimal audio processing latency
- **No Dropouts**: Stable audio output without interruptions
- **Full Dynamic Range**: Proper volume control across full range

### Integration Validation
- **Service Integration**: Seamless HY300 service audio control
- **Kodi Compatibility**: Full Kodi audio feature support  
- **Hardware Acceleration**: Integration with GPU and AV1 audio
- **User Experience**: Intuitive audio control and configuration

## Risk Assessment

### Technical Risks
- **Hardware Compatibility**: H713 audio codec driver compatibility
- **Audio Latency**: Potential audio-video synchronization issues
- **Digital Output**: SPDIF signal compatibility with projector
- **Service Integration**: Complex audio routing service coordination

### Mitigation Strategies
- **Incremental Testing**: Layer-by-layer audio system validation
- **Fallback Options**: Multiple audio output options available
- **Performance Monitoring**: Continuous audio performance validation
- **Hardware Testing**: Comprehensive SPDIF output validation

## Success Metrics
- **Hardware Detection**: 100% reliable audio hardware detection
- **Audio Quality**: Lossless digital audio output capability
- **Integration**: Seamless Kodi and service audio integration
- **Performance**: Low-latency, high-quality audio output
- **User Experience**: Simple, reliable audio control

## Documentation Updates Required

### Hardware Status Updates
- Update `docs/HY300_HARDWARE_ENABLEMENT_STATUS.md` with audio status
- Document audio configuration and routing details
- Update service integration audio specifications

### Technical Documentation
- Create audio configuration and troubleshooting guide
- Document SPDIF output specifications and requirements
- Update Kodi audio integration documentation

## Next Steps After Completion
- **Device Tree Completion** (Task 024): Add remaining hardware nodes
- **Hardware Testing Preparation** (Task 025): FEL mode testing preparation
- **Motor Driver Integration** (Task 020): Hardware keystone control

This task establishes high-quality audio output as an essential component of the complete media center experience with both digital and analog output capabilities.