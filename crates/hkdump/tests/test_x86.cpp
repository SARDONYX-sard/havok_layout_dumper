// Skyrim LE (x86) support: decode a synthetic 32-bit snapshot with the same code as x64.

#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <string_view>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ClassSerializer.h"
#include "MemoryView.h"
#include "RE/Havok/hkClass.h"
#include "RE/Havok/hkClassMember.h"
#include "Scanner.h"
#include "Signature.h"

using Catch::Matchers::ContainsSubstring;

namespace {
    using enum RE::hkClassMember::Type;
    using hkClass32 = RE::hkClassT<std::uint32_t>;
    using hkClassMember32 = RE::hkClassMemberT<std::uint32_t>;
    using hkClassEnum32 = RE::hkClassEnumT<std::uint32_t>;
    using hkClassEnumItem32 = RE::hkClassEnumItemT<std::uint32_t>;

    constexpr std::uint32_t kBase = 0x00400000;
    constexpr std::uint32_t kRva = 0x1000;

    /// Builds a 32-bit `.data` region at `kBase + kRva` out of x86 reflection structs.
    class Image32 {
    public:
        template<class T>
        std::uint32_t Put(const T& a_value) {
            const auto address = Alloc(sizeof(T));
            std::memcpy(_bytes.data() + (address - kBase - kRva), &a_value, sizeof(T));
            return address;
        }

        template<class T>
        std::uint32_t PutArray(std::initializer_list<T> a_values) {
            const auto first = Alloc(0);
            for (const auto& value : a_values) {
                Put(value);
            }
            return first;
        }

        std::uint32_t String(std::string_view a_text) {
            const auto address = Alloc(a_text.size() + 1);
            std::memcpy(_bytes.data() + (address - kBase - kRva), a_text.data(), a_text.size());
            return address;
        }

        [[nodiscard]] hkdump::SnapshotView View() const {
            return hkdump::SnapshotView{hkdump::ImageSnapshot{
                .imageBase = kBase,
                .imageSize = 0x100000,
                .pointerSize = 4,
                .regions = {{.rva = kRva, .bytes = _bytes}},
            }};
        }

    private:
        std::uint32_t Alloc(std::size_t a_size) {
            const auto offset = (_bytes.size() + 3) & ~std::size_t{3};
            _bytes.resize(offset + a_size);
            return kBase + kRva + static_cast<std::uint32_t>(offset);
        }

        std::vector<std::byte> _bytes;
    };

    hkClassMember32 Member(
        Image32& a_image,
        std::string_view a_name,
        std::uint8_t a_type,
        std::uint8_t a_subtype,
        std::uint16_t a_offset,
        std::uint32_t a_enum = 0
    ) {
        return {
            .name = a_image.String(a_name),
            .enumClass = a_enum,
            .type = a_type,
            .subtype = a_subtype,
            .offset = a_offset,
        };
    }
} // namespace

TEST_CASE("x86 snapshot: hkAabb", "[x86]") {
    Image32 image;
    const auto members = image.PutArray({
        Member(image, "min", TYPE_VECTOR4, TYPE_VOID, 0),
        Member(image, "max", TYPE_VECTOR4, TYPE_VOID, 16),
    });
    const auto aabb = image.Put(
        hkClass32{
            .name = image.String("hkAabb"),
            .objectSize = 32,
            .declaredMembers = members,
            .numDeclaredMembers = 2,
        }
    );

    const auto view = image.View();
    const auto db = hkdump::ScanClasses(view);
    REQUIRE(db.Classes().size() == 1);
    const auto& cls = db.Classes()[0];
    CHECK(cls.address == aabb);
    CHECK(db.PointerSize() == 4);
    CHECK(hkdump::Signature(cls, db) == 0x4a948b16U); // same as x64: layout independent

    hkdump::ClassSerializer serializer{db, view};
    const auto json = serializer.ToJson(cls);
    CHECK_THAT(json, ContainsSubstring(R"("size_x86": 32,
  "size_x86_64": 0,)"));
    CHECK_THAT(json, ContainsSubstring(R"("offset_x86": 16,
      "offset_x86_64": 0,
      "type_size_x86": 16,
      "type_size_x86_64": 0,)"));
}

TEST_CASE("x86 snapshot: enums, items and pointer sized members", "[x86]") {
    Image32 image;

    const auto items = image.PutArray({
        hkClassEnumItem32{.value = 0, .name = image.String("NONE")},
        hkClassEnumItem32{.value = 1, .name = image.String("POINTSOUP")},
        hkClassEnumItem32{.value = 2, .name = image.String("ENTITIES")},
    });
    const auto enumeration = image.Put(hkClassEnum32{.name = image.String("ArrayType"), .items = items, .numItems = 3});
    const auto attr = image.Put(
        hkClass32{
            .name = image.String("hkArrayTypeAttribute"),
            .objectSize = 1,
            .declaredEnums = enumeration,
            .numDeclaredEnums = 1,
            .declaredMembers = image.PutArray({Member(image, "type", TYPE_ENUM, TYPE_INT8, 0, enumeration)}),
            .numDeclaredMembers = 1,
        }
    );

    // pointer / array sizes follow the target (x86) pointer size
    const auto holder = image.Put(
        hkClass32{
            .name = image.String("Holder32"),
            .objectSize = 20,
            .declaredMembers = image.PutArray({
                Member(image, "p", TYPE_POINTER, TYPE_VOID, 0),
                Member(image, "arr", TYPE_ARRAY, TYPE_INT32, 4),
                Member(image, "s", TYPE_CSTRING, TYPE_VOID, 16),
            }),
            .numDeclaredMembers = 3,
        }
    );

    const auto view = image.View();
    const std::vector<std::uint64_t> roots{attr, holder};
    const auto db = hkdump::LoadClasses(view, roots);
    REQUIRE(db.Classes().size() == 2);
    CHECK(hkdump::Signature(db.FindClass(attr).value(), db) == 0xd404a39aU);
    REQUIRE(db.FindEnum(enumeration).has_value());
    CHECK(db.FindEnum(enumeration).value().get().items.at(2).name == "ENTITIES");

    hkdump::ClassSerializer serializer{db, view};
    const auto json = serializer.ToJson(db.FindClass(holder).value());
    CHECK_THAT(json, ContainsSubstring(R"("name": "arr",
      "has_string": false,
      "ctype": "hkArray<hkInt32>",
      "vtype": "TYPE_ARRAY",
      "vsubtype": "TYPE_INT32",
      "offset_x86": 4,
      "offset_x86_64": 0,
      "type_size_x86": 12,)"));
    CHECK_THAT(json, ContainsSubstring(R"("has_string": true)"));
}
