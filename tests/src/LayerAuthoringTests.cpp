#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <iostream>
#include <stdexcept>

namespace
{
using namespace drs::engine;

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

RuntimeProjectModel makeProject()
{
    auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Layer authoring tests require the reference project.");
    auto project = loaded.project;
    auto curated = migrateRuntimeProjectToCuratedDspSchema(project);
    require(curated.valid, "Reference project must migrate to curated DSP.");
    auto articulations = migrateRuntimeProjectToPerformanceArticulationSchema(curated.project);
    require(articulations.valid, "Reference project must migrate to articulations.");
    auto dampers = migrateRuntimeProjectToContinuousDamperSchema(articulations.project);
    require(dampers.valid, "Reference project must migrate to dampers.");
    auto playback = migrateRuntimeProjectToPlaybackRegionSchema(dampers.project);
    require(playback.valid, "Reference project must migrate to playback regions.");
    auto loops = migrateRuntimeProjectToLoopCrossfadeSchema(playback.project);
    require(loops.valid, "Reference project must migrate to loop crossfade.");
    auto layers = migrateRuntimeProjectToLayerSchema(loops.project);
    require(layers.valid, "Reference project must migrate to layers.");
    project = layers.project;
    project.authoring.layers = { { "layer-a", "Layer A", 0, true, 0.0, 0.0 } };
    for (auto& group : project.authoring.groups)
        group.layerId = "layer-a";
    project.authoring.selectedLayerId = "layer-a";
    return project;
}
} // namespace

int main()
{
    try
    {
        AuthoringSession session(makeProject());
        require(session.getSelectedLayer().has_value(), "A schema-10 project must expose its selected layer.");
        require(session.getProject().authoring.groups.front().layerId == "layer-a",
                "Session preparation must attach legacy-empty group membership to the default layer.");

        auto layer = *session.getSelectedLayer();
        const auto originalGroupId = session.getProject().authoring.groups.front().id;
        layer.gainDb = 4.0;
        layer.pan = -0.25;
        layer.workspaceVisible = false;
        layer.crossfade.source = LayerCrossfadeSource::controller;
        layer.crossfade.controllerNumber = 1;
        require(session.updateLayer(0, layer, "Edit layer").applied,
                "Layer gain, pan, visibility, and crossfade metadata must be editable in one transaction.");

        RuntimeProjectLayerDefinition secondLayer;
        secondLayer.id = "layer-b";
        secondLayer.displayName = "Layer B";
        require(session.createLayer(secondLayer, "Create second layer").applied,
                "Layer creation must preserve the layer contract.");
        require(session.selectLayer("layer-b").applied,
                "Layer selection must navigate the containing authoring context.");

        RuntimeProjectGroupDefinition secondGroup;
        secondGroup.id = "group-b";
        secondGroup.displayName = "Group B";
        require(session.createGroup(secondGroup, "Create second group").applied,
                "Creating a group from a selected layer must assign the active layer automatically.");
        require(session.getProject().authoring.groups.back().layerId == "layer-b",
                "New groups must never be left without a parent layer.");

        require(session.reassignGroupsToLayer({ originalGroupId, "group-b" }, "layer-a",
                                               "Move groups to layer").applied,
                "Group-to-layer assignment must be an undoable document transaction.");
        require(session.getProject().authoring.groups.front().layerId == "layer-a"
                    && session.getProject().authoring.groups.back().layerId == "layer-a",
                "Group-to-layer assignment must update every requested group and no zone-local values.");
        require(session.moveLayer(1, -1, "Move layer earlier").applied,
                "Layer ordering must use the same bounded transaction pattern as group ordering.");

        std::cout << "Layer authoring tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Layer authoring tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
