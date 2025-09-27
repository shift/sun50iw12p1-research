#include "channel_manager.h"
#include "v4l2_device.h"
#include <kodi/General.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace hdmi_pvr {

// ChannelSettings Implementation
bool ChannelSettings::LoadFromFile(const std::string& config_path) {
    std::ifstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    inputs.clear();
    std::string line;
    InputSource current_input;
    bool in_input_section = false;
    uint32_t current_input_id = 0;

    while (std::getline(file, line)) {
        // Remove whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty() || line[0] == '#') continue;

        if (line == "[general]") {
            in_input_section = false;
            continue;
        }

        if (line.substr(0, 7) == "[input_") {
            if (in_input_section && current_input_id > 0) {
                inputs[current_input_id] = current_input;
            }
            in_input_section = true;
            current_input = InputSource{};
            current_input_id = std::stoul(line.substr(7, line.find(']') - 7));
            current_input.input_id = current_input_id;
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        if (in_input_section) {
            if (key == "type") {
                if (value == "HDMI") current_input.type = InputType::HDMI;
                else if (value == "COMPONENT") current_input.type = InputType::COMPONENT;
                else if (value == "COMPOSITE") current_input.type = InputType::COMPOSITE;
                else if (value == "SVIDEO") current_input.type = InputType::SVIDEO;
                else current_input.type = InputType::UNKNOWN;
            } else if (key == "name") {
                current_input.name = value;
            } else if (key == "enabled") {
                current_input.enabled = (value == "true" || value == "1");
            } else if (key == "auto_detect") {
                current_input.auto_detect = (value == "true" || value == "1");
            } else if (key == "channel_number") {
                current_input.channel_number = std::stoul(value);
            } else if (key == "display_name") {
                current_input.display_name = value;
            } else if (key == "description") {
                current_input.description = value;
            }
        } else {
            if (key == "auto_channel_numbering") {
                auto_channel_numbering = (value == "true" || value == "1");
            } else if (key == "base_channel_number") {
                base_channel_number = std::stoul(value);
            } else if (key == "enable_epg") {
                enable_epg = (value == "true" || value == "1");
            } else if (key == "epg_duration_hours") {
                epg_duration_hours = std::stoul(value);
            }
        }
    }

    // Add last input if we were in an input section
    if (in_input_section && current_input_id > 0) {
        inputs[current_input_id] = current_input;
    }

    return true;
}

bool ChannelSettings::SaveToFile(const std::string& config_path) const {
    std::ofstream file(config_path);
    if (!file.is_open()) {
        return false;
    }

    file << "# HDMI PVR Channel Configuration\n";
    file << "[general]\n";
    file << "auto_channel_numbering=" << (auto_channel_numbering ? "true" : "false") << "\n";
    file << "base_channel_number=" << base_channel_number << "\n";
    file << "enable_epg=" << (enable_epg ? "true" : "false") << "\n";
    file << "epg_duration_hours=" << epg_duration_hours << "\n";
    file << "\n";

    for (const auto& [input_id, input] : inputs) {
        file << "[input_" << input_id << "]\n";
        file << "type=" << ChannelManager::InputTypeToString(input.type) << "\n";
        file << "name=" << input.name << "\n";
        file << "enabled=" << (input.enabled ? "true" : "false") << "\n";
        file << "auto_detect=" << (input.auto_detect ? "true" : "false") << "\n";
        file << "channel_number=" << input.channel_number << "\n";
        file << "display_name=" << input.display_name << "\n";
        file << "description=" << input.description << "\n";
        file << "\n";
    }

    return true;
}

// ChannelManager Implementation
ChannelManager::ChannelManager(V4L2Device* v4l2_device)
    : m_v4l2_device(v4l2_device)
    , m_owns_v4l2_device(false)
{
    if (!m_v4l2_device) {
        m_v4l2_device = new V4L2Device();
        m_owns_v4l2_device = true;
    }
}

ChannelManager::~ChannelManager() {
    Shutdown();
    if (m_owns_v4l2_device) {
        delete m_v4l2_device;
        m_v4l2_device = nullptr;
    }
}

bool ChannelManager::Initialize(const std::string& config_path) {
    if (m_initialized.load()) {
        return true;
    }

    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);

    m_config_path = config_path.empty() ? "hdmi_pvr_channels.conf" : config_path;

    // Initialize V4L2 device
    if (m_v4l2_device && !m_v4l2_device->IsOpen()) {
        if (!m_v4l2_device->Open()) {
            kodi::Log(ADDON_LOG_WARNING, "Failed to open V4L2 device, continuing with simulation mode");
        } else {
            m_v4l2_device->QueryCapabilities();
        }
    }

    // Load configuration
    if (!LoadChannelSettings(m_config_path)) {
        kodi::Log(ADDON_LOG_INFO, "No existing configuration found, loading defaults");
        if (!LoadDefaultConfiguration()) {
            kodi::Log(ADDON_LOG_ERROR, "Failed to load default configuration");
            return false;
        }
    }

    // Probe for V4L2 inputs if device is available
    if (m_v4l2_device && m_v4l2_device->IsOpen()) {
        ProbeV4L2Inputs();
    }

    // Update input mapping
    if (!UpdateInputMapping()) {
        kodi::Log(ADDON_LOG_ERROR, "Failed to update input mapping");
        return false;
    }

    // Assign channel numbers if auto-numbering is enabled
    if (m_settings.auto_channel_numbering) {
        AssignChannelNumbers();
    }

    // Set first available channel as active
    if (!m_input_sources.empty()) {
        m_active_channel_id = m_input_sources.begin()->second.channel_number;
        m_current_input_id = m_input_sources.begin()->first;
    }

    m_initialized = true;
    kodi::Log(ADDON_LOG_INFO, "ChannelManager initialized with %zu input sources", m_input_sources.size());
    LogInputSources();

    return true;
}

