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
        require(projectLoad.loaded, "Phase 2 reference project must load for Zone Groups Sprint 1 coverage.");

        const auto& project = projectLoad.project;
        require(!project.authoring.zones.empty(),
                "Phase 2 reference project must expose authored zones for Zone Groups Sprint 1 coverage.");

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
                "Sprint 1 authored project manifests must still persist per-zone groupId membership.");
        require(serializedProject.find("\"groups\"") == std::string::npos,
                "Sprint 1 authored project manifests must not persist explicit groups yet.");
        require(serializedProject.find("\"selectedGroupId\"") == std::string::npos,
                "Sprint 1 authored project manifests must not persist selectedGroupId yet.");

        for (const auto& bus : project.authoring.routingBuses)
        {
            require(bus.inputSourceId == "master" || zoneIds.count(bus.inputSourceId) != 0,
                    "Sprint 1 routing inputs must still resolve only to master or a direct zone id.");
        }

        PlaybackSnapshotBuilder builder;
        const auto snapshotResult = buildSnapshot(builder, project);
        require(snapshotResult.built, "Phase 2 reference project should build a snapshot for Zone Groups Sprint 1.");
        require(snapshotResult.activationEligible,
                "Zone Groups Sprint 1 contract fixture should remain activation-eligible.");
        require(snapshotResult.findings.empty(),
                "Zone Groups Sprint 1 contract fixture should not surface snapshot findings.");
        require(snapshotResult.snapshot.groupRoutes.size() == expectedGroupOrder.size(),
                "Snapshot group route count should stay aligned with distinct zone-owned group ids.");

        for (std::size_t index = 0; index < expectedGroupOrder.size(); ++index)
        {
            const auto& expectedGroupId = expectedGroupOrder[index];
            const auto expectedIterator = expectedRoutes.find(expectedGroupId);
            require(expectedIterator != expectedRoutes.end(),
                    "Expected Zone Groups Sprint 1 route should remain addressable by group id.");

            const auto& route = snapshotResult.snapshot.groupRoutes[index];
            require(route.groupId == expectedGroupId,
                    "Snapshot group route ordering should remain first-seen zone group order in Sprint 1.");
            require(route.zoneIds == expectedIterator->second.zoneIds,
                    "Snapshot group route membership should remain derived directly from zone membership in Sprint 1.");
            require(route.articulationIds == expectedIterator->second.articulationIds,
                    "Snapshot group articulation membership should remain derived directly from grouped zones in Sprint 1.");
        }

        const auto serializedSnapshot = serializeImmutablePlaybackSnapshot(snapshotResult.snapshot);
        require(serializedSnapshot.find("\"groupRoutes\"") != std::string::npos,
                "Sprint 1 playback snapshots must continue to serialize groupRoutes.");
        require(serializedSnapshot.find("\"groups\"") == std::string::npos,
                "Sprint 1 playback snapshots must not serialize explicit authored groups yet.");

        const auto manifestDirectory =
            std::filesystem::path(projectLoad.manifestPath.empty() ? "." : projectLoad.manifestPath).parent_path();
        const auto tempPath = manifestDirectory / "drs-phase1-zone-groups-contract-roundtrip.json";
        {
            std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
            require(output.good(), "Could not open the temporary Zone Groups Sprint 1 manifest path.");
            output << serializedProject;
        }

        const auto roundTripLoad = loadRuntimeProjectManifest(tempPath.generic_string());
        std::filesystem::remove(tempPath);

        require(roundTripLoad.loaded, "Sprint 1 project contract manifest should round-trip through the loader.");
        require(roundTripLoad.project.authoring.zones.size() == project.authoring.zones.size(),
                "Sprint 1 project round-trip should preserve authored zone count.");

        for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
        {
            require(roundTripLoad.project.authoring.zones[index].id == project.authoring.zones[index].id,
                    "Sprint 1 project round-trip should preserve zone ordering.");
            require(roundTripLoad.project.authoring.zones[index].groupId
                        == project.authoring.zones[index].groupId,
                    "Sprint 1 project round-trip should preserve per-zone group membership exactly.");
        }

        std::cout << "Phase 1 Zone Groups Sprint 1 contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 Zone Groups Sprint 1 contract tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
