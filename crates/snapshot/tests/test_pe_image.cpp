// PE image reading only makes sense for Windows executables.
#ifdef _WIN32

#include <array>
#include <filesystem>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "PeImage.h"
#include "Snapshot.h"

// Provided by the MSVC linker: DOS header of the current module.
extern "C" const std::byte __ImageBase; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

namespace {
    std::uintptr_t SelfBase() { return reinterpret_cast<std::uintptr_t>(&__ImageBase); }
} // namespace

TEST_CASE("ReadPeImage parses the running test executable", "[pe]") {
    const auto image = hkdump::ReadPeImage(SelfBase());
    REQUIRE(image.has_value());
    CHECK(image.value().is64);
    CHECK(image.value().sizeOfImage > 0);
    CHECK(image.value().FindSection(".text") != nullptr);
    CHECK(image.value().FindSection(".rdata") != nullptr);
    CHECK(image.value().FindSection(".data") != nullptr);
    CHECK(image.value().FindSection(".nope") == nullptr);
}

TEST_CASE("ReadPeImage rejects non PE memory", "[pe]") {
    static const std::uint64_t notPe = 0;
    CHECK_FALSE(hkdump::ReadPeImage(reinterpret_cast<std::uintptr_t>(&notPe)).has_value());
    CHECK_FALSE(hkdump::ReadPeImage(0).has_value());
}

TEST_CASE("WriteModuleSnapshot captures .rdata / .data of a module", "[pe][snapshot]") {
    const auto path = std::filesystem::temp_directory_path() / "hkdump_test_module" / "self.hksnap";
    std::filesystem::remove_all(path.parent_path());

    REQUIRE_FALSE(hkdump::WriteModuleSnapshot(SelfBase(), path).has_value());

    const auto image = hkdump::ReadPeImage(SelfBase()).value();
    const auto snapshot = hkdump::ReadImageSnapshot(path).value();
    CHECK(snapshot.imageBase == SelfBase());
    CHECK(snapshot.imageSize == image.sizeOfImage);
    CHECK(snapshot.pointerSize == sizeof(void*));
    REQUIRE(snapshot.regions.size() == 2);
    CHECK(snapshot.regions.at(0).rva == image.FindSection(".rdata")->rva);
    CHECK(snapshot.regions.at(1).bytes.size() == image.FindSection(".data")->virtualSize);

    std::filesystem::remove_all(path.parent_path());
}

TEST_CASE("WriteModuleSnapshot reports missing sections", "[pe][snapshot]") {
    const std::array<std::string_view, 1> sections{".nope"};
    const auto error = hkdump::WriteModuleSnapshot(SelfBase(), "unused.hksnap", sections);
    REQUIRE(error.has_value());
    CHECK(error.value() == "section .nope not found");
}

#endif // _WIN32