void ChannelManager::Shutdown() {
    if (!m_initialized.load()) {
        return;
    }

    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);

    // Save current configuration
    if (!m_config_path.empty()) {
        SaveChannelSettings(m_config_path);
    }

    // Clear all data
    ClearChannelData();

    // Close V4L2 device if we own it
    if (m_v4l2_device && m_owns_v4l2_device && m_v4l2_device->IsOpen()) {
        m_v4l2_device->Close();
    }

    m_initialized = false;
    kodi::Log(ADDON_LOG_INFO, "ChannelManager shutdown complete");
}

int ChannelManager::GetChannelCount() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    return static_cast<int>(m_input_sources.size());
}

PVR_ERROR ChannelManager::GetChannels(kodi::addon::PVRChannelsResultSet& results) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    for (const auto& [input_id, input_source] : m_input_sources) {
        if (!input_source.enabled) {
            continue;
        }

        kodi::addon::PVRChannel channel = CreateKodiChannel(input_source, input_source.channel_number);
        results.Add(channel);
    }

    kodi::Log(ADDON_LOG_DEBUG, "Returned %d channels to Kodi", results.Size());
    return PVR_ERROR_NO_ERROR;
}

PVR_ERROR ChannelManager::GetChannelInfo(uint32_t channel_id, ChannelInfo& channel_info) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    // Find input source by channel number
    for (const auto& [input_id, input_source] : m_input_sources) {
        if (input_source.channel_number == channel_id) {
            channel_info.channel_id = channel_id;
            channel_info.channel_name = input_source.display_name.empty() ? input_source.name : input_source.display_name;
            channel_info.channel_icon = input_source.icon_path;
            channel_info.channel_number = input_source.channel_number;
            channel_info.sub_channel_number = input_source.sub_channel_number;
            channel_info.preview_enabled = true;
            channel_info.radio = false;
            channel_info.is_hidden = !input_source.enabled;
            return PVR_ERROR_NO_ERROR;
        }
    }

    return PVR_ERROR_INVALID_PARAMETERS;
}

