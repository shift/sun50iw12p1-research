#pragma once

#include "types.h"
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>

namespace hdmi_pvr {

// Forward declaration
class V4L2Device;

/**
 * SignalMonitor class for HDMI signal detection and monitoring
 * 
 * Provides thread-safe monitoring of HDMI signal status including:
 * - Connection detection and hot-plug events
 * - Signal strength and quality measurement
 * - Format detection and change notification
 * - Real-time status updates with callback support
 */
class SignalMonitor {
public:
    // Callback function type for signal status changes
    using StatusCallback = std::function<void(const SignalStatus&)>;
    using HotPlugCallback = std::function<void(bool connected)>;

    /**
     * Constructor
     * @param v4l2_device Shared pointer to V4L2Device for hardware access
     */
    explicit SignalMonitor(std::shared_ptr<V4L2Device> v4l2_device);
    
    /**
     * Destructor - ensures clean shutdown of monitoring thread
     */
    ~SignalMonitor();

    // Disable copy construction and assignment
    SignalMonitor(const SignalMonitor&) = delete;
    SignalMonitor& operator=(const SignalMonitor&) = delete;

    // Move constructor and assignment
    SignalMonitor(SignalMonitor&& other) noexcept;
    SignalMonitor& operator=(SignalMonitor&& other) noexcept;

    /**
     * Initialize and start the signal monitoring thread
     * @return true if initialization successful, false otherwise
     */
    bool Initialize();

    /**
     * Shutdown the signal monitor and stop all monitoring activities
     */
    void Shutdown();

    /**
     * Check if the monitor is currently active
     * @return true if monitoring is active, false otherwise
     */
    bool IsActive() const { return m_active.load(); }

    /**
     * Get the current signal status (thread-safe)
     * @return Current SignalStatus structure
     */
    SignalStatus GetSignalStatus() const;

    /**
     * Force an immediate signal status update
     * @return true if update successful, false otherwise
     */
    bool UpdateSignalStatus();

    /**
     * Check if HDMI signal is currently connected
     * @return true if signal is connected, false otherwise
     */
    bool IsSignalConnected() const;

    /**
     * Check if HDMI signal is locked and stable
     * @return true if signal is locked, false otherwise
     */
    bool IsSignalLocked() const;

    /**
     * Get current signal strength percentage
     * @return Signal strength (0-100%)
     */
    uint8_t GetSignalStrength() const;

    /**
     * Get current signal quality percentage
     * @return Signal quality (0-100%)
     */
    uint8_t GetSignalQuality() const;

    /**
     * Get the current video format of the HDMI signal
     * @return VideoFormat structure with current format details
     */
    VideoFormat GetVideoFormat() const;

    /**
     * Get the current audio format of the HDMI signal
     * @return AudioFormat structure with current format details
     */
    AudioFormat GetAudioFormat() const;

    /**
     * Register a callback for signal status changes
     * @param callback Function to call when signal status changes
     */
    void SetStatusCallback(StatusCallback callback);

    /**
     * Register a callback for hot-plug events
     * @param callback Function to call when connection state changes
     */
    void SetHotPlugCallback(HotPlugCallback callback);

    /**
     * Set the monitoring update interval
     * @param interval_ms Update interval in milliseconds (default: 1000ms)
     */
    void SetUpdateInterval(uint32_t interval_ms);

    /**
     * Get the current monitoring update interval
     * @return Update interval in milliseconds
     */
    uint32_t GetUpdateInterval() const { return m_update_interval_ms; }

    /**
     * Enable or disable detailed signal analysis
     * @param enable true to enable detailed analysis, false for basic monitoring
     */
    void SetDetailedAnalysis(bool enable) { m_detailed_analysis = enable; }

    /**
     * Check if detailed signal analysis is enabled
     * @return true if detailed analysis enabled, false otherwise
     */
    bool IsDetailedAnalysisEnabled() const { return m_detailed_analysis; }

private:
    // V4L2 device for hardware access
    std::shared_ptr<V4L2Device> m_v4l2_device;

    // Thread management
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_shutdown_requested{false};
    std::thread m_monitor_thread;
    std::mutex m_thread_mutex;
    std::condition_variable m_thread_cv;

    // Signal status protection
    mutable std::mutex m_status_mutex;
    SignalStatus m_current_status;
    SignalStatus m_previous_status;

    // Callback management
    mutable std::mutex m_callback_mutex;
    StatusCallback m_status_callback;
    HotPlugCallback m_hotplug_callback;

    // Configuration
    std::atomic<uint32_t> m_update_interval_ms{1000};
    std::atomic<bool> m_detailed_analysis{true};

    // Signal stability tracking
    std::chrono::steady_clock::time_point m_last_stable_time;
    uint32_t m_stability_threshold_ms = 2000;  // 2 seconds for stability
    uint32_t m_consecutive_stable_readings = 0;
    static constexpr uint32_t MIN_STABLE_READINGS = 3;

    // Hot-plug detection state
    bool m_last_connection_state = false;
    std::chrono::steady_clock::time_point m_last_hotplug_time;
    static constexpr uint32_t HOTPLUG_DEBOUNCE_MS = 500;

    // Signal quality history for averaging
    static constexpr size_t QUALITY_HISTORY_SIZE = 10;
    std::vector<uint8_t> m_strength_history;
    std::vector<uint8_t> m_quality_history;
    size_t m_history_index = 0;

    /**
     * Main monitoring thread function
     */
    void MonitorThread();

    /**
     * Perform a single signal status check
     * @return true if status was successfully updated, false otherwise
     */
    bool CheckSignalStatus();

    /**
     * Analyze signal stability and update stability metrics
     * @param current_status Current signal status to analyze
     */
    void AnalyzeSignalStability(const SignalStatus& current_status);

    /**
     * Check for hot-plug events and trigger callbacks if needed
     * @param current_status Current signal status
     */
    void CheckHotPlugEvents(const SignalStatus& current_status);

    /**
     * Update signal quality history for averaging
     * @param strength Current signal strength
     * @param quality Current signal quality
     */
    void UpdateQualityHistory(uint8_t strength, uint8_t quality);

    /**
     * Calculate averaged signal strength from history
     * @return Averaged signal strength (0-100%)
     */
    uint8_t GetAveragedStrength() const;

    /**
     * Calculate averaged signal quality from history
     * @return Averaged signal quality (0-100%)
     */
    uint8_t GetAveragedQuality() const;

    /**
     * Trigger status change callback if registered
     * @param status Current signal status to report
     */
    void TriggerStatusCallback(const SignalStatus& status);

    /**
     * Trigger hot-plug callback if registered
     * @param connected Current connection state
     */
    void TriggerHotPlugCallback(bool connected);

    /**
     * Check if two signal statuses are significantly different
     * @param status1 First status to compare
     * @param status2 Second status to compare
     * @return true if statuses are significantly different, false otherwise
     */
    bool IsSignificantChange(const SignalStatus& status1, const SignalStatus& status2) const;

    /**
     * Perform detailed signal analysis if enabled
     * @param status Signal status to enhance with detailed analysis
     */
    void PerformDetailedAnalysis(SignalStatus& status);

    /**
     * Reset internal state for clean shutdown or restart
     */
    void ResetState();

    /**
     * Validate that the V4L2 device is ready for monitoring
     * @return true if device is ready, false otherwise
     */
    bool ValidateDevice() const;
};

} // namespace hdmi_pvr