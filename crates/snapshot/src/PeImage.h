#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hkdump {
    /// Section of a mapped PE image.
    struct PeSection {
        std::string name{};           // e.g. ".rdata"
        std::uint32_t rva{0};         // VirtualAddress
        std::uint32_t virtualSize{0}; // Misc.VirtualSize (includes zero-initialized BSS)
    };

    /// Headers of a PE image mapped by the Windows loader (PE32 or PE32+).
    struct PeImage {
        std::uintptr_t base{0};
        std::uint32_t sizeOfImage{0};
        bool is64{false};
        std::vector<PeSection> sections{};

        [[nodiscard]] const PeSection* FindSection(std::string_view a_name) const noexcept;
    };

    /// Reads the headers of a mapped image (`base` = HMODULE). nullopt when not a PE image.
    [[nodiscard]] std::optional<PeImage> ReadPeImage(std::uintptr_t a_base);
} // namespace hkdump
