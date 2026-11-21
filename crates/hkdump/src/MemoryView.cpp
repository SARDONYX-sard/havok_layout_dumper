#include "MemoryView.h"

#include <algorithm>
#include <cstring>

#include "PeImage.h"

namespace hkdump {
    std::optional<std::uint64_t> MemoryView::GetPointer(std::uint64_t a_address) const {
        if (_pointerSize == 4) {
            const auto value = Get<std::uint32_t>(a_address);
            return value ? std::optional<std::uint64_t>{*value} : std::nullopt;
        }
        return Get<std::uint64_t>(a_address);
    }

    std::optional<std::string>
        MemoryView::GetIdent(std::uint64_t a_address, std::size_t a_maxLen, bool a_allowDigitHead) const {
        const auto is_alpha = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; };
        const auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
        const auto is_head = [&](char c) { return is_alpha(c) || (a_allowDigitHead && is_digit(c)); };
        const auto is_tail = [&](char c) { return is_alpha(c) || is_digit(c) || c == ':'; };

        std::string result;
        for (std::size_t i = 0; i < a_maxLen; ++i) {
            const auto c = Get<char>(a_address + i);
            if (!c) {
                return std::nullopt;
            }
            if (*c == '\0') {
                return result.empty() ? std::nullopt : std::optional{std::move(result)};
            }
            if (i == 0 ? !is_head(*c) : !is_tail(*c)) {
                return std::nullopt;
            }
            result.push_back(*c);
        }
        return std::nullopt;
    }

    SnapshotView::SnapshotView(ImageSnapshot a_snapshot)
        : MemoryView(a_snapshot.pointerSize)
        , _snapshot(std::move(a_snapshot)) {}

    bool SnapshotView::Read(std::uint64_t a_address, std::span<std::byte> a_out) const {
        for (const auto& region : _snapshot.regions) {
            const auto begin = _snapshot.imageBase + region.rva;
            const auto size = static_cast<std::uint64_t>(region.bytes.size());
            if (a_address >= begin && a_address - begin <= size && a_out.size() <= size - (a_address - begin)) {
                const auto offset = static_cast<std::size_t>(a_address - begin);
                std::ranges::copy(std::span{region.bytes}.subspan(offset, a_out.size()), a_out.begin());
                return true;
            }
        }
        return false;
    }

    std::vector<AddressRange> SnapshotView::ScanRegions() const {
        std::vector<AddressRange> result;
        for (const auto& region : _snapshot.regions) {
            const auto begin = _snapshot.imageBase + region.rva;
            result.push_back({.begin = begin, .end = begin + region.bytes.size()});
        }
        return result;
    }

    LiveView::LiveView(AddressRange a_readable, std::vector<AddressRange> a_scan, std::uint64_t a_imageBase) noexcept
        : MemoryView(sizeof(void*))
        , _readable(a_readable)
        , _scan(std::move(a_scan))
        , _imageBase(a_imageBase) {}

    std::optional<LiveView> LiveView::ForModule(std::uintptr_t a_base) {
        const auto image = ReadPeImage(a_base);
        if (!image) {
            return std::nullopt;
        }
        std::vector<AddressRange> scan;
        for (const auto name : kDefaultSnapshotSections) {
            if (const auto* const section = image->FindSection(name)) {
                scan.push_back({.begin = a_base + section->rva, .end = a_base + section->rva + section->virtualSize});
            }
        }
        return LiveView{{.begin = a_base, .end = a_base + image->sizeOfImage}, std::move(scan), a_base};
    }

    bool LiveView::Read(std::uint64_t a_address, std::span<std::byte> a_out) const {
        if (a_address < _readable.begin || a_address > _readable.end || a_out.size() > _readable.end - a_address) {
            return false;
        }
        std::memcpy(a_out.data(), reinterpret_cast<const void*>(static_cast<std::uintptr_t>(a_address)), a_out.size());
        return true;
    }
} // namespace hkdump
