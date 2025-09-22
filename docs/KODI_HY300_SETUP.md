# HY300 Kodi Media Center Configuration

## Overview
Complete Kodi configuration optimized for the HY300 projector with required addons and projector-specific customizations.

## Included Components

### Core Kodi Setup
- **Base**: Kodi 20.x (Nexus) with hardware acceleration
- **Skin**: Nimbus skin (modern, clean, projector-friendly interface)
- **Optimization**: Custom settings for projector environment and remote control navigation

### Required Addons
1. **Nimbus Skin** - Clean, modern interface optimized for 1080p projection
2. **FenLightAM** - Lightweight media addon for movies and TV shows
3. **CocoScrapers** - Web scraping module supporting content discovery

### HY300-Specific Plugins
1. **HY300 Keystone Correction** - Motor and digital keystone adjustment
2. **HY300 WiFi Setup** - IR remote-controlled network configuration

## Configuration Details

### Hardware Acceleration
- **Video Decoding**: H.264/AVC, H.265/HEVC via Cedrus VPU
- **GPU Acceleration**: Mali-G31 MP2 (when available)
- **Audio**: PulseAudio with projector speaker optimization

### Display Settings
- **Resolution**: 1920x1080 (native projector resolution)
- **Refresh Rate**: Auto-detection with whitelist
- **Color Space**: Full range RGB for projector compatibility
- **Aspect Ratio**: 16:9 with keystone correction support

### Remote Control Optimization
- **Navigation**: Optimized for IR remote control input
- **Mouse**: Disabled (IR remote only)
- **Timeouts**: Extended for remote control response times
- **Interface**: Large touch targets, clear visual feedback

### Network Configuration
- **Buffering**: Optimized for streaming content
- **Cache**: 20MB memory buffer for smooth playback
- **Timeout**: Extended timeouts for network sources
- **Protocols**: HTTP/HTTPS with proxy support

## Installation Process

### VM Testing (Development)
```bash
# Build VM with Kodi configuration
nix build .#hy300-vm

# Run VM for testing
./result/bin/run-hy300-vm

# Kodi will auto-start with HY300 configuration
```

### Hardware Deployment
```bash
# Install complete Kodi configuration
install-hy300-kodi-complete

# Manual startup (if needed)
hy300-kodi

# Service management
systemctl enable kodi-hy300
systemctl start kodi-hy300
```

## Directory Structure

### Kodi Configuration
```
/home/projector/.kodi/
├── userdata/
│   ├── guisettings.xml      # Main settings with Nimbus skin
│   ├── sources.xml          # Media source definitions
│   ├── advancedsettings.xml # Performance optimizations
│   └── playlists/           # User playlists
└── addons/
    ├── skin.nimbus/         # Nimbus skin files
    ├── plugin.video.fenlightam/
    ├── script.module.cocoscrapers/
    ├── plugin.video.hy300keystone/
    └── plugin.program.hy300wifi/
```

### Media Directories
```
/media/
├── videos/    # Local video content
├── music/     # Local audio content
└── pictures/  # Local images and photos
```

## Addon Functionality

### FenLightAM Features
- **Movies**: Popular, latest, top rated, now playing, upcoming
- **TV Shows**: Popular, airing today, on air, top rated
- **Search**: Content search functionality
- **Lists**: Personal watchlists and favorites
- **Quality**: Multiple resolution options (480p-4K)

### CocoScrapers Features
- **Web Scraping**: Robust content source discovery
- **Error Handling**: Graceful failure and retry mechanisms
- **Performance**: Optimized for projector hardware limitations
- **Compatibility**: Works with FenLightAM and other addons

### HY300 Keystone Plugin
- **Manual Adjustment**: Motor control with fine positioning
- **Digital Correction**: 4-corner keystoning
- **Auto Correction**: Accelerometer-based automatic adjustment
- **Profiles**: Save/load keystone configurations
- **Calibration**: Accelerometer calibration tools

