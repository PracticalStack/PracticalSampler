#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
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

std::uint64_t computePreparedSampleDataBytes(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    std::uint64_t sampleDataBytes = 0;

    for (const auto& sample : prepared.samples)
    {
        sampleDataBytes += static_cast<std::uint64_t>(sample.channelCount)
            * sample.frameCount
            * static_cast<std::uint64_t>(sizeof(float));
    }

    return sampleDataBytes;
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

        drs::engine::PreparedPlaybackService previewDecodeService;
        const auto coldPreviewRevision0 = buildSnapshot(snapshotBuilder, controller.getProject(), 0, false);
        const auto queuedColdPreviewRevision0 = previewDecodeService.enqueuePreviewBuild(coldPreviewRevision0);
        require(queuedColdPreviewRevision0.accepted, "Cold preview preparation should queue successfully.");
        const auto processedColdPreview = previewDecodeService.processNextQueuedBuild(referenceStream);
        require(processedColdPreview.processed, "Cold preview preparation should process through the worker.");
        require(processedColdPreview.lane == drs::engine::PreparedPlaybackWorkLane::preview,
                "Cold preview preparation should stay on the preview lane.");
        require(processedColdPreview.result.built,
                "Cold preview preparation should succeed for the reference content.");
        require(processedColdPreview.result.metrics.preparedSampleDataBytes
                    == computePreparedSampleDataBytes(processedColdPreview.result.prepared),
                "Cold preview preparation should expose deterministic prepared sample-data bytes.");
        require(processedColdPreview.result.metrics.decodedBytes > 0,
                "Cold preview preparation should decode source samples through the worker-owned preparation seam.");
        const auto queuedWarmPreviewRevision0 = previewDecodeService.enqueuePreviewBuild(coldPreviewRevision0);
        require(queuedWarmPreviewRevision0.accepted, "Warm preview preparation should queue successfully.");
        const auto processedWarmPreview = previewDecodeService.processNextQueuedBuild(referenceStream);
        require(processedWarmPreview.processed && processedWarmPreview.result.built,
                "Warm preview preparation should still succeed for the same reference content.");
        require(processedWarmPreview.result.prepared.preparedContentDigest
                    == processedColdPreview.result.prepared.preparedContentDigest,
                "Warm preview preparation should preserve the prepared digest for unchanged content.");
        require(processedWarmPreview.result.metrics.preparedSampleDataBytes
                    == processedColdPreview.result.metrics.preparedSampleDataBytes,
                "Warm preview preparation should preserve deterministic prepared sample-data bytes.");
        require(processedWarmPreview.result.metrics.decodedBytes == 0,
                "Warm preview preparation should not re-decode unchanged sample handles.");

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
        require(processedPublish.result.metrics.preparedSampleDataBytes
                    == computePreparedSampleDataBytes(processedPublish.result.prepared),
                "Cold publish preparation should expose deterministic prepared sample-data bytes.");
        require(processedPublish.result.metrics.preparedSampleDataBytes
                    == processedColdPreview.result.metrics.preparedSampleDataBytes,
                "Equivalent cold preview and publish source content should report the same prepared sample-data bytes.");
        require(processedPublish.result.metrics.decodedBytes > 0,
                "Cold publish preparation should decode source samples through the worker-owned preparation seam.");
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
        require(processedInvalidatingPreview.result.metrics.preparedSampleDataBytes
                    == computePreparedSampleDataBytes(processedInvalidatingPreview.result.prepared),
                "Invalidating preview preparation should preserve deterministic prepared sample-data bytes.");
        require(processedInvalidatingPreview.result.metrics.decodedBytes > 0,
                "Invalidating one source should re-decode the worker-owned cold-miss handle.");
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

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migrated worker coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before worker coverage runs.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "migrated-worker-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "migrated-worker-zone-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Migrated Worker Zone A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto migratedImport = migratedSession.appendImportedContent({ importedSampleSource },
                                                                          { importedZone },
                                                                          "Import migrated worker zone");
        require(migratedImport.applied, "Migrated worker coverage should accept imported authoring content.");
        require(migratedImport.documentState.revision == 1,
                "Imported migrated worker content should advance the draft revision.");

        drs::engine::PlaybackSnapshotBuilder migratedSnapshotBuilder;
        drs::engine::PreparedPlaybackService migratedPreparedWorker;
        const auto migratedPreviewSnapshot = buildSnapshot(migratedSnapshotBuilder,
                                                           migratedSession.getProject(),
                                                           migratedImport.documentState.revision,
                                                           false);
        require(migratedPreviewSnapshot.built,
                "Imported migrated worker content should build a valid preview snapshot.");
        const auto queuedMigratedPreview = migratedPreparedWorker.enqueuePreviewBuild(migratedPreviewSnapshot);
        require(queuedMigratedPreview.accepted,
                "Imported migrated worker preview should queue successfully.");
        const auto processedMigratedPreview = migratedPreparedWorker.processNextQueuedBuild(referenceStream);
        require(processedMigratedPreview.processed, "Imported migrated worker preview should process.");
        require(processedMigratedPreview.lane == drs::engine::PreparedPlaybackWorkLane::preview,
                "Imported migrated worker preview should stay on the preview lane.");
        require(processedMigratedPreview.result.built && processedMigratedPreview.result.activationEligible,
                "Imported migrated worker preview should prepare successfully.");
        require(processedMigratedPreview.result.metrics.preparedSampleCount
                    == migratedSession.getProject().sampleSources.size(),
                "Imported migrated worker preview should materialize every migrated sample identity.");
        require(processedMigratedPreview.result.metrics.preparedZoneCount
                    == migratedSession.getProject().authoring.zones.size(),
                "Imported migrated worker preview should materialize every migrated playable zone.");
        require(processedMigratedPreview.result.metrics.cacheMissCount
                    == migratedSession.getProject().sampleSources.size(),
                "Imported migrated worker preview should cold-miss every prepared sample handle on first build.");
        require(processedMigratedPreview.result.metrics.cacheHitCount == 0,
                "Imported migrated worker preview should not report cache hits on the first build.");
        require(processedMigratedPreview.result.metrics.preparedSampleDataBytes
                    == computePreparedSampleDataBytes(processedMigratedPreview.result.prepared),
                "Imported migrated worker preview should expose deterministic prepared sample-data bytes.");
        require(processedMigratedPreview.result.metrics.decodedBytes
                    == processedMigratedPreview.result.metrics.preparedSampleDataBytes,
                "Imported migrated worker preview should decode the full prepared sample-data footprint.");
        require(processedMigratedPreview.result.prepared.zones.size() == 1,
                "Imported migrated worker preview should expose one imported playable zone.");
        require(processedMigratedPreview.result.prepared.zones.front().zoneId == importedZone.id,
                "Imported migrated worker preview should preserve the imported zone identity.");

        const auto migratedPublishSnapshot = buildSnapshot(migratedSnapshotBuilder,
                                                           migratedSession.getProject(),
                                                           migratedImport.documentState.revision,
                                                           true);
        require(migratedPublishSnapshot.built,
                "Imported migrated worker content should build a valid publish snapshot.");
        const auto queuedMigratedPublish = migratedPreparedWorker.enqueuePublishBuild(migratedPublishSnapshot);
        require(queuedMigratedPublish.accepted,
                "Imported migrated worker publish should queue successfully.");
        const auto processedMigratedPublish = migratedPreparedWorker.processNextQueuedBuild(referenceStream);
        require(processedMigratedPublish.processed, "Imported migrated worker publish should process.");
        require(processedMigratedPublish.lane == drs::engine::PreparedPlaybackWorkLane::performance,
                "Imported migrated worker publish should stay on the publish lane.");
        require(processedMigratedPublish.result.built && processedMigratedPublish.result.activationEligible,
                "Imported migrated worker publish should prepare successfully.");
        require(processedMigratedPublish.result.prepared.snapshotContentDigest
                    == processedMigratedPreview.result.prepared.snapshotContentDigest,
                "Imported migrated worker preview and publish should share the same immutable snapshot digest.");
        require(processedMigratedPublish.result.prepared.preparedContentDigest
                    == processedMigratedPreview.result.prepared.preparedContentDigest,
                "Imported migrated worker preview and publish should share the same prepared digest.");
        require(processedMigratedPublish.result.metrics.cacheHitCount
                    == migratedSession.getProject().sampleSources.size(),
                "Imported migrated worker publish should reuse every prepared sample handle.");
        require(processedMigratedPublish.result.metrics.cacheMissCount == 0,
                "Imported migrated worker publish should not cold-miss after preview warmed the cache.");
        require(processedMigratedPublish.result.metrics.preparedSampleDataBytes
                    == processedMigratedPreview.result.metrics.preparedSampleDataBytes,
                "Imported migrated worker publish should preserve deterministic prepared sample-data bytes.");
        require(processedMigratedPublish.result.metrics.decodedBytes == 0,
                "Imported migrated worker publish should not re-decode warm prepared sample handles.");
        require(processedMigratedPublish.result.prepared.zones.size() == 1,
                "Imported migrated worker publish should preserve the imported playable zone.");
        require(processedMigratedPublish.result.prepared.zones.front().zoneId == importedZone.id,
                "Imported migrated worker publish should preserve the imported zone identity.");

        std::cout << "Phase 1 prepared playback worker tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 prepared playback worker tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
