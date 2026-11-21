// SKSE plugin (Skyrim LE, x86): snapshot of TESV.exe once data is loaded.
// SKSE 1.7.x only uses SKSEPlugin_Query / SKSEPlugin_Load (cdecl, 32-bit).

#include <cstdint>

#include "SnapshotPlugin.h"
#include "XSE.h"

#ifndef HKDUMP_PLUGIN_NAME
#define HKDUMP_PLUGIN_NAME "hkdump_skse"
#endif

namespace {
    constexpr std::uint32_t kPluginVersion = 1;

    constexpr plugin::Config kConfig{
        .name = HKDUMP_PLUGIN_NAME,
        .sender = "SKSE",
        .messagingInterfaceId = 5, // kInterface_Messaging
        .readyMessage = 8,         // kMessage_DataLoaded
    };
} // namespace

// NOLINTBEGIN(misc-use-internal-linkage, readability-identifier-naming): SKSE entry points
extern "C" __declspec(dllexport) bool __cdecl
SKSEPlugin_Query(const xse::Interface* /*a_skse*/, xse::PluginInfo* a_info) {
    a_info->infoVersion = 1;
    a_info->name = kConfig.name.data();
    a_info->version = kPluginVersion;
    return true;
}

extern "C" __declspec(dllexport) bool __cdecl SKSEPlugin_Load(const xse::Interface* a_skse) {
    return plugin::Load(kConfig, a_skse);
}
// NOLINTEND(misc-use-internal-linkage, readability-identifier-naming)
