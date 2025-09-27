/*
 *  HDMI Input PVR Client for HY300 Projector
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "hdmi_client.h"
#include <kodi/General.h>
#include <algorithm>
#include <chrono>
#include <thread>

namespace hdmi_pvr {

HdmiClient::HdmiClient() = default;

HdmiClient::~HdmiClient() {
    Shutdown();
}

bool HdmiClient::Initialize() {
    if (m_initialized.load()) {
        return true;
    }

    kodi::Log(ADDON_LOG_INFO, "Initializing HDMI client...");

    // Load settings first
    if (!LoadSettings()) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to load settings");
        return false;
    }

    // Initialize components
    if (!InitializeComponents()) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to initialize components");
        ShutdownComponents();
        return false;
    }

    // Start monitoring thread
    m_shutdown_requested = false;
    m_monitor_thread = std::thread(&HdmiClient::MonitorThread, this);

    m_initialized = true;
    kodi::Log(ADDON_LOG_INFO, "HDMI client initialized successfully");
    return true;
}

void HdmiClient::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    kodi::Log(ADDON_LOG_INFO, "Shutting down HDMI client...");

    // Request shutdown and wait for monitor thread
    m_shutdown_requested = true;
    if (m_monitor_thread.joinable()) {
        m_monitor_thread.join();
    }

    // Shutdown components
    ShutdownComponents();

    m_initialized = false;
    kodi::Log(ADDON_LOG_INFO, "HDMI client shutdown complete");
}

bool HdmiClient::SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue) {
    bool changed = false;

    if (settingName == "device_path") {
        std::string new_path = settingValue.GetString();
        if (new_path != m_device_path) {
            m_device_path = new_path;
            changed = true;
            kodi::Log(ADDON_LOG_INFO, "Device path changed to: %s", m_device_path.c_str());
        }
    }
    else if (settingName == "buffer_count") {
        uint32_t new_count = static_cast<uint32_t>(settingValue.GetInt());
        if (new_count != m_buffer_count && new_count >= 2 && new_count <= 16) {
            m_buffer_count = new_count;
            changed = true;
            kodi::Log(ADDON_LOG_INFO, "Buffer count changed to: %u", m_buffer_count);
        }
    }
    else if (settingName == "hardware_decoding") {
        bool new_value = settingValue.GetBoolean();
        if (new_value != m_hardware_decoding) {
            m_hardware_decoding = new_value;
            changed = true;
            kodi::Log(ADDON_LOG_INFO, "Hardware decoding %s", m_hardware_decoding ? "enabled" : "disabled");
        }
    }
    else if (settingName == "audio_enabled") {
        bool new_value = settingValue.GetBoolean();
        if (new_value != m_audio_enabled) {
            m_audio_enabled = new_value;
            changed = true;
            kodi::Log(ADDON_LOG_INFO, "Audio %s", m_audio_enabled ? "enabled" : "disabled");
        }
    }

    return changed;
}

int HdmiClient::GetChannelCount() const {
    if (!m_initialized.load() || !m_channel_manager) {
        return 0;
    }
    return m_channel_manager->GetChannelCount();
}

PVR_ERROR HdmiClient::GetChannels(kodi::addon::PVRChannelsResultSet& results) {
    if (!m_initialized.load() || !m_channel_manager) {
        return PVR_ERROR_SERVER_ERROR;
    }
    return m_channel_manager->GetChannels(results);
}

PVR_ERROR HdmiClient::GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) {
    if (!m_initialized.load() || !m_channel_manager) {
        return PVR_ERROR_SERVER_ERROR;
    }

    auto epg_entries = m_channel_manager->GenerateEPG(static_cast<uint32_t>(channelUid), start, end);
    
    for (const auto& entry : epg_entries) {
        kodi::addon::PVREPGTag tag;
        tag.SetUniqueBroadcastId(entry.unique_id);
        tag.SetUniqueChannelId(entry.channel_id);
        tag.SetTitle(entry.title);
        tag.SetPlot(entry.plot);
        tag.SetGenreType(0);
        tag.SetGenreSubType(0);
        tag.SetStartTime(entry.start_time);
        tag.SetEndTime(entry.end_time);
        tag.SetPlotOutline(entry.genre);
        
        results.Add(tag);
    }

    return PVR_ERROR_NO_ERROR;
}

bool HdmiClient::OpenLiveStream(const kodi::addon::PVRChannel& channel) {
    if (!m_initialized.load() || !m_stream_processor || !m_v4l2_device) {
        kodi::Log(ADDON_LOG_ERROR, "Cannot open live stream - components not initialized");
        return false;
    }

    if (m_streaming.load()) {
        kodi::Log(ADDON_LOG_WARNING, "Stream already open");
        return true;
    }

    kodi::Log(ADDON_LOG_INFO, "Opening live stream for channel %u", channel.GetUniqueId());

    // Switch to the requested channel input
    if (m_channel_manager && !m_channel_manager->SetActiveChannel(channel.GetUniqueId())) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to switch to channel %u", channel.GetUniqueId());
        return false;
    }

    // Wait for signal to stabilize
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Detect input format
    VideoFormat video_format;
    if (!m_v4l2_device->DetectInputFormat(video_format)) {
        kodi::Log(ADDON_LOG_WARNING, "Could not detect input format, using default");
        video_format.width = 1920;
        video_format.height = 1080;
        video_format.fps = 60;
        video_format.fourcc = 0; // Use default
        video_format.interlaced = false;
    }

    // Set the format on V4L2 device
    if (!m_v4l2_device->SetFormat(video_format)) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to set video format: %s", video_format.to_string().c_str());
        return false;
    }

    // Configure audio format
    AudioFormat audio_format;
    audio_format.sample_rate = 48000;
    audio_format.channels = 2;
    audio_format.bit_depth = 16;
    audio_format.compressed = false;

    // Start streaming
    if (!m_stream_processor->StartStreaming(video_format, audio_format)) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to start stream processor");
        return false;
    }

    m_streaming = true;
    kodi::Log(ADDON_LOG_INFO, "Live stream opened successfully");
    return true;
}

void HdmiClient::CloseLiveStream() {
    if (!m_streaming.load()) {
        return;
    }

    kodi::Log(ADDON_LOG_INFO, "Closing live stream");

    if (m_stream_processor) {
        m_stream_processor->StopStreaming();
    }

    m_streaming = false;
    kodi::Log(ADDON_LOG_INFO, "Live stream closed");
}

int HdmiClient::ReadLiveStream(unsigned char* buffer, unsigned int size) {
    if (!m_streaming.load() || !m_stream_processor || !buffer || size == 0) {
        return -1;
    }

    return m_stream_processor->ReadLiveStream(buffer, size);
}

PVR_ERROR HdmiClient::GetStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties) {
    if (!m_streaming.load() || !m_stream_processor) {
        return PVR_ERROR_SERVER_ERROR;
    }

    return m_stream_processor->GetStreamProperties(properties);
}

PVR_ERROR HdmiClient::GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) {
    if (!m_initialized.load() || !m_signal_monitor) {
        return PVR_ERROR_SERVER_ERROR;
    }

    SignalStatus status = m_signal_monitor->GetSignalStatus();
    
    signalStatus.SetAdapterName("HY300 HDMI Input");
    signalStatus.SetAdapterStatus(status.connected ? "Connected" : "No Signal");
    signalStatus.SetServiceName(status.device_name);
    signalStatus.SetMuxName("HDMI Input");
    signalStatus.SetSignal(static_cast<int>(status.signal_strength * 655.35)); // Scale to 0-65535
    signalStatus.SetSNR(static_cast<int>(status.signal_quality * 655.35));
    signalStatus.SetBER(0);
    signalStatus.SetUNC(0);

    return PVR_ERROR_NO_ERROR;
}

bool HdmiClient::OpenDemuxStream(const kodi::addon::PVRChannel& channel) {
    if (!m_initialized.load() || !m_stream_processor) {
        return false;
    }

    // First ensure live stream is open
    if (!m_streaming.load() && !OpenLiveStream(channel)) {
        return false;
    }

    return m_stream_processor->OpenDemuxStream();
}

void HdmiClient::CloseDemuxStream() {
    if (m_stream_processor) {
        m_stream_processor->CloseDemuxStream();
    }
}

DEMUX_PACKET* HdmiClient::DemuxRead() {
    if (!m_stream_processor) {
        return nullptr;
    }
    return m_stream_processor->DemuxRead();
}

void HdmiClient::DemuxAbort() {
    if (m_stream_processor) {
        m_stream_processor->DemuxAbort();
    }
}

void HdmiClient::DemuxFlush() {
    if (m_stream_processor) {
        m_stream_processor->DemuxFlush();
    }
}

void HdmiClient::DemuxReset() {
    if (m_stream_processor) {
        m_stream_processor->DemuxReset();
    }
}

PVR_ERROR HdmiClient::CallMenuHook(const kodi::addon::PVRMenuhook& menuhook, const kodi::addon::PVRChannel& channel) {
    kodi::Log(ADDON_LOG_INFO, "Menu hook called: %u for channel %u", menuhook.GetHookId(), channel.GetUniqueId());
    
    switch (menuhook.GetHookId()) {
        case 1: // Refresh signal status
            if (m_signal_monitor) {
                m_signal_monitor->UpdateSignalStatus();
            }
            break;
        case 2: // Switch input
            if (m_channel_manager) {
                m_channel_manager->DetectActiveInputs();
            }
            break;
        default:
            return PVR_ERROR_NOT_IMPLEMENTED;
    }

    return PVR_ERROR_NO_ERROR;
}

// Private methods

bool HdmiClient::InitializeComponents() {
    try {
        // Initialize V4L2 device
        m_v4l2_device = std::make_unique<V4L2Device>(m_device_path);
        if (!m_v4l2_device->Open()) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to open V4L2 device: %s", m_device_path.c_str());
            return false;
        }

        if (!m_v4l2_device->QueryCapabilities()) {
            kodi::Log(ADDON_LOG_ERROR, "V4L2 device does not support required capabilities");
            return false;
        }

        kodi::Log(ADDON_LOG_INFO, "V4L2 device opened: %s (driver: %s)", 
                  m_v4l2_device->GetCardName().c_str(), 
                  m_v4l2_device->GetDriverName().c_str());

        // Initialize channel manager
        m_channel_manager = std::make_unique<ChannelManager>(m_v4l2_device.get());
        if (!m_channel_manager->Initialize()) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to initialize channel manager");
            return false;
        }

        // Initialize stream processor
        m_stream_processor = std::make_unique<StreamProcessor>(m_v4l2_device.get());
        if (!m_stream_processor->Initialize()) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to initialize stream processor");
            return false;
        }

        // Set buffer parameters
        if (!m_stream_processor->SetBufferParameters(m_buffer_count, 1024 * 1024)) {
            kodi::Log(ADDON_LOG_WARNING, "Failed to set buffer parameters, using defaults");
        }

        // Initialize signal monitor with shared V4L2 device
        auto shared_v4l2 = std::shared_ptr<V4L2Device>(m_v4l2_device.get(), [](V4L2Device*){});
        m_signal_monitor = std::make_unique<SignalMonitor>(shared_v4l2);
        if (!m_signal_monitor->Initialize()) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to initialize signal monitor");
            return false;
        }

        // Allocate V4L2 buffers
        if (!m_v4l2_device->AllocateBuffers(m_buffer_count)) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to allocate V4L2 buffers");
            return false;
        }

        kodi::Log(ADDON_LOG_INFO, "All components initialized successfully");
        return true;
    }
    catch (const std::exception& e) {
        kodi::Log(ADDON_LOG_ERROR, "Exception during component initialization: %s", e.what());
        return false;
    }
}

void HdmiClient::ShutdownComponents() {
    // Shutdown in reverse order
    if (m_signal_monitor) {
        m_signal_monitor->Shutdown();
        m_signal_monitor.reset();
    }

    if (m_stream_processor) {
        m_stream_processor->Shutdown();
        m_stream_processor.reset();
    }

    if (m_channel_manager) {
        m_channel_manager->Shutdown();
        m_channel_manager.reset();
    }

    if (m_v4l2_device) {
        m_v4l2_device->Close();
        m_v4l2_device.reset();
    }

    kodi::Log(ADDON_LOG_INFO, "All components shut down");
}

bool HdmiClient::LoadSettings() {
    try {
        // Load device path setting
        m_device_path = kodi::addon::GetSettingString("device_path", "/dev/video0");
        
        // Load buffer count setting  
        m_buffer_count = static_cast<uint32_t>(kodi::addon::GetSettingInt("buffer_count", 4));
        if (m_buffer_count < 2) m_buffer_count = 2;
        if (m_buffer_count > 16) m_buffer_count = 16;
        
        // Load hardware decoding setting
        m_hardware_decoding = kodi::addon::GetSettingBoolean("hardware_decoding", true);
        
        // Load audio enabled setting
        m_audio_enabled = kodi::addon::GetSettingBoolean("audio_enabled", true);

        kodi::Log(ADDON_LOG_INFO, "Settings loaded - Device: %s, Buffers: %u, HW Decode: %s, Audio: %s",
                  m_device_path.c_str(), m_buffer_count,
                  m_hardware_decoding ? "enabled" : "disabled",
                  m_audio_enabled ? "enabled" : "disabled");

        return true;
    }
    catch (const std::exception& e) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to load settings: %s", e.what());
        return false;
    }
}

void HdmiClient::MonitorThread() {
    kodi::Log(ADDON_LOG_DEBUG, "Monitor thread started");

    while (!m_shutdown_requested.load()) {
        try {
            UpdateSignalStatus();
            
            // Sleep for 1 second before next update
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        catch (const std::exception& e) {
            kodi::Log(ADDON_LOG_ERROR, "Exception in monitor thread: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }

    kodi::Log(ADDON_LOG_DEBUG, "Monitor thread stopped");
}

void HdmiClient::UpdateSignalStatus() {
    if (!m_v4l2_device || !m_signal_monitor) {
        return;
    }

    // Force signal status update
    m_signal_monitor->UpdateSignalStatus();

    // Update channel status if channel manager exists
    if (m_channel_manager) {
        SignalStatus status = m_signal_monitor->GetSignalStatus();
        uint32_t active_channel = m_channel_manager->GetActiveChannel();
        if (active_channel > 0) {
            m_channel_manager->UpdateChannelStatus(active_channel, status);
        }
    }
}

} // namespace hdmi_pvr