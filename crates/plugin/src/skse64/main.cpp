// SKSE64 plugin (Skyrim SE / AE / VR, x64): snapshot of SkyrimSE.exe once data is loaded.
// Works with pre-AE (SKSEPlugin_Query) and AE (SKSEPlugin_Version) loaders.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

#include "SnapshotPlugin.h"
#include "XSE.h"

#ifndef HKDUMP_PLUGIN_NAME
#define HKDUMP_PLUGIN_NAME "hkdump_skse64"
#endif

namespace {
    constexpr std::uint32_t kPluginVersion = 1;

    constexpr plugin::Config kConfig{
        .name = HKDUMP_PLUGIN_NAME,
        .sender = "SKSE",
        .messagingInterfaceId = 5, // kInterface_Messaging
        .readyMessage = 8,         // kMessage_DataLoaded
    };

    /// SKSE64 (AE) `SKSEPluginVersionData`.
    struct PluginVersionData {
        std::uint32_t dataVersion;
        std::uint32_t pluginVersion;
        char name[256];
        char author[256];
        char supportEmail[252];
        std::uint32_t versionIndependenceEx;
        std::uint32_t versionIndependence;
        std::uint32_t compatibleVersions[16];
        std::uint32_t seVersionRequired;
    };
    static_assert(offsetof(PluginVersionData, versionIndependenceEx) == 0x304);
    static_assert(sizeof(PluginVersionData) == 0x350);

    constexpr PluginVersionData MakeVersionData() noexcept {
        PluginVersionData data{};
        data.dataVersion = 1;
        data.pluginVersion = kPluginVersion;
        std::ranges::copy(kConfig.name, std::begin(data.name));
        std::ranges::copy(std::string_view{"SARDONYX"}, std::begin(data.author));
        // Only reads its own image: independent of runtime addresses / game structures.
        data.versionIndependenceEx = 1 << 0; // kVersionIndependentEx_NoStructUse
        data.versionIndependence = 1 << 1;   // kVersionIndependent_Signatures
        return data;
    }
} // namespace

// NOLINTBEGIN(misc-use-internal-linkage, readability-identifier-naming): SKSE entry points
extern "C" __declspec(dllexport) constinit const PluginVersionData SKSEPlugin_Version = MakeVersionData();

extern "C" __declspec(dllexport) bool SKSEPlugin_Query(const xse::Interface* /*a_skse*/, xse::PluginInfo* a_info) {
    a_info->infoVersion = 1;
    a_info->name = kConfig.name.data();
    a_info->version = kPluginVersion;
    return true;
}

extern "C" __declspec(dllexport) bool SKSEPlugin_Load(const xse::Interface* a_skse) {
    return plugin::Load(kConfig, a_skse);
}
// NOLINTEND(misc-use-internal-linkage, readability-identifier-naming)
