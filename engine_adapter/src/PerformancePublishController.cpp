#include "drs/engine/PerformancePublishController.h"
#include "drs/engine/DraftPlaybackContract.h"

#include <algorithm>
#include <atomic>
#include <utility>

namespace drs::engine
{
namespace
{
void updateMaximum(std::uint64_t& maximum, std::uint64_t value) noexcept
{
    maximum = std::max(maximum, value);
}

bool sameCapturedInput(const PerformancePublishRequestIdentity& identity,
                       std::uint64_t projectGeneration,
                       std::size_t draftRevision,
                       const std::string& authoredContentDigest,
                       const std::string& macroSchemaDigest) noexcept
{
    return identity.projectGeneration == projectGeneration
        && identity.draftRevision == draftRevision
        && identity.authoredContentDigest == authoredContentDigest
        && identity.macroSchemaDigest == macroSchemaDigest;
}

bool matchesAcceptedActivation(const PerformancePublishControllerSnapshot& snapshot,
                               const PerformancePublishActivationPayload& payload) noexcept
{
    const auto& playback = payload.playbackPayload;
    return snapshot.hasRequest
        && payload.activationToken != 0
        && playback != nullptr
        && playback->lane == PlaybackActivationLane::performance
        && playback->activationEligible
        && playback->lifecycleState == PlaybackSnapshotLifecycleState::active
        && playback->snapshot != nullptr
        && playback->prepared != nullptr
        && payload.requestIdentity == snapshot.currentRequest.identity
        && payload.revision == snapshot.currentRequest.identity.draftRevision
        && payload.snapshotBuildId != 0
        && payload.preparedBuildId == snapshot.acceptedPreparedBuildId
        && payload.preparedBuildId != 0
        && payload.snapshotContentDigest == snapshot.currentRequest.identity.authoredContentDigest
        && payload.preparedContentDigest == snapshot.acceptedPreparedDigest
        && payload.routeDigest == snapshot.acceptedRouteDigest
        && payload.sourceProvenanceDigest == snapshot.acceptedSourceProvenanceDigest
        && payload.macroSchemaDigest == snapshot.acceptedMacroSchemaDigest
        && payload.macroSchemaDigest == snapshot.currentRequest.identity.macroSchemaDigest
        && payload.macroBindings != nullptr
        && payload.macroBindings->revision == payload.revision
        && payload.macroBindings->macroSchemaDigest == payload.macroSchemaDigest
        && playback->revision == payload.revision
        && playback->snapshotBuildId == payload.snapshotBuildId
        && playback->preparedBuildId == payload.preparedBuildId
        && playback->snapshotContentDigest == payload.snapshotContentDigest
        && playback->preparedContentDigest == payload.preparedContentDigest
        && playback->routeDigest == payload.routeDigest
        && playback->sourceProvenanceDigest == payload.sourceProvenanceDigest
        && playback->macroSchemaDigest == payload.macroSchemaDigest
        && playback->retainedPreparedBytes == payload.retainedPreparedBytes;
}
} // namespace

PerformancePublishController::PerformancePublishController(
    PerformancePublishControllerConfig nextConfig)
    : config(nextConfig)
{
    config.maximumCompletionRecords = std::max<std::size_t>(1, config.maximumCompletionRecords);
    publishSnapshot();
}

PerformancePublishRequestResult PerformancePublishController::request(
    std::uint64_t projectGeneration,
    std::size_t draftRevision,
    std::string authoredContentDigest,
    std::string macroSchemaDigest,
    std::uint64_t nowMicros,
    PerformancePublishRequestOrigin origin)
{
    PerformancePublishRequestResult result;
    if (projectGeneration == 0 || authoredContentDigest.empty() || macroSchemaDigest.empty())
        return result;

    const auto duplicateLiveRequest = snapshot.hasRequest
        && sameCapturedInput(snapshot.currentRequest.identity,
                             projectGeneration,
                             draftRevision,
                             authoredContentDigest,
                             macroSchemaDigest)
        && (snapshot.preparationState == PerformancePublishPreparationState::queued
            || snapshot.preparationState == PerformancePublishPreparationState::preparing
            || snapshot.preparationState == PerformancePublishPreparationState::ready)
        && !snapshot.hasFailedRequest;
    if (duplicateLiveRequest)
    {
        ++snapshot.duplicateSuppressedCount;
        result.duplicateSuppressed = true;
        result.request = snapshot.currentRequest;
        publishSnapshot();
        return result;
    }

    const auto previousWasLive = snapshot.hasRequest
        && (snapshot.preparationState == PerformancePublishPreparationState::queued
            || snapshot.preparationState == PerformancePublishPreparationState::preparing
            || snapshot.activationState == PerformancePublishActivationState::pending);
    if (previousWasLive)
    {
        recordCompletion(snapshot.currentRequest.identity,
                         PerformancePublishPreparationState::superseded,
                         snapshot.acceptedPreparedBuildId,
                         false);
        ++snapshot.supersededCount;
        result.supersededPrevious = true;
        if (snapshot.preparationState == PerformancePublishPreparationState::preparing)
        {
            ++snapshot.canceledCount;
            result.cancellationRequested = true;
        }
    }

    if (snapshot.hasRequest)
        ++cancellationGeneration;

    PerformancePublishRequest next;
    next.identity.requestId = nextRequestId++;
    next.identity.cancellationGeneration = cancellationGeneration;
    next.identity.projectGeneration = projectGeneration;
    next.identity.origin = origin;
    next.identity.draftRevision = draftRevision;
    next.identity.authoredContentDigest = std::move(authoredContentDigest);
    next.identity.macroSchemaDigest = std::move(macroSchemaDigest);

    snapshot.hasRequest = true;
    snapshot.currentRequest = next;
    snapshot.hasFailedRequest = false;
    snapshot.failedRequestIdentity = {};
    snapshot.failureFinding = {};
    snapshot.preparationState = PerformancePublishPreparationState::queued;
    snapshot.activationState = PerformancePublishActivationState::noActivation;
    snapshot.acceptedPreparedBuildId = 0;
    snapshot.acceptedPreparedDigest.clear();
    snapshot.acceptedRouteDigest.clear();
    snapshot.acceptedSourceProvenanceDigest.clear();
    snapshot.acceptedMacroSchemaDigest.clear();
    snapshot.pendingActivationToken = 0;
    snapshot.pendingSnapshotBuildId = 0;
    snapshot.pendingPayloadBytes = 0;
    snapshot.requestReceivedAtMicros = nowMicros;
    snapshot.launchedAtMicros = 0;
    snapshot.readyAtMicros = 0;
    snapshot.activationPendingAtMicros = 0;
    snapshot.activeAtMicros = 0;
    snapshot.pendingDepth = 1;
    snapshot.maximumPendingDepth = std::max(snapshot.maximumPendingDepth, snapshot.pendingDepth);
    ++snapshot.requestedCount;

    result.accepted = true;
    result.request = std::move(next);
    publishSnapshot();
    return result;
}

bool PerformancePublishController::markPreparing(
    const PerformancePublishRequestIdentity& identity,
    std::uint64_t nowMicros)
{
    if (!isCurrent(identity) || !transitionTo(PerformancePublishPreparationState::preparing))
        return false;
    snapshot.launchedAtMicros = nowMicros;
    ++snapshot.launchedCount;
    publishSnapshot();
    return true;
}

bool PerformancePublishController::acceptPrepared(const PerformancePublishResult& result,
                                                   std::uint64_t nowMicros)
{
    if (!isCurrent(result.identity)
        || snapshot.preparationState != PerformancePublishPreparationState::preparing
        || !performancePublishResultIsEligible(snapshot.currentRequest.identity, result))
    {
        ++snapshot.rejectedCount;
        recordCompletion(result.identity,
                         PerformancePublishPreparationState::superseded,
                         result.preparedBuildId,
                         false);
        publishSnapshot();
        return false;
    }
    if (!transitionTo(PerformancePublishPreparationState::ready))
        return false;

    snapshot.acceptedPreparedBuildId = result.preparedBuildId;
    snapshot.acceptedPreparedDigest = result.preparedContentDigest;
    snapshot.acceptedRouteDigest = result.routeDigest;
    snapshot.acceptedSourceProvenanceDigest = result.sourceProvenanceDigest;
    snapshot.acceptedMacroSchemaDigest = result.preparedMacroSchemaDigest;
    snapshot.readyAtMicros = nowMicros;
    if (nowMicros != 0 && snapshot.launchedAtMicros != 0 && nowMicros >= snapshot.launchedAtMicros)
    {
        snapshot.lastPreparationMicros = nowMicros - snapshot.launchedAtMicros;
        updateMaximum(snapshot.maxPreparationMicros, snapshot.lastPreparationMicros);
    }
    if (nowMicros != 0
        && snapshot.requestReceivedAtMicros != 0
        && nowMicros >= snapshot.requestReceivedAtMicros)
    {
        snapshot.lastRequestToReadyMicros = nowMicros - snapshot.requestReceivedAtMicros;
        updateMaximum(snapshot.maxRequestToReadyMicros, snapshot.lastRequestToReadyMicros);
    }
    snapshot.pendingDepth = 0;
    ++snapshot.completedCount;
    ++snapshot.acceptedCount;
    recordCompletion(result.identity,
                     PerformancePublishPreparationState::ready,
                     result.preparedBuildId,
                     true);
    publishSnapshot();
    return true;
}

bool PerformancePublishController::authorizeActivation(
    const PerformancePublishActivationPayload& payload,
    std::uint64_t nowMicros)
{
    if (!matchesAcceptedActivation(snapshot, payload)
        || snapshot.preparationState != PerformancePublishPreparationState::ready
        || snapshot.activationState != PerformancePublishActivationState::noActivation)
    {
        ++snapshot.activationAuthorizationRejectedCount;
        publishSnapshot();
        return false;
    }
    snapshot.activationState = PerformancePublishActivationState::pending;
    snapshot.pendingActivationToken = payload.activationToken;
    snapshot.pendingSnapshotBuildId = payload.snapshotBuildId;
    snapshot.pendingPayloadBytes = payload.retainedPreparedBytes;
    snapshot.activationPendingAtMicros = nowMicros;
    ++snapshot.activationAuthorizedCount;
    publishSnapshot();
    return true;
}

bool PerformancePublishController::acknowledgeActivation(
    const PerformancePublishActivationPayload& payload,
    std::uint64_t nowMicros)
{
    if (!matchesAcceptedActivation(snapshot, payload)
        || snapshot.activationState != PerformancePublishActivationState::pending
        || snapshot.pendingActivationToken != payload.activationToken
        || snapshot.pendingSnapshotBuildId != payload.snapshotBuildId
        || snapshot.pendingPayloadBytes != payload.retainedPreparedBytes)
    {
        ++snapshot.activationAcknowledgementRejectedCount;
        publishSnapshot();
        return false;
    }
    snapshot.activationState = PerformancePublishActivationState::active;
    snapshot.hasActiveRequest = true;
    snapshot.activeRequestIdentity = payload.requestIdentity;
    snapshot.activeActivationToken = payload.activationToken;
    snapshot.activeSnapshotBuildId = payload.snapshotBuildId;
    snapshot.activePreparedBuildId = payload.preparedBuildId;
    snapshot.activePayloadBytes = payload.retainedPreparedBytes;
    snapshot.activePreparedDigest = payload.preparedContentDigest;
    snapshot.activeRouteDigest = payload.routeDigest;
    snapshot.activeSourceProvenanceDigest = payload.sourceProvenanceDigest;
    snapshot.activeMacroSchemaDigest = payload.macroSchemaDigest;
    snapshot.pendingActivationToken = 0;
    snapshot.pendingSnapshotBuildId = 0;
    snapshot.pendingPayloadBytes = 0;
    snapshot.activeAtMicros = nowMicros;
    if (nowMicros != 0 && snapshot.requestReceivedAtMicros != 0
        && nowMicros >= snapshot.requestReceivedAtMicros)
    {
        snapshot.lastRequestToActiveMicros = nowMicros - snapshot.requestReceivedAtMicros;
        updateMaximum(snapshot.maxRequestToActiveMicros, snapshot.lastRequestToActiveMicros);
    }
    ++snapshot.activationCount;
    publishSnapshot();
    return true;
}

bool PerformancePublishController::rejectActivationStaging(
    const PerformancePublishActivationPayload& payload,
    PerformancePublishFinding finding)
{
    if (!isCurrent(payload.requestIdentity)
        || snapshot.preparationState != PerformancePublishPreparationState::ready
        || snapshot.activationState != PerformancePublishActivationState::pending
        || snapshot.pendingActivationToken != payload.activationToken)
    {
        ++snapshot.activationStagingRejectedCount;
        publishSnapshot();
        return false;
    }
    if (!transitionTo(PerformancePublishPreparationState::failed))
        return false;
    snapshot.hasFailedRequest = true;
    snapshot.failedRequestIdentity = payload.requestIdentity;
    snapshot.failureFinding = std::move(finding);
    snapshot.activationState = PerformancePublishActivationState::noActivation;
    snapshot.pendingActivationToken = 0;
    snapshot.pendingSnapshotBuildId = 0;
    snapshot.pendingPayloadBytes = 0;
    ++snapshot.failedCount;
    ++snapshot.activationStagingRejectedCount;
    recordCompletion(payload.requestIdentity,
                     PerformancePublishPreparationState::failed,
                     payload.preparedBuildId,
                     false);
    publishSnapshot();
    return true;
}

bool PerformancePublishController::fail(const PerformancePublishRequestIdentity& identity,
                                        PerformancePublishFinding finding)
{
    if (!isCurrent(identity))
    {
        ++snapshot.rejectedCount;
        recordCompletion(identity, PerformancePublishPreparationState::failed, 0, false);
        publishSnapshot();
        return false;
    }
    const auto wasReady = snapshot.preparationState == PerformancePublishPreparationState::ready;
    if (snapshot.preparationState == PerformancePublishPreparationState::queued)
    {
        if (!transitionTo(PerformancePublishPreparationState::preparing))
            return false;
        ++snapshot.launchedCount;
    }
    if (!transitionTo(PerformancePublishPreparationState::failed))
        return false;

    snapshot.hasFailedRequest = true;
    snapshot.failedRequestIdentity = identity;
    snapshot.failureFinding = std::move(finding);
    snapshot.activationState = PerformancePublishActivationState::noActivation;
    snapshot.pendingActivationToken = 0;
    snapshot.pendingSnapshotBuildId = 0;
    snapshot.pendingPayloadBytes = 0;
    snapshot.pendingDepth = 0;
    if (!wasReady)
        ++snapshot.completedCount;
    ++snapshot.failedCount;
    recordCompletion(identity, PerformancePublishPreparationState::failed, 0, false);
    publishSnapshot();
    return true;
}

bool PerformancePublishController::cancelCurrent()
{
    if (!snapshot.hasRequest)
        return false;
    const auto state = snapshot.preparationState;
    if (state != PerformancePublishPreparationState::queued
        && state != PerformancePublishPreparationState::preparing
        && !(state == PerformancePublishPreparationState::ready
             && snapshot.activationState == PerformancePublishActivationState::pending))
        return false;
    const auto wasReady = state == PerformancePublishPreparationState::ready;
    if (!transitionTo(PerformancePublishPreparationState::canceled))
        return false;
    snapshot.activationState = PerformancePublishActivationState::noActivation;
    snapshot.pendingActivationToken = 0;
    snapshot.pendingSnapshotBuildId = 0;
    snapshot.pendingPayloadBytes = 0;
    snapshot.pendingDepth = 0;
    ++snapshot.canceledCount;
    if (!wasReady)
        ++snapshot.completedCount;
    ++cancellationGeneration;
    recordCompletion(snapshot.currentRequest.identity,
                     PerformancePublishPreparationState::canceled,
                     snapshot.acceptedPreparedBuildId,
                     false);
    publishSnapshot();
    return true;
}

void PerformancePublishController::reset(bool clearActive,
                                         bool advanceCancellationGeneration)
{
    const auto requestedCount = snapshot.requestedCount;
    const auto duplicateSuppressedCount = snapshot.duplicateSuppressedCount;
    const auto launchedCount = snapshot.launchedCount;
    const auto canceledCount = snapshot.canceledCount;
    const auto supersededCount = snapshot.supersededCount;
    const auto completedCount = snapshot.completedCount;
    const auto acceptedCount = snapshot.acceptedCount;
    const auto rejectedCount = snapshot.rejectedCount;
    const auto failedCount = snapshot.failedCount;
    const auto activationCount = snapshot.activationCount;
    const auto activationAuthorizedCount = snapshot.activationAuthorizedCount;
    const auto activationAuthorizationRejectedCount = snapshot.activationAuthorizationRejectedCount;
    const auto activationStagingRejectedCount = snapshot.activationStagingRejectedCount;
    const auto activationAcknowledgementRejectedCount = snapshot.activationAcknowledgementRejectedCount;
    const auto maximumPendingDepth = snapshot.maximumPendingDepth;
    const auto activeRequestIdentity = snapshot.activeRequestIdentity;
    const auto activeActivationToken = snapshot.activeActivationToken;
    const auto activeSnapshotBuildId = snapshot.activeSnapshotBuildId;
    const auto activePreparedBuildId = snapshot.activePreparedBuildId;
    const auto activePayloadBytes = snapshot.activePayloadBytes;
    const auto activePreparedDigest = snapshot.activePreparedDigest;
    const auto activeRouteDigest = snapshot.activeRouteDigest;
    const auto activeSourceProvenanceDigest = snapshot.activeSourceProvenanceDigest;
    const auto activeMacroSchemaDigest = snapshot.activeMacroSchemaDigest;
    const auto hasActiveRequest = snapshot.hasActiveRequest;

    snapshot = {};
    snapshot.requestedCount = requestedCount;
    snapshot.duplicateSuppressedCount = duplicateSuppressedCount;
    snapshot.launchedCount = launchedCount;
    snapshot.canceledCount = canceledCount;
    snapshot.supersededCount = supersededCount;
    snapshot.completedCount = completedCount;
    snapshot.acceptedCount = acceptedCount;
    snapshot.rejectedCount = rejectedCount;
    snapshot.failedCount = failedCount;
    snapshot.activationCount = activationCount;
    snapshot.activationAuthorizedCount = activationAuthorizedCount;
    snapshot.activationAuthorizationRejectedCount = activationAuthorizationRejectedCount;
    snapshot.activationStagingRejectedCount = activationStagingRejectedCount;
    snapshot.activationAcknowledgementRejectedCount = activationAcknowledgementRejectedCount;
    snapshot.maximumPendingDepth = maximumPendingDepth;
    if (!clearActive)
    {
        snapshot.hasActiveRequest = hasActiveRequest;
        snapshot.activeRequestIdentity = activeRequestIdentity;
        snapshot.activeActivationToken = activeActivationToken;
        snapshot.activeSnapshotBuildId = activeSnapshotBuildId;
        snapshot.activePreparedBuildId = activePreparedBuildId;
        snapshot.activePayloadBytes = activePayloadBytes;
        snapshot.activePreparedDigest = activePreparedDigest;
        snapshot.activeRouteDigest = activeRouteDigest;
        snapshot.activeSourceProvenanceDigest = activeSourceProvenanceDigest;
        snapshot.activeMacroSchemaDigest = activeMacroSchemaDigest;
    }
    if (advanceCancellationGeneration)
        ++cancellationGeneration;
    publishSnapshot();
}

PerformancePublishControllerSnapshot PerformancePublishController::getSnapshot() const
{
    const auto published = std::atomic_load_explicit(&publishedSnapshot,
                                                      std::memory_order_acquire);
    return published != nullptr ? *published : PerformancePublishControllerSnapshot {};
}

bool PerformancePublishController::isCurrent(
    const PerformancePublishRequestIdentity& identity) const noexcept
{
    return snapshot.hasRequest && snapshot.currentRequest.identity == identity;
}

bool PerformancePublishController::transitionTo(PerformancePublishPreparationState state)
{
    if (!isPerformancePublishPreparationTransitionAllowed(snapshot.preparationState, state))
        return false;
    snapshot.preparationState = state;
    return true;
}

void PerformancePublishController::publishSnapshot()
{
    auto next = std::make_shared<const PerformancePublishControllerSnapshot>(snapshot);
    std::atomic_store_explicit(&publishedSnapshot, std::move(next), std::memory_order_release);
}

void PerformancePublishController::recordCompletion(
    const PerformancePublishRequestIdentity& identity,
    PerformancePublishPreparationState outcome,
    std::uint64_t preparedBuildId,
    bool accepted)
{
    completionRecords.push_back({ identity, outcome, preparedBuildId, accepted });
    while (completionRecords.size() > config.maximumCompletionRecords)
        completionRecords.pop_front();
    snapshot.retainedCompletionRecordCount = completionRecords.size();
}
} // namespace drs::engine
