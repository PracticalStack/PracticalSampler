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
    const auto& repairedProject = session.getProject();
    const auto& repairedAnchor = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "pad-a3-low")];
    const auto& repairedCompatible = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "pad-a3-low-alt")];
    const auto& repairedForeign = repairedProject.authoring.zones[requireZoneIndex(repairedProject, "lead-a4-sustain")];

    require(!repairedAnchor.roundRobin.has_value()
                && !repairedCompatible.roundRobin.has_value()
                && !repairedForeign.roundRobin.has_value(),
            "Opening malformed partial or cross-group pools must clear Round Robin from every affected group.");
}

void runAllOrNothingGroupRoundRobinCoverage()
{
    using namespace drs::engine;

    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Group RR contract coverage requires the Phase 2 reference project.");

    auto project = loaded.project;
    for (const auto sourceZoneId : { std::string("pad-a3-low"), std::string("pad-a3-high") })
    {
        auto alternate = project.authoring.zones[requireZoneIndex(project, sourceZoneId)];
        alternate.id += "-alt";
        alternate.displayName += " Alt";
        alternate.roundRobin.reset();
        alternate.roundRobinLength = 0;
        alternate.roundRobinPosition = 0;
        project.authoring.zones.push_back(std::move(alternate));
    }
    project.authoring.selectedGroupId = "pad-core";
    project.authoring.selectedZoneId = "pad-a3-low";

    AuthoringSession session(project);
    const auto initialStatus = session.getSelectedGroupRoundRobinStatus();
    require(initialStatus.eligible && !initialStatus.enabled,
            "A group whose every mapping has an alternate should be eligible while its toggle remains off.");

    require(session.setSelectedGroupRoundRobinEnabled(
                true, RoundRobinMode::sequential, "Enable group Round Robin").applied,
            "An eligible group should enable Round Robin as one transaction.");
    const auto enabledStatus = session.getSelectedGroupRoundRobinStatus();
    require(enabledStatus.enabled && enabledStatus.mode == RoundRobinMode::sequential,
            "Enabling group Round Robin should pool every member in cycle mode.");
    for (const auto& zone : session.getProject().authoring.zones)
    {
        if (zone.groupId == "pad-core")
            require(zone.roundRobin.has_value(),
                    "Every member of an enabled Round Robin group must carry a descriptor.");
    }

    require(session.setSelectedGroupRoundRobinMode(
                RoundRobinMode::random, "Use random group Round Robin").applied,
            "An enabled group should switch from cycle to random mode.");
    require(session.getSelectedGroupRoundRobinStatus().mode == RoundRobinMode::random,
            "The selected group should report random mode after the mode transaction.");

    auto compatibleImport = session.getProject().authoring.zones[
        requireZoneIndex(session.getProject(), "pad-a3-low")];
    compatibleImport.id = "pad-a3-low-imported";
    compatibleImport.displayName = "Pad Low Imported";
    compatibleImport.roundRobin.reset();
    compatibleImport.roundRobinLength = 0;
    compatibleImport.roundRobinPosition = 0;
    require(session.appendImportedContent({},
                                          { compatibleImport },
                                          {},
                                          {},
                                          "Import compatible zone into enabled group",
                                          false).applied,
            "Importing into a schema-4 group project should not attempt a legacy Round Robin migration.");
    require(session.getProject().schemaVersion == 4
                && session.getProject().authoring.schemaVersion == 3
                && session.getSelectedGroupRoundRobinStatus().enabled
                && session.getSelectedGroupRoundRobinStatus().mode == RoundRobinMode::random,
            "A compatible import should preserve the group schema and rebuild the enabled random pool.");

    require(session.selectZone("pad-a3-low-alt").applied,
            "Invalidation coverage should select one member of the enabled group.");
    auto incompatibleZone = *session.getSelectedZone();
    incompatibleZone.rootKey += 1;
    require(session.updateSelectedZone(incompatibleZone, "Break group Round Robin eligibility").applied,
            "Editing an enabled group member should remain an undoable transaction.");

    const auto invalidatedStatus = session.getSelectedGroupRoundRobinStatus();
    require(!invalidatedStatus.enabled
                && !invalidatedStatus.eligible
                && std::find(invalidatedStatus.incompatibleZoneIds.begin(),
                             invalidatedStatus.incompatibleZoneIds.end(),
                             "pad-a3-low-alt") != invalidatedStatus.incompatibleZoneIds.end(),
            "An incompatible member must disable the whole group and identify the offending zone.");
    for (const auto& zone : session.getProject().authoring.zones)
    {
        if (zone.groupId == "pad-core")
            require(!zone.roundRobin.has_value()
                        && zone.roundRobinLength == 0
                        && zone.roundRobinPosition == 0,
                    "Invalidating one member must remove Round Robin metadata from the whole group.");
    }

    const auto rejectedEnable = session.setSelectedGroupRoundRobinEnabled(
        true, RoundRobinMode::random, "Reject invalid group Round Robin");
    require(!rejectedEnable.applied
                && !rejectedEnable.issues.empty()
                && rejectedEnable.issues.front().find("pad-a3-low-alt") != std::string::npos
                && !session.getSelectedGroupRoundRobinStatus().enabled,
            "Re-enabling an invalid group must warn about incorrect zones and keep the toggle off.");
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
        runAllOrNothingGroupRoundRobinCoverage();
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
