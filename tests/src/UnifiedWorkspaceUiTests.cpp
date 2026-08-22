#include "shared/AuthoringPanel.h"
#include "shared/authoring/InstrumentStructureBrowser.h"
#include "shared/authoring/StructureScope.h"
#include "shared/authoring/UnifiedWorkspaceLayout.h"
#include "shared/authoring/ZoneMapCanvas.h"
#include "support/StructureViewerFixtures.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value, const std::string& message)
{
    if (!value) throw std::runtime_error(message);
}

juce::Component* findDescendantById(juce::Component& root, const juce::String& id)
{
    if (root.getComponentID() == id) return &root;
    for (int index = 0; index < root.getNumChildComponents(); ++index)
        if (auto* match = findDescendantById(*root.getChildComponent(index), id)) return match;
    return nullptr;
}
}

int main()
{
    try
    {
        juce::ScopedJuceInitialiser_GUI juceInitialiser;
        const auto project = drs::tests::makeStructureViewerFixture();
        drs::app::authoring::AuthoringStructureSelection selection;
        selection.replace(drs::app::authoring::StructureSelectionKind::group,
                          { "group-piano-sustain" }, "group-piano-sustain");
        drs::app::authoring::InstrumentStructureBrowser browser;
        browser.setBounds(0, 0, 300, 640);
        browser.setRows(drs::app::authoring::buildInstrumentStructureRows(
            project, selection, { drs::app::authoring::kInstrumentStructureId,
                                  "layer-piano", "group-piano-sustain" }, {}));
        browser.setSelection(selection);
        require(!browser.getRows().empty(), "UI browser should render a root row.");
        require(browser.getRows().front().depth == 0, "Root row must have depth zero.");
        require(browser.getRows().size() < 32, "Fixture browser should remain compact and virtualizable.");

        const auto largeProject = drs::tests::makeStructureViewerFixture(180);
        browser.setBounds(0, 0, 300, 220);
        browser.setRows(drs::app::authoring::buildInstrumentStructureRows(
            largeProject, selection, {}, {}));
        browser.getList().setVerticalPosition(0.72);
        const auto scrollPositionBeforeSelection = browser.getList().getViewport()->getViewPositionY();
        require(scrollPositionBeforeSelection > 0,
                "Scroll-preservation fixture must place the hierarchy below its first row.");
        browser.setSelection(selection);
        require(browser.getList().getViewport()->getViewPositionY() == scrollPositionBeforeSelection,
                "Synchronizing a tree selection must not scroll the hierarchy to the selected row.");
        browser.setRows(drs::app::authoring::buildInstrumentStructureRows(
            largeProject, selection, {}, {}));
        require(browser.getList().getViewport()->getViewPositionY() == scrollPositionBeforeSelection,
                "Refreshing hierarchy rows after selection must preserve the current scroll position.");

        browser.setBounds(0, 0, 1120, 800);
        require(browser.getBounds().getWidth() == 1120 && browser.getBounds().getHeight() == 800,
                "Browser must accept expanded shell bounds without a three-column minimum.");
        const auto wide = drs::app::authoring::calculateUnifiedWorkspaceLayout({ { 0, 0, 1120, 700 }, 300, 340, 12, true });
        require(!wide.browser.isEmpty() && !wide.map.isEmpty() && !wide.inspector.isEmpty(),
                "Expanded unified shell must allocate browser, Map, and inspector rectangles.");
        const auto inspectorOnly = drs::app::authoring::calculateUnifiedWorkspaceLayout({ { 0, 0, 1120, 700 }, 300, 340, 12, false });
        require(inspectorOnly.map.isEmpty() && inspectorOnly.inspector.getWidth() > wide.inspector.getWidth(),
                "Map-hidden shell must reclaim the Map width for the inspector.");
        drs::app::authoring::ZoneMapCanvas map;
        map.setScopeSummary("Showing zones: Piano Sustain · 2");
        require(map.getScopeSummaryLabel().getComponentID() == "authoringZoneMapScopeSummary",
                "Zone Map must expose an accessible scope summary control.");

        drs::engine::AuthoringSession session(project);
        drs::app::AuthoringPanel panel(session);
        panel.setBounds(0, 0, 1120, 800);
        panel.setVisible(true);
        panel.reloadFromSession();
        auto* tree = dynamic_cast<juce::ListBox*>(findDescendantById(panel, "authoringInstrumentStructureBrowser"));
        require(tree != nullptr, "Unified workspace must expose its hierarchy ListBox.");
        auto* panelBrowser = dynamic_cast<drs::app::authoring::InstrumentStructureBrowser*>(tree->getParentComponent());
        require(panelBrowser != nullptr, "Hierarchy ListBox must remain owned by InstrumentStructureBrowser.");
        const auto& panelRows = panelBrowser->getRows();
        const auto panelGroup = std::find_if(panelRows.begin(), panelRows.end(), [](const auto& row)
        {
            return row.kind == drs::app::authoring::InstrumentStructureRowKind::group;
        });
        require(panelGroup != panelRows.end(), "Unified workspace fixture must expose a selectable group row.");
        const auto selectedGroupId = panelGroup->id;
        tree->selectRow(static_cast<int>(std::distance(panelRows.begin(), panelGroup)), false, true);
        require(session.getSelectedGroup().has_value() && session.getSelectedGroup()->id == selectedGroupId,
                "Selecting a hierarchy group must update session selection without aborting.");

        auto macroProject = project;
        drs::engine::RuntimeProjectFxSlotDefinition gainSlot;
        gainSlot.id = "instrument-gain";
        gainSlot.displayName = "Instrument Gain";
        gainSlot.effectType = "drs.gain";
        gainSlot.effectVersion = 1;
        gainSlot.parameters.push_back({ "gainDb", 0.0 });
        macroProject.authoring.fxSlots.push_back(std::move(gainSlot));
        drs::engine::RuntimeProjectRoutingBusDefinition instrumentBus;
        instrumentBus.id = "instrument-bus";
        instrumentBus.displayName = "Instrument Bus";
        instrumentBus.inputSourceId = "master";
        instrumentBus.fxSlotIds.push_back("instrument-gain");
        macroProject.authoring.routingBuses.push_back(std::move(instrumentBus));
        drs::engine::RuntimeProjectMacroDefinition instrumentMacro;
        instrumentMacro.id = "instrument";
        instrumentMacro.name = "Instrument";
        instrumentMacro.defaultValue = 0.5;
        instrumentMacro.minValue = 0.0;
        instrumentMacro.maxValue = 1.0;
        drs::engine::RuntimeProjectMacroTargetDefinition gainTarget;
        gainTarget.parameterId = "dsp.instrument-gain.gainDb";
        gainTarget.parameterPath = "curatedDsp.instrument-gain.gainDb";
        gainTarget.role = "mix";
        gainTarget.dspSlotId = "instrument-gain";
        gainTarget.dspParameterId = "gainDb";
        gainTarget.sourceMinimum = 0.0;
        gainTarget.sourceMaximum = 1.0;
        gainTarget.destinationMinimum = -96.0;
        gainTarget.destinationMaximum = 6.0;
        gainTarget.controlLaw.id = "drs.mixerGain.v1";
        gainTarget.controlLaw.version = 1;
        instrumentMacro.targets.push_back(std::move(gainTarget));
        macroProject.authoring.macros.push_back(std::move(instrumentMacro));

        drs::engine::AuthoringSession macroValidationSession(macroProject);
        auto directlyEditedMacro = macroValidationSession.getProject().authoring.macros.front();
        directlyEditedMacro.defaultValue = 0.6;
        const auto directMacroEdit = macroValidationSession.updateMacro(
            0, directlyEditedMacro, "Validate structured macro fixture");
        require(directMacroEdit.applied,
                "Structured macro fixture must accept a direct Default-only transaction: "
                    + (directMacroEdit.issues.empty() ? std::string("unknown rejection")
                                                       : directMacroEdit.issues.front()));

        drs::engine::AuthoringSession macroSession(macroProject);
        drs::app::AuthoringPanel macroPanel(macroSession);
        macroPanel.setBounds(0, 0, 1120, 800);
        macroPanel.setVisible(true);
        macroPanel.reloadFromSession();
        auto* defaultSlider = dynamic_cast<juce::Slider*>(
            findDescendantById(macroPanel, "authoringMacroDefaultSlider"));
        require(defaultSlider != nullptr, "Macro workbench must expose its Default slider.");
        defaultSlider->setValue(0.75, juce::dontSendNotification);
        require(static_cast<bool>(defaultSlider->onDragEnd),
                "Macro Default slider must expose its commit callback.");
        defaultSlider->onDragEnd();
        const auto& updatedMacro = macroSession.getProject().authoring.macros.front();
        require(updatedMacro.defaultValue == 0.75,
                "Changing a macro Default must commit without retargeting its structured DSP assignment.");
        require(updatedMacro.targets.front().destinationMinimum == -96.0
                    && updatedMacro.targets.front().destinationMaximum == 6.0
                    && updatedMacro.targets.front().controlLaw.id == "drs.mixerGain.v1",
                "Changing Default must preserve the target's role-specific control-law range.");
        std::cout << "Unified workspace UI tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Unified workspace UI tests failed: " << error.what() << "\n";
        return 1;
    }
}
