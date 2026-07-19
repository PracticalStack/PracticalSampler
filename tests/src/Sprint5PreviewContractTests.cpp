#include "drs/engine/AuthoringPreviewContract.h"

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
    using Preparation = AuthoringPreviewPreparationState;
    using Event = AuthoringPreviewNotePolicyEvent;
    using Action = AuthoringPreviewNotePolicyAction;

    try
    {
        require(!authoringPreviewScopeIsEligible(AuthoringPreviewScope::selectedZone, false),
                "Selected-zone Preview must reject a request without a selected zone.");
        require(authoringPreviewScopeIsEligible(AuthoringPreviewScope::selectedZone, true),
                "Selected-zone Preview must accept an identified selected zone.");
        require(authoringPreviewScopeIsEligible(AuthoringPreviewScope::currentDraft, false),
                "Current-draft Preview must not require a selected zone.");

        require(isAuthoringPreviewPreparationTransitionAllowed(Preparation::idle, Preparation::queued)
                    && isAuthoringPreviewPreparationTransitionAllowed(Preparation::queued, Preparation::preparing)
                    && isAuthoringPreviewPreparationTransitionAllowed(Preparation::preparing, Preparation::ready),
                "The ordinary Preview preparation lifecycle must remain executable.");
        require(isAuthoringPreviewPreparationTransitionAllowed(Preparation::queued, Preparation::superseded)
                    && isAuthoringPreviewPreparationTransitionAllowed(Preparation::preparing, Preparation::canceled)
                    && isAuthoringPreviewPreparationTransitionAllowed(Preparation::preparing, Preparation::failed),
                "Queued/running work must have explicit terminal outcomes.");
        require(!isAuthoringPreviewPreparationTransitionAllowed(Preparation::idle, Preparation::ready)
                    && !isAuthoringPreviewPreparationTransitionAllowed(Preparation::failed, Preparation::ready)
                    && !isAuthoringPreviewPreparationTransitionAllowed(Preparation::superseded, Preparation::preparing),
                "Preview results must not skip request ownership or revive terminal work.");

        const AuthoringPreviewRequestIdentity first {
            17, 3, 42, AuthoringPreviewScope::selectedZone, "pad-a3-high"
        };
        auto differentSelection = first;
        differentSelection.selectedZoneId = "lead-a4-sustain";
        auto differentScope = first;
        differentScope.scope = AuthoringPreviewScope::currentDraft;
        auto differentGeneration = first;
        ++differentGeneration.cancellationGeneration;
        require(first != differentSelection && first != differentScope && first != differentGeneration,
                "Request identity must distinguish selection, scope, and cancellation generation independently of revision.");

        require(authoringPreviewNotePolicyFor(Event::activationReplacement) == Action::allowOldVoicesToFinish
                    && authoringPreviewNotePolicyFor(Event::selectionChange) == Action::allowOldVoicesToFinish
                    && authoringPreviewNotePolicyFor(Event::draftBecomesStale) == Action::allowOldVoicesToFinish,
                "Replacement, selection, and staleness must preserve old-model voice leases.");
        require(authoringPreviewNotePolicyFor(Event::previewStop) == Action::releasePreviewVoices,
                "Preview stop must release only Preview voices.");
        require(authoringPreviewNotePolicyFor(Event::projectClose) == Action::resetPreviewVoicesAndClearActivation,
                "Project close must reset Preview and clear its activation.");
        require(authoringPreviewNotePolicyFor(Event::deviceRestart) == Action::resetPreviewVoicesAndPreserveActivation,
                "Device restart must reset Preview voices while preserving the usable activation.");

        std::cout << "Mini Sprint 5.1 Preview contract matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.1 Preview contract matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
