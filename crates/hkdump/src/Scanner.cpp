#include "Scanner.h"

#include <algorithm>
#include <unordered_set>

#include "RE/Havok/hkClass.h"
#include "RE/Havok/hkClassMember.h"

namespace hkdump {
    using enum RE::hkClassMember::Type;

    namespace {
        /// The class at `a_address` has a pointer to a class-like name (parent / member class refs).
        template<class Ptr>
        bool has_ident_name(const MemoryView& a_view, std::uint64_t a_address) {
            const auto name = a_view.Get<Ptr>(a_address + offsetof(RE::hkClassT<Ptr>, name));
            return name && *name != 0 && a_view.GetIdent(*name);
        }

        template<class Ptr>
        std::optional<EnumInfo> decode_enum(const MemoryView& a_view, std::uint64_t a_address) {
            const auto raw = a_view.Get<RE::hkClassEnumT<Ptr>>(a_address);
            if (!raw || raw->numItems < 0 || raw->numItems > 4096) {
                return std::nullopt;
            }
            auto name = a_view.GetIdent(raw->name);
            if (!name) {
                return std::nullopt;
            }

            EnumInfo result{.address = a_address, .name = std::move(*name), .items = {}, .flags = raw->flags};
            for (std::int32_t i = 0; i < raw->numItems; ++i) {
                const auto item = a_view.Get<RE::hkClassEnumItemT<Ptr>>(
                    std::uint64_t{raw->items} + (static_cast<std::uint64_t>(i) * sizeof(RE::hkClassEnumItemT<Ptr>))
                );
                auto itemName = item ? a_view.GetIdent(item->name) : std::nullopt;
                if (!itemName) {
                    return std::nullopt;
                }
                result.items.push_back({.name = std::move(*itemName), .value = item->value});
            }
            return result;
        }

        template<class Ptr>
        std::optional<MemberInfo> decode_member(
            const MemoryView& a_view, std::uint64_t a_address, std::int32_t a_objectSize, std::vector<EnumInfo>& a_enums
        ) {
            const auto raw = a_view.Get<RE::hkClassMemberT<Ptr>>(a_address);
            if (!raw || raw->type >= TYPE_MAX || raw->subtype >= TYPE_MAX || raw->cArraySize < 0
                || raw->offset > a_objectSize) {
                return std::nullopt;
            }
            // NOTE: Member names may start with a digit. (e.g. `hkpLimitedHingeConstraintDataAtoms::2dAng`)
            //       Rejecting it drops the class and every class referring to it (e.g. `hkpLimitedHingeConstraintData`).
            auto name = a_view.GetIdent(raw->name, 256, true);
            if (!name) {
                return std::nullopt;
            }
            if (raw->klass != 0 && !has_ident_name<Ptr>(a_view, raw->klass)) {
                return std::nullopt;
            }
            if (raw->enumClass != 0) {
                auto decoded = decode_enum<Ptr>(a_view, raw->enumClass);
                if (!decoded) {
                    return std::nullopt;
                }
                a_enums.push_back(std::move(*decoded));
            }

            return MemberInfo{
                .name = std::move(*name),
                .classAddress = raw->klass,
                .enumAddress = raw->enumClass,
                .type = raw->type,
                .subtype = raw->subtype,
                .cArraySize = raw->cArraySize,
                .flags = raw->flags,
                .offset = raw->offset,
            };
        }

        template<class Ptr>
        std::optional<DecodedClass> decode_class(const MemoryView& a_view, std::uint64_t a_address) {
            const auto raw = a_view.Get<RE::hkClassT<Ptr>>(a_address);
            // objectSize > 0 also rejects NiRTTI ({name, parent} only, followed by zeros)
            if (!raw || raw->objectSize <= 0 || raw->objectSize > (1 << 20) || raw->numImplementedInterfaces < 0
                || raw->numImplementedInterfaces > 64 || raw->numDeclaredEnums < 0 || raw->numDeclaredEnums > 256
                || raw->numDeclaredMembers < 0 || raw->numDeclaredMembers > 1024 || raw->describedVersion < 0
                || raw->describedVersion > 1000 || raw->flags > 0xFFFF) {
                return std::nullopt;
            }

            auto name = a_view.GetIdent(raw->name);
            if (!name) {
                return std::nullopt;
            }
            if (raw->parent != 0 && !has_ident_name<Ptr>(a_view, raw->parent)) {
                return std::nullopt;
            }
            if (raw->defaults != 0 && !a_view.Get<std::int32_t>(raw->defaults)) {
                return std::nullopt;
            }
            if (raw->attributes != 0 && !a_view.Get<std::uint8_t>(raw->attributes)) {
                return std::nullopt;
            }

            DecodedClass result{
                .klass =
                    {
                        .address = a_address,
                        .name = std::move(*name),
                        .parentAddress = raw->parent,
                        .objectSize = raw->objectSize,
                        .numImplementedInterfaces = raw->numImplementedInterfaces,
                        .declaredEnums = {},
                        .declaredMembers = {},
                        .defaults = raw->defaults,
                        .flags = raw->flags,
                        .describedVersion = raw->describedVersion,
                    },
                .enums = {},
            };

            for (std::int32_t i = 0; i < raw->numDeclaredEnums; ++i) {
                const auto address = std::uint64_t{raw->declaredEnums}
                    + (static_cast<std::uint64_t>(i) * sizeof(RE::hkClassEnumT<Ptr>));
                auto decoded = decode_enum<Ptr>(a_view, address);
                if (!decoded) {
                    return std::nullopt;
                }
                result.klass.declaredEnums.push_back(address);
                result.enums.push_back(std::move(*decoded));
            }

            for (std::int32_t i = 0; i < raw->numDeclaredMembers; ++i) {
                const auto address = std::uint64_t{raw->declaredMembers}
                    + (static_cast<std::uint64_t>(i) * sizeof(RE::hkClassMemberT<Ptr>));
                auto member = decode_member<Ptr>(a_view, address, raw->objectSize, result.enums);
                if (!member) {
                    return std::nullopt;
                }
                result.klass.declaredMembers.push_back(std::move(*member));
            }
            return result;
        }

