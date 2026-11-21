#include <catch2/catch_test_macros.hpp>

#include "Fixtures.h"
#include "TypeInfo.h"

using namespace fixtures;

TEST_CASE("TypeName", "[type_info]") {
    CHECK(hkdump::TypeName(TYPE_VOID) == "TYPE_VOID");
    CHECK(hkdump::TypeName(TYPE_VECTOR4) == "TYPE_VECTOR4");
    CHECK(hkdump::TypeName(TYPE_RELARRAY) == "TYPE_RELARRAY");
    CHECK(hkdump::TypeName(TYPE_MAX) == "TYPE_VOID"); // out of range
}

TEST_CASE("ElementSize (x64)", "[type_info]") {
    CHECK(hkdump::ElementSize(Member("a", TYPE_BOOL, TYPE_VOID, 0)) == 1);
    CHECK(hkdump::ElementSize(Member("a", TYPE_REAL, TYPE_VOID, 0)) == 4);
    CHECK(hkdump::ElementSize(Member("a", TYPE_POINTER, TYPE_STRUCT, 0)) == 8);
    CHECK(hkdump::ElementSize(Member("a", TYPE_ARRAY, TYPE_INT32, 0)) == 16);
    CHECK(hkdump::ElementSize(Member("a", TYPE_QSTRANSFORM, TYPE_VOID, 0)) == 48);
    CHECK(hkdump::ElementSize(Member("a", TYPE_ENUM, TYPE_UINT16, 0)) == 2);
    CHECK(hkdump::ElementSize(Member("a", TYPE_FLAGS, TYPE_UINT32, 0)) == 4);
    CHECK(hkdump::ElementSize(Member("a", TYPE_STRUCT, TYPE_VOID, 0, FLAGS_NONE, &hkAabb)) == 32);
}

TEST_CASE("MakeCType follows serde-hkx conventions", "[type_info]") {
    CHECK(hkdump::MakeCType(Member("a", TYPE_REAL, TYPE_VOID, 0)) == "hkReal");
    CHECK(hkdump::MakeCType(Member("a", TYPE_REAL, TYPE_VOID, 0, FLAGS_NONE, nullptr, nullptr, 16)) == "hkReal[16]");
    CHECK(hkdump::MakeCType(Member("a", TYPE_CSTRING, TYPE_VOID, 0)) == "char*");
    CHECK(hkdump::MakeCType(Member("a", TYPE_POINTER, TYPE_VOID, 0)) == "void*");
    CHECK(hkdump::MakeCType(Member("a", TYPE_POINTER, TYPE_STRUCT, 0, FLAGS_NONE, &hkAabb)) == "struct hkAabb*");
    // pointer C-arrays have no `[N]` suffix
    CHECK(
        hkdump::MakeCType(Member("a", TYPE_POINTER, TYPE_STRUCT, 0, FLAGS_NONE, &hkAabb, nullptr, 3))
        == "struct hkAabb*"
    );
    CHECK(hkdump::MakeCType(Member("a", TYPE_STRUCT, TYPE_VOID, 0, FLAGS_NONE, &hkAabb)) == "struct hkAabb");
    CHECK(
        hkdump::MakeCType(Member("a", TYPE_STRUCT, TYPE_VOID, 0, FLAGS_NONE, &hkAabb, nullptr, 2)) == "struct hkAabb[2]"
    );
    CHECK(hkdump::MakeCType(Member("a", TYPE_ARRAY, TYPE_VOID, 0)) == "hkArray<void>");
    CHECK(hkdump::MakeCType(Member("a", TYPE_ARRAY, TYPE_POINTER, 0)) == "hkArray<void*>");
    CHECK(hkdump::MakeCType(Member("a", TYPE_ARRAY, TYPE_POINTER, 0, FLAGS_NONE, &hkAabb)) == "hkArray<hkAabb*>");
    CHECK(hkdump::MakeCType(Member("a", TYPE_ARRAY, TYPE_STRUCT, 0, FLAGS_NONE, &hkAabb)) == "hkArray<struct hkAabb>");
    CHECK(hkdump::MakeCType(Member("a", TYPE_ARRAY, TYPE_STRINGPTR, 0)) == "hkArray<hkStringPtr>");
    CHECK(
        hkdump::MakeCType(Member("a", TYPE_SIMPLEARRAY, TYPE_STRUCT, 0, FLAGS_NONE, &hkAabb))
        == "hkSimpleArray<struct hkAabb>"
    );
    CHECK(
        hkdump::MakeCType(Member("a", TYPE_ENUM, TYPE_INT8, 0, FLAGS_NONE, nullptr, hkArrayTypeAttributeEnums.data()))
        == "enum ArrayType"
    );
    CHECK(hkdump::MakeCType(Member("a", TYPE_ENUM, TYPE_INT8, 0)) == "enum unknown");
    CHECK(hkdump::MakeCType(Member("a", TYPE_FLAGS, TYPE_UINT8, 0)) == "flags unknown");
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
