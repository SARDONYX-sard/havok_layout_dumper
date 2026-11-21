#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "ClassRef.h"
#include "MemoryRange.h"

namespace hkdump {
    /// Heuristic validation of a single `hkClassEnum` (all pointers inside `a_range`).
    /// Takes a raw pointer on purpose: the address is untrusted (may be null / garbage).
    [[nodiscard]] bool IsValidEnum(const RE::hkClassEnum* a_enum, const MemoryRange& a_range) noexcept;

    /// Heuristic validation of a single `hkClass`.
    /// Shallow: parent / member classes are only checked to have a valid name.
    [[nodiscard]] bool IsValidClass(const RE::hkClass* a_class, const MemoryRange& a_range) noexcept;

    /// Scans `a_regions` (8-byte aligned) for static `hkClass` tables and returns them plus the
    /// closure of their parents and member classes, in discovery order (no duplicates).
    [[nodiscard]] std::vector<ClassRef>
    FindClasses(std::span<const std::span<const std::byte>> a_regions, const MemoryRange& a_range);
} // namespace hkdump
