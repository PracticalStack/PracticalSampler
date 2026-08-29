#include "drs/engine/AuthoringSession.h"
#include "drs/engine/SfzImportProjection.h"
#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "shared/MessageThreadMetrics.h"
#include "shared/PerformancePanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::uint64_t elapsedMicros(const Clock::time_point startedAt)
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - startedAt).count());
}

drs::engine::RuntimeProjectModel makeBlankProject(const fs::path& sfzPath)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "qualification.ui-responsiveness-salamander";
    project.displayName = "UI Responsiveness Salamander Baseline";
    project.contentRootPath = sfzPath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (sfzPath.parent_path() / "ui-baseline.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}

juce::Component* findById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findById(*root.getChildComponent(index), id))
            return match;
    }
    return nullptr;
}

template <typename ComponentType>
ComponentType* findByType(juce::Component& root)
{
    if (auto* match = dynamic_cast<ComponentType*>(&root))
        return match;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findByType<ComponentType>(*root.getChildComponent(index)))
            return match;
    }
    return nullptr;
}

double maximumFor(const std::vector<drs::app::MessageThreadSpanStatistics>& statistics,
                  const drs::app::MessageThreadSpanKind kind)
{
    for (const auto& span : statistics)
    {
        if (span.kind == kind)
            return span.maximumMilliseconds;
    }
    return 0.0;
}

