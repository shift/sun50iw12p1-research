/*
 *  HDMI Input PVR Client for HY300 Projector
 *  Copyright (C) 2025 HY300 Project
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "hdmi_client.h"
#include <kodi/addon-instance/PVR.h>
#include <kodi/General.h>
#include <memory>

// Global HDMI client instance
std::unique_ptr<hdmi_pvr::HdmiClient> g_hdmi_client;

class ATTR_DLL_LOCAL CHdmiInputPVR : public kodi::addon::CAddonBase,
                                     public kodi::addon::CInstancePVR
{
public:
    CHdmiInputPVR() = default;
    ~CHdmiInputPVR() override = default;

    ADDON_STATUS Create() override
    {
        kodi::Log(ADDON_LOG_INFO, "HDMI Input PVR Client starting...");
        
        try {
            g_hdmi_client = std::make_unique<hdmi_pvr::HdmiClient>();
            if (!g_hdmi_client->Initialize()) {
                kodi::Log(ADDON_LOG_ERROR, "Failed to initialize HDMI client");
                return ADDON_STATUS_PERMANENT_FAILURE;
            }
            
            kodi::Log(ADDON_LOG_INFO, "HDMI Input PVR Client started successfully");
            return ADDON_STATUS_OK;
        }
        catch (const std::exception& e) {
            kodi::Log(ADDON_LOG_ERROR, "Exception during startup: %s", e.what());
            return ADDON_STATUS_PERMANENT_FAILURE;
        }
    }

    void Destroy() override
    {
        kodi::Log(ADDON_LOG_INFO, "HDMI Input PVR Client shutting down...");
        
        if (g_hdmi_client) {
            g_hdmi_client->Shutdown();
            g_hdmi_client.reset();
        }
        
        kodi::Log(ADDON_LOG_INFO, "HDMI Input PVR Client shutdown complete");
    }

    ADDON_STATUS SetSetting(const std::string& settingName, const kodi::addon::CSettingValue& settingValue) override
    {
        if (g_hdmi_client) {
            return g_hdmi_client->SetSetting(settingName, settingValue) ? ADDON_STATUS_OK : ADDON_STATUS_NEED_RESTART;
        }
        return ADDON_STATUS_OK;
    }

    // PVR Client capabilities
    PVR_ERROR GetCapabilities(kodi::addon::PVRCapabilities& capabilities) override
    {
        capabilities.SetSupportsEPG(true);
        capabilities.SetSupportsTV(true);
        capabilities.SetSupportsRadio(false);
        capabilities.SetSupportsChannelGroups(false);
        capabilities.SetSupportsRecordings(false);
        capabilities.SetSupportsTimers(false);
        capabilities.SetSupportsChannelScan(false);
        capabilities.SetSupportsChannelSettings(true);
        capabilities.SetSupportsLastPlayedPosition(false);
        capabilities.SetHandlesInputStream(true);
        capabilities.SetHandlesDemuxing(true);
        
        return PVR_ERROR_NO_ERROR;
    }

    PVR_ERROR GetBackendName(std::string& name) override
    {
        name = "HY300 HDMI Input";
        return PVR_ERROR_NO_ERROR;
    }

    PVR_ERROR GetBackendVersion(std::string& version) override
    {
        version = "1.0.0";
        return PVR_ERROR_NO_ERROR;
    }

    PVR_ERROR GetBackendHostname(std::string& hostname) override
    {
        hostname = "localhost";
        return PVR_ERROR_NO_ERROR;
    }

    PVR_ERROR GetConnectionString(std::string& connection) override
    {
        connection = "HDMI Input V4L2 Device";
        return PVR_ERROR_NO_ERROR;
    }

    // Channel operations
    PVR_ERROR GetChannelsAmount(int& amount) override
    {
        if (!g_hdmi_client) {
            return PVR_ERROR_SERVER_ERROR;
        }
        
        amount = g_hdmi_client->GetChannelCount();
        return PVR_ERROR_NO_ERROR;
    }

    PVR_ERROR GetChannels(bool radio, kodi::addon::PVRChannelsResultSet& results) override
    {
        if (!g_hdmi_client || radio) {
            return PVR_ERROR_NO_ERROR;  // No radio channels
        }

        return g_hdmi_client->GetChannels(results);
    }

    // EPG operations
    PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, kodi::addon::PVREPGTagsResultSet& results) override
    {
        if (!g_hdmi_client) {
            return PVR_ERROR_SERVER_ERROR;
        }

        return g_hdmi_client->GetEPGForChannel(channelUid, start, end, results);
    }

    // Stream operations
    bool OpenLiveStream(const kodi::addon::PVRChannel& channel) override
    {
        if (!g_hdmi_client) {
            return false;
        }

        return g_hdmi_client->OpenLiveStream(channel);
    }

    void CloseLiveStream() override
    {
        if (g_hdmi_client) {
            g_hdmi_client->CloseLiveStream();
        }
    }

    int ReadLiveStream(unsigned char* buffer, unsigned int size) override
    {
        if (!g_hdmi_client) {
            return -1;
        }

        return g_hdmi_client->ReadLiveStream(buffer, size);
    }

    long long SeekLiveStream(long long position, int whence) override
    {
        // Live streams don't support seeking
        return -1;
    }

    long long LengthLiveStream() override
    {
        // Live streams have no defined length
        return -1;
    }

    PVR_ERROR GetStreamProperties(std::vector<kodi::addon::PVRStreamProperty>& properties) override
    {
        if (!g_hdmi_client) {
            return PVR_ERROR_SERVER_ERROR;
        }

        return g_hdmi_client->GetStreamProperties(properties);
    }

    PVR_ERROR GetSignalStatus(int channelUid, kodi::addon::PVRSignalStatus& signalStatus) override
    {
        if (!g_hdmi_client) {
            return PVR_ERROR_SERVER_ERROR;
        }

        return g_hdmi_client->GetSignalStatus(channelUid, signalStatus);
    }

    // Demux operations for hardware-accelerated streaming
    bool OpenDemuxStream(const kodi::addon::PVRChannel& channel) override
    {
        if (!g_hdmi_client) {
            return false;
        }

        return g_hdmi_client->OpenDemuxStream(channel);
    }

    void CloseDemuxStream() override
    {
        if (g_hdmi_client) {
            g_hdmi_client->CloseDemuxStream();
        }
    }

    DEMUX_PACKET* DemuxRead() override
    {
        if (!g_hdmi_client) {
            return nullptr;
        }

        return g_hdmi_client->DemuxRead();
    }

    void DemuxAbort() override
    {
        if (g_hdmi_client) {
            g_hdmi_client->DemuxAbort();
        }
    }

    void DemuxFlush() override
    {
        if (g_hdmi_client) {
            g_hdmi_client->DemuxFlush();
        }
    }

    void DemuxReset() override
    {
        if (g_hdmi_client) {
            g_hdmi_client->DemuxReset();
        }
    }

    // Menu hook for HDMI input settings
    PVR_ERROR CallMenuHook(const kodi::addon::PVRMenuhook& menuhook, const kodi::addon::PVRChannel& channel) override
    {
        if (!g_hdmi_client) {
            return PVR_ERROR_SERVER_ERROR;
        }

        return g_hdmi_client->CallMenuHook(menuhook, channel);
    }
};

// Kodi addon entry points
ADDONCREATOR(CHdmiInputPVR)