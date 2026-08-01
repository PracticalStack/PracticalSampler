#include "drs/engine/PerformancePublishPresentation.h"

namespace drs::engine
{
const char* toString(PerformancePublishPresentationState state) noexcept
{
    switch (state)
    {
        case PerformancePublishPresentationState::idle: return "Idle";
        case PerformancePublishPresentationState::queued: return "Queued";
        case PerformancePublishPresentationState::preparing: return "Preparing";
        case PerformancePublishPresentationState::ready: return "Ready";
        case PerformancePublishPresentationState::activating: return "Activating";
        case PerformancePublishPresentationState::active: return "Active";
        case PerformancePublishPresentationState::stale: return "Stale";
        case PerformancePublishPresentationState::failed: return "Failed";
        case PerformancePublishPresentationState::canceled: return "Canceled";
        case PerformancePublishPresentationState::superseded: return "Superseded";
    }
    return "Idle";
}

PerformancePublishPresentationSnapshot buildPerformancePublishPresentationSnapshot(
    const DraftPlaybackStatus& draft,
    const PerformancePublishControllerSnapshot& controller,
    const PreparedPlaybackWorkerStatus& worker,
    std::uint64_t publicationSequence)
{
    PerformancePublishPresentationSnapshot result;
    result.publicationSequence = publicationSequence;
    result.projectOpen = draft.projectOpen;
    result.draftRevision = draft.draftRevision;
    result.previewRevision = draft.preview.revision;
    result.previewContentDigest = draft.preview.contentDigest;
    result.hasRequestedPublish = controller.hasRequest;
    if (controller.hasRequest)
    {
        result.requestedPublishRevision = controller.currentRequest.identity.draftRevision;
        result.requestedPublishDigest = controller.currentRequest.identity.authoredContentDigest;
        result.draftContentDigest = result.requestedPublishDigest;
    }
    else if (draft.preview.revision == draft.draftRevision)
    {
        result.draftContentDigest = draft.preview.contentDigest;
    }

    result.hasActivePublished = controller.hasActiveRequest || draft.performance.available;
    result.activePublishedRevision = controller.hasActiveRequest
        ? controller.activeRequestIdentity.draftRevision : draft.performance.revision;
    result.activePublishedDigest = controller.hasActiveRequest
        ? controller.activeRequestIdentity.authoredContentDigest : draft.performance.contentDigest;
    result.activeMacroSchemaDigest = controller.hasActiveRequest
        ? controller.activeMacroSchemaDigest : draft.performance.macroSchemaDigest;
    result.hasLastKnownGood = result.hasActivePublished;
    result.lastKnownGoodRevision = result.activePublishedRevision;
    result.lastKnownGoodDigest = result.activePublishedDigest;
    result.preparedBuildId = controller.hasActiveRequest
        ? controller.activePreparedBuildId : draft.performance.preparedBuildId;
    result.activePayloadBytes = controller.hasActiveRequest
        ? controller.activePayloadBytes : draft.performance.activationPayloadRetainedBytes;

    result.hasFailure = controller.hasFailedRequest;
    if (result.hasFailure)
    {
        result.failedRevision = controller.failedRequestIdentity.draftRevision;
        result.failedDigest = controller.failedRequestIdentity.authoredContentDigest;
        result.findingCode = controller.failureFinding.code;
        result.findingPath = controller.failureFinding.path;
        result.findingMessage = controller.failureFinding.message;
    }
    result.exposedMacroCount = controller.exposedMacroCount;
    result.hiddenMacroCount = controller.hiddenMacroCount;
    result.assignedMacroCount = controller.assignedMacroCount;
    result.unassignedMacroCount = controller.unassignedMacroCount;
    result.availableHostSlotCount = controller.availableHostSlotCount;
    result.activeHostSlotCount = controller.activeHostSlotCount;

    result.pendingWorkCount = worker.pendingWorkCount;
    result.inFlightWorkCount = worker.inFlightWorkCount;
    result.lastPreparationMicros = controller.lastPreparationMicros;
    result.lastRequestToReadyMicros = controller.lastRequestToReadyMicros;
    result.lastRequestToActiveMicros = controller.lastRequestToActiveMicros;
    result.dirty = !result.hasActivePublished
        || result.activePublishedRevision != result.draftRevision
        || (!result.draftContentDigest.empty()
            && result.activePublishedDigest != result.draftContentDigest);

    const auto busy = controller.preparationState == PerformancePublishPreparationState::queued
        || controller.preparationState == PerformancePublishPreparationState::preparing
        || controller.activationState == PerformancePublishActivationState::pending;
    result.canPublish = result.projectOpen && result.dirty && !busy;

    if (!result.projectOpen)
        result.state = PerformancePublishPresentationState::idle;
    else if (controller.hasFailedRequest)
        result.state = PerformancePublishPresentationState::failed;
    else if (controller.activationState == PerformancePublishActivationState::pending)
        result.state = PerformancePublishPresentationState::activating;
    else if (controller.activationState == PerformancePublishActivationState::active)
        result.state = result.dirty
            ? PerformancePublishPresentationState::stale
            : PerformancePublishPresentationState::active;
    else if (controller.preparationState == PerformancePublishPreparationState::queued)
        result.state = PerformancePublishPresentationState::queued;
    else if (controller.preparationState == PerformancePublishPreparationState::preparing)
        result.state = PerformancePublishPresentationState::preparing;
    else if (controller.preparationState == PerformancePublishPreparationState::ready)
        result.state = PerformancePublishPresentationState::ready;
    else if (controller.preparationState == PerformancePublishPreparationState::canceled)
        result.state = PerformancePublishPresentationState::canceled;
    else if (controller.preparationState == PerformancePublishPreparationState::superseded)
        result.state = PerformancePublishPresentationState::superseded;
    else if (result.hasActivePublished && result.dirty)
        result.state = PerformancePublishPresentationState::stale;
    else if (result.hasActivePublished)
        result.state = PerformancePublishPresentationState::active;
    else
        result.state = PerformancePublishPresentationState::idle;

    switch (result.state)
    {
        case PerformancePublishPresentationState::idle:
            result.progress = 0.0;
            result.guidance = result.projectOpen
                ? "Prepare the current draft, then Publish it to Performance."
                : "Open a project to enable Publish.";
            break;
        case PerformancePublishPresentationState::queued:
            result.progress = 0.2;
            result.guidance = "Publish is queued behind bounded preparation work.";
            break;
        case PerformancePublishPresentationState::preparing:
            result.progress = 0.5;
            result.guidance = "Preparing the complete immutable Performance revision.";
            break;
        case PerformancePublishPresentationState::ready:
            result.progress = 0.75;
            result.guidance = "Preparation is ready; waiting for activation authorization.";
            break;
        case PerformancePublishPresentationState::activating:
            result.progress = 0.9;
            result.guidance = "Activation is pending at the next audio block boundary.";
            break;
        case PerformancePublishPresentationState::active:
            result.progress = 1.0;
            result.guidance = "The current draft is active in Performance.";
            break;
        case PerformancePublishPresentationState::stale:
            result.progress = 1.0;
            result.guidance = "Performance is using last-known-good; Publish the newer draft when ready.";
            break;
        case PerformancePublishPresentationState::failed:
            result.progress = 0.0;
            result.guidance = result.findingMessage.empty()
                ? "Publish failed; repair the current draft and retry. Last-known-good remains active."
                : result.findingMessage + " Last-known-good remains active.";
            break;
        case PerformancePublishPresentationState::canceled:
            result.progress = 0.0;
            result.guidance = "Publish was canceled; the last-known-good revision remains active.";
            break;
        case PerformancePublishPresentationState::superseded:
            result.progress = 0.0;
            result.guidance = "A newer Publish superseded this request.";
            break;
    }
    result.stateLabel = toString(result.state);
    result.progressLabel = result.stateLabel + " "
        + std::to_string(static_cast<int>(result.progress * 100.0)) + "%";
    return result;
}
} // namespace drs::engine
