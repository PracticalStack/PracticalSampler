#include "drs/engine/AuthoringPreviewCommandAdapter.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "shared/authoring/ZoneMapCanvas.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
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

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantById(*root.getChildComponent(index), id))
            return match;
    return nullptr;
}

bool waitForPreviewReady(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        if (processor.getAuthoringPreviewControllerSnapshot().preparationState
            == drs::engine::AuthoringPreviewPreparationState::ready)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
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

float magnitude(const juce::AudioBuffer<float>& buffer, int start, int length)
{
    auto result = 0.0f;
    for (auto channel = 0; channel < buffer.getNumChannels(); ++channel)
        result = std::max(result, buffer.getMagnitude(channel, start, length));
    return result;
}

drs::engine::AuthoringPreviewCommand makeCommand(
    drs::engine::AuthoringPreviewCommandType type,
    drs::engine::AuthoringPreviewAuditionSource source,
    int note = 60,
    float velocity = 0.75f,
    std::uint32_t sampleOffset = 0)
{
    drs::engine::AuthoringPreviewCommand command;
    command.type = type;
    command.source = source;
    command.midiNote = note;
    command.velocity = velocity;
    command.sampleOffset = sampleOffset;
    return command;
}

void runCommandOwnershipContract()
{
    using namespace drs::engine;
    AuthoringPreviewCommandAdapter adapter;

    auto missingSelection = makeCommand(AuthoringPreviewCommandType::auditionSelectedZone,
                                        AuthoringPreviewAuditionSource::summaryPreview);
    require(!adapter.dispatch(missingSelection).accepted,
            "Selected-zone audition must require an explicit selection identity.");

    auto missingGroup = makeCommand(AuthoringPreviewCommandType::auditionSelectedGroup,
                                    AuthoringPreviewAuditionSource::summaryPreview);
    require(!adapter.dispatch(missingGroup).accepted,
            "Selected-group audition must require an explicit group identity.");

    auto selectedGroup = makeCommand(AuthoringPreviewCommandType::auditionSelectedGroup,
                                     AuthoringPreviewAuditionSource::inspector);
    selectedGroup.emitNote = false;
    selectedGroup.selectedGroupId = "pad-core";
    selectedGroup.selectedZoneId = "pad-a3-low";
    const auto groupDispatch = adapter.dispatch(selectedGroup);
    require(groupDispatch.accepted && groupDispatch.preparationRequested
                && groupDispatch.requestedScope == AuthoringPreviewScope::selectedGroup
                && !groupDispatch.hasEvent,
            "Selected-group audition must request group-scoped preparation without inventing a note event.");

    auto currentDraft = makeCommand(AuthoringPreviewCommandType::auditionCurrentDraft,
                                    AuthoringPreviewAuditionSource::inspector);
    currentDraft.emitNote = false;
    const auto currentDispatch = adapter.dispatch(currentDraft);
    require(currentDispatch.accepted && currentDispatch.preparationRequested
                && currentDispatch.requestedScope == AuthoringPreviewScope::currentDraft
                && !currentDispatch.hasEvent,
            "Current-draft audition must request preparation without inventing a note event.");

    auto ranged = makeCommand(AuthoringPreviewCommandType::auditionSelectedZone,
                              AuthoringPreviewAuditionSource::inspector);
    ranged.selectedZoneId = "pad-a3-high";
    ranged.hasAuditionRegion = true;
    ranged.auditionStartFrame = 120;
    ranged.auditionEndFrameExclusive = 240;
    ranged.hasAuditionInitialFrame = true;
    ranged.auditionInitialFrame = 190;
    ranged.auditionLoopEnabled = true;
    ranged.auditionLoopStartFrame = 160;
    ranged.auditionLoopEndFrameExclusive = 200;
    const auto rangedDispatch = adapter.dispatch(ranged);
    require(rangedDispatch.accepted && rangedDispatch.hasEvent
                && rangedDispatch.event.hasAuditionRegion
                && rangedDispatch.event.auditionStartFrame == 120
                && rangedDispatch.event.auditionEndFrameExclusive == 240
                && rangedDispatch.event.hasAuditionInitialFrame
                && rangedDispatch.event.auditionInitialFrame == 190
                && rangedDispatch.event.auditionLoopEnabled,
            "A temporary audition range and seam-focused initial frame must survive Preview command adaptation without changing project state.");
    auto invalidRange = ranged;
    invalidRange.auditionLoopEndFrameExclusive = 300;
    require(!adapter.dispatch(invalidRange).accepted,
            "Preview adaptation must reject a temporary loop outside its audition range.");
    invalidRange = ranged;
    invalidRange.auditionInitialFrame = 300;
    require(!adapter.dispatch(invalidRange).accepted,
            "Preview adaptation must reject a temporary initial frame outside its audition range.");
    require(adapter.dispatch(makeCommand(AuthoringPreviewCommandType::noteOff,
                                         AuthoringPreviewAuditionSource::inspector,
                                         ranged.midiNote)).accepted,
            "Temporary ranged audition coverage must release its owned Preview note.");

    constexpr std::array sources {
        AuthoringPreviewAuditionSource::summaryPreview,
        AuthoringPreviewAuditionSource::authoringKeyboard,
        AuthoringPreviewAuditionSource::zoneMap,
        AuthoringPreviewAuditionSource::inspector
    };
    for (const auto source : sources)
        require(adapter.dispatch(makeCommand(AuthoringPreviewCommandType::noteOn, source, 64)).hasEvent,
                "Every creator audition source must enter the same note-on adapter.");
    require(adapter.getSnapshot().ownedNoteCount == 4
                && adapter.getSnapshot().distinctOwnedNoteCount == 1,
            "Cross-source ownership must retain every owner of a repeated MIDI note.");

    for (std::size_t index = 0; index < sources.size(); ++index)
    {
        const auto released = adapter.dispatch(
            makeCommand(AuthoringPreviewCommandType::noteOff, sources[index], 64));
        require(released.accepted && released.hasEvent == (index + 1 == sources.size()),
                "Only the final owner may emit the shared note-off event.");
    }

    auto repeated = makeCommand(AuthoringPreviewCommandType::noteOn,
                                AuthoringPreviewAuditionSource::authoringKeyboard, 67);
    adapter.dispatch(repeated);
    adapter.dispatch(repeated);
    const auto firstRepeatedOff = adapter.dispatch(
        makeCommand(AuthoringPreviewCommandType::noteOff,
                    AuthoringPreviewAuditionSource::authoringKeyboard, 67));
    require(firstRepeatedOff.accepted && !firstRepeatedOff.hasEvent
                && adapter.getSnapshot().ownedNoteCount == 1,
            "Repeated-note ownership must survive the first matching note-off.");
    require(adapter.dispatch(makeCommand(AuthoringPreviewCommandType::stopAll,
                                         AuthoringPreviewAuditionSource::authoringKeyboard,
                                         0, 0.0f, 31)).event.type
                == AuthoringPreviewEventType::allNotesOff
                && adapter.getSnapshot().ownedNoteCount == 0,
            "Stop-all must recover missed note-offs and clear ownership.");
    require(adapter.dispatch(makeCommand(AuthoringPreviewCommandType::emergencyReset,
                                         AuthoringPreviewAuditionSource::inspector,
                                         0, 0.0f, 47)).event.type
                == AuthoringPreviewEventType::reset,
            "Emergency reset must be a distinct Preview command.");
}

void prepareAuthoredPreview(drs::plugin::Processor& processor,
                            const drs::engine::RuntimeProjectModel& project)
{
    processor.prepareToPlay(48000.0, 256);
    processor.replaceAuthoringProject(project);
    require(processor.getAuthoringSession().selectZone("pad-a3-high").applied,
            "Preview audition coverage requires the looping pad zone.");
    processor.serviceMessageThreadWork();
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    require(waitForPreviewReady(processor), "Authored Preview preparation did not settle.");
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    crossBlock(processor, buffer, midi);
}

void verifyReleaseEditRebuildsProjectPreview(
    const drs::engine::RuntimeProjectModel& sourceProject)
{
    using namespace drs::engine;
    auto project = sourceProject;
    const auto authoredZone = std::find_if(project.authoring.zones.begin(),
                                           project.authoring.zones.end(),
                                           [](const auto& zone) { return zone.id == "pad-a3-high"; });
    require(authoredZone != project.authoring.zones.end(),
            "Release invalidation coverage requires the looping pad zone.");
    authoredZone->releaseSeconds = 1.0;
    authoredZone->releaseShape = sfzDefaultReleaseShape;

    drs::plugin::Processor processor;
    prepareAuthoredPreview(processor, project);
    const auto initialController = processor.getAuthoringPreviewControllerSnapshot();
    require(initialController.hasRequest && !initialController.currentRequest.requestSignature.empty(),
            "Release invalidation coverage requires an initial selected-zone Preview signature.");
    const auto initialSignature = initialController.currentRequest.requestSignature;

    auto editedZone = processor.getAuthoringSession().getSelectedZone();
    require(editedZone.has_value(),
            "Release invalidation coverage requires the selected authored zone.");
    editedZone->releaseSeconds = 0.005;
    editedZone->releaseShape = 0.0;
    const auto edit = processor.getAuthoringSession().updateSelectedZone(
        *editedZone, "Update zone release");
    require(edit.applied, "The project-mode release edit must commit before Preview refresh.");
    const auto editedRevision = processor.getAuthoringSession().getDocumentState().revision;

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    auto activatedEditedRelease = false;
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        crossBlock(processor, buffer, midi);
        const auto controller = processor.getAuthoringPreviewControllerSnapshot();
        if (controller.hasActiveRequest
            && controller.activeRequestIdentity.draftRevision == editedRevision
            && controller.currentRequest.requestSignature != initialSignature)
        {
            require(controller.currentRequest.invalidationCategory
                        == AuthoringPreviewInvalidationCategory::release,
                    "A release edit must be classified as a release-law invalidation.");
            activatedEditedRelease = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(activatedEditedRelease,
            "Project-mode Preview reused a stale prepared model after release seconds/shape changed.");

    processor.queueAuthoringPreviewNoteOn(57, 0.8f);
    crossBlock(processor, buffer, midi);
    require(processor.getRealtimeSafetySnapshot().authoringPreviewActiveVoiceCount == 1,
            "The rebuilt project Preview must start the edited zone.");
    processor.queueAuthoringPreviewNoteOff(57);
    crossBlock(processor, buffer, midi);
    require(processor.getRealtimeSafetySnapshot().authoringPreviewActiveVoiceCount == 0,
            "The rebuilt project Preview must use the edited 5 ms Linear release law.");
}

void runProcessorTimingAndIsolation(const drs::engine::RuntimeProjectModel& project)
{
    using namespace drs::engine;
    drs::plugin::Processor processor;
    prepareAuthoredPreview(processor, project);

    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    auto boundedAudition = makeCommand(AuthoringPreviewCommandType::noteOn,
                                       AuthoringPreviewAuditionSource::inspector,
                                       57, 0.8f);
    boundedAudition.hasAuditionRegion = true;
    boundedAudition.auditionStartFrame = 0;
    boundedAudition.auditionEndFrameExclusive = 4;
    require(processor.submitAuthoringPreviewCommand(boundedAudition),
            "A bounded selection audition should enter the Preview event queue.");
    crossBlock(processor, buffer, midi);
    require(processor.getRealtimeSafetySnapshot().authoringPreviewActiveVoiceCount == 0
                && processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 1,
            "A four-frame Preview override must cross the realtime queue and finish inside one audio block.");
    require(processor.submitAuthoringPreviewCommand(
                makeCommand(AuthoringPreviewCommandType::noteOff,
                            AuthoringPreviewAuditionSource::inspector, 57)),
            "A naturally finished selection audition must still release command ownership.");
    crossBlock(processor, buffer, midi);

    auto exact = makeCommand(AuthoringPreviewCommandType::noteOn,
                             AuthoringPreviewAuditionSource::authoringKeyboard,
                             57, 0.8f, 73);
    require(processor.submitAuthoringPreviewCommand(exact),
            "Exact-offset Preview note-on should enter the command queue.");
    crossBlock(processor, buffer, midi);
    require(magnitude(buffer, 0, 73) <= 0.000001f
                && magnitude(buffer, 73, buffer.getNumSamples() - 73) > 0.0001f,
            "Preview note-on must retain its exact sample offset.");

    require(processor.submitAuthoringPreviewCommand(exact)
                && processor.submitAuthoringPreviewCommand(
                    makeCommand(AuthoringPreviewCommandType::noteOff,
                                AuthoringPreviewAuditionSource::authoringKeyboard, 57)),
            "Repeated note coverage should accept a second owner and first release.");
    crossBlock(processor, buffer, midi);
    require(processor.getRealtimeSafetySnapshot().authoringPreviewActiveVoiceCount >= 2
                && processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 1,
            "First repeated note-off must not release voices while one owner remains.");
    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::noteOff,
                    AuthoringPreviewAuditionSource::authoringKeyboard, 57));
    crossBlock(processor, buffer, midi);
    require(processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 0,
            "Final repeated note-off must clear note ownership.");

    require(processor.getEngineFacade().publishCurrentDraft(),
            "Isolation coverage requires a published Performance payload.");
    require(processor.getEngineFacade().waitForPreparedPlaybackIdle(std::chrono::milliseconds(3000)),
            "Performance preparation did not settle.");
    processor.serviceMessageThreadWork();
    crossBlock(processor, buffer, midi);
    const auto performancePayload = processor.getEngineFacade().getPerformanceActivationPayload();
    require(performancePayload != nullptr, "Performance isolation payload is missing.");

    processor.queuePerformanceSurfaceNoteOn(57, 0.8f);
    midi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 11);
    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::noteOn,
                    AuthoringPreviewAuditionSource::summaryPreview, 57));
    crossBlock(processor, buffer, midi);
    auto beforeStop = processor.getRealtimeSafetySnapshot();
    require(beforeStop.performanceActiveVoiceCount >= 2
                && beforeStop.authoringPreviewActiveVoiceCount >= 1,
            "Same-note cross-lane coverage must populate both isolated voice pools.");

    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::stopAll,
                    AuthoringPreviewAuditionSource::summaryPreview, 0, 0.0f, 19));
    crossBlock(processor, buffer, midi);
    auto afterStop = processor.getRealtimeSafetySnapshot();
    require(afterStop.performanceActiveVoiceCount >= 2
                && afterStop.activePublishedRevision == performancePayload->revision
                && afterStop.activePreparedBuildId == performancePayload->preparedBuildId
                && processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 0,
            "Preview stop must not release Performance voices or change Performance identity.");

    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::noteOn,
                    AuthoringPreviewAuditionSource::inspector, 60));
    crossBlock(processor, buffer, midi);
    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::emergencyReset,
                    AuthoringPreviewAuditionSource::inspector, 0, 0.0f, 23));
    crossBlock(processor, buffer, midi);
    const auto afterReset = processor.getRealtimeSafetySnapshot();
    require(afterReset.authoringPreviewActiveVoiceCount == 0
                && afterReset.performanceActiveVoiceCount >= 2
                && afterReset.activePublishedRevision == performancePayload->revision,
            "Preview emergency reset must be lane-local and leave Performance audible.");

    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::noteOn,
                    AuthoringPreviewAuditionSource::zoneMap, 57));
    crossBlock(processor, buffer, midi);
    require(processor.getAuthoringSession().selectZone("lead-a4-sustain").applied,
            "Selection-policy coverage requires a replacement selected zone.");
    processor.serviceMessageThreadWork();
    std::this_thread::sleep_for(std::chrono::milliseconds(15));
    if (!waitForPreviewReady(processor))
    {
        const auto controller = processor.getAuthoringPreviewControllerSnapshot();
        const auto draft = processor.getEngineFacade().getDraftPlaybackStatus();
        throw std::runtime_error(
            "Replacement selected-zone Preview did not settle: controller="
            + std::to_string(static_cast<int>(controller.preparationState))
            + " activation=" + std::to_string(static_cast<int>(controller.activationState))
            + " pending=" + std::to_string(draft.pendingPreview.active)
            + " preview=" + draft.preview.state
            + " event=" + draft.lastEvent
            + " failure=" + controller.failureState);
    }
    crossBlock(processor, buffer, midi);
    require(processor.getRealtimeSafetySnapshot().authoringPreviewActiveVoiceCount >= 1,
            "Selection/activation replacement must let old-model Preview voices continue.");
    processor.submitAuthoringPreviewCommand(
        makeCommand(AuthoringPreviewCommandType::noteOff,
                    AuthoringPreviewAuditionSource::zoneMap, 57));
    crossBlock(processor, buffer, midi);
}