bool ChannelManager::AddInputSource(const InputSource& input) {
    if (!ValidateInputSource(input)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_channel_mutex);

    // Check if input_id already exists
    if (m_input_sources.find(input.input_id) != m_input_sources.end()) {
        kodi::Log(ADDON_LOG_WARNING, "Input source with ID %u already exists", input.input_id);
        return false;
    }

    // Check channel number availability
    if (!IsChannelNumberAvailable(input.channel_number)) {
        kodi::Log(ADDON_LOG_WARNING, "Channel number %u is already in use", input.channel_number);
        return false;
    }

    m_input_sources[input.input_id] = input;
    UpdateInputMapping();

    kodi::Log(ADDON_LOG_INFO, "Added input source: ID=%u, Name='%s', Channel=%u",
              input.input_id, input.name.c_str(), input.channel_number);

    return true;
}

bool ChannelManager::RemoveInputSource(uint32_t input_id) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    auto it = m_input_sources.find(input_id);
    if (it == m_input_sources.end()) {
        return false;
    }

    uint32_t channel_number = it->second.channel_number;
    m_input_sources.erase(it);

    // Remove from channel mapping
    auto channel_it = m_channel_to_input_map.find(channel_number);
    if (channel_it != m_channel_to_input_map.end()) {
        m_channel_to_input_map.erase(channel_it);
    }

    // Remove from status tracking
    auto status_it = m_channel_status.find(channel_number);
    if (status_it != m_channel_status.end()) {
        m_channel_status.erase(status_it);
    }

    kodi::Log(ADDON_LOG_INFO, "Removed input source: ID=%u", input_id);
    return true;
}

bool ChannelManager::UpdateInputSource(uint32_t input_id, const InputSource& input) {
    if (!ValidateInputSource(input) || input.input_id != input_id) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_channel_mutex);

    auto it = m_input_sources.find(input_id);
    if (it == m_input_sources.end()) {
        return false;
    }

    uint32_t old_channel_number = it->second.channel_number;
    uint32_t new_channel_number = input.channel_number;

    // Check if new channel number is available (unless it's the same)
    if (old_channel_number != new_channel_number && !IsChannelNumberAvailable(new_channel_number)) {
        kodi::Log(ADDON_LOG_WARNING, "Channel number %u is already in use", new_channel_number);
        return false;
    }

    // Update input source
    it->second = input;

    // Update channel mapping if channel number changed
    if (old_channel_number != new_channel_number) {
        m_channel_to_input_map.erase(old_channel_number);
        m_channel_to_input_map[new_channel_number] = input_id;

        // Move status data
        auto status_it = m_channel_status.find(old_channel_number);
        if (status_it != m_channel_status.end()) {
            m_channel_status[new_channel_number] = status_it->second;
            m_channel_status.erase(status_it);
        }
    }

    kodi::Log(ADDON_LOG_INFO, "Updated input source: ID=%u, Name='%s', Channel=%u",
              input_id, input.name.c_str(), input.channel_number);

    return true;
}

std::vector<InputSource> ChannelManager::GetInputSources() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    std::vector<InputSource> sources;
    sources.reserve(m_input_sources.size());

    for (const auto& [input_id, input_source] : m_input_sources) {
        sources.push_back(input_source);
    }

    return sources;
}

InputSource* ChannelManager::GetInputSource(uint32_t input_id) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    auto it = m_input_sources.find(input_id);
    return (it != m_input_sources.end()) ? &it->second : nullptr;
}

const InputSource* ChannelManager::GetInputSource(uint32_t input_id) const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    auto it = m_input_sources.find(input_id);
    return (it != m_input_sources.end()) ? &it->second : nullptr;
}

bool ChannelManager::SetActiveChannel(uint32_t channel_id) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    auto channel_it = m_channel_to_input_map.find(channel_id);
    if (channel_it == m_channel_to_input_map.end()) {
        return false;
    }

    uint32_t input_id = channel_it->second;
    auto input_it = m_input_sources.find(input_id);
    if (input_it == m_input_sources.end() || !input_it->second.enabled) {
        return false;
    }

    m_active_channel_id = channel_id;
    m_current_input_id = input_id;

    // Switch V4L2 input if device is available
    if (m_v4l2_device && m_v4l2_device->IsOpen()) {
        m_v4l2_device->SetInput(input_id);
    }

    kodi::Log(ADDON_LOG_INFO, "Set active channel to %u (input %u)", channel_id, input_id);
    return true;
}

