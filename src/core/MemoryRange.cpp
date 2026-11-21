#include "MemoryRange.h"

namespace hkdump {
    MemoryRange MemoryRange::FromPeImage(std::uintptr_t a_base) noexcept {
        // PE32+: e_lfanew @ 0x3C, OptionalHeader @ NT + 0x18, SizeOfImage @ Optional + 0x38
        const auto ntOff = *reinterpret_cast<const std::int32_t*>(a_base + 0x3C);
        const auto sizeOfImage = *reinterpret_cast<const std::uint32_t*>(a_base + ntOff + 0x18 + 0x38);
        return {a_base, a_base + sizeOfImage};
    }

    bool MemoryRange::is_ident(const char* a_str, std::size_t a_maxLen) const noexcept {
        if (!readable(a_str)) {
            return false;
        }

        const auto is_head = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
        const auto is_tail = [&](char c) { return is_head(c) || (c >= '0' && c <= '9') || c == ':'; };

        for (std::size_t i = 0; i < a_maxLen; ++i) {
            if (!contains(a_str + i)) {
                return false;
            }
            const char c = a_str[i];
            if (c == '\0') {
                return i > 0;
            }
            if (i == 0 ? !is_head(c) : !is_tail(c)) {
                return false;
            }
        }
        return false;
    }
} // namespace hkdump
