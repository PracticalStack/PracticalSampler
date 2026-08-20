#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeLoader.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "Could not read fixture " + path.generic_string() + ".");
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    require(!text.empty(), "Fixture is empty: " + path.generic_string() + ".");
    return text;
}

fs::path fixturePath(const char* fileName)
{
    const auto path = fs::path(DRS_SOURCE_ROOT) / "tests/fixtures/layer-contract" / fileName;
    require(fs::exists(path), "Layer contract fixture is missing: " + path.generic_string());
    return path;
}

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(
    drs::engine::PlaybackSnapshotBuilder& builder,
    const drs::engine::RuntimeProjectModel& project)
{
    const auto request = builder.requestBuild(0, true);
    require(request.accepted, "Phase 0 group characterization build request should be accepted.");
    return builder.buildSnapshot(request, project);
}

void verifyHierarchyFixture()
{
    const auto root = json::parse(readText(fixturePath("layer-hierarchy.fixture.json")));
    require(root.at("fixtureId") == "layer-contract-hierarchy-v1",
            "Layer hierarchy fixture identity changed unexpectedly.");
    require(root.at("layerDefinitions").size() == 1
                && root.at("groupDefinitions").size() == 2
                && root.at("zoneDefinitions").size() == 3,
            "Layer hierarchy fixture must contain one layer, two groups, and three zones.");

    const auto& groupDefinitions = root.at("groupDefinitions");
    require(groupDefinitions.at(0).at("layerId") == "default-layer"
                && groupDefinitions.at(1).at("layerId") == "default-layer",
            "Phase 0 hierarchy fixture must express layer membership on child groups.");
    require(root.at("expectedMembership").at("default-layer").size() == 2
                && root.at("expectedMembership").at("soft-group").size() == 2
                && root.at("expectedMembership").at("hard-group").size() == 1,
            "Phase 0 hierarchy fixture membership counts changed unexpectedly.");

    const auto defaultingRoot = json::parse(readText(fixturePath("layer-defaulting.fixture.json")));
    require(defaultingRoot.at("fixtureId") == "layer-contract-defaulting-v1"
                && defaultingRoot.at("input").at("groupId") == ""
                && defaultingRoot.at("expected").at("layerId") == "default-layer"
                && defaultingRoot.at("expected").at("groupId") == "default-group",
            "Layer defaulting fixture must require a default layer and default group for an ungrouped import.");

    const auto crossfadeRoot = json::parse(readText(fixturePath("layer-crossfade.fixture.json")));
    require(crossfadeRoot.at("fixtureId") == "layer-contract-crossfade-v1"
                && crossfadeRoot.at("cases").size() == 4,
            "Layer crossfade fixture must retain no-crossfade, velocity, controller, and invalid cases.");
    require(crossfadeRoot.at("cases").at(0).at("source") == "none"
                && crossfadeRoot.at("cases").at(1).at("source") == "velocity"
                && crossfadeRoot.at("cases").at(2).at("source") == "controller"
                && crossfadeRoot.at("cases").at(2).at("controllerNumber") == 1,
            "Layer crossfade fixture source cases changed unexpectedly.");
}

void verifyCurrentGroupCharacterization()
{
    using namespace drs::engine;

    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Phase 0 group characterization requires the Phase 2 reference project.");
    const auto& project = loaded.project;
    require(project.schemaVersion == 4 && project.authoring.schemaVersion == 3,
            "Phase 0 group characterization fixture must remain on the explicit group schema.");
    require(project.authoring.groups.size() == 2 && !project.authoring.zones.empty(),
            "Phase 0 group characterization fixture inventory changed unexpectedly.");

    std::unordered_map<std::string, std::vector<std::string>> expectedZoneIdsByGroup;
    for (const auto& zone : project.authoring.zones)
    {
        require(!zone.id.empty() && !zone.groupId.empty(),
                "Current explicit group behavior requires every zone to have stable group membership.");
        expectedZoneIdsByGroup[zone.groupId].push_back(zone.id);
    }
    require(expectedZoneIdsByGroup.size() == project.authoring.groups.size(),
            "Current group definitions must cover every distinct zone group id.");

    auto editedProject = project;
    const auto groupIndex = std::size_t { 0 };
    const auto groupId = editedProject.authoring.groups[groupIndex].id;
    editedProject.authoring.groups[groupIndex].gainDb = -4.25;
    editedProject.authoring.groups[groupIndex].pan = 0.35;
    const auto originalZoneGain = editedProject.authoring.zones.front().gainDb;
    const auto originalZonePan = editedProject.authoring.zones.front().pan;

    PlaybackSnapshotBuilder builder;
    const auto snapshotResult = buildSnapshot(builder, editedProject);
    require(snapshotResult.built && snapshotResult.findings.empty(),
            "Current group characterization project must build without snapshot findings.");

    const auto groupRoute = std::find_if(snapshotResult.snapshot.groupRoutes.begin(),
                                         snapshotResult.snapshot.groupRoutes.end(),
                                         [&](const auto& route) { return route.groupId == groupId; });
    require(groupRoute != snapshotResult.snapshot.groupRoutes.end(),
            "Current group characterization must materialize an independent group route.");
    require(groupRoute->gainDb == -4.25 && groupRoute->pan == 0.35,
            "Group gain and pan must remain on the independent group route.");

    const auto zoneSnapshot = std::find_if(snapshotResult.snapshot.zones.begin(),
                                           snapshotResult.snapshot.zones.end(),
                                           [&](const auto& zone) { return zone.groupId == groupId; });
    require(zoneSnapshot != snapshotResult.snapshot.zones.end(),
            "Current group characterization must retain group member zones in the snapshot.");
    require(zoneSnapshot->gainDb == originalZoneGain && zoneSnapshot->pan == originalZonePan,
            "Group edits must not be flattened into child zone gain or pan values.");
    require(groupRoute->zoneIds == expectedZoneIdsByGroup.at(groupId),
            "Group route membership must remain derived from zone groupId relationships.");
}
} // namespace

int main()
{
    try
    {
        verifyHierarchyFixture();
        verifyCurrentGroupCharacterization();
        std::cout << "Layer contract Phase 0 baseline and fixture tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Layer contract Phase 0 baseline and fixture tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