bool ChannelManager::IsChannelAvailable(uint32_t channel_id) const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    auto it = m_channel_to_input_map.find(channel_id);
    if (it == m_channel_to_input_map.end()) {
        return false;
    }

    auto input_it = m_input_sources.find(it->second);
    return (input_it != m_input_sources.end() && input_it->second.enabled);
}

std::string ChannelManager::GetChannelName(uint32_t channel_id) const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    auto channel_it = m_channel_to_input_map.find(channel_id);
    if (channel_it == m_channel_to_input_map.end()) {
        return "";
    }

    auto input_it = m_input_sources.find(channel_it->second);
    if (input_it == m_input_sources.end()) {
        return "";
    }

    const auto& input = input_it->second;
    return input.display_name.empty() ? input.name : input.display_name;
}

bool ChannelManager::SwitchToInput(uint32_t input_id) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    auto it = m_input_sources.find(input_id);
    if (it == m_input_sources.end() || !it->second.enabled) {
        return false;
    }

    m_current_input_id = input_id;
    m_active_channel_id = it->second.channel_number;

    // Switch V4L2 input if device is available
    if (m_v4l2_device && m_v4l2_device->IsOpen()) {
        if (!m_v4l2_device->SetInput(input_id)) {
            kodi::Log(ADDON_LOG_WARNING, "Failed to switch V4L2 device to input %u", input_id);
        }
    }

    kodi::Log(ADDON_LOG_INFO, "Switched to input %u (channel %u)", input_id, m_active_channel_id.load());
    return true;
}

bool ChannelManager::DetectActiveInputs() {
    if (!m_v4l2_device || !m_v4l2_device->IsOpen()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_channel_mutex);

    for (auto& [input_id, input_source] : m_input_sources) {
        if (!input_source.auto_detect) {
            continue;
        }

        // Check signal presence for this input
        m_v4l2_device->SetInput(input_id);
        bool signal_present = m_v4l2_device->CheckSignalPresent();

        // Update channel status
        uint32_t channel_number = input_source.channel_number;
        SignalStatus& status = m_channel_status[channel_number];
        status.connected = signal_present;
        status.signal_locked = signal_present;
        status.last_update = std::chrono::steady_clock::now();

        if (signal_present) {
            status.signal_strength = 100;
            status.signal_quality = 90;
            VideoFormat format;
            if (m_v4l2_device->DetectInputFormat(format)) {
                status.video_format = format;
            }
        } else {
            status.signal_strength = 0;
            status.signal_quality = 0;
        }
    }

    return true;
}

std::vector<uint32_t> ChannelManager::GetActiveInputs() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    std::vector<uint32_t> active_inputs;

    for (const auto& [channel_number, status] : m_channel_status) {
        if (status.connected && status.signal_locked) {
            auto channel_it = m_channel_to_input_map.find(channel_number);
            if (channel_it != m_channel_to_input_map.end()) {
                active_inputs.push_back(channel_it->second);
            }
        }
    }

    return active_inputs;
}

bool ChannelManager::SetChannelSettings(const ChannelSettings& settings) {
    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);

    m_settings = settings;
    
    // Update input sources from settings
    m_input_sources = settings.inputs;
    
    // Update mappings
    UpdateInputMapping();
    
    // Reassign channel numbers if auto-numbering is enabled
    if (m_settings.auto_channel_numbering) {
        AssignChannelNumbers();
    }

    kodi::Log(ADDON_LOG_INFO, "Updated channel settings with %zu input sources", m_input_sources.size());
    return true;
}

ChannelSettings ChannelManager::GetChannelSettings() const {
    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);

    ChannelSettings settings = m_settings;
    settings.inputs = m_input_sources;
    return settings;
}

