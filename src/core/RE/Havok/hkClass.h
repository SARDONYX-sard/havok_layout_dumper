#pragma once

// Havok 2014.1 (x64) static reflection: class description.
//
// CommonLibF4 only forward-declares `RE::hkClass` (see RE/Havok/hkReferencedObject.h),
// so the layout is supplemented here in the same `RE` namespace.
// Only the memory layout matters: we never call into Havok, we only read
// the statically-initialized reflection tables in the executable image.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>

#include "RE/Havok/hkClassEnum.h"
#include "RE/Havok/hkClassMember.h"

namespace RE {
    class hkClass {
    public:
        // Mirrors the Havok declaration: unscoped (used as bit flags / `using enum`) with the field's width.
        // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class,performance-enum-size)
        enum FlagValues : std::uint32_t {
            FLAGS_NONE = 0,
            FLAGS_NOT_SERIALIZABLE = 1,
        };

        // members
        const char* name{};                      // 00
        const hkClass* parent{};                 // 08
        std::int32_t objectSize{};               // 10
        std::int32_t numImplementedInterfaces{}; // 14
        const hkClassEnum* declaredEnums{};      // 18
        std::int32_t numDeclaredEnums{};         // 20
        std::uint32_t pad24{};                   // 24
        const hkClassMember* declaredMembers{};  // 28
        std::int32_t numDeclaredMembers{};       // 30
        std::uint32_t pad34{};                   // 34
        const void* defaults{};                  // 38 { int32 offsets[numDeclaredMembers]; ...values }
        const void* attributes{};                // 40 hkCustomAttributes*
        std::uint32_t flags{};                   // 48 FlagValues
        std::int32_t describedVersion{};         // 4C

        [[nodiscard]] std::string_view GetName() const noexcept { return name; }

        [[nodiscard]] std::span<const hkClassEnum> GetDeclaredEnums() const noexcept {
            return {declaredEnums, numDeclaredEnums > 0 ? static_cast<std::size_t>(numDeclaredEnums) : 0};
        }

        [[nodiscard]] std::span<const hkClassMember> GetDeclaredMembers() const noexcept {
            return {declaredMembers, numDeclaredMembers > 0 ? static_cast<std::size_t>(numDeclaredMembers) : 0};
        }

        /// `*this`, then each parent up to the root: `for (const hkClass& c : cls.GetChain())`.
        class Chain {
        public:
            class Iterator {
            public:
                using iterator_category = std::forward_iterator_tag;
                using value_type = hkClass;
                using difference_type = std::ptrdiff_t;
                using pointer = const hkClass*;
                using reference = const hkClass&;

                Iterator() noexcept = default;
                explicit Iterator(const hkClass* a_class) noexcept : _class(a_class) {}

                reference operator*() const noexcept { return *_class; }
                pointer operator->() const noexcept { return _class; }
                Iterator& operator++() noexcept {
                    _class = _class->parent;
                    return *this;
                }
                Iterator operator++(int) noexcept {
                    auto tmp = *this;
                    ++*this;
                    return tmp;
                }
                bool operator==(const Iterator&) const noexcept = default;

            private:
                const hkClass* _class{nullptr};
            };

            explicit Chain(const hkClass& a_class) noexcept : _first(&a_class) {}

            [[nodiscard]] Iterator begin() const noexcept { return Iterator{_first}; }
            [[nodiscard]] Iterator end() const noexcept { return Iterator{}; }

        private:
            const hkClass* _first;
        };

        [[nodiscard]] Chain GetChain() const noexcept { return Chain{*this}; }

        /// Root of the inheritance chain (itself when it has no parent).
        [[nodiscard]] const hkClass& GetRoot() const noexcept {
            const hkClass* c = this;
            while (c->parent != nullptr) {
                c = c->parent;
            }
            return *c;
        }
    };
    static_assert(sizeof(hkClass) == 0x50);
    static_assert(offsetof(hkClass, declaredMembers) == 0x28);
    static_assert(offsetof(hkClass, describedVersion) == 0x4C);
}
