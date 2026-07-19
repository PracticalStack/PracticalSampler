#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/AuthoringPreviewPreparation.h"
#include "drs/engine/AuthoringPreviewRecovery.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SamplerPlaybackContext.h"
#include "plugin/PluginProcessor.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <array>
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

drs::engine::AuthoringPreviewRequestResult request(
    drs::engine::AuthoringPreviewController& controller,
    std::size_t revision,
    const std::string& signature,
    std::uint64_t now)
{
    return controller.request(drs::engine::AuthoringPreviewScope::selectedZone,
                              revision, "pad-a3-high",
                              drs::engine::AuthoringPreviewRequestReason::recovery,
                              drs::engine::AuthoringPreviewInvalidationCategory::sourceAssignment,
                              signature, now);
}

void runRecoveryContract()
{
    using namespace drs::engine;
    constexpr std::array<std::pair<const char*, AuthoringPreviewFailureFamily>, 7> families {{
        { "missing-sample-source-asset", AuthoringPreviewFailureFamily::missingSource },
        { "prepared-sample-format-unsupported", AuthoringPreviewFailureFamily::unsupportedFormat },
        { "invalid-zone-loop-range", AuthoringPreviewFailureFamily::invalidRange },
        { "duplicate-zone-id", AuthoringPreviewFailureFamily::routeConflict },
        { "prepared-sample-decode-failed", AuthoringPreviewFailureFamily::decodeFailure },
        { "preview-request-canceled", AuthoringPreviewFailureFamily::cancellation },
        { "preview-activation-slot-exhausted", AuthoringPreviewFailureFamily::resourcePressure }
    }};
    for (const auto& [code, family] : families)
    {
        const auto finding = classifyAuthoringPreviewFailure(code, "test.path", "test failure");
        require(finding.family == family && !finding.code.empty() && !finding.path.empty(),
                std::string("Stable Preview failure family mismatch for ") + code);
    }

    AuthoringPreviewController controller;
    const auto first = request(controller, 1, "ready-a", 0);
    require(first.accepted && controller.launchIfEligible(50000).launched
                && controller.acceptPrepared(first.request.identity, 101)
                && controller.markActivationPending(first.request.identity)
                && controller.markActive(first.request.identity),
            "Recovery contract requires an initial last-known-good activation.");

    const auto queued = request(controller, 2, "cancel-b", 10);
    require(queued.accepted && controller.cancelCurrent(),
            "A queued recovery request should be cancelable.");
    auto snapshot = controller.getSnapshot();
    require(snapshot.hasActiveRequest
                && snapshot.activeRequestIdentity.draftRevision == 1
                && snapshot.activePreparedBuildId == 101,
            "Cancellation must preserve the last-known-good request identity.");

    const auto superseded = request(controller, 3, "superseded-c", 20);
    const auto newest = request(controller, 4, "failed-d", 21);
    require(superseded.accepted && newest.accepted && newest.supersededPrevious,
            "A newer recovery request must supersede queued obsolete work.");
    const auto missing = classifyAuthoringPreviewFailure(
        "missing-sample-source-asset", "sampleSources[0].path", "Source is missing.");
    require(controller.launchIfEligible(50000).launched
                && controller.fail(newest.request.identity, missing),
            "The newest request should accept its structured failure.");
    snapshot = controller.getSnapshot();
    require(snapshot.hasActiveRequest && snapshot.activeRequestIdentity.draftRevision == 1
                && snapshot.hasFailedRequest && snapshot.failedRequestIdentity.draftRevision == 4
                && snapshot.failureFinding.family == AuthoringPreviewFailureFamily::missingSource,
            "Active, failed, and current Preview identities must remain independent.");

    const auto repaired = request(controller, 5, "repaired-e", 60000);
    require(repaired.accepted && controller.launchIfEligible(120000).launched
                && controller.acceptPrepared(repaired.request.identity, 202)
                && controller.markActivationPending(repaired.request.identity)
                && controller.markActive(repaired.request.identity),
            "A repaired request should replace last-known-good only after activation.");
    snapshot = controller.getSnapshot();
    require(snapshot.activeRequestIdentity.draftRevision == 5
                && snapshot.activePreparedBuildId == 202
                && !snapshot.hasFailedRequest && snapshot.failureFinding.code.empty(),
            "Successful repair must clear the obsolete failed identity.");
}

