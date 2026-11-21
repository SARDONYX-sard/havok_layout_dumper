#include "ClassSerializer.h"

#include <algorithm>
#include <format>

#include "Json.h"
#include "RE/Havok/hkClassMember.h"
#include "Signature.h"
#include "TypeInfo.h"

namespace hkdump {
    using enum RE::hkClassMember::Type;

    std::optional<JsonFormat> ParseJsonFormat(std::string_view a_name) noexcept {
        if (a_name == "serde-hkx") {
            return JsonFormat::SerdeHkx;
        }
        if (a_name == "d-merge") {
            return JsonFormat::DMerge;
        }
        return std::nullopt;
    }

    namespace {
        /// havok_class_info_gen `is_ref_kind` (`nemesis` feature).
        constexpr bool IsDMergeRefKind(std::uint8_t a_type) noexcept {
            switch (a_type) {
            case TYPE_CSTRING:
            case TYPE_STRINGPTR:
            case TYPE_VARIANT:
            case TYPE_INT8:
            case TYPE_INT16:
            case TYPE_INT32:
            case TYPE_INT64:
            case TYPE_UINT8:
            case TYPE_UINT16:
            case TYPE_UINT32:
            case TYPE_UINT64:
            case TYPE_POINTER:
                return true;
            default:
                return false;
            }
        }

        constexpr bool IsArray(std::uint8_t a_type) noexcept {
            return a_type == TYPE_ARRAY || a_type == TYPE_SIMPLEARRAY;
        }
    } // namespace

    ClassSerializer::ClassSerializer(const ClassDb& a_db, const MemoryView& a_view, JsonFormat a_format)
        : _db(a_db)
        , _view(a_view)
        , _format(a_format) {
        for (const auto& cls : _db.get().Classes()) {
            for (const auto& m : cls.declaredMembers) {
                if (m.enumAddress != 0 && (m.type == TYPE_ENUM || m.type == TYPE_FLAGS)) {
                    _enumUsage.try_emplace(m.enumAddress, m.type, m.subtype);
                }
            }
        }
    }

    bool ClassSerializer::HasVTable(const ClassInfo& a_class) const {
        return _db.get().Chain(a_class).back().get().numImplementedInterfaces > 0;
    }

    bool ClassSerializer::ClassHasString(const ClassInfo& a_class) {
        if (const auto it = _hasString.find(a_class.address); it != _hasString.end()) {
            return it->second;
        }
        _hasString[a_class.address] = false; // cycle guard

        const auto parent = _db.get().Parent(a_class);
        const bool result = (parent && ClassHasString(*parent))
            || std::ranges::any_of(a_class.declaredMembers, [this](const MemberInfo& m) { return MemberHasString(m); });
        _hasString[a_class.address] = result;
        return result;
    }

    bool ClassSerializer::MemberHasString(const MemberInfo& a_member) {
        const auto is_str = [](std::uint8_t t) { return t == TYPE_CSTRING || t == TYPE_STRINGPTR; };
        if (is_str(a_member.type) || is_str(a_member.subtype)) {
            return true;
        }
        // Embedded structs (directly or as array elements). Pointers do not count.
        const bool embedded
            = a_member.type == TYPE_STRUCT || (a_member.type != TYPE_POINTER && a_member.subtype == TYPE_STRUCT);
        const auto klass = _db.get().FindClass(a_member.classAddress);
        return embedded && klass && ClassHasString(*klass);
    }

    bool ClassSerializer::MemberHasRef(const MemberInfo& a_member) {
        const auto struct_has_ref = [&] {
            const auto klass = _db.get().FindClass(a_member.classAddress);
            return klass && ClassHasRef(*klass);
        };
        if (a_member.type == TYPE_STRUCT) {
            return struct_has_ref();
        }
        if (IsArray(a_member.type)) {
            return a_member.subtype == TYPE_STRUCT ? struct_has_ref() : IsDMergeRefKind(a_member.subtype);
        }
        if (a_member.type == TYPE_ENUM || a_member.type == TYPE_FLAGS) {
            return false;
        }
        return IsDMergeRefKind(a_member.type);
    }

    bool ClassSerializer::ClassHasRef(const ClassInfo& a_class) {
        if (const auto it = _hasRef.find(a_class.address); it != _hasRef.end()) {
            return it->second;
        }
        _hasRef[a_class.address] = false; // cycle guard

        const auto member_has_ref = [this](const MemberInfo& m) {
            if (IsDMergeRefKind(m.type) || IsDMergeRefKind(m.subtype)) {
                return true;
            }
            const bool embedded = m.type == TYPE_STRUCT || (IsArray(m.type) && m.subtype == TYPE_STRUCT);
            const auto klass = _db.get().FindClass(m.classAddress);
            return embedded && klass && ClassHasRef(*klass);
        };
        const auto chain = _db.get().Chain(a_class);
        const bool result = std::ranges::any_of(chain, [&](const ClassInfo& cls) {
            return std::ranges::any_of(cls.declaredMembers, member_has_ref);
        });
        _hasRef[a_class.address] = result;
        return result;
    }

