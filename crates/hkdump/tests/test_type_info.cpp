#include <catch2/catch_test_macros.hpp>

#include "Fixtures.h"
#include "TestView.h"
#include "TypeInfo.h"

using namespace fixtures;
using testing::Addr;

namespace {
    hkdump::MemberInfo
    M(std::uint8_t a_type,
      std::uint8_t a_subtype,
      std::uint64_t a_class = 0,
      std::uint64_t a_enum = 0,
      std::int16_t a_arrsize = 0) {
        return {
            .name = "a",
            .classAddress = a_class,
            .enumAddress = a_enum,
            .type = a_type,
            .subtype = a_subtype,
            .cArraySize = a_arrsize,
            .flags = 0,
            .offset = 0,
        };
    }

    const hkdump::ClassDb& Db() {
        static const auto db = testing::Load({Addr(hkAabb), Addr(hkArrayTypeAttribute)});
        return db;
    }
} // namespace

TEST_CASE("TypeName", "[type_info]") {
    CHECK(hkdump::TypeName(TYPE_VOID) == "TYPE_VOID");
    CHECK(hkdump::TypeName(TYPE_VECTOR4) == "TYPE_VECTOR4");
    CHECK(hkdump::TypeName(TYPE_RELARRAY) == "TYPE_RELARRAY");
    CHECK(hkdump::TypeName(TYPE_MAX) == "TYPE_VOID"); // out of range
}

TEST_CASE("BasicSize depends on the target pointer size", "[type_info]") {
    CHECK(hkdump::BasicSize(TYPE_REAL, 8) == 4);
    CHECK(hkdump::BasicSize(TYPE_POINTER, 8) == 8);
    CHECK(hkdump::BasicSize(TYPE_POINTER, 4) == 4);
    CHECK(hkdump::BasicSize(TYPE_ARRAY, 8) == 16);
    CHECK(hkdump::BasicSize(TYPE_ARRAY, 4) == 12);
    CHECK(hkdump::BasicSize(TYPE_ULONG, 4) == 4);
    CHECK(hkdump::BasicSize(TYPE_UINT64, 4) == 8);
    CHECK(hkdump::BasicSize(TYPE_QSTRANSFORM, 4) == 48);
}

TEST_CASE("ElementSize (x64)", "[type_info]") {
    CHECK(hkdump::ElementSize(M(TYPE_BOOL, TYPE_VOID), Db()) == 1);
    CHECK(hkdump::ElementSize(M(TYPE_POINTER, TYPE_STRUCT), Db()) == 8);
    CHECK(hkdump::ElementSize(M(TYPE_ARRAY, TYPE_INT32), Db()) == 16);
    CHECK(hkdump::ElementSize(M(TYPE_ENUM, TYPE_UINT16), Db()) == 2);
    CHECK(hkdump::ElementSize(M(TYPE_FLAGS, TYPE_UINT32), Db()) == 4);
    CHECK(hkdump::ElementSize(M(TYPE_STRUCT, TYPE_VOID, Addr(hkAabb)), Db()) == 32);
}

TEST_CASE("MakeCType follows serde-hkx conventions", "[type_info]") {
    const auto aabb = Addr(hkAabb);
    const auto arrayType = Addr(hkArrayTypeAttributeEnums.at(0));

    CHECK(hkdump::MakeCType(M(TYPE_REAL, TYPE_VOID), Db()) == "hkReal");
    CHECK(hkdump::MakeCType(M(TYPE_REAL, TYPE_VOID, 0, 0, 16), Db()) == "hkReal[16]");
    CHECK(hkdump::MakeCType(M(TYPE_CSTRING, TYPE_VOID), Db()) == "char*");
    CHECK(hkdump::MakeCType(M(TYPE_POINTER, TYPE_VOID), Db()) == "void*");
    CHECK(hkdump::MakeCType(M(TYPE_POINTER, TYPE_STRUCT, aabb), Db()) == "struct hkAabb*");
    // pointer C-arrays have no `[N]` suffix
    CHECK(hkdump::MakeCType(M(TYPE_POINTER, TYPE_STRUCT, aabb, 0, 3), Db()) == "struct hkAabb*");
    CHECK(hkdump::MakeCType(M(TYPE_STRUCT, TYPE_VOID, aabb), Db()) == "struct hkAabb");
    CHECK(hkdump::MakeCType(M(TYPE_STRUCT, TYPE_VOID, aabb, 0, 2), Db()) == "struct hkAabb[2]");
    CHECK(hkdump::MakeCType(M(TYPE_ARRAY, TYPE_VOID), Db()) == "hkArray<void>");
    CHECK(hkdump::MakeCType(M(TYPE_ARRAY, TYPE_POINTER), Db()) == "hkArray<void*>");
    CHECK(hkdump::MakeCType(M(TYPE_ARRAY, TYPE_POINTER, aabb), Db()) == "hkArray<hkAabb*>");
    CHECK(hkdump::MakeCType(M(TYPE_ARRAY, TYPE_STRUCT, aabb), Db()) == "hkArray<struct hkAabb>");
    CHECK(hkdump::MakeCType(M(TYPE_ARRAY, TYPE_STRINGPTR), Db()) == "hkArray<hkStringPtr>");
    CHECK(hkdump::MakeCType(M(TYPE_SIMPLEARRAY, TYPE_STRUCT, aabb), Db()) == "hkSimpleArray<struct hkAabb>");
    CHECK(hkdump::MakeCType(M(TYPE_ENUM, TYPE_INT8, 0, arrayType), Db()) == "enum ArrayType");
    CHECK(hkdump::MakeCType(M(TYPE_ENUM, TYPE_INT8), Db()) == "enum unknown");
    CHECK(hkdump::MakeCType(M(TYPE_FLAGS, TYPE_UINT8), Db()) == "flags unknown");
}

TEST_CASE("MakeFlags", "[type_info]") {
    CHECK(hkdump::MakeFlags(FLAGS_NONE) == "FLAGS_NONE");
    CHECK(hkdump::MakeFlags(ALIGN_16 | SERIALIZE_IGNORED) == "FLAGS_NONE|ALIGN_16|SERIALIZE_IGNORED");
    CHECK(hkdump::MakeFlags(ALIGN_8 | NOT_OWNED) == "FLAGS_NONE|ALIGN_8|NOT_OWNED");
    // enum storage size bits are expressed by vsubtype instead
    CHECK(hkdump::MakeFlags(DEPRECATED_SIZE_8) == "FLAGS_NONE");
    // unknown bits are kept visible
    CHECK(hkdump::MakeFlags(0x0800) == "FLAGS_NONE|0x800");
}

TEST_CASE("MakeDMergeFlags", "[type_info]") {
    CHECK(hkdump::MakeDMergeFlags(FLAGS_NONE) == "0");
    CHECK(hkdump::MakeDMergeFlags(DEPRECATED_SIZE_8) == "0");
    CHECK(hkdump::MakeDMergeFlags(SERIALIZE_IGNORED) == "SERIALIZE_IGNORED");
    CHECK(hkdump::MakeDMergeFlags(ALIGN_16 | SERIALIZE_IGNORED) == "ALIGN_16|SERIALIZE_IGNORED");
    CHECK(hkdump::MakeDMergeFlags(ALIGN_8 | 0x4000) == "ALIGN_8|16384"); // unknown bits: decimal
}
