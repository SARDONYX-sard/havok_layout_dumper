#pragma once

// Havok static reflection tables (`hkClass`, `hkClassMember`, `hkClassEnum`, `hkClassEnum::Item`)
// as they lie in the game's memory, for any target pointer size.
//
// `Ptr` is the target pointer as an integer: `std::uint32_t` = x86 (Skyrim LE),
// `std::uint64_t` = x64 (Skyrim SE / Fallout 4). Natural alignment reproduces the x64 padding,
// so one definition covers both layouts and the decoder can copy them straight out of a snapshot.
//
// Reference: d-merge-serde-hkx/assets/classes/{hkClass,hkClassMember,hkClassEnum,hkClassEnumItem}.json
//            (`offset_x86` / `offset_x86_64`, `size_x86` / `size_x86_64`).

#include <cstddef>
#include <cstdint>

namespace RE {
    //                                     x64    x86
    template<class Ptr>
    struct hkClassEnumItemT {
        std::int32_t value{}; //              00     00
        Ptr name{};           // const char*  08     04
    };

    template<class Ptr>
    struct hkClassEnumT {
        Ptr name{};              // const char*                 00  00
        Ptr items{};             // const hkClassEnumItemT*     08  04
        std::int32_t numItems{}; //                            10  08
        Ptr attributes{};        // const hkCustomAttributes*   18  0C
        std::uint32_t flags{};   //                             20  10
    };

    template<class Ptr>
    struct hkClassMemberT {
        Ptr name{};                // const char*                00  00
        Ptr klass{};               // const hkClassT*            08  04
        Ptr enumClass{};           // const hkClassEnumT*        10  08
        std::uint8_t type{};       // hkClassMember::Type        18  0C
        std::uint8_t subtype{};    // hkClassMember::Type        19  0D
        std::int16_t cArraySize{}; //                           1A  0E
        std::uint16_t flags{};     // hkClassMember::FlagValues  1C  10
        std::uint16_t offset{};    //                            1E  12
        Ptr attributes{};          // const hkCustomAttributes*  20  14
    };

    template<class Ptr>
    struct hkClassT {
        Ptr name{};                              // const char*               00  00
        Ptr parent{};                            // const hkClassT*           08  04
        std::int32_t objectSize{};               //                           10  08
        std::int32_t numImplementedInterfaces{}; //                           14  0C
        Ptr declaredEnums{};                     // const hkClassEnumT*       18  10
        std::int32_t numDeclaredEnums{};         //                           20  14
        Ptr declaredMembers{};                   // const hkClassMemberT*     28  18
        std::int32_t numDeclaredMembers{};       //                           30  1C
        Ptr defaults{};                          // { i32 offsets[]; ... }    38  20
        Ptr attributes{};                        // const hkCustomAttributes* 40  24
        std::uint32_t flags{};                   //                           48  28
        std::int32_t describedVersion{};         //                           4C  2C
    };

    // x64: Skyrim SE / Fallout 4
    static_assert(sizeof(hkClassEnumItemT<std::uint64_t>) == 0x10);
    static_assert(sizeof(hkClassEnumT<std::uint64_t>) == 0x28);
    static_assert(offsetof(hkClassEnumT<std::uint64_t>, attributes) == 0x18);
    static_assert(sizeof(hkClassMemberT<std::uint64_t>) == 0x28);
    static_assert(offsetof(hkClassMemberT<std::uint64_t>, type) == 0x18);
    static_assert(offsetof(hkClassMemberT<std::uint64_t>, attributes) == 0x20);
    static_assert(sizeof(hkClassT<std::uint64_t>) == 0x50);
    static_assert(offsetof(hkClassT<std::uint64_t>, declaredMembers) == 0x28);
    static_assert(offsetof(hkClassT<std::uint64_t>, describedVersion) == 0x4C);

    // x86: Skyrim LE
    static_assert(sizeof(hkClassEnumItemT<std::uint32_t>) == 0x08);
    static_assert(sizeof(hkClassEnumT<std::uint32_t>) == 0x14);
    static_assert(offsetof(hkClassEnumT<std::uint32_t>, attributes) == 0x0C);
    static_assert(sizeof(hkClassMemberT<std::uint32_t>) == 0x18);
    static_assert(offsetof(hkClassMemberT<std::uint32_t>, type) == 0x0C);
    static_assert(offsetof(hkClassMemberT<std::uint32_t>, attributes) == 0x14);
    static_assert(sizeof(hkClassT<std::uint32_t>) == 0x30);
    static_assert(offsetof(hkClassT<std::uint32_t>, declaredMembers) == 0x18);
    static_assert(offsetof(hkClassT<std::uint32_t>, describedVersion) == 0x2C);
} // namespace RE
