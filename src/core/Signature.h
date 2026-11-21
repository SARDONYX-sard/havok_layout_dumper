#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "RE/Havok/hkClass.h"

namespace hkdump {
    /// Standard CRC32 (poly 0xEDB88320)
    class Crc32Writer {
    public:
        void write(std::span<const std::byte> a_data) noexcept;
        void write(std::string_view a_str) noexcept { write(std::as_bytes(std::span{a_str})); } // without NUL
        void write16(std::uint16_t a_value) noexcept { write(std::as_bytes(std::span{&a_value, 1})); }
        void write32(std::uint32_t a_value) noexcept { write(std::as_bytes(std::span{&a_value, 1})); }

        [[nodiscard]] std::uint32_t get() const noexcept { return ~_crc; }

    private:
        std::uint32_t _crc{0xFFFFFFFF};
    };

    /// (class + all parents) verified against the serde-hkx class set.
    [[nodiscard]] std::uint32_t Signature(const RE::hkClass& a_class);
} // namespace hkdump
