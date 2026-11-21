#include "Model.h"

#include <algorithm>

namespace hkdump {
    void ClassDb::AddClass(ClassInfo a_class) {
        if (_classIndex.contains(a_class.address)) {
            return;
        }
        _classIndex.emplace(a_class.address, _classes.size());
        _classes.push_back(std::move(a_class));
    }

    void ClassDb::AddEnum(EnumInfo a_enum) {
        if (!_enums.contains(a_enum.address)) {
            const auto address = a_enum.address;
            _enums.emplace(address, std::move(a_enum));
        }
    }

    void ClassDb::RetainClasses(const std::function<bool(const ClassInfo&)>& a_keep) {
        std::erase_if(_classes, [&](const ClassInfo& c) { return !a_keep(c); });
        _classIndex.clear();
        for (std::size_t i = 0; i < _classes.size(); ++i) {
            _classIndex.emplace(_classes[i].address, i);
        }
    }

    OptRef<ClassInfo> ClassDb::FindClass(std::uint64_t a_address) const {
        if (const auto it = _classIndex.find(a_address); a_address != 0 && it != _classIndex.end()) {
            return std::cref(_classes[it->second]);
        }
        return std::nullopt;
    }

    OptRef<EnumInfo> ClassDb::FindEnum(std::uint64_t a_address) const {
        if (const auto it = _enums.find(a_address); a_address != 0 && it != _enums.end()) {
            return std::cref(it->second);
        }
        return std::nullopt;
    }

    std::vector<std::reference_wrapper<const ClassInfo>> ClassDb::Chain(const ClassInfo& a_class) const {
        std::vector<std::reference_wrapper<const ClassInfo>> chain{std::cref(a_class)};
        for (auto parent = Parent(a_class); parent; parent = Parent(parent->get())) {
            if (chain.size() > 256) { // corrupt / cyclic data guard
                break;
            }
            chain.push_back(*parent);
        }
        return chain;
    }
} // namespace hkdump