bool waitForState(drs::plugin::Processor& processor,
                  drs::engine::AuthoringPreviewPreparationState expected,
                  std::chrono::milliseconds timeout = std::chrono::milliseconds(10000))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        if (processor.getAuthoringPreviewControllerSnapshot().preparationState == expected)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    processor.serviceMessageThreadWork();
    return processor.getAuthoringPreviewControllerSnapshot().preparationState == expected;
}

void crossBlock(drs::plugin::Processor& processor,
                juce::AudioBuffer<float>& buffer,
                juce::MidiBuffer& midi)
{
    buffer.clear();
    processor.processBlock(buffer, midi);
    midi.clear();
    processor.serviceMessageThreadWork();
}

void makeRevision(drs::plugin::Processor& processor, double gainDb, const std::string& label)
{
    auto zone = processor.getAuthoringSession().getSelectedZone();
    require(zone.has_value(), label + " requires a selected zone.");
    zone->gainDb = gainDb;
    require(processor.getAuthoringSession().updateSelectedZone(*zone, label).applied,
            label + " should create a new authored revision.");
}

void prepareSelected(drs::plugin::Processor& processor,
                     juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     const std::string& label)
{
    require(waitForState(processor, drs::engine::AuthoringPreviewPreparationState::ready),
            label + " did not become ready; state="
                + std::to_string(static_cast<int>(
                    processor.getAuthoringPreviewControllerSnapshot().preparationState)));
    crossBlock(processor, buffer, midi);
}

void runActivationSlotPressure(
    const drs::engine::PlaybackActivationPayloadPtr& payload,
    const drs::engine::AuthoringPreviewRequest& request)
{
    using namespace drs::engine;
    const auto preparation = prepareAuthoringPreviewRenderModel(payload, request);
    require(preparation.prepared && preparation.model != nullptr,
            "Slot-pressure coverage requires a prepared Preview render model.");

    SamplerPlaybackContext context(PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "Slot-pressure Preview context should prepare.");
    std::array<float, 256> left {};
    std::array<float, 256> right {};
    std::array<float*, 2> channels { left.data(), right.data() };
    SamplerAudioBufferView output { channels.data(),
                                    static_cast<std::uint32_t>(channels.size()),
                                    static_cast<std::uint32_t>(left.size()) };
    const SamplerRenderEvent noteOn { SamplerRenderEventType::noteOn, 0, 57, 0.7f };
    const SamplerRenderEventView events { &noteOn, 1 };

    for (std::size_t slot = 0; slot < SamplerPlaybackContext::activationSlotCapacity; ++slot)
    {
        require(context.stageActivation(preparation.model),
                "Each available activation slot should accept one immutable leased model.");
        require(context.renderBlock(output, events).accepted,
                "Each staged Preview model should activate and retain a looping voice lease.");
    }
    require(!context.stageActivation(preparation.model),
            "The fixed activation pool must reject a fifth live leased model.");
    const auto finding = classifyAuthoringPreviewFailure(
        "preview-activation-slot-exhausted", "preview.activationSlots",
        "Authoring Preview activation slots are exhausted.");
    require(finding.family == AuthoringPreviewFailureFamily::resourcePressure
                && context.getSnapshot().hasActiveActivation,
            "Slot exhaustion must classify as resource pressure without clearing the active model.");
    context.closeAtBlockBoundary();
    context.serviceRetirements();
}

