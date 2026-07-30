#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

std::string joinIssues(const std::vector<std::string>& issues)
{
    std::string joined;
    for (std::size_t index = 0; index < issues.size(); ++index)
    {
        if (index != 0)
            joined += " | ";
        joined += issues[index];
    }
    return joined;
}

std::size_t requireGroupIndex(const drs::engine::RuntimeProjectModel& project,
                              const std::string& groupId)
{
    for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
    {
        if (project.authoring.groups[index].id == groupId)
            return index;
    }

    throw std::runtime_error("Could not resolve group '" + groupId + "'.");
}

bool hasGroupId(const drs::engine::RuntimeProjectModel& project, const std::string& groupId)
{
    for (const auto& group : project.authoring.groups)
        if (group.id == groupId)
            return true;
    return false;
}

std::string describeGroupOrder(const drs::engine::RuntimeProjectModel& project)
{
    std::string description;
    for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
    {
        if (index != 0)
            description += " | ";
        description += project.authoring.groups[index].id
            + ":" + std::to_string(project.authoring.groups[index].displayOrder);
    }
    return description;
}

drs::engine::RuntimeProjectModel makeLegalGroupRoutingFixture()
{
    const auto loaded = drs::engine::loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "Legal group-routing fixture requires the Phase 2 reference project.");

    auto project = loaded.project;
    const auto padGroupIndex = requireGroupIndex(project, "pad-core");
    project.authoring.routingBuses[0].inputSourceId = "groups/pad-core";
    project.authoring.groups[padGroupIndex].routingBusId = project.authoring.routingBuses[0].id;
    return project;
}

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto loaded = loadPhase2ReferenceProjectManifest();
        require(loaded.loaded, "Phase 2 reference project must load before Zone Group transaction tests run.");
        const auto& baseProject = loaded.project;
        require(baseProject.authoring.groups.size() == 2
                    && baseProject.authoring.selectedGroupId == "lead-core",
                "Phase 2 reference fixture group baseline changed unexpectedly.");

        AuthoringSession selectionSession(baseProject);
        require(selectionSession.getSelectedGroup().has_value()
                    && selectionSession.getSelectedGroup()->id == "lead-core",
                "Initial selected group should remain the lead fixture group.");
        require(selectionSession.selectGroup("pad-core").applied,
                "Selecting an existing authored group should create an undoable transaction.");
        require(selectionSession.getSelectedGroup()->id == "pad-core"
                    && selectionSession.getSelectedZone()->groupId == "pad-core",
                "Selecting a group should move authoring focus onto one of that group's member zones.");

        AuthoringSession deleteFallbackSession(baseProject);
        const auto deleteFallbackResult = deleteFallbackSession.deleteSelectedSample();
        require(deleteFallbackResult.applied,
                "Deleting the selected lead zone should remain an undoable authoring transaction. Issues: "
                    + joinIssues(deleteFallbackResult.issues));
        require(deleteFallbackSession.getProject().authoring.selectedGroupId == "pad-core"
                    && deleteFallbackSession.getSelectedZone()->groupId == "pad-core",
                "Deleting the final zone in the selected group should deterministically fall back to another group.");

        AuthoringSession createSession(baseProject);
        RuntimeProjectGroupDefinition airGroup;
        airGroup.id = "air-stack";
        airGroup.displayName = "Air Stack";
        const auto createResult = createSession.createGroup(airGroup, "Create air group");
        require(createResult.applied,
                "Creating an empty authored group should produce an undoable document transaction.");
        require(createSession.getProject().authoring.selectedGroupId == "air-stack"
                    && createSession.getProject().authoring.selectedZoneId == baseProject.authoring.selectedZoneId,
                "Creating an empty group should select the new group without disturbing the active zone selection.");
        require(!createSession.createGroup(airGroup, "Create duplicate air group").applied,
                "Duplicate group ids must be rejected before entering document history.");

        const auto createdGroupIndex = requireGroupIndex(createSession.getProject(), "air-stack");
        require(createSession.moveGroup(createdGroupIndex, -1, "Move air group earlier").applied,
                "Reordering authored groups should produce an undoable transaction.");
        require(createSession.getProject().authoring.groups[1].id == "air-stack"
                    && createSession.getProject().authoring.groups[1].displayOrder == 1,
                "Group reordering should update both authored order and normalized displayOrder. Actual: "
                    + describeGroupOrder(createSession.getProject()));
        require(createSession.undo().applied
                    && requireGroupIndex(createSession.getProject(), "air-stack") == 2,
                "Undo should restore the previous authored group order.");
        require(createSession.redo().applied
                    && requireGroupIndex(createSession.getProject(), "air-stack") == 1,
                "Redo should restore the reordered authored group order.");

        AuthoringSession hideSession(baseProject);
        require(hideSession.selectGroup("pad-core").applied,
                "Hide fallback coverage should be able to select the pad group.");
        const auto padGroupIndex = requireGroupIndex(hideSession.getProject(), "pad-core");
        auto hiddenPadGroup = hideSession.getProject().authoring.groups[padGroupIndex];
        hiddenPadGroup.displayName = "Pads";
        hiddenPadGroup.gainDb = -2.0;
        hiddenPadGroup.pan = 0.25;
        hiddenPadGroup.workspaceVisible = false;
        require(hideSession.updateGroup(padGroupIndex, hiddenPadGroup, "Hide pad group").applied,
                "Editing group visibility and mix fields should remain an undoable authoring transaction.");
        require(hideSession.getProject().authoring.groups[padGroupIndex].displayName == "Pads"
                    && hideSession.getProject().authoring.groups[padGroupIndex].gainDb == -2.0
                    && hideSession.getProject().authoring.groups[padGroupIndex].pan == 0.25
                    && !hideSession.getProject().authoring.groups[padGroupIndex].workspaceVisible,
                "Group edit transactions should persist renamed display, mix, and visibility fields.");
        require(hideSession.getProject().authoring.selectedGroupId == "lead-core"
                    && hideSession.getSelectedZone()->groupId == "lead-core",
                "Hiding the selected group should fall back to a visible authored group and its representative zone.");

        AuthoringSession routingSession(makeLegalGroupRoutingFixture());
        const auto routingPadGroupIndex = requireGroupIndex(routingSession.getProject(), "pad-core");
        auto routedPadGroup = routingSession.getProject().authoring.groups[routingPadGroupIndex];
        routedPadGroup.displayName = "Pad Stack";
        routedPadGroup.gainDb = -3.0;
        routedPadGroup.pan = 0.2;
        require(routingSession.updateGroup(routingPadGroupIndex,
                                           routedPadGroup,
                                           "Tune grouped routing and mix").applied,
                "Group edits should support legal routing-bus ownership alongside mix-field edits.");
        require(routingSession.getProject().authoring.groups[routingPadGroupIndex].routingBusId
                        == routingSession.getProject().authoring.routingBuses[0].id
                    && routingSession.getProject().authoring.groups[routingPadGroupIndex].gainDb == -3.0,
                "Group edits should preserve legal routingBus ownership while updating mix fields.");

        AuthoringSession reassignSession(baseProject);
        RuntimeProjectGroupDefinition layerGroup;
        layerGroup.id = "layer-core";
        layerGroup.displayName = "Layer Core";
        require(reassignSession.createGroup(layerGroup, "Create layer group").applied,
                "Reassignment coverage should be able to create an empty destination group.");
        require(reassignSession.reassignZoneToGroup("lead-a4-sustain", "layer-core", "Move lead zone to layer group").applied,
                "Moving a zone between groups should remain an undoable authoring transaction.");
        require(reassignSession.getSelectedZone()->groupId == "layer-core"
                    && reassignSession.getProject().authoring.selectedGroupId == "layer-core",
                "Reassigning the selected zone should move selectedGroupId with it.");
        require(reassignSession.deleteGroup("lead-core", "Delete empty lead group").applied,
                "Deleting an empty authored group should be allowed.");
        require(!hasGroupId(reassignSession.getProject(), "lead-core"),
                "Deleting an empty group should remove it from the authored group set.");
        require(!reassignSession.deleteGroup("pad-core", "Delete non-empty pad group").applied,
                "Non-empty authored groups must be rejected by delete transactions.");
        require(!reassignSession.reassignZoneToGroup("lead-a4-sustain", "missing-group", "Reject missing target").applied,
                "Zone reassignment must reject unknown target groups.");

        auto groupRoundRobinProject = baseProject;
        auto compatiblePadZone = groupRoundRobinProject.authoring.zones.front();
        compatiblePadZone.id = "pad-a3-low-alt";
        compatiblePadZone.displayName = "Pad Low Alt";
        compatiblePadZone.roundRobin.reset();
        compatiblePadZone.roundRobinLength = 0;
        compatiblePadZone.roundRobinPosition = 0;
        groupRoundRobinProject.authoring.zones.push_back(compatiblePadZone);
        auto compatibleHighPadZone = groupRoundRobinProject.authoring.zones[1];
        compatibleHighPadZone.id = "pad-a3-high-alt";
        compatibleHighPadZone.displayName = "Pad High Alt";
        compatibleHighPadZone.roundRobin.reset();
        compatibleHighPadZone.roundRobinLength = 0;
        compatibleHighPadZone.roundRobinPosition = 0;
        groupRoundRobinProject.authoring.zones.push_back(compatibleHighPadZone);
        AuthoringSession groupRoundRobinSession(groupRoundRobinProject);
        require(groupRoundRobinSession.selectGroup("pad-core").applied,
                "Group Round Robin coverage requires selecting the pad group.");
        require(groupRoundRobinSession.addCompatibleZonesToSelectedGroupRoundRobinPool(
                    "Pool compatible pad group zones").applied,
                "Group-owned Round Robin actions should be able to pool anchor-compatible zones.");
        const auto& groupedZones = groupRoundRobinSession.getProject().authoring.zones;
        const auto anchorIterator = std::find_if(groupedZones.begin(), groupedZones.end(),
                                                 [](const auto& zone)
                                                 {
                                                     return zone.id == "pad-a3-low";
                                                 });
        const auto compatibleIterator = std::find_if(groupedZones.begin(), groupedZones.end(),
                                                     [](const auto& zone)
                                                     {
                                                         return zone.id == "pad-a3-low-alt";
                                                     });
        const auto highIterator = std::find_if(groupedZones.begin(), groupedZones.end(),
                                               [](const auto& zone)
                                               {
                                                   return zone.id == "pad-a3-high";
                                               });
        const auto compatibleHighIterator = std::find_if(groupedZones.begin(), groupedZones.end(),
                                                         [](const auto& zone)
                                                         {
                                                             return zone.id == "pad-a3-high-alt";
                                                         });
        require(anchorIterator != groupedZones.end()
                    && compatibleIterator != groupedZones.end()
                    && highIterator != groupedZones.end()
                    && compatibleHighIterator != groupedZones.end()
                    && anchorIterator->roundRobin.has_value()
                    && compatibleIterator->roundRobin.has_value()
                    && anchorIterator->roundRobin->poolId == compatibleIterator->roundRobin->poolId
                    && highIterator->roundRobin.has_value()
                    && compatibleHighIterator->roundRobin.has_value()
                    && highIterator->roundRobin->poolId == compatibleHighIterator->roundRobin->poolId
                    && highIterator->roundRobin->poolId != anchorIterator->roundRobin->poolId,
                "Group-owned Round Robin must include every zone while keeping distinct mappings in separate pools.");
        require(groupRoundRobinSession.normalizeSelectedGroupRoundRobinPool(
                    "Normalize selected-group Round Robin").applied,
                "Group-owned Round Robin pools should support normalization from the group surface.");
        require(groupRoundRobinSession.setSelectedGroupRoundRobinMode(
                    RoundRobinMode::random,
                    "Use random selected-group Round Robin").applied,
                "Group-owned Round Robin should support switching from cycle to random mode.");

        const auto roundRobinPath = fs::temp_directory_path() / "drs-phase2-zone-group-round-robin.drsproj";
        const auto roundRobinText = serializeRuntimeProjectManifest(groupRoundRobinSession.getProject(),
                                                                    roundRobinPath.generic_string());
        {
            std::ofstream output(roundRobinPath, std::ios::binary | std::ios::trunc);
            output << roundRobinText;
        }
        const auto roundRobinLoad = loadRuntimeProjectManifest(roundRobinPath.generic_string());
        fs::remove(roundRobinPath);
        require(roundRobinLoad.loaded, "Group-owned Round Robin state should survive project save/load.");
        const auto loadedAnchor = std::find_if(roundRobinLoad.project.authoring.zones.begin(),
                                               roundRobinLoad.project.authoring.zones.end(),
                                               [](const auto& zone)
                                               {
                                                   return zone.id == "pad-a3-low";
                                               });
        const auto loadedCompatible = std::find_if(roundRobinLoad.project.authoring.zones.begin(),
                                                   roundRobinLoad.project.authoring.zones.end(),
                                                   [](const auto& zone)
                                                   {
                                                       return zone.id == "pad-a3-low-alt";
                                                   });
        require(loadedAnchor != roundRobinLoad.project.authoring.zones.end()
                    && loadedCompatible != roundRobinLoad.project.authoring.zones.end()
                    && loadedAnchor->roundRobin.has_value()
                    && loadedCompatible->roundRobin.has_value()
                    && loadedAnchor->roundRobin->poolId == loadedCompatible->roundRobin->poolId
                    && loadedAnchor->roundRobin->mode == RoundRobinMode::random
                    && loadedCompatible->roundRobin->mode == RoundRobinMode::random,
                "Group-owned Round Robin save/load coverage should preserve pooled zones and random mode.");

        const auto tempPath = fs::temp_directory_path() / "drs-phase2-zone-group-transactions.drsproj";
        const auto roundTripText = serializeRuntimeProjectManifest(reassignSession.getProject(),
                                                                   tempPath.generic_string());
        {
            std::ofstream output(tempPath, std::ios::binary | std::ios::trunc);
            output << roundTripText;
        }
        const auto roundTripLoad = loadRuntimeProjectManifest(tempPath.generic_string());
        fs::remove(tempPath);
        require(roundTripLoad.loaded, "Zone Group transaction round-trip project should reload successfully.");
        require(roundTripLoad.project.authoring.selectedGroupId == "layer-core"
                    && hasGroupId(roundTripLoad.project, "layer-core")
                    && !hasGroupId(roundTripLoad.project, "lead-core")
                    && readText(loaded.manifestPath).find("\"selectedGroupId\"") != std::string::npos,
                "Zone Group transaction save/load coverage should preserve selectedGroupId and authored group mutations.");

        std::cout << "Phase 2 Zone Group transaction tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 Zone Group transaction tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
