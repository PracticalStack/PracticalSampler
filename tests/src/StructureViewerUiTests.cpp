#include "shared/authoring/StructureViewer.h"
#include "shared/authoring/StructureViewModels.h"
#include "support/StructureViewerFixtures.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <iostream>
#include <stdexcept>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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
        auto model = drs::app::authoring::buildStructureHierarchyViewModel(
            project, selection, "layer-piano", "group-piano-sustain");

        drs::app::authoring::StructureViewer viewer;
        viewer.setSize(1280, 720);
        viewer.setViewModel(model);
        viewer.setSelection(selection);
        require(viewer.getViewModel().layers.size() == 3
                    && viewer.getViewModel().groups.size() == 2
                    && viewer.getViewModel().zones.size() == 2,
                "Structure UI should render the expected parent-scoped columns.");
        require(viewer.getLayerList().getSelectedRow() < 0
                    && viewer.getGroupList().getSelectedRow() >= 0,
                "Stable group selection should synchronize only the active entity column.");

        viewer.setColumnWidths(240, 320, 600);
        const auto widths = viewer.getColumnWidths();
        require(widths[0] >= 150 && widths[1] >= 180 && widths[2] >= 240,
                "Column widths should enforce minimum useful dimensions.");
        viewer.setSize(560, 420);
        viewer.resized();
        require(!viewer.getLayerList().getBounds().isEmpty()
                    && !viewer.getGroupList().getBounds().isEmpty()
                    && !viewer.getZoneList().getBounds().isEmpty(),
                "Compact layout should preserve all hierarchy columns.");

        int callbackCount = 0;
        viewer.setOnSelectionChanged([&](const auto kind, auto ids, auto primary)
        {
            ++callbackCount;
            require(kind == drs::app::authoring::StructureSelectionKind::group
                        && ids.size() == 1 && primary == "group-piano-release",
                    "List selection callback should emit stable IDs and a primary row.");
        });
        viewer.getGroupList().selectRow(1, false, true);
        require(callbackCount > 0, "Selecting a structure row should notify the coordinator.");

        drs::app::authoring::AuthoringStructureSelection zoneSelection;
        zoneSelection.replace(drs::app::authoring::StructureSelectionKind::zone,
                              { "zone-piano-high" }, "zone-piano-high");
        auto zoneModel = drs::app::authoring::buildStructureHierarchyViewModel(
            project, zoneSelection, "layer-piano", "group-piano-sustain");
        viewer.setViewModel(zoneModel);
        viewer.setSelection(zoneSelection);
        require(viewer.revealZone("zone-piano-high")
                    && viewer.getZoneList().getSelectedRow() == 1,
                "Reveal in Structure should select and scroll to the stable zone ID.");
        require(!viewer.revealZone("missing-zone"),
                "Reveal in Structure should reject an ID outside the current projection.");

        std::cout << "Structure viewer UI tests passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Structure viewer UI tests failed: " << error.what() << "\n";
        return 1;
    }
}
