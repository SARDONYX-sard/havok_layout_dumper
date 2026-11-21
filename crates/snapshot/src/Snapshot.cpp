#include "Snapshot.h"

#include <format>
#include <fstream>

#include "PathUtf8.h"
#include "PeImage.h"

namespace hkdump {
    namespace {
        constexpr std::string_view kMagic = "HKSNAP01";

        template<class T>
        void write_pod(std::ofstream& a_out, const T& a_value) {
            a_out.write(reinterpret_cast<const char*>(&a_value), sizeof(T));
        }

        template<class T>
        bool read_pod(std::ifstream& a_in, T& a_value) {
            return static_cast<bool>(a_in.read(reinterpret_cast<char*>(&a_value), sizeof(T)));
        }
    } // namespace

    std::optional<std::string> WriteImageSnapshot(
        const std::filesystem::path& a_path, const ImageSnapshot& a_header, std::span<const SnapshotSource> a_regions
    ) {
        std::error_code ec;
        if (a_path.has_parent_path()) {
            std::filesystem::create_directories(a_path.parent_path(), ec);
            if (ec) {
                return std::format("failed to create {}: {}", ToUtf8(a_path.parent_path()), ec.message());
            }
        }

        std::ofstream out{a_path, std::ios::binary | std::ios::trunc};
        if (!out) {
            return std::format("failed to open {}", ToUtf8(a_path));
        }

        out.write(kMagic.data(), static_cast<std::streamsize>(kMagic.size()));
        write_pod(out, a_header.imageBase);
        write_pod(out, a_header.imageSize);
        write_pod(out, static_cast<std::uint32_t>(a_regions.size()));
        write_pod(out, a_header.pointerSize);
        for (const auto& region : a_regions) {
            write_pod(out, region.rva);
            write_pod(out, static_cast<std::uint64_t>(region.bytes.size()));
            out.write(
                reinterpret_cast<const char*>(region.bytes.data()), static_cast<std::streamsize>(region.bytes.size())
            );
        }

        if (!out) {
            return std::format("failed to write {}", ToUtf8(a_path));
        }
        return std::nullopt;
    }

    std::optional<std::string> WriteModuleSnapshot(
        std::uintptr_t a_moduleBase, const std::filesystem::path& a_path, std::span<const std::string_view> a_sections
    ) {
        const auto image = ReadPeImage(a_moduleBase);
        if (!image) {
            return std::format("not a PE image: {:#x}", a_moduleBase);
        }

        std::vector<SnapshotSource> regions;
        for (const auto name : a_sections) {
            const auto* const section = image->FindSection(name);
            if (section == nullptr) {
                return std::format("section {} not found", name);
            }
            regions.push_back({
                .rva = section->rva,
                .bytes = {reinterpret_cast<const std::byte*>(a_moduleBase + section->rva), section->virtualSize},
            });
        }

        const ImageSnapshot header{
            .imageBase = a_moduleBase,
            .imageSize = image->sizeOfImage,
            .pointerSize = sizeof(void*),
        };
        return WriteImageSnapshot(a_path, header, regions);
    }

    std::optional<ImageSnapshot> ReadImageSnapshot(const std::filesystem::path& a_path) {
        std::ifstream in{a_path, std::ios::binary};
        std::array<char, kMagic.size()> magic{};
        if (!in || !in.read(magic.data(), magic.size()) || std::string_view{magic.data(), magic.size()} != kMagic) {
            return std::nullopt;
        }

        ImageSnapshot snapshot;
        std::uint32_t count = 0;
        if (!read_pod(in, snapshot.imageBase) || !read_pod(in, snapshot.imageSize) || !read_pod(in, count)
            || !read_pod(in, snapshot.pointerSize)) {
            return std::nullopt;
        }
        if (snapshot.pointerSize == 0) {
            snapshot.pointerSize = 8; // written before the field existed (x64 only)
        }
        if (snapshot.pointerSize != 4 && snapshot.pointerSize != 8) {
            return std::nullopt;
        }

        for (std::uint32_t i = 0; i < count; ++i) {
            ImageSnapshot::Region region;
            std::uint64_t size = 0;
            if (!read_pod(in, region.rva) || !read_pod(in, size) || size > snapshot.imageSize) {
                return std::nullopt;
            }
            region.bytes.resize(static_cast<std::size_t>(size));
            if (!in.read(reinterpret_cast<char*>(region.bytes.data()), static_cast<std::streamsize>(size))) {
                return std::nullopt;
            }
            snapshot.regions.push_back(std::move(region));
        }
        return snapshot;
    }
} // namespace hkdump
