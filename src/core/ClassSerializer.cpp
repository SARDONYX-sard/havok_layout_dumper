#include "ClassSerializer.h"

#include <algorithm>
#include <format>

#include "Json.h"
#include "Signature.h"
#include "TypeInfo.h"

namespace hkdump {
    using RE::hkClass;
    using RE::hkClassMember;
    using enum RE::hkClassMember::Type;

    ClassSerializer::ClassSerializer(std::span<const ClassRef> a_all, const MemoryRange& a_range) : _range(a_range) {
        for (const hkClass& cls : a_all) {
            for (const auto& m : cls.GetDeclaredMembers()) {
                if (m.enumClass && (m.type == TYPE_ENUM || m.type == TYPE_FLAGS)) {
                    _enumUsage.try_emplace(m.enumClass, m.type, m.subtype);
                }
            }
        }
    }

    bool ClassSerializer::HasVTable(const hkClass& a_class) noexcept {
        return a_class.GetRoot().numImplementedInterfaces > 0;
    }

    bool ClassSerializer::ClassHasString(const hkClass& a_class) {
        const auto* const key = &a_class;
        if (const auto it = _hasString.find(key); it != _hasString.end()) {
            return it->second;
        }
        _hasString[key] = false; // cycle guard

        const bool result = (a_class.parent && ClassHasString(*a_class.parent))
            || std::ranges::any_of(a_class.GetDeclaredMembers(), [this](const hkClassMember& m) {
                                return MemberHasString(m);
                            });
        _hasString[key] = result;
        return result;
    }

    bool ClassSerializer::MemberHasString(const hkClassMember& a_member) {
        const auto is_str = [](std::uint8_t t) { return t == TYPE_CSTRING || t == TYPE_STRINGPTR; };
        if (is_str(a_member.type) || is_str(a_member.subtype)) {
            return true;
        }
        // Embedded structs (directly or as array elements). Pointers do not count.
        const bool embedded
            = a_member.type == TYPE_STRUCT || (a_member.type != TYPE_POINTER && a_member.subtype == TYPE_STRUCT);
        return embedded && a_member.klass && ClassHasString(*a_member.klass);
    }

    std::int64_t
    ClassSerializer::DefaultValue(const hkClass& a_class, std::size_t a_index, const hkClassMember& a_member) const {
        // layout: { int32 offsets[numDeclaredMembers]; values... }, offset -1: none, -2: zero
        const auto offset = _range.read<std::int32_t>(a_class.defaults, a_index * sizeof(std::int32_t));
        if (!offset || *offset < 0) {
            return 0;
        }

        const auto read = [&]<class T>() -> std::int64_t {
            const auto value = _range.read<T>(a_class.defaults, static_cast<std::size_t>(*offset));
            return value ? static_cast<std::int64_t>(*value) : 0;
        };
        const auto read_int = [&](std::uint8_t a_type) -> std::int64_t {
            switch (a_type) {
            case TYPE_BOOL:
            case TYPE_CHAR:
            case TYPE_INT8:
                return read.operator()<std::int8_t>();
            case TYPE_UINT8:
                return read.operator()<std::uint8_t>();
            case TYPE_INT16:
                return read.operator()<std::int16_t>();
            case TYPE_UINT16:
            case TYPE_HALF: // raw bits
                return read.operator()<std::uint16_t>();
            case TYPE_INT32:
                return read.operator()<std::int32_t>();
            case TYPE_UINT32:
                return read.operator()<std::uint32_t>();
            case TYPE_INT64:
            case TYPE_UINT64:
            case TYPE_ULONG:
                return read.operator()<std::int64_t>();
            case TYPE_REAL: // raw IEEE754 bits
                return read.operator()<std::uint32_t>();
            default:
                return 0;
            }
        };

        switch (a_member.type) {
        case TYPE_ENUM:
        case TYPE_FLAGS:
            return read_int(a_member.subtype);
        default:
            return read_int(a_member.type);
        }
    }

    std::string ClassSerializer::ToJson(const hkClass& a_class) {
        Json::Writer w;
        w.begin_object();

        w.key("name").value(a_class.GetName());
        w.key("version").value(static_cast<std::int64_t>(a_class.describedVersion));
        w.key("signature").value(std::format("0x{:08x}", Signature(a_class)));
        w.key("size_x86").value(std::int64_t{0});
        w.key("size_x86_64").value(static_cast<std::int64_t>(a_class.objectSize));
        if (a_class.parent) {
            w.key("parent").value(a_class.parent->GetName());
        }
        w.key("parent_has_string").value(a_class.parent && ClassHasString(*a_class.parent));
        w.key("vtable").value(HasVTable(a_class));
        w.key("has_string").value(ClassHasString(a_class));

        if (const auto enums = a_class.GetDeclaredEnums(); !enums.empty()) {
            w.key("enums").begin_array();
            for (const auto& e : enums) {
                std::uint8_t vtype = TYPE_ENUM;
                std::uint8_t vsubtype = TYPE_VOID;
                if (const auto it = _enumUsage.find(&e); it != _enumUsage.end()) {
                    vtype = it->second.first;
                    vsubtype = it->second.second;
                }

                w.begin_object();
                w.key("name").value(e.GetName());
                w.key("vtype").value(TypeName(vtype));
                w.key("vsubtype").value(TypeName(vsubtype));
                w.key("flags").value(std::format("{:08x}", e.flags));
                w.key("enum_item").begin_array();
                for (const auto& item : e.GetItems()) {
                    w.begin_object();
                    w.key("name").value(item.GetName());
                    w.key("value").value(static_cast<std::int64_t>(item.value));
                    w.end_object();
                }
                w.end_array();
                w.end_object();
            }
            w.end_array();
        }

        w.key("members").begin_array();
        const auto members = a_class.GetDeclaredMembers();
        for (std::size_t i = 0; i < members.size(); ++i) {
            const auto& m = members[i];
            const auto arrsize = static_cast<std::uint32_t>(m.cArraySize);

            w.begin_object();
            w.key("name").value(m.GetName());
            if (m.klass) {
                w.key("class_ref").value(m.klass->GetName());
            }
            if (m.enumClass) {
                w.key("enum_ref").value(m.enumClass->GetName());
            }
            w.key("has_string").value(MemberHasString(m));
            w.key("ctype").value(MakeCType(m));
            w.key("vtype").value(TypeName(m.type));
            w.key("vsubtype").value(TypeName(m.subtype));
            w.key("offset_x86").value(std::int64_t{0});
            w.key("offset_x86_64").value(static_cast<std::int64_t>(m.offset));
            w.key("type_size_x86").value(std::int64_t{0});
            w.key("type_size_x86_64")
                .value(static_cast<std::int64_t>(ElementSize(m)) * std::max<std::int64_t>(arrsize, 1));
            w.key("arrsize").value(static_cast<std::int64_t>(arrsize));
            w.key("flags").value(MakeFlags(m.flags));
            w.key("default").value(DefaultValue(a_class, i, m));
            w.end_object();
        }
        w.end_array();

        w.end_object();
        return w.finish();
    }
} // namespace hkdump
