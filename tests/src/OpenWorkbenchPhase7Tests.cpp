#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/MacroWorkbenchView.h"
#include "shared/authoring/OpenWorkbenchVisualSystem.h"
#include "shared/authoring/RepeatedStructureList.h"

#include <juce_gui_extra/juce_gui_extra.h>

#include <iostream>
#include <stdexcept>
#include <string>

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
    require(component != nullptr, "Missing Phase 7 component: " + id.toStdString());
    return *component;
}

void openMacroWorkbench(drs::app::AuthoringPanel& panel)
{
    auto& toggle = requireComponent<juce::Button>(panel, "authoringWorkbenchToggleButton");
    if (toggle.getButtonText() == "Show Workbench")
        toggle.onClick();
    requireComponent<juce::Button>(panel, "authoringWorkbenchMacrosTab").onClick();
}

void requireRegionInside(const juce::Rectangle<int>& content,
                         const juce::Rectangle<int>& region,
                         const std::string& message)
{
    require(!region.isEmpty() && content.contains(region), message);
}

drs::engine::RuntimeProjectModel loadFixtureProject()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 7 requires the Phase 2 reference project.");
    return loaded.project;
}

void qualifyWideHierarchyAndTargetTransactions()
{
    drs::engine::AuthoringSession session(loadFixtureProject());
    auto panel = std::make_unique<drs::app::AuthoringPanel>(
        session, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    panel->setSize(1120, 800);
    panel->setVisible(true);
    panel->resized();
    panel->reloadFromSession();
    openMacroWorkbench(*panel);

    auto* content = findDescendantById(*panel, "authoringMacroContent");
    require(content != nullptr, "Phase 7 must retain the macro content compatibility ID.");
    const auto layout = panel->getMacroWorkbenchLayoutSnapshot();
    require(layout.mode == drs::app::authoring::MacroWorkbenchView::LayoutMode::wideThreeRegion,
            "The expanded target shell must use the three-region Macro layout.");
    requireRegionInside(content->getLocalBounds(), layout.listRegion,
                        "The ordered Macro list region must remain bounded.");
    requireRegionInside(content->getLocalBounds(), layout.definitionRegion,
                        "The Macro definition region must remain bounded.");
    requireRegionInside(content->getLocalBounds(), layout.assignmentsRegion,
                        "The assigned-target region must remain bounded.");
    require(layout.listRegion.getRight() < layout.definitionRegion.getX()
                && layout.definitionRegion.getRight() < layout.assignmentsRegion.getX(),
            "Wide Macro regions must be ordered list, definition, then assigned targets.");

    require(requireComponent<juce::Label>(*panel, "authoringMacroIdentityHeading").getText()
                == "Identity & Host"
                && requireComponent<juce::Label>(*panel, "authoringMacroRangeHeading").getText()
                    == "Range"
                && requireComponent<juce::Label>(*panel, "authoringMacroAssignmentsHeading").getText()
                    == "Assigned Targets",
            "Phase 7 must expose the intended definition-and-assignment hierarchy in text.");
    require(requireComponent<juce::Label>(*panel, "authoringMacroRangeStatusLabel")
                .getText().contains("Default"),
            "The selected Macro range must expose explicit numeric validation text.");

    auto& macroList = requireComponent<drs::app::authoring::RepeatedStructureList>(
        *panel, "authoringMacroList");
    auto& assignmentList = requireComponent<drs::app::authoring::RepeatedStructureList>(
        *panel, "authoringMacroAssignmentList");
    auto selectedMacro = session.getSelectedMacro();
    require(selectedMacro.has_value() && assignmentList.getRowCount()
                == static_cast<int>(selectedMacro->targets.size()),
            "Assigned Targets must show every target on the selected Macro.");
    require(assignmentList.getDescription().contains("target")
                && requireComponent<juce::Label>(*panel, "authoringMacroAssignmentDetailLabel")
                    .getText().contains("Target"),
            "The selected assignment must retain target identity and mapping context.");

    const auto listFocus = macroList.getListBox().getExplicitFocusOrder();
    const auto nameFocus = requireComponent<juce::TextEditor>(
        *panel, "authoringMacroNameEditor").getExplicitFocusOrder();
    const auto rangeFocus = requireComponent<juce::Slider>(
        *panel, "authoringMacroMaxSlider").getExplicitFocusOrder();
    const auto assignmentListFocus = assignmentList.getListBox().getExplicitFocusOrder();
    const auto assignmentFocus = requireComponent<juce::ComboBox>(
        *panel, "authoringMacroAssignmentSelector").getExplicitFocusOrder();
    const auto secondaryFocus = requireComponent<juce::Button>(
        *panel, "authoringMacroCreateButton").getExplicitFocusOrder();
    require(listFocus < nameFocus && nameFocus < rangeFocus
                && rangeFocus < assignmentListFocus
                && assignmentListFocus < assignmentFocus
                && assignmentFocus < secondaryFocus,
            "Macro focus order must follow list, definition/range, assignments, then secondary actions.");
    require(requireComponent<juce::Button>(*panel, "authoringMacroDeleteButton")
                .findColour(juce::TextButton::textColourOffId)
                == drs::app::authoring::visual::error,
            "Delete must remain visually separated as a destructive Macro action.");

    const auto selectedMacroId = selectedMacro->id;
    const auto targetCountBeforeAdd = selectedMacro->targets.size();
    const auto revisionBeforeAdd = session.getDocumentState().revision;
    auto& addTarget = requireComponent<juce::Button>(*panel, "authoringMacroAssignmentAddButton");
    require(addTarget.isEnabled(), "A selected Macro must enable Add Target.");
    addTarget.onClick();
    selectedMacro = session.getSelectedMacro();
    require(selectedMacro.has_value() && selectedMacro->id == selectedMacroId
                && selectedMacro->targets.size() == targetCountBeforeAdd + 1
                && session.getDocumentState().revision == revisionBeforeAdd + 1,
            "Add Target must use one existing document transaction and preserve Macro selection.");
    require(assignmentList.getRowCount() == static_cast<int>(targetCountBeforeAdd + 1)
                && assignmentList.getSelectedIndex() == static_cast<int>(targetCountBeforeAdd),
            "The new target must appear selected without losing Macro scope.");

    require(session.undo().applied, "Add Target must remain undoable.");
    panel->reloadFromSession();
    require(session.getSelectedMacro().has_value()
                && session.getSelectedMacro()->targets.size() == targetCountBeforeAdd,
            "Undo must restore the previous Macro target list.");
    require(session.redo().applied, "Add Target must remain redoable.");
    panel->reloadFromSession();
    require(session.getSelectedMacro().has_value()
                && session.getSelectedMacro()->targets.size() == targetCountBeforeAdd + 1,
            "Redo must restore the added Macro target.");

    auto& removeTarget = requireComponent<juce::Button>(
        *panel, "authoringMacroAssignmentRemoveButton");
    require(removeTarget.isEnabled(), "A selected assignment must enable Remove Target.");
    removeTarget.onClick();
    require(session.getSelectedMacro().has_value()
                && session.getSelectedMacro()->targets.size() == targetCountBeforeAdd,
            "Remove Target must remove only the selected assignment.");

    const auto selectedIndexBeforeZoneChange = macroList.getSelectedIndex();
    const auto& zones = session.getProject().authoring.zones;
    if (zones.size() > 1)
    {
        require(session.selectZone(zones.back().id).applied,
                "Phase 7 fixture must allow a zone selection change.");
        panel->refreshNow();
        require(macroList.getSelectedIndex() == selectedIndexBeforeZoneChange
                    && session.getSelectedMacro().has_value()
                    && session.getSelectedMacro()->id == selectedMacroId,
                "Zone selection refreshes must not disturb Macro selection.");
    }
}

void qualifyNormalAndShortLayouts()
{
    auto project = loadFixtureProject();
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
    openMacroWorkbench(*normal);
    const auto normalLayout = normal->getMacroWorkbenchLayoutSnapshot();
    require(normalLayout.mode
                == drs::app::authoring::MacroWorkbenchView::LayoutMode::normalListDetail,
            "The compact target shell must use the list/detail Macro layout.");
    auto& normalViewport = requireComponent<juce::Viewport>(*normal, "authoringMacroViewport");
    require(!normalViewport.getVerticalScrollBar().isVisible(),
            "The standard compact shell must keep the complete Macro editor above the fold.");

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
    openMacroWorkbench(*shortPanel);
    const auto shortLayout = shortPanel->getMacroWorkbenchLayoutSnapshot();
    auto& shortViewport = requireComponent<juce::Viewport>(*shortPanel, "authoringMacroViewport");
    require(shortLayout.mode
                == drs::app::authoring::MacroWorkbenchView::LayoutMode::compactStacked
                && shortLayout.listRegion.getBottom() < shortLayout.definitionRegion.getY()
                && shortLayout.definitionRegion.getBottom() < shortLayout.assignmentsRegion.getY()
                && shortViewport.getVerticalScrollBar().isVisible(),
            "Short hosts must use a scroll-safe list, definition, assignment sequence.");
}

void qualifyEmptyStates()
{
    auto project = loadFixtureProject();
    project.authoring.macros.clear();
    drs::engine::AuthoringSession session(std::move(project));
    auto panel = std::make_unique<drs::app::AuthoringPanel>(
        session, drs::app::AuthoringPanel::WaveformPreviewProvider {},
        drs::app::AuthoringPanel::AuthoringPreviewStatusProvider {},
        drs::app::AuthoringPanel::ImportResponsivenessProvider {},
        drs::app::AuthoringPanel::LayoutMode::expanded);
    panel->setSize(1120, 800);
    panel->setVisible(true);
    panel->resized();
    panel->reloadFromSession();
    openMacroWorkbench(*panel);

    require(requireComponent<juce::Label>(*panel, "authoringMacroDefinitionEmptyState")
                .isVisible()
                && requireComponent<juce::Label>(*panel, "authoringMacroDefinitionEmptyState")
                    .getText().contains("Create or select")
                && requireComponent<drs::app::authoring::RepeatedStructureList>(
                       *panel, "authoringMacroAssignmentList")
                    .getViewModel().emptyStateText.find("Create or select") != std::string::npos,
            "No-Macro state must explain the next valid action in definition and assignment regions.");
    require(requireComponent<juce::Button>(*panel, "authoringMacroCreateButton").isEnabled()
                && !requireComponent<juce::Button>(
                       *panel, "authoringMacroAssignmentAddButton").isEnabled(),
            "No-Macro state must retain Create while disabling target operations.");
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyWideHierarchyAndTargetTransactions();
        qualifyNormalAndShortLayouts();
        qualifyEmptyStates();
        std::cout << "Open Workbench Phase 7 Macro qualification passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 7 Macro qualification failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
