#pragma once

// Hand written hkClass reflection tables used as test fixtures.
//
// Classes and expected signatures are taken from serde-hkx `assets/classes` (Havok 2010.2,
// Skyrim). The signature algorithm was verified against that data set, so these act as
// golden values for `hkdump::Signature`.

#include <array>
#include <cstdint>

#include "RE/Havok/hkClass.h"
#include "RE/Havok/hkClassMember.h"

namespace fixtures {
    using enum RE::hkClassMember::Type;
    using enum RE::hkClassMember::FlagValues;

    // In-process (x64) tables: target pointers are this process' addresses as integers.
    using Ptr = std::uint64_t;
    using hkClass = RE::hkClassT<Ptr>;
    using hkClassMember = RE::hkClassMemberT<Ptr>;
    using hkClassEnum = RE::hkClassEnumT<Ptr>;
    using hkClassEnumItem = RE::hkClassEnumItemT<Ptr>;

    /// Address of a fixture / string literal as a target pointer.
    inline Ptr P(const void* a_object) noexcept { return reinterpret_cast<std::uintptr_t>(a_object); }

    inline hkClassMember Member(
        const char* a_name,
        std::uint8_t a_type,
        std::uint8_t a_subtype,
        std::uint16_t a_offset,
        std::uint16_t a_flags = FLAGS_NONE,
        const hkClass* a_class = nullptr,
        const hkClassEnum* a_enum = nullptr,
        std::int16_t a_arrsize = 0
    ) noexcept {
        return {
            .name = P(a_name),
            .klass = P(a_class),
            .enumClass = P(a_enum),
            .type = a_type,
            .subtype = a_subtype,
            .cArraySize = a_arrsize,
            .flags = a_flags,
            .offset = a_offset,
            .attributes = 0,
        };
    }

    // ---- hkBaseObject (0xe0708a00): vtable root, no members
    inline const hkClass hkBaseObject{
        .name = P("hkBaseObject"),
        .objectSize = 8,
        .numImplementedInterfaces = 1,
    };

