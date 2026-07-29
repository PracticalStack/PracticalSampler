#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::size_t requireZoneIndex(const drs::engine::RuntimeProjectModel& project,
                             const std::string& zoneId)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const auto& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    require(iterator != project.authoring.zones.end(),
            "Could not resolve zone '" + zoneId + "'.");
    return static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), iterator));
}

std::size_t requireGroupIndex(const drs::engine::RuntimeProjectModel& project,
                              const std::string& groupId)
{
    const auto iterator = std::find_if(project.authoring.groups.begin(),
                                       project.authoring.groups.end(),
                                       [&](const auto& group)
                                       {
                                           return group.id == groupId;
                                       });
    require(iterator != project.authoring.groups.end(),
            "Could not resolve group '" + groupId + "'.");
    return static_cast<std::size_t>(std::distance(project.authoring.groups.begin(), iterator));
}

void applyRoundRobinAssignment(drs::engine::RuntimeProjectZoneDefinition& zone,
                               const std::string& poolId,
                               int slotCount,
                               int slotIndex)
{
    zone.roundRobin = drs::engine::RoundRobinDescriptor {
        poolId,
        slotCount,
        slotIndex,
        drs::engine::RoundRobinMode::sequential
    };
    zone.roundRobinLength = slotCount;
    zone.roundRobinPosition = slotIndex;
}

void runDeletedAnchorRecoveryCoverage()
{
    using namespace drs::engine;

    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Sprint 8 anchor recovery coverage requires the Phase 2 reference project.");

    AuthoringSession session(loaded.project);
    require(session.selectGroup("pad-core").applied,
            "Sprint 8 anchor recovery coverage should be able to select the pad group.");
    require(session.selectZone("pad-a3-low").applied,
            "Sprint 8 anchor recovery coverage should be able to select the pad anchor zone.");

    const auto deleteResult = session.deleteSelectedSample();
    require(deleteResult.applied,
            "Deleting the selected group audition anchor should remain undoable.");

    const auto selectedGroup = session.getSelectedGroup();
    require(selectedGroup.has_value()
                && selectedGroup->id == "pad-core"
                && selectedGroup->auditionAnchorZoneId == "pad-a3-high",
            "Deleting a group's audition anchor should fall back to the next surviving member zone.");
    require(session.getSelectedZone().has_value()
                && session.getSelectedZone()->id == "pad-a3-high",
            "Deleting the selected anchor should keep authoring focus on the surviving group member.");

    const auto previewRequest = session.buildSelectedGroupPreviewRequest();
    require(previewRequest.available
                && previewRequest.groupId == "pad-core"
                && previewRequest.anchorZoneId == "pad-a3-high",
            "Selected-group preview should recover onto the fallback audition anchor.");
}

void runMalformedGroupRoundRobinRepairCoverage()
{
    using namespace drs::engine;

    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Sprint 8 RR repair coverage requires the Phase 2 reference project.");

    auto project = loaded.project;
    auto compatibleZone = project.authoring.zones[requireZoneIndex(project, "pad-a3-low")];
    compatibleZone.id = "pad-a3-low-alt";
    compatibleZone.displayName = "Pad Low Alt";
    compatibleZone.roundRobin.reset();
    compatibleZone.roundRobinLength = 0;
    compatibleZone.roundRobinPosition = 0;
    project.authoring.zones.push_back(compatibleZone);

    auto& anchorZone = project.authoring.zones[requireZoneIndex(project, "pad-a3-low")];
    auto& foreignZone = project.authoring.zones[requireZoneIndex(project, "lead-a4-sustain")];
    applyRoundRobinAssignment(anchorZone, "rr-shared", 2, 1);
    applyRoundRobinAssignment(foreignZone, "rr-shared", 2, 2);
    project.authoring.selectedGroupId = "pad-core";
    project.authoring.selectedZoneId = "pad-a3-low";

    AuthoringSession session(project);
    require(session.addCompatibleZonesToSelectedGroupRoundRobinPool(
                "Repair malformed selected-group RR pool").applied,
            "Group-owned RR add-compatible should repair malformed foreign ownership before expanding the pool.");

    const auto& repairedProject = session.getProject();
    const auto& repairedAnchor = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "pad-a3-low")];
    const auto& repairedCompatible = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "pad-a3-low-alt")];
    const auto& repairedForeign = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "lead-a4-sustain")];

    require(repairedAnchor.roundRobin.has_value()
                && repairedCompatible.roundRobin.has_value()
                && repairedForeign.roundRobin.has_value(),
            "Sprint 8 RR repair coverage expects all authored RR descriptors to remain explicit.");
    require(repairedAnchor.roundRobin->poolId == repairedCompatible.roundRobin->poolId
                && repairedAnchor.roundRobin->slotCount == 2
                && repairedCompatible.roundRobin->slotCount == 2,
            "Compatible group members should remain pooled together after malformed RR repair.");
    require(repairedForeign.roundRobin->poolId != repairedAnchor.roundRobin->poolId
                && repairedForeign.roundRobin->slotCount == 1
                && repairedForeign.roundRobin->slotIndex == 1,
            "Malformed foreign RR ownership should be isolated into its own single-zone pool.");
}

