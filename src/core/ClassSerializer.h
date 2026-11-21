#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>

#include "ClassRef.h"
#include "MemoryRange.h"

namespace hkdump {
    /// Converts `hkClass` reflection data into serde-hkx class JSON
    /// (`serde-hkx/assets/classes/*.json` format).
    ///
    /// Fallout 4 is x64 only: `*_x86` fields are always 0.
    class ClassSerializer {
    public:
        /// `a_all` is the whole class set: used to infer the storage type of declared enums
        /// from members that use them (also across classes).
        ClassSerializer(std::span<const ClassRef> a_all, const MemoryRange& a_range);

        [[nodiscard]] std::string ToJson(const RE::hkClass& a_class);

        /// Root class implements an interface (= has a vtable).
        [[nodiscard]] static bool HasVTable(const RE::hkClass& a_class) noexcept;

        /// Class (incl. parents) contains `CString` / `StringPtr`, directly or via embedded structs.
        [[nodiscard]] bool ClassHasString(const RE::hkClass& a_class);
        [[nodiscard]] bool MemberHasString(const RE::hkClassMember& a_member);

        /// Declared default of a member (`hkClass::m_defaults`), 0 if none.
        /// Integers/enums as value, `hkReal` / `hkHalf` as raw bits.
        [[nodiscard]] std::int64_t
        DefaultValue(const RE::hkClass& a_class, std::size_t a_index, const RE::hkClassMember& a_member) const;

    private:
        MemoryRange _range;
        // keyed by table identity (address); never dereferenced through the key
        std::unordered_map<const RE::hkClass*, bool> _hasString;
        // enum -> (vtype, vsubtype) of a member that uses it
        std::unordered_map<const RE::hkClassEnum*, std::pair<std::uint8_t, std::uint8_t>> _enumUsage;
    };
} // namespace hkdump
