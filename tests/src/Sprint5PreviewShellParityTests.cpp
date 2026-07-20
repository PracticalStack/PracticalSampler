#include "drs/engine/AuthoringPreviewController.h"
#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "shared/AuthoringPanel.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

#include <algorithm>
#include <atomic>
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

juce::Component* findDescendant(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;
    for (auto* child : root.getChildren())
    {
        if (auto* match = findDescendant(*child, componentId))
            return match;
    }
    return nullptr;
}

drs::app::AuthoringPanel& requireAuthoringPanel(juce::Component& shell)
{
    auto* tabs = dynamic_cast<juce::TabbedComponent*>(findDescendant(shell, "workspaceTabs"));
    require(tabs != nullptr && tabs->getNumTabs() >= 2,
            "Shell must expose Perform and Map tabs.");
    tabs->setCurrentTabIndex(1);
    auto* component = findDescendant(shell, "authoringWorkspace");
    auto* panel = dynamic_cast<drs::app::AuthoringPanel*>(component);
    require(panel != nullptr, "Shell must expose the shared authoring workspace.");
    return *panel;
}

void crossBlock(drs::plugin::Processor& processor)
{
    juce::AudioBuffer<float> buffer(2, 256);
    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.serviceMessageThreadWork();
}

bool waitForReady(drs::plugin::Processor& processor)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
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

drs::app::AuthoringPreviewStatusSnapshot failWithMissingSource(
    drs::plugin::Processor& processor,
    const std::string& shellName)
{
    auto project = processor.getAuthoringSession().getProject();
    const auto selected = processor.getAuthoringSession().getSelectedZone();
    require(selected.has_value(), shellName + " failure guidance requires a selected zone.");
    auto source = std::find_if(project.sampleSources.begin(), project.sampleSources.end(),
                               [&](const auto& item)
                               {
                                   return item.id == selected->sampleSourceId;
                               });
    require(source != project.sampleSources.end(),
            shellName + " failure guidance requires the selected sample source.");
    source->path = project.contentRootPath + "/missing-sprint5-status.wav";
    processor.replaceAuthoringProject(std::move(project));
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < deadline)
    {
        processor.serviceMessageThreadWork();
        if (processor.getAuthoringPreviewControllerSnapshot().preparationState
            == drs::engine::AuthoringPreviewPreparationState::failed)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    const auto status = processor.getAuthoringPreviewStatusSnapshot();
    require(status.presentationState == drs::engine::AuthoringPreviewPresentationState::failed
                && status.usingLastKnownGood
                && status.stateLabel == "Failed — Last Good Active"
                && !status.findings.empty()
                && !status.blockingPrerequisite.empty()
                && !status.creatorGuidance.empty(),
            shellName + " must expose actionable missing-source guidance and last-known-good identity.");
    return status;
}

drs::app::AuthoringPreviewStatusSnapshot prepareShell(
    drs::plugin::Processor& processor,
    const drs::engine::RuntimeProjectModel& source,
    const std::string& shellName)
{
    auto project = source;
    project.authoring.selectedZoneId = "pad-a3-high";
    processor.prepareToPlay(48000.0, 256);
    processor.replaceAuthoringProject(std::move(project));
    require(waitForReady(processor), shellName + " Preview did not reach Ready.");
    crossBlock(processor);
    const auto status = processor.getAuthoringPreviewStatusSnapshot();
    require(status.available
                && status.presentationState == drs::engine::AuthoringPreviewPresentationState::active
                && status.stateLabel == "Ready"
                && status.activeRevision == status.draftRevision
                && status.activePreparedBuildId != 0
                && !status.activeSnapshotDigest.empty()
                && !status.activePreparedDigest.empty(),
            shellName + " must publish a complete immutable active Preview identity.");
    return status;
}