        /// Cheap pre-filter: random (name, 0, 0, ...) tuples are skipped before full decoding.
        /// Empty classes (e.g. hkBaseObject) are still found later as parents.
        template<class Ptr>
        bool has_content(const MemoryView& a_view, std::uint64_t a_address) {
            const auto raw = a_view.Get<RE::hkClassT<Ptr>>(a_address);
            return raw && (raw->numDeclaredMembers > 0 || raw->numDeclaredEnums > 0 || raw->parent != 0);
        }

        template<class Ptr>
        std::vector<std::uint64_t> scan_roots(const MemoryView& a_view) {
            constexpr auto align = std::uint64_t{sizeof(Ptr)};
            std::vector<std::uint64_t> roots;
            for (const auto region : a_view.ScanRegions()) {
                const auto begin = (region.begin + align - 1) & ~(align - 1);
                for (auto address = begin; address + sizeof(RE::hkClassT<Ptr>) <= region.end; address += align) {
                    if (has_content<Ptr>(a_view, address) && decode_class<Ptr>(a_view, address)) {
                        roots.push_back(address);
                    }
                }
            }
            return roots;
        }
    } // namespace

    std::optional<EnumInfo> DecodeEnum(const MemoryView& a_view, std::uint64_t a_address) {
        return a_view.PointerSize() == 4 ? decode_enum<std::uint32_t>(a_view, a_address)
                                         : decode_enum<std::uint64_t>(a_view, a_address);
    }

    std::optional<DecodedClass> DecodeClass(const MemoryView& a_view, std::uint64_t a_address) {
        return a_view.PointerSize() == 4 ? decode_class<std::uint32_t>(a_view, a_address)
                                         : decode_class<std::uint64_t>(a_view, a_address);
    }

    ClassDb LoadClasses(const MemoryView& a_view, std::span<const std::uint64_t> a_roots) {
        ClassDb db{a_view.PointerSize()};
        std::unordered_set<std::uint64_t> seen;
        std::vector<std::uint64_t> work;

        const auto push = [&](std::uint64_t a_address) {
            if (a_address == 0 || !seen.insert(a_address).second) {
                return;
            }
            auto decoded = DecodeClass(a_view, a_address);
            if (!decoded) {
                return;
            }
            for (auto& e : decoded->enums) {
                db.AddEnum(std::move(e));
            }
            work.push_back(a_address);
            db.AddClass(std::move(decoded->klass));
        };

        for (const auto root : a_roots) {
            push(root);
        }
        while (!work.empty()) {
            const auto address = work.back();
            work.pop_back();
            const auto cls = db.FindClass(address);
            if (!cls) {
                continue;
            }
            // copy the references first: `push` may grow the class vector
            std::vector<std::uint64_t> refs{cls->get().parentAddress};
            for (const auto& m : cls->get().declaredMembers) {
                refs.push_back(m.classAddress);
            }
            for (const auto ref : refs) {
                push(ref);
            }
        }

        // Only keep classes whose whole graph (parent chain, member classes) was accepted, so
        // serialization never walks into something that merely looked like a class name.
        for (std::size_t size = 0; size != db.Classes().size();) {
            size = db.Classes().size();
            const auto resolvable
                = [&](std::uint64_t a_address) { return a_address == 0 || db.FindClass(a_address).has_value(); };
            std::unordered_set<std::uint64_t> keep;
            for (const auto& c : db.Classes()) {
                if (resolvable(c.parentAddress) && std::ranges::all_of(c.declaredMembers, [&](const auto& m) {
                        return resolvable(m.classAddress);
                    })) {
                    keep.insert(c.address);
                }
            }
            db.RetainClasses([&](const ClassInfo& c) { return keep.contains(c.address); });
        }
        return db;
    }

    ClassDb ScanClasses(const MemoryView& a_view) {
        const auto roots
            = a_view.PointerSize() == 4 ? scan_roots<std::uint32_t>(a_view) : scan_roots<std::uint64_t>(a_view);
        return LoadClasses(a_view, roots);
    }
} // namespace hkdump
