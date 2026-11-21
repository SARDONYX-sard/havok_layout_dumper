#include "Dump.h"

#include <format>
#include <fstream>
#include <unordered_set>

#include "ClassSerializer.h"
#include "PathUtf8.h"

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

    std::vector<DumpEntry> BuildDump(const ClassDb& a_db, const MemoryView& a_view) {
        ClassSerializer serializer{a_db, a_view};

        std::vector<DumpEntry> entries;
        entries.reserve(a_db.Classes().size());
        std::unordered_set<std::string> used;

        for (const auto& cls : a_db.Classes()) {
            DumpEntry entry{.klass = cls};

            auto stem = SanitizeFileName(cls.name);
            if (used.contains(stem)) {
                stem += std::format("_{:X}", cls.address - a_view.ImageBase());
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
            result.errors.push_back(std::format("failed to create {}: {}", ToUtf8(a_outDir), ec.message()));
            return result;
        }

        for (const auto& entry : a_entries) {
            const auto path = a_outDir / entry.fileName;
            std::ofstream file{path, std::ios::binary | std::ios::trunc};
            if (!file || !(file << entry.json)) {
                result.errors.push_back(std::format("failed to write {}", ToUtf8(path)));
                continue;
            }
            ++result.written;
        }
        return result;
    }
} // namespace hkdump
