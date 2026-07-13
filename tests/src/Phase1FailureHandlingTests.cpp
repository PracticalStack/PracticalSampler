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

bool containsIssue(const std::vector<std::string>& issues, const std::string& needle)
{
    for (const auto& issue : issues)
    {
        if (issue.find(needle) != std::string::npos)
            return true;
    }

    return false;
}

void requirePerformanceLeadSession(const drs::engine::RuntimeSessionStateSnapshot& sessionState,
                                   const std::string& context)
{
    require(sessionState.loadProfileId == "performance", context + " load profile changed unexpectedly.");
    require(sessionState.selectedArticulationId == "lead", context + " articulation changed unexpectedly.");
}
} // namespace

int main()
{
    try
    {
        drs::engine::EngineFacade engineFacade;

        const auto presetRoot = fs::path(drs::engine::getPhase1RuntimeRootPath()) / "preset-state";
        const auto leadPresetPath = presetRoot / "reference" / "lead-performance-state.drpreset.json";
        const auto restoreLead = engineFacade.restorePresetStateJson(readTextFile(leadPresetPath));
        require(restoreLead.restored, "Lead fixture should restore before failure-handling probes run.");
        requirePerformanceLeadSession(engineFacade.getCurrentSessionState(), "Initial session");

        const auto missingContentProbe = engineFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::missingContent);
        require(missingContentProbe.attempted && missingContentProbe.failedGracefully,
                "Missing-content probe should fail gracefully.");
        require(missingContentProbe.state == "Reference manifest invalid",
                "Missing-content probe state changed unexpectedly.");
        require(containsIssue(missingContentProbe.issues, "Zone sample does not exist"),
                "Missing-content probe should explain the missing sample.");
        requirePerformanceLeadSession(engineFacade.getCurrentSessionState(), "After missing-content probe");

        const auto checksumProbe = engineFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::badChecksum);
        require(checksumProbe.attempted && checksumProbe.failedGracefully,
                "Bad-checksum probe should fail gracefully.");
        require(checksumProbe.state == "Stream-container invalid",
                "Bad-checksum probe state changed unexpectedly.");
        require(containsIssue(checksumProbe.issues, "checksum mismatch"),
                "Bad-checksum probe should explain the checksum mismatch.");
        requirePerformanceLeadSession(engineFacade.getCurrentSessionState(), "After bad-checksum probe");

        const auto schemaProbe = engineFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::schemaMismatch);
        require(schemaProbe.attempted && schemaProbe.failedGracefully,
                "Schema-mismatch probe should fail gracefully.");
        require(schemaProbe.state == "Reference manifest invalid",
                "Schema-mismatch probe state changed unexpectedly.");
        require(containsIssue(schemaProbe.issues, "schemaName"),
                "Schema-mismatch probe should explain the schema issue.");
        requirePerformanceLeadSession(engineFacade.getCurrentSessionState(), "After schema-mismatch probe");

        const auto partialProbe = engineFacade.probeContentFailure(
            drs::engine::EngineContentFailureCategory::partialCompiledArtifact);
        require(partialProbe.attempted && partialProbe.failedGracefully,
                "Partial-artifact probe should fail gracefully.");
        require(partialProbe.state == "Reference manifest invalid",
                "Partial-artifact probe state changed unexpectedly.");
        require(containsIssue(partialProbe.issues, "Compiled stream asset must exist"),
                "Partial-artifact probe should explain the missing compiled stream asset.");
        requirePerformanceLeadSession(engineFacade.getCurrentSessionState(), "After partial-artifact probe");

        const auto diagnosticsAfterProbe = engineFacade.getDiagnosticsSnapshot();
        require(diagnosticsAfterProbe.lastContentProbeCategory == "partial-compiled-artifact",
                "Diagnostics snapshot should remember the last probe category.");
        require(diagnosticsAfterProbe.lastContentProbeFailedGracefully,
                "Diagnostics snapshot should flag graceful failure handling.");
        require(!diagnosticsAfterProbe.failureState.empty(),
                "Diagnostics snapshot should surface the last content probe failure state.");

        engineFacade.clearContentFailureProbe();
        const auto diagnosticsAfterClear = engineFacade.getDiagnosticsSnapshot();
        require(diagnosticsAfterClear.lastContentProbeCategory.empty(),
                "Clearing the content probe should remove the last probe category.");
        require(diagnosticsAfterClear.failureState.empty(),
                "Clearing the content probe should clear the visible failure state when no state-restore error remains.");

        std::cout << "Phase 1 failure-handling tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 failure-handling tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
