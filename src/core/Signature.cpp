#include "Signature.h"

#include "TypeInfo.h"

namespace hkdump {
    using RE::hkClass;
    using RE::hkClassEnum;
    using enum RE::hkClassMember::Type;
    using enum RE::hkClassMember::FlagValues;

    void Crc32Writer::write(std::span<const std::byte> a_data) noexcept {
        for (const auto byte : a_data) {
            _crc ^= std::to_integer<std::uint32_t>(byte);
            for (int k = 0; k < 8; ++k) {
                _crc = (_crc & 1) ? (0xEDB88320U ^ (_crc >> 1)) : (_crc >> 1);
            }
        }
    }

    namespace {
        void write_enum_signature(Crc32Writer& a_w, const hkClassEnum& a_enum) {
            a_w.write(a_enum.GetName());
            for (const auto& item : a_enum.GetItems()) {
                a_w.write(item.GetName());
                a_w.write32(static_cast<std::uint32_t>(item.value));
            }
            a_w.write32(static_cast<std::uint32_t>(a_enum.numItems));
        }

        void write_class_signature(Crc32Writer& a_w, const hkClass& a_class) {
            a_w.write32(static_cast<std::uint32_t>(a_class.numImplementedInterfaces));

            for (const auto& e : a_class.GetDeclaredEnums()) {
                write_enum_signature(a_w, e);
            }
            a_w.write32(static_cast<std::uint32_t>(a_class.numDeclaredEnums));

            for (const auto& m : a_class.GetDeclaredMembers()) {
                std::uint16_t type = m.type;
                std::uint16_t subtype = m.subtype;
                std::uint16_t flags = m.flags;
                std::uint16_t extra = 0;

                if (m.type == TYPE_ENUM || m.type == TYPE_FLAGS) {
                    // enum storage size is folded into flags (DEPRECATED_SIZE_*)
                    flags ^= static_cast<std::uint16_t>(BasicSize(m.subtype) << 3);
                    subtype = 0;
                }

                if (flags & SERIALIZE_IGNORED) {
                    flags ^= SERIALIZE_IGNORED;
                    if (subtype != 0) {
                        extra = subtype;
                    }
                    subtype = type;
                    type = TYPE_ZERO;
                }

                // embedded structs contribute their full (parent chained) layout
                if (m.klass && m.type != TYPE_POINTER && m.subtype != TYPE_POINTER) {
                    for (const hkClass& c : m.klass->GetChain()) {
                        write_class_signature(a_w, c);
                    }
                }
                if (m.enumClass) {
                    write_enum_signature(a_w, *m.enumClass);
                }

                a_w.write(m.GetName());
                a_w.write16(type);
                a_w.write16(subtype);
                if (extra != 0) {
                    a_w.write16(extra);
                }
                a_w.write16(static_cast<std::uint16_t>(m.cArraySize));
                a_w.write16(flags);
            }
            a_w.write32(static_cast<std::uint32_t>(a_class.numDeclaredMembers));
        }
    } // namespace

    std::uint32_t Signature(const hkClass& a_class) {
        Crc32Writer w;
        for (const hkClass& c : a_class.GetChain()) {
            write_class_signature(w, c);
        }
        return w.get();
    }
} // namespace hkdump