void runProcessorRecovery(const drs::engine::RuntimeProjectModel& sourceProject)
{
    using namespace drs::engine;
    auto project = sourceProject;
    project.authoring.selectedZoneId = "pad-a3-high";

    drs::plugin::Processor processor;
    processor.prepareToPlay(48000.0, 256);
    processor.replaceAuthoringProject(project);
    makeRevision(processor, 0.1, "Prime recovery Preview");

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    prepareSelected(processor, buffer, midi, "Initial recovery Preview");
    const auto ready = processor.getRealtimeSafetySnapshot();
    require(ready.activeAuthoringPreviewRevision != 0,
            "Recovery coverage requires an active Preview revision.");

    require(processor.getEngineFacade().publishCurrentDraft()
                && processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::milliseconds(4000)),
            "Recovery isolation requires a published Performance activation.");
    processor.serviceMessageThreadWork();
    crossBlock(processor, buffer, midi);
    processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
    crossBlock(processor, buffer, midi);
    const auto performanceIdentity = processor.getRealtimeSafetySnapshot().activePublishedRevision;

    auto invalid = processor.getAuthoringSession().getProject();
    const auto zone = processor.getAuthoringSession().getSelectedZone();
    require(zone.has_value(), "Missing-source recovery requires the selected pad zone.");
    const auto sample = std::find_if(invalid.sampleSources.begin(), invalid.sampleSources.end(),
                                     [&](const auto& item) { return item.id == zone->sampleSourceId; });
    require(sample != invalid.sampleSources.end(), "Selected sample source is missing from the fixture.");
    sample->path = invalid.contentRootPath + "/missing-sprint5-recovery.wav";
    processor.replaceAuthoringProject(invalid);
    require(waitForState(processor, AuthoringPreviewPreparationState::failed),
            "Missing source should produce a failed Preview request.");

    const auto failedController = processor.getAuthoringPreviewControllerSnapshot();
    const auto failedStatus = processor.getAuthoringPreviewStatusSnapshot();
    require(failedController.hasActiveRequest && failedController.hasFailedRequest
                && failedController.failureFinding.family == AuthoringPreviewFailureFamily::missingSource
                && failedStatus.usingLastKnownGood
                && failedStatus.audibleRevision == ready.activeAuthoringPreviewRevision
                && failedStatus.failedRevision == failedController.failedRequestIdentity.draftRevision,
            "Failed status must identify both the requested failure and audible last-known-good revision.");

    processor.queueAuthoringPreviewNoteOn(57, 0.7f);
    crossBlock(processor, buffer, midi);
    require(buffer.getMagnitude(0, buffer.getNumSamples()) > 0.0001f,
            "A failed repair should leave last-known-good Preview audible.");
    require(processor.getRealtimeSafetySnapshot().activePublishedRevision == performanceIdentity,
            "Preview failure must not change Performance identity.");

    processor.replaceAuthoringProject(project);
    makeRevision(processor, 0.2, "Repair missing Preview source");
    prepareSelected(processor, buffer, midi, "Repaired Preview");
    const auto repairedStatus = processor.getAuthoringPreviewStatusSnapshot();
    require(!repairedStatus.usingLastKnownGood && repairedStatus.failedRevision == 0
                && repairedStatus.activeRevision == repairedStatus.draftRevision,
            "A corrected source must activate cleanly and clear obsolete failure identity.");

    const auto repairedController = processor.getAuthoringPreviewControllerSnapshot();
    require(repairedController.hasRequest,
            "Slot-pressure coverage requires the repaired Preview request identity.");
    runActivationSlotPressure(processor.getEngineFacade().getPreviewActivationPayload(),
                              repairedController.currentRequest);
    require(processor.getRealtimeSafetySnapshot().activePublishedRevision == performanceIdentity,
            "Independent Preview slot pressure must not change Performance.");

    RuntimeProjectModel unloaded;
    unloaded.schemaName = "drs.project";
    unloaded.schemaVersion = 2;
    unloaded.displayName = "No Project Loaded";
    unloaded.authoring.schemaName = "drs.authoring";
    unloaded.authoring.schemaVersion = 1;
    processor.closeAuthoringProject(unloaded);
    crossBlock(processor, buffer, midi);
    processor.serviceMessageThreadWork();
    const auto closed = processor.getRealtimeSafetySnapshot();
    require(closed.activeAuthoringPreviewRevision == 0
                && closed.authoringPreviewActiveVoiceCount == 0
                && processor.getEngineFacade().getDraftPlaybackStatus().projectOpen == false
                && closed.activePublishedRevision == performanceIdentity,
            "Project close must clear Preview at the audio boundary without touching Performance.");

    processor.queueAuthoringPreviewNoteOn(57, 0.7f);
    crossBlock(processor, buffer, midi);
    const auto closedAudition = processor.getRealtimeSafetySnapshot();
    require(closedAudition.authoringPreviewActiveVoiceCount == 0
                && closedAudition.performanceActiveVoiceCount == closed.performanceActiveVoiceCount,
            "Audition without a usable Preview activation must stay silent in Preview and not fall through to Performance.");

    processor.replaceAuthoringProject(project);
    makeRevision(processor, 0.25, "Reopen repaired Preview project");
    prepareSelected(processor, buffer, midi, "Reopened Preview");
    require(processor.getAuthoringPreviewStatusSnapshot().auditionAvailable
                && processor.getRealtimeSafetySnapshot().retiredActivationBacklog == 0
                && processor.getRealtimeSafetySnapshot().activePublishedRevision == performanceIdentity,
            "Reopen must recover Preview and reclaim closed-project payloads without Performance changes.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 5.6 requires the authored reference project.");
        runRecoveryContract();
        runProcessorRecovery(loaded.project);
        std::cout << "Mini Sprint 5.6 last-known-good and recovery matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.6 recovery matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
