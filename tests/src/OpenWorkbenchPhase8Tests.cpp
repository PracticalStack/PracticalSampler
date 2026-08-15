#include "drs/engine/AuthoringSession.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"
#include "shared/authoring/RoutingWorkbenchView.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <algorithm>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id)
        return &root;
    for (auto* child : root.getChildren())
        if (auto* match = findDescendantById(*child, id); match != nullptr)
            return match;
    return nullptr;
}

template <typename ComponentType>
ComponentType& requireComponent(juce::Component& root, const juce::String& id)
{
    auto* component = dynamic_cast<ComponentType*>(findDescendantById(root, id));
    require(component != nullptr, "Missing Phase 8 component: " + id.toStdString());
    return *component;
}

void openRoutingWorkbench(drs::app::AuthoringPanel& panel)
{
    auto& toggle = requireComponent<juce::Button>(panel, "authoringWorkbenchToggleButton");
    if (toggle.getButtonText() == "Show Workbench")
        toggle.onClick();
    requireComponent<juce::Button>(panel, "authoringWorkbenchRoutingTab").onClick();
}

void requireRegionInside(const juce::Rectangle<int>& content,
                         const juce::Rectangle<int>& region,
                         const std::string& message)
{
    require(!region.isEmpty() && content.contains(region), message);
}

drs::engine::RuntimeProjectModel loadRoutingFixture()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 8 requires the Phase 2 reference project.");
    auto project = loaded.project;
    require(!project.authoring.fxSlots.empty(),
            "Phase 8 fixture requires at least one curated FX slot.");

    auto& selectedFx = project.authoring.fxSlots.front();
    selectedFx.effectType = "drs.gain";
    selectedFx.effectVersion = 1;
    selectedFx.unavailable = false;
    selectedFx.legacyInert = false;
    selectedFx.parameters.clear();
    const auto* descriptor = drs::engine::findCuratedDspEffect(
        selectedFx.effectType, selectedFx.effectVersion);
    require(descriptor != nullptr && !descriptor->parameters.empty(),
            "Phase 8 fixture requires the curated Gain descriptor.");
    for (const auto& parameter : descriptor->parameters)
        selectedFx.parameters.push_back({ std::string(parameter.id), parameter.defaultValue });

    drs::engine::RuntimeProjectRoutingBusDefinition bus;
    bus.id = "phase8-master";
    bus.displayName = "Instrument Master";
    bus.inputSourceId = "master";
    for (std::size_t index = 0; index < std::min<std::size_t>(2, project.authoring.fxSlots.size()); ++index)
        bus.fxSlotIds.push_back(project.authoring.fxSlots[index].id);
    project.authoring.routingBuses = { std::move(bus) };
    return project;
}

