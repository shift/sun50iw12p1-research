#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace hdmi_pvr {

// Video format structure
struct VideoFormat {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t fps = 0;
    uint32_t fourcc = 0;
    bool interlaced = false;
    
    bool is_valid() const {
        return width > 0 && height > 0 && fps > 0;
    }
    
    std::string to_string() const {
        return std::to_string(width) + "x" + std::to_string(height) + 
               (interlaced ? "i" : "p") + "@" + std::to_string(fps);
    }
};

// Audio format structure
struct AudioFormat {
    uint32_t sample_rate = 48000;
    uint32_t channels = 2;
    uint32_t bit_depth = 16;
    bool compressed = false;
    
    bool is_valid() const {
        return sample_rate > 0 && channels > 0 && bit_depth > 0;
    }
};

// HDMI signal status
struct SignalStatus {
    bool connected = false;
    bool signal_locked = false;
    uint8_t signal_strength = 0;  // 0-100%
    uint8_t signal_quality = 0;   // 0-100%
    VideoFormat video_format;
    AudioFormat audio_format;
    std::string device_name;      // From EDID
    std::chrono::steady_clock::time_point last_update;
    
    bool is_stable() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_update);
        return connected && signal_locked && elapsed.count() < 5;
    }
};

// Video buffer for streaming
struct VideoBuffer {
    void* data = nullptr;
    size_t size = 0;
    uint64_t timestamp = 0;
    bool in_use = false;
    
    VideoBuffer() = default;
    VideoBuffer(size_t buffer_size) : size(buffer_size) {
        data = aligned_alloc(4096, size);  // Page-aligned for DMA
    }
    
    ~VideoBuffer() {
        if (data) {
            free(data);
            data = nullptr;
        }
    }
    
    // Move constructor
    VideoBuffer(VideoBuffer&& other) noexcept 
        : data(other.data), size(other.size), 
          timestamp(other.timestamp), in_use(other.in_use) {
        other.data = nullptr;
        other.size = 0;
    }
    
    // Move assignment
    VideoBuffer& operator=(VideoBuffer&& other) noexcept {
        if (this != &other) {
            if (data) free(data);
            data = other.data;
            size = other.size;
            timestamp = other.timestamp;
            in_use = other.in_use;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
    
    // Disable copy
    VideoBuffer(const VideoBuffer&) = delete;
    VideoBuffer& operator=(const VideoBuffer&) = delete;
};

// Audio buffer for streaming
struct AudioBuffer {
    void* data = nullptr;
    size_t size = 0;
    uint64_t timestamp = 0;
    bool in_use = false;
    
    AudioBuffer() = default;
    AudioBuffer(size_t buffer_size) : size(buffer_size) {
        data = malloc(size);
    }
    
    ~AudioBuffer() {
        if (data) {
            free(data);
            data = nullptr;
        }
    }
    
    // Move constructor
    AudioBuffer(AudioBuffer&& other) noexcept 
        : data(other.data), size(other.size), 
          timestamp(other.timestamp), in_use(other.in_use) {
        other.data = nullptr;
        other.size = 0;
    }
    
    // Move assignment
    AudioBuffer& operator=(AudioBuffer&& other) noexcept {
        if (this != &other) {
            if (data) free(data);
            data = other.data;
            size = other.size;
            timestamp = other.timestamp;
            in_use = other.in_use;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
    
    // Disable copy
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;
};

// Channel information
struct ChannelInfo {
    uint32_t channel_id = 1;
    std::string channel_name = "HDMI Input";
    std::string channel_icon;
    bool preview_enabled = true;
    bool radio = false;
    uint32_t channel_number = 1;
    uint32_t sub_channel_number = 0;
    std::string encryption_name;
    bool is_hidden = false;
};

// EPG entry for HDMI input
struct EpgEntry {
    uint32_t broadcast_id = 1;
    uint32_t channel_id = 1;
    std::string title = "External Device Input";
    std::string plot = "Live HDMI input from connected device";
    std::string genre = "Input Source";
    time_t start_time;
    time_t end_time;
    uint32_t unique_id = 1;
    
    EpgEntry() {
        auto now = std::chrono::system_clock::now();
        start_time = std::chrono::system_clock::to_time_t(now);
        end_time = start_time + (24 * 3600);  // 24 hours
    }
};

// Stream properties
struct StreamProperties {
    uint32_t stream_id = 0;
    std::string codec_name;
    std::string language = "und";
    uint32_t identifier = 0;
    uint32_t bandwidth = 0;
    
    // Video properties
    uint32_t width = 0;
    uint32_t height = 0;
    double fps = 0.0;
    uint32_t bitrate = 0;
    
    // Audio properties
    uint32_t channels = 0;
    uint32_t sample_rate = 0;
    uint32_t bits_per_sample = 0;
};

} // namespace hdmi_pvr