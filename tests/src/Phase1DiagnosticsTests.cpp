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
        require(defaultDiagnostics.previewBuildId != 0,
                "Default diagnostics snapshot should expose a non-zero preview snapshot build id.");
        require(!defaultDiagnostics.previewContentDigest.empty(),
                "Default diagnostics snapshot should expose the preview snapshot digest.");
        require(defaultDiagnostics.previewPreparedBuildId != 0,
                "Default diagnostics snapshot should expose a non-zero prepared playback build id.");
        require(!defaultDiagnostics.previewPreparedContentDigest.empty(),
                "Default diagnostics snapshot should expose the prepared playback digest.");
        require(defaultDiagnostics.previewPreparedSampleCount > 0,
                "Default diagnostics snapshot should expose prepared sample counts.");
        require(defaultDiagnostics.previewPreparedOwnershipRecordCount > 0,
                "Default diagnostics snapshot should expose prepared ownership-record counts.");
        require(defaultDiagnostics.previewPreparedOwnershipBytes == defaultDiagnostics.previewPreparedBytes,
                "Default diagnostics snapshot should expose ownership-safe prepared byte totals.");
        require(defaultDiagnostics.previewPreparedBytes == defaultDiagnostics.previewPreparedSampleDataBytes,
                "Default diagnostics snapshot should expose prepared residency bytes that match retained decoded sample data.");
        require(defaultDiagnostics.previewPreparedBuildMicros > 0,
                "Default diagnostics snapshot should expose prepared build duration.");
        require(defaultDiagnostics.previewPreparedSampleDataBytes > 0,
                "Default diagnostics snapshot should expose prepared sample-data bytes.");
        require(defaultDiagnostics.previewPreparedDecodedBytes > 0,
                "Default diagnostics snapshot should expose prepared decoded bytes for the cold bootstrap build.");
        require(defaultDiagnostics.previewPreparationCacheHitRate >= 0.0
                    && defaultDiagnostics.previewPreparationCacheHitRate <= 1.0,
                "Default diagnostics snapshot should expose a normalized preview prepared cache hit rate.");
        require(defaultDiagnostics.preparedWorkerPendingCount == 0,
                "Default diagnostics snapshot should not leave prepared worker jobs pending.");
        require(defaultDiagnostics.preparedWorkerConfiguredMaxPendingCount == 2,
                "Default diagnostics snapshot should expose the configured queued-work budget.");
        require(defaultDiagnostics.preparedWorkerConfiguredMaxInFlightCount == 1,
                "Default diagnostics snapshot should expose the single-worker in-flight budget.");
        require(defaultDiagnostics.preparedWorkerActiveOwnershipRecordCount > 0,
                "Default diagnostics snapshot should expose active worker ownership-record counts.");
        require(defaultDiagnostics.preparedWorkerActiveOwnershipBytes > 0,
                "Default diagnostics snapshot should expose active worker ownership bytes.");
        require(defaultDiagnostics.preparedWorkerRetiredOwnershipRecordCount == 0,
                "Default diagnostics snapshot should not expose retired worker ownership backlog by default.");
        require(defaultDiagnostics.preparedCacheRetentionWorkingSetCount == 2,
                "Default diagnostics snapshot should expose the two-working-set prepared cache policy.");
        require(defaultDiagnostics.preparedCacheByteBudget
                    == defaultDiagnostics.preparedCacheWorkingSetBytes
                        * defaultDiagnostics.preparedCacheRetentionWorkingSetCount,
                "Default diagnostics snapshot should expose a prepared cache budget derived from the working-set target.");
        require(defaultDiagnostics.preparedCacheResidentBytes
                    == defaultDiagnostics.preparedWorkerActiveOwnershipBytes + defaultDiagnostics.preparedWorkerRetiredBytes,
                "Default diagnostics snapshot should expose prepared cache resident bytes as active plus retired ownership bytes.");
        require(defaultDiagnostics.preparedCacheResidentBytes <= defaultDiagnostics.preparedCacheByteBudget,
                "Default diagnostics snapshot should keep prepared cache residency within the configured budget.");
        require(defaultDiagnostics.preparedCachePressureState == "Nominal",
                "Default diagnostics snapshot should report nominal prepared cache pressure.");
        require(defaultDiagnostics.preparedWorkerLastCancellationLane.empty()
                    && defaultDiagnostics.preparedWorkerLastCancellationReason.empty(),
                "Default diagnostics snapshot should not expose queue-cancellation reasons before any cancellation occurs.");
        require(defaultDiagnostics.preparedWorkerLastSupersededLane.empty()
                    && defaultDiagnostics.preparedWorkerLastSupersededReason.empty(),
                "Default diagnostics snapshot should not expose queue-supersede reasons before any supersede occurs.");
        require(defaultDiagnostics.previewFindings.empty() && defaultDiagnostics.publishedFindings.empty(),
                "Default diagnostics snapshot should not surface snapshot findings for the reference project.");
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
        require(leadDiagnostics.previewBuildId != 0 && leadDiagnostics.publishedBuildId != 0,
                "Lead diagnostics snapshot should preserve snapshot build identities.");
        require(!leadDiagnostics.previewContentDigest.empty() && !leadDiagnostics.publishedContentDigest.empty(),
                "Lead diagnostics snapshot should preserve snapshot digests.");
        require(leadDiagnostics.previewPreparedBuildId != 0 && leadDiagnostics.publishedPreparedBuildId != 0,
                "Lead diagnostics snapshot should preserve prepared playback build identities.");
        require(!leadDiagnostics.previewPreparedContentDigest.empty() && !leadDiagnostics.publishedPreparedContentDigest.empty(),
                "Lead diagnostics snapshot should preserve prepared playback digests.");
        require(leadDiagnostics.previewPreparedOwnershipRecordCount > 0
                    && leadDiagnostics.publishedPreparedOwnershipRecordCount > 0,
                "Lead diagnostics snapshot should preserve prepared ownership-record counts.");
        require(leadDiagnostics.previewPreparedBuildMicros > 0 && leadDiagnostics.publishedPreparedBuildMicros > 0,
                "Lead diagnostics snapshot should preserve prepared build durations.");
        require(leadDiagnostics.previewPreparedSampleDataBytes > 0
                    && leadDiagnostics.publishedPreparedSampleDataBytes > 0,
                "Lead diagnostics snapshot should preserve prepared sample-data byte counts.");
        require(leadDiagnostics.previewPreparedBytes == leadDiagnostics.previewPreparedOwnershipBytes
                    && leadDiagnostics.previewPreparedBytes == leadDiagnostics.previewPreparedSampleDataBytes
                    && leadDiagnostics.publishedPreparedBytes == leadDiagnostics.publishedPreparedOwnershipBytes
                    && leadDiagnostics.publishedPreparedBytes == leadDiagnostics.publishedPreparedSampleDataBytes,
                "Lead diagnostics snapshot should keep prepared residency, ownership, and retained sample-data bytes aligned.");
        require(leadDiagnostics.previewPreparationCacheHitRate >= 0.0
                    && leadDiagnostics.previewPreparationCacheHitRate <= 1.0
                    && leadDiagnostics.publishedPreparationCacheHitRate >= 0.0
                    && leadDiagnostics.publishedPreparationCacheHitRate <= 1.0,
                "Lead diagnostics snapshot should preserve normalized prepared cache hit rates.");
        require(leadDiagnostics.preparedCacheRetentionWorkingSetCount == 2,
                "Lead diagnostics snapshot should preserve the two-working-set prepared cache policy.");
        require(leadDiagnostics.preparedCacheByteBudget
                    == leadDiagnostics.preparedCacheWorkingSetBytes
                        * leadDiagnostics.preparedCacheRetentionWorkingSetCount,
                "Lead diagnostics snapshot should preserve a prepared cache budget derived from the working-set target.");
        require(leadDiagnostics.preparedCacheResidentBytes
                    == leadDiagnostics.preparedWorkerActiveOwnershipBytes + leadDiagnostics.preparedWorkerRetiredBytes,
                "Lead diagnostics snapshot should preserve prepared cache resident bytes.");
        require(leadDiagnostics.preparedCacheResidentBytes <= leadDiagnostics.preparedCacheByteBudget,
                "Lead diagnostics snapshot should remain within the configured prepared cache budget.");
        require(leadDiagnostics.preparedCachePressureState == "Nominal",
                "Lead diagnostics snapshot should report nominal prepared cache pressure.");
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
