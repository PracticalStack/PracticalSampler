#include "drs/engine/AuthoringSession.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/PerformancePanel.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <chrono>
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

void requireComponentPresent(juce::Component& root,
                             const juce::String& componentId,
                             const bool expectedPresent,
                             const std::string& message)
{
    const auto present = findDescendantById(root, componentId) != nullptr;
    require(present == expectedPresent, message);
}

void requireLabelContains(juce::Component& root,
                          const juce::String& componentId,
                          const std::string& expectedFragment,
                          const std::string& message)
{
    const auto text = requireLabel(root, componentId).getText().toStdString();
    require(text.find(expectedFragment) != std::string::npos, message + " Text: " + text);
}

void refreshPanel(drs::app::PerformancePanel& panel)
{
    pumpMessages(20);
    panel.refreshNow();
    pumpMessages(20);
}

bool waitForWorkerToSettle(drs::engine::EngineFacade& engineFacade,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
{
    return engineFacade.waitForPreparedPlaybackIdle(timeout);
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;

        drs::engine::EngineFacade engineFacade;
        engineFacade.resetSessionStateToDefault();
        drs::app::PerformancePanel panel(engineFacade);
        panel.setSize(1280, 960);
        DesktopHostedComponent host(panel);
        refreshPanel(panel);

        requireComponentPresent(panel,
                                "performanceArtworkPanel",
                                true,
                                "Performance panel should expose an artwork surface above the controls.");
        requireComponentPresent(panel,
                                "performanceLoadDefaultButton",
                                false,
                                "Performance panel should no longer expose the Load Default demo button.");
        requireComponentPresent(panel,
                                "performanceLoadLeadButton",
                                false,
                                "Performance panel should no longer expose the Load Lead demo button.");
        requireComponentPresent(panel,
                                "performanceArticulationLabel",
                                false,
                                "Performance panel should no longer expose the demo articulation strip.");
        requireComponentPresent(panel,
                                "performancePatchStatusLabel",
                                false,
                                "Performance panel should remove the old internal patch-status block.");
        requireComponentPresent(panel,
                                "performancePreviewStatusLabel",
                                false,
                                "Performance panel should remove the old preview-status block.");
        requireComponentPresent(panel,
                                "performanceKeyboardHintLabel",
                                false,
                                "Performance panel should remove the old keyboard hint label.");
        requireComponentPresent(panel,
                                "performanceDiagnosticsToggle",
                                false,
                                "Performance panel should remove the old diagnostics toggle from the player surface.");
        requireLabelContains(panel,
                             "performanceLoadIndicatorLabel",
                             "Publish Ready r0",
                             "The facade-only UI fixture should show its prepared bootstrap revision before a host acknowledges activation.");
        requireLabelContains(panel,
                             "performanceMacroStripLabel",
                             "Instrument Controls",
                             "Performance panel should label the control area as instrument controls.");
        auto& instrumentControlsToggle = requireButton(
            panel, "performanceMacroStripToggleButton");
        require(instrumentControlsToggle.getButtonText() == "Show Controls",
                "Instrument Controls should start collapsed with an explicit expansion action.");
        auto* referenceMacroSlider = findDescendantById(panel, "performanceMacroSlider.tone");
        require(referenceMacroSlider != nullptr && !referenceMacroSlider->isVisible(),
                "Reference Instrument Controls should remain hidden on initial plug-in presentation.");
        instrumentControlsToggle.onClick();
        require(instrumentControlsToggle.getButtonText() == "Hide Controls"
                    && referenceMacroSlider->isVisible(),
                "Explicitly showing Instrument Controls should reveal its player-facing controls. button="
                    + instrumentControlsToggle.getButtonText().toStdString()
                    + " visible=" + std::to_string(referenceMacroSlider->isVisible())
                    + " bounds=" + referenceMacroSlider->getBounds().toString().toStdString());
        referenceMacroSlider->grabKeyboardFocus();
        instrumentControlsToggle.onClick();
        require(instrumentControlsToggle.getButtonText() == "Show Controls"
                    && !referenceMacroSlider->isVisible()
                    && referenceMacroSlider->getBounds().isEmpty(),
                "Collapsing Instrument Controls should hide and release the control content area.");
        require(juce::Component::getCurrentlyFocusedComponent() == &instrumentControlsToggle,
                "Collapsing focused Instrument Controls should return keyboard focus to the disclosure button.");
        panel.refreshNow();
        require(instrumentControlsToggle.getButtonText() == "Show Controls"
                    && !referenceMacroSlider->isVisible(),
                "Performance refreshes should preserve the collapsed Instrument Controls state.");
        instrumentControlsToggle.onClick();
        require(instrumentControlsToggle.getButtonText() == "Hide Controls"
                    && referenceMacroSlider->isVisible(),
                "Expanding Instrument Controls should restore its player-facing controls.");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before performance UI migration coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before performance UI migration coverage runs.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept the migrated project for performance UI coverage.");
        require(engineFacade.reopenDraftPlaybackProject(0),
                "Engine facade should reopen against the migrated project for performance UI coverage.");
        refreshPanel(panel);

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Migrated project without imported zones should queue preview validation.");
        require(engineFacade.publishCurrentDraft(),
                "Migrated project without imported zones should queue publish validation.");
        require(engineFacade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(1500)),
                "Migrated project Preview and Publish validation should settle asynchronously.");
        refreshPanel(panel);

        requireLabelContains(panel,
                             "performanceLoadIndicatorLabel",
                             "Publish Failed",
                             "The compact Performance status chip should keep a failed Publish fully visible.");
        require(requireLabel(panel, "performanceLoadIndicatorLabel").findColour(
                    juce::Label::backgroundColourId) == juce::Colour::fromRGB(172, 41, 41),
                "A failed Publish must use the danger colour even when a fallback instrument remains loaded.");
        require(requireLabel(panel, "performanceLoadIndicatorLabel").getDescription().contains(
                    "no-playable-zones"),
                "The compact failed Publish chip must retain the structured finding in its accessible description.");

        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "performance-ui-migrated-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "performance-ui-migrated-zone-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Performance UI Migrated Zone A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto importResult = migratedSession.appendImportedContent({ importedSampleSource },
                                                                        { importedZone },
                                                                        "Import migrated performance UI zone");
        require(importResult.applied, "Migrated project should accept imported authoring content for performance UI coverage.");
        require(migratedSession.selectZone(importedZone.id).applied,
                "Migrated project should explicitly select its imported zone before editing it.");
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept the imported migrated project.");
        require(engineFacade.stageDraftRevision(importResult.documentState.revision),
                "Engine facade should stage the imported migrated draft revision.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Imported migrated project should prepare preview successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Imported migrated preview should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        require(engineFacade.publishCurrentDraft(),
                "Imported migrated project should publish successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Imported migrated publish should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        refreshPanel(panel);

        requireLabelContains(panel,
                             "performanceLoadIndicatorLabel",
                             "Publish Ready r1",
                             "The compact Performance status chip should expose the recovered published revision without clipping.");

        auto editedZone = *migratedSession.getSelectedZone();
        editedZone.gainDb = 2.5;
        editedZone.pan = -0.2;
        const auto editResult = migratedSession.updateSelectedZone(editedZone,
                                                                   "Shape migrated performance UI zone");
        require(editResult.applied, "Migrated project should accept edited authoring content for performance UI coverage.");
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept the edited migrated project.");
        require(engineFacade.stageDraftRevision(editResult.documentState.revision),
                "Engine facade should stage the edited migrated draft revision.");
        refreshPanel(panel);

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Edited migrated draft should prepare preview successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Edited migrated preview should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        require(engineFacade.publishCurrentDraft(),
                "Edited migrated draft should publish successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Edited migrated publish should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        refreshPanel(panel);

        requireLabelContains(panel,
                             "performanceLoadIndicatorLabel",
                             "Publish Ready r2",
                             "Performance panel should advance the compact publish indicator when the edited draft is republished.");

        const auto phase2ProjectLoad = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2ProjectLoad.loaded,
                "Phase 2 reference project must load before Sprint 5 mixer coverage runs.");

        auto mixedExposureProject = phase2ProjectLoad.project;
        mixedExposureProject.projectId += "-performance-mixer-ui";
        mixedExposureProject.authoring.macros = {
            { "layer-blend", "Layer Blend", 0.42, 0.0, 1.0, {}, true },
            { "pedal-helper", "Pedal Helper", 0.18, 0.0, 1.0, {}, false }
        };
        require(engineFacade.replaceDraftPlaybackAuthoringProject(mixedExposureProject),
                "Engine facade should accept the mixed-exposure project for performance mixer coverage.");
        require(engineFacade.reopenDraftPlaybackProject(0),
                "Engine facade should reopen the mixed-exposure project for performance mixer coverage.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Mixed-exposure project should prepare preview before publish.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Mixed-exposure preview should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        require(engineFacade.publishCurrentDraft(),
                "Mixed-exposure project should publish successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Mixed-exposure publish should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        const auto mixedActivation = engineFacade.authorizePerformanceActivation();
        require(mixedActivation != nullptr,
                "Mixed-exposure publish should authorize an active performance binding.");
        require(engineFacade.acknowledgePerformanceActivation(mixedActivation),
                "Mixed-exposure publish should acknowledge the active performance binding.");
        refreshPanel(panel);

        requireLabelContains(panel,
                             "performanceMacroStripLabel",
                             "Instrument Controls | 1 Exposed",
                             "Performance panel should switch into mixer mode when published exposed controls are active.");
        requireLabelContains(panel,
                             "performanceMixerNameLabel.layer-blend",
                             "Layer Blend",
                             "Performance mixer should expose the published authored control label on the active slot.");
        requireComponentPresent(panel,
                                "performanceMixerNameLabel.pedal-helper",
                                false,
                                "Performance mixer should hide assigned helper controls from the default player surface.");
        requireLabelContains(panel,
                             "statusMacroNameLabel.motion",
                             "Pedal Helper",
                             "Diagnostics should continue to expose hidden helper controls through the full published macro table.");
        require(!requireLabel(panel, "performanceMixerEmptyStateLabel").isVisible(),
                "Performance mixer empty state should stay hidden while at least one exposed control is available.");
        auto* publishedLayerControl = findDescendantById(
            panel, "performanceMixerNameLabel.layer-blend");
        require(publishedLayerControl != nullptr && publishedLayerControl->isShowing(),
                "Published Instrument Controls should be showing before the panel is collapsed.");
        instrumentControlsToggle.onClick();
        require(instrumentControlsToggle.getButtonText() == "Show Controls"
                    && !publishedLayerControl->isShowing(),
                "Collapsing Instrument Controls should hide the published packaged-instrument mixer.");
        refreshPanel(panel);
        require(instrumentControlsToggle.getButtonText() == "Show Controls"
                    && !publishedLayerControl->isShowing(),
                "Published topology refreshes should preserve the collapsed Instrument Controls state.");
        instrumentControlsToggle.onClick();
        require(instrumentControlsToggle.getButtonText() == "Hide Controls"
                    && publishedLayerControl->isShowing(),
                "Expanding Instrument Controls should restore the published packaged-instrument mixer.");

        auto hiddenOnlyProject = phase2ProjectLoad.project;
        hiddenOnlyProject.projectId += "-performance-mixer-hidden-only";
        hiddenOnlyProject.authoring.macros = {
            { "pedal-noise", "Pedal Noise", 0.25, 0.0, 1.0, {}, false },
            { "release-helper", "Release Helper", 0.12, 0.0, 1.0, {}, false }
        };
        require(engineFacade.replaceDraftPlaybackAuthoringProject(hiddenOnlyProject),
                "Engine facade should accept the hidden-only helper project for performance mixer coverage.");
        require(engineFacade.reopenDraftPlaybackProject(0),
                "Engine facade should reopen the hidden-only helper project for performance mixer coverage.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Hidden-only helper project should prepare preview before publish.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Hidden-only helper preview should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        require(engineFacade.publishCurrentDraft(),
                "Hidden-only helper project should publish successfully.");
        require(waitForWorkerToSettle(engineFacade, std::chrono::milliseconds(1500)),
                "Hidden-only helper publish should settle through the prepared-playback worker.");
        engineFacade.serviceBackgroundWork();
        const auto hiddenOnlyActivation = engineFacade.authorizePerformanceActivation();
        require(hiddenOnlyActivation != nullptr,
                "Hidden-only helper publish should authorize an active performance binding.");
        require(engineFacade.acknowledgePerformanceActivation(hiddenOnlyActivation),
                "Hidden-only helper publish should acknowledge the active performance binding.");
        refreshPanel(panel);

        requireLabelContains(panel,
                             "performanceMacroStripLabel",
                             "Instrument Controls | None Exposed",
                             "Performance panel should call out when a published instrument exposes no player-facing controls.");
        requireLabelContains(panel,
                             "performanceMixerEmptyStateLabel",
                             "This instrument publishes no exposed performance controls.",
                             "Performance mixer should render an explicit empty state when only hidden helpers are published.");
        require(requireLabel(panel, "performanceMixerEmptyStateLabel").isVisible(),
                "Performance mixer empty state should become visible when all published controls are hidden.");
        requireComponentPresent(panel,
                                "performanceMixerNameLabel.layer-blend",
                                false,
                                "Performance mixer should remove the prior exposed control when the new publication exposes none.");
        requireComponentPresent(panel,
                                "performanceMixerNameLabel.pedal-helper",
                                false,
                                "Performance mixer should keep hidden helper slots off the player surface.");
        requireLabelContains(panel,
                             "statusMacroNameLabel.tone",
                             "Pedal Noise",
                             "Diagnostics should continue to expose the first hidden helper binding.");
        requireLabelContains(panel,
                             "statusMacroNameLabel.motion",
                             "Release Helper",
                             "Diagnostics should continue to expose the second hidden helper binding.");

        std::cout << "Phase 2 performance UI tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 performance UI tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
