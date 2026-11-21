#include "PeImage.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace hkdump {
    namespace {
        template<class T>
        T read_at(std::uintptr_t a_addr) noexcept {
            T value{};
            std::memcpy(&value, reinterpret_cast<const void*>(a_addr), sizeof(T));
            return value;
        }

        constexpr std::uint16_t kDosMagic = 0x5A4D;    // "MZ"
        constexpr std::uint32_t kNtSignature = 0x4550; // "PE\0\0"
        constexpr std::uint16_t kPe32Magic = 0x10B;
        constexpr std::uint16_t kPe32PlusMagic = 0x20B;
        constexpr std::size_t kSectionHeaderSize = 40;
    } // namespace

    const PeSection* PeImage::FindSection(std::string_view a_name) const noexcept {
        const auto it = std::ranges::find(sections, a_name, &PeSection::name);
        return it != sections.end() ? &*it : nullptr;
    }

    std::optional<PeImage> ReadPeImage(std::uintptr_t a_base) {
        if (a_base == 0 || read_at<std::uint16_t>(a_base) != kDosMagic) {
            return std::nullopt;
        }

        const auto nt = a_base + read_at<std::uint32_t>(a_base + 0x3C); // e_lfanew
        if (read_at<std::uint32_t>(nt) != kNtSignature) {
            return std::nullopt;
        }

        // IMAGE_FILE_HEADER @ nt + 4
        const auto numSections = read_at<std::uint16_t>(nt + 4 + 2);
        const auto optionalSize = read_at<std::uint16_t>(nt + 4 + 16);
        const auto optional = nt + 4 + 20;

        PeImage image;
        image.base = a_base;

        const auto magic = read_at<std::uint16_t>(optional);
        if (magic != kPe32Magic && magic != kPe32PlusMagic) {
            return std::nullopt;
        }
        image.is64 = magic == kPe32PlusMagic;
        image.sizeOfImage = read_at<std::uint32_t>(optional + 56); // same offset for PE32 / PE32+

        auto header = optional + optionalSize;
        for (std::uint16_t i = 0; i < numSections; ++i, header += kSectionHeaderSize) {
            const auto rawName = read_at<std::array<char, 8>>(header);
            const auto len = static_cast<std::size_t>(std::ranges::find(rawName, '\0') - rawName.begin());
            image.sections.push_back({
                .name = std::string{rawName.data(), len},
                .rva = read_at<std::uint32_t>(header + 12),
                .virtualSize = read_at<std::uint32_t>(header + 8),
            });
        }
        return image;
    }
} // namespace hkdump
