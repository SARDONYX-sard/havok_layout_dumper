#pragma once

// Havok 2014.1 (x64) static reflection: enum description.
// Not provided by CommonLibF4, so it is supplemented here in the same `RE` namespace.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace RE {
    class hkClassEnum {
    public:
        class Item {
        public:
            std::int32_t value{};  // 00
            std::uint32_t pad04{}; // 04
            const char* name{};    // 08

            [[nodiscard]] std::string_view GetName() const noexcept { return name; }
        };
        static_assert(sizeof(Item) == 0x10);

        // Mirrors the Havok declaration: unscoped (used as bit flags / `using enum`) with the field's width.
        // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class,performance-enum-size)
        enum FlagValues : std::uint32_t {
            FLAGS_NONE = 0,
        };

        // members
        const char* name{};       // 00
        const Item* items{};      // 08
        std::int32_t numItems{};  // 10
        std::uint32_t pad14{};    // 14
        const void* attributes{}; // 18 hkCustomAttributes*
        std::uint32_t flags{};    // 20 FlagValues
        std::uint32_t pad24{};    // 24

        [[nodiscard]] std::string_view GetName() const noexcept { return name; }

        [[nodiscard]] std::span<const Item> GetItems() const noexcept {
            return {items, numItems > 0 ? static_cast<std::size_t>(numItems) : 0};
        }
    };
    static_assert(sizeof(hkClassEnum) == 0x28);
}