### HY300 WiFi Plugin
- **Network Scanning**: WiFi network discovery
- **Connection**: WPA/WPA2 network authentication
- **Management**: Connection status and saved profiles
- **QR Codes**: Mobile device setup assistance
- **Remote Control**: Full navigation via IR remote

## Performance Optimizations

### Memory Management
- **Cache Size**: 20MB memory buffer for network content
- **Buffer Mode**: Aggressive buffering for smooth playback
- **Read Factor**: 4x read-ahead for network sources

### CPU/GPU Settings
- **Dirty Regions**: Algorithm 3 for efficient screen updates
- **VSync**: Enabled for tear-free video playback
- **Hardware Decoding**: Automatic codec selection

### Network Buffering
- **Internet Streams**: 4MB buffer for online content
- **LAN Streams**: 2MB buffer for local network content
- **DVD Content**: 2MB buffer for optical media

## Troubleshooting

### Common Issues

**Skin Not Loading**
```bash
# Reset to default skin
rm /home/projector/.kodi/userdata/guisettings.xml
# Restart Kodi - will use default skin
# Reinstall configuration
install-hy300-kodi-complete
```

**Addons Not Appearing**
```bash
# Check addon installation
ls /home/projector/.kodi/addons/
# Verify permissions
chown -R projector:projector /home/projector/.kodi
# Clear addon database
rm /home/projector/.kodi/userdata/Database/Addons*.db
```

**Network Buffering Issues**
```bash
# Check network performance
ping -c 5 8.8.8.8
# Increase buffer size in advancedsettings.xml
# Restart Kodi service
```

**Hardware Acceleration Not Working**
```bash
# Check VA-API support
vainfo
# Verify driver installation
lsmod | grep radeon
# Check Kodi logs
tail -f /home/projector/.kodi/temp/kodi.log
```

### Logs and Debugging
- **Kodi Logs**: `/home/projector/.kodi/temp/kodi.log`
- **Addon Logs**: Check Kodi log for addon-specific messages
- **System Logs**: `journalctl -u kodi-hy300 -f`
- **Debug Mode**: Enable in Settings → System → Logging

## Customization

### Skin Modifications
- Colors and themes in Nimbus skin settings
- Custom backgrounds and fanart
- Widget configuration for home screen
- Menu customization for projector use

### Addon Configuration
- **FenLightAM**: Quality preferences, search settings
- **CocoScrapers**: Scraper selection and timeouts
- **Keystone**: Sensitivity and calibration settings
- **WiFi**: Network preferences and security settings

### Advanced Settings
- Modify `/home/projector/.kodi/userdata/advancedsettings.xml`
- Hardware-specific optimizations
- Network and caching adjustments
- Audio and video processing tweaks

## Integration with HY300 Services

### Keystone Correction
- Motor control via `/usr/bin/hy300-keystone-control`
- Real-time adjustment during video playback
- Automatic correction based on accelerometer data
- Profile storage for different mounting positions

### WiFi Management
- Integration with NetworkManager
- FIFO control interface at `/run/wifi-setup-control`
- Status monitoring and connection management
- Mobile device pairing via QR codes

### Hardware Monitoring
- Temperature monitoring (CPU/GPU)
- Performance metrics collection
- Hardware status integration
- Automatic thermal management

## Future Enhancements

### Planned Features
- **Voice Control**: Integration with speech recognition
- **Mobile App**: Android/iOS remote control app
- **Cloud Sync**: Settings and profile synchronization
- **HDR Support**: High dynamic range content handling

### Hardware Integration
- **HDMI Input**: Live video input switching
- **USB Camera**: Video conferencing and content capture
- **Bluetooth**: Audio device pairing and control
- **Sensor Integration**: Environmental monitoring and adjustment

This configuration provides a complete, optimized Kodi setup specifically designed for the HY300 projector's unique requirements and capabilities.