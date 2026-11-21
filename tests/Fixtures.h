#pragma once

// Hand written hkClass reflection tables used as test fixtures.
//
// Classes and expected signatures are taken from serde-hkx `assets/classes` (Havok 2010.2,
// Skyrim). The signature algorithm was verified against that data set, so these act as
// golden values for `hkdump::Signature`.

#include <array>
#include <cstdint>

#include "RE/Havok/hkClass.h"

namespace fixtures {
    using RE::hkClass;
    using RE::hkClassEnum;
    using RE::hkClassMember;
    using enum RE::hkClassMember::Type;
    using enum RE::hkClassMember::FlagValues;

    constexpr hkClassMember Member(
        const char* a_name,
        std::uint8_t a_type,
        std::uint8_t a_subtype,
        std::uint16_t a_offset,
        std::uint16_t a_flags = FLAGS_NONE,
        const hkClass* a_class = nullptr,
        const hkClassEnum* a_enum = nullptr,
        std::int16_t a_arrsize = 0
    ) {
        return {
            .name = a_name,
            .klass = a_class,
            .enumClass = a_enum,
            .type = a_type,
            .subtype = a_subtype,
            .cArraySize = a_arrsize,
            .flags = a_flags,
            .offset = a_offset,
            .attributes = nullptr,
        };
    }

    // ---- hkBaseObject (0xe0708a00): vtable root, no members
    inline const hkClass hkBaseObject{
        .name = "hkBaseObject",
        .objectSize = 8,
        .numImplementedInterfaces = 1,
    };

    // ---- hkReferencedObject (0x3b1c1113): parent + SERIALIZE_IGNORED members
    inline const std::array hkReferencedObjectMembers{
        Member("memSizeAndFlags", TYPE_UINT16, TYPE_VOID, 8, SERIALIZE_IGNORED),
        Member("referenceCount", TYPE_INT16, TYPE_VOID, 10, SERIALIZE_IGNORED),
    };
    inline const hkClass hkReferencedObject{
        .name = "hkReferencedObject",
        .parent = &hkBaseObject,
        .objectSize = 16,
        .declaredMembers = hkReferencedObjectMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(hkReferencedObjectMembers.size()),
    };

    // ---- hkAabb (0x4a948b16): plain struct
    inline const std::array hkAabbMembers{
        Member("min", TYPE_VECTOR4, TYPE_VOID, 0),
        Member("max", TYPE_VECTOR4, TYPE_VOID, 16),
    };
    inline const hkClass hkAabb{
        .name = "hkAabb",
        .objectSize = 32,
        .declaredMembers = hkAabbMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(hkAabbMembers.size()),
    };

    // ---- hkArrayTypeAttribute (0xd404a39a): declared enum + enum member
    inline const std::array ArrayTypeItems{
        hkClassEnum::Item{.value = 0, .name = "NONE"},
        hkClassEnum::Item{.value = 1, .name = "POINTSOUP"},
        hkClassEnum::Item{.value = 2, .name = "ENTITIES"},
    };
    inline const std::array hkArrayTypeAttributeEnums{
        hkClassEnum{
            .name = "ArrayType",
            .items = ArrayTypeItems.data(),
            .numItems = static_cast<std::int32_t>(ArrayTypeItems.size()),
        },
    };
    inline const std::array hkArrayTypeAttributeMembers{
        Member("type", TYPE_ENUM, TYPE_INT8, 0, FLAGS_NONE, nullptr, hkArrayTypeAttributeEnums.data()),
    };
    inline const hkClass hkArrayTypeAttribute{
        .name = "hkArrayTypeAttribute",
        .objectSize = 1,
        .declaredEnums = hkArrayTypeAttributeEnums.data(),
        .numDeclaredEnums = static_cast<std::int32_t>(hkArrayTypeAttributeEnums.size()),
        .declaredMembers = hkArrayTypeAttributeMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(hkArrayTypeAttributeMembers.size()),
    };

    // ---- hkbStateMachineTransitionInfoReference (0x9810c2d0)
    inline const std::array hkbStateMachineTransitionInfoReferenceMembers{
        Member("fromStateIndex", TYPE_INT16, TYPE_VOID, 0),
        Member("transitionIndex", TYPE_INT16, TYPE_VOID, 2),
        Member("stateMachineId", TYPE_INT16, TYPE_VOID, 4),
    };
    inline const hkClass hkbStateMachineTransitionInfoReference{
        .name = "hkbStateMachineTransitionInfoReference",
        .objectSize = 6,
        .declaredMembers = hkbStateMachineTransitionInfoReferenceMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(hkbStateMachineTransitionInfoReferenceMembers.size()),
    };

    // pointer target only (its own layout does not affect the referrer's signature)
    inline const hkClass hkbNodeInternalStateInfo{
        .name = "hkbNodeInternalStateInfo",
        .parent = &hkReferencedObject,
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
        .name = "hkbStateMachineActiveTransitionInfo",
        .objectSize = 40,
        .declaredMembers = hkbStateMachineActiveTransitionInfoMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(hkbStateMachineActiveTransitionInfoMembers.size()),
    };

    // ---- strings: has_string propagation (direct / embedded struct / array of struct / pointer)
    inline const std::array NamedMembers{
        Member("name", TYPE_STRINGPTR, TYPE_VOID, 0),
    };
    inline const hkClass Named{
        .name = "Named",
        .objectSize = 8,
        .declaredMembers = NamedMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(NamedMembers.size()),
    };
    inline const std::array HolderMembers{
        Member("ptr", TYPE_POINTER, TYPE_STRUCT, 0, FLAGS_NONE, &Named),
        Member("embedded", TYPE_STRUCT, TYPE_VOID, 8, FLAGS_NONE, &Named),
        Member("list", TYPE_ARRAY, TYPE_STRUCT, 16, FLAGS_NONE, &Named),
        Member("value", TYPE_INT32, TYPE_VOID, 32),
    };
    inline const hkClass Holder{
        .name = "Holder",
        .objectSize = 40,
        .declaredMembers = HolderMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(HolderMembers.size()),
    };
    inline const hkClass NamedChild{
        .name = "NamedChild",
        .parent = &Named,
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
        .real = 3.0f,
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
        .name = "WithDefaults",
        .objectSize = 16,
        .declaredMembers = WithDefaultsMembers.data(),
        .numDeclaredMembers = static_cast<std::int32_t>(WithDefaultsMembers.size()),
        .defaults = &WithDefaultsDefaults,
    };
} // namespace fixtures
