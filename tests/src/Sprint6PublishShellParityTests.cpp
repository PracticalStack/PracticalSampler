#include "drs/engine/PerformancePublishCommandAdapter.h"
#include "drs/engine/PerformancePublishPresentation.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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

void crossBoundary(drs::plugin::Processor& processor, juce::MidiBuffer midi = {})
{
    juce::AudioBuffer<float> buffer(2, 256);
    buffer.clear();
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForActive(drs::plugin::Processor& processor, std::size_t revision)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        crossBoundary(processor);
        const auto presentation = processor.getPerformancePublishPresentationSnapshot();
        if (presentation != nullptr
            && presentation->state == drs::engine::PerformancePublishPresentationState::active
            && presentation->activePublishedRevision == revision)
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto final = processor.getPerformancePublishPresentationSnapshot();
    const auto controller = processor.getPerformancePublishControllerSnapshot();
    std::cerr << "Active timeout revision=" << revision
              << " state=" << (final != nullptr ? final->stateLabel : "null")
              << " active=" << (final != nullptr ? final->activePublishedRevision : 0)
              << " requested=" << (final != nullptr ? final->requestedPublishRevision : 0)
              << " failure=" << controller.failureFinding.code << std::endl;
    return false;
}

juce::Component* findById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto* child : root.getChildren())
        if (auto* found = findById(*child, id))
            return found;
    return nullptr;
}

juce::TabbedComponent* findTabs(juce::Component& root)
{
    if (auto* tabs = dynamic_cast<juce::TabbedComponent*>(&root))
        return tabs;
    for (auto* child : root.getChildren())
        if (auto* tabs = findTabs(*child))
            return tabs;
    return nullptr;
}

std::string readText(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(static_cast<bool>(input), "Could not read source audit file " + path.generic_string());
    return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}
} // namespace

