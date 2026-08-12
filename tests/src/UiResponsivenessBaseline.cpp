#include "drs/engine/AuthoringSession.h"
#include "drs/engine/SfzImportProjection.h"
#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "shared/MessageThreadMetrics.h"
#include "shared/PerformancePanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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
        require(processor.waitForHostStatePublication(30000),
                "Salamander host-state serialization did not publish the newest checkpoint.");
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

        const auto packageLoadStarted = Clock::now();
        const auto packageLoad = processor.loadPerformancePackageWorkspace(
            juce::File(packagePath.string()));
        const auto packageLoadMicros = elapsedMicros(packageLoadStarted);
        require(packageLoad.loaded, "Valid playable package did not load: " + packageLoad.state);
        processor.prepareToPlay(44100.0, 512);
        processor.serviceMessageThreadWork();

        drs::app::PerformancePanel performancePanel(
            processor.getEngineFacade(),
            {},
            [&processor](const int note, const float velocity)
            {
                processor.queuePerformanceSurfaceNoteOn(note, velocity);
            },
            [&processor](const int note)
            {
                processor.queuePerformanceSurfaceNoteOff(note);
            });
        performancePanel.setSize(1280, 900);
        performancePanel.refreshNow();

        const auto performanceRefreshStarted = Clock::now();
        performancePanel.refreshNow();
        const auto performanceRefreshMicros = elapsedMicros(performanceRefreshStarted);

        const auto keyboardStarted = Clock::now();
        performancePanel.getKeyboardState().noteOn(1, 60, 0.8f);
        performancePanel.getKeyboardState().noteOff(1, 60, 0.8f);
        const auto keyboardCallbacksMicros = elapsedMicros(keyboardStarted);

        performancePanel.getKeyboardState().noteOn(1, 69, 0.8f);
        juce::AudioBuffer<float> firstAudioBlock(2, 512);
        firstAudioBlock.clear();
        juce::MidiBuffer emptyMidi;
        processor.processBlock(firstAudioBlock, emptyMidi);
        const auto nextBlockMagnitude = firstAudioBlock.getMagnitude(0, firstAudioBlock.getNumSamples());
        performancePanel.getKeyboardState().noteOff(1, 69, 0.8f);
        require(nextBlockMagnitude > 0.0001f,
                "A queued Performance keyboard note was not heard on the next audio block.");

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
               << "  \"packageLoadMicros\": " << packageLoadMicros << ",\n"
               << "  \"performanceRefreshMicros\": " << performanceRefreshMicros << ",\n"
               << "  \"keyboardCallbacksMicros\": " << keyboardCallbacksMicros << ",\n"
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