    // ---- hkReferencedObject (0x3b1c1113): parent + SERIALIZE_IGNORED members
    inline const std::array hkReferencedObjectMembers{
        Member("memSizeAndFlags", TYPE_UINT16, TYPE_VOID, 8, SERIALIZE_IGNORED),
        Member("referenceCount", TYPE_INT16, TYPE_VOID, 10, SERIALIZE_IGNORED),
    };
    inline const hkClass hkReferencedObject{
        .name = P("hkReferencedObject"),
        .parent = P(&hkBaseObject),
        .objectSize = 16,
        .declaredMembers = P(hkReferencedObjectMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(hkReferencedObjectMembers.size()),
    };

    // ---- hkAabb (0x4a948b16): plain struct
    inline const std::array hkAabbMembers{
        Member("min", TYPE_VECTOR4, TYPE_VOID, 0),
        Member("max", TYPE_VECTOR4, TYPE_VOID, 16),
    };
    inline const hkClass hkAabb{
        .name = P("hkAabb"),
        .objectSize = 32,
        .declaredMembers = P(hkAabbMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(hkAabbMembers.size()),
    };

    // ---- hkArrayTypeAttribute (0xd404a39a): declared enum + enum member
    inline const std::array ArrayTypeItems{
        hkClassEnumItem{.value = 0, .name = P("NONE")},
        hkClassEnumItem{.value = 1, .name = P("POINTSOUP")},
        hkClassEnumItem{.value = 2, .name = P("ENTITIES")},
    };
    inline const std::array hkArrayTypeAttributeEnums{
        hkClassEnum{
            .name = P("ArrayType"),
            .items = P(ArrayTypeItems.data()),
            .numItems = static_cast<std::int32_t>(ArrayTypeItems.size()),
        },
    };
    inline const std::array hkArrayTypeAttributeMembers{
        Member("type", TYPE_ENUM, TYPE_INT8, 0, FLAGS_NONE, nullptr, hkArrayTypeAttributeEnums.data())
    };
    inline const hkClass hkArrayTypeAttribute{
        .name = P("hkArrayTypeAttribute"),
        .objectSize = 1,
        .declaredEnums = P(hkArrayTypeAttributeEnums.data()),
        .numDeclaredEnums = static_cast<std::int32_t>(hkArrayTypeAttributeEnums.size()),
        .declaredMembers = P(hkArrayTypeAttributeMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(hkArrayTypeAttributeMembers.size()),
    };

    // declares an enum that no member uses (storage type unknown)
    inline const std::array UnusedEnumEnums{
        hkClassEnum{
            .name = P("Unused"),
            .items = P(ArrayTypeItems.data()),
            .numItems = static_cast<std::int32_t>(ArrayTypeItems.size()),
        },
    };
    inline const hkClass UnusedEnumHolder{
        .name = P("UnusedEnumHolder"),
        .objectSize = 4,
        .declaredEnums = P(UnusedEnumEnums.data()),
        .numDeclaredEnums = static_cast<std::int32_t>(UnusedEnumEnums.size()),
    };

    // ---- hkbStateMachineTransitionInfoReference (0x9810c2d0)
    inline const std::array hkbStateMachineTransitionInfoReferenceMembers{
        Member("fromStateIndex", TYPE_INT16, TYPE_VOID, 0),
        Member("transitionIndex", TYPE_INT16, TYPE_VOID, 2),
        Member("stateMachineId", TYPE_INT16, TYPE_VOID, 4),
    };
    inline const hkClass hkbStateMachineTransitionInfoReference{
        .name = P("hkbStateMachineTransitionInfoReference"),
        .objectSize = 6,
        .declaredMembers = P(hkbStateMachineTransitionInfoReferenceMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(hkbStateMachineTransitionInfoReferenceMembers.size()),
    };

    // pointer target only (its own layout does not affect the referrer's signature)
    inline const hkClass hkbNodeInternalStateInfo{
        .name = P("hkbNodeInternalStateInfo"),
        .parent = P(&hkReferencedObject),
        .objectSize = 16,
    };

    // ---- hkbStateMachineActiveTransitionInfo (0xbb90d54f): pointers, embedded structs, ignored member
    inline const std::array hkbStateMachineActiveTransitionInfoMembers{
        Member("transitionEffect", TYPE_POINTER, TYPE_VOID, 0, SERIALIZE_IGNORED),
        Member(
            "transitionEffectInternalStateInfo", TYPE_POINTER, TYPE_STRUCT, 8, FLAGS_NONE, &hkbNodeInternalStateInfo
        ),
        Member(
            "transitionInfoReference", TYPE_STRUCT, TYPE_VOID, 16, FLAGS_NONE, &hkbStateMachineTransitionInfoReference
        ),
        Member(
            "transitionInfoReferenceForTE",
            TYPE_STRUCT,
            TYPE_VOID,
            22,
            FLAGS_NONE,
            &hkbStateMachineTransitionInfoReference
        ),
        Member("fromStateId", TYPE_INT32, TYPE_VOID, 28),
        Member("toStateId", TYPE_INT32, TYPE_VOID, 32),
        Member("isReturnToPreviousState", TYPE_BOOL, TYPE_VOID, 36),
    };
    inline const hkClass hkbStateMachineActiveTransitionInfo{
        .name = P("hkbStateMachineActiveTransitionInfo"),
        .objectSize = 40,
        .declaredMembers = P(hkbStateMachineActiveTransitionInfoMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(hkbStateMachineActiveTransitionInfoMembers.size()),
    };

    // ---- strings: has_string propagation (direct / embedded struct / array of struct / pointer)
    inline const std::array NamedMembers{
        Member("name", TYPE_STRINGPTR, TYPE_VOID, 0),
    };
    inline const hkClass Named{
        .name = P("Named"),
        .objectSize = 8,
        .declaredMembers = P(NamedMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(NamedMembers.size()),
    };
    inline const std::array HolderMembers{
        Member("ptr", TYPE_POINTER, TYPE_STRUCT, 0, FLAGS_NONE, &Named),
        Member("embedded", TYPE_STRUCT, TYPE_VOID, 8, FLAGS_NONE, &Named),
        Member("list", TYPE_ARRAY, TYPE_STRUCT, 16, FLAGS_NONE, &Named),
        Member("value", TYPE_INT32, TYPE_VOID, 32),
    };
    inline const hkClass Holder{
        .name = P("Holder"),
        .objectSize = 40,
        .declaredMembers = P(HolderMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(HolderMembers.size()),
    };
    inline const hkClass NamedChild{
        .name = P("NamedChild"),
        .parent = P(&Named),
        .objectSize = 8,
    };

    // ---- defaults: { int32 offsets[N]; values... }
    struct WithDefaultsDefaultStruct {
        std::int32_t offsets[4];
        float real;           // 16
        std::int32_t integer; // 20
        std::uint8_t mode;    // 24
    };
    inline const WithDefaultsDefaultStruct WithDefaultsDefaults{
        .offsets = {16, 20, 24, -1},
        .real = 3.0F,
        .integer = -64,
        .mode = 2,
    };
    inline const std::array WithDefaultsMembers{
        Member("real", TYPE_REAL, TYPE_VOID, 0),
        Member("integer", TYPE_INT32, TYPE_VOID, 4),
        Member("mode", TYPE_ENUM, TYPE_UINT8, 8, FLAGS_NONE, nullptr, hkArrayTypeAttributeEnums.data()),
        Member("noDefault", TYPE_INT32, TYPE_VOID, 12),
    };
    inline const hkClass WithDefaults{
        .name = P("WithDefaults"),
        .objectSize = 16,
        .declaredMembers = P(WithDefaultsMembers.data()),
        .numDeclaredMembers = static_cast<std::int32_t>(WithDefaultsMembers.size()),
        .defaults = P(&WithDefaultsDefaults),
    };
} // namespace fixtures
