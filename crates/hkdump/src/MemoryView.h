#pragma once

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "Snapshot.h"

namespace hkdump {
    /// Address range `[begin, end)` in the target's address space.
    struct AddressRange {
        std::uint64_t begin{0};
        std::uint64_t end{0};
    };

    /// Read-only access to the target process memory (`pointerSize` 4 or 8), independent of the
    /// host platform: the decoder never dereferences target addresses as native pointers.
    class MemoryView {
    public:
        explicit MemoryView(std::uint32_t a_pointerSize) noexcept : _pointerSize(a_pointerSize) {}
        MemoryView(const MemoryView&) = default;
        MemoryView(MemoryView&&) = default;
        MemoryView& operator=(const MemoryView&) = default;
        MemoryView& operator=(MemoryView&&) = default;
        virtual ~MemoryView() = default;

        /// Copies `a_out.size()` bytes at `a_address`; false when not fully readable.
        [[nodiscard]] virtual bool Read(std::uint64_t a_address, std::span<std::byte> a_out) const = 0;

        /// Regions worth scanning for reflection tables.
        [[nodiscard]] virtual std::vector<AddressRange> ScanRegions() const = 0;

        /// Start of the image (used for RVAs in messages / file names).
        [[nodiscard]] virtual std::uint64_t ImageBase() const noexcept = 0;

        [[nodiscard]] std::uint32_t PointerSize() const noexcept { return _pointerSize; }

        template<class T>
            requires std::is_trivially_copyable_v<T>
        [[nodiscard]] std::optional<T> Get(std::uint64_t a_address) const {
            std::array<std::byte, sizeof(T)> buffer{};
            if (!Read(a_address, buffer)) {
                return std::nullopt;
            }
            return std::bit_cast<T>(buffer);
        }

        /// Target pointer (4 or 8 bytes), zero-extended.
        [[nodiscard]] std::optional<std::uint64_t> GetPointer(std::uint64_t a_address) const;

        /// NUL terminated C identifier (`[A-Za-z_][A-Za-z0-9_:]*`), nullopt otherwise.
        ///
        /// `a_allowDigitHead`: Also accept a leading digit (`[A-Za-z0-9_][A-Za-z0-9_:]*`).
        /// Havok member names may start with a digit (e.g. `hkpLimitedHingeConstraintDataAtoms::2dAng`),
        /// so use it only for member names. Class / enum names stay strict to keep the scan false positives low.
        [[nodiscard]] std::optional<std::string>
            GetIdent(std::uint64_t a_address, std::size_t a_maxLen = 256, bool a_allowDigitHead = false) const;

    private:
        std::uint32_t _pointerSize;
    };

    /// View over an image snapshot file (the offline case).
    class SnapshotView final : public MemoryView {
    public:
        explicit SnapshotView(ImageSnapshot a_snapshot);

        [[nodiscard]] bool Read(std::uint64_t a_address, std::span<std::byte> a_out) const override;
        [[nodiscard]] std::vector<AddressRange> ScanRegions() const override;
        [[nodiscard]] std::uint64_t ImageBase() const noexcept override { return _snapshot.imageBase; }

    private:
        ImageSnapshot _snapshot;
    };

    /// View over memory of the current process (tests / in-process use). Pointer size is native.
    class LiveView final : public MemoryView {
    public:
        /// `a_readable`: what may be read, `a_scan`: what `ScanRegions` returns.
        LiveView(AddressRange a_readable, std::vector<AddressRange> a_scan, std::uint64_t a_imageBase) noexcept;

        /// Whole mapped image of a module (`a_base` = HMODULE); scans `.rdata` / `.data`.
        [[nodiscard]] static std::optional<LiveView> ForModule(std::uintptr_t a_base);

        [[nodiscard]] bool Read(std::uint64_t a_address, std::span<std::byte> a_out) const override;
        [[nodiscard]] std::vector<AddressRange> ScanRegions() const override { return _scan; }
        [[nodiscard]] std::uint64_t ImageBase() const noexcept override { return _imageBase; }

    private:
        AddressRange _readable;
        std::vector<AddressRange> _scan;
        std::uint64_t _imageBase;
    };
} // namespace hkdump
