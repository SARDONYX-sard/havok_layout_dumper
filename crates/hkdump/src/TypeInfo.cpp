#include "TypeInfo.h"

#include <array>
#include <format>
#include <string>
#include <utility>

#include "RE/Havok/hkClassMember.h"

namespace hkdump {
    using namespace std::literals;
    using enum RE::hkClassMember::Type;
    using enum RE::hkClassMember::FlagValues;

    namespace {
        /// `hkClassMember::Type` -> serde-hkx `vtype` / `vsubtype` string.
        constexpr std::array<std::string_view, TYPE_MAX> kTypeNames{
            "TYPE_VOID"sv,
            "TYPE_BOOL"sv,
            "TYPE_CHAR"sv,
            "TYPE_INT8"sv,
            "TYPE_UINT8"sv,
            "TYPE_INT16"sv,
            "TYPE_UINT16"sv,
            "TYPE_INT32"sv,
            "TYPE_UINT32"sv,
            "TYPE_INT64"sv,
            "TYPE_UINT64"sv,
            "TYPE_REAL"sv,
            "TYPE_VECTOR4"sv,
            "TYPE_QUATERNION"sv,
            "TYPE_MATRIX3"sv,
            "TYPE_ROTATION"sv,
            "TYPE_QSTRANSFORM"sv,
            "TYPE_MATRIX4"sv,
            "TYPE_TRANSFORM"sv,
            "TYPE_ZERO"sv,
            "TYPE_POINTER"sv,
            "TYPE_FUNCTIONPOINTER"sv,
            "TYPE_ARRAY"sv,
            "TYPE_INPLACEARRAY"sv,
            "TYPE_ENUM"sv,
            "TYPE_STRUCT"sv,
            "TYPE_SIMPLEARRAY"sv,
            "TYPE_HOMOGENEOUSARRAY"sv,
            "TYPE_VARIANT"sv,
            "TYPE_CSTRING"sv,
            "TYPE_ULONG"sv,
            "TYPE_FLAGS"sv,
            "TYPE_HALF"sv,
            "TYPE_STRINGPTR"sv,
            "TYPE_RELARRAY"sv,
        };

        /// Havok basic type -> C type string (serde-hkx `ctype` convention).
        std::string basic_ctype(std::uint8_t a_type) {
            switch (a_type) {
            case TYPE_VOID:
                return "void";
            case TYPE_BOOL:
                return "hkBool";
            case TYPE_CHAR:
                return "hkChar";
            case TYPE_INT8:
                return "hkInt8";
            case TYPE_UINT8:
                return "hkUint8";
            case TYPE_INT16:
                return "hkInt16";
            case TYPE_UINT16:
                return "hkUint16";
            case TYPE_INT32:
                return "hkInt32";
            case TYPE_UINT32:
                return "hkUint32";
            case TYPE_INT64:
                return "hkInt64";
            case TYPE_UINT64:
                return "hkUint64";
            case TYPE_REAL:
                return "hkReal";
            case TYPE_VECTOR4:
                return "hkVector4";
            case TYPE_QUATERNION:
                return "hkQuaternion";
            case TYPE_MATRIX3:
                return "hkMatrix3";
            case TYPE_ROTATION:
                return "hkRotation";
            case TYPE_QSTRANSFORM:
                return "hkQsTransform";
            case TYPE_MATRIX4:
                return "hkMatrix4";
            case TYPE_TRANSFORM:
                return "hkTransform";
            case TYPE_ZERO:
                return "hkZero";
            case TYPE_FUNCTIONPOINTER:
                return "void*";
            case TYPE_VARIANT:
                return "hkVariant";
            case TYPE_CSTRING:
                return "char*";
            case TYPE_ULONG:
                return "hkUlong";
            case TYPE_HALF:
                return "hkHalf";
            case TYPE_STRINGPTR:
                return "hkStringPtr";
            default:
                return std::string(TypeName(a_type));
            }
        }
    } // namespace

    std::uint32_t BasicSize(std::uint8_t a_type, std::uint32_t a_pointerSize) noexcept {
        const auto p = a_pointerSize;
        switch (a_type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_INT8:
        case TYPE_UINT8:
            return 1;
        case TYPE_INT16:
        case TYPE_UINT16:
        case TYPE_HALF:
            return 2;
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_REAL:
        case TYPE_RELARRAY: // hkRelArray: u16 size + u16 offset
            return 4;
        case TYPE_INT64:
        case TYPE_UINT64:
            return 8;
        case TYPE_ULONG: // pointer sized
        case TYPE_POINTER:
        case TYPE_FUNCTIONPOINTER:
        case TYPE_CSTRING:
        case TYPE_STRINGPTR:
            return p;
        case TYPE_VECTOR4:
        case TYPE_QUATERNION:
            return 16;
        case TYPE_ARRAY: // T* data; int size; int capacityAndFlags
            return p + 8;
        case TYPE_SIMPLEARRAY: // T* data; int size (+ padding on x64)
        case TYPE_VARIANT:     // void* object; hkClass* class
            return 2 * p;
        case TYPE_HOMOGENEOUSARRAY: // hkClass*; void* data; int size (+ padding on x64)
            return 3 * p;
        case TYPE_MATRIX3:
        case TYPE_ROTATION:
        case TYPE_QSTRANSFORM:
            return 48;
        case TYPE_MATRIX4:
        case TYPE_TRANSFORM:
            return 64;
        default:
            return 0;
        }
    }

