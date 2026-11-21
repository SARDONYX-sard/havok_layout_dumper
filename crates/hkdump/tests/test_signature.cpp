#include <string_view>

#include <catch2/catch_test_macros.hpp>

#include "Fixtures.h"
#include "Signature.h"
#include "TestView.h"

using testing::Addr;

namespace {
    std::uint32_t SignatureOf(const fixtures::hkClass& a_class) {
        const auto db = testing::Load({Addr(a_class)});
        return hkdump::Signature(testing::Get(db, Addr(a_class)), db);
    }
} // namespace

TEST_CASE("Crc32Writer matches standard CRC-32", "[signature]") {
    hkdump::Crc32Writer w;
    w.write(std::string_view{"123456789"});
    CHECK(w.get() == 0xCBF43926U);
}

TEST_CASE("Signature matches Havok hkClass::getSignature", "[signature]") {
    using namespace fixtures;

    SECTION("vtable root without members") { CHECK(SignatureOf(hkBaseObject) == 0xe0708a00U); }
    SECTION("parent chain + SERIALIZE_IGNORED members") { CHECK(SignatureOf(hkReferencedObject) == 0x3b1c1113U); }
    SECTION("plain struct") { CHECK(SignatureOf(hkAabb) == 0x4a948b16U); }
    SECTION("declared enum + enum member (storage size folded into flags)") {
        CHECK(SignatureOf(hkArrayTypeAttribute) == 0xd404a39aU);
    }
    SECTION("small struct") { CHECK(SignatureOf(hkbStateMachineTransitionInfoReference) == 0x9810c2d0U); }
    SECTION("pointers, embedded structs and ignored pointer") {
        CHECK(SignatureOf(hkbStateMachineActiveTransitionInfo) == 0xbb90d54fU);
    }
}
