#include "shared/authoring/StructureViewModels.h"
#include "shared/authoring/StructureViewer.h"
#include "support/StructureViewerFixtures.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        const auto project = drs::tests::makeStructureViewerFixture(1000);
        drs::app::authoring::AuthoringStructureSelection selection;
        selection.replace(drs::app::authoring::StructureSelectionKind::group,
                          { "group-piano-sustain" },
                          "group-piano-sustain");

        const auto start = std::chrono::steady_clock::now();
        std::vector<long long> projectionTimings;
        drs::app::authoring::StructureHierarchyViewModel projection;
        for (int iteration = 0; iteration < 30; ++iteration)
        {
            const auto iterationStart = std::chrono::steady_clock::now();
            projection = drs::app::authoring::buildStructureHierarchyViewModel(
                project, selection, "layer-piano", "group-piano-sustain");
            require(projection.layers.size() == 3, "Large fixture layer projection changed unexpectedly.");
            require(projection.groups.size() == 2, "Large fixture group projection changed unexpectedly.");
            require(projection.zones.size() == 502, "Large fixture primary-group zone projection changed unexpectedly.");
            projectionTimings.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - iterationStart).count());
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        require(elapsed < 2500, "Large fixture hierarchy projection exceeded the contract timing budget.");

        drs::app::authoring::StructureViewProjectionOptions diagnosticOptions;
        diagnosticOptions.includeDiagnostics = true;
        diagnosticOptions.showPotentialCollisionsOnly = true;
        const auto diagnosticStart = std::chrono::steady_clock::now();
        const auto diagnosticProjection = drs::app::authoring::buildStructureHierarchyViewModel(
            project, selection, "layer-piano", "group-piano-sustain", diagnosticOptions);
        const auto diagnosticElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - diagnosticStart).count();
        require(!diagnosticProjection.zones.empty(), "Diagnostic projection should retain potential collisions.");
        require(diagnosticElapsed < 2500, "Large fixture diagnostic analysis exceeded the contract timing budget.");

        juce::ScopedJuceInitialiser_GUI juceInitialiser;
        drs::app::authoring::StructureViewer viewer;
        viewer.setSize(1280, 720);
        viewer.setViewModel(projection);
        viewer.setColumnWidths(210, 280, 720);
        require(!viewer.getLayerList().getBounds().isEmpty()
                    && !viewer.getGroupList().getBounds().isEmpty()
                    && !viewer.getZoneList().getBounds().isEmpty(),
                "Qualification viewer columns must remain reachable at expanded size.");
        viewer.setSize(560, 420);
        viewer.resized();
        require(!viewer.getLayerList().getBounds().isEmpty()
                    && !viewer.getGroupList().getBounds().isEmpty()
                    && !viewer.getZoneList().getBounds().isEmpty(),
                "Qualification viewer columns must remain reachable at compact size.");

        const auto artifactDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getChildFile("drs-structure-viewer-qualification");
        artifactDirectory.createDirectory();
        std::ofstream geometry(artifactDirectory.getChildFile("geometry.txt").getFullPathName().toStdString());
        geometry << "expanded=1280x720\ncompact=560x420\nprojection_count=30\n"
                 << "diagnostic_ms=" << diagnosticElapsed << "\n";
        geometry.close();
        juce::PNGImageFormat png;
        auto image = viewer.createComponentSnapshot(viewer.getLocalBounds(), false);
        juce::FileOutputStream stream(artifactDirectory.getChildFile("structure-viewer.png"));
        require(stream.openedOk() && png.writeImageToStream(image, stream),
                "Qualification should write a Structure viewer screenshot artifact.");

        std::sort(projectionTimings.begin(), projectionTimings.end());
        const auto p95 = projectionTimings[static_cast<std::size_t>(projectionTimings.size() * 95 / 100)];
        require(p95 < 250000, "Large fixture p95 projection timing exceeded 250 ms.");

        std::cout << "Structure viewer qualification passed: 1005 zones, 30 projections, "
                  << elapsed << " ms, p95 " << p95 << " us, diagnostics " << diagnosticElapsed << " ms.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Structure viewer qualification failed: " << error.what() << "\n";
        return 1;
    }
}