    std::int64_t ClassSerializer::DefaultValue(const ClassInfo& a_class, std::size_t a_index) const {
        if (a_class.defaults == 0 || a_index >= a_class.declaredMembers.size()) {
            return 0;
        }
        const auto& member = a_class.declaredMembers[a_index];

        // layout: { int32 offsets[numDeclaredMembers]; values... }, offset -1: none, -2: zero
        const auto offset = _view.get().Get<std::int32_t>(a_class.defaults + (a_index * sizeof(std::int32_t)));
        if (!offset || *offset < 0) {
            return 0;
        }

        const auto address = a_class.defaults + static_cast<std::uint64_t>(*offset);
        const auto read = [&]<class T>() -> std::int64_t {
            const auto value = _view.get().Get<T>(address);
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
                return read.operator()<std::int64_t>();
            case TYPE_ULONG: // pointer sized
                return _db.get().PointerSize() == 4 ? read.operator()<std::uint32_t>()
                                                    : read.operator()<std::int64_t>();
            case TYPE_REAL: // raw IEEE754 bits
                return read.operator()<std::uint32_t>();
            default:
                return 0;
            }
        };

        switch (member.type) {
        case TYPE_ENUM:
        case TYPE_FLAGS:
            return read_int(member.subtype);
        default:
            return read_int(member.type);
        }
    }

    std::string ClassSerializer::ToJson(const ClassInfo& a_class) {
        const bool x86 = _db.get().PointerSize() == 4;
        const bool dmerge = _format == JsonFormat::DMerge;
        // Only the snapshot's architecture is known; the other layout is written as 0.
        const auto layout = [&](std::int64_t a_value) { return std::pair{x86 ? a_value : 0, x86 ? 0 : a_value}; };

        Json::Writer w;
        w.begin_object();

        w.key("name").value(a_class.name);
        w.key("version").value(static_cast<std::int64_t>(a_class.describedVersion));
        // serde-hkx style: no zero padding (e.g. "0x11e7c11")
        w.key("signature").value(std::format("0x{:x}", Signature(a_class, _db)));
        const auto [size86, size64] = layout(a_class.objectSize);
        w.key("size_x86").value(size86);
        w.key("size_x86_64").value(size64);
        const auto parent = _db.get().Parent(a_class);
        if (parent) {
            w.key("parent").value(parent->get().name);
        }
        if (dmerge) {
            // havok_class_info_gen `nemesis` feature: always true at class level
            w.key("parent_has_ref").value(true);
            w.key("vtable").value(HasVTable(a_class));
            w.key("has_ref").value(true);
        } else {
            w.key("parent_has_string").value(parent && ClassHasString(*parent));
            w.key("vtable").value(HasVTable(a_class));
            w.key("has_string").value(ClassHasString(a_class));
        }

        if (!a_class.declaredEnums.empty()) {
            w.key("enums").begin_array();
            for (const auto address : a_class.declaredEnums) {
                const auto e = _db.get().FindEnum(address);
                if (!e) {
                    continue;
                }

                std::uint8_t vtype = TYPE_ENUM;
                std::uint8_t vsubtype = TYPE_VOID;
                if (const auto it = _enumUsage.find(address); it != _enumUsage.end()) {
                    vtype = it->second.first;
                    vsubtype = it->second.second;
                }

                w.begin_object();
                w.key("name").value(e->get().name);
                w.key("vtype").value(TypeName(vtype));
                w.key("vsubtype").value(TypeName(vsubtype));
                w.key("flags").value(std::format("{:08x}", e->get().flags));
                w.key("enum_item").begin_array();
                for (const auto& item : e->get().items) {
                    w.begin_object();
                    w.key("name").value(item.name);
                    w.key("value").value(static_cast<std::int64_t>(item.value));
                    w.end_object();
                }
                w.end_array();
                w.end_object();
            }
            w.end_array();
        }

        w.key("members").begin_array();
        for (std::size_t i = 0; i < a_class.declaredMembers.size(); ++i) {
            const auto& m = a_class.declaredMembers[i];
            const auto arrsize = static_cast<std::int64_t>(m.cArraySize);

            w.begin_object();
            w.key("name").value(m.name);
            if (const auto klass = _db.get().FindClass(m.classAddress)) {
                w.key("class_ref").value(klass->get().name);
            }
            if (const auto e = _db.get().FindEnum(m.enumAddress)) {
                w.key("enum_ref").value(e->get().name);
            }
            if (dmerge) {
                w.key("has_ref").value(MemberHasRef(m));
            } else {
                w.key("has_string").value(MemberHasString(m));
            }
            w.key("ctype").value(MakeCType(m, _db));
            w.key("vtype").value(TypeName(m.type));
            w.key("vsubtype").value(TypeName(m.subtype));
            const auto [offset86, offset64] = layout(m.offset);
            w.key("offset_x86").value(offset86);
            w.key("offset_x86_64").value(offset64);
            const auto [typeSize86, typeSize64]
                = layout(static_cast<std::int64_t>(ElementSize(m, _db)) * std::max<std::int64_t>(arrsize, 1));
            w.key("type_size_x86").value(typeSize86);
            w.key("type_size_x86_64").value(typeSize64);
            w.key("arrsize").value(arrsize);
            w.key("flags").value(dmerge ? MakeDMergeFlags(m.flags) : MakeFlags(m.flags));
            w.key("default").value(DefaultValue(a_class, i));
            w.end_object();
        }
        w.end_array();

        w.end_object();
        return w.finish();
    }
} // namespace hkdump
