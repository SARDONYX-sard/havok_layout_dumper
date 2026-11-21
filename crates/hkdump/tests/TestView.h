#pragma once

// Helpers to decode the in-process `RE::` fixtures through the same code path as snapshots.
//
// The fixtures live in the test executable's static data, so a `LiveView` over the executable
// image lets the decoder read them like a game snapshot. Only how to find that image is OS specific.

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

#include "MemoryView.h"
#include "Model.h"
#include "Scanner.h"

#ifdef _WIN32
// Provided by the MSVC linker: DOS header of the current module.
extern "C" const std::byte __ImageBase; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#elifdef __APPLE__
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#else
// Provided by the GNU / LLVM linkers for ELF executables.
extern "C" const char __executable_start[]; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
extern "C" const char _end[];               // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)
#endif

namespace testing {
    /// Readable range of the test executable image (code, read-only data, data, bss).
    [[nodiscard]] inline hkdump::AddressRange SelfImage() {
#ifdef _WIN32
        const auto base = reinterpret_cast<std::uintptr_t>(&__ImageBase);
        const auto image = hkdump::LiveView::ForModule(base).value();
        auto end = base;
        for (const auto& r : image.ScanRegions()) {
            end = std::max<std::uint64_t>(end, r.end);
        }
        return {.begin = base, .end = end};
#elifdef __APPLE__
        const auto* const header = &_mh_execute_header;
        auto end = reinterpret_cast<std::uintptr_t>(header);
        for (const char* segment : {"__TEXT", "__DATA_CONST", "__DATA"}) {
            unsigned long size = 0;
            if (const auto* data = getsegmentdata(header, segment, &size)) {
                end = std::max<std::uint64_t>(end, reinterpret_cast<std::uintptr_t>(data) + size);
            }
        }
        return {.begin = reinterpret_cast<std::uintptr_t>(header), .end = end};
#else
        return {
            .begin = reinterpret_cast<std::uintptr_t>(__executable_start), .end = reinterpret_cast<std::uintptr_t>(_end)
        };
#endif
    }

    [[nodiscard]] inline std::uint64_t SelfBase() { return SelfImage().begin; }

    template<class T>
    [[nodiscard]] std::uint64_t Addr(const T& a_object) {
        return reinterpret_cast<std::uintptr_t>(&a_object);
    }

    /// The test executable image (fixtures live in its static data). Scans `a_scan` only.
    [[nodiscard]] inline hkdump::LiveView SelfView(std::vector<hkdump::AddressRange> a_scan = {}) {
        return hkdump::LiveView{SelfImage(), std::move(a_scan), SelfBase()};
    }

    /// Decodes fixture classes (+ their parents / member classes).
    [[nodiscard]] inline hkdump::ClassDb Load(std::initializer_list<std::uint64_t> a_roots) {
        const std::vector<std::uint64_t> roots{a_roots};
        return hkdump::LoadClasses(SelfView(), roots);
    }

    [[nodiscard]] inline const hkdump::ClassInfo& Get(const hkdump::ClassDb& a_db, std::uint64_t a_address) {
        return a_db.FindClass(a_address).value().get();
    }
} // namespace testing
