#include "drs/engine/AuthoringPreviewController.h"

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
    using namespace drs::engine;
    try
    {
        AuthoringPreviewController controller({ 0, 0, 32, 8 });
        const auto missingSelection = controller.request(
            AuthoringPreviewScope::selectedZone, 4, {},
            AuthoringPreviewRequestReason::explicitSelectedZoneAudition,
            AuthoringPreviewInvalidationCategory::selection, "4|selected|", 0);
        require(!missingSelection.accepted && !controller.getSnapshot().hasRequest,
                "Selected-zone requests without a selection must be rejected.");

        const auto first = controller.request(
            AuthoringPreviewScope::selectedZone, 4, "pad-a3-high",
            AuthoringPreviewRequestReason::projectOpened,
            AuthoringPreviewInvalidationCategory::selection,
            "4|selected|pad-a3-high", 0);
        require(first.accepted && first.request.identity.requestId == 1,
                "The first eligible request should receive the first stable identity.");
        require(controller.launchIfEligible(0).launched,
                "The current queued request should enter Preparing.");

        const auto duplicate = controller.request(
            AuthoringPreviewScope::selectedZone, 4, "pad-a3-high",
            AuthoringPreviewRequestReason::selectionChanged,
            AuthoringPreviewInvalidationCategory::selection,
            "4|selected|pad-a3-high", 0);
        require(!duplicate.accepted && controller.getSnapshot().requestedCount == 1,
                "An equivalent observation must not create duplicate work.");

        const auto replacement = controller.request(
            AuthoringPreviewScope::selectedZone, 4, "lead-a4-sustain",
            AuthoringPreviewRequestReason::selectionChanged,
            AuthoringPreviewInvalidationCategory::selection,
            "4|selected|lead-a4-sustain", 0);
        require(replacement.accepted && replacement.supersededPrevious
                    && replacement.request.identity.requestId == 2,
                "Selection identity must supersede same-revision work.");
        require(!controller.acceptPrepared(first.request.identity, 1001),
                "A superseded completion must be rejected by full request identity.");
        auto wrongRequest = replacement.request.identity;
        ++wrongRequest.requestId;
        auto wrongRevision = replacement.request.identity;
        ++wrongRevision.draftRevision;
        auto wrongScope = replacement.request.identity;
        wrongScope.scope = AuthoringPreviewScope::currentDraft;
        auto wrongSelection = replacement.request.identity;
        wrongSelection.selectedZoneId = "pad-a3-high";
        auto wrongGeneration = replacement.request.identity;
        ++wrongGeneration.cancellationGeneration;
        require(!controller.acceptPrepared(wrongRequest, 1002)
                    && !controller.acceptPrepared(wrongRevision, 1002)
                    && !controller.acceptPrepared(wrongScope, 1002)
                    && !controller.acceptPrepared(wrongSelection, 1002)
                    && !controller.acceptPrepared(wrongGeneration, 1002),
                "Every request identity field must participate in result acceptance.");
        require(!controller.markActivationPending(replacement.request.identity)
                    && !controller.markActive(replacement.request.identity),
                "Activation cannot skip preparation or pending activation.");
        require(controller.launchIfEligible(0).launched
                    && controller.acceptPrepared(replacement.request.identity, 1002)
                    && controller.markActivationPending(replacement.request.identity)
                    && controller.markActive(replacement.request.identity),
                "The newest request should progress through preparation and activation.");

        auto snapshot = controller.getSnapshot();
        require(snapshot.preparationState == AuthoringPreviewPreparationState::ready
                    && snapshot.activationState == AuthoringPreviewActivationState::active
                    && snapshot.acceptedPreparedBuildId == 1002
                    && snapshot.supersededCount == 1
                    && snapshot.rejectedCount == 6
                    && snapshot.acceptedCount == 1
                    && snapshot.activationCount == 1,
                "Controller diagnostics should reconcile newest-wins acceptance.");

        const auto currentDraft = controller.request(
            AuthoringPreviewScope::currentDraft, 5, {},
            AuthoringPreviewRequestReason::explicitCurrentDraftAudition,
            AuthoringPreviewInvalidationCategory::previewScope, "5|draft", 1);
        require(currentDraft.accepted && currentDraft.supersededPrevious,
                "Current-draft scope should supersede the previous active request without selection.");
        require(controller.cancelCurrent(), "Queued work should support explicit cancellation.");
        require(!controller.acceptPrepared(currentDraft.request.identity, 1003),
                "Canceled work must not become Ready.");

        const auto recovery = controller.request(
            AuthoringPreviewScope::currentDraft, 5, {},
            AuthoringPreviewRequestReason::recovery,
            AuthoringPreviewInvalidationCategory::previewScope, "5|draft|recovery", 2);
        require(recovery.accepted && !recovery.supersededPrevious
                    && recovery.request.identity.cancellationGeneration
                        > currentDraft.request.identity.cancellationGeneration,
                "Recovery after cancellation must use a newer generation without re-superseding terminal work.");
        require(controller.launchIfEligible(2).launched
                    && controller.fail(recovery.request.identity, "missing source"),
                "The current running request should accept a structured failure.");
        snapshot = controller.getSnapshot();
        require(snapshot.preparationState == AuthoringPreviewPreparationState::failed
                    && snapshot.failureState == "missing source",
                "Failure state must belong to the matching current request.");

        controller.reset();
        require(!controller.getSnapshot().hasRequest
                    && controller.getSnapshot().preparationState == AuthoringPreviewPreparationState::idle,
                "Reset should clear current lifecycle state.");

        std::cout << "Mini Sprint 5.2 Preview controller state-machine matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.2 Preview controller state-machine matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
