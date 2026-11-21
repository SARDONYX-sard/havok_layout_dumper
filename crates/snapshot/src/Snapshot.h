#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hkdump {
    /// Raw copy of parts of a loaded PE image (e.g. `.rdata` / `.data` after static initialization).
    ///
    /// Written in-game by the xSE plugins, decoded offline (any platform) by `hkdump`.
    ///
    /// File format (little endian):
    ///   "HKSNAP01" | u64 imageBase | u64 imageSize | u32 regionCount | u32 pointerSize
    ///   regionCount x { u64 rva | u64 size | u8[size] bytes }
    /// `pointerSize` is 4 (x86, e.g. Skyrim LE) or 8 (x64); 0 in old files means 8.
    struct ImageSnapshot {
        struct Region {
            std::uint64_t rva{0};
            std::vector<std::byte> bytes{};
        };

        std::uint64_t imageBase{0};
        std::uint64_t imageSize{0};
        std::uint32_t pointerSize{8};
        std::vector<Region> regions{};
    };

    /// A region of the live image to save: `bytes` must lie at `imageBase + rva`.
    struct SnapshotSource {
        std::uint64_t rva{0};
        std::span<const std::byte> bytes{};
    };

    /// Writes a snapshot file. Returns an error message on failure.
    [[nodiscard]] std::optional<std::string> WriteImageSnapshot(
        const std::filesystem::path& a_path,
        const ImageSnapshot& a_header, // regions are ignored: taken from `a_regions`
        std::span<const SnapshotSource> a_regions
    );

    /// Sections saved by `WriteModuleSnapshot` by default: read-only data + (BSS backed) data.
    inline constexpr std::array<std::string_view, 2> kDefaultSnapshotSections{".rdata", ".data"};

    /// Snapshots the given sections of a module mapped in this process (`a_moduleBase` = HMODULE).
    /// Call after static initialization (runtime-built reflection tables live in `.data` BSS).
    [[nodiscard]] std::optional<std::string> WriteModuleSnapshot(
        std::uintptr_t a_moduleBase,
        const std::filesystem::path& a_path,
        std::span<const std::string_view> a_sections = kDefaultSnapshotSections
    );

    /// Reads a snapshot file (nullopt when missing / malformed).
    [[nodiscard]] std::optional<ImageSnapshot> ReadImageSnapshot(const std::filesystem::path& a_path);
} // namespace hkdump
