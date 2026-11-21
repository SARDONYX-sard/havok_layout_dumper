#include <bit>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ClassSerializer.h"
#include "Fixtures.h"

using namespace fixtures;
using Catch::Matchers::ContainsSubstring;

namespace {
    hkdump::ClassSerializer MakeSerializer(std::vector<hkdump::ClassRef> a_all = {}) {
        return {a_all, hkdump::MemoryRange::Everything()};
    }
} // namespace

TEST_CASE("ToJson matches serde-hkx class JSON (x86 fields zeroed)", "[serializer]") {
    auto serializer = MakeSerializer();

    // serde-hkx/assets/classes/hkAabb.json with `*_x86` = 0
    CHECK(serializer.ToJson(hkAabb) == R"({
  "name": "hkAabb",
  "version": 0,
  "signature": "0x4a948b16",
  "size_x86": 0,
  "size_x86_64": 32,
  "parent_has_string": false,
  "vtable": false,
  "has_string": false,
  "members": [
    {
      "name": "min",
      "has_string": false,
      "ctype": "hkVector4",
      "vtype": "TYPE_VECTOR4",
      "vsubtype": "TYPE_VOID",
      "offset_x86": 0,
      "offset_x86_64": 0,
      "type_size_x86": 0,
      "type_size_x86_64": 16,
      "arrsize": 0,
      "flags": "FLAGS_NONE",
      "default": 0
    },
    {
      "name": "max",
      "has_string": false,
      "ctype": "hkVector4",
      "vtype": "TYPE_VECTOR4",
      "vsubtype": "TYPE_VOID",
      "offset_x86": 0,
      "offset_x86_64": 16,
      "type_size_x86": 0,
      "type_size_x86_64": 16,
      "arrsize": 0,
      "flags": "FLAGS_NONE",
      "default": 0
    }
  ]
}
)");
}

TEST_CASE("ToJson writes parent, vtable and enums", "[serializer]") {
    auto serializer = MakeSerializer({hkArrayTypeAttribute, hkReferencedObject});

    const auto refObj = serializer.ToJson(hkReferencedObject);
    CHECK_THAT(refObj, ContainsSubstring(R"("parent": "hkBaseObject")"));
    CHECK_THAT(refObj, ContainsSubstring(R"("vtable": true)"));
    CHECK_THAT(refObj, ContainsSubstring(R"("flags": "FLAGS_NONE|SERIALIZE_IGNORED")"));

    const auto attr = serializer.ToJson(hkArrayTypeAttribute);
    CHECK_THAT(attr, ContainsSubstring(R"("signature": "0xd404a39a")"));
    // enum storage type is inferred from the member using it
    CHECK_THAT(attr, ContainsSubstring(R"(  "enums": [
    {
      "name": "ArrayType",
      "vtype": "TYPE_ENUM",
      "vsubtype": "TYPE_INT8",
      "flags": "00000000",
      "enum_item": [
        {
          "name": "NONE",
          "value": 0
        },)"));
    CHECK_THAT(attr, ContainsSubstring(R"("enum_ref": "ArrayType")"));
    CHECK_THAT(attr, ContainsSubstring(R"("ctype": "enum ArrayType")"));
}

TEST_CASE("Unused enum falls back to TYPE_ENUM / TYPE_VOID", "[serializer]") {
    auto serializer = MakeSerializer(); // no usage information
    CHECK_THAT(serializer.ToJson(hkArrayTypeAttribute), ContainsSubstring(R"("vsubtype": "TYPE_VOID",
      "flags": "00000000")"));
}

TEST_CASE("HasVTable looks at the root class", "[serializer]") {
    CHECK(hkdump::ClassSerializer::HasVTable(hkBaseObject));
    CHECK(hkdump::ClassSerializer::HasVTable(hkbNodeInternalStateInfo)); // via hkReferencedObject
    CHECK_FALSE(hkdump::ClassSerializer::HasVTable(hkAabb));
}

TEST_CASE("has_string propagation", "[serializer]") {
    auto serializer = MakeSerializer();

    CHECK(serializer.ClassHasString(Named));
    CHECK(serializer.ClassHasString(NamedChild)); // inherited
    CHECK(serializer.ClassHasString(Holder));
    CHECK_FALSE(serializer.ClassHasString(hkAabb));

    CHECK_FALSE(serializer.MemberHasString(HolderMembers[0])); // pointer does not count
    CHECK(serializer.MemberHasString(HolderMembers[1]));       // embedded struct
    CHECK(serializer.MemberHasString(HolderMembers[2]));       // hkArray<struct>
    CHECK_FALSE(serializer.MemberHasString(HolderMembers[3]));

    const auto json = serializer.ToJson(NamedChild);
    CHECK_THAT(json, ContainsSubstring(R"("parent_has_string": true)"));
    CHECK_THAT(json, ContainsSubstring(R"("has_string": true)"));
}

TEST_CASE("DefaultValue reads hkClass::m_defaults", "[serializer]") {
    auto serializer = MakeSerializer();
    const auto& m = WithDefaultsMembers;

    CHECK(serializer.DefaultValue(WithDefaults, 0, m[0]) == std::bit_cast<std::uint32_t>(3.0F)); // raw bits
    CHECK(serializer.DefaultValue(WithDefaults, 1, m[1]) == -64);
    CHECK(serializer.DefaultValue(WithDefaults, 2, m[2]) == 2);       // enum: read by storage subtype
    CHECK(serializer.DefaultValue(WithDefaults, 3, m[3]) == 0);       // offset -1: no default
    CHECK(serializer.DefaultValue(hkAabb, 0, hkAabbMembers[0]) == 0); // no defaults table

    CHECK_THAT(serializer.ToJson(WithDefaults), ContainsSubstring(R"("default": 1077936128)"));
}
