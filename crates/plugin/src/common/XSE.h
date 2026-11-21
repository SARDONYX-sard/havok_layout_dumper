#pragma once

// Plugin ABI shared by F4SE, SKSE64 and SKSE (LE): the parts this project uses have the same
// shape in all three (see f4se/skse64/skse PluginAPI.h). IDs and version data differ per loader.

#include <cstdint>

namespace xse {
    using PluginHandle = std::uint32_t;

    /// F4SEInterface / SKSEInterface (prefix common to all versions).
    struct Interface {
        std::uint32_t xseVersion;
        std::uint32_t runtimeVersion;
        std::uint32_t editorVersion;
        std::uint32_t isEditor;
        void*(__cdecl* QueryInterface)(std::uint32_t a_id);
        PluginHandle(__cdecl* GetPluginHandle)();
        std::uint32_t(__cdecl* GetReleaseIndex)();
    };

    struct Message {
        const char* sender;
        std::uint32_t type;
        std::uint32_t dataLen;
        void* data;
    };

    using EventCallback = void(__cdecl*)(Message* a_msg);

    struct MessagingInterface {
        std::uint32_t interfaceVersion;
        bool(__cdecl* RegisterListener)(PluginHandle a_listener, const char* a_sender, EventCallback a_handler);
    };

    /// `xSEPlugin_Query` output (pre-NG F4SE, pre-AE SKSE64, SKSE LE).
    struct PluginInfo {
        std::uint32_t infoVersion; // 1
        const char* name;
        std::uint32_t version;
    };
} // namespace xse
