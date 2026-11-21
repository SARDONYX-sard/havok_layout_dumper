#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ClassRef.h"
#include "MemoryRange.h"

namespace hkdump {
    struct DumpEntry {
        ClassRef klass;
        std::string fileName{}; // `<ClassName>.json` (sanitized, unique)
        std::string json{};
        bool renamed{false}; // name collided with an earlier class -> `_<offset>` suffix added
    };

    struct WriteResult {
        std::size_t written{0};
        std::vector<std::string> errors{};
    };

    /// Replaces characters that are invalid in Windows file names with `_`.
    [[nodiscard]] std::string SanitizeFileName(std::string_view a_name);

    /// Serializes every class and assigns unique file names.
    /// Duplicates get `_<hex offset from a_range.lo()>` appended (RVA for a PE image range).
    [[nodiscard]] std::vector<DumpEntry> BuildDump(std::span<const ClassRef> a_classes, const MemoryRange& a_range);

    /// Writes entries into `a_outDir` (created if missing).
    [[nodiscard]] WriteResult WriteDump(const std::filesystem::path& a_outDir, std::span<const DumpEntry> a_entries);
} // namespace hkdump
