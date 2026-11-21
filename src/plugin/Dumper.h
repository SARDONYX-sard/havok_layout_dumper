#pragma once

namespace Dumper {
    /// Scans the executable image for static hkClass reflection tables and writes
    /// one serde-hkx compatible `<ClassName>.json` per class into `a_outDir`.
    ///
    /// Returns the number of written classes.
    std::size_t DumpAll(const std::filesystem::path& a_outDir);
}