    std::uint32_t ElementSize(const MemberInfo& a_member, const ClassDb& a_db) {
        switch (a_member.type) {
        case TYPE_STRUCT: {
            const auto klass = a_db.FindClass(a_member.classAddress);
            return klass ? static_cast<std::uint32_t>(klass->get().objectSize) : 0;
        }
        case TYPE_ENUM:
        case TYPE_FLAGS:
        case TYPE_ZERO:
            return BasicSize(a_member.subtype, a_db.PointerSize());
        default:
            return BasicSize(a_member.type, a_db.PointerSize());
        }
    }

    std::string_view TypeName(std::uint8_t a_type) noexcept {
        return a_type < TYPE_MAX ? kTypeNames[a_type] : "TYPE_VOID"sv;
    }

    std::string MakeCType(const MemberInfo& a_member, const ClassDb& a_db) {
        const auto klass = a_db.FindClass(a_member.classAddress);
        const auto enumeration = a_db.FindEnum(a_member.enumAddress);
        const std::string className{klass ? klass->get().name : ""};
        const std::string enumName{enumeration ? enumeration->get().name : "unknown"};

        // element type inside an array container
        const auto elem = [&]() -> std::string {
            switch (a_member.subtype) {
            case TYPE_POINTER:
                return klass ? className + "*" : "void*";
            case TYPE_STRUCT:
                return "struct " + className;
            case TYPE_ENUM:
                return "enum " + enumName;
            case TYPE_FLAGS:
                return "flags " + enumName;
            default:
                return basic_ctype(a_member.subtype);
            }
        };

        std::string ctype;
        switch (a_member.type) {
        case TYPE_POINTER:
            // NOTE: serde-hkx convention: no `[N]` suffix for pointer C-arrays.
            return klass ? "struct " + className + "*" : "void*";
        case TYPE_STRUCT:
            ctype = "struct " + className;
            break;
        case TYPE_ENUM:
            ctype = "enum " + enumName;
            break;
        case TYPE_FLAGS:
            ctype = "flags " + enumName;
            break;
        case TYPE_ARRAY:
            ctype = "hkArray<" + elem() + ">";
            break;
        case TYPE_INPLACEARRAY:
            ctype = "hkInplaceArray<" + elem() + ">";
            break;
        case TYPE_SIMPLEARRAY:
            ctype = "hkSimpleArray<" + elem() + ">";
            break;
        case TYPE_RELARRAY:
            ctype = "hkRelArray<" + elem() + ">";
            break;
        case TYPE_HOMOGENEOUSARRAY:
            ctype = "hkHomogeneousArray";
            break;
        default:
            ctype = basic_ctype(a_member.type);
            break;
        }

        if (a_member.cArraySize > 0) {
            ctype += std::format("[{}]", a_member.cArraySize);
        }
        return ctype;
    }

    namespace {
        constexpr std::array<std::pair<std::uint16_t, std::string_view>, 4> kFlagNames{{
            {ALIGN_8, "ALIGN_8"},
            {ALIGN_16, "ALIGN_16"},
            {NOT_OWNED, "NOT_OWNED"},
            {SERIALIZE_IGNORED, "SERIALIZE_IGNORED"},
        }};

        /// Bits without a name. DEPRECATED_SIZE_* (enum storage size) is already expressed by `vsubtype`.
        constexpr std::uint16_t UnknownFlags(std::uint16_t a_flags) noexcept {
            constexpr std::uint16_t known = DEPRECATED_SIZE_8 | DEPRECATED_SIZE_16 | DEPRECATED_SIZE_32 | ALIGN_8
                | ALIGN_16 | NOT_OWNED | SERIALIZE_IGNORED;
            return static_cast<std::uint16_t>(a_flags & ~known);
        }
    } // namespace

    std::string MakeFlags(std::uint16_t a_flags) {
        std::string result = "FLAGS_NONE";
        for (const auto& [bit, name] : kFlagNames) {
            if (a_flags & bit) {
                result += '|';
                result += name;
            }
        }
        if (const auto unknown = UnknownFlags(a_flags); unknown != 0) {
            result += std::format("|0x{:x}", unknown);
        }
        return result;
    }

    std::string MakeDMergeFlags(std::uint16_t a_flags) {
        std::string result;
        const auto append = [&](std::string_view a_part) {
            if (!result.empty()) {
                result += '|';
            }
            result += a_part;
        };
        for (const auto& [bit, name] : kFlagNames) {
            if (a_flags & bit) {
                append(name);
            }
        }
        if (const auto unknown = UnknownFlags(a_flags); unknown != 0) {
            append(std::to_string(unknown));
        }
        return result.empty() ? "0" : result;
    }
} // namespace hkdump
