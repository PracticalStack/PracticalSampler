#include "drs/engine/RuntimeLoader.h"
#include "plugin/PluginEditor.h"
#include "plugin/PluginProcessor.h"
#include "standalone/MainComponent.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
class DesktopHostedComponent final : public juce::Component
{
public:
    explicit DesktopHostedComponent(juce::Component& contentToHost)
        : hostedContent(contentToHost)
    {
        addAndMakeVisible(hostedContent);
        setSize(hostedContent.getWidth(), hostedContent.getHeight());
        addToDesktop(0);
        setVisible(true);
        toFront(true);
        resized();
    }

    ~DesktopHostedComponent() override
    {
        removeChildComponent(&hostedContent);
        setVisible(false);
        removeFromDesktop();
    }

    void resized() override
    {
        hostedContent.setBounds(getLocalBounds());
    }

private:
    juce::Component& hostedContent;
};

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void pumpMessages(int millis = 20)
{
#if JUCE_MODAL_LOOPS_PERMITTED
    if (auto* messageManager = juce::MessageManager::getInstanceWithoutCreating())
        messageManager->runDispatchLoopUntil(millis);
    else
        juce::Thread::sleep(millis);
#else
    juce::Thread::sleep(millis);
#endif
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& componentId)
{
    if (root.getComponentID() == componentId)
        return &root;

    for (int index = 0; index < root.getNumChildComponents(); ++index)
    {
        if (auto* match = findDescendantById(*root.getChildComponent(index), componentId))
            return match;
    }

    return nullptr;
}

juce::Label& requireLabel(juce::Component& root, const juce::String& componentId)
{
    auto* label = dynamic_cast<juce::Label*>(findDescendantById(root, componentId));
    require(label != nullptr, "Missing label ID: " + componentId.toStdString());
    return *label;
}

juce::Button& requireButton(juce::Component& root, const juce::String& componentId)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, componentId));
    require(button != nullptr, "Missing button ID: " + componentId.toStdString());
    return *button;
}

juce::TabbedComponent& requireTabs(juce::Component& root)
{
    auto* tabs = dynamic_cast<juce::TabbedComponent*>(findDescendantById(root, "workspaceTabs"));
    require(tabs != nullptr, "Workspace tabs should be available in the hosted authoring shell.");
    return *tabs;
}

drs::app::AuthoringPanel& requireAuthoringPanel(juce::Component& root)
{
    auto* panel = dynamic_cast<drs::app::AuthoringPanel*>(findDescendantById(root, "authoringWorkspace"));
    require(panel != nullptr, "Authoring workspace should be available in the hosted shell.");
    return *panel;
}

std::string getLabelText(juce::Component& root, const juce::String& componentId)
{
    return requireLabel(root, componentId).getText().toStdString();
}

bool waitForWorkerToSettle(drs::engine::EngineFacade& engineFacade,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() <= deadline)
    {
        const auto& workerStatus = engineFacade.getPreparedPlaybackWorkerStatus();
        if (workerStatus.pendingWorkCount == 0 && workerStatus.inFlightWorkCount == 0)
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    const auto& workerStatus = engineFacade.getPreparedPlaybackWorkerStatus();
    return workerStatus.pendingWorkCount == 0 && workerStatus.inFlightWorkCount == 0;
}

bool waitForAutomaticPreview(drs::plugin::Processor& processor,
                             std::size_t expectedRevision,
                             std::chrono::milliseconds timeout = std::chrono::milliseconds(3000))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() <= deadline)
    {
        processor.serviceMessageThreadWork();
        const auto& status = processor.getEngineFacade().getDraftPlaybackStatus();
        if (!status.pendingPreview.active
            && status.preview.revision == expectedRevision
            && status.preview.state == "Ready")
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
}

bool waitForLabelContains(juce::Component& root,
                          const juce::String& componentId,
                          const std::string& expectedFragment,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(1500))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() <= deadline)
    {
        pumpMessages(25);
        if (requireLabel(root, componentId).getText().toStdString().find(expectedFragment) != std::string::npos)
            return true;
    }

    return requireLabel(root, componentId).getText().toStdString().find(expectedFragment) != std::string::npos;
}

