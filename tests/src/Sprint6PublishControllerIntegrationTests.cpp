#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <chrono>
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

void crossBlockBoundary(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForActivePublish(drs::plugin::Processor& processor,
                          std::size_t expectedRevision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        crossBlockBoundary(processor);
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        if (controller.hasActiveRequest
            && controller.activationState == drs::engine::PerformancePublishActivationState::active
            && controller.activeRequestIdentity.draftRevision == expectedRevision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

bool waitForPublishRequest(drs::plugin::Processor& processor,
                           std::size_t expectedRevision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        const auto controller = processor.getPerformancePublishControllerSnapshot();
        if (controller.hasRequest
            && controller.currentRequest.identity.draftRevision == expectedRevision
            && !controller.currentRequest.identity.authoredContentDigest.empty()
            && !controller.currentRequest.identity.macroSchemaDigest.empty())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        const auto project = loadPhase2ReferenceProjectManifest();
        require(project.loaded, "Publish controller integration requires the authored reference project.");

        drs::plugin::Processor processor;
        processor.prepareToPlay(48000.0, 256);
        processor.replaceAuthoringProject(project.project);
        processor.serviceMessageThreadWork();

        const auto draftRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(processor.getEngineFacade().publishCurrentDraft(),
                "The compatibility facade command must enter the typed Publish controller.");
        const auto queued = processor.getEngineFacade().getDraftPlaybackStatus();
        require(queued.pendingPerformance.active
                    && queued.pendingPerformance.requestedRevision == draftRevision,
                "Facade Publish must queue immutable snapshot work before controller identity exists.");
        require(waitForPublishRequest(processor, draftRevision),
                "The snapshot worker must deliver complete Publish identity to the controller.");
        const auto preparing = processor.getPerformancePublishControllerSnapshot();
        require(preparing.hasRequest
                    && preparing.currentRequest.identity.draftRevision == draftRevision
                    && !preparing.currentRequest.identity.authoredContentDigest.empty()
                    && !preparing.currentRequest.identity.macroSchemaDigest.empty()
                    && (preparing.preparationState == PerformancePublishPreparationState::preparing
                        || preparing.preparationState == PerformancePublishPreparationState::ready),
                "Worker completion must capture complete identity and launch through the controller.");

        require(waitForActivePublish(processor, draftRevision),
                "Worker completion and existing block-boundary activation must reconcile to Active.");
        const auto firstActive = processor.getPerformancePublishControllerSnapshot();
        const auto firstPayload = processor.getEngineFacade().getPerformanceActivationPayload();
        const auto firstContext = processor.getRealtimeSafetySnapshot();
        require(firstPayload != nullptr
                    && firstActive.activePreparedBuildId == firstPayload->preparedBuildId
                    && firstActive.activeRequestIdentity.draftRevision == firstPayload->revision
                    && firstContext.activePublishedRevision == firstPayload->revision,
                "Controller, facade payload, processor context, and diagnostics must agree exactly.");

        const auto requestedBeforeDuplicate = firstActive.requestedCount;
        require(processor.getEngineFacade().publishCurrentDraft(),
                "An exact active Publish duplicate should be accepted as a no-op.");
        const auto duplicate = processor.getPerformancePublishControllerSnapshot();
        require(duplicate.requestedCount == requestedBeforeDuplicate
                    && duplicate.duplicateSuppressedCount == firstActive.duplicateSuppressedCount + 1
                    && duplicate.activeRequestIdentity == firstActive.activeRequestIdentity,
                "Exact duplicate publication must not build, restage, or change active identity.");

        require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
                "Integration coverage requires an authored zone edit.");
        auto zone = processor.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "The edited zone must remain selected.");
        zone->gainDb -= 1.0;
        require(processor.getAuthoringSession().updateSelectedZone(*zone, "Sprint 6 controller edit").applied,
                "The authored edit must advance the draft revision.");
        processor.serviceMessageThreadWork();
        const auto editedRevision = processor.getAuthoringSession().getDocumentState().revision;
        require(editedRevision > draftRevision
                    && processor.getEngineFacade().publishCurrentDraft(),
                "A changed captured draft must create a newer explicit Publish request.");
        const auto newerQueued = processor.getEngineFacade().getDraftPlaybackStatus();
        require(newerQueued.pendingPerformance.active
                    && newerQueued.pendingPerformance.requestedRevision == editedRevision,
                "A changed draft Publish must queue the newest immutable revision.");
        require(waitForPublishRequest(processor, editedRevision),
                "The newer snapshot worker completion must reach the Publish controller.");
        const auto newerPreparing = processor.getPerformancePublishControllerSnapshot();
        require(newerPreparing.currentRequest.identity.requestId
                    > firstActive.activeRequestIdentity.requestId
                    && newerPreparing.currentRequest.identity.draftRevision == editedRevision
                    && newerPreparing.hasActiveRequest
                    && newerPreparing.activeRequestIdentity == firstActive.activeRequestIdentity
                    && newerPreparing.activePreparedBuildId == firstActive.activePreparedBuildId,
                "New preparation must keep exact last-known-good active identity independent.");

        require(waitForActivePublish(processor, editedRevision),
                "The newer exact request must become active through the unchanged activation path.");
        const auto secondActive = processor.getPerformancePublishControllerSnapshot();
        require(secondActive.activationCount == firstActive.activationCount + 1
                    && secondActive.activeRequestIdentity.draftRevision == editedRevision
                    && secondActive.activePreparedBuildId != firstActive.activePreparedBuildId,
                "A successful newer Publish must advance controller activation exactly once.");

        processor.getEngineFacade().closeDraftPlaybackProject();
        const auto closed = processor.getPerformancePublishControllerSnapshot();
        require(!closed.hasRequest && !closed.hasActiveRequest
                    && closed.preparationState == PerformancePublishPreparationState::idle,
                "Project close must clear controller request/active identity and invalidate old completions.");

        std::cout << "Mini Sprint 6.2 facade/processor Publish controller integration matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.2 Publish controller integration matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
