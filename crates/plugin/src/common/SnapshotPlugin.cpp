#include "SnapshotPlugin.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

#include <Windows.h>

#include "PathUtf8.h"
#include "Snapshot.h"

namespace plugin {
    namespace {
        // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables): set once in Load
        const Config* g_config = nullptr;
        // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

        std::filesystem::path ExePath() {
            std::wstring buffer(MAX_PATH, L'\0');
            for (;;) {
                const auto len = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
                if (len < buffer.size()) {
                    buffer.resize(len);
                    return buffer;
                }
                buffer.resize(buffer.size() * 2);
            }
        }

        /// `<game>/Data/hkdump_snapshots`
        std::filesystem::path OutputDirectory() { return ExePath().parent_path() / L"Data" / L"hkdump_snapshots"; }

        void Log(std::string_view a_pluginName, std::string_view a_message) {
            const auto dir = OutputDirectory();
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            std::ofstream file{dir / (std::string{a_pluginName} + ".log"), std::ios::app};
            file << std::format("[{:%Y-%m-%d %H:%M:%S}] {}\n", std::chrono::system_clock::now(), a_message);
        }

        void __cdecl OnMessage(xse::Message* a_msg) {
            if (g_config != nullptr && a_msg != nullptr && a_msg->type == g_config->readyMessage) {
                TakeSnapshot(g_config->name);
            }
        }
    } // namespace

    void TakeSnapshot(std::string_view a_pluginName) {
        const auto exe = ExePath();
        const auto path = OutputDirectory() / (exe.stem().wstring() + L".hksnap");
        const auto base = reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));

        if (const auto error = hkdump::WriteModuleSnapshot(base, path)) {
            Log(a_pluginName, "snapshot failed: " + *error);
            return;
        }
        Log(a_pluginName,
            std::format(
                "wrote {} (x{}) - decode with `hkdump <snapshot> <out dir>`",
                hkdump::ToUtf8(path),
                sizeof(void*) == 4 ? "86" : "64"
            ));
    }

    bool Load(const Config& a_config, const xse::Interface* a_xse) {
        g_config = &a_config;
        const auto handle = a_xse->GetPluginHandle();
        const auto* const messaging
            = static_cast<const xse::MessagingInterface*>(a_xse->QueryInterface(a_config.messagingInterfaceId));

        if (messaging == nullptr || !messaging->RegisterListener(handle, a_config.sender, OnMessage)) {
            Log(a_config.name, "messaging interface unavailable: taking snapshot now (classes may be incomplete)");
            TakeSnapshot(a_config.name);
        }
        return true;
    }
} // namespace plugin
