#include <algorithm>
#include <array>
#include <cstring>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "Fixtures.h"
#include "Scanner.h"
#include "TestView.h"

using namespace fixtures;
using testing::Addr;

namespace {
    // Writable static storage (inside the test image) used as a fake `.data` section.
    alignas(8) std::array<std::byte, 0x400> g_region;

    void FillGarbage() { std::ranges::fill(g_region, std::byte{0xAB}); }

    template<class T>
    std::uint64_t Put(std::size_t a_offset, const T& a_value) {
        std::memcpy(g_region.data() + a_offset, &a_value, sizeof(T));
        return Addr(g_region) + a_offset;
    }

    /// Image readable, only `g_region` scanned.
    hkdump::LiveView RegionView() {
        return testing::SelfView({{.begin = Addr(g_region), .end = Addr(g_region) + g_region.size()}});
    }

    bool Contains(const hkdump::ClassDb& a_db, std::string_view a_name) {
        return std::ranges::any_of(a_db.Classes(), [&](const auto& c) { return c.name == a_name; });
    }
} // namespace

TEST_CASE("DecodeClass accepts real tables", "[scanner]") {
    const auto view = testing::SelfView();
    CHECK(hkdump::DecodeClass(view, Addr(hkAabb)).has_value());
    CHECK(hkdump::DecodeClass(view, Addr(hkBaseObject)).has_value());
    CHECK(hkdump::DecodeClass(view, Addr(hkbStateMachineActiveTransitionInfo)).has_value());

    const auto attr = hkdump::DecodeClass(view, Addr(hkArrayTypeAttribute));
    REQUIRE(attr.has_value());
    CHECK(attr.value().klass.name == "hkArrayTypeAttribute");
    CHECK(attr.value().klass.declaredMembers.size() == 1);
    REQUIRE(attr.value().enums.size() == 2); // declared + referenced by the member (same table)
    CHECK(attr.value().enums.at(0).items.size() == 3);

    const auto e = hkdump::DecodeEnum(view, Addr(hkArrayTypeAttributeEnums.at(0)));
    REQUIRE(e.has_value());
    CHECK(e.value().items.at(1).name == "POINTSOUP");
}

TEST_CASE("DecodeClass rejects broken tables", "[scanner]") {
    const auto view = RegionView();
    FillGarbage();

    SECTION("unreadable address") {
        CHECK_FALSE(hkdump::DecodeClass(view, 0).has_value());
        CHECK_FALSE(hkdump::DecodeClass(view, 0x10).has_value());
    }
    SECTION("name is not an identifier") {
        auto c = hkAabb;
        c.name = P("1abc");
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
        c.name = P("has space");
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
    }
    SECTION("insane counts / sizes") {
        auto c = hkAabb;
        c.numDeclaredMembers = -1;
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
        c = hkAabb;
        c.objectSize = 1 << 24;
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
        c = hkAabb;
        c.objectSize = 0; // NiRTTI-like
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
    }
    SECTION("member out of class / bad type") {
        auto members = hkAabbMembers;
        auto c = hkAabb;
        Put(0x200, members);
        c.declaredMembers = Addr(g_region) + 0x200;
        CHECK(hkdump::DecodeClass(view, Put(0, c)).has_value());

        members.at(1).offset = 64; // > objectSize
        Put(0x200, members);
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());

        members = hkAabbMembers;
        members.at(0).type = RE::hkClassMember::TYPE_MAX;
        Put(0x200, members);
        CHECK_FALSE(hkdump::DecodeClass(view, Put(0, c)).has_value());
    }
}

TEST_CASE("ScanClasses scans regions and follows parents / member classes", "[scanner]") {
    FillGarbage();
    // misaligned to the region start on purpose: scanner walks pointer-aligned addresses
    const auto first = Put(0x48, hkbStateMachineActiveTransitionInfo);
    const auto second = Put(0x100, hkArrayTypeAttribute);

    const auto db = hkdump::ScanClasses(RegionView());

    REQUIRE(db.Classes().size() == 6);
    CHECK(db.Classes()[0].address == first);
    CHECK(db.Classes()[1].address == second);
    // closure: member classes and the parent chain of the pointer target
    CHECK(Contains(db, "hkbStateMachineTransitionInfoReference"));
    CHECK(Contains(db, "hkbNodeInternalStateInfo"));
    CHECK(Contains(db, "hkReferencedObject"));
    CHECK(Contains(db, "hkBaseObject"));
}

TEST_CASE("ScanClasses ignores garbage and empty candidates", "[scanner]") {
    FillGarbage();
    // name only, no members / enums / parent -> skipped by the pre-filter
    Put(0x10, hkClass{.name = P("Lonely"), .objectSize = 4});
    CHECK(hkdump::ScanClasses(RegionView()).Classes().empty());
}

TEST_CASE("ScanClasses drops classes whose parent is not a valid hkClass", "[scanner]") {
    FillGarbage();

    // NiRTTI looks like the head of an hkClass: { name, parent } followed by zeros.
    static const hkClass niNode{.name = P("NiNode")};
    Put(0x20, hkClass{.name = P("BSSceneGraph"), .parent = P(&niNode), .objectSize = 8});
    Put(0x100, hkAabb);

    const auto db = hkdump::ScanClasses(RegionView());
    CHECK_FALSE(hkdump::DecodeClass(testing::SelfView(), Addr(niNode)).has_value()); // objectSize 0
    REQUIRE(db.Classes().size() == 1);
    CHECK(db.Classes()[0].name == "hkAabb");
}

TEST_CASE("ClassDb resolves parents and chains", "[scanner]") {
    const auto db = testing::Load({Addr(hkbNodeInternalStateInfo)});
    const auto& cls = testing::Get(db, Addr(hkbNodeInternalStateInfo));

    const auto chain = db.Chain(cls);
    REQUIRE(chain.size() == 3);
    CHECK(chain.at(1).get().name == "hkReferencedObject");
    CHECK(chain.at(2).get().name == "hkBaseObject");
    CHECK_FALSE(db.FindClass(0).has_value());
    CHECK(db.PointerSize() == sizeof(void*));
}
