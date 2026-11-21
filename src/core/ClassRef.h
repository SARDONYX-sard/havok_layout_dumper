#pragma once

#include <functional>

#include "RE/Havok/hkClass.h"

namespace hkdump {
    /// Non-owning, non-null reference to a (static) `hkClass` table.
    using ClassRef = std::reference_wrapper<const RE::hkClass>;
} // namespace hkdump
