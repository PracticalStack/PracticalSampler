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
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForPreviewState(drs::plugin::Processor& processor,
                         drs::engine::AuthoringPreviewPreparationState expected)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        if (processor.getAuthoringPreviewControllerSnapshot().preparationState == expected)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto controller = processor.getAuthoringPreviewControllerSnapshot();
    const auto& draft = processor.getEngineFacade().getDraftPlaybackStatus();
    const auto worker = processor.getEngineFacade().getPreparedPlaybackWorkerStatus();
    std::cerr << "Preview deadline diagnostics: preparation="
              << static_cast<int>(controller.preparationState)
              << " activation=" << static_cast<int>(controller.activationState)
              << " hasRequest=" << controller.hasRequest
              << " failed=" << controller.hasFailedRequest
              << " draftPending=" << draft.pendingPreview.active
              << " draftState=" << draft.preview.state
              << " draftFindings=" << draft.preview.findings.size()
              << " failureCode=" << controller.failureFinding.code
              << " failurePath=" << controller.failureFinding.path
              << " failureMessage=" << controller.failureFinding.message
              << " workerPending=" << worker.pendingWorkCount
              << " workerRunning=" << worker.inFlightWorkCount
              << " workerLast=" << worker.lastEvent << std::endl;
    return false;
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        const auto project = loadPhase2ReferenceProjectManifest();
        require(project.loaded, "Preview controller integration requires the authored reference project.");

        drs::plugin::Processor processor;
        processor.prepareToPlay(48000.0, 256);
        processor.replaceAuthoringProject(project.project);

        const auto initial = processor.getAuthoringPreviewControllerSnapshot();
        require(initial.hasRequest
                    && initial.currentRequest.identity.scope == AuthoringPreviewScope::selectedZone,
                "Opening an authored project should create a typed selected-zone Preview request.");

        require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
                "Integration coverage should select a replacement Preview zone.");
        require(processor.serviceMessageThreadWork(),
                "Selection observation should be serviced through the Preview controller.");
        const auto coalescing = processor.getAuthoringPreviewControllerSnapshot();
        require(coalescing.preparationState == AuthoringPreviewPreparationState::queued,
                "Selection edits should remain queued during the bounded coalescing window.");
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        require(waitForPreviewState(processor, AuthoringPreviewPreparationState::ready),
                "Selection Preview preparation should settle through the asynchronous worker.");
        const auto selected = processor.getAuthoringPreviewControllerSnapshot();
        require(selected.hasRequest
                    && selected.currentRequest.identity.requestId > initial.currentRequest.identity.requestId
                    && selected.currentRequest.identity.selectedZoneId == "pad-a3-high"
                    && selected.currentRequest.reason == AuthoringPreviewRequestReason::selectionChanged
                    && selected.preparationState == AuthoringPreviewPreparationState::ready,
                "Selection changes should create a newer typed request and accept only its prepared result.");

        crossBlockBoundary(processor);
        const auto active = processor.getAuthoringPreviewControllerSnapshot();
        require(active.activationState == AuthoringPreviewActivationState::active
                    && active.acceptedPreparedBuildId != 0
                    && active.activationCount >= 1,
                "The controller should reconcile block-boundary Preview activation.");

        const auto performanceBefore = processor.getRealtimeSafetySnapshot().activePublishedRevision;
        processor.queueAuthoringPreviewNoteOn(60, 0.75f);
        crossBlockBoundary(processor);
        const auto diagnostics = processor.getRealtimeSafetySnapshot();
        require(diagnostics.authoringPreviewActiveVoiceCount > 0
                    && diagnostics.performanceActiveVoiceCount == 0
                    && diagnostics.activePublishedRevision == performanceBefore,
                "Controller-driven Preview audition must remain isolated from Performance.");

        const auto duplicateCount = active.requestedCount;
        processor.serviceMessageThreadWork();
        require(processor.getAuthoringPreviewControllerSnapshot().requestedCount == duplicateCount,
                "Unchanged message servicing must not create duplicate Preview requests.");

        const auto worker = processor.getEngineFacade().getPreparedPlaybackWorkerStatus();
        require(worker.maxPendingWorkCount <= worker.configuredMaxPendingWorkCount,
                "Controller-driven Preview work must preserve the facade worker queue bound.");

        std::cout << "Mini Sprint 5.2/5.3 processor/controller integration matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.2 processor/controller integration matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
