#include "Dumper.h"

namespace {
    void dump() {
        const auto logDir = logger::log_directory();
        if (!logDir) {
            logger::error("failed to resolve F4SE log directory");
            return;
        }

        const auto outDir = *logDir / "HavokClasses";
        logger::info("dumping Havok classes to {}", outDir.string());

        const auto count = Dumper::DumpAll(outDir);
        logger::info("wrote {} class json files", count);
    }

    void on_message(F4SE::MessagingInterface::Message* a_msg) {
        // Reflection tables are static data, but wait until the game finished
        // initializing so any lazily-unpacked image sections are in place.
        if (a_msg && a_msg->type == F4SE::MessagingInterface::kPostLoad) {
            dump();
        }
    }
}

F4SE_PLUGIN_LOAD(const F4SE::LoadInterface* a_f4se) {
    F4SE::Init(a_f4se);

    const auto* const messaging = F4SE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(on_message)) {
        logger::warn("messaging interface unavailable, dumping immediately");
        dump();
    }

    return true;
}
