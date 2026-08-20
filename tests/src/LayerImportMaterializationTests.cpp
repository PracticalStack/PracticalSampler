#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

RuntimeProjectModel buildLayerProject()
{
    auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Layer import materialization requires the Phase 2 reference project.");

    auto project = loaded.project;
    const auto curated = migrateRuntimeProjectToCuratedDspSchema(project);
    require(curated.valid, "Layer import fixture must migrate through curated DSP schema.");
    project = curated.project;
    const auto articulations = migrateRuntimeProjectToPerformanceArticulationSchema(project);
    require(articulations.valid, "Layer import fixture must migrate through articulation schema.");
    project = articulations.project;
    const auto dampers = migrateRuntimeProjectToContinuousDamperSchema(project);
    require(dampers.valid, "Layer import fixture must migrate through continuous damper schema.");
    project = dampers.project;
    const auto playback = migrateRuntimeProjectToPlaybackRegionSchema(project);
    require(playback.valid, "Layer import fixture must migrate through playback region schema.");
    project = playback.project;
    const auto loops = migrateRuntimeProjectToLoopCrossfadeSchema(project);
    require(loops.valid, "Layer import fixture must migrate through loop crossfade schema.");
    project = loops.project;
    const auto layers = migrateRuntimeProjectToLayerSchema(project);
    require(layers.valid, "Layer import fixture must migrate through layer schema.");
    return layers.project;
}

void verifyUngroupedZoneCreatesBothDefaults()
{
    auto project = buildLayerProject();
    const auto sourceZone = project.authoring.zones.front();
    project.authoring.zones.clear();
    project.authoring.groups.clear();
    project.authoring.layers.clear();
    project.authoring.fxSlots.clear();
    project.authoring.routingBuses.clear();
    project.authoring.selectedZoneId.clear();
    project.authoring.selectedGroupId.clear();
    project.authoring.selectedLayerId.clear();

    auto zone = sourceZone;
    zone.id = "imported-ungrouped-zone";
    zone.displayName = zone.id;
    zone.groupId.clear();
    AuthoringSession session(project);
    const auto applied = session.appendImportedContent({}, { zone }, "Import ungrouped layer fixture");
    if (!applied.applied)
    {
        const auto detail = applied.issues.empty() ? std::string { "no import issue" } : applied.issues.front();
        throw std::runtime_error("Ungrouped zone import must commit atomically: " + detail);
    }

    const auto& imported = session.getProject();
    require(imported.authoring.zones.size() == 1
                && imported.authoring.zones.front().groupId == "default-group",
            "An ungrouped imported zone must attach to default-group.");
    require(imported.authoring.groups.size() == 1
                && imported.authoring.groups.front().id == "default-group"
                && imported.authoring.groups.front().layerId == "default-layer",
            "An ungrouped imported zone must create default-group in default-layer.");
    require(imported.authoring.layers.size() == 1
                && imported.authoring.layers.front().id == "default-layer"
                && imported.authoring.selectedLayerId == "default-layer",
            "An ungrouped imported zone must create and select default-layer.");
    require(!imported.authoring.notes.empty(),
            "Default hierarchy synthesis must record authoring provenance notes.");
}

void verifyExplicitImportedGroupGetsDefaultLayer()
{
    auto project = buildLayerProject();
    const auto sourceZone = project.authoring.zones.front();
    project.authoring.zones.clear();
    project.authoring.groups.clear();
    project.authoring.layers.clear();
    project.authoring.fxSlots.clear();
    project.authoring.routingBuses.clear();
    project.authoring.selectedZoneId.clear();
    project.authoring.selectedGroupId.clear();
    project.authoring.selectedLayerId.clear();

    auto zone = sourceZone;
    zone.id = "imported-named-zone";
    zone.displayName = zone.id;
    zone.groupId = "named-import-group";
    RuntimeProjectGroupDefinition group;
    group.id = "named-import-group";
    group.displayName = "Named Import Group";

    AuthoringSession session(project);
    const auto applied = session.appendImportedContent({}, { zone }, 0.0, { group }, {}, {},
                                                        "Import named layer fixture", false);
    require(applied.applied, "Explicit group import must commit atomically.");

    const auto& imported = session.getProject();
    require(imported.authoring.groups.size() == 1
                && imported.authoring.groups.front().layerId == "default-layer",
            "Imported groups without a layer must attach to default-layer.");
    require(imported.authoring.layers.size() == 1
                && imported.authoring.layers.front().id == "default-layer",
            "Imported groups without a layer must synthesize default-layer.");
}

void verifyAppendPreservesExistingContainers()
{
    auto project = buildLayerProject();
    require(!project.authoring.layers.empty() && !project.authoring.groups.empty(),
            "Append preservation fixture must contain authored layer and group containers.");

    const auto existingLayer = project.authoring.layers.front();
    const auto existingGroup = project.authoring.groups.front();
    auto zone = project.authoring.zones.front();
    zone.id = "appended-existing-group-zone";
    zone.displayName = zone.id;
    zone.groupId = existingGroup.id;

    AuthoringSession session(project);
    const auto applied = session.appendImportedContent({}, { zone }, "Append to existing layer fixture");
    require(applied.applied, "Append into an existing layer/group must commit atomically.");

    const auto& appended = session.getProject();
    require(appended.authoring.layers.size() == project.authoring.layers.size()
                && appended.authoring.groups.size() == project.authoring.groups.size(),
            "Appending a zone with existing membership must not create duplicate containers.");
    require(appended.authoring.layers.front().id == existingLayer.id
                && appended.authoring.layers.front().gainDb == existingLayer.gainDb
                && appended.authoring.layers.front().pan == existingLayer.pan
                && appended.authoring.groups.front().id == existingGroup.id
                && appended.authoring.groups.front().layerId == existingGroup.layerId
                && appended.authoring.groups.front().displayOrder == existingGroup.displayOrder,
            "Appending a zone must preserve existing layer/group identity and values.");
}
} // namespace

int main()
{
    try
    {
        verifyUngroupedZoneCreatesBothDefaults();
        verifyExplicitImportedGroupGetsDefaultLayer();
        verifyAppendPreservesExistingContainers();
        std::cout << "Layer import materialization tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Layer import materialization tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
