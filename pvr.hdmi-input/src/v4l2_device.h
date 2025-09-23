#pragma once

#include "types.h"
#include <linux/videodev2.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>

namespace hdmi_pvr {

class V4L2Device {
public:
    explicit V4L2Device(const std::string& device_path = "/dev/video0");
    ~V4L2Device();

    // Device management
    bool Open();
    void Close();
    bool IsOpen() const { return m_fd >= 0; }

    // Device capabilities
    bool QueryCapabilities();
    bool SupportsVideoCapture() const { return m_supports_capture; }
    bool SupportsStreaming() const { return m_supports_streaming; }
    std::string GetDriverName() const { return m_driver_name; }
    std::string GetCardName() const { return m_card_name; }

    // Format management
    bool SetFormat(const VideoFormat& format);
    VideoFormat GetFormat() const;
    std::vector<VideoFormat> GetSupportedFormats();
    bool DetectInputFormat(VideoFormat& format);

    // Buffer management
    bool AllocateBuffers(uint32_t buffer_count);
    void DeallocateBuffers();
    uint32_t GetBufferCount() const { return m_buffer_count; }

    // Streaming control
    bool StartStreaming();
    bool StopStreaming();
    bool IsStreaming() const { return m_streaming; }

    // Frame capture
    bool CaptureFrame(VideoBuffer& buffer);
    bool QueueBuffer(uint32_t index);
    bool DequeueBuffer(uint32_t& index, uint64_t& timestamp);

    // Signal detection
    bool CheckSignalPresent();
    SignalStatus GetSignalStatus();

    // Settings
    bool SetInput(uint32_t input);
    uint32_t GetInput() const;
    std::vector<std::string> GetInputNames();

private:
    struct Buffer {
        void* start = nullptr;
        size_t length = 0;
        bool mapped = false;
    };

    // Device properties
    std::string m_device_path;
    int m_fd = -1;
    bool m_supports_capture = false;
    bool m_supports_streaming = false;
    std::string m_driver_name;
    std::string m_card_name;
    uint32_t m_driver_version = 0;

    // Format state
    VideoFormat m_current_format;
    std::vector<VideoFormat> m_supported_formats;

    // Buffer state
    std::vector<Buffer> m_buffers;
    uint32_t m_buffer_count = 0;
    std::atomic<bool> m_streaming{false};

    // Signal status
    mutable std::mutex m_signal_mutex;
    SignalStatus m_signal_status;
    std::chrono::steady_clock::time_point m_last_signal_check;

    // Internal helpers
    bool QueryFormat(uint32_t pixel_format, std::vector<VideoFormat>& formats);
    bool TestFormat(const VideoFormat& format);
    bool MapBuffers();
    void UnmapBuffers();
    bool UpdateSignalStatus();
    uint32_t V4L2PixelFormatToFourCC(uint32_t v4l2_format) const;
    uint32_t FourCCToV4L2PixelFormat(uint32_t fourcc) const;
};

} // namespace hdmi_pvr