std::uint64_t observationsFor(
    const std::vector<drs::app::MessageThreadSpanStatistics>& statistics,
    const drs::app::MessageThreadSpanKind kind)
{
    for (const auto& span : statistics)
    {
        if (span.kind == kind)
            return span.observationCount;
    }
    return 0;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        require(argc >= 4,
                "Usage: drs_ui_responsiveness_baseline <salamander.sfz> <valid.drpkg> <report.json>");
        const auto sfzPath = fs::absolute(fs::path(argv[1]));
        const auto packagePath = fs::absolute(fs::path(argv[2]));
        const auto reportPath = fs::absolute(fs::path(argv[3]));
        require(fs::is_regular_file(sfzPath), "Salamander SFZ fixture is missing.");
        require(fs::is_regular_file(packagePath), "Playable-package fixture is missing.");

        juce::ScopedJuceInitialiser_GUI gui;

        const auto blankProject = makeBlankProject(sfzPath);
        const auto projection = drs::engine::projectSfzImportDocument(
            blankProject, sfzPath.generic_string());
        require(projection.projected && projection.playable && projection.zones.size() == 1700,
                "Salamander projection did not produce the qualified 1,700-zone workspace.");

        drs::engine::AuthoringSession session(blankProject);
        require(drs::engine::applySfzImportProjection(
                    session, projection, "Load UI responsiveness baseline").applied,
                "Salamander projection could not be applied to the authoring session.");

        drs::app::AuthoringPanel authoringPanel(session);
        authoringPanel.setSize(1440, 980);
        drs::app::MessageThreadMetrics::resetForTests();

        const auto authoringRefreshStarted = Clock::now();
        authoringPanel.refreshNow();
        const auto authoringRefreshMicros = elapsedMicros(authoringRefreshStarted);

        auto* zoneSelector = dynamic_cast<juce::ComboBox*>(
            findById(authoringPanel, "authoringZoneSelector"));
        require(zoneSelector != nullptr && zoneSelector->getNumItems() >= 2,
                "Authoring UI did not expose the Salamander zone selector.");
        const auto documentBeforeSelection = session.getDocumentState();
        const auto workspaceRevisionBeforeSelection = session.getWorkspaceSelectionRevision();
        const auto persistedZoneBeforeSelection = session.getProject().authoring.selectedZoneId;
        const auto persistedGroupBeforeSelection = session.getProject().authoring.selectedGroupId;
        const auto zoneSelectionStarted = Clock::now();
        zoneSelector->setSelectedId(2, juce::sendNotificationSync);
        const auto zoneSelectionMicros = elapsedMicros(zoneSelectionStarted);
        const auto documentAfterSelection = session.getDocumentState();
        require(zoneSelectionMicros < 16'667,
                "Large-project zone selection exceeded one 60 Hz frame: "
                    + std::to_string(zoneSelectionMicros) + " us.");
        require(documentAfterSelection.revision == documentBeforeSelection.revision
                    && documentAfterSelection.savedRevision == documentBeforeSelection.savedRevision
                    && documentAfterSelection.dirty == documentBeforeSelection.dirty
                    && documentAfterSelection.undoDepth == documentBeforeSelection.undoDepth
                    && documentAfterSelection.redoDepth == documentBeforeSelection.redoDepth,
                "Workspace selection changed authored document state or history.");
        require(session.getWorkspaceSelectionRevision() == workspaceRevisionBeforeSelection + 1,
                "Zone selection did not advance the workspace-selection revision exactly once.");
        require(session.getProject().authoring.selectedZoneId == persistedZoneBeforeSelection
                    && session.getProject().authoring.selectedGroupId == persistedGroupBeforeSelection,
                "Workspace selection leaked into persisted project selection fields.");
        const auto selectionStatistics = drs::app::MessageThreadMetrics::getStatistics();
        require(maximumFor(selectionStatistics, drs::app::MessageThreadSpanKind::authoringRefresh) == 0.0,
                "Selection-only presentation unexpectedly ran the full Authoring refresh.");
        require(maximumFor(selectionStatistics, drs::app::MessageThreadSpanKind::hostStateSerialization) == 0.0,
                "Zone selection unexpectedly serialized host state.");

        drs::plugin::Processor processor;
        const auto projectPublicationStarted = Clock::now();
        require(processor.replaceAuthoringProject(session.getProject()),
                "Processor rejected the Salamander authoring project.");
        const auto projectPublicationMicros = elapsedMicros(projectPublicationStarted);

        const auto hostStateBeforeInteraction = processor.getHostStatePublicationStatus();
        require(hostStateBeforeInteraction.inFlight || hostStateBeforeInteraction.pendingCount != 0,
                "Salamander host-state work settled before concurrent interaction coverage began.");
        const auto hostStateInteractionStarted = Clock::now();
        require(processor.getAuthoringSession().selectZone(projection.zones.back().id).applied,
                "Host-state latency coverage could not navigate during serialization.");
        const auto hostStateInteractionMicros = elapsedMicros(hostStateInteractionStarted);
        require(hostStateInteractionMicros < 16667,
                "Interaction while host-state serialization was active exceeded one 60 Hz frame: "
                    + std::to_string(hostStateInteractionMicros) + " us.");
        const auto hostStatePublished = processor.waitForHostStatePublication(30000);
        const auto hostStateWaitStatus = processor.getHostStatePublicationStatus();
        require(hostStatePublished,
                "Salamander host-state serialization did not publish the newest checkpoint. "
                    "submitted=" + std::to_string(hostStateWaitStatus.latestSubmittedRequestId)
                    + " completed=" + std::to_string(hostStateWaitStatus.latestCompletedRequestId)
                    + " failures=" + std::to_string(hostStateWaitStatus.failedCount)
                    + " durationUs=" + std::to_string(hostStateWaitStatus.lastDurationMicros)
                    + " projectBytes=" + std::to_string(
                        drs::engine::serializeRuntimeProjectManifest(
                            session.getProject(), "ui-baseline.drsproj").size()));
        const auto hostStateStatus = processor.getHostStatePublicationStatus();
        juce::MemoryBlock salamanderHostState;
        processor.getStateInformation(salamanderHostState);
        const auto parsedHostState = drs::engine::parseHostSessionState(std::string(
            static_cast<const char*>(salamanderHostState.getData()), salamanderHostState.getSize()));
        require(parsedHostState.isValidHostState()
                    && parsedHostState.hostState->authoringState.projectSnapshot.has_value()
                    && parsedHostState.hostState->authoringState.projectSnapshot->authoring.zones.size()
                        == projection.zones.size()
                    && hostStateStatus.latestCompletedRequestId
                        == hostStateStatus.latestSubmittedRequestId,
                "The background serializer did not publish the newest valid Salamander checkpoint.");

        const auto previewDispatchStarted = Clock::now();
        const auto fingerprintComputationsBeforePreview
            = processor.getCurrentDraftPreviewFingerprintComputationCount();
        processor.requestAuthoringPreview(drs::engine::AuthoringPreviewScope::currentDraft);
        const auto previewDispatchMicros = elapsedMicros(previewDispatchStarted);
        processor.prepareToPlay(44100.0, 512);

        const auto previewReadyStarted = Clock::now();
        auto observedSourceProgress = false;
        auto previewReady = false;
        while (elapsedMicros(previewReadyStarted) < 30000000)
        {
            processor.serviceMessageThreadWork();
            juce::AudioBuffer<float> activationBlock(2, 512);
            activationBlock.clear();
            juce::MidiBuffer activationMidi;
            processor.processBlock(activationBlock, activationMidi);
            const auto status = processor.getEngineFacade().getDraftPlaybackStatus();
            observedSourceProgress = observedSourceProgress
                || (status.pendingPreview.progressTotal == projection.sampleSources.size()
                    && status.pendingPreview.progressOrdinal > 0
                    && !status.pendingPreview.progressPhase.empty());
            previewReady = status.preview.revision == status.draftRevision
                && status.preview.state == "Ready";
            if (previewReady)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        const auto previewReadyMicros = elapsedMicros(previewReadyStarted);
        require(previewReady, "Salamander full-draft Preview did not reach Ready.");
        require(observedSourceProgress,
                "Salamander full-draft Preview exposed no per-source preparation progress.");
        const auto fingerprintComputationsAfterPreview
            = processor.getCurrentDraftPreviewFingerprintComputationCount();
        require(fingerprintComputationsAfterPreview == fingerprintComputationsBeforePreview + 1,
                "Salamander full-draft Preview recomputed its serialized project fingerprint while the authored revision was unchanged.");
        for (auto tick = 0; tick < 8; ++tick)
            processor.serviceMessageThreadWork();
        require(processor.getCurrentDraftPreviewFingerprintComputationCount()
                    == fingerprintComputationsAfterPreview,
                "Repeated message-thread service ticks recomputed the unchanged full-draft project fingerprint.");

        const auto publishDispatchStarted = Clock::now();
        const auto publishAccepted = processor.submitPerformancePublishCommand(
            {}, drs::engine::PerformancePublishCommandSource::authoringWorkspace);
        const auto publishDispatchMicros = elapsedMicros(publishDispatchStarted);
        require(publishAccepted, "Salamander draft Publish dispatch was rejected.");
        require(previewDispatchMicros < 16667,
                "Salamander Preview dispatch exceeded one 60 Hz frame.");
        require(publishDispatchMicros < 16667,
                "Salamander Publish dispatch exceeded one 60 Hz frame.");

        const auto publishReadyStarted = Clock::now();
        auto publishReady = false;
        while (elapsedMicros(publishReadyStarted) < 5000000)
        {
            processor.serviceMessageThreadWork();
            juce::AudioBuffer<float> activationBlock(2, 512);
            activationBlock.clear();
            juce::MidiBuffer activationMidi;
            processor.processBlock(activationBlock, activationMidi);
            const auto status = processor.getEngineFacade().getDraftPlaybackStatus();
            publishReady = status.performance.revision == status.draftRevision
                && status.performance.state == "Active";
            if (publishReady)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        const auto publishReadyMicros = elapsedMicros(publishReadyStarted);
        const auto authoredPlaybackStatus = processor.getEngineFacade().getDraftPlaybackStatus();
        require(publishReady, "Salamander Publish did not reach Active.");
        require(authoredPlaybackStatus.performance.reusedPreviewPayload,
                "Salamander Publish did not reuse the exact full-draft Preview payload.");
        require(authoredPlaybackStatus.performance.buildId == authoredPlaybackStatus.preview.buildId
                    && authoredPlaybackStatus.performance.preparedBuildId
                        == authoredPlaybackStatus.preview.preparedBuildId,
                "Salamander Publish did not preserve the exact Preview build identities.");

        std::unique_ptr<juce::AudioProcessorEditor> authoredEditor(processor.createEditor());
        require(authoredEditor != nullptr, "Could not create the real 4 Hz plug-in editor.");
        auto* authoredPerformancePanel
            = findByType<drs::app::PerformancePanel>(*authoredEditor);
        require(authoredPerformancePanel != nullptr,
                "The plug-in editor did not expose its Performance surface.");
        juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
        drs::app::MessageThreadMetrics::resetForTests();

        authoredPerformancePanel->getKeyboardState().noteOn(1, 60, 0.8f);
        juce::AudioBuffer<float> authoredOnScreenBlock(2, 512);
        authoredOnScreenBlock.clear();
        juce::MidiBuffer authoredOnScreenMidi;
        processor.processBlock(authoredOnScreenBlock, authoredOnScreenMidi);
        const auto authoredOnScreenMagnitude = authoredOnScreenBlock.getMagnitude(
            0, authoredOnScreenBlock.getNumSamples());
        authoredPerformancePanel->getKeyboardState().noteOff(1, 60, 0.8f);
        juce::AudioBuffer<float> authoredReleaseBlock(2, 512);
        authoredReleaseBlock.clear();
        juce::MidiBuffer authoredReleaseMidi;
        processor.processBlock(authoredReleaseBlock, authoredReleaseMidi);

        juce::AudioBuffer<float> authoredHostBlock(2, 512);
        authoredHostBlock.clear();
        juce::MidiBuffer authoredHostMidi;
        authoredHostMidi.addEvent(juce::MidiMessage::noteOn(1, 60, 0.8f), 0);
        processor.processBlock(authoredHostBlock, authoredHostMidi);
        const auto authoredHostMagnitude = authoredHostBlock.getMagnitude(
            0, authoredHostBlock.getNumSamples());
        require(authoredOnScreenMagnitude > 0.0001f && authoredHostMagnitude > 0.0001f,
                "Host and on-screen MIDI did not both render the large published piano.");
        juce::AudioBuffer<float> authoredHostReleaseBlock(2, 512);
        authoredHostReleaseBlock.clear();
        juce::MidiBuffer authoredHostReleaseMidi;
        authoredHostReleaseMidi.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        processor.processBlock(authoredHostReleaseBlock, authoredHostReleaseMidi);

        const auto fingerprintComputationsBeforeContinuousPlaying
            = processor.getCurrentDraftPreviewFingerprintComputationCount();
        constexpr auto continuousPlaybackSeconds = 180;
        const auto continuousBlockCount = static_cast<int>(
            (continuousPlaybackSeconds * 44100 + 511) / 512);
        std::atomic<int> concurrentAudioBlockCount {0};
        std::atomic<bool> stopConcurrentAudio {false};
        std::atomic<bool> concurrentAudioFailed {false};
        std::thread concurrentAudioThread([&]
        {
            try
            {
                juce::AudioBuffer<float> playbackBlock(2, 512);
                for (int blockIndex = 0;
                     blockIndex < continuousBlockCount
                        && !stopConcurrentAudio.load(std::memory_order_acquire);
                     ++blockIndex)
                {
                    playbackBlock.clear();
                    juce::MidiBuffer playbackMidi;
                    if (blockIndex == 0)
                        playbackMidi.addEvent(
                            juce::MidiMessage::controllerEvent(1, 64, 127), 0);
                    if (blockIndex == continuousBlockCount - 1)
                        playbackMidi.addEvent(
                            juce::MidiMessage::controllerEvent(1, 64, 0), 0);
                    processor.processBlock(playbackBlock, playbackMidi);
                    concurrentAudioBlockCount.store(blockIndex + 1,
                                                    std::memory_order_release);
                }
            }
            catch (...)
            {
                concurrentAudioFailed.store(true, std::memory_order_release);
            }
        });

        auto nextGestureBlock = 0;
        auto notesHeld = false;
        std::uint64_t maximumConcurrentDispatchMicros = 0;
        // This deadline detects a stuck render worker; callback and message-thread
        // latency have their own explicit gates below. Do not add synthetic pacing
        // to this accelerated three-minute audio-time qualification.
        const auto concurrentDeadline = Clock::now() + std::chrono::seconds(270);
        while (concurrentAudioBlockCount.load(std::memory_order_acquire)
                   < continuousBlockCount
               && Clock::now() < concurrentDeadline)
        {
            const auto currentBlock
                = concurrentAudioBlockCount.load(std::memory_order_acquire);
            if (currentBlock >= nextGestureBlock)
            {
                for (const auto note : { 48, 52, 55, 60 })
                {
                    if (notesHeld)
                        authoredPerformancePanel->getKeyboardState().noteOff(
                            1, note, 0.72f);
                    else
                        authoredPerformancePanel->getKeyboardState().noteOn(
                            1, note, 0.72f);
                }
                notesHeld = !notesHeld;
                nextGestureBlock = currentBlock + (notesHeld ? 96 : 288);
            }

            const auto dispatchStartedAt = Clock::now();
            juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
            maximumConcurrentDispatchMicros = std::max(
                maximumConcurrentDispatchMicros, elapsedMicros(dispatchStartedAt));
        }
        if (notesHeld)
            for (const auto note : { 48, 52, 55, 60 })
                authoredPerformancePanel->getKeyboardState().noteOff(1, note, 0.72f);
        juce::MessageManager::getInstance()->runDispatchLoopUntil(20);
        stopConcurrentAudio.store(true, std::memory_order_release);
        concurrentAudioThread.join();
        require(!concurrentAudioFailed.load(std::memory_order_acquire),
                "The concurrent large-piano audio thread failed.");
        require(concurrentAudioBlockCount.load(std::memory_order_acquire)
                    == continuousBlockCount,
                "The concurrent large-piano audio thread did not advance three minutes of blocks.");
        require(maximumConcurrentDispatchMicros < 100000,
                "The message thread stalled for more than 100 ms while concurrent piano audio was running.");
        juce::MessageManager::getInstance()->runDispatchLoopUntil(300);
        const auto continuousMetrics = drs::app::MessageThreadMetrics::getStatistics();
        require(maximumFor(continuousMetrics,
                           drs::app::MessageThreadSpanKind::performanceRefresh) == 0.0,
                "Notes, chords, sustain, releases, or timer ticks rebuilt the large-piano Performance surface.");
        require(maximumFor(continuousMetrics,
                           drs::app::MessageThreadSpanKind::performanceKeyboardCallback) < 1.0,
                "A large-piano keyboard callback exceeded the 1 ms queue-only budget.");
        require(observationsFor(continuousMetrics,
                                drs::app::MessageThreadSpanKind::editorTimerWork) >= 4,
                "Continuous-playing coverage did not exercise the real 4 Hz editor loop.");
        require(observationsFor(continuousMetrics,
                                drs::app::MessageThreadSpanKind::editorServiceWork) >= 4
                    && observationsFor(continuousMetrics,
                                       drs::app::MessageThreadSpanKind::editorPerformanceWork) >= 4
                    && observationsFor(continuousMetrics,
                                       drs::app::MessageThreadSpanKind::editorAuthoringWork) >= 4,
                "Concurrent coverage did not instrument each active 4 Hz editor section.");
        require(maximumFor(continuousMetrics,
                           drs::app::MessageThreadSpanKind::editorServiceWork) < 100.0
                    && maximumFor(continuousMetrics,
                                  drs::app::MessageThreadSpanKind::editorPerformanceWork) < 100.0
                    && maximumFor(continuousMetrics,
                                  drs::app::MessageThreadSpanKind::editorAuthoringWork) < 100.0,
                "An instrumented 4 Hz editor section exceeded the 100 ms responsiveness gate.");
        require(processor.getCurrentDraftPreviewFingerprintComputationCount()
                    == fingerprintComputationsBeforeContinuousPlaying,
                "Continuous playing fingerprinted or resolved the unchanged authored project on the message thread.");
        const auto continuousRealtimeSafety = processor.getRealtimeSafetySnapshot();
        require(continuousRealtimeSafety.samplePathResolutionsOnAudioThread == 0
                    && continuousRealtimeSafety.sampleDecodeEntriesOnAudioThread == 0
                    && continuousRealtimeSafety.authoringSampleLoadsOnAudioThread == 0,
                "Continuous playing opened, resolved, or decoded sample files from the realtime path.");
        authoredEditor.reset();

        processor.queuePerformanceSurfaceNoteOn(60, 0.8f);
        juce::AudioBuffer<float> authoredNoteBlock(2, 512);
        authoredNoteBlock.clear();
        juce::MidiBuffer authoredNoteMidi;
        processor.processBlock(authoredNoteBlock, authoredNoteMidi);
        const auto authoredNextBlockMagnitude = authoredNoteBlock.getMagnitude(
            0, authoredNoteBlock.getNumSamples());
        processor.queuePerformanceSurfaceNoteOff(60);
        require(authoredNextBlockMagnitude > 0.0001f,
                "The published Salamander keyboard note was not heard on the next audio block.");

        const auto fingerprintComputationsBeforeEdit
            = processor.getCurrentDraftPreviewFingerprintComputationCount();
        const auto currentMasterGain = processor.getAuthoringSession().getProject().authoring.masterGainDb;
        require(processor.getAuthoringSession().updateMasterGain(
                    currentMasterGain + 0.125, "Exercise full-draft fingerprint invalidation").applied,
                "Salamander fingerprint coverage could not advance the authored revision.");
        processor.serviceMessageThreadWork();
        require(processor.getCurrentDraftPreviewFingerprintComputationCount()
                    == fingerprintComputationsBeforeEdit + 1,
                "An authored revision change did not recompute the full-draft project fingerprint exactly once.");
        for (auto tick = 0; tick < 4; ++tick)
            processor.serviceMessageThreadWork();
        require(processor.getCurrentDraftPreviewFingerprintComputationCount()
                    == fingerprintComputationsBeforeEdit + 1,
                "Message-thread service recomputed the revised full-draft fingerprint more than once.");

        const auto packageLoadStarted = Clock::now();
        const auto packageLoad = processor.loadPerformancePackageWorkspace(
            juce::File(packagePath.string()));
        const auto packageLoadMicros = elapsedMicros(packageLoadStarted);
        require(packageLoad.loaded, "Valid playable package did not load: " + packageLoad.state);
        processor.prepareToPlay(44100.0, 512);
        processor.serviceMessageThreadWork();

        std::unique_ptr<juce::AudioProcessorEditor> packageEditor(processor.createEditor());
        require(packageEditor != nullptr, "Could not create the package Performance editor.");
        auto* performancePanel = findByType<drs::app::PerformancePanel>(*packageEditor);
        require(performancePanel != nullptr,
                "The package editor did not expose its Performance surface.");
        juce::MessageManager::getInstance()->runDispatchLoopUntil(300);

        const auto performanceRefreshStarted = Clock::now();
        performancePanel->refreshNow();
        const auto performanceRefreshMicros = elapsedMicros(performanceRefreshStarted);

        drs::app::MessageThreadMetrics::resetForTests();
        const auto presentationBeforeTimer
            = processor.getPerformancePublishPresentationSnapshot();
        const auto lifecycleBeforeTimer = processor.getEngineFacade()
            .getPerformancePublishLifecycleRevision();
        const auto telemetryBeforeTimer = processor.getEngineFacade()
            .getPerformanceTelemetryRevision();
        juce::MessageManager::getInstance()->runDispatchLoopUntil(1300);
        const auto presentationAfterTimer
            = processor.getPerformancePublishPresentationSnapshot();
        const auto timerMetrics = drs::app::MessageThreadMetrics::getStatistics();
        require(presentationBeforeTimer == presentationAfterTimer,
                "The 4 Hz service loop republished an unchanged Publish presentation snapshot.");
        require(processor.getEngineFacade().getPerformancePublishLifecycleRevision()
                    == lifecycleBeforeTimer,
                "Package telemetry advanced the Publish lifecycle revision.");
        require(processor.getEngineFacade().getPerformanceTelemetryRevision()
                    > telemetryBeforeTimer,
                "The 4 Hz package loop did not advance Diagnostics telemetry.");
        require(maximumFor(timerMetrics,
                           drs::app::MessageThreadSpanKind::performanceRefresh) == 0.0,
                "Package telemetry rebuilt the Performance surface.");
        require(observationsFor(timerMetrics,
                                drs::app::MessageThreadSpanKind::diagnosticsRefresh) == 0,
                "Hidden Diagnostics refreshed from package telemetry.");
        require(observationsFor(timerMetrics,
                                drs::app::MessageThreadSpanKind::editorTimerWork) >= 4,
                "Package telemetry coverage did not exercise the real 4 Hz editor loop.");

        const auto keyboardStarted = Clock::now();
        performancePanel->getKeyboardState().noteOn(1, 60, 0.8f);
        performancePanel->getKeyboardState().noteOff(1, 60, 0.8f);
        const auto keyboardCallbacksMicros = elapsedMicros(keyboardStarted);

        performancePanel->getKeyboardState().noteOn(1, 69, 0.8f);
        juce::AudioBuffer<float> firstAudioBlock(2, 512);
        firstAudioBlock.clear();
        juce::MidiBuffer emptyMidi;
        processor.processBlock(firstAudioBlock, emptyMidi);
        const auto nextBlockMagnitude = firstAudioBlock.getMagnitude(0, firstAudioBlock.getNumSamples());
        performancePanel->getKeyboardState().noteOff(1, 69, 0.8f);
        require(nextBlockMagnitude > 0.0001f,
                "A queued Performance keyboard note was not heard on the next audio block.");

        std::atomic<bool> stopPackageAudio {false};
        std::atomic<bool> packageAudioFailed {false};
        std::atomic<std::uint64_t> maximumPackageAudioBlockMicros {0};
        std::thread packageAudioThread([&]
        {
            try
            {
                juce::AudioBuffer<float> packageBlock(2, 512);
                while (!stopPackageAudio.load(std::memory_order_acquire))
                {
                    packageBlock.clear();
                    juce::MidiBuffer packageMidi;
                    const auto blockStartedAt = Clock::now();
                    processor.processBlock(packageBlock, packageMidi);
                    const auto blockMicros = elapsedMicros(blockStartedAt);
                    auto maximum = maximumPackageAudioBlockMicros.load(std::memory_order_relaxed);
                    while (maximum < blockMicros
                           && !maximumPackageAudioBlockMicros.compare_exchange_weak(
                               maximum, blockMicros, std::memory_order_relaxed))
                    {
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            catch (...)
            {
                packageAudioFailed.store(true, std::memory_order_release);
            }
        });

        std::uint64_t maximumPackageDispatchMicros = 0;
        const auto packagePlayStartedAt = Clock::now();
        auto sentSecondPackageNote = false;
        performancePanel->getKeyboardState().noteOn(1, 60, 0.8f);
        while (Clock::now() - packagePlayStartedAt < std::chrono::seconds(18))
        {
            if (!sentSecondPackageNote
                && Clock::now() - packagePlayStartedAt > std::chrono::milliseconds(50))
            {
                performancePanel->getKeyboardState().noteOn(1, 64, 0.8f);
                sentSecondPackageNote = true;
            }
            const auto dispatchStartedAt = Clock::now();
            juce::MessageManager::getInstance()->runDispatchLoopUntil(1);
            maximumPackageDispatchMicros = std::max(
                maximumPackageDispatchMicros, elapsedMicros(dispatchStartedAt));
        }
        performancePanel->getKeyboardState().noteOff(1, 60, 0.8f);
        performancePanel->getKeyboardState().noteOff(1, 64, 0.8f);
        stopPackageAudio.store(true, std::memory_order_release);
        packageAudioThread.join();
        require(!packageAudioFailed.load(std::memory_order_acquire),
                "The sustained package audio thread failed.");
        require(maximumPackageDispatchMicros < 100000,
                "The message thread stalled for more than 100 ms during sustained package playback.");
        require(maximumPackageAudioBlockMicros.load(std::memory_order_relaxed) < 100000,
                "A package audio callback exceeded the 100 ms test ceiling.");

        const auto slowSpans = drs::app::MessageThreadMetrics::getSlowSpanStatistics();
        fs::create_directories(reportPath.parent_path());
        std::ofstream report(reportPath, std::ios::binary | std::ios::trunc);
        require(report.good(), "Could not create UI responsiveness baseline report.");
        report << "{\n"
               << "  \"schema\": \"drs.uiResponsivenessBaseline\",\n"
               << "  \"salamanderZones\": " << projection.zones.size() << ",\n"
               << "  \"salamanderSources\": " << projection.sampleSources.size() << ",\n"
               << "  \"authoringRefreshMicros\": " << authoringRefreshMicros << ",\n"
               << "  \"zoneSelectionMicros\": " << zoneSelectionMicros << ",\n"
               << "  \"selectionDocumentRevisionDelta\": "
               << (documentAfterSelection.revision - documentBeforeSelection.revision) << ",\n"
               << "  \"selectionUndoDepthDelta\": "
               << (documentAfterSelection.undoDepth - documentBeforeSelection.undoDepth) << ",\n"
               << "  \"projectPublicationMicros\": " << projectPublicationMicros << ",\n"
               << "  \"hostStateInteractionMicros\": " << hostStateInteractionMicros << ",\n"
               << "  \"hostStateWorkerMicros\": " << hostStateStatus.lastDurationMicros << ",\n"
               << "  \"hostStateCoalescedCount\": " << hostStateStatus.coalescedCount << ",\n"
               << "  \"previewDispatchMicros\": " << previewDispatchMicros << ",\n"
               << "  \"previewReadyMicros\": " << previewReadyMicros << ",\n"
               << "  \"observedSourceProgress\": "
               << (observedSourceProgress ? "true" : "false") << ",\n"
               << "  \"publishDispatchMicros\": " << publishDispatchMicros << ",\n"
               << "  \"publishReadyMicros\": " << publishReadyMicros << ",\n"
               << "  \"publishReusedPreviewPayload\": "
               << (authoredPlaybackStatus.performance.reusedPreviewPayload ? "true" : "false") << ",\n"
               << "  \"authoredNextBlockMagnitude\": " << authoredNextBlockMagnitude << ",\n"
               << "  \"continuousPlaybackSeconds\": " << continuousPlaybackSeconds << ",\n"
               << "  \"maximumConcurrentDispatchMicros\": "
               << maximumConcurrentDispatchMicros << ",\n"
               << "  \"authoredOnScreenMagnitude\": " << authoredOnScreenMagnitude << ",\n"
               << "  \"authoredHostMagnitude\": " << authoredHostMagnitude << ",\n"
               << "  \"packageLoadMicros\": " << packageLoadMicros << ",\n"
               << "  \"performanceRefreshMicros\": " << performanceRefreshMicros << ",\n"
               << "  \"keyboardCallbacksMicros\": " << keyboardCallbacksMicros << ",\n"
               << "  \"maximumPackageDispatchMicros\": "
               << maximumPackageDispatchMicros << ",\n"
               << "  \"maximumPackageAudioBlockMicros\": "
               << maximumPackageAudioBlockMicros.load(std::memory_order_relaxed) << ",\n"
               << "  \"nextBlockMagnitude\": " << nextBlockMagnitude << ",\n"
               << std::fixed << std::setprecision(3)
               << "  \"maxInstrumentedAuthoringRefreshMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::authoringRefresh) << ",\n"
               << "  \"maxInstrumentedZoneSelectionMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::zoneSelection) << ",\n"
               << "  \"maxInstrumentedPerformanceRefreshMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::performanceRefresh) << ",\n"
               << "  \"maxInstrumentedKeyboardCallbackMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::performanceKeyboardCallback) << ",\n"
               << "  \"maxInstrumentedEditorTimerMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::editorTimerWork) << ",\n"
               << "  \"maxInstrumentedEngineServiceMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::editorServiceWork) << ",\n"
               << "  \"maxInstrumentedPerformanceSectionMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::editorPerformanceWork) << ",\n"
               << "  \"maxInstrumentedAuthoringSectionMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::editorAuthoringWork) << ",\n"
               << "  \"maxInstrumentedHostStateSerializationMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::hostStateSerialization) << ",\n"
               << "  \"maxInstrumentedPreviewDispatchMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::previewDispatch) << ",\n"
               << "  \"maxInstrumentedPublishDispatchMs\": "
               << maximumFor(slowSpans, drs::app::MessageThreadSpanKind::publishDispatch) << ",\n"
               << "  \"slowSpanKinds\": " << slowSpans.size() << "\n"
               << "}\n";
        require(report.good(), "UI responsiveness baseline report write failed.");

        std::cout << "UI responsiveness baseline passed: authoringRefreshUs="
                  << authoringRefreshMicros << " zoneSelectionUs=" << zoneSelectionMicros
                  << " projectPublicationUs=" << projectPublicationMicros
                  << " hostStateInteractionUs=" << hostStateInteractionMicros
                  << " hostStateWorkerUs=" << hostStateStatus.lastDurationMicros
                  << " previewDispatchUs=" << previewDispatchMicros
                  << " previewReadyUs=" << previewReadyMicros
                  << " publishDispatchUs=" << publishDispatchMicros
                  << " publishReadyUs=" << publishReadyMicros
                  << " packageLoadUs=" << packageLoadMicros
                  << " performanceRefreshUs=" << performanceRefreshMicros
                  << " keyboardCallbacksUs=" << keyboardCallbacksMicros << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "UI responsiveness baseline failed: " << error.what() << std::endl;
        return 1;
    }
}
