/*
 *  HDMI Input PVR Client for HY300 Projector
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "types.h"
#include "v4l2_device.h"
#include <kodi/addon-instance/PVR.h>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>

namespace hdmi_pvr {

/**
 * StreamProcessor handles real-time video/audio stream processing for HDMI input.
 * 
 * This class provides:
 * - Real-time stream capture from V4L2 HDMI devices
 * - Hardware-accelerated demuxing support
 * - Thread-safe buffer management with RAII patterns
 * - Live stream reading for Kodi PVR integration
 * - Comprehensive error recovery and signal monitoring
 * 
 * The streaming pipeline:
 * 1. V4L2Device captures raw frames from HDMI input
 * 2. StreamProcessor manages buffer pools and threading
 * 3. Demux operations provide hardware-accelerated stream parsing
 * 4. Kodi PVR reads processed stream data for playback
 */
class StreamProcessor {
public:
    /**
     * Constructor
     * @param v4l2_device Pointer to V4L2Device for HDMI capture (can be nullptr for delayed initialization)
     */
    explicit StreamProcessor(V4L2Device* v4l2_device = nullptr);
    
    /**
     * Destructor - ensures clean shutdown
     */
    ~StreamProcessor();

    // Disable copy operations for thread safety
    StreamProcessor(const StreamProcessor&) = delete;
    StreamProcessor& operator=(const StreamProcessor&) = delete;

    //
    // Initialization and lifecycle management
    //

    /**
     * Initialize the stream processor
     * @return true if initialization successful
     */
    bool Initialize();

    /**
     * Shutdown the stream processor and cleanup resources
     */
    void Shutdown();

    /**
     * Check if processor is initialized
     * @return true if initialized and ready
     */
    bool IsInitialized() const { return m_initialized.load(); }

    /**
     * Set the V4L2 device for capture (can be changed during runtime)
     * @param v4l2_device Pointer to V4L2Device
     * @return true if device set successfully
     */
    bool SetV4L2Device(V4L2Device* v4l2_device);

    //
    // Stream operations
    //

    /**
     * Start streaming with specified formats
     * @param video_fmt Video format to capture
     * @param audio_fmt Audio format to capture
     * @return true if streaming started successfully
     */
    bool StartStreaming(const VideoFormat& video_fmt, const AudioFormat& audio_fmt);

    /**
     * Stop streaming and cleanup buffers
     */
    void StopStreaming();

    /**
     * Check if currently streaming
     * @return true if streaming active
     */
    bool IsStreaming() const { return m_streaming.load(); }

    //
    // Data reading for Kodi PVR
    //

    /**
     * Read live stream data for Kodi PVR
     * @param buffer Buffer to fill with stream data
     * @param size Maximum size to read
     * @return Number of bytes read, -1 on error, 0 if no data available
     */
    int ReadLiveStream(unsigned char* buffer, unsigned int size);

    /**
     * Get stream properties for Kodi PVR
     * @param properties Vector to fill with stream properties
     * @return PVR_ERROR_NO_ERROR on success
     */
    PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties);

    //
    // Demux operations for hardware acceleration
    //

    /**
     * Open demux stream for hardware-accelerated processing
     * @return true if demux stream opened successfully
     */
    bool OpenDemuxStream();

    /**
     * Close demux stream
     */
    void CloseDemuxStream();

    /**
     * Read demux packet (hardware-accelerated)
     * @return Pointer to DEMUX_PACKET or nullptr if no data/error
     */
    DEMUX_PACKET* DemuxRead();

    /**
     * Abort demux operations
     */
    void DemuxAbort();

    /**
     * Flush demux buffers
     */
    void DemuxFlush();

    /**
     * Reset demux state
     */
    void DemuxReset();

    //
    // Stream status and information
    //

    /**
     * Get current stream formats
     * @param video_fmt Reference to fill with video format
     * @param audio_fmt Reference to fill with audio format
     * @return true if formats retrieved successfully
     */
    bool GetCurrentFormat(VideoFormat& video_fmt, AudioFormat& audio_fmt);

    /**
     * Get current stream bitrate
     * @return Stream bitrate in bits per second
     */
    uint64_t GetStreamBitrate() const { return m_stream_bitrate.load(); }

    /**
     * Check if HDMI signal is present
     * @return true if signal detected and stable
     */
    bool IsSignalPresent() const;

    //
    // Buffer management
    //

    /**
     * Set buffer parameters for streaming
     * @param buffer_count Number of buffers to allocate
     * @param buffer_size Size of each buffer in bytes
     * @return true if parameters set successfully
     */
    bool SetBufferParameters(uint32_t buffer_count, uint32_t buffer_size);

    /**
     * Get buffer statistics
     * @param total_buffers Total number of allocated buffers
     * @param used_buffers Number of buffers currently in use
     * @param dropped_frames Number of frames dropped due to buffer overflow
     */
    void GetBufferStatistics(uint32_t& total_buffers, uint32_t& used_buffers, uint32_t& dropped_frames);

