// hkdump: image snapshot (`*.hksnap`, written in-game by the xSE plugins) -> class JSON.
//
//   hkdump [-v] [--format serde-hkx|d-merge] <snapshot.hksnap> <out dir>
//
// `--format`: serde-hkx (default) or d-merge-serde-hkx flavored class JSON (see `JsonFormat`).
//
// Platform independent: the snapshot is decoded with the target's pointer size (x86 / x64), so a
// single build handles Skyrim LE, Skyrim SE and Fallout 4 snapshots on any OS.

#include <cstdio>
#include <filesystem>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Dump.h"
#include "MemoryView.h"
#include "PathUtf8.h"
#include "Scanner.h"
#include "Snapshot.h"

namespace {
    struct Options {
        std::filesystem::path snapshot;
        std::filesystem::path outDir;
        hkdump::JsonFormat format{hkdump::JsonFormat::SerdeHkx};
        bool verbose{false};
    };

    int Run(const Options& a_options) {
        auto snapshot = hkdump::ReadImageSnapshot(a_options.snapshot);
        if (!snapshot) {
            std::println(stderr, "failed to read snapshot: {}", hkdump::ToUtf8(a_options.snapshot));
            return 1;
        }
        std::println(
            "{}: image {:#x} (x{}), {} regions",
            hkdump::ToUtf8(a_options.snapshot),
            snapshot->imageBase,
            snapshot->pointerSize == 4 ? "86" : "64",
            snapshot->regions.size()
        );

        const hkdump::SnapshotView view{std::move(*snapshot)};
        const auto db = hkdump::ScanClasses(view);
        std::println("found {} hkClass tables", db.Classes().size());
        if (a_options.verbose) {
            for (const auto& cls : db.Classes()) {
                std::println("  {:#x} {}", cls.address - view.ImageBase(), cls.name);
            }
        }

        const auto entries = hkdump::BuildDump(db, view, a_options.format);
        for (const auto& entry : entries) {
            if (entry.renamed) {
                std::println("duplicate class name {} -> {}", entry.klass.get().name, entry.fileName);
            }
        }

        const auto result = hkdump::WriteDump(a_options.outDir, entries);
        for (const auto& error : result.errors) {
            std::println(stderr, "{}", error);
        }
        std::println("wrote {} files to {}", result.written, hkdump::ToUtf8(a_options.outDir));
        return result.errors.empty() ? 0 : 1;
    }

    constexpr std::string_view kUsage = "usage: hkdump [-v] [--format serde-hkx|d-merge] <snapshot.hksnap> <out dir>";

    template<class Char>
    int Main(std::span<Char*> a_args) {
        Options options;
        std::vector<std::filesystem::path> positional;
        std::optional<std::string> format;
        const auto args = a_args.subspan(1);
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::filesystem::path value{args[i]};
            const auto text = hkdump::ToUtf8(value);
            if (text == "-v" || text == "--verbose") {
                options.verbose = true;
            } else if (text == "--format" && i + 1 < args.size()) {
                format = hkdump::ToUtf8(std::filesystem::path{args[++i]});
            } else if (text.starts_with("--format=")) {
                format = text.substr(std::string_view{"--format="}.size());
            } else {
                positional.push_back(value);
            }
        }
        if (format) {
            const auto parsed = hkdump::ParseJsonFormat(*format);
            if (!parsed) {
                std::println(stderr, "unknown format: {}\n{}", *format, kUsage);
                return 2;
            }
            options.format = *parsed;
        }
        if (positional.size() != 2) {
            std::println(stderr, "{}", kUsage);
            return 2;
        }
        options.snapshot = positional[0];
        options.outDir = positional[1];
        return Run(options);
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
