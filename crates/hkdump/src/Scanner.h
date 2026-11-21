#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "MemoryView.h"
#include "Model.h"

namespace hkdump {
    /// Decodes + validates one `hkClassEnum` at `a_address` (nullopt when it does not look like one).
    [[nodiscard]] std::optional<EnumInfo> DecodeEnum(const MemoryView& a_view, std::uint64_t a_address);

    /// A decoded class plus the enums its members reference.
    struct DecodedClass {
        ClassInfo klass;
        std::vector<EnumInfo> enums;
    };

    /// Decodes + validates one `hkClass` at `a_address` (heuristic; shallow: parent / member classes
    /// are only checked to have a valid name).
    [[nodiscard]] std::optional<DecodedClass> DecodeClass(const MemoryView& a_view, std::uint64_t a_address);

    /// Decodes `a_roots` plus the closure of their parents / member classes. Classes whose graph is
    /// not fully decodable are dropped, so consumers never see dangling references.
    [[nodiscard]] ClassDb LoadClasses(const MemoryView& a_view, std::span<const std::uint64_t> a_roots);

    /// Scans `a_view.ScanRegions()` (8 / 4-byte aligned) for `hkClass` tables, then `LoadClasses`.
    [[nodiscard]] ClassDb ScanClasses(const MemoryView& a_view);
} // namespace hkdump
