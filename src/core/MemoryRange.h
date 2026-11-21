#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>

namespace hkdump {
    /// Address range `[lo, hi)` that may be dereferenced safely.
    ///
    /// All pointer validation in the scanner / serializer goes through this, so tests can
    /// run against in-process fixtures while the plugin uses the game image bounds.
    class MemoryRange {
    public:
        constexpr MemoryRange() noexcept = default;
        constexpr MemoryRange(std::uintptr_t a_lo, std::uintptr_t a_hi) noexcept : _lo(a_lo), _hi(a_hi) {}

        /// Every non-null address is treated as readable (unit tests with trusted fixtures).
        [[nodiscard]] static constexpr MemoryRange Everything() noexcept {
            return {1, std::numeric_limits<std::uintptr_t>::max()};
        }

        /// Range of a mapped PE32+ image: `[base, base + OptionalHeader.SizeOfImage)`.
        [[nodiscard]] static MemoryRange FromPeImage(std::uintptr_t a_base) noexcept;

        [[nodiscard]] constexpr std::uintptr_t lo() const noexcept { return _lo; }
        [[nodiscard]] constexpr std::uintptr_t hi() const noexcept { return _hi; }

        [[nodiscard]] bool contains(const void* a_ptr, std::size_t a_size = 1) const noexcept {
            const auto p = reinterpret_cast<std::uintptr_t>(a_ptr);
            return p >= _lo && p < _hi && a_size <= _hi - p;
        }

        template<class T>
        [[nodiscard]] bool readable(const T* a_ptr, std::size_t a_count = 1) const noexcept {
            return a_ptr != nullptr && contains(a_ptr, sizeof(T) * a_count);
        }

        /// Offset of an object from the range start (RVA for a PE image range).
        template<class T>
        [[nodiscard]] std::uintptr_t offset_of(const T& a_object) const noexcept {
            return reinterpret_cast<std::uintptr_t>(&a_object) - _lo;
        }

        /// Reads a `T` at `a_base + a_offset` when the whole value lies inside the range.
        /// All raw address arithmetic on untrusted memory is confined here.
        template<class T>
        [[nodiscard]] std::optional<T> read(const void* a_base, std::size_t a_offset = 0) const noexcept {
            if (a_base == nullptr) {
                return std::nullopt;
            }
            const auto addr = reinterpret_cast<std::uintptr_t>(a_base) + a_offset;
            const auto* const src = reinterpret_cast<const void*>(addr);
            if (!contains(src, sizeof(T))) {
                return std::nullopt;
            }
            T value;
            std::memcpy(&value, src, sizeof(T));
            return value;
        }

        /// C identifier-ish NUL terminated string inside the range (Havok names).
        [[nodiscard]] bool is_ident(const char* a_str, std::size_t a_maxLen = 256) const noexcept;

    private:
        std::uintptr_t _lo{0};
        std::uintptr_t _hi{0};
    };
} // namespace hkdump