private:
    //
    // Internal data structures
    //

    /**
     * Stream buffer for internal processing
     */
    struct StreamBuffer {
        std::unique_ptr<uint8_t[]> data;
        size_t size = 0;
        size_t capacity = 0;
        uint64_t timestamp = 0;
        bool in_use = false;
        
        StreamBuffer() = default;
        explicit StreamBuffer(size_t buffer_size);
        ~StreamBuffer() = default;
        
        // Move semantics for efficient buffer management
        StreamBuffer(StreamBuffer&& other) noexcept;
        StreamBuffer& operator=(StreamBuffer&& other) noexcept;
        
        // Disable copy
        StreamBuffer(const StreamBuffer&) = delete;
        StreamBuffer& operator=(const StreamBuffer&) = delete;
        
        bool Allocate(size_t buffer_size);
        void Reset();
    };

    /**
     * Buffer pool for efficient memory management
     */
    class BufferPool {
    public:
        explicit BufferPool(size_t buffer_count, size_t buffer_size);
        ~BufferPool() = default;
        
        StreamBuffer* GetBuffer();
        void ReturnBuffer(StreamBuffer* buffer);
        void Clear();
        
        size_t GetTotalBuffers() const { return m_buffers.size(); }
        size_t GetUsedBuffers() const;
        
    private:
        std::vector<std::unique_ptr<StreamBuffer>> m_buffers;
        std::queue<StreamBuffer*> m_available_buffers;
        mutable std::mutex m_mutex;
    };

    //
    // Device and state management
    //

    V4L2Device* m_v4l2_device = nullptr;  ///< V4L2 device for capture (not owned)
    std::atomic<bool> m_initialized{false};  ///< Initialization status
    std::atomic<bool> m_streaming{false};  ///< Streaming status
    std::atomic<bool> m_demux_open{false};  ///< Demux stream status

    //
    // Format and stream properties
    //

    mutable std::mutex m_format_mutex;
    VideoFormat m_current_video_format;  ///< Current video format
    AudioFormat m_current_audio_format;  ///< Current audio format
    std::atomic<uint64_t> m_stream_bitrate{0};  ///< Current stream bitrate

    //
    // Buffer management
    //

    std::unique_ptr<BufferPool> m_buffer_pool;
    std::queue<StreamBuffer*> m_ready_buffers;  ///< Buffers ready for reading
    mutable std::mutex m_buffer_mutex;
    std::condition_variable m_buffer_condition;

    uint32_t m_buffer_count = 8;  ///< Number of buffers to allocate
    uint32_t m_buffer_size = 1024 * 1024;  ///< Size of each buffer (1MB default)
    std::atomic<uint32_t> m_dropped_frames{0};  ///< Frame drop counter

    //
    // Threading
    //

    std::unique_ptr<std::thread> m_capture_thread;
    std::atomic<bool> m_capture_thread_running{false};
    std::condition_variable m_capture_condition;

    //
    // Demux support
    //

    std::queue<std::unique_ptr<DEMUX_PACKET>> m_demux_packets;
    mutable std::mutex m_demux_mutex;
    std::condition_variable m_demux_condition;
    std::atomic<bool> m_demux_abort{false};

    //
    // Statistics and monitoring
    //

    std::chrono::steady_clock::time_point m_stream_start_time;
    std::atomic<uint64_t> m_total_bytes_processed{0};
    std::atomic<uint64_t> m_total_frames_processed{0};

    //
    // Internal methods
    //

    /**
     * Main capture thread function
     */
    void CaptureThreadFunction();

    /**
     * Process captured frame from V4L2
     * @param v4l2_buffer V4L2 buffer containing frame data
     * @param timestamp Frame timestamp
     * @return true if frame processed successfully
     */
    bool ProcessCapturedFrame(const VideoBuffer& v4l2_buffer, uint64_t timestamp);

    /**
     * Create demux packet from stream buffer
     * @param stream_buffer Source stream buffer
     * @return Unique pointer to DEMUX_PACKET or nullptr on error
     */
    std::unique_ptr<DEMUX_PACKET> CreateDemuxPacket(const StreamBuffer& stream_buffer);

    /**
     * Update stream bitrate calculation
     * @param bytes_processed Number of bytes processed since last update
     */
    void UpdateBitrate(size_t bytes_processed);

    /**
     * Validate video format compatibility
     * @param format Video format to validate
     * @return true if format is supported
     */
    bool ValidateVideoFormat(const VideoFormat& format) const;

    /**
     * Validate audio format compatibility
     * @param format Audio format to validate
     * @return true if format is supported
     */
    bool ValidateAudioFormat(const AudioFormat& format) const;

    /**
     * Cleanup internal resources
     */
    void CleanupResources();
};

} // namespace hdmi_pvr