void runUiSourceAndEditorCloseCoverage(const drs::engine::RuntimeProjectModel& project)
{
    using namespace drs::engine;
    drs::plugin::Processor processor;
    prepareAuthoredPreview(processor, project);

    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        editor->setSize(1180, 760);
        auto* tabs = dynamic_cast<juce::TabbedComponent*>(
            findDescendantById(*editor, "workspaceTabs"));
        require(tabs != nullptr, "Editor-close coverage requires the workspace tabs.");
        tabs->setCurrentTabIndex(1);

        auto* summary = dynamic_cast<juce::Button*>(
            findDescendantById(*editor, "authoringPreviewButton"));
        auto* inspector = dynamic_cast<juce::Button*>(
            findDescendantById(*editor, "authoringInspectorPreviewButton"));
        auto* zoneMap = dynamic_cast<drs::app::authoring::ZoneMapCanvas*>(
            findDescendantById(*editor, "authoringZoneMap"));
        require(summary != nullptr && inspector != nullptr && zoneMap != nullptr,
                "All 5.5 authoring audition surfaces must be present in the editor (summary="
                    + std::to_string(summary != nullptr) + ", inspector="
                    + std::to_string(inspector != nullptr) + ", zoneMap="
                    + std::to_string(zoneMap != nullptr) + ").");

        summary->onClick();
        inspector->onClick();
        auto zoneMapAuditioned = false;
        for (auto y = 12; y < zoneMap->getHeight() - 12 && !zoneMapAuditioned; y += 8)
            for (auto x = 12; x < zoneMap->getWidth() - 12 && !zoneMapAuditioned; x += 8)
                zoneMapAuditioned = zoneMap->requestAuditionAt({ static_cast<float>(x),
                                                                 static_cast<float>(y) });
        require(zoneMapAuditioned,
                "A zone-map audition gesture must enter the shared command callback.");

        processor.queueAuthoringPreviewNoteOn(57, 0.7f);
        const auto owned = processor.getAuthoringPreviewCommandSnapshot();
        require(owned.ownedNoteCount >= 4,
                "Summary, keyboard, zone-map, and inspector must all own notes through one adapter.");
        processor.queueAuthoringPreviewNoteOff(57);
    }

    require(processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 0,
            "Editor teardown must release outstanding timed UI audition ownership.");
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    crossBlock(processor, buffer, midi);
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 5.5 requires the authored reference project.");
        runCommandOwnershipContract();
        verifyReleaseEditRebuildsProjectPreview(loaded.project);
        runProcessorTimingAndIsolation(loaded.project);
        runUiSourceAndEditorCloseCoverage(loaded.project);
        std::cout << "Mini Sprint 5.5 audition command and Preview-only routing matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.5 Preview audition matrix failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
