#pragma once

#include <filesystem>
#include <string>

namespace hkdump {
    /// UTF-8 representation of a path for messages / logs.
    ///
    /// `path.string()` converts to the ANSI code page: non-ASCII directories (e.g. "ドキュメント")
    /// become mojibake in UTF-8 logs, or throw when not representable.
    [[nodiscard]] inline std::string ToUtf8(const std::filesystem::path& a_path) {
        const auto u8 = a_path.u8string();
        return {reinterpret_cast<const char*>(u8.data()), u8.size()};
    }
} // namespace hkdump
