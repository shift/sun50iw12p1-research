/*
 *  HDMI Input PVR Client for HY300 Projector
 *  Stream Processor Implementation
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stream_processor.h"
#include "v4l2_device.h"
#include <kodi/General.h>
#include <algorithm>
#include <cstring>

namespace hdmi_pvr {

//
// StreamBuffer implementation
//

StreamProcessor::StreamBuffer::StreamBuffer(size_t buffer_size) {
    Allocate(buffer_size);
}

StreamProcessor::StreamBuffer::StreamBuffer(StreamBuffer&& other) noexcept 
    : data(std::move(other.data))
    , size(other.size)
    , capacity(other.capacity)
    , timestamp(other.timestamp)
    , in_use(other.in_use) {
    other.size = 0;
    other.capacity = 0;
    other.timestamp = 0;
    other.in_use = false;
}

StreamProcessor::StreamBuffer& StreamProcessor::StreamBuffer::operator=(StreamBuffer&& other) noexcept {
    if (this != &other) {
        data = std::move(other.data);
        size = other.size;
        capacity = other.capacity;
        timestamp = other.timestamp;
        in_use = other.in_use;
        
        other.size = 0;
        other.capacity = 0;
        other.timestamp = 0;
        other.in_use = false;
    }
    return *this;
}

bool StreamProcessor::StreamBuffer::Allocate(size_t buffer_size) {
    try {
        data = std::make_unique<uint8_t[]>(buffer_size);
        capacity = buffer_size;
        size = 0;
        in_use = false;
        return true;
    } catch (const std::bad_alloc&) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to allocate stream buffer of size %zu", buffer_size);
        return false;
    }
}

void StreamProcessor::StreamBuffer::Reset() {
    size = 0;
    timestamp = 0;
    in_use = false;
}

//
// BufferPool implementation
//

StreamProcessor::BufferPool::BufferPool(size_t buffer_count, size_t buffer_size) {
    m_buffers.reserve(buffer_count);
    
    for (size_t i = 0; i < buffer_count; ++i) {
        auto buffer = std::make_unique<StreamBuffer>(buffer_size);
        if (buffer && buffer->data) {
            m_available_buffers.push(buffer.get());
            m_buffers.push_back(std::move(buffer));
        }
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "BufferPool initialized with %zu buffers of size %zu", 
              m_buffers.size(), buffer_size);
}

StreamProcessor::StreamBuffer* StreamProcessor::BufferPool::GetBuffer() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_available_buffers.empty()) {
        return nullptr;
    }
    
    StreamBuffer* buffer = m_available_buffers.front();
    m_available_buffers.pop();
    buffer->Reset();
    buffer->in_use = true;
    
    return buffer;
}

void StreamProcessor::BufferPool::ReturnBuffer(StreamBuffer* buffer) {
    if (!buffer) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    buffer->Reset();
    buffer->in_use = false;
    m_available_buffers.push(buffer);
}

void StreamProcessor::BufferPool::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Clear available buffer queue
    std::queue<StreamBuffer*> empty;
    m_available_buffers.swap(empty);
    
    // Reset all buffers and make them available
    for (auto& buffer : m_buffers) {
        if (buffer) {
            buffer->Reset();
            buffer->in_use = false;
            m_available_buffers.push(buffer.get());
        }
    }
}

size_t StreamProcessor::BufferPool::GetUsedBuffers() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_buffers.size() - m_available_buffers.size();
}

//
// StreamProcessor main implementation
//

StreamProcessor::StreamProcessor(V4L2Device* v4l2_device)
    : m_v4l2_device(v4l2_device) {
    kodi::Log(ADDON_LOG_DEBUG, "StreamProcessor created");
}

StreamProcessor::~StreamProcessor() {
    kodi::Log(ADDON_LOG_DEBUG, "StreamProcessor destructor called");
    Shutdown();
}

bool StreamProcessor::Initialize() {
    if (m_initialized.load()) {
        kodi::Log(ADDON_LOG_WARNING, "StreamProcessor already initialized");
        return true;
    }
    
    if (!m_v4l2_device) {
        kodi::Log(ADDON_LOG_ERROR, "No V4L2 device set for stream processor");
        return false;
    }
    
    // Initialize buffer pool
    m_buffer_pool = std::make_unique<BufferPool>(m_buffer_count, m_buffer_size);
    if (!m_buffer_pool || m_buffer_pool->GetTotalBuffers() == 0) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to initialize buffer pool");
        return false;
    }
    
    m_initialized.store(true);
    kodi::Log(ADDON_LOG_INFO, "StreamProcessor initialized successfully");
    return true;
}

void StreamProcessor::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Shutting down StreamProcessor");
    
    // Stop streaming if active
    StopStreaming();
    
    // Close demux if open
    if (m_demux_open.load()) {
        CloseDemuxStream();
    }
    
    // Clean up resources
    CleanupResources();
    
    m_initialized.store(false);
    kodi::Log(ADDON_LOG_INFO, "StreamProcessor shutdown complete");
}

bool StreamProcessor::SetV4L2Device(V4L2Device* v4l2_device) {
    if (m_streaming.load()) {
        kodi::Log(ADDON_LOG_ERROR, "Cannot change V4L2 device while streaming");
        return false;
    }
    
    m_v4l2_device = v4l2_device;
    kodi::Log(ADDON_LOG_DEBUG, "V4L2 device set");
    return true;
}

bool StreamProcessor::StartStreaming(const VideoFormat& video_fmt, const AudioFormat& audio_fmt) {
    if (!m_initialized.load()) {
        kodi::Log(ADDON_LOG_ERROR, "StreamProcessor not initialized");
        return false;
    }
    
    if (m_streaming.load()) {
        kodi::Log(ADDON_LOG_WARNING, "Already streaming, stopping current stream first");
        StopStreaming();
    }
    
    if (!m_v4l2_device) {
        kodi::Log(ADDON_LOG_ERROR, "No V4L2 device available for streaming");
        return false;
    }
    
    // Validate formats
    if (!ValidateVideoFormat(video_fmt)) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid video format for streaming");
        return false;
    }
    
    if (!ValidateAudioFormat(audio_fmt)) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid audio format for streaming");
        return false;
    }
    
    // Store current formats
    {
        std::lock_guard<std::mutex> lock(m_format_mutex);
        m_current_video_format = video_fmt;
        m_current_audio_format = audio_fmt;
    }
    
    // Start V4L2 streaming
    if (!m_v4l2_device->StartStreaming()) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to start V4L2 streaming");
        return false;
    }
    
    // Reset statistics
    m_stream_start_time = std::chrono::steady_clock::now();
    m_total_bytes_processed.store(0);
    m_total_frames_processed.store(0);
    m_dropped_frames.store(0);
    m_stream_bitrate.store(0);
    
    // Clear buffer pool
    m_buffer_pool->Clear();
    
    // Start capture thread
    m_capture_thread_running.store(true);
    m_capture_thread = std::make_unique<std::thread>(&StreamProcessor::CaptureThreadFunction, this);
    
    m_streaming.store(true);
    kodi::Log(ADDON_LOG_INFO, "Streaming started - Video: %dx%d, Audio: %dHz", 
              video_fmt.width, video_fmt.height, audio_fmt.sample_rate);
    
    return true;
}

void StreamProcessor::StopStreaming() {
    if (!m_streaming.load()) {
        return;
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Stopping streaming");
    
    // Signal capture thread to stop
    m_capture_thread_running.store(false);
    m_capture_condition.notify_all();
    
    // Wait for capture thread to finish
    if (m_capture_thread && m_capture_thread->joinable()) {
        m_capture_thread->join();
        m_capture_thread.reset();
    }
    
    // Stop V4L2 streaming
    if (m_v4l2_device) {
        m_v4l2_device->StopStreaming();
    }
    
    // Clear buffers
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        std::queue<StreamBuffer*> empty_queue;
        m_ready_buffers.swap(empty_queue);
    }
    m_buffer_condition.notify_all();
    
    m_streaming.store(false);
    kodi::Log(ADDON_LOG_INFO, "Streaming stopped");
}

int StreamProcessor::ReadLiveStream(unsigned char* buffer, unsigned int size) {
    if (!m_streaming.load()) {
        return -1;  // Error: not streaming
    }
    
    if (!buffer || size == 0) {
        return -1;  // Error: invalid parameters
    }
    
    // Wait for data with timeout
    std::unique_lock<std::mutex> lock(m_buffer_mutex);
    if (m_ready_buffers.empty()) {
        auto timeout = std::chrono::milliseconds(100);  // 100ms timeout
        if (!m_buffer_condition.wait_for(lock, timeout, [this]() {
            return !m_ready_buffers.empty() || !m_streaming.load();
        })) {
            return 0;  // Timeout: no data available
        }
    }
    
    if (!m_streaming.load()) {
        return -1;  // Error: streaming was stopped
    }
    
    if (m_ready_buffers.empty()) {
        return 0;  // No data available
    }
    
    // Get next buffer
    StreamBuffer* stream_buffer = m_ready_buffers.front();
    m_ready_buffers.pop();
    lock.unlock();
    
    if (!stream_buffer || !stream_buffer->data) {
        return -1;  // Error: invalid buffer
    }
    
    // Copy data to output buffer
    size_t bytes_to_copy = std::min(static_cast<size_t>(size), stream_buffer->size);
    std::memcpy(buffer, stream_buffer->data.get(), bytes_to_copy);
    
    // Return buffer to pool
    m_buffer_pool->ReturnBuffer(stream_buffer);
    
    return static_cast<int>(bytes_to_copy);
}

PVR_ERROR StreamProcessor::GetStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties) {
    if (!m_streaming.load()) {
        kodi::Log(ADDON_LOG_ERROR, "Cannot get stream properties: not streaming");
        return PVR_ERROR_FAILED;
    }
    
    std::lock_guard<std::mutex> lock(m_format_mutex);
    
    properties.clear();
    
    // Video properties
    if (m_current_video_format.width > 0 && m_current_video_format.height > 0) {
        kodi::addon::PVRStreamProperty video_codec;
        video_codec.SetName("codec_video");
        video_codec.SetValue("h264");  // Assume H.264 for HDMI input
        properties.push_back(video_codec);
        
        kodi::addon::PVRStreamProperty video_width;
        video_width.SetName("video_width");
        video_width.SetValue(std::to_string(m_current_video_format.width));
        properties.push_back(video_width);
        
        kodi::addon::PVRStreamProperty video_height;
        video_height.SetName("video_height");
        video_height.SetValue(std::to_string(m_current_video_format.height));
        properties.push_back(video_height);
        
        kodi::addon::PVRStreamProperty video_fps;
        video_fps.SetName("video_fps");
        video_fps.SetValue(std::to_string(m_current_video_format.framerate));
        properties.push_back(video_fps);
    }
    
    // Audio properties
    if (m_current_audio_format.sample_rate > 0) {
        kodi::addon::PVRStreamProperty audio_codec;
        audio_codec.SetName("codec_audio");
        audio_codec.SetValue("pcm");  // Assume PCM for HDMI audio
        properties.push_back(audio_codec);
        
        kodi::addon::PVRStreamProperty audio_channels;
        audio_channels.SetName("audio_channels");
        audio_channels.SetValue(std::to_string(m_current_audio_format.channels));
        properties.push_back(audio_channels);
        
        kodi::addon::PVRStreamProperty audio_samplerate;
        audio_samplerate.SetName("audio_samplerate");
        audio_samplerate.SetValue(std::to_string(m_current_audio_format.sample_rate));
        properties.push_back(audio_samplerate);
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Stream properties: %zu items", properties.size());
    return PVR_ERROR_NO_ERROR;
}

bool StreamProcessor::OpenDemuxStream() {
    if (m_demux_open.load()) {
        kodi::Log(ADDON_LOG_WARNING, "Demux stream already open");
        return true;
    }
    
    if (!m_streaming.load()) {
        kodi::Log(ADDON_LOG_ERROR, "Cannot open demux: not streaming");
        return false;
    }
    
    // Clear demux packet queue
    {
        std::lock_guard<std::mutex> lock(m_demux_mutex);
        std::queue<std::unique_ptr<DEMUX_PACKET>> empty_queue;
        m_demux_packets.swap(empty_queue);
    }
    
    m_demux_abort.store(false);
    m_demux_open.store(true);
    
    kodi::Log(ADDON_LOG_DEBUG, "Demux stream opened");
    return true;
}

void StreamProcessor::CloseDemuxStream() {
    if (!m_demux_open.load()) {
        return;
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Closing demux stream");
    
    m_demux_abort.store(true);
    m_demux_condition.notify_all();
    
    // Clear demux packet queue
    {
        std::lock_guard<std::mutex> lock(m_demux_mutex);
        std::queue<std::unique_ptr<DEMUX_PACKET>> empty_queue;
        m_demux_packets.swap(empty_queue);
    }
    
    m_demux_open.store(false);
    kodi::Log(ADDON_LOG_DEBUG, "Demux stream closed");
}

DEMUX_PACKET* StreamProcessor::DemuxRead() {
    if (!m_demux_open.load()) {
        return nullptr;
    }
    
    if (m_demux_abort.load()) {
        return nullptr;
    }
    
    std::unique_lock<std::mutex> lock(m_demux_mutex);
    
    // Wait for packet with timeout
    auto timeout = std::chrono::milliseconds(100);
    if (!m_demux_condition.wait_for(lock, timeout, [this]() {
        return !m_demux_packets.empty() || m_demux_abort.load();
    })) {
        return nullptr;  // Timeout
    }
    
    if (m_demux_abort.load() || m_demux_packets.empty()) {
        return nullptr;
    }
    
    auto packet = std::move(m_demux_packets.front());
    m_demux_packets.pop();
    
    return packet.release();
}

void StreamProcessor::DemuxAbort() {
    kodi::Log(ADDON_LOG_DEBUG, "Demux abort requested");
    m_demux_abort.store(true);
    m_demux_condition.notify_all();
}

void StreamProcessor::DemuxFlush() {
    kodi::Log(ADDON_LOG_DEBUG, "Demux flush requested");
    
    std::lock_guard<std::mutex> lock(m_demux_mutex);
    std::queue<std::unique_ptr<DEMUX_PACKET>> empty_queue;
    m_demux_packets.swap(empty_queue);
}

void StreamProcessor::DemuxReset() {
    kodi::Log(ADDON_LOG_DEBUG, "Demux reset requested");
    DemuxFlush();
}

bool StreamProcessor::GetCurrentFormat(VideoFormat& video_fmt, AudioFormat& audio_fmt) {
    std::lock_guard<std::mutex> lock(m_format_mutex);
    video_fmt = m_current_video_format;
    audio_fmt = m_current_audio_format;
    return m_streaming.load();
}

bool StreamProcessor::IsSignalPresent() const {
    if (!m_v4l2_device) {
        return false;
    }
    
    return m_v4l2_device->IsSignalPresent();
}

bool StreamProcessor::SetBufferParameters(uint32_t buffer_count, uint32_t buffer_size) {
    if (m_streaming.load()) {
        kodi::Log(ADDON_LOG_ERROR, "Cannot change buffer parameters while streaming");
        return false;
    }
    
    if (buffer_count == 0 || buffer_size == 0) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid buffer parameters: count=%u, size=%u", 
                  buffer_count, buffer_size);
        return false;
    }
    
    m_buffer_count = buffer_count;
    m_buffer_size = buffer_size;
    
    // Reinitialize buffer pool if already initialized
    if (m_initialized.load()) {
        m_buffer_pool = std::make_unique<BufferPool>(m_buffer_count, m_buffer_size);
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Buffer parameters set: count=%u, size=%u", 
              buffer_count, buffer_size);
    return true;
}

void StreamProcessor::GetBufferStatistics(uint32_t& total_buffers, uint32_t& used_buffers, 
                                         uint32_t& dropped_frames) {
    total_buffers = m_buffer_pool ? static_cast<uint32_t>(m_buffer_pool->GetTotalBuffers()) : 0;
    used_buffers = m_buffer_pool ? static_cast<uint32_t>(m_buffer_pool->GetUsedBuffers()) : 0;
    dropped_frames = m_dropped_frames.load();
}

//
// Private methods
//

void StreamProcessor::CaptureThreadFunction() {
    kodi::Log(ADDON_LOG_DEBUG, "Capture thread started");
    
    while (m_capture_thread_running.load()) {
        if (!m_v4l2_device) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // Capture frame from V4L2 device
        VideoBuffer v4l2_buffer;
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
            
        if (m_v4l2_device->CaptureFrame(v4l2_buffer, 100)) {  // 100ms timeout
            ProcessCapturedFrame(v4l2_buffer, timestamp);
        }
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Capture thread finished");
}

bool StreamProcessor::ProcessCapturedFrame(const VideoBuffer& v4l2_buffer, uint64_t timestamp) {
    if (!v4l2_buffer.data || v4l2_buffer.size == 0) {
        return false;
    }
    
    // Get buffer from pool
    StreamBuffer* stream_buffer = m_buffer_pool->GetBuffer();
    if (!stream_buffer) {
        // Buffer pool exhausted - drop frame
        m_dropped_frames.fetch_add(1);
        kodi::Log(ADDON_LOG_WARNING, "Dropped frame: buffer pool exhausted");
        return false;
    }
    
    // Ensure buffer capacity
    if (stream_buffer->capacity < v4l2_buffer.size) {
        if (!stream_buffer->Allocate(v4l2_buffer.size)) {
            m_buffer_pool->ReturnBuffer(stream_buffer);
            m_dropped_frames.fetch_add(1);
            return false;
        }
    }
    
    // Copy frame data
    std::memcpy(stream_buffer->data.get(), v4l2_buffer.data, v4l2_buffer.size);
    stream_buffer->size = v4l2_buffer.size;
    stream_buffer->timestamp = timestamp;
    
    // Add to ready buffer queue
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        m_ready_buffers.push(stream_buffer);
    }
    m_buffer_condition.notify_one();
    
    // Create demux packet if demux is open
    if (m_demux_open.load() && !m_demux_abort.load()) {
        auto demux_packet = CreateDemuxPacket(*stream_buffer);
        if (demux_packet) {
            std::lock_guard<std::mutex> lock(m_demux_mutex);
            m_demux_packets.push(std::move(demux_packet));
            m_demux_condition.notify_one();
        }
    }
    
    // Update statistics
    m_total_frames_processed.fetch_add(1);
    UpdateBitrate(v4l2_buffer.size);
    
    return true;
}

std::unique_ptr<DEMUX_PACKET> StreamProcessor::CreateDemuxPacket(const StreamBuffer& stream_buffer) {
    if (!stream_buffer.data || stream_buffer.size == 0) {
        return nullptr;
    }
    
    auto packet = std::make_unique<DEMUX_PACKET>();
    if (!packet) {
        return nullptr;
    }
    
    // Allocate packet data
    packet->pData = new uint8_t[stream_buffer.size];
    if (!packet->pData) {
        return nullptr;
    }
    
    // Copy data
    std::memcpy(packet->pData, stream_buffer.data.get(), stream_buffer.size);
    packet->iSize = static_cast<int>(stream_buffer.size);
    packet->pts = static_cast<double>(stream_buffer.timestamp);
    packet->dts = packet->pts;
    packet->duration = 0;  // Will be set by Kodi
    packet->iStreamId = 0;  // Video stream
    
    return packet;
}

void StreamProcessor::UpdateBitrate(size_t bytes_processed) {
    m_total_bytes_processed.fetch_add(bytes_processed);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_stream_start_time);
    
    if (elapsed.count() > 0) {
        uint64_t total_bytes = m_total_bytes_processed.load();
        uint64_t bitrate = (total_bytes * 8 * 1000) / elapsed.count();  // bits per second
        m_stream_bitrate.store(bitrate);
    }
}

bool StreamProcessor::ValidateVideoFormat(const VideoFormat& format) const {
    // Check for valid dimensions
    if (format.width == 0 || format.height == 0) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid video dimensions: %dx%d", format.width, format.height);
        return false;
    }
    
    // Check for reasonable dimensions (avoid extremely large buffers)
    if (format.width > 3840 || format.height > 2160) {
        kodi::Log(ADDON_LOG_WARNING, "Large video dimensions: %dx%d", format.width, format.height);
    }
    
    // Check framerate
    if (format.framerate <= 0 || format.framerate > 120) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid framerate: %d", format.framerate);
        return false;
    }
    
    return true;
}

bool StreamProcessor::ValidateAudioFormat(const AudioFormat& format) const {
    // Check sample rate
    if (format.sample_rate == 0) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid sample rate: %d", format.sample_rate);
        return false;
    }
    
    // Check channels
    if (format.channels == 0 || format.channels > 8) {
        kodi::Log(ADDON_LOG_ERROR, "Invalid channel count: %d", format.channels);
        return false;
    }
    
    return true;
}

void StreamProcessor::CleanupResources() {
    // Clear buffer pool
    m_buffer_pool.reset();
    
    // Clear ready buffer queue
    {
        std::lock_guard<std::mutex> lock(m_buffer_mutex);
        std::queue<StreamBuffer*> empty_queue;
        m_ready_buffers.swap(empty_queue);
    }
    
    // Clear demux packet queue
    {
        std::lock_guard<std::mutex> lock(m_demux_mutex);
        std::queue<std::unique_ptr<DEMUX_PACKET>> empty_queue;
        m_demux_packets.swap(empty_queue);
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Resources cleaned up");
}

} // namespace hdmi_pvr