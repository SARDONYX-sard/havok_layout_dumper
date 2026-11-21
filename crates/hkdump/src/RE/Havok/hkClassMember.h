#pragma once

// Havok `hkClassMember` constants (member type / flag values). Layout: see hkClass.h.

#include <cstdint>

namespace RE {
    struct hkClassMember {
        // Mirrors the Havok declaration: unscoped (used as bit flags / `using enum`) with the field's width.
        // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class,performance-enum-size)
        enum Type : std::uint8_t {
            TYPE_VOID = 0,
            TYPE_BOOL,
            TYPE_CHAR,
            TYPE_INT8,
            TYPE_UINT8,
            TYPE_INT16,
            TYPE_UINT16,
            TYPE_INT32,
            TYPE_UINT32,
            TYPE_INT64,
            TYPE_UINT64,
            TYPE_REAL,
            TYPE_VECTOR4,
            TYPE_QUATERNION,
            TYPE_MATRIX3,
            TYPE_ROTATION,
            TYPE_QSTRANSFORM,
            TYPE_MATRIX4,
            TYPE_TRANSFORM,
            TYPE_ZERO,
            TYPE_POINTER,
            TYPE_FUNCTIONPOINTER,
            TYPE_ARRAY,
            TYPE_INPLACEARRAY,
            TYPE_ENUM,
            TYPE_STRUCT,
            TYPE_SIMPLEARRAY,
            TYPE_HOMOGENEOUSARRAY,
            TYPE_VARIANT,
            TYPE_CSTRING,
            TYPE_ULONG,
            TYPE_FLAGS,
            TYPE_HALF,
            TYPE_STRINGPTR,
            TYPE_RELARRAY,
            TYPE_MAX,
        };

        // NOLINTNEXTLINE(cppcoreguidelines-use-enum-class,performance-enum-size)
        enum FlagValues : std::uint16_t {
            FLAGS_NONE = 0,
            DEPRECATED_SIZE_8 = 8,
            DEPRECATED_SIZE_16 = 16,
            DEPRECATED_SIZE_32 = 32,
            ALIGN_8 = 128,
            ALIGN_16 = 256,
            NOT_OWNED = 512,
            SERIALIZE_IGNORED = 1024,
        };
    };
} // namespace RE
