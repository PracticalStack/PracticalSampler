#include "drs/engine/PerformancePublishController.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
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
using namespace drs::engine;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

PlaybackSnapshotBuildResult buildSnapshot(PlaybackSnapshotBuilder& builder,
                                          const RuntimeProjectModel& project,
                                          std::size_t revision,
                                          bool publish)
{
    const auto request = builder.requestBuild(revision, publish);
    const auto result = builder.buildSnapshot(request, project);
    require(result.built && result.activationEligible,
            "Scheduling fixtures must produce activation-eligible immutable snapshots.");
    return result;
}

bool waitFor(const std::function<bool()>& condition,
             std::chrono::milliseconds timeout = std::chrono::milliseconds(3000))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() <= deadline)
    {
        if (condition())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return condition();
}

PerformancePublishResult eligibleResult(const PerformancePublishRequestIdentity& identity,
                                        std::uint64_t buildId)
{
    PerformancePublishResult result;
    result.identity = identity;
    result.completeProject = true;
    result.activationEligible = true;
    result.preparedBuildId = buildId;
    result.preparedContentDigest = "prepared:" + std::to_string(buildId);
    result.routeDigest = "routes:" + std::to_string(buildId);
    result.sourceProvenanceDigest = "sources:" + std::to_string(buildId);
    result.preparedMacroSchemaDigest = identity.macroSchemaDigest;
    return result;
}

RuntimeProjectModel buildCancellationProject(const RuntimeProjectModel& source)
{
    auto project = source;
    require(!source.sampleSources.empty() && !source.authoring.zones.empty(),
            "Cancellation fixture requires a reference source and zone.");
    const auto sampleTemplate = source.sampleSources.front();
    const auto zoneTemplate = source.authoring.zones.front();
    for (int index = 0; index < 512; ++index)
    {
        auto sample = sampleTemplate;
        sample.id = "cancel-source-" + std::to_string(index);
        project.sampleSources.push_back(sample);
        auto zone = zoneTemplate;
        zone.id = "cancel-zone-" + std::to_string(index);
        zone.sampleSourceId = sample.id;
        zone.keyLow = 0;
        zone.keyHigh = 127;
        project.authoring.zones.push_back(zone);
    }
    return project;
}
} // namespace