bool ChannelManager::LoadChannelSettings(const std::string& config_path) {
    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    
    if (!m_settings.LoadFromFile(config_path)) {
        return false;
    }

    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);
    m_input_sources = m_settings.inputs;
    UpdateInputMapping();

    kodi::Log(ADDON_LOG_INFO, "Loaded channel settings from %s with %zu input sources",
              config_path.c_str(), m_input_sources.size());

    return true;
}

bool ChannelManager::SaveChannelSettings(const std::string& config_path) const {
    std::lock_guard<std::mutex> settings_lock(m_settings_mutex);
    std::lock_guard<std::mutex> channel_lock(m_channel_mutex);

    ChannelSettings settings = m_settings;
    settings.inputs = m_input_sources;

    bool result = settings.SaveToFile(config_path);
    if (result) {
        kodi::Log(ADDON_LOG_INFO, "Saved channel settings to %s", config_path.c_str());
    } else {
        kodi::Log(ADDON_LOG_ERROR, "Failed to save channel settings to %s", config_path.c_str());
    }

    return result;
}

bool ChannelManager::UpdateChannelMetadata(uint32_t channel_id, const std::string& name,
                                         const std::string& icon, const std::string& description) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    auto channel_it = m_channel_to_input_map.find(channel_id);
    if (channel_it == m_channel_to_input_map.end()) {
        return false;
    }

    auto input_it = m_input_sources.find(channel_it->second);
    if (input_it == m_input_sources.end()) {
        return false;
    }

    InputSource& input = input_it->second;
    if (!name.empty()) input.display_name = name;
    if (!icon.empty()) input.icon_path = icon;
    if (!description.empty()) input.description = description;

    kodi::Log(ADDON_LOG_INFO, "Updated metadata for channel %u", channel_id);
    return true;
}

std::vector<EpgEntry> ChannelManager::GenerateEPG(uint32_t channel_id, time_t start_time, time_t end_time) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    std::vector<EpgEntry> epg_entries;

    if (!m_settings.enable_epg) {
        return epg_entries;
    }

    auto channel_it = m_channel_to_input_map.find(channel_id);
    if (channel_it == m_channel_to_input_map.end()) {
        return epg_entries;
    }

    auto input_it = m_input_sources.find(channel_it->second);
    if (input_it == m_input_sources.end()) {
        return epg_entries;
    }

    const InputSource& input = input_it->second;
    SignalStatus status;
    auto status_it = m_channel_status.find(channel_id);
    if (status_it != m_channel_status.end()) {
        status = status_it->second;
    }

    // Generate EPG entries for the time range
    time_t current_time = start_time;
    uint32_t broadcast_id = channel_id * 1000; // Base broadcast ID

    while (current_time < end_time) {
        time_t entry_duration = 3600; // 1 hour entries
        if (current_time + entry_duration > end_time) {
            entry_duration = end_time - current_time;
        }

        EpgEntry entry = CreateBasicEpgEntry(channel_id, current_time, entry_duration);
        entry.broadcast_id = broadcast_id++;
        entry.title = GenerateEpgTitle(input, status);
        entry.plot = GenerateEpgDescription(input, status);

        epg_entries.push_back(entry);
        current_time += entry_duration;
    }

    return epg_entries;
}

bool ChannelManager::UpdateChannelStatus(uint32_t channel_id, const SignalStatus& status) {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    m_channel_status[channel_id] = status;
    return true;
}

SignalStatus ChannelManager::GetChannelStatus(uint32_t channel_id) const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    auto it = m_channel_status.find(channel_id);
    return (it != m_channel_status.end()) ? it->second : SignalStatus{};
}

void ChannelManager::RefreshAllChannelStatus() {
    if (!m_v4l2_device || !m_v4l2_device->IsOpen()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_channel_mutex);

    for (const auto& [input_id, input_source] : m_input_sources) {
        uint32_t channel_number = input_source.channel_number;
        
        // Switch to input and get status
        m_v4l2_device->SetInput(input_id);
        SignalStatus status = m_v4l2_device->GetSignalStatus();
        
        m_channel_status[channel_number] = status;
    }
}

