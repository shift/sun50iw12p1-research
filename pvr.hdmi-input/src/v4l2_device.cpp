/*
 *  V4L2 Device Manager for HY300 HDMI Input
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "v4l2_device.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <thread>
#include <sys/select.h>

namespace hdmi_pvr {

V4L2Device::V4L2Device(const std::string& device_path)
    : m_device_path(device_path)
    , m_signal_status{}
{
    m_signal_status.last_update = std::chrono::steady_clock::now();
}

V4L2Device::~V4L2Device() {
    Close();
}

bool V4L2Device::Open() {
    if (IsOpen()) {
        return true;
    }

    // Open device with non-blocking I/O for better control
    m_fd = open(m_device_path.c_str(), O_RDWR | O_NONBLOCK);
    if (m_fd < 0) {
        return false;
    }

    // Query device capabilities
    if (!QueryCapabilities()) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // Verify required capabilities
    if (!m_supports_capture || !m_supports_streaming) {
        close(m_fd);
        m_fd = -1;
        return false;
    }

    return true;
}

void V4L2Device::Close() {
    if (!IsOpen()) {
        return;
    }

    // Stop streaming if active
    if (m_streaming) {
        StopStreaming();
    }

    // Deallocate buffers
    DeallocateBuffers();

    // Close device
    close(m_fd);
    m_fd = -1;

    // Reset state
    m_supports_capture = false;
    m_supports_streaming = false;
    m_driver_name.clear();
    m_card_name.clear();
    m_driver_version = 0;
    m_current_format = {};
    m_supported_formats.clear();
}

bool V4L2Device::QueryCapabilities() {
    if (!IsOpen()) {
        return false;
    }

    struct v4l2_capability cap = {};
    if (ioctl(m_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        return false;
    }

    // Store capability information
    m_driver_name = reinterpret_cast<const char*>(cap.driver);
    m_card_name = reinterpret_cast<const char*>(cap.card);
    m_driver_version = cap.version;

    // Check required capabilities
    m_supports_capture = (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) != 0;
    m_supports_streaming = (cap.capabilities & V4L2_CAP_STREAMING) != 0;

    return true;
}

bool V4L2Device::SetFormat(const VideoFormat& format) {
    if (!IsOpen()) {
        return false;
    }

    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = format.width;
    fmt.fmt.pix.height = format.height;
    fmt.fmt.pix.pixelformat = FourCCToV4L2PixelFormat(format.fourcc);
    fmt.fmt.pix.field = format.interlaced ? V4L2_FIELD_INTERLACED : V4L2_FIELD_NONE;

    if (ioctl(m_fd, VIDIOC_S_FMT, &fmt) < 0) {
        return false;
    }

    // Verify the format was set correctly
    VideoFormat actual_format;
    actual_format.width = fmt.fmt.pix.width;
    actual_format.height = fmt.fmt.pix.height;
    actual_format.fourcc = V4L2PixelFormatToFourCC(fmt.fmt.pix.pixelformat);
    actual_format.interlaced = (fmt.fmt.pix.field == V4L2_FIELD_INTERLACED);
    actual_format.fps = format.fps; // FPS is set separately

    // Set frame rate
    struct v4l2_streamparm param = {};
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.parm.capture.timeperframe.numerator = 1;
    param.parm.capture.timeperframe.denominator = format.fps;

    if (ioctl(m_fd, VIDIOC_S_PARM, &param) == 0) {
        actual_format.fps = param.parm.capture.timeperframe.denominator / 
                           param.parm.capture.timeperframe.numerator;
    }

    m_current_format = actual_format;
    return true;
}

VideoFormat V4L2Device::GetFormat() const {
    if (!IsOpen()) {
        return {};
    }

    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(m_fd, VIDIOC_G_FMT, &fmt) < 0) {
        return {};
    }

    VideoFormat format;
    format.width = fmt.fmt.pix.width;
    format.height = fmt.fmt.pix.height;
    format.fourcc = V4L2PixelFormatToFourCC(fmt.fmt.pix.pixelformat);
    format.interlaced = (fmt.fmt.pix.field == V4L2_FIELD_INTERLACED);

    // Get frame rate
    struct v4l2_streamparm param = {};
    param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_G_PARM, &param) == 0) {
        if (param.parm.capture.timeperframe.numerator > 0) {
            format.fps = param.parm.capture.timeperframe.denominator / 
                        param.parm.capture.timeperframe.numerator;
        }
    }

    return format;
}

std::vector<VideoFormat> V4L2Device::GetSupportedFormats() {
    if (!IsOpen()) {
        return {};
    }

    if (!m_supported_formats.empty()) {
        return m_supported_formats;
    }

    // Common pixel formats to check
    std::vector<uint32_t> pixel_formats = {
        V4L2_PIX_FMT_YUYV,
        V4L2_PIX_FMT_UYVY,
        V4L2_PIX_FMT_NV12,
        V4L2_PIX_FMT_NV21,
        V4L2_PIX_FMT_YUV420,
        V4L2_PIX_FMT_BGR24,
        V4L2_PIX_FMT_RGB24,
        V4L2_PIX_FMT_MJPEG
    };

    for (uint32_t pixel_format : pixel_formats) {
        QueryFormat(pixel_format, m_supported_formats);
    }

    return m_supported_formats;
}

bool V4L2Device::DetectInputFormat(VideoFormat& format) {
    if (!IsOpen()) {
        return false;
    }

    // Try to get current input standard
    v4l2_std_id std_id = 0;
    if (ioctl(m_fd, VIDIOC_G_STD, &std_id) == 0) {
        // Standard video format detected - use common HD standards
        if (std_id & V4L2_STD_525_60) {
            // NTSC-style 480p60
            format.width = 720;
            format.height = 480;
            format.fps = 60;
            format.interlaced = false;
        } else if (std_id & V4L2_STD_625_50) {
            // PAL-style 576p50
            format.width = 720;
            format.height = 576;
            format.fps = 50;
            format.interlaced = false;
        }
    }

    // If no standard detected, try DV timings for digital formats
    struct v4l2_dv_timings timings = {};
    if (ioctl(m_fd, VIDIOC_G_DV_TIMINGS, &timings) == 0) {
        if (timings.type == V4L2_DV_BT_656_1120) {
            const auto& bt = timings.bt;
            format.width = bt.width;
            format.height = bt.height;
            format.interlaced = (bt.interlaced == V4L2_DV_INTERLACED);
            
            // Calculate frame rate
            uint64_t pixclk = bt.pixelclock;
            uint32_t htotal = bt.width + bt.hfrontporch + bt.hsync + bt.hbackporch;
            uint32_t vtotal = bt.height + bt.vfrontporch + bt.vsync + bt.vbackporch;
            if (htotal > 0 && vtotal > 0) {
                format.fps = static_cast<uint32_t>(pixclk / (htotal * vtotal));
            }
        }
    }

    // Get current pixel format
    VideoFormat current = GetFormat();
    if (current.is_valid()) {
        format.fourcc = current.fourcc;
        if (format.width == 0) format.width = current.width;
        if (format.height == 0) format.height = current.height;
        if (format.fps == 0) format.fps = current.fps;
    }

    return format.is_valid();
}

bool V4L2Device::AllocateBuffers(uint32_t buffer_count) {
    if (!IsOpen() || buffer_count == 0) {
        return false;
    }

    // Deallocate existing buffers first
    DeallocateBuffers();

    // Request buffers from driver
    struct v4l2_requestbuffers req = {};
    req.count = buffer_count;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_REQBUFS, &req) < 0) {
        return false;
    }

    if (req.count == 0) {
        return false;
    }

    // Allocate buffer structures
    m_buffers.resize(req.count);
    m_buffer_count = req.count;

    // Map buffers to userspace
    if (!MapBuffers()) {
        DeallocateBuffers();
        return false;
    }

    return true;
}

void V4L2Device::DeallocateBuffers() {
    // Unmap buffers
    UnmapBuffers();

    // Release buffers
    if (IsOpen() && m_buffer_count > 0) {
        struct v4l2_requestbuffers req = {};
        req.count = 0;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        ioctl(m_fd, VIDIOC_REQBUFS, &req);
    }

    m_buffers.clear();
    m_buffer_count = 0;
}

bool V4L2Device::StartStreaming() {
    if (!IsOpen() || m_streaming || m_buffer_count == 0) {
        return false;
    }

    // Queue all buffers
    for (uint32_t i = 0; i < m_buffer_count; ++i) {
        if (!QueueBuffer(i)) {
            return false;
        }
    }

    // Start streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMON, &type) < 0) {
        return false;
    }

    m_streaming = true;
    return true;
}

bool V4L2Device::StopStreaming() {
    if (!IsOpen() || !m_streaming) {
        return true;
    }

    // Stop streaming
    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd, VIDIOC_STREAMOFF, &type) < 0) {
        return false;
    }

    m_streaming = false;
    return true;
}

bool V4L2Device::CaptureFrame(VideoBuffer& buffer) {
    if (!IsOpen() || !m_streaming) {
        return false;
    }

    // Wait for frame with timeout
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    struct timeval timeout = {1, 0}; // 1 second timeout
    int ret = select(m_fd + 1, &fds, nullptr, nullptr, &timeout);
    
    if (ret <= 0) {
        return false; // Timeout or error
    }

    // Dequeue buffer
    uint32_t index;
    uint64_t timestamp;
    if (!DequeueBuffer(index, timestamp)) {
        return false;
    }

    // Copy frame data
    if (index < m_buffers.size() && m_buffers[index].mapped) {
        size_t frame_size = m_buffers[index].length;
        
        // Ensure buffer is large enough
        if (buffer.size < frame_size) {
            if (buffer.data) {
                free(buffer.data);
            }
            buffer.data = aligned_alloc(4096, frame_size);
            buffer.size = frame_size;
        }

        if (buffer.data) {
            memcpy(buffer.data, m_buffers[index].start, frame_size);
            buffer.timestamp = timestamp;
            buffer.in_use = true;
        }
    }

    // Queue buffer back for next capture
    QueueBuffer(index);

    return buffer.data != nullptr;
}

bool V4L2Device::QueueBuffer(uint32_t index) {
    if (!IsOpen() || index >= m_buffer_count) {
        return false;
    }

    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    return ioctl(m_fd, VIDIOC_QBUF, &buf) >= 0;
}

bool V4L2Device::DequeueBuffer(uint32_t& index, uint64_t& timestamp) {
    if (!IsOpen()) {
        return false;
    }

    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(m_fd, VIDIOC_DQBUF, &buf) < 0) {
        return false;
    }

    index = buf.index;
    timestamp = buf.timestamp.tv_sec * 1000000ULL + buf.timestamp.tv_usec;
    
    return true;
}

bool V4L2Device::CheckSignalPresent() {
    std::lock_guard<std::mutex> lock(m_signal_mutex);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_signal_check);
    
    // Update signal status every 500ms
    if (elapsed.count() > 500) {
        UpdateSignalStatus();
        m_last_signal_check = now;
    }
    
    return m_signal_status.connected && m_signal_status.signal_locked;
}

SignalStatus V4L2Device::GetSignalStatus() {
    std::lock_guard<std::mutex> lock(m_signal_mutex);
    UpdateSignalStatus();
    return m_signal_status;
}

bool V4L2Device::SetInput(uint32_t input) {
    if (!IsOpen()) {
        return false;
    }

    int inp = static_cast<int>(input);
    return ioctl(m_fd, VIDIOC_S_INPUT, &inp) >= 0;
}

uint32_t V4L2Device::GetInput() const {
    if (!IsOpen()) {
        return 0;
    }

    int input = 0;
    if (ioctl(m_fd, VIDIOC_G_INPUT, &input) < 0) {
        return 0;
    }

    return static_cast<uint32_t>(input);
}

std::vector<std::string> V4L2Device::GetInputNames() {
    std::vector<std::string> names;
    
    if (!IsOpen()) {
        return names;
    }

    for (uint32_t i = 0; i < 16; ++i) { // Check up to 16 inputs
        struct v4l2_input input = {};
        input.index = i;
        
        if (ioctl(m_fd, VIDIOC_ENUMINPUT, &input) < 0) {
            break;
        }
        
        names.emplace_back(reinterpret_cast<const char*>(input.name));
    }

    return names;
}

// Private helper methods

bool V4L2Device::QueryFormat(uint32_t pixel_format, std::vector<VideoFormat>& formats) {
    if (!IsOpen()) {
        return false;
    }

    // Common resolutions to test
    std::vector<std::pair<uint32_t, uint32_t>> resolutions = {
        {640, 480}, {720, 480}, {720, 576},    // SD
        {1280, 720},                           // HD
        {1920, 1080},                          // Full HD
        {3840, 2160}                           // 4K
    };

    std::vector<uint32_t> frame_rates = {24, 25, 30, 50, 60};

    for (const auto& res : resolutions) {
        for (uint32_t fps : frame_rates) {
            VideoFormat format;
            format.width = res.first;
            format.height = res.second;
            format.fps = fps;
            format.fourcc = V4L2PixelFormatToFourCC(pixel_format);
            format.interlaced = false;

            if (TestFormat(format)) {
                formats.push_back(format);
                
                // Also test interlaced version for applicable formats
                if (fps <= 30) {
                    format.interlaced = true;
                    if (TestFormat(format)) {
                        formats.push_back(format);
                    }
                }
            }
        }
    }

    return !formats.empty();
}

bool V4L2Device::TestFormat(const VideoFormat& format) {
    if (!IsOpen()) {
        return false;
    }

    struct v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = format.width;
    fmt.fmt.pix.height = format.height;
    fmt.fmt.pix.pixelformat = FourCCToV4L2PixelFormat(format.fourcc);
    fmt.fmt.pix.field = format.interlaced ? V4L2_FIELD_INTERLACED : V4L2_FIELD_NONE;

    // Use VIDIOC_TRY_FMT to test without changing current format
    return ioctl(m_fd, VIDIOC_TRY_FMT, &fmt) >= 0;
}

bool V4L2Device::MapBuffers() {
    if (!IsOpen() || m_buffer_count == 0) {
        return false;
    }

    for (uint32_t i = 0; i < m_buffer_count; ++i) {
        struct v4l2_buffer buf = {};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(m_fd, VIDIOC_QUERYBUF, &buf) < 0) {
            return false;
        }

        m_buffers[i].length = buf.length;
        m_buffers[i].start = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE,
                                 MAP_SHARED, m_fd, buf.m.offset);

        if (m_buffers[i].start == MAP_FAILED) {
            m_buffers[i].start = nullptr;
            return false;
        }

        m_buffers[i].mapped = true;
    }

    return true;
}

void V4L2Device::UnmapBuffers() {
    for (auto& buffer : m_buffers) {
        if (buffer.mapped && buffer.start) {
            munmap(buffer.start, buffer.length);
            buffer.start = nullptr;
            buffer.mapped = false;
        }
    }
}

bool V4L2Device::UpdateSignalStatus() {
    if (!IsOpen()) {
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    m_signal_status.last_update = now;

    // Check input status
    uint32_t current_input = GetInput();
    struct v4l2_input input = {};
    input.index = current_input;

    if (ioctl(m_fd, VIDIOC_ENUMINPUT, &input) >= 0) {
        m_signal_status.connected = (input.status & V4L2_IN_ST_NO_SIGNAL) == 0;
        m_signal_status.signal_locked = (input.status & V4L2_IN_ST_NO_SYNC) == 0;
        
        // Estimate signal quality based on status flags
        uint8_t quality = 100;
        if (input.status & V4L2_IN_ST_NO_H_LOCK) quality -= 25;
        if (input.status & V4L2_IN_ST_NO_V_LOCK) quality -= 25;
        if (input.status & V4L2_IN_ST_NO_STD_LOCK) quality -= 25;
        if (input.status & V4L2_IN_ST_NO_SYNC) quality -= 25;
        
        m_signal_status.signal_quality = m_signal_status.connected ? quality : 0;
        m_signal_status.signal_strength = m_signal_status.connected ? 85 : 0; // Estimate
    }

    // Get current video format for signal status
    if (m_signal_status.connected) {
        VideoFormat current_format;
        if (DetectInputFormat(current_format)) {
            m_signal_status.video_format = current_format;
        }
        
        // Set device name from EDID if available
        // This would require additional EDID parsing which depends on driver support
        m_signal_status.device_name = "HDMI Input Device";
    } else {
        m_signal_status.video_format = {};
        m_signal_status.device_name.clear();
    }

    return true;
}

uint32_t V4L2Device::V4L2PixelFormatToFourCC(uint32_t v4l2_format) const {
    // V4L2 pixel formats are already FourCC values in most cases
    return v4l2_format;
}

uint32_t V4L2Device::FourCCToV4L2PixelFormat(uint32_t fourcc) const {
    // FourCC values map directly to V4L2 pixel formats in most cases
    // Handle special cases if needed
    switch (fourcc) {
        case 0: // Default format
            return V4L2_PIX_FMT_YUYV;
        default:
            return fourcc;
    }
}

} // namespace hdmi_pvr
