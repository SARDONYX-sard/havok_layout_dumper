#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace hkdump {
    /// Decoded (owned, platform independent) copy of Havok reflection data.
    /// Target addresses are kept as plain integers and resolved through `ClassDb`.

    struct EnumItemInfo {
        std::string name{};
        std::int32_t value{0};
    };

    struct EnumInfo {
        std::uint64_t address{0};
        std::string name{};
        std::vector<EnumItemInfo> items{};
        std::uint32_t flags{0};
    };

    struct MemberInfo {
        std::string name{};
        std::uint64_t classAddress{0}; // 0: none
        std::uint64_t enumAddress{0};  // 0: none
        std::uint8_t type{0};          // RE::hkClassMember::Type
        std::uint8_t subtype{0};
        std::int16_t cArraySize{0};
        std::uint16_t flags{0}; // RE::hkClassMember::FlagValues
        std::uint16_t offset{0};
    };

    struct ClassInfo {
        std::uint64_t address{0};
        std::string name{};
        std::uint64_t parentAddress{0}; // 0: none
        std::int32_t objectSize{0};
        std::int32_t numImplementedInterfaces{0};
        std::vector<std::uint64_t> declaredEnums{}; // addresses, resolved by ClassDb::FindEnum
        std::vector<MemberInfo> declaredMembers{};
        std::uint64_t defaults{0}; // { int32 offsets[numDeclaredMembers]; values... }
        std::uint32_t flags{0};
        std::int32_t describedVersion{0};
    };

    template<class T>
    using OptRef = std::optional<std::reference_wrapper<const T>>;

    /// All decoded classes / enums of one target, in discovery order.
    // NOLINTNEXTLINE(bugprone-exception-escape): implicit move; MSVC's `std::unordered_map` move allocates
    class ClassDb {
    public:
        // not noexcept: MSVC's `std::unordered_map` default constructor allocates its buckets
        explicit ClassDb(std::uint32_t a_pointerSize) : _pointerSize(a_pointerSize) {}

        void AddClass(ClassInfo a_class);
        void AddEnum(EnumInfo a_enum);
        /// Keeps classes for which `a_keep` returns true (indices are rebuilt).
        void RetainClasses(const std::function<bool(const ClassInfo&)>& a_keep);

        [[nodiscard]] std::span<const ClassInfo> Classes() const noexcept { return _classes; }
        [[nodiscard]] OptRef<ClassInfo> FindClass(std::uint64_t a_address) const;
        [[nodiscard]] OptRef<EnumInfo> FindEnum(std::uint64_t a_address) const;
        [[nodiscard]] OptRef<ClassInfo> Parent(const ClassInfo& a_class) const {
            return FindClass(a_class.parentAddress);
        }

        /// `a_class`, then each parent up to the root.
        [[nodiscard]] std::vector<std::reference_wrapper<const ClassInfo>> Chain(const ClassInfo& a_class) const;

        [[nodiscard]] std::uint32_t PointerSize() const noexcept { return _pointerSize; }

    private:
        std::uint32_t _pointerSize;
        std::vector<ClassInfo> _classes;
        std::unordered_map<std::uint64_t, std::size_t> _classIndex;
        std::unordered_map<std::uint64_t, EnumInfo> _enums;
    };
} // namespace hkdump