int main()
{
    using namespace drs::engine;
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        PerformancePublishCommandAdapter commandAdapter;
        auto commandDispatch = commandAdapter.dispatch(
            {}, PerformancePublishCommandSource::authoringWorkspace);
        require(commandDispatch.accepted, "The typed current-draft Publish command must be accepted.");
        commandAdapter.recordExecutionResult(true);
        const auto invalid = commandAdapter.dispatch(
            { static_cast<PerformancePublishCommandType>(255) },
            PerformancePublishCommandSource::externalApi);
        const auto adapterSnapshot = commandAdapter.getSnapshot();
        require(!invalid.accepted && invalid.rejectionCode != nullptr
                    && adapterSnapshot.acceptedCommandCount == 1
                    && adapterSnapshot.rejectedCommandCount == 1
                    && adapterSnapshot.executionAcceptedCount == 1,
                "The command adapter must expose typed acceptance and execution counters.");

        DraftPlaybackStatus draft;
        draft.projectOpen = true;
        draft.draftRevision = 2;
        draft.performance.available = true;
        draft.performance.revision = 1;
        draft.performance.contentDigest = "authored:1";
        PerformancePublishControllerSnapshot controller;
        controller.hasActiveRequest = true;
        controller.activeRequestIdentity.draftRevision = 1;
        controller.activeRequestIdentity.authoredContentDigest = "authored:1";
        PreparedPlaybackWorkerStatus worker;
        auto presentation = buildPerformancePublishPresentationSnapshot(draft, controller, worker, 1);
        require(presentation.state == PerformancePublishPresentationState::stale
                    && presentation.dirty && presentation.canPublish
                    && presentation.lastKnownGoodRevision == 1,
                "A newer draft must present Stale while preserving explicit last-known-good identity.");
        controller.hasRequest = true;
        controller.currentRequest.identity.draftRevision = 2;
        controller.currentRequest.identity.authoredContentDigest = "authored:2";
        controller.preparationState = PerformancePublishPreparationState::preparing;
        presentation = buildPerformancePublishPresentationSnapshot(draft, controller, worker, 2);
        require(presentation.state == PerformancePublishPresentationState::preparing
                    && !presentation.canPublish && presentation.progress == 0.5
                    && presentation.requestedPublishRevision == 2,
                "Preparing must expose requested identity, progress, and disabled duplicate action.");
        controller.preparationState = PerformancePublishPreparationState::failed;
        controller.hasFailedRequest = true;
        controller.failedRequestIdentity = controller.currentRequest.identity;
        controller.failureFinding = { PerformancePublishFindingSeverity::error,
                                      "invalid-route", "authoring.routes", "Repair the route." };
        presentation = buildPerformancePublishPresentationSnapshot(draft, controller, worker, 3);
        require(presentation.state == PerformancePublishPresentationState::failed
                    && presentation.failedRevision == 2
                    && presentation.hasLastKnownGood
                    && presentation.guidance.find("Repair the route") != std::string::npos,
                "Failure must retain failed and last-known-good identities with recovery guidance.");

        const auto projectLoad = loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Mini Sprint 6.8 requires the authored reference project.");
        drs::plugin::Processor plugin;
        plugin.prepareToPlay(48000.0, 256);
        plugin.replaceAuthoringProject(projectLoad.project);
        require(plugin.getAuthoringSession().selectZone("pad-a3-high").applied,
                "Shell parity requires one selected authored zone.");
        auto zone = plugin.getAuthoringSession().getSelectedZone();
        require(zone.has_value(), "The selected authored zone must remain available.");
        zone->gainDb -= 0.25;
        require(plugin.getAuthoringSession().updateSelectedZone(*zone, "Sprint 6.8 dirty state").applied,
                "A valid edit must create a dirty Publish revision.");
        plugin.serviceMessageThreadWork();
        auto pluginPresentation = plugin.getPerformancePublishPresentationSnapshot();
        require(pluginPresentation != nullptr && pluginPresentation->dirty
                    && pluginPresentation->canPublish,
                "The immutable presentation must expose dirty/enabled state before editor creation.");

        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(plugin.createEditor());
        require(editor != nullptr, "The plug-in editor must open for compact shell coverage.");
        auto* statusPublish = dynamic_cast<juce::TextButton*>(
            findById(*editor, "statusPublishDraftButton"));
        if (auto* tabs = findTabs(*editor))
            tabs->setCurrentTabIndex(1);
        auto* authoringPublish = dynamic_cast<juce::TextButton*>(
            findById(*editor, "authoringPublishDraftButton"));
        require(authoringPublish != nullptr && statusPublish != nullptr
                    && authoringPublish->getTitle().isNotEmpty()
                    && authoringPublish->getHelpText().isNotEmpty()
                    && statusPublish->getTitle().isNotEmpty()
                    && statusPublish->getHelpText().isNotEmpty(),
                "Compact shell Publish controls require stable IDs and accessible title/help metadata.");
        const auto presentationBeforeClose = pluginPresentation;
        editor.reset();
        require(plugin.getPerformancePublishPresentationSnapshot() == presentationBeforeClose,
                "Closing the editor must not replace processor-owned immutable Publish truth.");

        const auto revision = plugin.getAuthoringSession().getDocumentState().revision;
        require(plugin.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::authoringWorkspace)
                    && waitForActive(plugin, revision),
                "Editor-closed typed Publish must activate the exact current revision.");
        pluginPresentation = plugin.getPerformancePublishPresentationSnapshot();
        const auto pluginCommands = plugin.getPerformancePublishCommandSnapshot();
        require(pluginPresentation != nullptr
                    && pluginPresentation->state == PerformancePublishPresentationState::active
                    && !pluginPresentation->dirty
                    && pluginPresentation->activePublishedRevision == revision
                    && pluginPresentation->lastKnownGoodRevision == revision
                    && pluginCommands.acceptedCommandCount == 1
                    && pluginCommands.lastSource == PerformancePublishCommandSource::authoringWorkspace,
                "Typed command, active state, and last-known-good identity must survive editor closure.");
        editor.reset(plugin.createEditor());
        require(plugin.getPerformancePublishPresentationSnapshot()->activePublishedRevision == revision,
                "Reopening the editor must observe the same processor-owned active publication.");
        editor.reset();

        plugin.requestAuthoringPreview(AuthoringPreviewScope::currentDraft);
        for (int iteration = 0; iteration < 100 && !plugin.getAuthoringPreviewControllerSnapshot().hasActiveRequest; ++iteration)
        {
            plugin.serviceMessageThreadWork();
            crossBoundary(plugin);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        plugin.queueAuthoringPreviewNoteOn(57, 0.7f);
        crossBoundary(plugin);
        auto routing = plugin.getRealtimeSafetySnapshot();
        require(routing.authoringPreviewActiveVoiceCount > 0,
                "Authoring keyboard commands must route to Preview.");
        plugin.submitAuthoringPreviewCommand(
            { AuthoringPreviewCommandType::emergencyReset,
              AuthoringPreviewAuditionSource::authoringKeyboard });
        crossBoundary(plugin);
        juce::MidiBuffer hostMidi;
        hostMidi.addEvent(juce::MidiMessage::noteOn(1, 57, static_cast<juce::uint8>(100)), 0);
        crossBoundary(plugin, hostMidi);
        routing = plugin.getRealtimeSafetySnapshot();
        require(routing.performanceActiveVoiceCount > 0
                    && routing.authoringPreviewActiveVoiceCount == 0,
                "Host MIDI must route to Performance without leaking into Preview.");

        drs::standalone::MainComponent standalone(false);
        auto& standaloneProcessor = standalone.getProcessor();
        standaloneProcessor.prepareToPlay(48000.0, 256);
        standaloneProcessor.replaceAuthoringProject(projectLoad.project);
        require(standaloneProcessor.getAuthoringSession().selectZone("pad-a3-high").applied,
                "Expanded standalone shell requires the same authored selection.");
        auto standaloneZone = standaloneProcessor.getAuthoringSession().getSelectedZone();
        standaloneZone->gainDb -= 0.25;
        require(standaloneProcessor.getAuthoringSession().updateSelectedZone(
                    *standaloneZone, "Sprint 6.8 standalone dirty state").applied,
                "Standalone edit must create the matching dirty revision.");
        standaloneProcessor.serviceMessageThreadWork();
        const auto standaloneRevision
            = standaloneProcessor.getAuthoringSession().getDocumentState().revision;
        require(standaloneProcessor.submitPerformancePublishCommand(
                    {}, PerformancePublishCommandSource::authoringWorkspace)
                    && waitForActive(standaloneProcessor, standaloneRevision),
                "Standalone must use the same typed Publish path.");
        const auto standalonePresentation
            = standaloneProcessor.getPerformancePublishPresentationSnapshot();
        require(standalonePresentation != nullptr
                    && standalonePresentation->state == pluginPresentation->state
                    && standalonePresentation->activePublishedRevision
                        == pluginPresentation->activePublishedRevision
                    && standalonePresentation->dirty == pluginPresentation->dirty
                    && standalonePresentation->stateLabel == pluginPresentation->stateLabel,
                "Standalone and editor-closed plug-in shells must expose the same immutable truth.");

        const auto root = std::filesystem::path(DRS_SOURCE_ROOT);
        const auto statusSource = readText(root / "app/src/shared/StatusPanel.cpp");
        const auto pluginSource = readText(root / "app/src/plugin/PluginEditor.cpp");
        const auto standaloneSource = readText(root / "app/src/standalone/MainComponent.cpp");
        const auto processorSource = readText(root / "app/src/plugin/PluginProcessor.cpp");
        require(statusSource.find("engineFacade.publishCurrentDraft();") == std::string::npos
                    && pluginSource.find("owner.getEngineFacade().publishCurrentDraft();") == std::string::npos
                    && standaloneSource.find("processor.getEngineFacade().publishCurrentDraft();") == std::string::npos
                    && processorSource.find("submitPerformancePublishCommand(") != std::string::npos
                    && processorSource.find("performanceEvents.push(") != std::string::npos
                    && processorSource.find(
                        "drainRealtimeNoteEvents(authoringPreviewNoteQueue, authoringPreviewEvents")
                        != std::string::npos,
                "Source audit must enforce one typed Publish adapter and distinct lane event buffers.");

        std::cout << "Mini Sprint 6.8 Publish command, status, routing, and shell parity matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 6.8 shell parity matrix failed: " << exception.what() << std::endl;
        return 1;
    }
}
