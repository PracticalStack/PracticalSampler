#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"

#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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
        constexpr std::array invalidations {
            AuthoringPreviewInvalidationCategory::mapping,
            AuthoringPreviewInvalidationCategory::gain,
            AuthoringPreviewInvalidationCategory::pan,
            AuthoringPreviewInvalidationCategory::rootKey,
            AuthoringPreviewInvalidationCategory::keyBounds,
            AuthoringPreviewInvalidationCategory::velocityRange,
            AuthoringPreviewInvalidationCategory::sampleStartOffset,
            AuthoringPreviewInvalidationCategory::loop,
            AuthoringPreviewInvalidationCategory::sourceAssignment,
            AuthoringPreviewInvalidationCategory::selection,
            AuthoringPreviewInvalidationCategory::previewScope
        };
        std::vector<std::string> signatures;
        for (const auto invalidation : invalidations)
        {
            signatures.push_back(buildAuthoringPreviewRequestSignature(
                AuthoringPreviewScope::selectedZone, "zone-a", invalidation, "content-a"));
        }
        for (std::size_t left = 0; left < signatures.size(); ++left)
        {
            for (std::size_t right = left + 1; right < signatures.size(); ++right)
                require(signatures[left] != signatures[right],
                        "Each Preview invalidation category must contribute to request identity.");
        }
        require(buildAuthoringPreviewRequestSignature(
                    AuthoringPreviewScope::selectedZone, "zone-a",
                    AuthoringPreviewInvalidationCategory::mapping, "content-a")
                    != buildAuthoringPreviewRequestSignature(
                        AuthoringPreviewScope::selectedZone, "zone-b",
                        AuthoringPreviewInvalidationCategory::mapping, "content-a"),
                "A different selection must never have an equivalent request signature.");

        AuthoringPreviewController burstController({ 10, 40, 16, 4 });
        std::vector<AuthoringPreviewRequestIdentity> launchedIdentities;
        for (std::uint64_t edit = 0; edit <= 200; ++edit)
        {
            const auto selection = edit % 7 == 0 ? "zone-b" : "zone-a";
            const auto invalidation = edit % 5 == 0
                ? AuthoringPreviewInvalidationCategory::selection
                : AuthoringPreviewInvalidationCategory::gain;
            const auto signature = buildAuthoringPreviewRequestSignature(
                AuthoringPreviewScope::selectedZone,
                selection,
                invalidation,
                "content-" + std::to_string(edit));
            const auto request = burstController.request(
                AuthoringPreviewScope::selectedZone,
                static_cast<std::size_t>(edit + 1),
                selection,
                invalidation == AuthoringPreviewInvalidationCategory::selection
                    ? AuthoringPreviewRequestReason::selectionChanged
                    : AuthoringPreviewRequestReason::authoringChanged,
                invalidation,
                signature,
                edit);
            require(request.accepted, "Every distinct burst candidate should replace the pending candidate.");
            const auto launch = burstController.launchIfEligible(edit);
            if (launch.launched)
                launchedIdentities.push_back(launch.request.identity);
        }

        auto finalLaunch = burstController.launchIfEligible(1000);
        if (finalLaunch.launched)
            launchedIdentities.push_back(finalLaunch.request.identity);
        require(!launchedIdentities.empty(), "The maximum launch deadline must eventually launch burst work.");

        auto burstSnapshot = burstController.getSnapshot();
        require(burstSnapshot.requestedCount == 201
                    && burstSnapshot.coalescedCount >= 190
                    && burstSnapshot.launchedCount == launchedIdentities.size()
                    && burstSnapshot.launchedCount <= 6
                    && burstSnapshot.canceledCount < burstSnapshot.requestedCount
                    && burstSnapshot.maximumPendingDepth == 1
                    && burstSnapshot.pendingDepth == 1
                    && burstSnapshot.retainedCompletionRecordCount <= 16
                    && burstSnapshot.configuredMaximumLaunchDelayMicros == 40,
                "Hundreds of edits must retain one pending candidate, bounded launches, and bounded records.");

        for (std::size_t index = launchedIdentities.size(); index > 1; --index)
        {
            require(!burstController.acceptPrepared(launchedIdentities[index - 2], 5000 + index),
                    "Reordered obsolete worker completions must be rejected.");
        }
        const auto newestIdentity = burstController.getSnapshot().currentRequest.identity;
        require(burstController.acceptPrepared(newestIdentity, 9001)
                    && burstController.markActivationPending(newestIdentity)
                    && burstController.markActive(newestIdentity),
                "Only the newest burst result may become Ready and Active.");
        burstSnapshot = burstController.getSnapshot();
        require(burstSnapshot.acceptedCount == 1
                    && burstSnapshot.activationCount == 1
                    && burstSnapshot.pendingDepth == 0
                    && burstSnapshot.retainedCompletionRecordCount <= 16,
                "Newest-wins completion must preserve bounded accounting.");

        AuthoringPreviewController warmController({ 0, 0, 8, 2 });
        const auto warmSignature = buildAuthoringPreviewRequestSignature(
            AuthoringPreviewScope::selectedZone, "zone-a",
            AuthoringPreviewInvalidationCategory::gain, "gain=-3");
        const auto cold = warmController.request(
            AuthoringPreviewScope::selectedZone, 10, "zone-a",
            AuthoringPreviewRequestReason::authoringChanged,
            AuthoringPreviewInvalidationCategory::gain, warmSignature, 0);
        require(warmController.launchIfEligible(0).launched
                    && warmController.acceptPrepared(cold.request.identity, 77),
                "The first content signature should populate the bounded warm record set.");
        const auto equivalent = warmController.request(
            AuthoringPreviewScope::selectedZone, 11, "zone-a",
            AuthoringPreviewRequestReason::authoringChanged,
            AuthoringPreviewInvalidationCategory::gain, warmSignature, 1);
        require(equivalent.accepted
                    && warmController.getSnapshot().reusablePreparedBuildId == 77
                    && warmController.launchIfEligible(1).warmPreparedResultAvailable
                    && warmController.acceptPrepared(equivalent.request.identity, 77)
                    && warmController.getSnapshot().reusedPreparedCount == 1,
                "Equivalent warm content should be reusable under the new request identity.");
        const auto differentSelection = warmController.request(
            AuthoringPreviewScope::selectedZone, 12, "zone-b",
            AuthoringPreviewRequestReason::selectionChanged,
            AuthoringPreviewInvalidationCategory::selection, warmSignature, 2);
        require(differentSelection.accepted
                    && warmController.getSnapshot().reusablePreparedBuildId == 0,
                "Warm results must never cross selection identity.");

        AuthoringPreviewController directController({ 12, 40, 8, 2 });
        const auto direct = directController.request(
            AuthoringPreviewScope::selectedZone, 1, "zone-a",
            AuthoringPreviewRequestReason::explicitSelectedZoneAudition,
            AuthoringPreviewInvalidationCategory::selection, "direct-zone-a", 0);
        require(direct.accepted
                    && !directController.launchIfEligible(1, false).launched
                    && directController.launchIfEligible(1, true).launched,
                "Direct audition may bypass coalescing only when content is already prepared.");
        require(directController.cancelCurrent(),
                "A launched direct audition must accept a cancellation race.");
        require(!directController.acceptPrepared(direct.request.identity, 88),
                "A completion racing cancellation must remain rejected.");
        const auto generationBeforeReset = direct.request.identity.cancellationGeneration;
        directController.reset();
        const auto reopened = directController.request(
            AuthoringPreviewScope::selectedZone, 1, "zone-a",
            AuthoringPreviewRequestReason::projectOpened,
            AuthoringPreviewInvalidationCategory::selection, "direct-zone-a", 50);
        require(reopened.accepted
                    && reopened.request.identity.cancellationGeneration > generationBeforeReset
                    && directController.getSnapshot().reusablePreparedBuildId == 0,
                "Close/reopen must advance cancellation identity and clear warm reuse state.");

        EngineFacade cancellationFacade;
        const auto cancellationProject = loadPhase2ReferenceProjectManifest();
        require(cancellationProject.loaded
                    && cancellationFacade.replaceDraftPlaybackAuthoringProject(
                        cancellationProject.project)
                    && cancellationFacade.refreshPreviewToCurrentDraft()
                    && cancellationFacade.waitForPreparedPlaybackIdle(
                        std::chrono::milliseconds(2000)),
                "Facade cancellation coverage requires a last-known-good authored Preview payload.");
        const auto retainedPreview = cancellationFacade.getPreviewActivationPayload();
        const auto stagedCancellationRevision = cancellationFacade.stageDraftRevision(1);
        const auto requestedCancellationPreview = stagedCancellationRevision
            && cancellationFacade.refreshPreviewToCurrentDraft();
        const auto cancellationDraft = cancellationFacade.getDraftPlaybackStatus();
        if (retainedPreview == nullptr || !stagedCancellationRevision
            || !requestedCancellationPreview || !cancellationDraft.pendingPreview.active)
        {
            std::cerr << "Cancellation setup diagnostics: retained="
                      << (retainedPreview != nullptr)
                      << " retainedRevision="
                      << (retainedPreview != nullptr ? retainedPreview->revision : 0)
                      << " draftRevision=" << cancellationDraft.draftRevision
                      << " staged=" << stagedCancellationRevision
                      << " requested=" << requestedCancellationPreview
                      << " pending=" << cancellationDraft.pendingPreview.active
                      << " lastEvent=" << cancellationDraft.lastEvent << std::endl;
        }
        require(retainedPreview != nullptr
                    && stagedCancellationRevision
                    && requestedCancellationPreview
                    && cancellationDraft.pendingPreview.active,
                "Facade cancellation coverage requires a queued Preview preparation.");
        require(cancellationFacade.cancelPreviewPreparation("5.3 deterministic cancellation race")
                    && !cancellationFacade.getDraftPlaybackStatus().pendingPreview.active,
                "Canceling obsolete Preview work must clear its contract request immediately.");
        require(cancellationFacade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(2000)),
                "Canceled physical worker work must drain without retaining an obsolete completion.");
        const auto afterCancellation = cancellationFacade.getPreviewActivationPayload();
        require(afterCancellation != nullptr
                    && afterCancellation->revision == retainedPreview->revision,
                "A late canceled worker completion must not replace last-known-good Preview payload.");
        const auto cancellationWorker = cancellationFacade.getPreparedPlaybackWorkerStatus();
        require(cancellationWorker.maxPendingWorkCount
                    <= cancellationWorker.configuredMaxPendingWorkCount,
                "Facade cancellation races must preserve configured worker bounds.");

        std::cout << "Mini Sprint 5.3 bounded coalescing and cancellation matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.3 bounded coalescing and cancellation matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