void runDeterministicMetricsContract()
{
    using namespace drs::engine;
    AuthoringPreviewController controller({ 10, 40, 32, 8 });
    const auto first = controller.request(
        AuthoringPreviewScope::selectedZone, 9, "zone-a",
        AuthoringPreviewRequestReason::explicitSelectedZoneAudition,
        AuthoringPreviewInvalidationCategory::gain, "metrics-a", 100);
    require(first.accepted && controller.launchIfEligible(115).launched
                && controller.acceptPrepared(first.request.identity, 41, 140, "snapshot-a", "prepared-a")
                && controller.markActivationPending(first.request.identity, 150)
                && controller.markActive(first.request.identity, 170),
            "Metrics contract must complete one deterministic Preview lifecycle.");
    auto snapshot = controller.getSnapshot();
    require(snapshot.lastRequestToLaunchMicros == 15
                && snapshot.lastPreparationMicros == 25
                && snapshot.lastReadyToActivationMicros == 30
                && snapshot.lastRequestToAudibleMicros == 70
                && snapshot.activeSnapshotDigest == "snapshot-a"
                && snapshot.activePreparedDigest == "prepared-a",
            "Lifecycle timing and immutable digest metrics must match their phase boundaries.");

    const auto cancel = controller.request(
        AuthoringPreviewScope::selectedZone, 10, "zone-a",
        AuthoringPreviewRequestReason::authoringChanged,
        AuthoringPreviewInvalidationCategory::gain, "metrics-b", 200);
    require(cancel.accepted && controller.launchIfEligible(215).launched,
            "Cancellation metrics require in-flight preparation.");
    const auto replacement = controller.request(
        AuthoringPreviewScope::selectedZone, 11, "zone-a",
        AuthoringPreviewRequestReason::authoringChanged,
        AuthoringPreviewInvalidationCategory::gain, "metrics-c", 225);
    snapshot = controller.getSnapshot();
    require(replacement.accepted && replacement.cancellationRequested
                && snapshot.lastCancellationMicros == 25
                && snapshot.maxCancellationMicros == 25
                && snapshot.canceledCount == 1
                && snapshot.pendingDepth == 1
                && snapshot.maximumPendingDepth == 1,
            "Supersession must publish bounded cancellation and queue-depth metrics.");
}

void requireShellControls(juce::Component& shell,
                          drs::plugin::Processor& processor,
                          const drs::app::AuthoringPreviewStatusSnapshot& expected,
                          const std::string& shellName)
{
    auto& panel = requireAuthoringPanel(shell);
    panel.refreshNow();
    auto* status = dynamic_cast<juce::Label*>(findDescendant(panel, "authoringWaveformStatusLabel"));
    auto* enabled = dynamic_cast<juce::ToggleButton*>(findDescendant(panel, "authoringPreviewEnabledToggle"));
    auto* stop = dynamic_cast<juce::TextButton*>(findDescendant(panel, "authoringPreviewStopButton"));
    require(status != nullptr && enabled != nullptr && stop != nullptr,
            shellName + " must expose the shared Preview status and controls.");
    require(status->getText().contains(expected.stateLabel)
                && enabled->getToggleState()
                && !enabled->getTitle().isEmpty()
                && !enabled->getDescription().isEmpty()
                && !stop->getTitle().isEmpty()
                && !stop->getHelpText().isEmpty(),
            shellName + " Preview controls must carry matching creator text and accessibility metadata.");
    require(enabled->getBounds().getWidth() > 0 && stop->getBounds().getWidth() > 0
                && panel.getLocalBounds().contains(enabled->getBounds())
                && panel.getLocalBounds().contains(stop->getBounds()),
            shellName + " compact Preview controls must remain inside the authoring workspace.");

    drs::engine::AuthoringPreviewCommand note;
    note.type = drs::engine::AuthoringPreviewCommandType::noteOn;
    note.source = drs::engine::AuthoringPreviewAuditionSource::summaryPreview;
    note.midiNote = 57;
    note.velocity = 0.75f;
    require(processor.submitAuthoringPreviewCommand(note),
            shellName + " must accept an authoring-only Preview note.");
    panel.refreshNow();
    require(stop->isEnabled(), shellName + " Stop must enable while Preview owns a note.");
    require(static_cast<bool>(stop->onClick),
            shellName + " Stop must expose its shared command handler.");
    stop->onClick();
    panel.refreshNow();
    require(processor.getAuthoringPreviewCommandSnapshot().ownedNoteCount == 0
                && !stop->isEnabled(),
            shellName + " Stop must clear only Preview-owned notes.");

    const auto activeIdentity = processor.getAuthoringPreviewStatusSnapshot();
    enabled->setToggleState(false, juce::sendNotificationSync);
    require(!enabled->getToggleState(), shellName + " Preview toggle must disable audition.");
    const auto disabledIdentity = processor.getAuthoringPreviewStatusSnapshot();
    require(disabledIdentity.activePreparedBuildId == activeIdentity.activePreparedBuildId
                && disabledIdentity.activePreparedDigest == activeIdentity.activePreparedDigest,
            shellName + " disabling audition must preserve the immutable active Preview model.");
}

