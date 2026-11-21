#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "Dump.h"
#include "Fixtures.h"

using namespace fixtures;
using Catch::Matchers::StartsWith;

namespace {
    std::string ReadFile(const std::filesystem::path& a_path) {
        std::ifstream file{a_path, std::ios::binary};
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    /// Unique temp directory removed at scope exit.
    struct TempDir {
        std::filesystem::path path;

        explicit TempDir(std::string_view a_name)
            : path(std::filesystem::temp_directory_path() / std::format("hkdump_test_{}", a_name)) {
            std::filesystem::remove_all(path);
        }
        TempDir(const TempDir&) = delete;
        TempDir(TempDir&&) = delete;
        TempDir& operator=(const TempDir&) = delete;
        TempDir& operator=(TempDir&&) = delete;

        ~TempDir() {
            try {
                std::error_code ec;
                std::filesystem::remove_all(path, ec);
            } catch (...) { // NOLINT(bugprone-empty-catch): best effort cleanup
            }
        }
    };
} // namespace

TEST_CASE("SanitizeFileName", "[dump]") {
    CHECK(hkdump::SanitizeFileName("hkAabb") == "hkAabb");
    CHECK(hkdump::SanitizeFileName("a:b<c>d*e?f\"g/h\\i|j") == "a_b_c_d_e_f_g_h_i_j");
}

TEST_CASE("BuildDump assigns unique file names", "[dump]") {
    const auto copy = hkAabb; // same name, different address
    const std::vector<hkdump::ClassRef> classes{hkAabb, hkBaseObject, copy};
    const hkdump::MemoryRange range{0, std::numeric_limits<std::uintptr_t>::max()};

    const auto entries = hkdump::BuildDump(classes, range);
    REQUIRE(entries.size() == 3);

    CHECK(entries[0].fileName == "hkAabb.json");
    CHECK_FALSE(entries[0].renamed);
    CHECK(entries[1].fileName == "hkBaseObject.json");

    CHECK(entries[2].renamed);
    CHECK(&entries[2].klass.get() == &copy);
    CHECK(entries[2].fileName == std::format("hkAabb_{:X}.json", reinterpret_cast<std::uintptr_t>(&copy)));

    CHECK_THAT(entries[0].json, StartsWith("{\n  \"name\": \"hkAabb\","));
}

TEST_CASE("WriteDump writes every entry", "[dump]") {
    TempDir dir{"write"};
    const std::vector<hkdump::DumpEntry> entries{
        {.klass = hkAabb, .fileName = "hkAabb.json", .json = "{}\n"},
        {.klass = hkBaseObject, .fileName = "hkBaseObject.json", .json = "[]\n"},
    };

    const auto result = hkdump::WriteDump(dir.path / "nested", entries); // created recursively
    CHECK(result.errors.empty());
    CHECK(result.written == 2);
    CHECK(ReadFile(dir.path / "nested" / "hkAabb.json") == "{}\n");
    CHECK(ReadFile(dir.path / "nested" / "hkBaseObject.json") == "[]\n");
}

TEST_CASE("WriteDump reports errors", "[dump]") {
    TempDir dir{"error"};
    std::filesystem::create_directories(dir.path);
    std::ofstream{dir.path / "file"} << "x"; // a file where a directory is expected

    const std::vector<hkdump::DumpEntry> entries{{.klass = hkAabb, .fileName = "hkAabb.json", .json = "{}"}};
    const auto result = hkdump::WriteDump(dir.path / "file", entries);
    CHECK(result.written == 0);
    CHECK(result.errors.size() == 1);
}
