#include "drs/engine/AuthoringPreviewController.h"

#include <algorithm>
#include <utility>

namespace drs::engine
{
AuthoringPreviewController::AuthoringPreviewController(AuthoringPreviewControllerConfig nextConfig)
    : config(nextConfig)
{
    config.maximumLaunchDelayMicros = std::max(config.maximumLaunchDelayMicros,
                                               config.coalescingWindowMicros);
    config.maximumCompletionRecords = std::max<std::size_t>(1, config.maximumCompletionRecords);
    config.maximumWarmPreparedRecords = std::max<std::size_t>(1, config.maximumWarmPreparedRecords);
    snapshot.configuredCoalescingWindowMicros = config.coalescingWindowMicros;
    snapshot.configuredMaximumLaunchDelayMicros = config.maximumLaunchDelayMicros;
}

AuthoringPreviewRequestResult AuthoringPreviewController::request(
    AuthoringPreviewScope scope,
    std::size_t draftRevision,
    std::string selectedZoneId,
    AuthoringPreviewRequestReason reason,
    AuthoringPreviewInvalidationCategory invalidationCategory,
    std::string requestSignature,
    std::uint64_t nowMicros)
{
    AuthoringPreviewRequestResult result;
    if (!authoringPreviewScopeIsEligible(scope, !selectedZoneId.empty()))
        return result;

    if (snapshot.hasRequest
        && snapshot.currentRequest.identity.draftRevision == draftRevision
        && snapshot.currentRequest.identity.scope == scope
        && snapshot.currentRequest.identity.selectedZoneId == selectedZoneId
        && snapshot.currentRequest.requestSignature == requestSignature)
    {
        const auto directAudition
            = reason == AuthoringPreviewRequestReason::explicitSelectedZoneAudition
            || reason == AuthoringPreviewRequestReason::explicitCurrentDraftAudition;
        if (directAudition
            && snapshot.preparationState == AuthoringPreviewPreparationState::queued)
        {
            snapshot.currentRequest.reason = reason;
            result.expeditedCurrent = true;
            result.request = snapshot.currentRequest;
        }
        return result;
    }

    const auto previousState = snapshot.preparationState;
    const auto supersedesLiveRequest
        = previousState == AuthoringPreviewPreparationState::queued
        || previousState == AuthoringPreviewPreparationState::preparing
        || previousState == AuthoringPreviewPreparationState::ready;
    const auto continuesCoalescingBurst
        = snapshot.hasRequest && previousState == AuthoringPreviewPreparationState::queued;
    if (snapshot.hasRequest && supersedesLiveRequest)
    {
        recordCompletion(snapshot.currentRequest.identity,
                         AuthoringPreviewPreparationState::superseded,
                         snapshot.acceptedPreparedBuildId,
                         false);
        ++snapshot.supersededCount;
        result.supersededPrevious = true;
        if (continuesCoalescingBurst)
            ++snapshot.coalescedCount;
        if (previousState == AuthoringPreviewPreparationState::preparing)
        {
            ++snapshot.canceledCount;
            ++cancellationGeneration;
            result.cancellationRequested = true;
        }
    }

    AuthoringPreviewRequest next;
    next.identity.requestId = nextRequestId++;
    next.identity.cancellationGeneration = cancellationGeneration;
    next.identity.draftRevision = draftRevision;
    next.identity.scope = scope;
    next.identity.selectedZoneId = std::move(selectedZoneId);
    next.reason = reason;
    next.invalidationCategory = invalidationCategory;
    next.requestSignature = std::move(requestSignature);

    const auto burstStartedAt = continuesCoalescingBurst
        ? snapshot.coalescingBurstStartedAtMicros
        : nowMicros;
    const auto windowDeadline = nowMicros + config.coalescingWindowMicros;
    const auto maximumDeadline = burstStartedAt + config.maximumLaunchDelayMicros;

    snapshot.hasRequest = true;
    snapshot.currentRequest = next;
    snapshot.preparationState = AuthoringPreviewPreparationState::queued;
    snapshot.activationState = AuthoringPreviewActivationState::noActivation;
    snapshot.acceptedPreparedBuildId = 0;
    snapshot.reusablePreparedBuildId = findReusablePreparedBuildId(
        next.identity.scope, next.identity.selectedZoneId, next.requestSignature);
    snapshot.coalescingBurstStartedAtMicros = burstStartedAt;
    snapshot.launchEligibleAtMicros = std::min(windowDeadline, maximumDeadline);
    snapshot.failureState.clear();
    snapshot.pendingDepth = 1;
    snapshot.maximumPendingDepth = std::max(snapshot.maximumPendingDepth, snapshot.pendingDepth);
    ++snapshot.requestedCount;
    result.accepted = true;
    result.request = std::move(next);
    return result;
}

AuthoringPreviewLaunchResult AuthoringPreviewController::launchIfEligible(
    std::uint64_t nowMicros,
    bool directAuditionContentPrepared)
{
    AuthoringPreviewLaunchResult result;
    if (!snapshot.hasRequest
        || snapshot.preparationState != AuthoringPreviewPreparationState::queued)
        return result;

    const auto directAudition
        = snapshot.currentRequest.reason == AuthoringPreviewRequestReason::explicitSelectedZoneAudition
        || snapshot.currentRequest.reason == AuthoringPreviewRequestReason::explicitCurrentDraftAudition;
    const auto projectOpened
        = snapshot.currentRequest.reason == AuthoringPreviewRequestReason::projectOpened;
    const auto deadlineReached = nowMicros >= snapshot.launchEligibleAtMicros;
    const auto maximumDeadline = snapshot.coalescingBurstStartedAtMicros
        + config.maximumLaunchDelayMicros;
    const auto maximumDelayReached = nowMicros >= maximumDeadline;
    const auto warmDirectAudition = directAudition && directAuditionContentPrepared;
    if (!projectOpened && !warmDirectAudition && !deadlineReached && !maximumDelayReached)
        return result;

    if (!transitionTo(AuthoringPreviewPreparationState::preparing))
        return result;

    ++snapshot.launchedCount;
    result.launched = true;
    result.deadlineForced = maximumDelayReached;
    result.warmPreparedResultAvailable = snapshot.reusablePreparedBuildId != 0;
    result.reusablePreparedBuildId = snapshot.reusablePreparedBuildId;
    result.request = snapshot.currentRequest;
    return result;
}

bool AuthoringPreviewController::acceptPrepared(const AuthoringPreviewRequestIdentity& identity,
                                                std::uint64_t preparedBuildId)
{
    if (!isCurrent(identity) || preparedBuildId == 0)
    {
        ++snapshot.rejectedCount;
        recordCompletion(identity, AuthoringPreviewPreparationState::superseded,
                         preparedBuildId, false);
        return false;
    }

    if (snapshot.preparationState == AuthoringPreviewPreparationState::queued)
    {
        if (!transitionTo(AuthoringPreviewPreparationState::preparing))
            return false;
        ++snapshot.launchedCount;
    }
    if (!transitionTo(AuthoringPreviewPreparationState::ready))
    {
        ++snapshot.rejectedCount;
        recordCompletion(identity, snapshot.preparationState, preparedBuildId, false);
        return false;
    }

    snapshot.acceptedPreparedBuildId = preparedBuildId;
    snapshot.failureState.clear();
    snapshot.pendingDepth = 0;
    ++snapshot.acceptedCount;
    if (snapshot.reusablePreparedBuildId == preparedBuildId)
        ++snapshot.reusedPreparedCount;
    rememberPreparedResult(snapshot.currentRequest, preparedBuildId);
    recordCompletion(identity, AuthoringPreviewPreparationState::ready, preparedBuildId, true);
    return true;
}

bool AuthoringPreviewController::markActivationPending(
    const AuthoringPreviewRequestIdentity& identity)
{
    if (!isCurrent(identity)
        || snapshot.preparationState != AuthoringPreviewPreparationState::ready)
        return false;
    snapshot.activationState = AuthoringPreviewActivationState::pending;
    return true;
}

bool AuthoringPreviewController::markActive(const AuthoringPreviewRequestIdentity& identity)
{
    if (!isCurrent(identity)
        || snapshot.activationState != AuthoringPreviewActivationState::pending)
        return false;
    snapshot.activationState = AuthoringPreviewActivationState::active;
    ++snapshot.activationCount;
    return true;
}

bool AuthoringPreviewController::fail(const AuthoringPreviewRequestIdentity& identity,
                                      std::string failureState)
{
    if (!isCurrent(identity))
    {
        ++snapshot.rejectedCount;
        recordCompletion(identity, AuthoringPreviewPreparationState::failed, 0, false);
        return false;
    }
    if (snapshot.preparationState == AuthoringPreviewPreparationState::queued)
    {
        transitionTo(AuthoringPreviewPreparationState::preparing);
        ++snapshot.launchedCount;
    }
    if (!transitionTo(AuthoringPreviewPreparationState::failed))
        return false;
    snapshot.failureState = std::move(failureState);
    snapshot.activationState = AuthoringPreviewActivationState::noActivation;
    snapshot.pendingDepth = 0;
    recordCompletion(identity, AuthoringPreviewPreparationState::failed, 0, false);
    return true;
}

bool AuthoringPreviewController::cancelCurrent()
{
    if (!snapshot.hasRequest
        || (snapshot.preparationState != AuthoringPreviewPreparationState::queued
            && snapshot.preparationState != AuthoringPreviewPreparationState::preparing))
        return false;
    snapshot.preparationState = AuthoringPreviewPreparationState::canceled;
    snapshot.activationState = AuthoringPreviewActivationState::noActivation;
    snapshot.pendingDepth = 0;
    ++snapshot.canceledCount;
    ++cancellationGeneration;
    recordCompletion(snapshot.currentRequest.identity,
                     AuthoringPreviewPreparationState::canceled, 0, false);
    return true;
}

void AuthoringPreviewController::recordWorkerCancellation()
{
    ++snapshot.canceledCount;
}

void AuthoringPreviewController::reset(bool advanceCancellationGeneration)
{
    if (snapshot.hasRequest
        && (snapshot.preparationState == AuthoringPreviewPreparationState::queued
            || snapshot.preparationState == AuthoringPreviewPreparationState::preparing))
    {
        recordCompletion(snapshot.currentRequest.identity,
                         AuthoringPreviewPreparationState::canceled, 0, false);
        ++snapshot.canceledCount;
    }
    if (advanceCancellationGeneration)
        ++cancellationGeneration;

    const auto retainedMetrics = snapshot;
    snapshot = {};
    snapshot.requestedCount = retainedMetrics.requestedCount;
    snapshot.coalescedCount = retainedMetrics.coalescedCount;
    snapshot.launchedCount = retainedMetrics.launchedCount;
    snapshot.canceledCount = retainedMetrics.canceledCount;
    snapshot.supersededCount = retainedMetrics.supersededCount;
    snapshot.completedCount = retainedMetrics.completedCount;
    snapshot.acceptedCount = retainedMetrics.acceptedCount;
    snapshot.rejectedCount = retainedMetrics.rejectedCount;
    snapshot.reusedPreparedCount = retainedMetrics.reusedPreparedCount;
    snapshot.maximumPendingDepth = retainedMetrics.maximumPendingDepth;
    snapshot.retainedCompletionRecordCount = completionRecords.size();
    snapshot.activationCount = retainedMetrics.activationCount;
    snapshot.configuredCoalescingWindowMicros = config.coalescingWindowMicros;
    snapshot.configuredMaximumLaunchDelayMicros = config.maximumLaunchDelayMicros;
    warmPreparedRecords.clear();
}

bool AuthoringPreviewController::isCurrent(
    const AuthoringPreviewRequestIdentity& identity) const noexcept
{
    return snapshot.hasRequest && snapshot.currentRequest.identity == identity;
}

bool AuthoringPreviewController::transitionTo(AuthoringPreviewPreparationState state)
{
    if (!isAuthoringPreviewPreparationTransitionAllowed(snapshot.preparationState, state))
        return false;
    snapshot.preparationState = state;
    return true;
}

void AuthoringPreviewController::recordCompletion(
    const AuthoringPreviewRequestIdentity& identity,
    AuthoringPreviewPreparationState outcome,
    std::uint64_t preparedBuildId,
    bool accepted)
{
    completionRecords.push_back({ identity, outcome, preparedBuildId, accepted });
    while (completionRecords.size() > config.maximumCompletionRecords)
        completionRecords.pop_front();
    ++snapshot.completedCount;
    snapshot.retainedCompletionRecordCount = completionRecords.size();
}

std::uint64_t AuthoringPreviewController::findReusablePreparedBuildId(
    AuthoringPreviewScope scope,
    const std::string& selectedZoneId,
    const std::string& requestSignature) const
{
    for (auto iterator = warmPreparedRecords.rbegin(); iterator != warmPreparedRecords.rend(); ++iterator)
    {
        if (iterator->scope == scope
            && iterator->selectedZoneId == selectedZoneId
            && iterator->requestSignature == requestSignature)
            return iterator->preparedBuildId;
    }
    return 0;
}

void AuthoringPreviewController::rememberPreparedResult(
    const AuthoringPreviewRequest& request,
    std::uint64_t preparedBuildId)
{
    for (auto iterator = warmPreparedRecords.begin(); iterator != warmPreparedRecords.end(); ++iterator)
    {
        if (iterator->scope == request.identity.scope
            && iterator->selectedZoneId == request.identity.selectedZoneId
            && iterator->requestSignature == request.requestSignature)
        {
            warmPreparedRecords.erase(iterator);
            break;
        }
    }
    warmPreparedRecords.push_back({ request.identity.scope,
                                    request.identity.selectedZoneId,
                                    request.requestSignature,
                                    preparedBuildId });
    while (warmPreparedRecords.size() > config.maximumWarmPreparedRecords)
        warmPreparedRecords.pop_front();
}
} // namespace drs::engine
