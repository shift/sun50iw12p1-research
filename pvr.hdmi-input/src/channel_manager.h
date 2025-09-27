#pragma once

#include "types.h"
#include <kodi/addon-instance/PVR.h>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <atomic>

namespace hdmi_pvr {

// Forward declaration
class V4L2Device;

// Input type enumeration
enum class InputType {
    HDMI = 0,
    COMPONENT = 1,
    COMPOSITE = 2,
    SVIDEO = 3,
    UNKNOWN = 255
};

// Input source configuration
struct InputSource {
    uint32_t input_id = 0;
    InputType type = InputType::HDMI;
    std::string name = "HDMI Input";
    std::string icon_path;
    bool enabled = true;
    bool auto_detect = true;
    uint32_t channel_number = 1;
    uint32_t sub_channel_number = 0;
    
    // Signal detection settings
    uint32_t detection_timeout_ms = 3000;
    uint8_t min_signal_strength = 50;
    
    // Display settings
    std::string display_name;
    std::string description;
    bool show_osd = true;
};

// Channel settings for persistence
struct ChannelSettings {
    std::map<uint32_t, InputSource> inputs;
    bool auto_channel_numbering = true;
    uint32_t base_channel_number = 1;
    std::string default_icon_path;
    bool enable_epg = true;
    uint32_t epg_duration_hours = 24;
    
    // Load/save settings
    bool LoadFromFile(const std::string& config_path);
    bool SaveToFile(const std::string& config_path) const;
};

class ChannelManager {
public:
    explicit ChannelManager(V4L2Device* v4l2_device = nullptr);
    ~ChannelManager();

    // Initialization and lifecycle
    bool Initialize(const std::string& config_path = "");
    void Shutdown();
    bool IsInitialized() const { return m_initialized; }

    // Channel enumeration for Kodi PVR
    int GetChannelCount() const;
    PVR_ERROR GetChannels(kodi::addon::PVRChannelsResultSet& results);
    PVR_ERROR GetChannelInfo(uint32_t channel_id, ChannelInfo& channel_info);

    // Input source management
    bool AddInputSource(const InputSource& input);
    bool RemoveInputSource(uint32_t input_id);
    bool UpdateInputSource(uint32_t input_id, const InputSource& input);
    std::vector<InputSource> GetInputSources() const;
    InputSource* GetInputSource(uint32_t input_id);
    const InputSource* GetInputSource(uint32_t input_id) const;

    // Channel operations
    bool SetActiveChannel(uint32_t channel_id);
    uint32_t GetActiveChannel() const { return m_active_channel_id; }
    bool IsChannelAvailable(uint32_t channel_id) const;
    std::string GetChannelName(uint32_t channel_id) const;

    // Input switching and detection
    bool SwitchToInput(uint32_t input_id);
    uint32_t GetCurrentInput() const { return m_current_input_id; }
    bool DetectActiveInputs();
    std::vector<uint32_t> GetActiveInputs() const;

    // Channel settings and configuration
    bool SetChannelSettings(const ChannelSettings& settings);
    ChannelSettings GetChannelSettings() const;
    bool LoadChannelSettings(const std::string& config_path);
    bool SaveChannelSettings(const std::string& config_path) const;

    // Channel metadata and EPG
    bool UpdateChannelMetadata(uint32_t channel_id, const std::string& name, 
                              const std::string& icon = "", const std::string& description = "");
    std::vector<EpgEntry> GenerateEPG(uint32_t channel_id, time_t start_time, time_t end_time);

    // Signal status integration
    bool UpdateChannelStatus(uint32_t channel_id, const SignalStatus& status);
    SignalStatus GetChannelStatus(uint32_t channel_id) const;
    void RefreshAllChannelStatus();

    // Thread-safe channel access
    void LockChannels() const { m_channel_mutex.lock(); }
    void UnlockChannels() const { m_channel_mutex.unlock(); }
    std::unique_lock<std::mutex> GetChannelLock() const { 
        return std::unique_lock<std::mutex>(m_channel_mutex); 
    }

    // Channel validation and health
    bool ValidateChannelConfiguration() const;
    std::vector<std::string> GetConfigurationErrors() const;
    void LogChannelStatus() const;

    // Kodi PVR integration helpers
    static kodi::addon::PVRChannel CreateKodiChannel(const InputSource& input, uint32_t unique_id);
    static InputType ParseInputType(const std::string& type_name);
    static std::string InputTypeToString(InputType type);

private:
    // Device integration
    V4L2Device* m_v4l2_device;
    bool m_owns_v4l2_device;

    // State management
    std::atomic<bool> m_initialized{false};
    std::atomic<uint32_t> m_active_channel_id{1};
    std::atomic<uint32_t> m_current_input_id{0};

    // Configuration
    ChannelSettings m_settings;
    std::string m_config_path;
    
    // Channel data
    std::map<uint32_t, InputSource> m_input_sources;
    std::map<uint32_t, uint32_t> m_channel_to_input_map;  // channel_id -> input_id
    std::map<uint32_t, SignalStatus> m_channel_status;

    // Thread safety
    mutable std::mutex m_channel_mutex;
    mutable std::mutex m_settings_mutex;

    // Internal helpers
    bool LoadDefaultConfiguration();
    bool ValidateInputSource(const InputSource& input) const;
    uint32_t GenerateChannelId(const InputSource& input) const;
    uint32_t GetNextAvailableChannelNumber() const;
    bool UpdateInputMapping();
    void ClearChannelData();
    
    // Configuration file management
    bool ParseConfigurationFile(const std::string& config_path);
    bool WriteConfigurationFile(const std::string& config_path) const;
    
    // Input detection and management
    bool ProbeV4L2Inputs();
    InputType DetectInputType(uint32_t v4l2_input_id) const;
    std::string GenerateInputName(InputType type, uint32_t input_id) const;
    
    // Channel number management
    bool AssignChannelNumbers();
    bool IsChannelNumberAvailable(uint32_t channel_number) const;
    
    // EPG generation helpers
    EpgEntry CreateBasicEpgEntry(uint32_t channel_id, time_t start_time, time_t duration) const;
    std::string GenerateEpgTitle(const InputSource& input, const SignalStatus& status) const;
    std::string GenerateEpgDescription(const InputSource& input, const SignalStatus& status) const;

    // Logging and debugging
    void LogInputSources() const;
    void LogChannelMapping() const;
    std::string GetStatusString() const;
};

} // namespace hdmi_pvr