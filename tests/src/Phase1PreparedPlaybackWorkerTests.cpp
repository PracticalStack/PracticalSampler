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

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(drs::engine::PlaybackSnapshotBuilder& builder,
                                                       const drs::engine::RuntimeProjectModel& project,
                                                       std::size_t revision,
                                                       bool activationRequested)
{
    const auto request = builder.requestBuild(revision, activationRequested);
    require(request.accepted, "Playback snapshot request should be accepted during worker tests.");
    return builder.buildSnapshot(request, project);
}
} // namespace

int main()
{
    try
    {
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before prepared worker tests run.");

        const auto referenceManifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded, "Phase 1 reference manifest must load before prepared worker tests run.");

        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded, "Phase 1 reference stream must load before prepared worker tests run.");

        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        drs::engine::PreparedPlaybackService preparedService;
        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);

        const auto previewRevision0 = buildSnapshot(snapshotBuilder, controller.getProject(), 0, false);
        const auto queuedPreviewRevision0 = preparedService.enqueuePreviewBuild(previewRevision0);
        require(queuedPreviewRevision0.accepted, "Initial preview preparation should queue successfully.");
        require(preparedService.getWorkerStatus().pendingWorkCount == 1,
                "Worker status should expose the queued preview request.");

        auto editedProject = controller.getProject();
        editedProject.authoring.zones[0].gainDb += 1.0;
        const auto firstCommit = controller.commitSnapshot(editedProject,
                                                           "Advance draft revision for preview supersede coverage",
                                                           {"authoring.zones[0].gainDb"});
        require(firstCommit.applied, "First worker test edit should commit successfully.");

        const auto previewRevision1 = buildSnapshot(snapshotBuilder,
                                                    controller.getProject(),
                                                    firstCommit.documentState.revision,
                                                    false);
        const auto queuedPreviewRevision1 = preparedService.enqueuePreviewBuild(previewRevision1);
        require(queuedPreviewRevision1.accepted, "Superseding preview preparation should queue successfully.");
        require(queuedPreviewRevision1.displacedResults.size() == 1,
                "Superseding preview preparation should displace the older queued preview job.");
        require(queuedPreviewRevision1.displacedResults.front().lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::superseded,
                "Displaced preview preparation should report the superseded lifecycle state.");
        require(preparedService.getWorkerStatus().pendingWorkCount == 1,
                "Preview supersede should keep only one preview job queued.");
        require(preparedService.getWorkerStatus().supersededCount == 1,
                "Worker status should track superseded preparation jobs.");

        const auto publishRevision1 = buildSnapshot(snapshotBuilder,
                                                    controller.getProject(),
                                                    firstCommit.documentState.revision,
                                                    true);
        const auto queuedPublishRevision1 = preparedService.enqueuePublishBuild(publishRevision1);
        require(queuedPublishRevision1.accepted, "Publish preparation should queue successfully.");
        require(preparedService.getWorkerStatus().pendingWorkCount == 2,
                "Worker status should expose both queued preview and publish jobs.");

        const auto processedPublish = preparedService.processNextQueuedBuild(referenceStream);
        require(processedPublish.processed, "Worker should process the highest-priority queued job.");
        require(processedPublish.lane == drs::engine::PreparedPlaybackWorkLane::performance,
                "Publish preparation should run ahead of preview work.");
        require(processedPublish.result.built,
                "Processed publish preparation should succeed for the reference content.");
        require(preparedService.getWorkerStatus().pendingWorkCount == 1,
                "Processing the publish job should leave only the preview job queued.");

        const auto canceledPreview = preparedService.cancelQueuedPreviewBuilds(
            "Preview preparation canceled during worker test");
        require(canceledPreview.size() == 1,
                "Canceling preview work should cancel the remaining queued preview job.");
        require(canceledPreview.front().lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled preview work should report the canceled lifecycle state.");
        require(!preparedService.hasPendingQueuedBuilds(),
                "Canceling the last queued preview job should leave no pending worker jobs.");
        require(preparedService.getWorkerStatus().cancellationCount == 1,
                "Worker status should track canceled preparation jobs.");

        auto invalidatingProject = controller.getProject();
        invalidatingProject.sampleSources[1].path = invalidatingProject.sampleSources[0].path;
        const auto secondCommit = controller.commitSnapshot(invalidatingProject,
                                                            "Invalidate one prepared cache key",
                                                            {"sampleSources[1].path"});
        require(secondCommit.applied, "Second worker test edit should commit successfully.");

        const auto invalidatingSnapshot = buildSnapshot(snapshotBuilder,
                                                        controller.getProject(),
                                                        secondCommit.documentState.revision,
                                                        false);
        const auto queuedInvalidatingPreview = preparedService.enqueuePreviewBuild(invalidatingSnapshot);
        require(queuedInvalidatingPreview.accepted, "Invalidating preview preparation should queue successfully.");
        const auto processedInvalidatingPreview = preparedService.processNextQueuedBuild(referenceStream);
        require(processedInvalidatingPreview.processed && processedInvalidatingPreview.result.built,
                "Invalidating preview preparation should still succeed.");
        require(processedInvalidatingPreview.result.metrics.cacheHitCount == 1,
                "Invalidating one source should preserve exactly one warm prepared handle.");
        require(processedInvalidatingPreview.result.metrics.cacheMissCount == 1,
                "Invalidating one source should rebuild exactly one prepared handle.");
        require(processedInvalidatingPreview.result.metrics.activeCachedOwnershipRecordCount == 2,
                "Worker metrics should expose the active cached ownership-record count after invalidation.");
        require(processedInvalidatingPreview.result.metrics.retiredOwnershipRecordCount == 1,
                "Worker metrics should expose one retired ownership record before cleanup.");
        require(processedInvalidatingPreview.result.metrics.retiredBytesAwaitingCleanup > 0,
                "Worker metrics should expose retired ownership bytes before cleanup.");
        require(preparedService.getWorkerStatus().retiredBytesAwaitingCleanup > 0,
                "Replacing a prepared cache key should leave retired bytes awaiting cleanup.");
        require(preparedService.getWorkerStatus().activeOwnershipRecordCount == 2,
                "Worker status should expose the active ownership-record backlog.");
        require(preparedService.getWorkerStatus().retiredOwnershipRecordCount == 1,
                "Worker status should expose the retired ownership-record backlog.");
        const auto retiredOwnershipRecords = preparedService.snapshotRetiredOwnershipRecords();
        require(retiredOwnershipRecords.size() == 1,
                "Replacing one prepared cache key should expose one retired ownership record before cleanup.");
        require(retiredOwnershipRecords.front().lifetimeState == "retired-awaiting-cleanup",
                "Retired ownership records should preserve an explicit retired-awaiting-cleanup state.");
        require(!retiredOwnershipRecords.front().retirementToken.empty(),
                "Retired ownership records should carry a retirement token that survives worker completion.");
        require(retiredOwnershipRecords.front().retiredByBuildId == processedInvalidatingPreview.result.buildId,
                "Retired ownership records should track the build that superseded the stale cache entry.");
        require(preparedService.retireStaleCacheEntries() > 0,
                "Worker should retire stale prepared cache entries on request.");
        require(preparedService.getWorkerStatus().retiredOwnershipRecordCount == 0,
                "Draining stale prepared cache entries should clear the retired ownership-record count.");
        require(preparedService.getWorkerStatus().retiredBytesAwaitingCleanup == 0,
                "Retiring stale prepared cache entries should clear the retained-byte backlog.");
        require(preparedService.snapshotRetiredOwnershipRecords().empty(),
                "Draining stale prepared cache entries should clear the retired ownership backlog.");

        std::cout << "Phase 1 prepared playback worker tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 prepared playback worker tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
