// hkdump: image snapshot (`*.hksnap`, written in-game by the xSE plugins) -> serde-hkx class JSON.
//
//   hkdump <snapshot.hksnap> <out dir>
//
// Platform independent: the snapshot is decoded with the target's pointer size (x86 / x64), so a
// single build handles Skyrim LE, Skyrim SE and Fallout 4 snapshots on any OS.

#include <cstdio>
#include <filesystem>
#include <print>
#include <span>

#include "Dump.h"
#include "MemoryView.h"
#include "PathUtf8.h"
#include "Scanner.h"
#include "Snapshot.h"

namespace {
    int Run(const std::filesystem::path& a_snapshot, const std::filesystem::path& a_outDir, bool a_verbose) {
        auto snapshot = hkdump::ReadImageSnapshot(a_snapshot);
        if (!snapshot) {
            std::println(stderr, "failed to read snapshot: {}", hkdump::ToUtf8(a_snapshot));
            return 1;
        }
        std::println(
            "{}: image {:#x} (x{}), {} regions",
            hkdump::ToUtf8(a_snapshot),
            snapshot->imageBase,
            snapshot->pointerSize == 4 ? "86" : "64",
            snapshot->regions.size()
        );

        const hkdump::SnapshotView view{std::move(*snapshot)};
        const auto db = hkdump::ScanClasses(view);
        std::println("found {} hkClass tables", db.Classes().size());
        if (a_verbose) {
            for (const auto& cls : db.Classes()) {
                std::println("  {:#x} {}", cls.address - view.ImageBase(), cls.name);
            }
        }

        const auto entries = hkdump::BuildDump(db, view);
        for (const auto& entry : entries) {
            if (entry.renamed) {
                std::println("duplicate class name {} -> {}", entry.klass.get().name, entry.fileName);
            }
        }

        const auto result = hkdump::WriteDump(a_outDir, entries);
        for (const auto& error : result.errors) {
            std::println(stderr, "{}", error);
        }
        std::println("wrote {} files to {}", result.written, hkdump::ToUtf8(a_outDir));
        return result.errors.empty() ? 0 : 1;
    }

    template<class Char>
    int Main(std::span<Char*> a_args) {
        std::vector<std::filesystem::path> positional;
        bool verbose = false;
        for (const auto* arg : a_args.subspan(1)) {
            const std::filesystem::path value{arg};
            if (value == "-v" || value == "--verbose") {
                verbose = true;
            } else {
                positional.push_back(value);
            }
        }
        if (positional.size() != 2) {
            std::println(stderr, "usage: hkdump [-v] <snapshot.hksnap> <out dir>");
            return 2;
        }
        return Run(positional[0], positional[1], verbose);
    }
} // namespace

#ifdef _WIN32
// UTF-16 arguments: keeps non-ASCII paths (e.g. "ドキュメント") intact.
int wmain(int a_argc, wchar_t** a_argv) { // NOLINT(misc-use-internal-linkage): entry point
    return Main(std::span{a_argv, static_cast<std::size_t>(a_argc)});
}
#else
int main(int a_argc, char** a_argv) { return Main(std::span{a_argv, static_cast<std::size_t>(a_argc)}); }
#endif
