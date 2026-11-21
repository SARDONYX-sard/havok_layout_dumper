#pragma once

#include <string_view>

#include "XSE.h"

namespace plugin {
    /// Static description of one xSE plugin build.
    struct Config {
        std::string_view name; // plugin / log file name
        const char* sender;    // messaging sender: "F4SE" / "SKSE" (string literal: the loader may keep it)
        std::uint32_t messagingInterfaceId;
        std::uint32_t readyMessage; // message after which the Havok tables exist (main menu / data loaded)
    };

    /// Common `xSEPlugin_Load` body: snapshot on `readyMessage` (or right away without messaging).
    bool Load(const Config& a_config, const xse::Interface* a_xse);

    /// Writes `<game>/Data/hkdump_snapshots/<exe>.hksnap` (`.rdata` / `.data` of the game exe).
    ///
    /// Under `Data/` on purpose: mod managers (MO2 VFS) redirect it to their overwrite folder.
    /// Decode it offline with `hkdump <snapshot> <out dir>`.
    void TakeSnapshot(std::string_view a_pluginName);
} // namespace plugin
