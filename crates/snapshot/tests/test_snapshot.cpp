#include <array>
#include <filesystem>
#include <fstream>

#include <catch2/catch_test_macros.hpp>

#include "Snapshot.h"

TEST_CASE("Image snapshot round trip", "[snapshot]") {
    const auto path = std::filesystem::temp_directory_path() / "hkdump_test_snapshot" / "image.hksnap";
    std::filesystem::remove_all(path.parent_path());

    const std::array a{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array b{std::byte{0xAB}};
    const std::array regions{
        hkdump::SnapshotSource{.rva = 0x1000, .bytes = a},
        hkdump::SnapshotSource{.rva = 0x8000, .bytes = b},
    };

    const hkdump::ImageSnapshot header{.imageBase = 0x140000000, .imageSize = 0x10000, .pointerSize = 4};
    REQUIRE_FALSE(hkdump::WriteImageSnapshot(path, header, regions).has_value());

    const auto result = hkdump::ReadImageSnapshot(path);
    REQUIRE(result.has_value());
    const auto& snapshot = result.value();
    CHECK(snapshot.imageBase == 0x140000000);
    CHECK(snapshot.imageSize == 0x10000);
    CHECK(snapshot.pointerSize == 4);
    REQUIRE(snapshot.regions.size() == 2);
    CHECK(snapshot.regions.at(0).rva == 0x1000);
    CHECK(snapshot.regions.at(0).bytes == std::vector<std::byte>(a.begin(), a.end()));
    CHECK(snapshot.regions.at(1).rva == 0x8000);
    CHECK(snapshot.regions.at(1).bytes == std::vector<std::byte>(b.begin(), b.end()));

    std::filesystem::remove_all(path.parent_path());
}

TEST_CASE("ReadImageSnapshot rejects other files", "[snapshot]") {
    const auto path = std::filesystem::temp_directory_path() / "hkdump_test_snapshot_bad.bin";
    std::ofstream{path, std::ios::binary} << "NOTASNAPSHOT";
    CHECK_FALSE(hkdump::ReadImageSnapshot(path).has_value());
    CHECK_FALSE(hkdump::ReadImageSnapshot(path.parent_path() / "missing.hksnap").has_value());
    std::filesystem::remove(path);
}
