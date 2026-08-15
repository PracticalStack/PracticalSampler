#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"
#include "shared/AuthoringPanel.h"
#include "shared/authoring/AuthoringWorkspaceLayout.h"
#include "shared/authoring/WorkbenchLayoutState.h"
#include "shared/authoring/WorkbenchSplitter.h"

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

juce::Button& requireButton(juce::Component& root, const juce::String& id)
{
    auto* button = dynamic_cast<juce::Button*>(findDescendantById(root, id));
    require(button != nullptr, "Missing button: " + id.toStdString());
    return *button;
}

juce::Component& requireComponent(juce::Component& root, const juce::String& id)
{
    auto* component = findDescendantById(root, id);
    require(component != nullptr, "Missing component: " + id.toStdString());
    return *component;
}

void qualifyLayoutState()
{
    using namespace drs::app::authoring;
    WorkbenchLayoutState state;
    require(state.getSizeMode() == WorkbenchSizeMode::collapsed
                && state.resolveHeight(600, 160, 8) == WorkbenchLayoutState::collapsedHeight,
            "Workbench state must begin as a collapsed tab rail.");

    state.setOpen(true);
    state.suggestHeightForTab(WorkbenchTab::waveform);
    require(state.getSizeMode() == WorkbenchSizeMode::standard
                && state.getRememberedHeight() == WorkbenchLayoutState::standardDefaultHeight,
            "Waveform must suggest the standard workbench height.");
    state.suggestHeightForTab(WorkbenchTab::macros);
    require(state.getSizeMode() == WorkbenchSizeMode::focused
                && state.getRememberedHeight() == WorkbenchLayoutState::focusedDefaultHeight,
            "Macro editing must suggest the focused workbench height before user sizing.");

    state.setUserHeight(236);
    state.suggestHeightForTab(WorkbenchTab::routing);
    require(state.getRememberedHeight() == 236 && state.hasUserHeight(),
            "Tab suggestions must not override a direct user height.");
    require(state.resolveHeight(390, 160, 8) == 222,
            "A short host must clamp the workbench to preserve map height and spacing.");
    require(state.getRememberedHeight() == 236,
            "Host clamping must not overwrite the remembered user height.");

    state.requestFocused();
    require(state.getSizeMode() == WorkbenchSizeMode::focused
                && state.getRememberedHeight() == WorkbenchLayoutState::focusedDefaultHeight,
            "Focused size must be directly requestable.");
    state.setOpen(false);
    require(state.getSizeMode() == WorkbenchSizeMode::collapsed
                && state.getRememberedHeight() == WorkbenchLayoutState::focusedDefaultHeight,
            "Collapsing must retain the last expanded height.");
}

