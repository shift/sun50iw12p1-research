#pragma once

#include "types.h"
#include "v4l2_device.h"
#include "channel_manager.h"
#include "stream_processor.h"
#include "signal_monitor.h"
#include <kodi/addon-instance/PVR.h>
#include <memory>
#include <atomic>
#include <thread>

namespace hdmi_pvr {

class HdmiClient {
public:
    HdmiClient();
    ~HdmiClient();

    // Initialization and lifecycle
    bool Initialize();
    void Shutdown();

    // Settings management
    bool SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue);

    // Channel operations
    int GetChannelCount() const;
    PVR_ERROR GetChannels(kodi::addon::PVRChannelsResultSet& results);

    // EPG operations
    PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results);

    // Stream operations
    bool OpenLiveStream(const kodi::addon::PVRChannel& channel);
    void CloseLiveStream();
    int ReadLiveStream(unsigned char* buffer, unsigned int size);
    PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties);

    // Signal status
    PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus);

    // Demux operations for hardware-accelerated streaming
    bool OpenDemuxStream(const kodi::addon::PVRChannel& channel);
    void CloseDemuxStream();
    DEMUX_PACKET* DemuxRead();
    void DemuxAbort();
    void DemuxFlush();
    void DemuxReset();

    // Menu hooks
    PVR_ERROR CallMenuHook(const kodi::addon::PVRMenuhook& menuhook, const kodi::addon::PVRChannel& channel);

private:
    // Component instances
    std::unique_ptr<V4L2Device> m_v4l2_device;
    std::unique_ptr<ChannelManager> m_channel_manager;
    std::unique_ptr<StreamProcessor> m_stream_processor;
    std::unique_ptr<SignalMonitor> m_signal_monitor;

    // State management
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_streaming{false};
    std::atomic<bool> m_shutdown_requested{false};

    // Threading
    std::thread m_monitor_thread;
    void MonitorThread();

    // Configuration
    std::string m_device_path{"/dev/video0"};
    uint32_t m_buffer_count{4};
    bool m_hardware_decoding{true};
    bool m_audio_enabled{true};

    // Internal helpers
    bool InitializeComponents();
    void ShutdownComponents();
    bool LoadSettings();
    void UpdateSignalStatus();
};

} // namespace hdmi_pvr