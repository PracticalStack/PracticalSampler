#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

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

std::vector<std::string> collectPreparedCacheKeys(const drs::engine::ImmutablePreparedPlayback& prepared)
{
    std::vector<std::string> cacheKeys;
    cacheKeys.reserve(prepared.samples.size());

    for (const auto& sample : prepared.samples)
        cacheKeys.push_back(sample.cacheKey);

    return cacheKeys;
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

bool waitForCondition(const std::function<bool()>& condition,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(500))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() <= deadline)
    {
        if (condition())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return condition();
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
        require(preparedService.getWorkerStatus().configuredMaxPendingWorkCount == 2,
                "Worker status should expose the configured queued-work budget.");
        require(preparedService.getWorkerStatus().configuredMaxInFlightWorkCount == 1,
                "Worker status should expose the single-worker in-flight budget.");

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
        require(preparedService.getWorkerStatus().lastSupersededLane == "preview",
                "Worker status should surface the superseded lane for same-lane preview replacement.");
        require(preparedService.getWorkerStatus().lastSupersededReason
                    == "Prepared playback build superseded by a newer preview request",
                "Worker status should surface the supersede reason for same-lane preview replacement.");

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
        require(preparedService.getWorkerStatus().lastCancellationLane == "preview",
                "Worker status should surface the canceled lane for queued preview cancellation.");
        require(preparedService.getWorkerStatus().lastCancellationReason
                    == "Preview preparation canceled during worker test",
                "Worker status should surface the cancellation reason for queued preview cancellation.");

        drs::engine::PreparedPlaybackService queuedCancellationCleanupService(
            "phase1-prepared-playback-v2",
            2,
            true);
        require(queuedCancellationCleanupService.isBackgroundWorkerEnabled(),
                "Cancellation cleanup coverage should exercise the background-worker queue.");
        require(queuedCancellationCleanupService.enqueuePreviewBuild(previewRevision0).accepted,
                "Cancellation cleanup coverage should queue preview work before cancellation.");
        require(queuedCancellationCleanupService.enqueuePublishBuild(publishRevision1).accepted,
                "Cancellation cleanup coverage should queue publish work before cancellation.");
        const auto canceledQueuedPublish = queuedCancellationCleanupService.cancelQueuedPublishBuilds(
            "Publish preparation canceled during cleanup coverage");
        require(canceledQueuedPublish.size() == 1,
                "Canceling queued publish work should return the displaced publish build.");
        require(canceledQueuedPublish.front().lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled queued publish work should report the canceled lifecycle state.");
        const auto canceledQueuedPreview = queuedCancellationCleanupService.cancelQueuedPreviewBuilds(
            "Preview preparation canceled during cleanup coverage");
        require(canceledQueuedPreview.size() == 1,
                "Canceling queued preview work should return the displaced preview build.");
        require(canceledQueuedPreview.front().lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled queued preview work should report the canceled lifecycle state.");
        require(queuedCancellationCleanupService.waitForWorkerIdle(100),
                "Canceling every queued background-worker job should leave the worker idle immediately.");
        require(!queuedCancellationCleanupService.hasPendingQueuedBuilds(),
                "Canceling every queued background-worker job should leave no pending jobs behind.");
        require(queuedCancellationCleanupService.drainCompletedBuilds().empty(),
                "Canceling queued background-worker jobs should not leave orphaned completed results behind.");
        require(queuedCancellationCleanupService.getWorkerStatus().cancellationCount == 2,
                "Cancellation cleanup coverage should record both queued lane cancellations.");
        require(queuedCancellationCleanupService.getWorkerStatus().activeOwnershipRecordCount == 0,
                "Canceling queued background-worker jobs should not materialize active ownership records.");
        require(queuedCancellationCleanupService.getWorkerStatus().activeOwnershipBytes == 0,
                "Canceling queued background-worker jobs should not materialize active ownership bytes.");
        require(queuedCancellationCleanupService.getWorkerStatus().retiredOwnershipRecordCount == 0,
                "Canceling queued background-worker jobs should not create a retired ownership backlog.");
        require(queuedCancellationCleanupService.getWorkerStatus().retiredBytesAwaitingCleanup == 0,
                "Canceling queued background-worker jobs should not create retired ownership bytes.");

        drs::engine::PreparedPlaybackService publishAdmissionPriorityService("phase1-prepared-playback-v2", 1, false);
        const auto maxBudgetPreview = buildSnapshot(snapshotBuilder, controller.getProject(), 0, false);
        const auto queuedMaxBudgetPreview = publishAdmissionPriorityService.enqueuePreviewBuild(maxBudgetPreview);
        require(queuedMaxBudgetPreview.accepted,
                "Preview preparation should queue successfully before publish-priority admission coverage.");
        require(publishAdmissionPriorityService.getWorkerStatus().pendingWorkCount == 1,
                "Publish-priority admission coverage should begin with the queue budget occupied by preview work.");

        const auto maxBudgetPublish = buildSnapshot(snapshotBuilder, controller.getProject(), 0, true);
        const auto queuedMaxBudgetPublish = publishAdmissionPriorityService.enqueuePublishBuild(maxBudgetPublish);
        require(queuedMaxBudgetPublish.accepted,
                "Publish preparation should still be admitted when preview already occupies the full queue budget.");
        require(queuedMaxBudgetPublish.displacedResults.size() == 1,
                "Higher-priority publish admission should displace one queued preview build when the queue budget is full.");
        require(queuedMaxBudgetPublish.displacedResults.front().lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::superseded,
                "Preview work displaced by publish admission should report the superseded lifecycle state.");
        require(queuedMaxBudgetPublish.displacedResults.front().state
                    == "Prepared playback build superseded by higher-priority publish request",
                "Publish-priority admission should expose a lane-aware supersede reason.");
        require(publishAdmissionPriorityService.getWorkerStatus().lastSupersededLane == "preview",
                "Publish-priority admission should preserve which queued lane was displaced.");
        require(publishAdmissionPriorityService.getWorkerStatus().lastSupersededReason
                    == "Prepared playback build superseded by higher-priority publish request",
                "Publish-priority admission should surface the higher-priority supersede reason in worker status.");
        require(publishAdmissionPriorityService.getWorkerStatus().pendingWorkCount == 1,
                "Publish-priority admission should keep the queue bounded to the configured worker budget.");
        const auto processedMaxBudgetPublish = publishAdmissionPriorityService.processNextQueuedBuild(referenceStream);
        require(processedMaxBudgetPublish.processed,
                "Publish-priority admission coverage should process the admitted publish build.");
        require(processedMaxBudgetPublish.lane == drs::engine::PreparedPlaybackWorkLane::performance,
                "When the queue budget is full, the surviving admitted work should be the publish lane.");
        require(processedMaxBudgetPublish.result.built,
                "The admitted publish build should still prepare successfully.");
        require(!publishAdmissionPriorityService.hasPendingQueuedBuilds(),
                "Processing the admitted publish build should leave no displaced preview work behind.");

        drs::engine::PreparedPlaybackService previewAdmissionPriorityService("phase1-prepared-playback-v2", 1, false);
        const auto queuedBudgetPublish = previewAdmissionPriorityService.enqueuePublishBuild(maxBudgetPublish);
        require(queuedBudgetPublish.accepted,
                "Publish preparation should queue successfully before lower-priority preview admission coverage.");
        const auto queuedBudgetPreview = previewAdmissionPriorityService.enqueuePreviewBuild(maxBudgetPreview);
        require(!queuedBudgetPreview.accepted,
                "Lower-priority preview work should not displace queued publish work when the queue budget is full.");
        require(queuedBudgetPreview.request.state == "Prepared playback queue is full",
                "Rejected preview admission should continue surfacing the queue-full state.");
        require(previewAdmissionPriorityService.getWorkerStatus().pendingWorkCount == 1,
                "Rejected preview admission should preserve the queued publish budget occupant.");

        drs::engine::PreparedPlaybackService boundedBackgroundWorkerService("phase1-prepared-playback-v2", 1, true);
        require(boundedBackgroundWorkerService.isBackgroundWorkerEnabled(),
                "Burst-bound coverage should use the real background worker path.");
        const auto burstPreviewRevision0 = buildSnapshot(snapshotBuilder, controller.getProject(), 0, false);
        const auto queuedBurstPreviewRevision0 = boundedBackgroundWorkerService.enqueuePreviewBuild(burstPreviewRevision0);
        require(queuedBurstPreviewRevision0.accepted,
                "Burst-bound coverage should admit the initial preview request.");
        const auto burstPreviewRevision1 = buildSnapshot(snapshotBuilder,
                                                         controller.getProject(),
                                                         firstCommit.documentState.revision,
                                                         false);
        const auto queuedBurstPreviewRevision1 = boundedBackgroundWorkerService.enqueuePreviewBuild(burstPreviewRevision1);
        require(queuedBurstPreviewRevision1.accepted,
                "Burst-bound coverage should supersede older preview work while holding the queue bound.");
        const auto burstPublishRevision = buildSnapshot(snapshotBuilder,
                                                        controller.getProject(),
                                                        firstCommit.documentState.revision,
                                                        true);
        const auto queuedBurstPublish = boundedBackgroundWorkerService.enqueuePublishBuild(burstPublishRevision);
        require(queuedBurstPublish.accepted,
                "Burst-bound coverage should still admit publish work within the bounded queue budget.");
        const auto rejectedBurstPreview = boundedBackgroundWorkerService.enqueuePreviewBuild(burstPreviewRevision0);
        require(!rejectedBurstPreview.accepted,
                "Burst-bound coverage should reject lower-priority overflow once the bounded queue budget is saturated.");
        auto boundedStatus = boundedBackgroundWorkerService.getWorkerStatus();
        require(boundedStatus.pendingWorkCount <= boundedStatus.configuredMaxPendingWorkCount,
                "Queued worker backlog must never exceed the configured queue budget before the background worker starts.");
        require(boundedStatus.inFlightWorkCount <= boundedStatus.configuredMaxInFlightWorkCount,
                "In-flight worker activity must never exceed the configured concurrency budget before the background worker starts.");
        require(boundedStatus.configuredMaxPendingWorkCount == 1,
                "Burst-bound coverage should surface the configured one-slot queue budget.");
        require(boundedStatus.configuredMaxInFlightWorkCount == 1,
                "Burst-bound coverage should surface the configured single-worker concurrency budget.");
        boundedBackgroundWorkerService.setBackgroundWorkerStream(referenceStream);
        require(waitForCondition(
                    [&]
                    {
                        const auto status = boundedBackgroundWorkerService.getWorkerStatus();
                        return status.inFlightWorkCount == 1 || status.completedWorkCount > 0;
                    }),
                "Burst-bound coverage should observe the background worker begin bounded processing after the stream becomes available.");
        require(boundedBackgroundWorkerService.waitForWorkerIdle(1500),
                "Burst-bound coverage should settle through the background worker.");
        boundedStatus = boundedBackgroundWorkerService.getWorkerStatus();
        require(boundedStatus.pendingWorkCount == 0,
                "Burst-bound coverage should leave no queued work after the bounded worker drains.");
        require(boundedStatus.inFlightWorkCount == 0,
                "Burst-bound coverage should leave no in-flight work after the bounded worker drains.");
        require(boundedStatus.maxPendingWorkCount <= boundedStatus.configuredMaxPendingWorkCount,
                "Observed queued backlog must never exceed the configured queue bound during burst processing.");
        require(boundedStatus.completedWorkCount == 1,
                "Burst-bound coverage should process only the single surviving bounded work item.");
        const auto boundedResults = boundedBackgroundWorkerService.drainCompletedBuilds();
        require(boundedResults.size() == 1,
                "Burst-bound coverage should leave exactly one completed background-worker result to drain.");
        require(boundedResults.front().lane == drs::engine::PreparedPlaybackWorkLane::performance,
                "Burst-bound coverage should preserve publish as the surviving bounded work item.");
        require(boundedResults.front().result.built,
                "The surviving bounded background-worker item should still prepare successfully.");
        require(boundedBackgroundWorkerService.drainCompletedBuilds().empty(),
                "Burst-bound coverage should not leave orphaned completed results after the first drain.");

        drs::engine::PreparedPlaybackService mixedChurnBackgroundWorkerService("phase1-prepared-playback-v2", 2, true);
        auto mixedChurnProject = controller.getProject();
        mixedChurnProject.authoring.zones[0].pan = 0.2;
        const auto mixedChurnCommit1 = controller.commitSnapshot(mixedChurnProject,
                                                                 "Advance draft revision for mixed worker churn coverage",
                                                                 {"authoring.zones[0].pan"});
        require(mixedChurnCommit1.applied,
                "Mixed worker churn coverage should commit the first churn revision successfully.");
        mixedChurnProject = controller.getProject();
        mixedChurnProject.authoring.zones[0].gainDb += 0.75;
        const auto mixedChurnCommit2 = controller.commitSnapshot(mixedChurnProject,
                                                                 "Advance draft revision for mixed worker churn coverage again",
                                                                 {"authoring.zones[0].gainDb"});
        require(mixedChurnCommit2.applied,
                "Mixed worker churn coverage should commit the second churn revision successfully.");

        const auto mixedPreviewRevision0 = buildSnapshot(snapshotBuilder, phase2Project.project, 0, false);
        const auto mixedPublishRevision0 = buildSnapshot(snapshotBuilder, phase2Project.project, 0, true);
        const auto mixedPreviewRevision1 = buildSnapshot(snapshotBuilder,
                                                         controller.getProject(),
                                                         mixedChurnCommit1.documentState.revision,
                                                         false);
        const auto mixedPublishRevision1 = buildSnapshot(snapshotBuilder,
                                                         controller.getProject(),
                                                         mixedChurnCommit1.documentState.revision,
                                                         true);
        const auto mixedPreviewRevision2 = buildSnapshot(snapshotBuilder,
                                                         controller.getProject(),
                                                         mixedChurnCommit2.documentState.revision,
                                                         false);
        const auto mixedPublishRevision2 = buildSnapshot(snapshotBuilder,
                                                         controller.getProject(),
                                                         mixedChurnCommit2.documentState.revision,
                                                         true);

        require(mixedChurnBackgroundWorkerService.enqueuePreviewBuild(mixedPreviewRevision0).accepted,
                "Mixed worker churn coverage should queue the initial preview request.");
        require(mixedChurnBackgroundWorkerService.enqueuePublishBuild(mixedPublishRevision0).accepted,
                "Mixed worker churn coverage should queue the initial publish request.");
        const auto supersededMixedPreview1 = mixedChurnBackgroundWorkerService.enqueuePreviewBuild(mixedPreviewRevision1);
        require(supersededMixedPreview1.accepted && supersededMixedPreview1.displacedResults.size() == 1,
                "Mixed worker churn coverage should supersede the older queued preview request.");
        const auto supersededMixedPublish1 = mixedChurnBackgroundWorkerService.enqueuePublishBuild(mixedPublishRevision1);
        require(supersededMixedPublish1.accepted && supersededMixedPublish1.displacedResults.size() == 1,
                "Mixed worker churn coverage should supersede the older queued publish request.");
        const auto supersededMixedPreview2 = mixedChurnBackgroundWorkerService.enqueuePreviewBuild(mixedPreviewRevision2);
        require(supersededMixedPreview2.accepted && supersededMixedPreview2.displacedResults.size() == 1,
                "Mixed worker churn coverage should supersede the intermediate queued preview request.");
        const auto supersededMixedPublish2 = mixedChurnBackgroundWorkerService.enqueuePublishBuild(mixedPublishRevision2);
        require(supersededMixedPublish2.accepted && supersededMixedPublish2.displacedResults.size() == 1,
                "Mixed worker churn coverage should supersede the intermediate queued publish request.");
        auto mixedChurnStatus = mixedChurnBackgroundWorkerService.getWorkerStatus();
        require(mixedChurnStatus.pendingWorkCount == 2,
                "Mixed worker churn coverage should leave only one preview and one publish request queued before worker start.");
        require(mixedChurnStatus.supersededCount >= 4,
                "Mixed worker churn coverage should record every queued supersede across both lanes.");

        mixedChurnBackgroundWorkerService.setBackgroundWorkerStream(referenceStream);
        require(mixedChurnBackgroundWorkerService.waitForWorkerIdle(1500),
                "Mixed worker churn coverage should settle through the background worker.");
        mixedChurnStatus = mixedChurnBackgroundWorkerService.getWorkerStatus();
        require(mixedChurnStatus.pendingWorkCount == 0 && mixedChurnStatus.inFlightWorkCount == 0,
                "Mixed worker churn coverage should leave no pending or in-flight work after settling.");
        const auto mixedChurnResults = mixedChurnBackgroundWorkerService.drainCompletedBuilds();
        require(mixedChurnResults.size() == 2,
                "Mixed worker churn coverage should complete only the latest preview and publish requests.");
        require(mixedChurnBackgroundWorkerService.drainCompletedBuilds().empty(),
                "Mixed worker churn coverage should not leave orphaned completed results after the first drain.");
        require(std::count_if(mixedChurnResults.begin(),
                              mixedChurnResults.end(),
                              [](const drs::engine::PreparedPlaybackWorkerStepResult& result)
                              {
                                  return result.lane == drs::engine::PreparedPlaybackWorkLane::preview;
                              }) == 1,
                "Mixed worker churn coverage should complete exactly one preview result.");
        require(std::count_if(mixedChurnResults.begin(),
                              mixedChurnResults.end(),
                              [](const drs::engine::PreparedPlaybackWorkerStepResult& result)
                              {
                                  return result.lane == drs::engine::PreparedPlaybackWorkLane::performance;
                              }) == 1,
                "Mixed worker churn coverage should complete exactly one publish result.");
        require(std::all_of(mixedChurnResults.begin(),
                            mixedChurnResults.end(),
                            [&](const drs::engine::PreparedPlaybackWorkerStepResult& result)
                            {
                                return result.result.requestedDraftRevision == mixedChurnCommit2.documentState.revision
                                    && result.result.built;
                            }),
                "Mixed worker churn coverage should complete only the latest draft revision after queued supersedes.");

        auto zoneOnlyEditedProject = controller.getProject();
        zoneOnlyEditedProject.authoring.zones[0].rootKey += 2;
        zoneOnlyEditedProject.authoring.zones[0].keyLow += 1;
        zoneOnlyEditedProject.authoring.zones[0].keyHigh -= 1;
        zoneOnlyEditedProject.authoring.zones[0].velocityLow += 3;
        zoneOnlyEditedProject.authoring.zones[0].velocityHigh -= 4;
        zoneOnlyEditedProject.authoring.zones[0].gainDb += 0.5;
        zoneOnlyEditedProject.authoring.zones[0].pan = -0.15;
        zoneOnlyEditedProject.authoring.zones[0].sampleStartFrame += 24;
        zoneOnlyEditedProject.authoring.zones[0].loopEnabled = !zoneOnlyEditedProject.authoring.zones[0].loopEnabled;
        const auto zoneOnlyCommit = controller.commitSnapshot(zoneOnlyEditedProject,
                                                              "Adjust worker test zone-only mapping values",
                                                              {"authoring.zones[0].rootKey",
                                                               "authoring.zones[0].keyLow",
                                                               "authoring.zones[0].keyHigh",
                                                               "authoring.zones[0].velocityLow",
                                                               "authoring.zones[0].velocityHigh",
                                                               "authoring.zones[0].gainDb",
                                                               "authoring.zones[0].pan",
                                                               "authoring.zones[0].sampleStartFrame",
                                                               "authoring.zones[0].loopEnabled"});
        require(zoneOnlyCommit.applied, "Zone-only worker test edit should commit successfully.");
        const auto zoneOnlySnapshot = buildSnapshot(snapshotBuilder,
                                                    controller.getProject(),
                                                    zoneOnlyCommit.documentState.revision,
                                                    false);
        const auto queuedZoneOnlyPreview = preparedService.enqueuePreviewBuild(zoneOnlySnapshot);
        require(queuedZoneOnlyPreview.accepted, "Zone-only preview preparation should queue successfully.");
        const auto processedZoneOnlyPreview = preparedService.processNextQueuedBuild(referenceStream);
        require(processedZoneOnlyPreview.processed && processedZoneOnlyPreview.result.built,
                "Zone-only preview preparation should still succeed.");
        require(processedZoneOnlyPreview.result.metrics.cacheHitCount == phase2Project.project.sampleSources.size(),
                "Zone-only worker edits should reuse every prepared sample handle.");
        require(processedZoneOnlyPreview.result.metrics.cacheMissCount == 0,
                "Zone-only worker edits should not invalidate prepared sample handles.");
        require(processedZoneOnlyPreview.result.metrics.decodedBytes == 0,
                "Zone-only worker edits should not re-decode warm prepared sample handles.");
        require(processedZoneOnlyPreview.result.metrics.preparedSampleDataBytes
                    == processedPublish.result.metrics.preparedSampleDataBytes,
                "Zone-only worker edits should preserve deterministic prepared sample-data bytes.");
        require(collectPreparedCacheKeys(processedZoneOnlyPreview.result.prepared)
                    == collectPreparedCacheKeys(processedPublish.result.prepared),
                "Zone-only worker edits should preserve prepared cache-key identity.");
        require(processedZoneOnlyPreview.result.prepared.samples == processedPublish.result.prepared.samples,
                "Zone-only worker edits should preserve prepared sample handles.");
        require(processedZoneOnlyPreview.result.prepared.streams == processedPublish.result.prepared.streams,
                "Zone-only worker edits should preserve prepared stream handles.");
        require(processedZoneOnlyPreview.result.prepared.ownershipRecords
                    == processedPublish.result.prepared.ownershipRecords,
                "Zone-only worker edits should preserve prepared ownership records.");
        require(processedZoneOnlyPreview.result.prepared.zones != processedPublish.result.prepared.zones,
                "Zone-only worker edits should still update prepared zone content.");
        require(processedZoneOnlyPreview.result.prepared.zones[0].rootKey
                    == zoneOnlyEditedProject.authoring.zones[0].rootKey
                    && processedZoneOnlyPreview.result.prepared.zones[0].keyLow
                        == zoneOnlyEditedProject.authoring.zones[0].keyLow
                    && processedZoneOnlyPreview.result.prepared.zones[0].keyHigh
                        == zoneOnlyEditedProject.authoring.zones[0].keyHigh
                    && processedZoneOnlyPreview.result.prepared.zones[0].velocityLow
                        == zoneOnlyEditedProject.authoring.zones[0].velocityLow
                    && processedZoneOnlyPreview.result.prepared.zones[0].velocityHigh
                        == zoneOnlyEditedProject.authoring.zones[0].velocityHigh
                    && processedZoneOnlyPreview.result.prepared.zones[0].gainDb
                        == zoneOnlyEditedProject.authoring.zones[0].gainDb
                    && processedZoneOnlyPreview.result.prepared.zones[0].pan
                        == zoneOnlyEditedProject.authoring.zones[0].pan
                    && processedZoneOnlyPreview.result.prepared.zones[0].sampleStartFrame
                        == zoneOnlyEditedProject.authoring.zones[0].sampleStartFrame
                    && processedZoneOnlyPreview.result.prepared.zones[0].loopEnabled
                        == zoneOnlyEditedProject.authoring.zones[0].loopEnabled,
                "Zone-only worker edits should flow updated mapping and mix values into prepared zones.");

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
        require(preparedService.getWorkerStatus().activeOwnershipBytes
                    == processedInvalidatingPreview.result.metrics.preparedOwnershipBytes,
                "Worker status should expose active ownership bytes for the surviving prepared cache set.");
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
        const auto originalInvalidatedSourcePath = phase2Project.project.sampleSources[1].path;
        auto restoredInvalidatedProject = controller.getProject();
        restoredInvalidatedProject.sampleSources[1].path = originalInvalidatedSourcePath;
        const auto thirdCommit = controller.commitSnapshot(restoredInvalidatedProject,
                                                           "Restore the invalidated prepared cache key",
                                                           {"sampleSources[1].path"});
        require(thirdCommit.applied, "Restoring the invalidated worker cache key should commit successfully.");

        const auto restoredInvalidatingSnapshot = buildSnapshot(snapshotBuilder,
                                                                controller.getProject(),
                                                                thirdCommit.documentState.revision,
                                                                false);
        const auto queuedRestoredInvalidatingPreview = preparedService.enqueuePreviewBuild(restoredInvalidatingSnapshot);
        require(queuedRestoredInvalidatingPreview.accepted,
                "Restoring the invalidated worker cache key should queue successfully.");
        const auto processedRestoredInvalidatingPreview = preparedService.processNextQueuedBuild(referenceStream);
        require(processedRestoredInvalidatingPreview.processed && processedRestoredInvalidatingPreview.result.built,
                "Restoring the invalidated worker cache key should still prepare successfully.");
        require(processedRestoredInvalidatingPreview.result.metrics.cacheHitCount == 1,
                "Restoring one invalidated source before cleanup should preserve the one still-active warm handle.");
        require(processedRestoredInvalidatingPreview.result.metrics.cacheMissCount == 1,
                "Restoring one invalidated source before cleanup should rebuild the stale retired handle.");
        require(processedRestoredInvalidatingPreview.result.metrics.retiredOwnershipRecordCount == 2,
                "Repeated invalidating edits before cleanup should accumulate two retired ownership records.");
        require(processedRestoredInvalidatingPreview.result.metrics.retiredBytesAwaitingCleanup
                    > processedInvalidatingPreview.result.metrics.retiredBytesAwaitingCleanup,
                "Repeated invalidating edits before cleanup should grow the retained-byte backlog.");
        const auto accumulatedRetiredOwnershipRecords = preparedService.snapshotRetiredOwnershipRecords();
        require(accumulatedRetiredOwnershipRecords.size() == 2,
                "Repeated invalidating edits before cleanup should expose both retired ownership records.");
        require(preparedService.serviceRetiredCacheCleanup(1) == 1,
                "Partial stale-cache cleanup should retire only one accumulated record at a time.");
        require(preparedService.getWorkerStatus().retiredOwnershipRecordCount == 1,
                "Partial stale-cache cleanup should leave one retired ownership record behind.");
        const auto retiredBytesAfterPartialCleanup = preparedService.getWorkerStatus().retiredBytesAwaitingCleanup;
        require(retiredBytesAfterPartialCleanup > 0,
                "Partial stale-cache cleanup should leave retained bytes awaiting the remaining cleanup pass.");
        require(preparedService.serviceRetiredCacheCleanup() == 1,
                "A second stale-cache cleanup pass should drain the final accumulated retirement record.");
        require(preparedService.getWorkerStatus().retiredOwnershipRecordCount == 0,
                "Draining stale prepared cache entries should clear the retired ownership-record count.");
        require(preparedService.getWorkerStatus().retiredBytesAwaitingCleanup == 0,
                "Retiring stale prepared cache entries should clear the retained-byte backlog.");
        require(preparedService.getWorkerStatus().activeOwnershipBytes
                    == processedRestoredInvalidatingPreview.result.metrics.preparedOwnershipBytes,
                "Retiring stale prepared cache entries should not disturb the surviving active ownership bytes.");
        require(preparedService.snapshotRetiredOwnershipRecords().empty(),
                "Draining stale prepared cache entries should clear the retired ownership backlog.");

        auto postCleanupInvalidatingProject = controller.getProject();
        postCleanupInvalidatingProject.sampleSources[1].path = postCleanupInvalidatingProject.sampleSources[0].path;
        const auto fourthCommit = controller.commitSnapshot(postCleanupInvalidatingProject,
                                                            "Rebuild a cleaned retired prepared cache key",
                                                            {"sampleSources[1].path"});
        require(fourthCommit.applied,
                "Post-cleanup invalidation coverage should commit successfully.");

        const auto postCleanupInvalidatingSnapshot = buildSnapshot(snapshotBuilder,
                                                                   controller.getProject(),
                                                                   fourthCommit.documentState.revision,
                                                                   false);
        const auto queuedPostCleanupInvalidatingPreview
            = preparedService.enqueuePreviewBuild(postCleanupInvalidatingSnapshot);
        require(queuedPostCleanupInvalidatingPreview.accepted,
                "Post-cleanup invalidation coverage should queue successfully.");
        const auto processedPostCleanupInvalidatingPreview = preparedService.processNextQueuedBuild(referenceStream);
        require(processedPostCleanupInvalidatingPreview.processed
                    && processedPostCleanupInvalidatingPreview.result.built,
                "Post-cleanup invalidation coverage should still prepare successfully.");
        require(processedPostCleanupInvalidatingPreview.result.metrics.cacheHitCount == 1,
                "After cleanup, only the still-active prepared handle should remain warm.");
        require(processedPostCleanupInvalidatingPreview.result.metrics.cacheMissCount == 1,
                "After cleanup, rebuilding a once-retired cache key should cold-miss instead of reusing retired state.");
        require(processedPostCleanupInvalidatingPreview.result.metrics.retiredOwnershipRecordCount == 1,
                "Post-cleanup invalidation coverage should begin a fresh retirement backlog for the newly replaced key.");
        require(preparedService.serviceRetiredCacheCleanup() == 1,
                "Post-cleanup invalidation coverage should drain the new retirement backlog cleanly.");
        require(preparedService.getWorkerStatus().retiredOwnershipRecordCount == 0,
                "Post-cleanup invalidation coverage should leave no retired backlog after servicing cleanup.");

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