void qualifyExpandedWorkbench()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 3 qualification requires the reference project.");
    drs::engine::AuthoringSession session(loaded.project);
    drs::app::AuthoringPanel panel(session, {}, {}, {}, drs::app::AuthoringPanel::LayoutMode::expanded);
    panel.setSize(1120, 800);
    panel.setVisible(true);
    panel.resized();
    panel.reloadFromSession();

    auto& region = requireComponent(panel, "authoringWorkbench");
    auto& map = requireComponent(panel, "authoringZoneMap");
    auto& splitter = dynamic_cast<drs::app::authoring::WorkbenchSplitter&>(
        requireComponent(panel, "authoringWorkbenchSplitter"));
    require(region.getHeight() == drs::app::authoring::WorkbenchLayoutState::standardDefaultHeight,
            "Expanded Waveform must begin in the 232 px Standard state.");
    require(splitter.isVisible() && splitter.getHeight() == 6
                && splitter.getExplicitFocusOrder() == 59,
            "Expanded workbench must expose a keyboard-focusable six-pixel splitter.");
    require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
            "Standard workbench must preserve the protected map height.");

    const auto initialUndoDepth = session.getDocumentState().undoDepth;
    requireButton(panel, "authoringWorkbenchMacrosTab").onClick();
    require(region.getHeight() == drs::app::authoring::WorkbenchLayoutState::focusedDefaultHeight,
            "Macros must suggest the 340 px Focused state.");
    auto& macroList = requireComponent(panel, "authoringMacroList");
    auto& macroName = requireComponent(panel, "authoringMacroNameEditor");
    require(panel.getLocalArea(&macroList, macroList.getLocalBounds()).getRight()
                < panel.getLocalArea(&macroName, macroName.getLocalBounds()).getX(),
            "Macros must use a first-class list/detail column layout at demonstration size.");
    require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
            "Focused macro editing must preserve the protected map height.");

    splitter.requestHeight(240);
    require(region.getHeight() == 240 && session.getDocumentState().undoDepth == initialUndoDepth,
            "Direct splitter sizing must be stable and create no authoring transaction.");
    requireButton(panel, "authoringWorkbenchRoutingTab").onClick();
    require(region.getHeight() == 240,
            "Switching tabs must retain a user-set workbench height.");

    if (loaded.project.authoring.zones.size() > 1u)
    {
        session.selectZone(loaded.project.authoring.zones[1].id);
        panel.refreshNow();
    }
    require(requireButton(panel, "authoringWorkbenchRoutingTab").getToggleState()
                && region.getHeight() == 240,
            "Zone selection refresh must preserve the active workbench tab and height.");

    panel.setSize(1120, 620);
    panel.resized();
    require(map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
            "A resized short host must preserve the map minimum.");
    panel.setSize(1120, 800);
    panel.resized();
    require(region.getHeight() == 240,
            "Returning to demonstration size must restore the remembered user height.");

    require(splitter.keyPressed(juce::KeyPress(juce::KeyPress::returnKey, {}, 0))
                && region.getHeight() == drs::app::authoring::WorkbenchLayoutState::focusedDefaultHeight,
            "The splitter keyboard action must switch from Standard to Focused.");
    auto& routingViewport = dynamic_cast<juce::Viewport&>(
        requireComponent(panel, "authoringRoutingViewport"));
    auto& routingContent = requireComponent(panel, "authoringRoutingContent");
    auto& fxHeading = requireComponent(panel, "authoringFxSectionLabel");
    auto& busHeading = requireComponent(panel, "authoringRoutingSectionLabel");
    require(panel.getLocalArea(&busHeading, busHeading.getLocalBounds()).getRight()
                < panel.getLocalArea(&fxHeading, fxHeading.getLocalBounds()).getX(),
            "Focused Routing must expose signal-path then selected-FX regions.");
    require(routingContent.getHeight() - routingViewport.getHeight() <= 112,
            "Focused Routing must keep primary path and parameter controls in view with bounded secondary overflow.");
    require(session.getDocumentState().undoDepth == initialUndoDepth,
            "Workbench navigation and sizing must stay outside authored undo history.");

    requireButton(panel, "authoringWorkbenchToggleButton").onClick();
    require(region.getHeight() == drs::app::authoring::WorkbenchLayoutState::collapsedHeight
                && !splitter.isVisible(),
            "Collapse must leave only the approximately 38 px tab rail.");
    requireButton(panel, "authoringWorkbenchToggleButton").onClick();
    require(region.getHeight() == drs::app::authoring::WorkbenchLayoutState::focusedDefaultHeight,
            "Re-expanding must restore the last expanded height.");

}

void qualifyCompactShortHost()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Compact Phase 3 qualification requires the reference project.");
    drs::engine::AuthoringSession session(loaded.project);
    drs::app::AuthoringPanel panel(session, {}, {}, {}, drs::app::AuthoringPanel::LayoutMode::compact);
    panel.setSize(820, 620);
    panel.setVisible(true);
    panel.resized();
    panel.reloadFromSession();

    auto& region = requireComponent(panel, "authoringWorkbench");
    auto& map = requireComponent(panel, "authoringZoneMap");
    require(region.getHeight() == drs::app::authoring::WorkbenchLayoutState::collapsedHeight,
            "Compact/plugin shell must begin with the same collapsed workbench state.");
    requireButton(panel, "authoringWorkbenchRoutingTab").onClick();
    require(region.getHeight() < drs::app::authoring::WorkbenchLayoutState::focusedMinimumHeight
                && map.getHeight() >= drs::app::authoring::minimumMapVisibleHeight,
            "Focused workbench must clamp below 320 px when a short plugin shell protects the map.");
    for (const auto* id : { "authoringWorkbenchWaveformTab", "authoringWorkbenchGroupsTab",
                            "authoringWorkbenchMacrosTab", "authoringWorkbenchRoutingTab",
                            "authoringWorkbenchPerformanceTab", "authoringWorkbenchArticulationsTab" })
    {
        require(findDescendantById(panel, id) != nullptr,
                std::string { "Compact shell is missing shared tab: " } + id);
    }
}
} // namespace

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI gui;
        qualifyLayoutState();
        qualifyExpandedWorkbench();
        qualifyCompactShortHost();
        std::cout << "Open Workbench Phase 3 tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Open Workbench Phase 3 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
