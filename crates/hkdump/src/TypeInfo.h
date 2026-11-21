#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "Model.h"

namespace hkdump {
    /// `hkClassMember::Type` -> serde-hkx `vtype` / `vsubtype` string (`TYPE_VOID` when out of range).
    [[nodiscard]] std::string_view TypeName(std::uint8_t a_type) noexcept;

    /// Size of a basic (non-struct) Havok type for the target pointer size. 0 when unknown.
    [[nodiscard]] std::uint32_t BasicSize(std::uint8_t a_type, std::uint32_t a_pointerSize = 8) noexcept;

    /// Size of a single element of the member (C-style array length not applied).
    [[nodiscard]] std::uint32_t ElementSize(const MemberInfo& a_member, const ClassDb& a_db);

    /// serde-hkx `ctype` string, e.g. `hkArray<struct hkbFoo>`, `hkReal[4]`, `enum Mode`.
    [[nodiscard]] std::string MakeCType(const MemberInfo& a_member, const ClassDb& a_db);

    /// serde-hkx `flags` string, e.g. `FLAGS_NONE|ALIGN_16|SERIALIZE_IGNORED`.
    [[nodiscard]] std::string MakeFlags(std::uint16_t a_flags);
} // namespace hkdump