bool ChannelManager::ValidateChannelConfiguration() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);

    // Check for duplicate channel numbers
    std::set<uint32_t> channel_numbers;
    for (const auto& [input_id, input_source] : m_input_sources) {
        if (channel_numbers.count(input_source.channel_number)) {
            return false;
        }
        channel_numbers.insert(input_source.channel_number);
    }

    // Check for valid input sources
    for (const auto& [input_id, input_source] : m_input_sources) {
        if (!ValidateInputSource(input_source)) {
            return false;
        }
    }

    return true;
}

std::vector<std::string> ChannelManager::GetConfigurationErrors() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    std::vector<std::string> errors;

    // Check for duplicate channel numbers
    std::map<uint32_t, std::vector<uint32_t>> channel_conflicts;
    for (const auto& [input_id, input_source] : m_input_sources) {
        channel_conflicts[input_source.channel_number].push_back(input_id);
    }

    for (const auto& [channel_number, input_ids] : channel_conflicts) {
        if (input_ids.size() > 1) {
            std::ostringstream oss;
            oss << "Channel number " << channel_number << " is used by multiple inputs: ";
            for (size_t i = 0; i < input_ids.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << input_ids[i];
            }
            errors.push_back(oss.str());
        }
    }

    // Validate each input source
    for (const auto& [input_id, input_source] : m_input_sources) {
        if (input_source.name.empty()) {
            errors.push_back("Input " + std::to_string(input_id) + " has empty name");
        }
        if (input_source.channel_number == 0) {
            errors.push_back("Input " + std::to_string(input_id) + " has invalid channel number 0");
        }
    }

    return errors;
}

void ChannelManager::LogChannelStatus() const {
    std::lock_guard<std::mutex> lock(m_channel_mutex);
    
    kodi::Log(ADDON_LOG_INFO, "=== Channel Manager Status ===");
    kodi::Log(ADDON_LOG_INFO, "Initialized: %s", m_initialized.load() ? "true" : "false");
    kodi::Log(ADDON_LOG_INFO, "Active Channel: %u", m_active_channel_id.load());
    kodi::Log(ADDON_LOG_INFO, "Current Input: %u", m_current_input_id.load());
    kodi::Log(ADDON_LOG_INFO, "Input Sources: %zu", m_input_sources.size());
    
    for (const auto& [input_id, input_source] : m_input_sources) {
        kodi::Log(ADDON_LOG_INFO, "  Input %u: %s (Channel %u, %s)",
                  input_id, input_source.name.c_str(), input_source.channel_number,
                  input_source.enabled ? "enabled" : "disabled");
    }
    
    kodi::Log(ADDON_LOG_INFO, "Channel Status: %zu entries", m_channel_status.size());
    for (const auto& [channel_id, status] : m_channel_status) {
        kodi::Log(ADDON_LOG_INFO, "  Channel %u: %s, signal=%u%%, quality=%u%%",
                  channel_id, status.connected ? "connected" : "disconnected",
                  status.signal_strength, status.signal_quality);
    }
}

// Static helper functions
kodi::addon::PVRChannel ChannelManager::CreateKodiChannel(const InputSource& input, uint32_t unique_id) {
    kodi::addon::PVRChannel channel;
    
    channel.SetUniqueId(unique_id);
    channel.SetChannelNumber(input.channel_number);
    channel.SetSubChannelNumber(input.sub_channel_number);
    channel.SetChannelName(input.display_name.empty() ? input.name : input.display_name);
    channel.SetIconPath(input.icon_path);
    channel.SetIsRadio(false);
    channel.SetIsHidden(!input.enabled);
    channel.SetHasArchive(false);
    
    return channel;
}

InputType ChannelManager::ParseInputType(const std::string& type_name) {
    std::string upper_type = type_name;
    std::transform(upper_type.begin(), upper_type.end(), upper_type.begin(), ::toupper);
    
    if (upper_type == "HDMI") return InputType::HDMI;
    if (upper_type == "COMPONENT") return InputType::COMPONENT;
    if (upper_type == "COMPOSITE") return InputType::COMPOSITE;
    if (upper_type == "SVIDEO") return InputType::SVIDEO;
    
    return InputType::UNKNOWN;
}

