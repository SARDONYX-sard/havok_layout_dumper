#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "MemoryView.h"
#include "Model.h"

namespace hkdump {
    /// Converts decoded `hkClass` data into serde-hkx class JSON (`serde-hkx/assets/classes/*.json`).
    ///
    /// The snapshot architecture decides which layout fields are filled: x64 snapshots fill
    /// `*_x86_64` (Skyrim SE / Fallout 4), x86 snapshots fill `*_x86` (Skyrim LE); the other side is 0.
    class ClassSerializer {
    public:
        /// `a_view` is used to read `hkClass::m_defaults`.
        ClassSerializer(const ClassDb& a_db, const MemoryView& a_view);

        [[nodiscard]] std::string ToJson(const ClassInfo& a_class);

        /// Root class implements an interface (= has a vtable).
        [[nodiscard]] bool HasVTable(const ClassInfo& a_class) const;

        /// Class (incl. parents) contains `CString` / `StringPtr`, directly or via embedded structs.
        [[nodiscard]] bool ClassHasString(const ClassInfo& a_class);
        [[nodiscard]] bool MemberHasString(const MemberInfo& a_member);

        /// Declared default of a member (`hkClass::m_defaults`), 0 if none.
        /// Integers/enums as value, `hkReal` / `hkHalf` as raw bits.
        [[nodiscard]] std::int64_t DefaultValue(const ClassInfo& a_class, std::size_t a_index) const;

    private:
        std::reference_wrapper<const ClassDb> _db;
        std::reference_wrapper<const MemoryView> _view;
        std::unordered_map<std::uint64_t, bool> _hasString;
        // enum address -> (vtype, vsubtype) of a member that uses it
        std::unordered_map<std::uint64_t, std::pair<std::uint8_t, std::uint8_t>> _enumUsage;
    };
} // namespace hkdump
