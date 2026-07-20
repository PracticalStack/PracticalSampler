#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PerformancePublishController.h"

#include <atomic>
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

drs::engine::PerformancePublishResult eligibleResult(
    const drs::engine::PerformancePublishRequestIdentity& identity,
    std::uint64_t preparedBuildId)
{
    drs::engine::PerformancePublishResult result;
    result.identity = identity;
    result.completeProject = true;
    result.activationEligible = true;
    result.preparedBuildId = preparedBuildId;
    result.preparedContentDigest = "prepared:" + std::to_string(preparedBuildId);
    result.routeDigest = "routes:" + std::to_string(preparedBuildId);
    result.sourceProvenanceDigest = "sources:" + std::to_string(preparedBuildId);
    result.preparedMacroSchemaDigest = identity.macroSchemaDigest;
    return result;
}

drs::engine::PerformancePublishActivationPayload activationPayload(
    const drs::engine::PerformancePublishResult& result,
    std::uint64_t activationToken)
{
    drs::engine::PerformancePublishActivationPayload payload;
    payload.activationToken = activationToken;
    payload.requestIdentity = result.identity;
    payload.revision = result.identity.draftRevision;
    payload.snapshotBuildId = 5000 + result.preparedBuildId;
    payload.preparedBuildId = result.preparedBuildId;
    payload.snapshotContentDigest = result.identity.authoredContentDigest;
    payload.preparedContentDigest = result.preparedContentDigest;
    payload.routeDigest = result.routeDigest;
    payload.sourceProvenanceDigest = result.sourceProvenanceDigest;
    payload.macroSchemaDigest = result.preparedMacroSchemaDigest;
    payload.retainedPreparedBytes = 4096;
    auto macroBindings = std::make_shared<drs::engine::ImmutablePublishedMacroBindingTable>();
    macroBindings->revision = payload.revision;
    macroBindings->macroSchemaDigest = payload.macroSchemaDigest;
    macroBindings->callbackView.revision = payload.revision;
    payload.macroBindings = std::move(macroBindings);
    auto playback = std::make_shared<drs::engine::PlaybackActivationPayload>();
    playback->lane = drs::engine::PlaybackActivationLane::performance;
    playback->revision = payload.revision;
    playback->snapshotBuildId = payload.snapshotBuildId;
    playback->preparedBuildId = payload.preparedBuildId;
    playback->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::active;
    playback->activationEligible = true;
    playback->snapshotContentDigest = payload.snapshotContentDigest;
    playback->preparedContentDigest = payload.preparedContentDigest;
    playback->routeDigest = payload.routeDigest;
    playback->sourceProvenanceDigest = payload.sourceProvenanceDigest;
    playback->macroSchemaDigest = payload.macroSchemaDigest;
    playback->retainedPreparedBytes = payload.retainedPreparedBytes;
    playback->snapshot = std::make_shared<drs::engine::ImmutablePlaybackSnapshot>();
    playback->prepared = std::make_shared<drs::engine::ImmutablePreparedPlayback>();
    payload.playbackPayload = std::move(playback);
    return payload;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        PerformancePublishController controller({ 3 });
        const auto first = controller.request(4, 17, "authored:17", "macros:a", 100);
        require(first.accepted && first.request.identity.requestId != 0
                    && first.request.identity.projectGeneration == 4
                    && first.request.identity.draftRevision == 17,
                "The controller must capture one complete typed Publish request identity.");
        require(controller.getSnapshot().preparationState == PerformancePublishPreparationState::queued,
                "An accepted Publish request must enter Queued.");

        const auto duplicate = controller.request(4, 17, "authored:17", "macros:a", 110);
        require(!duplicate.accepted && duplicate.duplicateSuppressed
                    && duplicate.request.identity == first.request.identity
                    && controller.getSnapshot().requestedCount == 1,
                "An exact live Publish duplicate must collapse without creating identity or work.");

        require(controller.markPreparing(first.request.identity, 120),
                "Queued Publish work must transition to Preparing.");
        const auto second = controller.request(4, 18, "authored:18", "macros:b", 130);
        require(second.accepted && second.supersededPrevious && second.cancellationRequested
                    && second.request.identity.requestId > first.request.identity.requestId
                    && second.request.identity.cancellationGeneration
                        > first.request.identity.cancellationGeneration,
                "A different explicit Publish must supersede and invalidate older preparing work.");

        const auto stale = eligibleResult(first.request.identity, 701);
        require(!controller.acceptPrepared(stale, 150),
                "An older completion must never become Ready.");
        require(controller.markPreparing(second.request.identity, 160),
                "The newest request must launch independently of the stale completion.");
        const auto current = eligibleResult(second.request.identity, 702);
        const auto currentActivation = activationPayload(current, 9002);
        require(controller.acceptPrepared(current, 220),
                "The exact newest eligible result must become Ready.");
        auto mismatchedActivation = currentActivation;
        ++mismatchedActivation.preparedBuildId;
        require(!controller.authorizeActivation(mismatchedActivation, 225)
                    && controller.getSnapshot().activationAuthorizationRejectedCount == 1,
                "A mismatched immutable activation payload must never enter a slot.");
        require(controller.authorizeActivation(currentActivation, 230)
                    && controller.acknowledgeActivation(currentActivation, 250),
                "The exact newest eligible result must progress through Ready, Pending, and Active.");

        const auto active = controller.getSnapshot();
        require(active.hasActiveRequest
                    && active.activeRequestIdentity == second.request.identity
                    && active.activePreparedBuildId == 702
                    && active.activePreparedDigest == "prepared:702"
                    && active.activeRouteDigest == "routes:702"
                    && active.activeSourceProvenanceDigest == "sources:702"
                    && active.activeMacroSchemaDigest == "macros:b",
                "The immutable snapshot must retain the exact active identity and prepared truth.");
        require(active.lastRequestToReadyMicros == 90
                    && active.maxRequestToReadyMicros >= active.lastRequestToReadyMicros,
                "Publish diagnostics must measure the complete request-to-ready interval.");

        const auto staging = controller.request(4, 19, "authored:19", "macros:c", 270);
        require(staging.accepted && controller.markPreparing(staging.request.identity, 275),
                "Staging-rejection coverage requires a newer prepared request.");
        const auto stagingResult = eligibleResult(staging.request.identity, 703);
        const auto stagingActivation = activationPayload(stagingResult, 9003);
        require(controller.acceptPrepared(stagingResult, 280)
                    && controller.authorizeActivation(stagingActivation, 285)
                    && controller.rejectActivationStaging(
                        stagingActivation,
                        { PerformancePublishFindingSeverity::error,
                          "activation-slot-rejected", "activationSlots", "No slot was available." }),
                "A controller-authorized payload must accept a typed staging rejection.");
        const auto stagingFailed = controller.getSnapshot();
        require(stagingFailed.preparationState == PerformancePublishPreparationState::failed
                    && stagingFailed.activationState == PerformancePublishActivationState::noActivation
                    && stagingFailed.hasFailedRequest
                    && stagingFailed.failedRequestIdentity == staging.request.identity
                    && stagingFailed.hasActiveRequest
                    && stagingFailed.activeRequestIdentity == second.request.identity
                    && stagingFailed.activePreparedBuildId == 702
                    && stagingFailed.pendingActivationToken == 0
                    && stagingFailed.activationStagingRejectedCount == 1,
                "Staging rejection must preserve and never relabel exact last-known-good truth.");
        require(!controller.acknowledgeActivation(stagingActivation, 290)
                    && controller.getSnapshot().activePreparedBuildId == 702,
                "A rejected or repeated activation acknowledgement must not advance Performance.");

        const auto third = controller.request(4, 19, "authored:19", "macros:c", 300);
        require(third.accepted && controller.markPreparing(third.request.identity, 310),
                "A newer Publish must queue while last-known-good remains independent.");
        require(controller.fail(third.request.identity,
                                { PerformancePublishFindingSeverity::error,
                                  "missing-source", "sampleSources/kick", "Source is unavailable." }),
                "The current request must accept a structured failure.");
        const auto failed = controller.getSnapshot();
        require(failed.preparationState == PerformancePublishPreparationState::failed
                    && failed.hasFailedRequest
                    && failed.failedRequestIdentity == third.request.identity
                    && failed.hasActiveRequest
                    && failed.activeRequestIdentity == second.request.identity
                    && failed.activePreparedBuildId == 702,
                "Failure must remain independent from exact last-known-good identity.");

        const auto retry = controller.request(4, 19, "authored:19", "macros:c", 320);
        require(retry.accepted && retry.request.identity.requestId > third.request.identity.requestId,
                "An explicit retry of a failed captured input must create a new request.");
        require(controller.cancelCurrent()
                    && controller.getSnapshot().preparationState
                        == PerformancePublishPreparationState::canceled
                    && controller.getSnapshot().activeRequestIdentity == second.request.identity
                    && controller.getSnapshot().activePreparedBuildId == 702,
                "Cancellation must terminate requested work while preserving last-known-good.");
        require(controller.getCompletionRecords().size() <= 3
                    && controller.getSnapshot().retainedCompletionRecordCount <= 3
                    && controller.getSnapshot().maximumPendingDepth == 1,
                "Completion history and pending identity must remain bounded by configuration.");

        PerformancePublishController concurrentController({ 8 });
        std::atomic<bool> stopReader { false };
        std::atomic<bool> coherent { true };
        std::atomic<std::size_t> readCount { 0 };
        std::thread reader([&]
        {
            while (!stopReader.load(std::memory_order_acquire))
            {
                const auto snapshot = concurrentController.getSnapshot();
                if ((snapshot.hasRequest
                     && (snapshot.currentRequest.identity.requestId == 0
                         || snapshot.currentRequest.identity.authoredContentDigest.empty()
                         || snapshot.currentRequest.identity.macroSchemaDigest.empty()))
                    || (snapshot.hasActiveRequest
                        && (snapshot.activePreparedBuildId == 0
                            || snapshot.activePreparedDigest.empty()
                            || snapshot.activeRouteDigest.empty()
                            || snapshot.activeSourceProvenanceDigest.empty()
                            || snapshot.activeMacroSchemaDigest.empty()))
                    || (snapshot.hasFailedRequest && snapshot.failureFinding.code.empty())
                    || snapshot.pendingDepth > 1)
                {
                    coherent.store(false, std::memory_order_release);
                    break;
                }
                readCount.fetch_add(1, std::memory_order_relaxed);
            }
        });

        for (std::uint64_t revision = 1; revision <= 40; ++revision)
        {
            const auto request = concurrentController.request(
                9, static_cast<std::size_t>(revision),
                "authored:" + std::to_string(revision),
                "macros:" + std::to_string(revision), revision * 100);
            require(request.accepted
                        && concurrentController.markPreparing(request.request.identity,
                                                              revision * 100 + 10),
                    "Concurrent snapshot exercise must launch each explicit request.");
            const auto result = eligibleResult(request.request.identity, 1000 + revision);
            const auto activation = activationPayload(result, 10000 + revision);
            require(concurrentController.acceptPrepared(result, revision * 100 + 20)
                        && concurrentController.authorizeActivation(activation,
                                                                    revision * 100 + 30)
                        && concurrentController.acknowledgeActivation(activation,
                                                                      revision * 100 + 40),
                    "Concurrent snapshot exercise must publish coherent complete states.");
        }
        stopReader.store(true, std::memory_order_release);
        reader.join();
        require(coherent.load(std::memory_order_acquire) && readCount.load() > 0,
                "Atomic immutable Publish snapshots must remain coherent for concurrent readers.");

        concurrentController.reset(true, true);
        const auto reset = concurrentController.getSnapshot();
        require(!reset.hasRequest && !reset.hasActiveRequest
                    && reset.preparationState == PerformancePublishPreparationState::idle,
                "Project reset must clear request and active identities without reviving work.");

        std::cout << "Mini Sprint 6.2 Publish controller matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.2 Publish controller matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
