#include "Scanner.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace hkdump {
    using RE::hkClass;
    using RE::hkClassEnum;
    using enum RE::hkClassMember::Type;

    bool IsValidEnum(const hkClassEnum* a_enum, const MemoryRange& a_range) noexcept {
        if (!a_range.readable(a_enum) || !a_range.is_ident(a_enum->name)) {
            return false;
        }
        if (a_enum->numItems < 0 || a_enum->numItems > 4096) {
            return false;
        }
        if (a_enum->numItems == 0) {
            return true;
        }
        if (!a_range.readable(a_enum->items, static_cast<std::size_t>(a_enum->numItems))) {
            return false;
        }
        return std::ranges::all_of(a_enum->GetItems(), [&](const auto& item) { return a_range.is_ident(item.name); });
    }

    bool IsValidClass(const hkClass* a_class, const MemoryRange& a_range) noexcept {
        if (!a_range.readable(a_class) || !a_range.is_ident(a_class->name)) {
            return false;
        }

        const auto& c = *a_class;
        if (c.objectSize < 0 || c.objectSize > (1 << 20) || c.numImplementedInterfaces < 0
            || c.numImplementedInterfaces > 64 || c.numDeclaredEnums < 0 || c.numDeclaredEnums > 256
            || c.numDeclaredMembers < 0 || c.numDeclaredMembers > 1024 || c.describedVersion < 0
            || c.describedVersion > 1000 || c.flags > 0xFFFF) {
            return false;
        }

        if (c.parent && (!a_range.readable(c.parent) || !a_range.is_ident(c.parent->name))) {
            return false;
        }
        if (c.defaults && !a_range.readable(static_cast<const std::int32_t*>(c.defaults))) {
            return false;
        }
        if (c.attributes && !a_range.contains(c.attributes)) {
            return false;
        }

        if (c.numDeclaredEnums > 0) {
            if (!a_range.readable(c.declaredEnums, static_cast<std::size_t>(c.numDeclaredEnums))) {
                return false;
            }
            if (!std::ranges::all_of(c.GetDeclaredEnums(), [&](const auto& e) { return IsValidEnum(&e, a_range); })) {
                return false;
            }
        }

        if (c.numDeclaredMembers > 0) {
            if (!a_range.readable(c.declaredMembers, static_cast<std::size_t>(c.numDeclaredMembers))) {
                return false;
            }
            for (const auto& m : c.GetDeclaredMembers()) {
                if (!a_range.is_ident(m.name) || m.type >= TYPE_MAX || m.subtype >= TYPE_MAX || m.cArraySize < 0
                    || std::cmp_greater(m.offset, c.objectSize)) {
                    return false;
                }
                if (m.klass && (!a_range.readable(m.klass) || !a_range.is_ident(m.klass->name))) {
                    return false;
                }
                if (m.enumClass && !IsValidEnum(m.enumClass, a_range)) {
                    return false;
                }
            }
        }

        return true;
    }

    std::vector<ClassRef>
    FindClasses(std::span<const std::span<const std::byte>> a_regions, const MemoryRange& a_range) {
        std::vector<ClassRef> result;
        std::unordered_set<const hkClass*> seen; // identity only
        std::vector<ClassRef> work;

        // `a_candidate` is an untrusted address: validated before it becomes a ClassRef.
        const auto push = [&](const hkClass* a_candidate) {
            if (a_candidate && !seen.contains(a_candidate) && IsValidClass(a_candidate, a_range)) {
                seen.insert(a_candidate);
                result.emplace_back(*a_candidate);
                work.emplace_back(*a_candidate);
            }
        };

        for (const auto region : a_regions) {
            auto begin = reinterpret_cast<std::uintptr_t>(region.data());
            const auto end = begin + region.size();
            begin = (begin + 7) & ~std::uintptr_t{7};

            for (auto addr = begin; addr + sizeof(hkClass) <= end; addr += 8) {
                const auto* const cls = reinterpret_cast<const hkClass*>(addr);
                // Require some content to avoid matching random (name, 0, 0...) tuples.
                // Empty classes (e.g. hkBaseObject) are picked up as parents below.
                if (cls->numDeclaredMembers <= 0 && cls->numDeclaredEnums <= 0 && !cls->parent) {
                    continue;
                }
                push(cls);
            }
        }

        while (!work.empty()) {
            const hkClass& cls = work.back();
            work.pop_back();
            push(cls.parent);
            for (const auto& m : cls.GetDeclaredMembers()) {
                push(m.klass);
            }
        }

        return result;
    }
} // namespace hkdump
