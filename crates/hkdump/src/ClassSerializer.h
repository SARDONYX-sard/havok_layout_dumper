#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "MemoryView.h"
#include "Model.h"

namespace hkdump {
    /// Class JSON flavor. Both share the layout; they differ in the reference-tracking fields and `flags`.
    enum class JsonFormat : std::uint8_t {
        /// serde-hkx `assets/classes/*.json`: `has_string` / `parent_has_string`, `flags` with `FLAGS_NONE`.
        SerdeHkx,
        /// d-merge-serde-hkx `assets/classes/*.json` (havok_class_info_gen, `nemesis` feature):
        /// `has_ref` / `parent_has_ref` (class level always true), `flags` as Rust `bitflags` (`0` when empty).
        DMerge,
    };

    /// `serde-hkx` / `d-merge` -> format.
    [[nodiscard]] std::optional<JsonFormat> ParseJsonFormat(std::string_view a_name) noexcept;

    /// Converts decoded `hkClass` data into class JSON of `JsonFormat`.
    ///
    /// The snapshot architecture decides which layout fields are filled: x64 snapshots fill
    /// `*_x86_64` (Skyrim SE / Fallout 4), x86 snapshots fill `*_x86` (Skyrim LE); the other side is 0.
    class ClassSerializer {
    public:
        /// `a_view` is used to read `hkClass::m_defaults`.
        ClassSerializer(const ClassDb& a_db, const MemoryView& a_view, JsonFormat a_format = JsonFormat::SerdeHkx);

        [[nodiscard]] std::string ToJson(const ClassInfo& a_class);

        /// Root class implements an interface (= has a vtable).
        [[nodiscard]] bool HasVTable(const ClassInfo& a_class) const;

        /// Class (incl. parents) contains `CString` / `StringPtr`, directly or via embedded structs.
        [[nodiscard]] bool ClassHasString(const ClassInfo& a_class);
        [[nodiscard]] bool MemberHasString(const MemberInfo& a_member);

        /// d-merge `has_ref` of a member (havok_class_info_gen `nemesis` rule): CString / StringPtr / Variant /
        /// integers / Pointer, embedded structs (directly or as array elements) when `ClassHasRef`, never enums.
        [[nodiscard]] bool MemberHasRef(const MemberInfo& a_member);
        /// Class (incl. parents) has a member whose type or subtype is a d-merge reference kind,
        /// directly or via embedded structs.
        [[nodiscard]] bool ClassHasRef(const ClassInfo& a_class);

        /// Declared default of a member (`hkClass::m_defaults`), 0 if none.
        /// Integers/enums as value, `hkReal` / `hkHalf` as raw bits.
        [[nodiscard]] std::int64_t DefaultValue(const ClassInfo& a_class, std::size_t a_index) const;

    private:
        std::reference_wrapper<const ClassDb> _db;
        std::reference_wrapper<const MemoryView> _view;
        JsonFormat _format;
        std::unordered_map<std::uint64_t, bool> _hasString;
        std::unordered_map<std::uint64_t, bool> _hasRef;
        // enum address -> (vtype, vsubtype) of a member that uses it
        std::unordered_map<std::uint64_t, std::pair<std::uint8_t, std::uint8_t>> _enumUsage;
    };
} // namespace hkdump
