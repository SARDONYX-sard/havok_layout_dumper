#include <bit>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ClassSerializer.h"
#include "Fixtures.h"
#include "TestView.h"

using namespace fixtures;
using Catch::Matchers::ContainsSubstring;
using testing::Addr;
using testing::Get;

namespace {
    /// Decoded fixtures + serializer reading defaults from this process.
    struct Context {
        hkdump::LiveView view = testing::SelfView();
        hkdump::ClassDb db;
        hkdump::ClassSerializer serializer;

        explicit Context(
            std::initializer_list<std::uint64_t> a_roots, hkdump::JsonFormat a_format = hkdump::JsonFormat::SerdeHkx
        )
            : db(testing::Load(a_roots))
            , serializer(db, view, a_format) {}

        [[nodiscard]] const hkdump::ClassInfo& Class(const fixtures::hkClass& a_class) const {
            return Get(db, Addr(a_class));
        }
        [[nodiscard]] std::string Json(const fixtures::hkClass& a_class) { return serializer.ToJson(Class(a_class)); }
    };
} // namespace

TEST_CASE("ToJson matches serde-hkx class JSON (x86 fields zeroed)", "[serializer]") {
    Context ctx{Addr(hkAabb)};

    // serde-hkx/assets/classes/hkAabb.json with `*_x86` = 0
    CHECK(ctx.Json(hkAabb) == R"({
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
    Context ctx{Addr(hkArrayTypeAttribute), Addr(hkReferencedObject)};

    const auto refObj = ctx.Json(hkReferencedObject);
    CHECK_THAT(refObj, ContainsSubstring(R"("parent": "hkBaseObject")"));
    CHECK_THAT(refObj, ContainsSubstring(R"("vtable": true)"));
    CHECK_THAT(refObj, ContainsSubstring(R"("flags": "FLAGS_NONE|SERIALIZE_IGNORED")"));

    const auto attr = ctx.Json(hkArrayTypeAttribute);
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
    Context ctx{Addr(UnusedEnumHolder)};
    CHECK_THAT(ctx.Json(UnusedEnumHolder), ContainsSubstring(R"("vsubtype": "TYPE_VOID",
      "flags": "00000000")"));
}

TEST_CASE("HasVTable looks at the root class", "[serializer]") {
    Context ctx{Addr(hkBaseObject), Addr(hkbNodeInternalStateInfo), Addr(hkAabb)};
    CHECK(ctx.serializer.HasVTable(ctx.Class(hkBaseObject)));
    CHECK(ctx.serializer.HasVTable(ctx.Class(hkbNodeInternalStateInfo))); // via hkReferencedObject
    CHECK_FALSE(ctx.serializer.HasVTable(ctx.Class(hkAabb)));
}

TEST_CASE("has_string propagation", "[serializer]") {
    Context ctx{Addr(Named), Addr(NamedChild), Addr(Holder), Addr(hkAabb)};
    auto& s = ctx.serializer;

    CHECK(s.ClassHasString(ctx.Class(Named)));
    CHECK(s.ClassHasString(ctx.Class(NamedChild))); // inherited
    CHECK(s.ClassHasString(ctx.Class(Holder)));
    CHECK_FALSE(s.ClassHasString(ctx.Class(hkAabb)));

    const auto& members = ctx.Class(Holder).declaredMembers;
    CHECK_FALSE(s.MemberHasString(members.at(0))); // pointer does not count
    CHECK(s.MemberHasString(members.at(1)));       // embedded struct
    CHECK(s.MemberHasString(members.at(2)));       // hkArray<struct>
    CHECK_FALSE(s.MemberHasString(members.at(3)));

    const auto json = ctx.Json(NamedChild);
    CHECK_THAT(json, ContainsSubstring(R"("parent_has_string": true)"));
    CHECK_THAT(json, ContainsSubstring(R"("has_string": true)"));
}

TEST_CASE("DefaultValue reads hkClass::m_defaults", "[serializer]") {
    Context ctx{Addr(WithDefaults), Addr(hkAabb)};
    const auto& withDefaults = ctx.Class(WithDefaults);

    CHECK(ctx.serializer.DefaultValue(withDefaults, 0) == std::bit_cast<std::uint32_t>(3.0F)); // raw bits
    CHECK(ctx.serializer.DefaultValue(withDefaults, 1) == -64);
    CHECK(ctx.serializer.DefaultValue(withDefaults, 2) == 2);      // enum: read by storage subtype
    CHECK(ctx.serializer.DefaultValue(withDefaults, 3) == 0);      // offset -1: no default
    CHECK(ctx.serializer.DefaultValue(withDefaults, 9) == 0);      // out of range
    CHECK(ctx.serializer.DefaultValue(ctx.Class(hkAabb), 0) == 0); // no defaults table

    CHECK_THAT(ctx.Json(WithDefaults), ContainsSubstring(R"("default": 1077936128)"));
}

TEST_CASE("ParseJsonFormat", "[serializer]") {
    CHECK(hkdump::ParseJsonFormat("serde-hkx") == hkdump::JsonFormat::SerdeHkx);
    CHECK(hkdump::ParseJsonFormat("d-merge") == hkdump::JsonFormat::DMerge);
    CHECK_FALSE(hkdump::ParseJsonFormat("serde_hkx").has_value());
}

TEST_CASE("ToJson matches d-merge-serde-hkx class JSON (x86 fields zeroed)", "[serializer]") {
    Context ctx{{Addr(hkAabb)}, hkdump::JsonFormat::DMerge};

    // d-merge-serde-hkx/assets/classes/hkAabb.json with `*_x86` = 0
    CHECK(ctx.Json(hkAabb) == R"({
  "name": "hkAabb",
  "version": 0,
  "signature": "0x4a948b16",
  "size_x86": 0,
  "size_x86_64": 32,
  "parent_has_ref": true,
  "vtable": false,
  "has_ref": true,
  "members": [
    {
      "name": "min",
      "has_ref": false,
      "ctype": "hkVector4",
      "vtype": "TYPE_VECTOR4",
      "vsubtype": "TYPE_VOID",
      "offset_x86": 0,
      "offset_x86_64": 0,
      "type_size_x86": 0,
      "type_size_x86_64": 16,
      "arrsize": 0,
      "flags": "0",
      "default": 0
    },
    {
      "name": "max",
      "has_ref": false,
      "ctype": "hkVector4",
      "vtype": "TYPE_VECTOR4",
      "vsubtype": "TYPE_VOID",
      "offset_x86": 0,
      "offset_x86_64": 16,
      "type_size_x86": 0,
      "type_size_x86_64": 16,
      "arrsize": 0,
      "flags": "0",
      "default": 0
    }
  ]
}
)");

    Context refObj{{Addr(hkReferencedObject)}, hkdump::JsonFormat::DMerge};
    CHECK_THAT(refObj.Json(hkReferencedObject), ContainsSubstring(R"("flags": "SERIALIZE_IGNORED")"));
}

TEST_CASE("d-merge has_ref (nemesis rule)", "[serializer]") {
    Context ctx{
        {Addr(Named), Addr(NamedChild), Addr(Holder), Addr(hkAabb), Addr(WithDefaults)}, hkdump::JsonFormat::DMerge
    };
    auto& s = ctx.serializer;

    CHECK(s.ClassHasRef(ctx.Class(Named)));
    CHECK(s.ClassHasRef(ctx.Class(NamedChild))); // inherited
    CHECK(s.ClassHasRef(ctx.Class(Holder)));
    CHECK_FALSE(s.ClassHasRef(ctx.Class(hkAabb))); // hkVector4 only

    const auto& holder = ctx.Class(Holder).declaredMembers;
    CHECK(s.MemberHasRef(holder.at(0))); // pointers count
    CHECK(s.MemberHasRef(holder.at(1))); // embedded struct with a string
    CHECK(s.MemberHasRef(holder.at(2))); // hkArray<struct>
    CHECK(s.MemberHasRef(holder.at(3))); // integers count

    const auto& withDefaults = ctx.Class(WithDefaults).declaredMembers;
    CHECK_FALSE(s.MemberHasRef(withDefaults.at(0))); // hkReal
    CHECK_FALSE(s.MemberHasRef(withDefaults.at(2))); // enums never

    // class level is always true, even without references
    const auto json = ctx.Json(hkAabb);
    CHECK_THAT(json, ContainsSubstring(R"("parent_has_ref": true)"));
    CHECK_THAT(json, ContainsSubstring(R"("has_ref": true)"));
}
