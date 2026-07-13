#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
} // namespace

int main()
{
    try
    {
        drs::engine::EngineFacade engineFacade;

        const auto defaultDiagnostics = engineFacade.getDiagnosticsSnapshot();
        require(defaultDiagnostics.available, "Default diagnostics snapshot must be available.");
        require(defaultDiagnostics.loadProfileId == "balanced",
                "Default diagnostics snapshot should follow the balanced load profile.");
        require(defaultDiagnostics.configuredMaxCachedPages == 4,
                "Balanced diagnostics snapshot should expose the balanced cache budget.");
        require(defaultDiagnostics.pageMissCount >= 3,
                "Diagnostics snapshot should expose at least three streamed page misses.");
        require(defaultDiagnostics.peakActiveVoiceCount >= 3,
                "Diagnostics snapshot should expose the peak routed voice count.");
        require(defaultDiagnostics.dormantPurgeCount >= 1,
                "Diagnostics snapshot should expose an explicit dormant purge.");
        require(defaultDiagnostics.evictedPageCount >= 1,
                "Diagnostics snapshot should expose a non-zero cumulative eviction count.");
        require(defaultDiagnostics.failureState.empty(),
                "Default diagnostics snapshot should not report a failure state.");

        const auto presetRoot = fs::path(drs::engine::getPhase1RuntimeRootPath()) / "preset-state";
        const auto leadPresetPath = presetRoot / "reference" / "lead-performance-state.drpreset.json";
        const auto invalidPresetPath = presetRoot / "negative" / "transient-diagnostics-leak.drpreset.json";

        const auto restoreLead = engineFacade.restorePresetStateJson(readTextFile(leadPresetPath));
        require(restoreLead.restored, "Lead fixture should restore before diagnostics are sampled.");

        const auto leadDiagnostics = engineFacade.getDiagnosticsSnapshot();
        require(leadDiagnostics.available, "Lead diagnostics snapshot must remain available.");
        require(leadDiagnostics.loadProfileId == "performance",
                "Lead diagnostics snapshot should follow the restored performance profile.");
        require(leadDiagnostics.configuredMaxCachedPages == 8,
                "Performance diagnostics snapshot should expose the performance cache budget.");
        require(leadDiagnostics.selectedArticulationId == "lead",
                "Lead diagnostics snapshot should expose the restored articulation.");
        require(leadDiagnostics.failureState.empty(),
                "Lead diagnostics snapshot should not report a failure state.");

        const auto rejectedRestore = engineFacade.restorePresetStateJson(readTextFile(invalidPresetPath));
        require(!rejectedRestore.restored, "Invalid diagnostics-leak fixture must be rejected.");

        const auto failedDiagnostics = engineFacade.getDiagnosticsSnapshot();
        require(failedDiagnostics.available, "Diagnostics snapshot should remain available after a rejected restore.");
        require(!failedDiagnostics.failureState.empty(),
                "Diagnostics snapshot should expose the rejected restore as a failure state.");
        require(failedDiagnostics.loadProfileId == "performance",
                "Rejected restore must preserve the last known-good load profile.");
        require(failedDiagnostics.selectedArticulationId == "lead",
                "Rejected restore must preserve the last known-good articulation.");

        std::cout << "Phase 1 diagnostics tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 diagnostics tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