void runConcurrentPublicationRead(drs::plugin::Processor& processor)
{
    std::atomic<bool> stop { false };
    std::atomic<bool> coherent { true };
    std::thread reader([&]
    {
        while (!stop.load(std::memory_order_acquire))
        {
            const auto status = processor.getAuthoringPreviewStatusSnapshot();
            if (status.activePreparedBuildId != 0
                && (status.activeSnapshotDigest.empty() || status.activePreparedDigest.empty()))
                coherent.store(false, std::memory_order_release);
        }
    });
    for (auto index = 0; index < 200; ++index)
        processor.serviceMessageThreadWork();
    stop.store(true, std::memory_order_release);
    reader.join();
    require(coherent.load(std::memory_order_acquire),
            "Concurrent shell readers must observe whole immutable Preview publications.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Mini Sprint 5.7 requires the authored reference project.");
        runDeterministicMetricsContract();

        drs::standalone::MainComponent standalone(false);
        standalone.setSize(900, 680);
        const auto standaloneStatus = prepareShell(standalone.getProcessor(), loaded.project,
                                                   "Standalone shell");

        drs::plugin::Processor pluginProcessor;
        const auto pluginStatus = prepareShell(pluginProcessor, loaded.project,
                                               "VST3 editor shell");
        require(standaloneStatus.presentationState == pluginStatus.presentationState
                    && standaloneStatus.stateLabel == pluginStatus.stateLabel
                    && standaloneStatus.activePreparedBuildId == pluginStatus.activePreparedBuildId
                    && standaloneStatus.activeSnapshotDigest == pluginStatus.activeSnapshotDigest
                    && standaloneStatus.activePreparedDigest == pluginStatus.activePreparedDigest,
                "Standalone and VST3 editor shells must publish equivalent Preview status identity.");

        requireShellControls(standalone, standalone.getProcessor(), standaloneStatus,
                             "Standalone shell");
        {
            drs::plugin::Editor editor(pluginProcessor);
            editor.setSize(900, 680);
            requireShellControls(editor, pluginProcessor, pluginStatus, "VST3 editor shell");
            runConcurrentPublicationRead(pluginProcessor);

            const auto standaloneFailed = failWithMissingSource(standalone.getProcessor(),
                                                                 "Standalone shell");
            const auto pluginFailed = failWithMissingSource(pluginProcessor, "VST3 editor shell");
            require(standaloneFailed.presentationState == pluginFailed.presentationState
                        && standaloneFailed.stateLabel == pluginFailed.stateLabel
                        && standaloneFailed.failureFamily == pluginFailed.failureFamily
                        && standaloneFailed.blockingPrerequisite == pluginFailed.blockingPrerequisite,
                    "Standalone and VST3 shells must present equivalent failure guidance.");
            auto& standalonePanel = requireAuthoringPanel(standalone);
            auto& pluginPanel = requireAuthoringPanel(editor);
            standalonePanel.refreshNow();
            pluginPanel.refreshNow();
            auto* standaloneLabel = dynamic_cast<juce::Label*>(
                findDescendant(standalonePanel, "authoringWaveformStatusLabel"));
            auto* pluginLabel = dynamic_cast<juce::Label*>(
                findDescendant(pluginPanel, "authoringWaveformStatusLabel"));
            require(standaloneLabel != nullptr && pluginLabel != nullptr
                        && standaloneLabel->getText().contains("Failed")
                        && pluginLabel->getText().contains("Failed"),
                    "Both shell status surfaces must expose the failed current request.");
        }
        const auto editorClosed = pluginProcessor.getAuthoringPreviewStatusSnapshot();
        require(editorClosed.activePreparedBuildId == pluginStatus.activePreparedBuildId
                    && editorClosed.activePreparedDigest == pluginStatus.activePreparedDigest
                    && editorClosed.presentationState
                        == drs::engine::AuthoringPreviewPresentationState::failed
                    && editorClosed.usingLastKnownGood,
                "Closing the VST3 editor must not change Preview activation lifetime or status identity.");

        std::cout << "Mini Sprint 5.7 Preview status, responsiveness metrics, and shell parity passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Mini Sprint 5.7 shell parity failed: " << exception.what() << std::endl;
        return 1;
    }
}
