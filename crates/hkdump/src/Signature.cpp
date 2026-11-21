#include "Signature.h"

#include "RE/Havok/hkClassMember.h"
#include "TypeInfo.h"

namespace hkdump {
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
        void write_enum_signature(Crc32Writer& a_w, const EnumInfo& a_enum) {
            a_w.write(a_enum.name);
            for (const auto& item : a_enum.items) {
                a_w.write(item.name);
                a_w.write32(static_cast<std::uint32_t>(item.value));
            }
            a_w.write32(static_cast<std::uint32_t>(a_enum.items.size()));
        }

        void write_class_signature(Crc32Writer& a_w, const ClassInfo& a_class, const ClassDb& a_db) {
            a_w.write32(static_cast<std::uint32_t>(a_class.numImplementedInterfaces));

            for (const auto address : a_class.declaredEnums) {
                if (const auto e = a_db.FindEnum(address)) {
                    write_enum_signature(a_w, *e);
                }
            }
            a_w.write32(static_cast<std::uint32_t>(a_class.declaredEnums.size()));

            for (const auto& m : a_class.declaredMembers) {
                std::uint16_t type = m.type;
                std::uint16_t subtype = m.subtype;
                std::uint16_t flags = m.flags;
                std::uint16_t extra = 0;

                if (m.type == TYPE_ENUM || m.type == TYPE_FLAGS) {
                    // enum storage size is folded into flags (DEPRECATED_SIZE_*); integer sizes are arch independent
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
                if (m.type != TYPE_POINTER && m.subtype != TYPE_POINTER) {
                    if (const auto klass = a_db.FindClass(m.classAddress)) {
                        for (const ClassInfo& c : a_db.Chain(*klass)) {
                            write_class_signature(a_w, c, a_db);
                        }
                    }
                }
                if (const auto e = a_db.FindEnum(m.enumAddress)) {
                    write_enum_signature(a_w, *e);
                }

                a_w.write(m.name);
                a_w.write16(type);
                a_w.write16(subtype);
                if (extra != 0) {
                    a_w.write16(extra);
                }
                a_w.write16(static_cast<std::uint16_t>(m.cArraySize));
                a_w.write16(flags);
            }
            a_w.write32(static_cast<std::uint32_t>(a_class.declaredMembers.size()));
        }
    } // namespace

    std::uint32_t Signature(const ClassInfo& a_class, const ClassDb& a_db) {
        Crc32Writer w;
        for (const ClassInfo& c : a_db.Chain(a_class)) {
            write_class_signature(w, c, a_db);
        }
        return w.get();
    }
} // namespace hkdump