bool waitForButtonEnabled(juce::Component& root,
                          const juce::String& componentId,
                          std::chrono::milliseconds timeout = std::chrono::milliseconds(1500))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() <= deadline)
    {
        pumpMessages(25);
        if (requireButton(root, componentId).isEnabled())
            return true;
    }

    return requireButton(root, componentId).isEnabled();
}

std::string buildDraftPlaybackSummary(const drs::engine::DraftPlaybackStatus& playbackStatus)
{
    return "Draft playback: draft r" + std::to_string(playbackStatus.draftRevision)
        + " | preview r" + std::to_string(playbackStatus.preview.revision)
        + " (" + playbackStatus.preview.state + ")"
        + " | published r" + std::to_string(playbackStatus.performance.revision)
        + " (" + playbackStatus.performance.state + ")";
}

void refreshAuthoringWorkspace(juce::Component& root)
{
    pumpMessages(20);
    requireAuthoringPanel(root).refreshNow();
    pumpMessages(20);
}

void exerciseHostedAuthoringPlaybackIntegration(juce::Component& root,
                                                drs::plugin::Processor& processor,
                                                const std::string& shellName)
{
    auto& tabs = requireTabs(root);
    tabs.setCurrentTabIndex(1);
    refreshAuthoringWorkspace(root);

    const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
    require(phase1Project.loaded, shellName + " should load the Phase 1 reference project before migration coverage.");
    const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
    require(migratedProject.valid, shellName + " should migrate the Phase 1 reference project before authoring playback coverage.");

    processor.replaceAuthoringProject(migratedProject.project);
    refreshAuthoringWorkspace(root);
    const auto hostedBaselinePlayback = buildDraftPlaybackSummary(processor.getEngineFacade().getDraftPlaybackStatus());
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 hostedBaselinePlayback),
            shellName + " should surface the migrated hosted draft-playback baseline in the authoring summary strip. Expected: "
                + hostedBaselinePlayback
                + " Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));

    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringPlaybackBannerLabel",
                                 "playback blocked: Import a sample and create at least one playable zone."),
            shellName + " should surface draft-playback guidance inside the workspace banner when migrated projects still lack playable zones.");
    require(!requireButton(root, "authoringPlaybackBannerPrepareButton").isVisible(),
            shellName + " should hide prepare-draft banner actions until the migrated project has a playable zone.");
    require(!requireButton(root, "authoringPlaybackBannerPublishButton").isVisible(),
            shellName + " should hide publish-draft banner actions until the migrated project has a ready draft.");

    drs::engine::RuntimeProjectSampleSource importedSampleSource;
    importedSampleSource.id = shellName + "-migrated-sine-a3";
    importedSampleSource.path = phase1Project.project.sampleSources[0].path;
    importedSampleSource.role = "imported-sustain";

    drs::engine::RuntimeProjectZoneDefinition importedZone;
    importedZone.id = shellName + "-migrated-zone-a3";
    importedZone.sampleSourceId = importedSampleSource.id;
    importedZone.displayName = shellName + " Migrated Zone A3";
    importedZone.groupId = "main";
    importedZone.articulationId = "sustain";
    importedZone.rootKey = 57;
    importedZone.keyLow = 57;
    importedZone.keyHigh = 57;
    importedZone.velocityLow = 1;
    importedZone.velocityHigh = 127;

    const auto importResult = processor.getAuthoringSession().appendImportedContent({ importedSampleSource },
                                                                                    { importedZone },
                                                                                    "Import migrated authoring shell zone");
    require(importResult.applied, shellName + " should accept imported authoring content.");
    require(processor.serviceMessageThreadWork(),
            shellName + " should sync imported authoring content into the draft-playback facade.");
    refreshAuthoringWorkspace(root);
    require(!requireButton(root, "authoringPlaybackBannerPrepareButton").isVisible(),
            shellName + " must not offer a duplicate manual prepare while the Sprint 5 Preview controller owns the build.");
    require(waitForAutomaticPreview(processor, 1),
            shellName + " should automatically prepare imported authored content through the Preview controller.");
    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 "Draft playback: draft r1 | preview r1 (Ready) | published r0 (Idle)"),
            shellName + " should surface the ready-but-unpublished draft state after preparing the imported migrated preview. Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));
    require(waitForButtonEnabled(root, "authoringPlaybackBannerPublishButton"),
            shellName + " should enable publish-draft banner actions once the latest draft preview is ready.");
    require(waitForLabelContains(root,
                                 "authoringPlaybackBannerLabel",
                                 "playback action: Publish the ready draft to the performance path."),
            shellName + " should guide the user to publish the ready migrated draft from the workspace banner.");
    require(static_cast<bool>(requireButton(root, "authoringPlaybackBannerPublishButton").onClick),
            shellName + " should expose a workspace banner publish-draft action.");
    requireButton(root, "authoringPlaybackBannerPublishButton").onClick();
    require(waitForWorkerToSettle(processor.getEngineFacade(), std::chrono::milliseconds(1500)),
            shellName + " should let imported migrated publish settle through the prepared-playback worker.");
    processor.serviceMessageThreadWork();
    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 "Draft playback: draft r1 | preview r1 (Ready) | published r1 (Active)"),
            shellName + " should surface the recovered migrated ready/active state in the authoring summary strip. Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));
    require(!findDescendantById(root, "authoringPlaybackBanner")->isVisible(),
            shellName + " should hide the workspace banner once the latest draft is active on the performance path.");

    auto selectedZone = processor.getAuthoringSession().getSelectedZone();
    require(selectedZone.has_value(), shellName + " should keep the imported migrated zone selected for edit coverage.");
    auto editedZone = *selectedZone;
    editedZone.gainDb = 2.5;
    editedZone.pan = -0.2;
    const auto editResult = processor.getAuthoringSession().updateSelectedZone(editedZone,
                                                                               "Shape migrated authoring shell zone");
    require(editResult.applied, shellName + " should accept edited authoring content.");
    require(processor.serviceMessageThreadWork(),
            shellName + " should sync edited authoring content into the draft-playback facade.");
    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 "Draft playback: draft r2 | preview r1 (Stale) | published r1 (Active)"),
            shellName + " should surface the stale-preview edited-draft state in the authoring summary strip. Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));
    require(waitForAutomaticPreview(processor, 2),
            shellName + " should automatically prepare the edited authored revision through Preview.");
    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 "Draft playback: draft r2 | preview r2 (Ready) | published r1 (Active)"),
            shellName + " should surface the ready-but-unpublished edited draft state after preview preparation. Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));
    require(waitForButtonEnabled(root, "authoringPlaybackBannerPublishButton"),
            shellName + " should re-enable publish-draft banner actions once the edited draft preview is ready.");
    requireButton(root, "authoringPlaybackBannerPublishButton").onClick();
    require(waitForWorkerToSettle(processor.getEngineFacade(), std::chrono::milliseconds(1500)),
            shellName + " should let edited migrated publish settle through the prepared-playback worker.");
    processor.serviceMessageThreadWork();
    refreshAuthoringWorkspace(root);
    require(waitForLabelContains(root,
                                 "authoringSummaryPlaybackLabel",
                                 "Draft playback: draft r2 | preview r2 (Ready) | published r2 (Active)"),
            shellName + " should surface the republished edited-draft state in the authoring summary strip. Text: "
                + getLabelText(root, "authoringSummaryPlaybackLabel"));
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        {
            drs::plugin::Processor pluginProcessor;
            pluginProcessor.prepareToPlay(44100.0, 512);
            drs::plugin::Editor pluginEditor(pluginProcessor);
            DesktopHostedComponent pluginHost(pluginEditor);
            exerciseHostedAuthoringPlaybackIntegration(pluginEditor, pluginProcessor, "Plugin shell");
        }

        {
            drs::standalone::MainComponent standalone(false);
            standalone.getProcessor().prepareToPlay(44100.0, 512);
            DesktopHostedComponent standaloneHost(standalone);
            exerciseHostedAuthoringPlaybackIntegration(standalone, standalone.getProcessor(), "Standalone shell");
        }

        std::cout << "Phase 2 authoring playback integration tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring playback integration tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
