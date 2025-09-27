/*
 *  HDMI Input PVR Client for HY300 Projector
 *  Signal Monitor Implementation
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "signal_monitor.h"
#include "v4l2_device.h"
#include <kodi/General.h>
#include <algorithm>
#include <numeric>

namespace hdmi_pvr {

SignalMonitor::SignalMonitor(std::shared_ptr<V4L2Device> v4l2_device)
    : m_v4l2_device(std::move(v4l2_device)) {
    
    // Initialize quality history vectors
    m_strength_history.resize(QUALITY_HISTORY_SIZE, 0);
    m_quality_history.resize(QUALITY_HISTORY_SIZE, 0);
    
    // Initialize status structures
    m_current_status = {};
    m_previous_status = {};
    
    m_last_stable_time = std::chrono::steady_clock::now();
    m_last_hotplug_time = std::chrono::steady_clock::now();
    
    kodi::Log(ADDON_LOG_DEBUG, "SignalMonitor created");
}

SignalMonitor::~SignalMonitor() {
    kodi::Log(ADDON_LOG_DEBUG, "SignalMonitor destructor called");
    Shutdown();
}

SignalMonitor::SignalMonitor(SignalMonitor&& other) noexcept 
    : m_v4l2_device(std::move(other.m_v4l2_device))
    , m_active(other.m_active.load())
    , m_shutdown_requested(other.m_shutdown_requested.load())
    , m_monitor_thread(std::move(other.m_monitor_thread))
    , m_current_status(std::move(other.m_current_status))
    , m_previous_status(std::move(other.m_previous_status))
    , m_status_callback(std::move(other.m_status_callback))
    , m_hotplug_callback(std::move(other.m_hotplug_callback))
    , m_update_interval_ms(other.m_update_interval_ms.load())
    , m_detailed_analysis(other.m_detailed_analysis.load())
    , m_last_stable_time(other.m_last_stable_time)
    , m_stability_threshold_ms(other.m_stability_threshold_ms)
    , m_consecutive_stable_readings(other.m_consecutive_stable_readings)
    , m_last_connection_state(other.m_last_connection_state)
    , m_last_hotplug_time(other.m_last_hotplug_time)
    , m_strength_history(std::move(other.m_strength_history))
    , m_quality_history(std::move(other.m_quality_history))
    , m_history_index(other.m_history_index) {
    
    // Reset other object
    other.m_active.store(false);
    other.m_shutdown_requested.store(false);
}

SignalMonitor& SignalMonitor::operator=(SignalMonitor&& other) noexcept {
    if (this != &other) {
        // Shutdown current instance
        Shutdown();
        
        // Move data from other
        m_v4l2_device = std::move(other.m_v4l2_device);
        m_active.store(other.m_active.load());
        m_shutdown_requested.store(other.m_shutdown_requested.load());
        m_monitor_thread = std::move(other.m_monitor_thread);
        m_current_status = std::move(other.m_current_status);
        m_previous_status = std::move(other.m_previous_status);
        m_status_callback = std::move(other.m_status_callback);
        m_hotplug_callback = std::move(other.m_hotplug_callback);
        m_update_interval_ms.store(other.m_update_interval_ms.load());
        m_detailed_analysis.store(other.m_detailed_analysis.load());
        m_last_stable_time = other.m_last_stable_time;
        m_stability_threshold_ms = other.m_stability_threshold_ms;
        m_consecutive_stable_readings = other.m_consecutive_stable_readings;
        m_last_connection_state = other.m_last_connection_state;
        m_last_hotplug_time = other.m_last_hotplug_time;
        m_strength_history = std::move(other.m_strength_history);
        m_quality_history = std::move(other.m_quality_history);
        m_history_index = other.m_history_index;
        
        // Reset other object
        other.m_active.store(false);
        other.m_shutdown_requested.store(false);
    }
    return *this;
}

bool SignalMonitor::Initialize() {
    if (m_active.load()) {
        kodi::Log(ADDON_LOG_WARNING, "SignalMonitor already initialized");
        return true;
    }
    
    if (!ValidateDevice()) {
        kodi::Log(ADDON_LOG_ERROR, "V4L2 device validation failed");
        return false;
    }
    
    // Reset state
    ResetState();
    
    // Start monitoring thread
    m_shutdown_requested.store(false);
    m_monitor_thread = std::thread(&SignalMonitor::MonitorThread, this);
    
    m_active.store(true);
    kodi::Log(ADDON_LOG_INFO, "SignalMonitor initialized successfully");
    return true;
}

void SignalMonitor::Shutdown() {
    if (!m_active.load()) {
        return;
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Shutting down SignalMonitor");
    
    // Signal shutdown to monitoring thread
    m_shutdown_requested.store(true);
    m_thread_cv.notify_all();
    
    // Wait for thread to finish
    if (m_monitor_thread.joinable()) {
        m_monitor_thread.join();
    }
    
    // Clear callbacks
    {
        std::lock_guard<std::mutex> lock(m_callback_mutex);
        m_status_callback = nullptr;
        m_hotplug_callback = nullptr;
    }
    
    m_active.store(false);
    kodi::Log(ADDON_LOG_INFO, "SignalMonitor shutdown complete");
}

SignalStatus SignalMonitor::GetSignalStatus() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status;
}

bool SignalMonitor::UpdateSignalStatus() {
    if (!m_active.load() || !m_v4l2_device) {
        return false;
    }
    
    return CheckSignalStatus();
}

bool SignalMonitor::IsSignalConnected() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.connected;
}

bool SignalMonitor::IsSignalLocked() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.signal_locked;
}

uint8_t SignalMonitor::GetSignalStrength() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.signal_strength;
}

uint8_t SignalMonitor::GetSignalQuality() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.signal_quality;
}

VideoFormat SignalMonitor::GetVideoFormat() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.video_format;
}

AudioFormat SignalMonitor::GetAudioFormat() const {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_current_status.audio_format;
}

void SignalMonitor::SetStatusCallback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_status_callback = std::move(callback);
    kodi::Log(ADDON_LOG_DEBUG, "Status callback registered");
}

void SignalMonitor::SetHotPlugCallback(HotPlugCallback callback) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    m_hotplug_callback = std::move(callback);
    kodi::Log(ADDON_LOG_DEBUG, "Hot-plug callback registered");
}

void SignalMonitor::SetUpdateInterval(uint32_t interval_ms) {
    if (interval_ms < 100) {
        interval_ms = 100; // Minimum 100ms
    } else if (interval_ms > 10000) {
        interval_ms = 10000; // Maximum 10s
    }
    
    m_update_interval_ms.store(interval_ms);
    m_thread_cv.notify_all();
    
    kodi::Log(ADDON_LOG_DEBUG, "Update interval set to %u ms", interval_ms);
}

// Private methods

void SignalMonitor::MonitorThread() {
    kodi::Log(ADDON_LOG_DEBUG, "Signal monitoring thread started");
    
    while (!m_shutdown_requested.load()) {
        try {
            // Check signal status
            CheckSignalStatus();
            
            // Wait for next update interval or shutdown signal
            std::unique_lock<std::mutex> lock(m_thread_mutex);
            auto timeout = std::chrono::milliseconds(m_update_interval_ms.load());
            m_thread_cv.wait_for(lock, timeout, [this]() {
                return m_shutdown_requested.load();
            });
        }
        catch (const std::exception& e) {
            kodi::Log(ADDON_LOG_ERROR, "Exception in signal monitoring thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    kodi::Log(ADDON_LOG_DEBUG, "Signal monitoring thread finished");
}

bool SignalMonitor::CheckSignalStatus() {
    if (!m_v4l2_device) {
        return false;
    }
    
    // Get current status from V4L2 device
    SignalStatus new_status = m_v4l2_device->GetSignalStatus();
    
    // Perform detailed analysis if enabled
    if (m_detailed_analysis.load()) {
        PerformDetailedAnalysis(new_status);
    }
    
    // Update quality history
    UpdateQualityHistory(new_status.signal_strength, new_status.signal_quality);
    
    // Apply averaged values
    new_status.signal_strength = GetAveragedStrength();
    new_status.signal_quality = GetAveragedQuality();
    
    // Analyze signal stability
    AnalyzeSignalStability(new_status);
    
    // Check for hot-plug events
    CheckHotPlugEvents(new_status);
    
    // Update current status
    bool status_changed = false;
    {
        std::lock_guard<std::mutex> lock(m_status_mutex);
        
        status_changed = IsSignificantChange(m_current_status, new_status);
        m_previous_status = m_current_status;
        m_current_status = new_status;
        m_current_status.last_update = std::chrono::steady_clock::now();
    }
    
    // Trigger callbacks if status changed significantly
    if (status_changed) {
        TriggerStatusCallback(new_status);
    }
    
    return true;
}

void SignalMonitor::AnalyzeSignalStability(const SignalStatus& current_status) {
    auto now = std::chrono::steady_clock::now();
    
    // Check if signal parameters are stable
    bool is_stable = current_status.connected && current_status.signal_locked &&
                    current_status.signal_strength > 50 && current_status.signal_quality > 50;
    
    if (is_stable) {
        auto stable_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_last_stable_time);
            
        if (stable_duration.count() >= m_stability_threshold_ms) {
            m_consecutive_stable_readings++;
        }
    } else {
        m_last_stable_time = now;
        m_consecutive_stable_readings = 0;
    }
}

void SignalMonitor::CheckHotPlugEvents(const SignalStatus& current_status) {
    bool current_connection = current_status.connected;
    
    // Check if connection state changed
    if (current_connection != m_last_connection_state) {
        auto now = std::chrono::steady_clock::now();
        auto debounce_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_last_hotplug_time);
        
        // Apply debounce filter
        if (debounce_elapsed.count() >= HOTPLUG_DEBOUNCE_MS) {
            kodi::Log(ADDON_LOG_INFO, "Hot-plug event detected: %s", 
                      current_connection ? "connected" : "disconnected");
            
            m_last_connection_state = current_connection;
            m_last_hotplug_time = now;
            
            TriggerHotPlugCallback(current_connection);
        }
    }
}

void SignalMonitor::UpdateQualityHistory(uint8_t strength, uint8_t quality) {
    m_strength_history[m_history_index] = strength;
    m_quality_history[m_history_index] = quality;
    
    m_history_index = (m_history_index + 1) % QUALITY_HISTORY_SIZE;
}

uint8_t SignalMonitor::GetAveragedStrength() const {
    if (m_strength_history.empty()) {
        return 0;
    }
    
    uint32_t sum = std::accumulate(m_strength_history.begin(), m_strength_history.end(), 0u);
    return static_cast<uint8_t>(sum / m_strength_history.size());
}

uint8_t SignalMonitor::GetAveragedQuality() const {
    if (m_quality_history.empty()) {
        return 0;
    }
    
    uint32_t sum = std::accumulate(m_quality_history.begin(), m_quality_history.end(), 0u);
    return static_cast<uint8_t>(sum / m_quality_history.size());
}

void SignalMonitor::TriggerStatusCallback(const SignalStatus& status) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    if (m_status_callback) {
        try {
            m_status_callback(status);
        }
        catch (const std::exception& e) {
            kodi::Log(ADDON_LOG_ERROR, "Exception in status callback: %s", e.what());
        }
    }
}

void SignalMonitor::TriggerHotPlugCallback(bool connected) {
    std::lock_guard<std::mutex> lock(m_callback_mutex);
    if (m_hotplug_callback) {
        try {
            m_hotplug_callback(connected);
        }
        catch (const std::exception& e) {
            kodi::Log(ADDON_LOG_ERROR, "Exception in hot-plug callback: %s", e.what());
        }
    }
}

bool SignalMonitor::IsSignificantChange(const SignalStatus& status1, const SignalStatus& status2) const {
    // Connection state change is always significant
    if (status1.connected != status2.connected || status1.signal_locked != status2.signal_locked) {
        return true;
    }
    
    // Signal strength/quality changes > 10% are significant
    if (std::abs(static_cast<int>(status1.signal_strength) - static_cast<int>(status2.signal_strength)) > 10) {
        return true;
    }
    
    if (std::abs(static_cast<int>(status1.signal_quality) - static_cast<int>(status2.signal_quality)) > 10) {
        return true;
    }
    
    // Video format changes are significant
    if (status1.video_format.width != status2.video_format.width ||
        status1.video_format.height != status2.video_format.height ||
        status1.video_format.fps != status2.video_format.fps) {
        return true;
    }
    
    // Audio format changes are significant
    if (status1.audio_format.sample_rate != status2.audio_format.sample_rate ||
        status1.audio_format.channels != status2.audio_format.channels) {
        return true;
    }
    
    return false;
}

void SignalMonitor::PerformDetailedAnalysis(SignalStatus& status) {
    if (!m_v4l2_device || !status.connected) {
        return;
    }
    
    // Try to detect video format if not already detected
    if (!status.video_format.is_valid()) {
        VideoFormat detected_format;
        if (m_v4l2_device->DetectInputFormat(detected_format)) {
            status.video_format = detected_format;
        }
    }
    
    // Enhance signal quality assessment based on format stability
    if (m_consecutive_stable_readings >= MIN_STABLE_READINGS) {
        // Bonus for stable signal
        status.signal_quality = std::min(100, static_cast<int>(status.signal_quality) + 10);
    }
    
    // Adjust quality based on resolution and refresh rate
    if (status.video_format.is_valid()) {
        uint32_t total_pixels = status.video_format.width * status.video_format.height;
        
        // Penalize quality for extremely high resolutions that might stress the system
        if (total_pixels > 3840 * 2160) {
            status.signal_quality = static_cast<uint8_t>(status.signal_quality * 0.9);
        }
        
        // Penalize quality for very high refresh rates
        if (status.video_format.fps > 60) {
            status.signal_quality = static_cast<uint8_t>(status.signal_quality * 0.95);
        }
    }
}

void SignalMonitor::ResetState() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    
    m_current_status = {};
    m_previous_status = {};
    
    m_last_stable_time = std::chrono::steady_clock::now();
    m_last_hotplug_time = std::chrono::steady_clock::now();
    
    m_consecutive_stable_readings = 0;
    m_last_connection_state = false;
    
    // Reset quality history
    std::fill(m_strength_history.begin(), m_strength_history.end(), 0);
    std::fill(m_quality_history.begin(), m_quality_history.end(), 0);
    m_history_index = 0;
    
    kodi::Log(ADDON_LOG_DEBUG, "SignalMonitor state reset");
}

bool SignalMonitor::ValidateDevice() const {
    if (!m_v4l2_device) {
        kodi::Log(ADDON_LOG_ERROR, "No V4L2 device provided");
        return false;
    }
    
    if (!m_v4l2_device->IsOpen()) {
        kodi::Log(ADDON_LOG_WARNING, "V4L2 device is not open, signal monitoring may be limited");
        // Don't fail validation - we can still provide basic monitoring
    }
    
    return true;
}

} // namespace hdmi_pvr