#include "Dump.h"

#include <format>
#include <fstream>
#include <unordered_set>

#include "ClassSerializer.h"

namespace hkdump {
    std::string SanitizeFileName(std::string_view a_name) {
        std::string result{a_name};
        for (auto& c : result) {
            switch (c) {
            case ':':
            case '<':
            case '>':
            case '*':
            case '?':
            case '"':
            case '/':
            case '\\':
            case '|':
                c = '_';
                break;
            default:
                break;
            }
        }
        return result;
    }

    std::vector<DumpEntry> BuildDump(std::span<const ClassRef> a_classes, const MemoryRange& a_range) {
        ClassSerializer serializer{a_classes, a_range};

        std::vector<DumpEntry> entries;
        entries.reserve(a_classes.size());
        std::unordered_set<std::string> used;

        for (const RE::hkClass& cls : a_classes) {
            DumpEntry entry{.klass = cls};

            auto stem = SanitizeFileName(cls.GetName());
            if (used.contains(stem)) {
                stem += std::format("_{:X}", a_range.offset_of(cls));
                entry.renamed = true;
            }
            used.insert(stem);

            entry.fileName = stem + ".json";
            entry.json = serializer.ToJson(cls);
            entries.push_back(std::move(entry));
        }
        return entries;
    }

    WriteResult WriteDump(const std::filesystem::path& a_outDir, std::span<const DumpEntry> a_entries) {
        WriteResult result;

        std::error_code ec;
        std::filesystem::create_directories(a_outDir, ec);
        if (ec) {
            result.errors.push_back(std::format("failed to create {}: {}", a_outDir.string(), ec.message()));
            return result;
        }

        for (const auto& entry : a_entries) {
            const auto path = a_outDir / entry.fileName;
            std::ofstream file{path, std::ios::binary | std::ios::trunc};
            if (!file || !(file << entry.json)) {
                result.errors.push_back(std::format("failed to write {}", path.string()));
                continue;
            }
            ++result.written;
        }
        return result;
    }
} // namespace hkdump