void qualifyWideHierarchyAndTransactions()
{
    drs::engine::AuthoringSession session(loadRoutingFixture());
    auto panel = std::make_unique<drs::app::AuthoringPanel>(
        session, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    panel->setSize(1120, 800);
    panel->setVisible(true);
    panel->resized();
    panel->reloadFromSession();
    openRoutingWorkbench(*panel);

    auto* content = findDescendantById(*panel, "authoringRoutingContent");
    require(content != nullptr, "Phase 8 must retain the Routing content compatibility ID.");
    const auto layout = panel->getRoutingWorkbenchLayoutSnapshot();
    require(layout.mode == drs::app::authoring::RoutingWorkbenchView::LayoutMode::wideTwoRegion,
            "The expanded target shell must use the two-region Routing layout.");
    requireRegionInside(content->getLocalBounds(), layout.signalPathRegion,
                        "The bus and signal-path region must remain bounded.");
    requireRegionInside(content->getLocalBounds(), layout.selectedFxRegion,
                        "The selected-FX detail region must remain bounded.");
    require(layout.signalPathRegion.getRight() < layout.selectedFxRegion.getX(),
            "Wide Routing must order signal path before selected-FX detail.");

    require(requireComponent<juce::Label>(*panel, "authoringRoutingSectionLabel")
                .getText() == "Bus & Signal Path"
                && requireComponent<juce::Label>(*panel, "authoringFxSectionLabel")
                    .getText() == "Selected Insert"
                && requireComponent<juce::Label>(*panel, "authoringRoutingSignalPathHeading")
                    .getText() == "Ordered Signal Path"
                && requireComponent<juce::Label>(*panel, "authoringFxParameterHeading")
                    .getText() == "Parameter & Macro Control",
            "Phase 8 must expose its routing hierarchy in text.");
    require(requireComponent<juce::Label>(*panel, "authoringRoutingSignalPathLabel")
                .getText().contains("Input")
                && requireComponent<juce::Label>(*panel, "authoringRoutingSignalPathLabel")
                    .getText().contains("Output"),
            "The selected bus must expose an ordered input-to-output path without diagnostics.");
    require(requireComponent<juce::Label>(*panel, "authoringFxContextLabel")
                .getText().contains("Owner")
                && requireComponent<juce::Label>(*panel, "authoringFxContextLabel")
                    .getText().contains("Insert"),
            "Selected FX detail must expose owner and insert position.");
    require(requireComponent<juce::Label>(*panel, "authoringFxMacroAssignmentSummary")
                .getText().contains("Macro control"),
            "Selected parameter detail must expose Macro-control assignment status.");

    const auto scopeFocus = requireComponent<juce::ComboBox>(
        *panel, "authoringDspScopeSelector").getExplicitFocusOrder();
    const auto busFocus = requireComponent<juce::ComboBox>(
        *panel, "authoringRoutingSelector").getExplicitFocusOrder();
    const auto insertFocus = requireComponent<juce::ComboBox>(
        *panel, "authoringRoutingInsertTwoSelector").getExplicitFocusOrder();
    const auto fxFocus = requireComponent<juce::ComboBox>(
        *panel, "authoringFxSelector").getExplicitFocusOrder();
    const auto parameterFocus = requireComponent<juce::Slider>(
        *panel, "authoringFxParameterSlider").getExplicitFocusOrder();
    const auto actionFocus = requireComponent<juce::Button>(
        *panel, "authoringFxAddButton").getExplicitFocusOrder();
    require(scopeFocus < busFocus && busFocus < insertFocus && insertFocus < fxFocus
                && fxFocus < parameterFocus && parameterFocus < actionFocus,
            "Routing focus order must follow scope/path, selected FX, parameter, then secondary actions.");
    require(requireComponent<juce::Button>(*panel, "authoringFxDeleteButton")
                .findColour(juce::TextButton::textColourOffId)
                == drs::app::authoring::visual::error,
            "Delete must remain visually separated as a destructive FX action.");

    auto& bypass = requireComponent<juce::Button>(*panel, "authoringFxBypassedToggle");
    require(bypass.isEnabled(), "The selected FX must expose its bypass transaction.");
    const auto revisionBeforeBypass = session.getDocumentState().revision;
    const auto bypassBefore = bypass.getToggleState();
    bypass.setToggleState(!bypassBefore, juce::dontSendNotification);
    bypass.onClick();
    require(session.getDocumentState().revision == revisionBeforeBypass + 1
                && session.getProject().authoring.fxSlots.front().bypassed != bypassBefore,
            "Bypass must commit through one existing FX transaction.");
    require(session.undo().applied, "Bypass must remain undoable.");
    panel->reloadFromSession();
    require(session.getProject().authoring.fxSlots.front().bypassed == bypassBefore,
            "Undo must restore FX bypass.");
    require(session.redo().applied, "Bypass must remain redoable.");
    panel->reloadFromSession();
    require(session.getProject().authoring.fxSlots.front().bypassed != bypassBefore,
            "Redo must restore the edited bypass state.");

    auto& parameterSlider = requireComponent<juce::Slider>(
        *panel, "authoringFxParameterSlider");
    require(parameterSlider.isEnabled(), "The selected curated FX must expose its parameter value.");
    const auto revisionBeforeParameter = session.getDocumentState().revision;
    const auto nextValue = parameterSlider.getValue() == parameterSlider.getMaximum()
        ? parameterSlider.getMinimum()
        : std::min(parameterSlider.getMaximum(), parameterSlider.getValue()
                                              + parameterSlider.getInterval() * 5.0);
    parameterSlider.setValue(nextValue, juce::dontSendNotification);
    parameterSlider.onDragEnd();
    require(session.getDocumentState().revision == revisionBeforeParameter + 1,
            "Parameter edits must continue through one existing transaction.");
}

void qualifyNormalAndShortLayouts()
{
    auto project = loadRoutingFixture();
    drs::engine::AuthoringSession normalSession(project);
    auto normal = std::make_unique<drs::app::AuthoringPanel>(
        normalSession, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::compact);
    normal->setSize(820, 700);
    normal->setVisible(true);
    normal->resized();
    normal->reloadFromSession();
    openRoutingWorkbench(*normal);
    require(normal->getRoutingWorkbenchLayoutSnapshot().mode
                == drs::app::authoring::RoutingWorkbenchView::LayoutMode::normalBalanced,
            "The standard compact shell must use the balanced Routing layout.");
    for (const auto& id : { "authoringFxNameEditor", "authoringFxOwnerSelector",
                            "authoringFxParameterSlider", "authoringFxAssignMacroButton",
                            "authoringFxAddButton", "authoringFxDeleteButton" })
    {
        auto* component = findDescendantById(*normal, id);
        require(component != nullptr && component->isVisible()
                    && !component->getBounds().isEmpty(),
                std::string("Normal Routing must retain every operation: ") + id);
    }
    require(findDescendantById(*normal, "authoringZoneMap")->getHeight()
                >= drs::app::authoring::minimumMapVisibleHeight,
            "Normal Routing must preserve the protected map height.");

    drs::engine::AuthoringSession shortSession(std::move(project));
    auto shortPanel = std::make_unique<drs::app::AuthoringPanel>(
        shortSession, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    shortPanel->setSize(900, 564);
    shortPanel->setVisible(true);
    shortPanel->resized();
    shortPanel->reloadFromSession();
    openRoutingWorkbench(*shortPanel);
    const auto shortLayout = shortPanel->getRoutingWorkbenchLayoutSnapshot();
    auto& viewport = requireComponent<juce::Viewport>(*shortPanel, "authoringRoutingViewport");
    require(shortLayout.mode
                == drs::app::authoring::RoutingWorkbenchView::LayoutMode::compactStacked
                && shortLayout.signalPathRegion.getBottom() < shortLayout.selectedFxRegion.getY()
                && viewport.getVerticalScrollBar().isVisible(),
            "Short hosts must expose a scroll-safe signal-path then selected-FX sequence.");
    require(findDescendantById(*shortPanel, "authoringZoneMap")->getHeight()
                >= drs::app::authoring::minimumMapVisibleHeight,
            "Short-host Routing must preserve the protected map height.");
}

void qualifyEmptyAndWarningStates()
{
    auto emptyProject = loadRoutingFixture();
    emptyProject.authoring.routingBuses.clear();
    drs::engine::AuthoringSession emptySession(std::move(emptyProject));
    auto emptyPanel = std::make_unique<drs::app::AuthoringPanel>(
        emptySession, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    emptyPanel->setSize(1120, 800);
    emptyPanel->setVisible(true);
    emptyPanel->resized();
    emptyPanel->reloadFromSession();
    openRoutingWorkbench(*emptyPanel);
    require(requireComponent<juce::Label>(*emptyPanel, "authoringRoutingEmptyState")
                .isVisible()
                && requireComponent<juce::Label>(*emptyPanel, "authoringFxEmptyState")
                    .isVisible()
                && requireComponent<juce::ComboBox>(*emptyPanel, "authoringDspScopeSelector")
                    .isVisible()
                && requireComponent<juce::Button>(*emptyPanel, "authoringFxAddButton").isEnabled(),
            "Empty Routing must explain missing scope while retaining the next valid Add Insert action.");

    auto warningProject = loadRoutingFixture();
    warningProject.authoring.fxSlots.front().legacyInert = true;
    drs::engine::AuthoringSession warningSession(std::move(warningProject));
    auto warningPanel = std::make_unique<drs::app::AuthoringPanel>(
        warningSession, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    warningPanel->setSize(1120, 800);
    warningPanel->setVisible(true);
    warningPanel->resized();
    warningPanel->reloadFromSession();
    openRoutingWorkbench(*warningPanel);
    require(requireComponent<juce::Label>(*warningPanel, "authoringFxDiagnosticsLabel")
                .findColour(juce::Label::textColourId)
                == drs::app::authoring::visual::warning,
            "Legacy or unavailable FX diagnostics must use the shared warning role adjacent to detail.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyWideHierarchyAndTransactions();
        qualifyNormalAndShortLayouts();
        qualifyEmptyAndWarningStates();
        std::cout << "Open Workbench Phase 8 Routing qualification passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 8 Routing qualification failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
