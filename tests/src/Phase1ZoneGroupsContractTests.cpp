#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct ExpectedGroupRoute
{
    std::vector<std::string> articulationIds;
    std::vector<std::string> zoneIds;
};

void appendUnique(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty())
        return;

    for (const auto& existing : values)
    {
        if (existing == value)
            return;
    }

    values.push_back(value);
}

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(drs::engine::PlaybackSnapshotBuilder& builder,
                                                       const drs::engine::RuntimeProjectModel& project)
{
    const auto request = builder.requestBuild(0, true);
    require(request.accepted, "Zone-group contract build request should be accepted.");
    return builder.buildSnapshot(request, project);
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto projectLoad = loadPhase2ReferenceProjectManifest();
        require(projectLoad.loaded, "Phase 2 reference project must load for Zone Groups Sprint 3 coverage.");

        const auto& project = projectLoad.project;
        require(!project.authoring.zones.empty(),
                "Phase 2 reference project must expose authored zones for Zone Groups Sprint 3 coverage.");
        require(project.schemaVersion == 4 && project.authoring.schemaVersion == 3,
                "Sprint 3 Zone Groups contract fixture should use the explicit group schema.");

        std::set<std::string> zoneIds;
        std::vector<std::string> expectedGroupOrder;
        std::map<std::string, ExpectedGroupRoute> expectedRoutes;

        for (const auto& zone : project.authoring.zones)
        {
            require(!zone.id.empty(), "Sprint 1 zone-group baseline requires stable zone ids.");
            require(!zone.groupId.empty(), "Sprint 1 zone-group baseline requires every zone to carry a groupId.");
            zoneIds.insert(zone.id);

            auto [iterator, inserted] = expectedRoutes.emplace(zone.groupId, ExpectedGroupRoute {});
            if (inserted)
                expectedGroupOrder.push_back(zone.groupId);

            iterator->second.zoneIds.push_back(zone.id);
            appendUnique(iterator->second.articulationIds, zone.articulationId);
        }

        const auto serializedProject =
            serializeRuntimeProjectManifest(project, projectLoad.manifestPath.empty()
                                                         ? std::string("phase2-reference.project.json")
                                                         : projectLoad.manifestPath);
        require(serializedProject.find("\"groupId\"") != std::string::npos,
                "Sprint 3 authored project manifests must persist per-zone groupId membership.");
        require(serializedProject.find("\"groups\"") != std::string::npos,
                "Sprint 3 authored project manifests must persist explicit groups.");
        require(serializedProject.find("\"selectedGroupId\"") != std::string::npos,
                "Sprint 3 authored project manifests must persist selectedGroupId.");
        require(project.authoring.groups.size() == expectedGroupOrder.size(),
                "Sprint 3 authored groups should cover every distinct zone-owned group id.");
        require(project.authoring.selectedGroupId == "lead-core",
                "Sprint 3 contract fixture selectedGroupId changed unexpectedly.");

        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            const auto& group = project.authoring.groups[index];
            require(group.id == expectedGroupOrder[index],
                    "Sprint 3 authored group ordering should remain deterministic.");
            require(!group.displayName.empty(),
                    "Sprint 3 authored groups must carry creator-facing display names.");
        }

        for (const auto& bus : project.authoring.routingBuses)
        {
            require(bus.inputSourceId == "master" || zoneIds.count(bus.inputSourceId) != 0,
                    "Sprint 2 routing inputs should still resolve only to master or a direct zone id.");
        }

        PlaybackSnapshotBuilder builder;
        const auto snapshotResult = buildSnapshot(builder, project);
        require(snapshotResult.built, "Phase 2 reference project should build a snapshot for Zone Groups Sprint 3.");
        require(snapshotResult.activationEligible,
                "Zone Groups Sprint 3 contract fixture should remain activation-eligible.");
        require(snapshotResult.findings.empty(),
                "Zone Groups Sprint 3 contract fixture should not surface snapshot findings.");
        require(snapshotResult.snapshot.groupRoutes.size() == expectedGroupOrder.size(),
                "Snapshot group route count should stay aligned with distinct authored group ids.");
        require(snapshotResult.snapshot.selectedGroupId == project.authoring.selectedGroupId,
                "Sprint 3 snapshots should preserve selectedGroupId.");

        for (std::size_t index = 0; index < expectedGroupOrder.size(); ++index)
        {
            const auto& expectedGroupId = expectedGroupOrder[index];
            const auto expectedIterator = expectedRoutes.find(expectedGroupId);
            require(expectedIterator != expectedRoutes.end(),
                    "Expected Zone Groups Sprint 2 route should remain addressable by group id.");

            const auto& route = snapshotResult.snapshot.groupRoutes[index];
            require(route.groupId == expectedGroupId,
                    "Snapshot group route ordering should follow authored group order in Sprint 3.");
            require(route.zoneIds == expectedIterator->second.zoneIds,
                    "Snapshot group route membership should remain derived from authored zone membership in Sprint 3.");
            require(route.articulationIds == expectedIterator->second.articulationIds,
                    "Snapshot group articulation membership should remain derived directly from grouped zones in Sprint 3.");
            require(route.displayName == project.authoring.groups[index].displayName
                        && route.routingSourceId == "groups/" + route.groupId
                        && route.gainDb == project.authoring.groups[index].gainDb
                        && route.pan == project.authoring.groups[index].pan,
                    "Sprint 3 snapshot group routes should retain immutable group metadata.");
        }

        const auto serializedSnapshot = serializeImmutablePlaybackSnapshot(snapshotResult.snapshot);
        require(serializedSnapshot.find("\"groupRoutes\"") != std::string::npos,
                "Sprint 3 playback snapshots must continue to serialize groupRoutes.");
        require(serializedSnapshot.find("\"selectedGroupId\"") != std::string::npos
                    && serializedSnapshot.find("\"routingSourceId\"") != std::string::npos
                    && serializedSnapshot.find("\"workspaceVisible\"") != std::string::npos,
                "Sprint 3 playback snapshots must serialize explicit group metadata.");

        const auto manifestDirectory =
            std::filesystem::path(projectLoad.manifestPath.empty() ? "." : projectLoad.manifestPath).parent_path();
        const auto tempPath = manifestDirectory / "drs-phase1-zone-groups-contract-roundtrip.json";
        {
            std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
            require(output.good(), "Could not open the temporary Zone Groups Sprint 3 manifest path.");
            output << serializedProject;
        }

        const auto roundTripLoad = loadRuntimeProjectManifest(tempPath.generic_string());
        std::filesystem::remove(tempPath);

        require(roundTripLoad.loaded, "Sprint 3 project contract manifest should round-trip through the loader.");
        require(roundTripLoad.project.authoring.zones.size() == project.authoring.zones.size(),
                "Sprint 3 project round-trip should preserve authored zone count.");
        require(roundTripLoad.project.authoring.groups.size() == project.authoring.groups.size(),
                "Sprint 3 project round-trip should preserve authored group count.");
        require(roundTripLoad.project.authoring.selectedGroupId == project.authoring.selectedGroupId,
                "Sprint 3 project round-trip should preserve selectedGroupId.");

        for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
        {
            require(roundTripLoad.project.authoring.zones[index].id == project.authoring.zones[index].id,
                    "Sprint 3 project round-trip should preserve zone ordering.");
            require(roundTripLoad.project.authoring.zones[index].groupId
                        == project.authoring.zones[index].groupId,
                    "Sprint 3 project round-trip should preserve per-zone group membership exactly.");
        }

        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            require(roundTripLoad.project.authoring.groups[index].id == project.authoring.groups[index].id,
                    "Sprint 3 project round-trip should preserve group ordering.");
            require(roundTripLoad.project.authoring.groups[index].displayName
                        == project.authoring.groups[index].displayName,
                    "Sprint 3 project round-trip should preserve authored group display names.");
        }

        std::cout << "Phase 1 Zone Groups Sprint 3 contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 Zone Groups Sprint 3 contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
