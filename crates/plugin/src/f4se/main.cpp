// F4SE plugin (Fallout 4 / VR, x64): snapshot of Fallout4.exe at the main menu.
// Works with pre-NG (F4SEPlugin_Query) and NG (F4SEPlugin_Version) loaders.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>

#include "SnapshotPlugin.h"
#include "XSE.h"

#ifndef HKDUMP_PLUGIN_NAME
#define HKDUMP_PLUGIN_NAME "hkdump_f4se"
#endif

namespace {
    constexpr std::uint32_t kPluginVersion = 1;

    constexpr plugin::Config kConfig{
        .name = HKDUMP_PLUGIN_NAME,
        .sender = "F4SE",
        .messagingInterfaceId = 1, // kInterface_Messaging
        .readyMessage = 10,        // kMessage_GameDataReady (main menu)
    };

    /// F4SE (NG, 0.7+) `F4SEPluginVersionData`.
    struct PluginVersionData {
        std::uint32_t dataVersion;
        std::uint32_t pluginVersion;
        char pluginName[256];
        char author[256];
        std::uint32_t addressIndependence;
        std::uint32_t structureIndependence;
        std::uint32_t compatibleVersions[16];
        std::uint32_t xseMinimum;
        std::uint32_t reservedNonBreaking;
        std::uint32_t reservedBreaking;
        std::uint8_t reserved[512];
    };
    static_assert(offsetof(PluginVersionData, addressIndependence) == 0x208);
    static_assert(sizeof(PluginVersionData) == 0x45C);

    constexpr PluginVersionData MakeVersionData() noexcept {
        PluginVersionData data{};
        data.dataVersion = 1;
        data.pluginVersion = kPluginVersion;
        std::ranges::copy(kConfig.name, std::begin(data.pluginName));
        std::ranges::copy(std::string_view{"SARDONYX"}, std::begin(data.author));
        // Only reads its own image: independent of runtime addresses / game structures.
        data.addressIndependence = 1 << 0;   // kAddressIndependence_Signatures
        data.structureIndependence = 1 << 0; // kStructureIndependence_NoStructs
        return data;
    }
} // namespace

// NOLINTBEGIN(misc-use-internal-linkage, readability-identifier-naming): F4SE entry points
extern "C" __declspec(dllexport) constinit const PluginVersionData F4SEPlugin_Version = MakeVersionData();

extern "C" __declspec(dllexport) bool F4SEPlugin_Query(const xse::Interface* /*a_f4se*/, xse::PluginInfo* a_info) {
    a_info->infoVersion = 1;
    a_info->name = kConfig.name.data();
    a_info->version = kPluginVersion;
    return true;
}

extern "C" __declspec(dllexport) bool F4SEPlugin_Load(const xse::Interface* a_f4se) {
    return plugin::Load(kConfig, a_f4se);
}
// NOLINTEND(misc-use-internal-linkage, readability-identifier-naming)
