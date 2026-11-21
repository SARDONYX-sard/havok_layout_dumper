#include "Dumper.h"

#include "Dump.h"
#include "MemoryRange.h"
#include "Scanner.h"

namespace Dumper {
    // Game specific part only: where to look (Fallout4.exe image / sections).
    // Everything else (detection, serialization, file output) lives in `hkdump` core and is unit tested.
    std::size_t DumpAll(const std::filesystem::path& a_outDir) {
        const auto& mod = REL::Module::get();
        const auto range = hkdump::MemoryRange::FromPeImage(mod.base());

        std::vector<std::span<const std::byte>> regions;
        for (const auto seg : {REL::Segment::rdata, REL::Segment::data}) {
            const auto segment = mod.segment(seg);
            regions.emplace_back(segment.pointer<const std::byte>(), segment.size());
        }

        const auto classes = hkdump::FindClasses(regions, range);
        logger::info("found {} hkClass candidates", classes.size());

        const auto entries = hkdump::BuildDump(classes, range);
        for (const auto& entry : entries) {
            const RE::hkClass& cls = entry.klass;
            const auto rva = range.offset_of(cls);
            if (entry.renamed) {
                logger::warn("duplicate class name {} (rva {:X}) -> {}", cls.GetName(), rva, entry.fileName);
            }
            logger::debug("{} @ rva {:X}", cls.GetName(), rva);
        }

        const auto result = hkdump::WriteDump(a_outDir, entries);
        for (const auto& error : result.errors) {
            logger::error("{}", error);
        }
        return result.written;
    }
} // namespace Dumper
