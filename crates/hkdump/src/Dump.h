#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ClassSerializer.h"
#include "MemoryView.h"
#include "Model.h"

namespace hkdump {
    struct DumpEntry {
        std::reference_wrapper<const ClassInfo> klass;
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

    /// Serializes every class of `a_db` and assigns unique file names.
    /// Duplicates get `_<hex RVA>` appended (address - `a_view.ImageBase()`).
    [[nodiscard]] std::vector<DumpEntry>
    BuildDump(const ClassDb& a_db, const MemoryView& a_view, JsonFormat a_format = JsonFormat::SerdeHkx);

    /// Writes entries into `a_outDir` (created if missing).
    [[nodiscard]] WriteResult WriteDump(const std::filesystem::path& a_outDir, std::span<const DumpEntry> a_entries);
} // namespace hkdump