drs::engine::RuntimeProjectModel makeLargeHiddenGroupFixture()
{
    using namespace drs::engine;

    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Sprint 8 large-project coverage requires the Phase 2 reference project.");

    auto project = loaded.project;
    const auto lowTemplate = project.authoring.zones[requireZoneIndex(project, "pad-a3-low")];
    const auto highTemplate = project.authoring.zones[requireZoneIndex(project, "pad-a3-high")];

    constexpr auto groupCount = 24;
    for (int index = 0; index < groupCount; ++index)
    {
        RuntimeProjectGroupDefinition group;
        group.id = "stress-group-" + std::to_string(index);
        group.displayName = "Stress Group " + std::to_string(index + 1);
        group.displayOrder = index;
        group.workspaceVisible = (index % 2) == 0;
        group.gainDb = -0.25 * static_cast<double>(index % 4);
        group.pan = 0.1 * static_cast<double>((index % 5) - 2);
        group.auditionAnchorZoneId = "stress-low-" + std::to_string(index);
        project.authoring.groups.push_back(group);

        auto lowZone = lowTemplate;
        lowZone.id = "stress-low-" + std::to_string(index);
        lowZone.displayName = "Stress Low " + std::to_string(index + 1);
        lowZone.groupId = group.id;
        lowZone.roundRobin.reset();
        lowZone.roundRobinLength = 0;
        lowZone.roundRobinPosition = 0;
        project.authoring.zones.push_back(lowZone);

        auto highZone = highTemplate;
        highZone.id = "stress-high-" + std::to_string(index);
        highZone.displayName = "Stress High " + std::to_string(index + 1);
        highZone.groupId = group.id;
        highZone.roundRobin.reset();
        highZone.roundRobinLength = 0;
        highZone.roundRobinPosition = 0;
        project.authoring.zones.push_back(highZone);
    }

    project.authoring.selectedGroupId = "stress-group-10";
    project.authoring.selectedZoneId = "stress-low-10";
    return project;
}

void runLargeProjectVisibilityStabilityCoverage()
{
    using namespace drs::engine;

    auto project = makeLargeHiddenGroupFixture();
    AuthoringSession session(project);
    const auto previewRequest = session.buildSelectedGroupPreviewRequest();
    require(previewRequest.available
                && previewRequest.groupId == "stress-group-10"
                && previewRequest.anchorZoneId == "stress-low-10",
            "Large authored group fixtures should still expose selected-group preview requests.");

    PlaybackSnapshotBuilder builder;
    const auto baseline = builder.buildSnapshot(builder.requestBuild(80, true), project);
    require(baseline.built && baseline.activationEligible,
            "Large authored group fixtures should remain snapshot-eligible.");
    require(baseline.snapshot.groupRoutes.size() == project.authoring.groups.size()
                && baseline.snapshot.zones.size() == project.authoring.zones.size(),
            "Large authored group fixtures should preserve every group and zone in immutable snapshots.");

    auto visibilityOnlyProject = project;
    for (auto& group : visibilityOnlyProject.authoring.groups)
        group.workspaceVisible = !group.workspaceVisible;

    const auto visibilityOnly = builder.buildSnapshot(builder.requestBuild(81, true), visibilityOnlyProject);
    require(visibilityOnly.built && visibilityOnly.activationEligible,
            "Large visibility-only group edits should still build snapshots.");
    require(visibilityOnly.snapshot.groupRoutes.size() == baseline.snapshot.groupRoutes.size()
                && visibilityOnly.snapshot.zones.size() == baseline.snapshot.zones.size()
                && visibilityOnly.snapshot.selectedGroupId == baseline.snapshot.selectedGroupId,
            "Large visibility-only group edits should preserve group addressing and snapshot coverage.");

    AuthoringSession hiddenSession(visibilityOnlyProject);
    const auto hiddenPreviewRequest = hiddenSession.buildSelectedGroupPreviewRequest();
    require(hiddenPreviewRequest.available
                && hiddenPreviewRequest.groupId == "stress-group-10"
                && hiddenPreviewRequest.anchorZoneId == "stress-low-10",
            "Large hidden-group fixtures should keep selected-group preview addressable.");
}
} // namespace

int main()
{
    try
    {
        runDeletedAnchorRecoveryCoverage();
        runMalformedGroupRoundRobinRepairCoverage();
        runLargeProjectVisibilityStabilityCoverage();

        std::cout << "Sprint 8 Zone Group hardening tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 8 Zone Group hardening tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
