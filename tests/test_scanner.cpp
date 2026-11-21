#include <algorithm>
#include <array>
#include <cstring>
#include <span>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "Fixtures.h"
#include "Scanner.h"

using namespace fixtures;

// Provided by the MSVC linker: DOS header of the current module.
extern "C" const std::byte __ImageBase; // NOLINT(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp)

namespace {
    /// Pointers must stay inside the test executable image (like the game image in the plugin).
    hkdump::MemoryRange TestImage() {
        return hkdump::MemoryRange::FromPeImage(reinterpret_cast<std::uintptr_t>(&__ImageBase));
    }

    // Writable static storage (inside the image) used as a fake `.data` section.
    alignas(8) std::array<std::byte, 0x400> g_region;

    void FillGarbage() { std::ranges::fill(g_region, std::byte{0xAB}); }

    template<class T>
    void Put(std::size_t a_offset, const T& a_value) {
        std::memcpy(g_region.data() + a_offset, &a_value, sizeof(T));
    }

    bool Contains(const std::vector<hkdump::ClassRef>& a_list, std::string_view a_name) {
        return std::ranges::any_of(a_list, [&](const hkClass& c) { return a_name == c.GetName(); });
    }
} // namespace

TEST_CASE("IsValidClass accepts real tables", "[scanner]") {
    const auto range = TestImage();
    CHECK(hkdump::IsValidClass(&hkAabb, range));
    CHECK(hkdump::IsValidClass(&hkBaseObject, range));
    CHECK(hkdump::IsValidClass(&hkArrayTypeAttribute, range));
    CHECK(hkdump::IsValidClass(&hkbStateMachineActiveTransitionInfo, range));
    CHECK(hkdump::IsValidEnum(hkArrayTypeAttributeEnums.data(), range));
}

TEST_CASE("IsValidClass rejects broken tables", "[scanner]") {
    const auto range = TestImage();

    SECTION("null / out of range") {
        CHECK_FALSE(hkdump::IsValidClass(nullptr, range));
        CHECK_FALSE(hkdump::IsValidClass(&hkAabb, hkdump::MemoryRange{1, 2}));
    }
    SECTION("name is not an identifier") {
        auto c = hkAabb;
        c.name = "1abc";
        CHECK_FALSE(hkdump::IsValidClass(&c, range));
        c.name = "has space";
        CHECK_FALSE(hkdump::IsValidClass(&c, range));
    }
    SECTION("insane counts") {
        auto c = hkAabb;
        c.numDeclaredMembers = -1;
        CHECK_FALSE(hkdump::IsValidClass(&c, range));
        c = hkAabb;
        c.objectSize = 1 << 24;
        CHECK_FALSE(hkdump::IsValidClass(&c, range));
    }
    SECTION("member out of class / bad type") {
        auto members = hkAabbMembers;
        auto c = hkAabb;
        c.declaredMembers = members.data();

        members[1].offset = 64; // > objectSize
        CHECK_FALSE(hkdump::IsValidClass(&c, range));

        members = hkAabbMembers;
        members[0].type = RE::hkClassMember::TYPE_MAX;
        CHECK_FALSE(hkdump::IsValidClass(&c, range));
    }
}

TEST_CASE("FindClasses scans regions and follows parents / member classes", "[scanner]") {
    const auto range = TestImage();
    FillGarbage();
    // misaligned to the region start on purpose: scanner walks 8-byte aligned addresses
    Put(0x48, hkbStateMachineActiveTransitionInfo);
    Put(0x100, hkArrayTypeAttribute);

    const std::array regions{std::span<const std::byte>{g_region}};
    const auto classes = hkdump::FindClasses(regions, range);

    CHECK(classes.size() == 6);
    CHECK(&classes[0].get() == reinterpret_cast<const hkClass*>(g_region.data() + 0x48));
    CHECK(&classes[1].get() == reinterpret_cast<const hkClass*>(g_region.data() + 0x100));
    // closure: member classes and the parent chain of the pointer target
    CHECK(Contains(classes, "hkbStateMachineTransitionInfoReference"));
    CHECK(Contains(classes, "hkbNodeInternalStateInfo"));
    CHECK(Contains(classes, "hkReferencedObject"));
    CHECK(Contains(classes, "hkBaseObject"));
}

TEST_CASE("FindClasses ignores garbage and empty candidates", "[scanner]") {
    const auto range = TestImage();
    FillGarbage();
    // name only, no members / enums / parent -> skipped by the pre-filter
    Put(0x10, hkClass{.name = "Lonely", .objectSize = 4});

    const std::array regions{std::span<const std::byte>{g_region}};
    CHECK(hkdump::FindClasses(regions, range).empty());
}
