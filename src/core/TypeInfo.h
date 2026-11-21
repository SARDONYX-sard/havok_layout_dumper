#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "RE/Havok/hkClassMember.h"

namespace hkdump {
    /// `hkClassMember::Type` -> serde-hkx `vtype` / `vsubtype` string (`TYPE_VOID` when out of range).
    [[nodiscard]] std::string_view TypeName(std::uint8_t a_type) noexcept;

    /// x64 size of a basic (non-struct) Havok type. 0 when unknown / context dependent.
    [[nodiscard]] std::uint32_t BasicSize(std::uint8_t a_type) noexcept;

    /// x64 size of a single element of the member (C-style array length not applied).
    [[nodiscard]] std::uint32_t ElementSize(const RE::hkClassMember& a_member) noexcept;

    /// serde-hkx `ctype` string, e.g. `hkArray<struct hkbFoo>`, `hkReal[4]`, `enum Mode`.
    [[nodiscard]] std::string MakeCType(const RE::hkClassMember& a_member);

    /// serde-hkx `flags` string, e.g. `FLAGS_NONE|ALIGN_16|SERIALIZE_IGNORED`.
    [[nodiscard]] std::string MakeFlags(std::uint16_t a_flags);
} // namespace hkdump