int main()
{
    try
    {
        const auto projectResult = loadPhase2ReferenceProjectManifest();
        const auto manifest = loadPhase1ReferenceInstrumentManifest();
        require(projectResult.loaded && manifest.loaded,
                "Sprint 6.4 requires the reference authored project and instrument.");
        const auto stream = loadRuntimeStreamContainerForInstrument(manifest);
        require(stream.loaded, "Sprint 6.4 requires the reference stream metadata.");

        PlaybackSnapshotBuilder builder;
        const auto preview = buildSnapshot(builder, projectResult.project, 1, false);
        const auto publish = buildSnapshot(builder, projectResult.project, 1, true);

        PreparedPlaybackService bounded("sprint6.4-bounded", 2, false);
        for (std::size_t index = 0; index < 1000; ++index)
        {
            const auto queued = index % 2 == 0
                ? bounded.enqueuePreviewBuild(preview)
                : bounded.enqueuePublishBuild(publish);
            require(queued.accepted,
                    "A 1,000-request mixed-lane burst must coalesce inside the fixed two-slot queue.");
            const auto status = bounded.getWorkerStatus();
            require(status.pendingWorkCount <= 2 && status.maxPendingWorkCount <= 2,
                    "Mixed-lane burst depth must remain constant-bounded.");
        }
        auto boundedStatus = bounded.getWorkerStatus();
        require(boundedStatus.pendingWorkCount == 2
                    && boundedStatus.supersededCount >= 998
                    && boundedStatus.configuredMaxPendingWorkCount == 2
                    && boundedStatus.configuredMaxInFlightWorkCount == 1
                    && boundedStatus.configuredMaxCompletedResultCount == 4,
                "Burst status must expose coalescing and every fixed scheduler bound.");
        const auto burstFirst = bounded.processNextQueuedBuild(stream);
        require(burstFirst.processed && burstFirst.lane == PreparedPlaybackWorkLane::performance
                    && burstFirst.result.lane == PreparedPlaybackWorkLane::performance
                    && burstFirst.result.priority == PreparedPlaybackJobPriority::performance
                    && burstFirst.result.completionDisposition
                        == PreparedPlaybackCompletionDisposition::completed,
                "The newest explicit Publish must dispatch first with typed completion truth.");

        PreparedPlaybackService fairness("sprint6.4-fairness", 2, false);
        require(fairness.enqueuePreviewBuild(preview).accepted
                    && fairness.enqueuePublishBuild(publish).accepted,
                "Fairness coverage must begin with both lanes queued.");
        for (int performanceDispatch = 0; performanceDispatch < 3; ++performanceDispatch)
        {
            const auto next = fairness.processNextQueuedBuild(stream);
            require(next.processed && next.lane == PreparedPlaybackWorkLane::performance,
                    "Publish must retain priority before the documented fairness quota.");
            if (performanceDispatch != 2)
                require(fairness.enqueuePublishBuild(publish).accepted,
                        "Fairness coverage must replenish the newest Publish candidate.");
        }
        require(fairness.enqueuePublishBuild(publish).accepted,
                "Fairness coverage requires one Publish candidate after the quota.");
        const auto fairnessDispatch = fairness.processNextQueuedBuild(stream);
        require(fairnessDispatch.processed && fairnessDispatch.lane == PreparedPlaybackWorkLane::preview,
                "Preview must dispatch after at most three consecutive Publish jobs.");
        const auto fairnessStatus = fairness.getWorkerStatus();
        require(fairnessStatus.maxConsecutivePerformanceDispatchCount == 3
                    && fairnessStatus.previewDispatchCount == 1
                    && fairnessStatus.performanceDispatchCount == 3,
                "Typed scheduler metrics must prove bounded cross-lane fairness.");

        auto mutableSnapshot = publish;
        const auto capturedDigest = mutableSnapshot.snapshot.contentDigest;
        PreparedPlaybackService capture("sprint6.4-capture", 2, false);
        require(capture.enqueuePublishBuild(mutableSnapshot).accepted,
                "Captured-input coverage must enqueue Publish work.");
        mutableSnapshot.snapshot.zones.front().gainDb += 12.0;
        mutableSnapshot.snapshot.contentDigest = "mutated-after-enqueue";
        const auto captured = capture.processNextQueuedBuild(stream);
        require(captured.result.built
                    && captured.result.prepared.snapshotContentDigest == capturedDigest,
                "Edits after enqueue must not mutate the immutable captured Publish input.");

        PreparedPlaybackSchedulerBudgets mailboxBudgets;
        mailboxBudgets.maximumCompletedResults = 3;
        PreparedPlaybackService mailbox("sprint6.4-mailbox", 2, true, mailboxBudgets);
        mailbox.setBackgroundWorkerStream(stream);
        for (int index = 0; index < 3; ++index)
        {
            require(mailbox.enqueuePublishBuild(publish).accepted,
                    "Mailbox coverage must admit each sequential Publish.");
            require(mailbox.waitForWorkerIdle(3000),
                    "Mailbox coverage must settle each background Publish.");
        }
        require(mailbox.enqueuePublishBuild(publish).accepted,
                "A full completion mailbox may retain one newest queued Publish candidate.");
        require(!mailbox.waitForWorkerIdle(20),
                "A full completion mailbox must apply backpressure instead of dropping completion identity.");
        auto mailboxStatus = mailbox.getWorkerStatus();
        require(mailboxStatus.completedResultCount == 3
                    && mailboxStatus.maxCompletedResultCount == 3
                    && mailboxStatus.pendingWorkCount == 1
                    && mailboxStatus.completionBackpressureCount == 1,
                "An unserviced completion mailbox must remain fixed while retaining bounded newest work.");
        require(mailbox.drainCompletedBuilds().size() == 3,
                "Draining must release the complete bounded mailbox without identity loss.");
        require(mailbox.waitForWorkerIdle(3000),
                "Mailbox drain must resume the bounded worker.");
        mailboxStatus = mailbox.getWorkerStatus();
        require(mailboxStatus.completedResultCount == 1 && mailboxStatus.pendingWorkCount == 0,
                "Backpressured newest work must complete after mailbox service.");
        require(mailbox.drainCompletedBuilds().size() == 1
                    && mailbox.getWorkerStatus().completedResultCount == 0,
                "The resumed completion must remain drainable with no orphaned identity.");

        const auto cancellationProject = buildCancellationProject(projectResult.project);
        const auto cancellationSnapshot = buildSnapshot(builder, cancellationProject, 2, true);
        PreparedPlaybackService cooperative("sprint6.4-cooperative", 2, true);
        cooperative.setBackgroundWorkerStream(stream);
        require(cooperative.enqueuePublishBuild(cancellationSnapshot).accepted,
                "Cooperative cancellation coverage must enqueue a large Publish.");
        require(waitFor([&]
                        {
                            return cooperative.getWorkerStatus().inFlightWorkCount == 1;
                        }),
                "Cooperative cancellation must observe the Publish in flight.");
        cooperative.cancelQueuedPublishBuilds("Explicit Sprint 6.4 in-flight cancellation");
        require(cooperative.waitForWorkerIdle(3000),
                "Cooperatively canceled work must leave the worker idle.");
        const auto cooperativeResults = cooperative.drainCompletedBuilds();
        const auto cooperativeStatus = cooperative.getWorkerStatus();
        require(cooperativeResults.size() == 1,
                "In-flight cancellation must produce exactly one terminal completion.");
        require(cooperativeResults.front().result.completionDisposition
                    == PreparedPlaybackCompletionDisposition::canceled
                    && cooperativeResults.front().result.lifecycleState
                        == PlaybackSnapshotLifecycleState::canceled,
                "In-flight cancellation must publish a typed canceled disposition and lifecycle.");
        require(cooperativeStatus.cooperativeCancellationCount == 1,
                "The worker must count one cooperatively observed cancellation.");
        require(cooperativeStatus.activeOwnershipBytes == 0,
                "Cooperative cancellation must roll back partial prepared-cache ownership.");

        PreparedPlaybackSchedulerBudgets pressureBudgets;
        pressureBudgets.maximumRetainedPreparedBytes = 1;
        pressureBudgets.maximumRequestToReadyMicros = 1;
        pressureBudgets.maximumMessageThreadServiceMicros = 1;
        PreparedPlaybackService pressure("sprint6.4-pressure", 2, false, pressureBudgets);
        require(pressure.enqueuePublishBuild(publish).accepted,
                "Pressure coverage must enqueue Publish work.");
        const auto pressureResult = pressure.processNextQueuedBuild(stream);
        pressure.recordMessageThreadServiceDuration(2);
        const auto pressureStatus = pressure.getWorkerStatus();
        require(pressureResult.result.built
                    && pressureStatus.maxObservedRetainedPreparedBytes > 1
                    && pressureStatus.retainedPreparedBytesBudgetViolationCount > 0
                    && pressureStatus.requestToReadyBudgetViolationCount > 0
                    && pressureStatus.messageThreadServiceBudgetViolationCount == 1,
                "Scheduler support budgets must be explicit, measured, and enforceable in tests.");

        PerformancePublishController controller({ 8 });
        const auto initial = controller.request(7, 1, "authored:1", "macros:1", 10);
        require(initial.accepted && controller.markPreparing(initial.request.identity, 20)
                    && controller.acceptPrepared(eligibleResult(initial.request.identity, 100), 30)
                    && controller.markActivationPending(initial.request.identity, 40)
                    && controller.markActive(initial.request.identity, 50),
                "Controller stress requires one last-known-good active Publish.");
        auto newest = initial;
        for (std::size_t index = 0; index < 1000; ++index)
        {
            newest = controller.request(7, index + 2,
                                        "authored:" + std::to_string(index + 2),
                                        "macros:" + std::to_string(index + 2),
                                        100 + index);
            require(newest.accepted && controller.getSnapshot().pendingDepth <= 1,
                    "One thousand explicit Publish identities must retain only the newest pending request.");
        }
        const auto duplicateCountBefore = controller.getSnapshot().duplicateSuppressedCount;
        for (int index = 0; index < 100; ++index)
        {
            const auto duplicate = controller.request(
                newest.request.identity.projectGeneration,
                newest.request.identity.draftRevision,
                newest.request.identity.authoredContentDigest,
                newest.request.identity.macroSchemaDigest,
                2000 + index);
            require(duplicate.duplicateSuppressed && !duplicate.accepted,
                    "Exact captured Publish duplicates must create no new identity or work.");
        }
        const auto stressed = controller.getSnapshot();
        require(stressed.hasActiveRequest
                    && stressed.activeRequestIdentity == initial.request.identity
                    && stressed.currentRequest.identity == newest.request.identity
                    && stressed.maximumPendingDepth == 1
                    && stressed.retainedCompletionRecordCount <= 8
                    && stressed.duplicateSuppressedCount == duplicateCountBefore + 100,
                "Publish churn must preserve last-known-good while bounding pending and completion identity.");
        require(controller.markPreparing(newest.request.identity, 3000)
                    && !controller.acceptPrepared(eligibleResult(initial.request.identity, 101), 3010)
                    && controller.acceptPrepared(eligibleResult(newest.request.identity, 1100), 3020),
                "Only the newest exact eligible Publish may advance after deterministic reordering.");

        std::cout << "Mini Sprint 6.4 bounded Publish scheduling matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.4 scheduling matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