std::string ChannelManager::InputTypeToString(InputType type) {
    switch (type) {
        case InputType::HDMI: return "HDMI";
        case InputType::COMPONENT: return "COMPONENT";
        case InputType::COMPOSITE: return "COMPOSITE";
        case InputType::SVIDEO: return "SVIDEO";
        case InputType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

// Private helper methods
bool ChannelManager::LoadDefaultConfiguration() {
    m_settings = ChannelSettings{};
    m_settings.auto_channel_numbering = true;
    m_settings.base_channel_number = 1;
    m_settings.enable_epg = true;
    m_settings.epg_duration_hours = 24;

    // Create default HDMI input
    InputSource hdmi_input;
    hdmi_input.input_id = 0;
    hdmi_input.type = InputType::HDMI;
    hdmi_input.name = "HDMI Input";
    hdmi_input.display_name = "HDMI Input";
    hdmi_input.description = "Primary HDMI input source";
    hdmi_input.enabled = true;
    hdmi_input.auto_detect = true;
    hdmi_input.channel_number = 1;
    hdmi_input.show_osd = true;

    m_input_sources[0] = hdmi_input;
    m_settings.inputs = m_input_sources;

    kodi::Log(ADDON_LOG_INFO, "Loaded default configuration with HDMI input");
    return true;
}

bool ChannelManager::ValidateInputSource(const InputSource& input) const {
    if (input.name.empty()) {
        return false;
    }
    if (input.channel_number == 0) {
        return false;
    }
    if (input.type == InputType::UNKNOWN) {
        return false;
    }
    return true;
}

uint32_t ChannelManager::GenerateChannelId(const InputSource& input) const {
    return input.channel_number;
}

uint32_t ChannelManager::GetNextAvailableChannelNumber() const {
    uint32_t next_number = m_settings.base_channel_number;
    
    while (!IsChannelNumberAvailable(next_number)) {
        next_number++;
        if (next_number > 999) { // Prevent infinite loop
            break;
        }
    }
    
    return next_number;
}

bool ChannelManager::UpdateInputMapping() {
    m_channel_to_input_map.clear();
    
    for (const auto& [input_id, input_source] : m_input_sources) {
        m_channel_to_input_map[input_source.channel_number] = input_id;
    }
    
    return true;
}

void ChannelManager::ClearChannelData() {
    m_input_sources.clear();
    m_channel_to_input_map.clear();  
    m_channel_status.clear();
    m_active_channel_id = 1;
    m_current_input_id = 0;
}

bool ChannelManager::ProbeV4L2Inputs() {
    if (!m_v4l2_device || !m_v4l2_device->IsOpen()) {
        return false;
    }

    std::vector<std::string> input_names = m_v4l2_device->GetInputNames();
    kodi::Log(ADDON_LOG_INFO, "Found %zu V4L2 inputs", input_names.size());

    // Create input sources for detected V4L2 inputs
    for (size_t i = 0; i < input_names.size(); ++i) {
        uint32_t input_id = static_cast<uint32_t>(i);
        
        // Skip if we already have this input configured
        if (m_input_sources.find(input_id) != m_input_sources.end()) {
            continue;
        }

        InputSource input;
        input.input_id = input_id;
        input.type = DetectInputType(input_id);
        input.name = input_names[i];
        input.display_name = input_names[i];
        input.enabled = true;
        input.auto_detect = true;
        input.channel_number = GetNextAvailableChannelNumber();

        m_input_sources[input_id] = input;
        kodi::Log(ADDON_LOG_INFO, "Added V4L2 input: %s (ID=%u, Channel=%u)",
                  input.name.c_str(), input_id, input.channel_number);
    }

    return true;
}

InputType ChannelManager::DetectInputType(uint32_t v4l2_input_id) const {
    // This is a simplified detection - in real implementation,
    // you might query V4L2 input capabilities or use device-specific knowledge
    return InputType::HDMI; // Default to HDMI for now
}

std::string ChannelManager::GenerateInputName(InputType type, uint32_t input_id) const {
    return InputTypeToString(type) + " Input " + std::to_string(input_id);
}

bool ChannelManager::AssignChannelNumbers() {
    uint32_t next_channel = m_settings.base_channel_number;
    
    for (auto& [input_id, input_source] : m_input_sources) {
        if (input_source.channel_number == 0 || !IsChannelNumberAvailable(input_source.channel_number)) {
            input_source.channel_number = next_channel++;
        }
    }
    
    return UpdateInputMapping();
}

bool ChannelManager::IsChannelNumberAvailable(uint32_t channel_number) const {
    for (const auto& [input_id, input_source] : m_input_sources) {
        if (input_source.channel_number == channel_number) {
            return false;
        }
    }
    return true;
}

EpgEntry ChannelManager::CreateBasicEpgEntry(uint32_t channel_id, time_t start_time, time_t duration) const {
    EpgEntry entry;
    entry.channel_id = channel_id;
    entry.start_time = start_time;
    entry.end_time = start_time + duration;
    entry.unique_id = channel_id * 10000 + (start_time % 10000);
    return entry;
}

std::string ChannelManager::GenerateEpgTitle(const InputSource& input, const SignalStatus& status) const {
    if (status.connected && status.signal_locked) {
        if (!status.device_name.empty()) {
            return status.device_name + " via " + input.name;
        }
        if (status.video_format.is_valid()) {
            return "Live Input - " + status.video_format.to_string();
        }
        return "Live Input - " + input.name;
    }
    return "No Signal - " + input.name;
}

std::string ChannelManager::GenerateEpgDescription(const InputSource& input, const SignalStatus& status) const {
    std::ostringstream desc;
    desc << "Input: " << input.name;
    
    if (status.connected && status.signal_locked) {
        desc << "\nStatus: Signal detected";
        desc << "\nSignal strength: " << static_cast<int>(status.signal_strength) << "%";
        desc << "\nSignal quality: " << static_cast<int>(status.signal_quality) << "%";
        
        if (status.video_format.is_valid()) {
            desc << "\nVideo: " << status.video_format.to_string();
        }
        
        if (status.audio_format.is_valid()) {
            desc << "\nAudio: " << status.audio_format.channels << " channels, ";
            desc << status.audio_format.sample_rate << "Hz, ";
            desc << status.audio_format.bit_depth << "-bit";
        }
        
        if (!status.device_name.empty()) {
            desc << "\nDevice: " << status.device_name;
        }
    } else {
        desc << "\nStatus: No signal detected";
    }
    
    if (!input.description.empty()) {
        desc << "\n\n" << input.description;
    }
    
    return desc.str();
}

void ChannelManager::LogInputSources() const {
    kodi::Log(ADDON_LOG_INFO, "=== Input Sources ===");
    for (const auto& [input_id, input_source] : m_input_sources) {
        kodi::Log(ADDON_LOG_INFO, "Input %u: %s (%s) - Channel %u - %s",
                  input_id, input_source.name.c_str(), 
                  InputTypeToString(input_source.type).c_str(),
                  input_source.channel_number,
                  input_source.enabled ? "enabled" : "disabled");
    }
}

void ChannelManager::LogChannelMapping() const {
    kodi::Log(ADDON_LOG_INFO, "=== Channel Mapping ===");
    for (const auto& [channel_id, input_id] : m_channel_to_input_map) {
        kodi::Log(ADDON_LOG_INFO, "Channel %u -> Input %u", channel_id, input_id);
    }
}

std::string ChannelManager::GetStatusString() const {
    std::ostringstream oss;
    oss << "ChannelManager[";
    oss << "initialized=" << (m_initialized.load() ? "true" : "false");
    oss << ", inputs=" << m_input_sources.size();
    oss << ", active_channel=" << m_active_channel_id.load();
    oss << ", current_input=" << m_current_input_id.load();
    oss << "]";
    return oss.str();
}

} // namespace hdmi_pvr