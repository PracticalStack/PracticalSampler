#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before prepared playback tests run.");

        const auto referenceManifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded, "Phase 1 reference manifest must load before prepared playback tests run.");

        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded, "Phase 1 reference stream must load before prepared playback tests run.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        drs::engine::PreparedPlaybackService preparedService;

        const auto firstSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        require(firstSnapshotRequest.accepted, "Initial playback snapshot request should be accepted.");
        const auto firstSnapshot = snapshotBuilder.buildSnapshot(firstSnapshotRequest, phase2Project.project);
        require(firstSnapshot.built, "Initial playback snapshot should build successfully.");

        const auto firstPreparedRequest = preparedService.requestBuild(firstSnapshot);
        require(firstPreparedRequest.accepted, "Prepared playback request should be accepted for a valid snapshot.");
        const auto firstPrepared = preparedService.prepare(firstPreparedRequest, firstSnapshot, referenceStream);
        require(firstPrepared.built, "Prepared playback should build from the reference snapshot.");
        require(firstPrepared.activationEligible, "Prepared playback should remain activation-eligible for valid content.");
        require(firstPrepared.metrics.preparedSampleCount == phase2Project.project.sampleSources.size(),
                "Prepared sample count changed unexpectedly.");
        require(firstPrepared.metrics.preparedZoneCount == phase2Project.project.authoring.zones.size(),
                "Prepared zone count changed unexpectedly.");
        require(firstPrepared.metrics.cacheMissCount == phase2Project.project.sampleSources.size(),
                "First prepared playback build should cold-miss every sample handle.");
        require(firstPrepared.metrics.cacheHitCount == 0,
                "First prepared playback build should not report cache hits.");
        require(!firstPrepared.prepared.preparedContentDigest.empty(),
                "Prepared playback builds must carry a deterministic content digest.");
        require(firstPrepared.prepared.samples[0].ownershipToken.find("cache:") == 0,
                "Prepared sample handles should expose an explicit ownership token.");

        const auto secondSnapshotRequest = snapshotBuilder.requestBuild(0, true);
        const auto secondSnapshot = snapshotBuilder.buildSnapshot(secondSnapshotRequest, phase2Project.project);
        const auto secondPreparedRequest = preparedService.requestBuild(secondSnapshot);
        const auto secondPrepared = preparedService.prepare(secondPreparedRequest, secondSnapshot, referenceStream);
        require(secondPrepared.built, "Repeated prepared playback build should still succeed.");
        require(secondPrepared.prepared.preparedContentDigest == firstPrepared.prepared.preparedContentDigest,
                "Repeated preparation of the same snapshot should produce the same prepared digest.");
        require(secondPrepared.metrics.cacheHitCount == phase2Project.project.sampleSources.size(),
                "Warm prepared playback build should hit the cache for every sample handle.");
        require(secondPrepared.metrics.cacheMissCount == 0,
                "Warm prepared playback build should not cold-miss unchanged sample handles.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        auto editedProject = controller.getProject();
        editedProject.sampleSources[1].path = editedProject.sampleSources[0].path;
        const auto commitResult = controller.commitSnapshot(editedProject,
                                                            "Swap the lead source path to invalidate one prepared key",
                                                            {"sampleSources[1].path"});
        require(commitResult.applied, "Edited project revision should commit before prepared playback rebuild.");

        const auto editedSnapshotRequest = snapshotBuilder.requestBuild(commitResult.documentState.revision, true);
        const auto editedSnapshot = snapshotBuilder.buildSnapshot(editedSnapshotRequest, controller.getProject());
        require(editedSnapshot.built, "Edited snapshot should still build successfully.");
        const auto editedPreparedRequest = preparedService.requestBuild(editedSnapshot);
        const auto editedPrepared = preparedService.prepare(editedPreparedRequest, editedSnapshot, referenceStream);
        require(editedPrepared.built, "Edited prepared playback should still succeed.");
        require(editedPrepared.prepared.preparedContentDigest != firstPrepared.prepared.preparedContentDigest,
                "Changing a sample source path should invalidate the prepared digest.");
        require(editedPrepared.metrics.cacheHitCount == 1,
                "Changing one source path should preserve exactly one cached prepared asset.");
        require(editedPrepared.metrics.cacheMissCount == 1,
                "Changing one source path should invalidate exactly one cached prepared asset.");

        auto invalidProject = phase2Project.project;
        invalidProject.sampleSources[0].path = invalidProject.contentRootPath + "/Samples/does-not-exist.wav";
        const auto invalidSnapshotRequest = snapshotBuilder.requestBuild(3, true);
        const auto invalidSnapshot = snapshotBuilder.buildSnapshot(invalidSnapshotRequest, invalidProject);
        require(!invalidSnapshot.built, "Invalid snapshot should fail before prepared playback begins.");
        const auto rejectedPreparedRequest = preparedService.requestBuild(invalidSnapshot);
        require(!rejectedPreparedRequest.accepted,
                "Prepared playback request must reject failed immutable snapshots.");
        const auto rejectedPrepared = preparedService.prepare(rejectedPreparedRequest, invalidSnapshot, referenceStream);
        require(!rejectedPrepared.built && !rejectedPrepared.activationEligible,
                "Rejected prepared playback result must not become activation-eligible.");

        std::cout << "Phase 1 prepared playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 prepared playback tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
