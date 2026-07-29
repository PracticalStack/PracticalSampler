#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cstddef>

namespace drs::engine
{
namespace
{
RuntimeProjectDocumentActionResult makeRejectedResult(const RuntimeProjectDocumentState& documentState,
                                                      const std::string& state,
                                                      const std::string& issue)
{
    RuntimeProjectDocumentActionResult result;
    result.state = state;
    result.issues.push_back(issue);
    result.documentState = documentState;
    return result;
}

bool sameVelocityCrossfadeDescriptor(const VelocityCrossfadeDescriptor& left,
                                     const VelocityCrossfadeDescriptor& right) noexcept
{
    return left.fadeInLowVelocity == right.fadeInLowVelocity
        && left.fadeInHighVelocity == right.fadeInHighVelocity
        && left.fadeOutLowVelocity == right.fadeOutLowVelocity
        && left.fadeOutHighVelocity == right.fadeOutHighVelocity
        && left.curve == right.curve;
}

bool isRoundRobinGroupingCompatible(const RuntimeProjectZoneDefinition& anchor,
                                    const RuntimeProjectZoneDefinition& candidate) noexcept
{
    const auto sameGroup = anchor.groupId == candidate.groupId;
    const auto sameArticulation = anchor.articulationId == candidate.articulationId;
    const auto sameRootKey = anchor.rootKey == candidate.rootKey;
    const auto sameKeyRange = anchor.keyLow == candidate.keyLow
        && anchor.keyHigh == candidate.keyHigh;
    const auto sameVelocityRange = anchor.velocityLow == candidate.velocityLow
        && anchor.velocityHigh == candidate.velocityHigh;
    const auto sameCrossfadeShape = sameVelocityCrossfadeDescriptor(
        anchor.velocityCrossfade, candidate.velocityCrossfade);
    const auto sameTriggerMode = anchor.triggerMode == candidate.triggerMode;

    return sameGroup
        && sameArticulation
        && sameRootKey
        && sameKeyRange
        && sameVelocityRange
        && sameCrossfadeShape
        && sameTriggerMode;
}

bool usesExplicitZoneGroupsSchema(const RuntimeProjectModel& project) noexcept
{
    return project.schemaVersion >= 4 && project.authoring.schemaVersion >= 3;
}

std::optional<std::size_t> findGroupIndexById(const RuntimeProjectModel& project,
                                              const std::string& groupId)
{
    const auto iterator = std::find_if(project.authoring.groups.begin(),
                                       project.authoring.groups.end(),
                                       [&](const RuntimeProjectGroupDefinition& group)
                                       {
                                           return group.id == groupId;
                                       });
    if (iterator == project.authoring.groups.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.groups.begin(), iterator));
}

std::optional<std::size_t> findZoneIndexById(const RuntimeProjectModel& project,
                                             const std::string& zoneId)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), iterator));
}

bool groupHasMembers(const RuntimeProjectModel& project, const std::string& groupId)
{
    return std::any_of(project.authoring.zones.begin(),
                       project.authoring.zones.end(),
                       [&](const RuntimeProjectZoneDefinition& zone)
                       {
                           return zone.groupId == groupId;
                       });
}

std::optional<std::string> findRepresentativeZoneIdForGroup(const RuntimeProjectModel& project,
                                                            const std::string& groupId)
{
    const auto groupIndex = findGroupIndexById(project, groupId);
    if (!groupIndex.has_value())
        return std::nullopt;

    const auto& group = project.authoring.groups[*groupIndex];
    const auto anchorIndex = findZoneIndexById(project, group.auditionAnchorZoneId);
    if (anchorIndex.has_value() && project.authoring.zones[*anchorIndex].groupId == groupId)
        return project.authoring.zones[*anchorIndex].id;

    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.groupId == groupId;
                                       });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;

    return iterator->id;
}

void normalizeGroupDisplayOrder(RuntimeProjectModel& project)
{
    for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        project.authoring.groups[index].displayOrder = static_cast<int>(index);
}

void sortAndNormalizeGroupDisplayOrder(RuntimeProjectModel& project)
{
    if (!usesExplicitZoneGroupsSchema(project))
        return;

    std::stable_sort(project.authoring.groups.begin(),
                     project.authoring.groups.end(),
                     [](const RuntimeProjectGroupDefinition& left,
                        const RuntimeProjectGroupDefinition& right)
                     {
                         if (left.displayOrder != right.displayOrder)
                             return left.displayOrder < right.displayOrder;
                         return left.id < right.id;
                     });

    normalizeGroupDisplayOrder(project);
}

void ensureExplicitZoneGroups(RuntimeProjectModel& project)
{
    if (!usesExplicitZoneGroupsSchema(project))
        return;

    auto& authoring = project.authoring;
    auto nextDisplayOrder = static_cast<int>(authoring.groups.size());
    for (const auto& group : authoring.groups)
        nextDisplayOrder = std::max(nextDisplayOrder, group.displayOrder + 1);

    for (const auto& zone : authoring.zones)
    {
        if (zone.groupId.empty())
            continue;

        const auto iterator = std::find_if(authoring.groups.begin(),
                                           authoring.groups.end(),
                                           [&](const RuntimeProjectGroupDefinition& group)
                                           {
                                               return group.id == zone.groupId;
                                           });
        if (iterator != authoring.groups.end())
            continue;

        RuntimeProjectGroupDefinition group;
        group.id = zone.groupId;
        group.displayName = zone.groupId;
        group.displayOrder = nextDisplayOrder++;
        group.workspaceVisible = true;
        group.gainDb = 0.0;
        group.pan = 0.0;
        group.auditionAnchorZoneId = zone.id;
        authoring.groups.push_back(std::move(group));
    }

    for (auto& group : authoring.groups)
    {
        if (group.displayName.empty())
            group.displayName = group.id;

        const auto anchorIterator = std::find_if(authoring.zones.begin(),
                                                 authoring.zones.end(),
                                                 [&](const RuntimeProjectZoneDefinition& zone)
                                                 {
                                                     return zone.id == group.auditionAnchorZoneId
                                                         && zone.groupId == group.id;
                                                 });
        if (anchorIterator != authoring.zones.end())
            continue;

        const auto firstMember = std::find_if(authoring.zones.begin(),
                                              authoring.zones.end(),
                                              [&](const RuntimeProjectZoneDefinition& zone)
                                              {
                                                  return zone.groupId == group.id;
                                              });
        group.auditionAnchorZoneId = firstMember != authoring.zones.end() ? firstMember->id : std::string {};
    }

    sortAndNormalizeGroupDisplayOrder(project);
}

void alignSelectedGroupToSelectedZone(RuntimeProjectModel& project)
{
    if (!usesExplicitZoneGroupsSchema(project))
        return;

    ensureExplicitZoneGroups(project);
    const auto zoneIndex = findZoneIndexById(project, project.authoring.selectedZoneId);
    if (zoneIndex.has_value())
    {
        project.authoring.selectedGroupId = project.authoring.zones[*zoneIndex].groupId;
        return;
    }

    const auto groupIndex = findGroupIndexById(project, project.authoring.selectedGroupId);
    if (groupIndex.has_value())
        return;

    project.authoring.selectedGroupId = project.authoring.groups.empty()
        ? std::string {}
        : project.authoring.groups.front().id;
}

std::string chooseFallbackSelectedGroupId(const RuntimeProjectModel& project,
                                          const std::string& preferredGroupId = {})
{
    if (!usesExplicitZoneGroupsSchema(project) || project.authoring.groups.empty())
        return {};

    const auto accept = [&](const RuntimeProjectGroupDefinition& group,
                            bool requireVisible,
                            bool requireMembers)
    {
        return (!requireVisible || group.workspaceVisible)
            && (!requireMembers || groupHasMembers(project, group.id));
    };

    const auto tryGroupId = [&](const std::string& candidateId,
                                bool requireVisible,
                                bool requireMembers) -> std::string
    {
        const auto groupIndex = findGroupIndexById(project, candidateId);
        if (!groupIndex.has_value())
            return {};

        const auto& group = project.authoring.groups[*groupIndex];
        return accept(group, requireVisible, requireMembers) ? group.id : std::string {};
    };

    if (!preferredGroupId.empty())
    {
        if (const auto preferred = tryGroupId(preferredGroupId, true, true); !preferred.empty())
            return preferred;
    }

    if (const auto zoneIndex = findZoneIndexById(project, project.authoring.selectedZoneId); zoneIndex.has_value())
    {
        const auto zoneGroupId = project.authoring.zones[*zoneIndex].groupId;
        if (const auto zoneGroup = tryGroupId(zoneGroupId, true, true); !zoneGroup.empty())
            return zoneGroup;
    }

    for (const auto& group : project.authoring.groups)
        if (accept(group, true, true))
            return group.id;

    if (!preferredGroupId.empty())
    {
        if (const auto preferred = tryGroupId(preferredGroupId, true, false); !preferred.empty())
            return preferred;
    }

    for (const auto& group : project.authoring.groups)
        if (accept(group, true, false))
            return group.id;

    if (const auto zoneIndex = findZoneIndexById(project, project.authoring.selectedZoneId); zoneIndex.has_value())
    {
        if (const auto zoneGroup = tryGroupId(project.authoring.zones[*zoneIndex].groupId, false, false);
            !zoneGroup.empty())
        {
            return zoneGroup;
        }
    }

    if (!preferredGroupId.empty())
    {
        if (const auto preferred = tryGroupId(preferredGroupId, false, false); !preferred.empty())
            return preferred;
    }

    return project.authoring.groups.front().id;
}

void applyFallbackGroupSelection(RuntimeProjectModel& project, const std::string& preferredGroupId = {})
{
    if (!usesExplicitZoneGroupsSchema(project))
        return;

    ensureExplicitZoneGroups(project);
    project.authoring.selectedGroupId = chooseFallbackSelectedGroupId(project, preferredGroupId);
    if (const auto representativeZoneId = findRepresentativeZoneIdForGroup(project, project.authoring.selectedGroupId);
        representativeZoneId.has_value())
    {
        project.authoring.selectedZoneId = *representativeZoneId;
    }
}

void clearRoundRobinAssignment(RuntimeProjectZoneDefinition& zone)
{
    zone.roundRobin.reset();
    zone.roundRobinLength = 0;
    zone.roundRobinPosition = 0;
}

void applyRoundRobinAssignment(RuntimeProjectZoneDefinition& zone,
                               const std::string& poolId,
                               int slotCount,
                               int slotIndex)
{
    zone.roundRobin = RoundRobinDescriptor {
        poolId,
        slotCount,
        slotIndex,
        RoundRobinMode::sequential
    };
    zone.roundRobinLength = slotCount;
    zone.roundRobinPosition = slotIndex;
}

std::vector<std::size_t> collectRoundRobinPoolMemberIndices(const RuntimeProjectModel& project,
                                                            const std::string& poolId)
{
    std::vector<std::size_t> indices;
    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        const auto& zone = project.authoring.zones[index];
        if (zone.roundRobin.has_value() && zone.roundRobin->poolId == poolId)
            indices.push_back(index);
    }
    return indices;
}

std::vector<std::size_t> collectCompatibleRoundRobinPoolMemberIndices(const RuntimeProjectModel& project,
                                                                      std::size_t anchorIndex,
                                                                      const std::string& poolId)
{
    std::vector<std::size_t> indices;
    const auto& anchorZone = project.authoring.zones[anchorIndex];
    for (const auto memberIndex : collectRoundRobinPoolMemberIndices(project, poolId))
    {
        if (isRoundRobinGroupingCompatible(anchorZone, project.authoring.zones[memberIndex]))
            indices.push_back(memberIndex);
    }

    if (std::find(indices.begin(), indices.end(), anchorIndex) == indices.end())
        indices.push_back(anchorIndex);

    return indices;
}

std::string allocateRoundRobinPoolId(const RuntimeProjectModel& project);

void normalizeRoundRobinPool(RuntimeProjectModel& project,
                             std::vector<std::size_t> memberIndices,
                             const std::string& poolId)
{
    std::stable_sort(memberIndices.begin(),
                     memberIndices.end(),
                     [&](std::size_t leftIndex, std::size_t rightIndex)
                     {
                         const auto& left = project.authoring.zones[leftIndex];
                         const auto& right = project.authoring.zones[rightIndex];
                         const auto leftSlot = left.roundRobin.has_value() ? left.roundRobin->slotIndex
                                                                           : left.roundRobinPosition;
                         const auto rightSlot = right.roundRobin.has_value() ? right.roundRobin->slotIndex
                                                                             : right.roundRobinPosition;
                         if (leftSlot != rightSlot)
                             return leftSlot < rightSlot;
                         return leftIndex < rightIndex;
                     });

    const auto slotCount = static_cast<int>(memberIndices.size());
    for (std::size_t ordinal = 0; ordinal < memberIndices.size(); ++ordinal)
        applyRoundRobinAssignment(project.authoring.zones[memberIndices[ordinal]],
                                  poolId,
                                  slotCount,
                                  static_cast<int>(ordinal) + 1);
}

void isolateMalformedRoundRobinPoolMembers(RuntimeProjectModel& project, std::size_t anchorIndex)
{
    auto& anchorZone = project.authoring.zones[anchorIndex];
    if (!anchorZone.roundRobin.has_value())
        return;

    const auto poolId = anchorZone.roundRobin->poolId;
    auto compatibleMemberIndices = collectCompatibleRoundRobinPoolMemberIndices(project,
                                                                                anchorIndex,
                                                                                poolId);
    auto malformedMemberIndices = collectRoundRobinPoolMemberIndices(project, poolId);
    malformedMemberIndices.erase(std::remove_if(malformedMemberIndices.begin(),
                                                malformedMemberIndices.end(),
                                                [&](const std::size_t memberIndex)
                                                {
                                                    return std::find(compatibleMemberIndices.begin(),
                                                                     compatibleMemberIndices.end(),
                                                                     memberIndex)
                                                        != compatibleMemberIndices.end();
                                                }),
                                 malformedMemberIndices.end());

    if (malformedMemberIndices.empty())
        return;

    normalizeRoundRobinPool(project, compatibleMemberIndices, poolId);
    normalizeRoundRobinPool(project,
                            malformedMemberIndices,
                            allocateRoundRobinPoolId(project));
}

std::string allocateRoundRobinPoolId(const RuntimeProjectModel& project)
{
    auto candidateIndex = 1;
    while (true)
    {
        const auto candidate = "rr-pool-" + std::to_string(candidateIndex);
        const auto exists = std::any_of(project.authoring.zones.begin(),
                                        project.authoring.zones.end(),
                                        [&](const RuntimeProjectZoneDefinition& zone)
                                        {
                                            return zone.roundRobin.has_value()
                                                && zone.roundRobin->poolId == candidate;
                                        });
        if (!exists)
            return candidate;

        ++candidateIndex;
    }
}

std::vector<AuthoringZoneSummary> buildZoneSummaries(const RuntimeProjectModel& project)
{
    std::vector<AuthoringZoneSummary> summaries;
    summaries.reserve(project.authoring.zones.size());

    for (const auto& zone : project.authoring.zones)
    {
        AuthoringZoneSummary summary;
        summary.id = zone.id;
        summary.displayName = zone.displayName;
        summary.sampleSourceId = zone.sampleSourceId;
        summary.articulationId = zone.articulationId;
        summary.rootKey = zone.rootKey;
        summary.keyLow = zone.keyLow;
        summary.keyHigh = zone.keyHigh;
        summary.velocityLow = zone.velocityLow;
        summary.velocityHigh = zone.velocityHigh;
        summary.velocityCrossfade = zone.velocityCrossfade;
        summary.gainDb = zone.gainDb;
        summary.pan = zone.pan;
        summary.loopEnabled = zone.loopEnabled;
        summary.roundRobin = zone.roundRobin;
        summary.roundRobinLength = zone.roundRobinLength;
        summary.roundRobinPosition = zone.roundRobinPosition;
        summary.triggerMode = zone.triggerMode;
        summary.selected = zone.id == project.authoring.selectedZoneId;
        summaries.push_back(std::move(summary));
    }

    return summaries;
}

std::optional<std::size_t> findSelectedZoneIndex(const RuntimeProjectModel& project)
{
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == project.authoring.selectedZoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.zones.begin(), iterator));
}

std::optional<std::size_t> findSelectedPerformanceBankIndex(const RuntimeProjectModel& project)
{
    const auto iterator = std::find_if(project.authoring.performanceBanks.begin(),
                                       project.authoring.performanceBanks.end(),
                                       [&](const RuntimeProjectPerformanceBankDefinition& performanceBank)
                                       {
                                           return performanceBank.id == project.authoring.selectedPerformanceBankId;
                                       });
    if (iterator == project.authoring.performanceBanks.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.performanceBanks.begin(), iterator));
}

std::optional<std::size_t> findSelectedGroupRoundRobinAnchorIndex(const RuntimeProjectModel& project)
{
    if (!usesExplicitZoneGroupsSchema(project))
        return std::nullopt;

    const auto groupIndex = findGroupIndexById(project, project.authoring.selectedGroupId);
    if (!groupIndex.has_value())
        return std::nullopt;

    const auto& group = project.authoring.groups[*groupIndex];
    if (const auto anchorIndex = findZoneIndexById(project, group.auditionAnchorZoneId);
        anchorIndex.has_value() && project.authoring.zones[*anchorIndex].groupId == group.id)
    {
        return anchorIndex;
    }

    if (const auto representativeZoneId = findRepresentativeZoneIdForGroup(project, group.id);
        representativeZoneId.has_value())
    {
        return findZoneIndexById(project, *representativeZoneId);
    }

    return std::nullopt;
}

std::vector<std::string> buildRoundRobinChangedPaths(const RuntimeProjectModel& project)
{
    std::vector<std::string> changedPaths { "authoring.zones" };
    if (usesExplicitZoneGroupsSchema(project))
    {
        changedPaths.push_back("authoring.groups");
        changedPaths.push_back("authoring.selectedGroupId");
    }
    return changedPaths;
}

void createRoundRobinPoolForAnchor(RuntimeProjectModel& project, std::size_t anchorIndex)
{
    auto& selectedZone = project.authoring.zones[anchorIndex];

    if (selectedZone.roundRobin.has_value())
    {
        auto previousPoolMembers = collectRoundRobinPoolMemberIndices(project, selectedZone.roundRobin->poolId);
        previousPoolMembers.erase(
            std::remove(previousPoolMembers.begin(), previousPoolMembers.end(), anchorIndex),
            previousPoolMembers.end());
        if (!previousPoolMembers.empty())
            normalizeRoundRobinPool(project, previousPoolMembers, selectedZone.roundRobin->poolId);
    }

    applyRoundRobinAssignment(selectedZone, allocateRoundRobinPoolId(project), 1, 1);
}

void addCompatibleZonesToAnchorRoundRobinPool(RuntimeProjectModel& project, std::size_t anchorIndex)
{
    const auto anchorZone = project.authoring.zones[anchorIndex];

    std::string poolId;
    std::vector<std::size_t> memberIndices;
    if (anchorZone.roundRobin.has_value())
    {
        poolId = anchorZone.roundRobin->poolId;
        memberIndices = collectRoundRobinPoolMemberIndices(project, poolId);
    }
    else
    {
        poolId = allocateRoundRobinPoolId(project);
        memberIndices.push_back(anchorIndex);
    }

    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        if (std::find(memberIndices.begin(), memberIndices.end(), index) != memberIndices.end())
            continue;

        const auto& candidate = project.authoring.zones[index];
        if (!isRoundRobinGroupingCompatible(anchorZone, candidate))
            continue;
        if (candidate.roundRobin.has_value())
            continue;

        memberIndices.push_back(index);
    }

    if (memberIndices.empty())
        memberIndices.push_back(anchorIndex);

    std::stable_sort(memberIndices.begin(),
                     memberIndices.end(),
                     [&](std::size_t leftIndex, std::size_t rightIndex)
                     {
                         if (!anchorZone.roundRobin.has_value())
                         {
                             if (leftIndex == anchorIndex)
                                 return true;
                             if (rightIndex == anchorIndex)
                                 return false;
                         }

                         const auto& left = project.authoring.zones[leftIndex];
                         const auto& right = project.authoring.zones[rightIndex];
                         const auto leftSlot = left.roundRobin.has_value() ? left.roundRobin->slotIndex : 0;
                         const auto rightSlot = right.roundRobin.has_value() ? right.roundRobin->slotIndex : 0;
                         if (leftSlot != rightSlot)
                         {
                             if (leftSlot == 0)
                                 return false;
                             if (rightSlot == 0)
                                 return true;
                             return leftSlot < rightSlot;
                         }
                         return leftIndex < rightIndex;
                     });

    normalizeRoundRobinPool(project, memberIndices, poolId);
}
} // namespace

AuthoringSession::AuthoringSession(RuntimeProjectModel project)
    : documentController(std::move(project))
{
}

const RuntimeProjectModel& AuthoringSession::getProject() const
{
    return documentController.getProject();
}

const RuntimeProjectDocumentState& AuthoringSession::getDocumentState() const
{
    return documentController.getDocumentState();
}

void AuthoringSession::replaceProject(RuntimeProjectModel project)
{
    documentController = RuntimeProjectDocumentController(std::move(project));
}

std::vector<AuthoringZoneSummary> AuthoringSession::getZoneSummaries() const
{
    return buildZoneSummaries(getProject());
}

std::optional<RuntimeProjectZoneDefinition> AuthoringSession::getSelectedZone() const
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return std::nullopt;

    return getProject().authoring.zones[*selectedZoneIndex];
}

std::optional<RuntimeProjectGroupDefinition> AuthoringSession::getSelectedGroup() const
{
    if (!usesExplicitZoneGroupsSchema(getProject()))
        return std::nullopt;

    const auto groupIndex = findGroupIndexById(getProject(), getProject().authoring.selectedGroupId);
    if (!groupIndex.has_value())
        return std::nullopt;

    return getProject().authoring.groups[*groupIndex];
}

std::optional<RuntimeProjectPerformanceBankDefinition> AuthoringSession::getSelectedPerformanceBank() const
{
    const auto selectedPerformanceBankIndex = findSelectedPerformanceBankIndex(getProject());
    if (!selectedPerformanceBankIndex.has_value())
        return std::nullopt;

    return getProject().authoring.performanceBanks[*selectedPerformanceBankIndex];
}

AuthoringZonePreviewRequest AuthoringSession::buildSelectedZonePreviewRequest() const
{
    AuthoringZonePreviewRequest request;
    const auto zone = getSelectedZone();
    if (!zone.has_value())
    {
        request.state = "No zone selected";
        return request;
    }

    request.available = true;
    request.midiNote = std::clamp(zone->rootKey, zone->keyLow, zone->keyHigh);
    request.velocity = std::clamp((zone->velocityLow + zone->velocityHigh) / 2, 1, 127);
    request.zoneId = zone->id;
    request.articulationId = zone->articulationId;
    request.state = "Zone preview ready";
    return request;
}

AuthoringGroupPreviewRequest AuthoringSession::buildSelectedGroupPreviewRequest() const
{
    AuthoringGroupPreviewRequest request;
    const auto group = getSelectedGroup();
    if (!group.has_value())
    {
        request.state = "No group selected";
        return request;
    }

    const auto anchorIndex = findSelectedGroupRoundRobinAnchorIndex(getProject());
    if (!anchorIndex.has_value())
    {
        request.groupId = group->id;
        request.state = "Selected group has no auditionable zones";
        return request;
    }

    const auto& anchorZone = getProject().authoring.zones[*anchorIndex];
    request.available = true;
    request.groupId = group->id;
    request.anchorZoneId = anchorZone.id;
    request.midiNote = std::clamp(anchorZone.rootKey, anchorZone.keyLow, anchorZone.keyHigh);
    request.velocity = std::clamp((anchorZone.velocityLow + anchorZone.velocityHigh) / 2, 1, 127);
    request.state = "Group preview ready";
    return request;
}

RuntimeProjectDocumentActionResult AuthoringSession::selectZone(const std::string& zoneId)
{
    auto project = getProject();
    const auto iterator = std::find_if(project.authoring.zones.begin(),
                                       project.authoring.zones.end(),
                                       [&](const RuntimeProjectZoneDefinition& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    if (iterator == project.authoring.zones.end())
        return makeRejectedResult(getDocumentState(),
                                  "Zone selection rejected",
                                  "Zone '" + zoneId + "' does not exist in the current authoring project.");

    project.authoring.selectedZoneId = zoneId;
    alignSelectedGroupToSelectedZone(project);

    std::vector<std::string> changedPaths { "authoring.selectedZoneId" };
    if (usesExplicitZoneGroupsSchema(project))
        changedPaths.push_back("authoring.selectedGroupId");

    return documentController.commitSnapshot(project, "Select zone", changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::selectPerformanceBank(const std::string& performanceBankId)
{
    auto project = getProject();
    const auto iterator = std::find_if(project.authoring.performanceBanks.begin(),
                                       project.authoring.performanceBanks.end(),
                                       [&](const RuntimeProjectPerformanceBankDefinition& performanceBank)
                                       {
                                           return performanceBank.id == performanceBankId;
                                       });
    if (iterator == project.authoring.performanceBanks.end())
        return makeRejectedResult(getDocumentState(),
                                  "Performance-bank selection rejected",
                                  "Performance bank '" + performanceBankId + "' does not exist in the current authoring project.");

    project.authoring.selectedPerformanceBankId = performanceBankId;
    return documentController.commitSnapshot(project,
                                             "Select performance bank",
                                             {"authoring.selectedPerformanceBankId"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updateSelectedZone(const RuntimeProjectZoneDefinition& zone,
                                                                        const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Zone edit rejected",
                                  "No zone is currently selected for editing.");

    auto project = getProject();
    project.authoring.zones[*selectedZoneIndex] = zone;
    project.authoring.selectedZoneId = zone.id;
    alignSelectedGroupToSelectedZone(project);

    std::vector<std::string> changedPaths {
        "authoring.zones[" + std::to_string(*selectedZoneIndex) + "]",
        "authoring.selectedZoneId"
    };
    if (usesExplicitZoneGroupsSchema(project))
        changedPaths.push_back("authoring.selectedGroupId");

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::selectGroup(const std::string& groupId)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Group selection rejected",
                                  "This project schema does not support explicit group selection.");

    ensureExplicitZoneGroups(project);
    const auto groupIndex = findGroupIndexById(project, groupId);
    if (!groupIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group selection rejected",
                                  "Group '" + groupId + "' does not exist in the current authoring project.");

    project.authoring.selectedGroupId = groupId;
    std::vector<std::string> changedPaths { "authoring.selectedGroupId" };
    if (const auto representativeZoneId = findRepresentativeZoneIdForGroup(project, groupId);
        representativeZoneId.has_value()
        && project.authoring.selectedZoneId != *representativeZoneId)
    {
        project.authoring.selectedZoneId = *representativeZoneId;
        changedPaths.push_back("authoring.selectedZoneId");
    }

    return documentController.commitSnapshot(project, "Select group", changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::createGroup(const RuntimeProjectGroupDefinition& group,
                                                                 const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Group creation rejected",
                                  "This project schema does not support explicit authored groups.");

    ensureExplicitZoneGroups(project);
    if (group.id.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Group creation rejected",
                                  "Group ids must be non-empty.");
    if (group.displayName.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Group creation rejected",
                                  "Group display names must be non-empty.");
    if (findGroupIndexById(project, group.id).has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group creation rejected",
                                  "Group id '" + group.id + "' already exists.");

    auto createdGroup = group;
    createdGroup.displayOrder = static_cast<int>(project.authoring.groups.size());
    if (!createdGroup.auditionAnchorZoneId.empty())
    {
        const auto anchorIndex = findZoneIndexById(project, createdGroup.auditionAnchorZoneId);
        if (!anchorIndex.has_value() || project.authoring.zones[*anchorIndex].groupId != createdGroup.id)
            createdGroup.auditionAnchorZoneId.clear();
    }

    project.authoring.groups.push_back(std::move(createdGroup));
    normalizeGroupDisplayOrder(project);
    project.authoring.selectedGroupId = group.id;

    return documentController.commitSnapshot(project,
                                             label,
                                             { "authoring.groups",
                                               "authoring.selectedGroupId" });
}

RuntimeProjectDocumentActionResult AuthoringSession::updateGroup(std::size_t groupIndex,
                                                                 const RuntimeProjectGroupDefinition& group,
                                                                 const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Group edit rejected",
                                  "This project schema does not support explicit authored groups.");
    if (groupIndex >= project.authoring.groups.size())
        return makeRejectedResult(getDocumentState(),
                                  "Group edit rejected",
                                  "Group index " + std::to_string(groupIndex) + " is out of range.");
    if (group.id.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Group edit rejected",
                                  "Group ids must be non-empty.");
    if (group.displayName.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Group edit rejected",
                                  "Group display names must be non-empty.");
    if (group.id != project.authoring.groups[groupIndex].id)
        return makeRejectedResult(getDocumentState(),
                                  "Group edit rejected",
                                  "Group ids are immutable once created.");

    ensureExplicitZoneGroups(project);
    const auto previousGroup = project.authoring.groups[groupIndex];
    project.authoring.groups[groupIndex] = group;
    project.authoring.groups[groupIndex].displayOrder = previousGroup.displayOrder;
    ensureExplicitZoneGroups(project);

    std::vector<std::string> changedPaths {
        "authoring.groups[" + std::to_string(groupIndex) + "]"
    };
    if (previousGroup.workspaceVisible && !project.authoring.groups[groupIndex].workspaceVisible
        && project.authoring.selectedGroupId == project.authoring.groups[groupIndex].id)
    {
        applyFallbackGroupSelection(project);
        changedPaths.push_back("authoring.selectedGroupId");
        changedPaths.push_back("authoring.selectedZoneId");
    }

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::moveGroup(std::size_t groupIndex,
                                                               int direction,
                                                               const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Group reorder rejected",
                                  "This project schema does not support explicit authored groups.");
    if (groupIndex >= project.authoring.groups.size())
        return makeRejectedResult(getDocumentState(),
                                  "Group reorder rejected",
                                  "Group index " + std::to_string(groupIndex) + " is out of range.");
    if (direction != -1 && direction != 1)
        return makeRejectedResult(getDocumentState(),
                                  "Group reorder rejected",
                                  "Group reordering only supports directions of -1 or 1.");

    const auto targetIndexSigned = static_cast<int>(groupIndex) + direction;
    if (targetIndexSigned < 0
        || targetIndexSigned >= static_cast<int>(project.authoring.groups.size()))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Group reorder rejected",
                                  "Group cannot move beyond the current authoring surface bounds.");
    }

    const auto targetIndex = static_cast<std::size_t>(targetIndexSigned);
    std::swap(project.authoring.groups[groupIndex], project.authoring.groups[targetIndex]);
    normalizeGroupDisplayOrder(project);

    return documentController.commitSnapshot(project,
                                             label,
                                             { "authoring.groups[" + std::to_string(groupIndex) + "]",
                                               "authoring.groups[" + std::to_string(targetIndex) + "]" });
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteGroup(const std::string& groupId,
                                                                 const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Group deletion rejected",
                                  "This project schema does not support explicit authored groups.");

    const auto groupIndex = findGroupIndexById(project, groupId);
    if (!groupIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group deletion rejected",
                                  "Group '" + groupId + "' does not exist in the current authoring project.");
    if (groupHasMembers(project, groupId))
        return makeRejectedResult(getDocumentState(),
                                  "Group deletion rejected",
                                  "Group '" + groupId + "' still owns zones and cannot be deleted.");

    project.authoring.groups.erase(project.authoring.groups.begin()
                                   + static_cast<std::ptrdiff_t>(*groupIndex));
    normalizeGroupDisplayOrder(project);

    std::vector<std::string> changedPaths { "authoring.groups" };
    if (project.authoring.selectedGroupId == groupId)
    {
        applyFallbackGroupSelection(project);
        changedPaths.push_back("authoring.selectedGroupId");
        changedPaths.push_back("authoring.selectedZoneId");
    }

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::reassignZoneToGroup(const std::string& zoneId,
                                                                         const std::string& groupId,
                                                                         const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "This project schema does not support explicit authored groups.");
    if (groupId.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Zones must always belong to a non-empty target group id.");

    const auto zoneIndex = findZoneIndexById(project, zoneId);
    if (!zoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Zone '" + zoneId + "' does not exist in the current authoring project.");

    const auto targetGroupIndex = findGroupIndexById(project, groupId);
    if (!targetGroupIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Target group '" + groupId + "' does not exist in the current authoring project.");

    auto& zone = project.authoring.zones[*zoneIndex];
    if (zone.groupId == groupId)
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Zone '" + zoneId + "' is already assigned to group '" + groupId + "'.");

    const auto previousGroupId = zone.groupId;
    zone.groupId = groupId;
    ensureExplicitZoneGroups(project);

    std::vector<std::string> changedPaths {
        "authoring.zones[" + std::to_string(*zoneIndex) + "]",
        "authoring.groups"
    };
    if (project.authoring.selectedZoneId == zoneId)
    {
        project.authoring.selectedGroupId = groupId;
        changedPaths.push_back("authoring.selectedGroupId");
    }
    else if (project.authoring.selectedGroupId == previousGroupId
             && !groupHasMembers(project, previousGroupId))
    {
        applyFallbackGroupSelection(project, groupId);
        changedPaths.push_back("authoring.selectedGroupId");
        changedPaths.push_back("authoring.selectedZoneId");
    }

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::createRoundRobinPoolForSelectedZone(const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin pool creation rejected",
                                  "No zone is currently selected for Round Robin editing.");

    auto project = getProject();
    createRoundRobinPoolForAnchor(project, *selectedZoneIndex);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::addCompatibleZonesToSelectedRoundRobinPool(const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin grouping rejected",
                                  "No zone is currently selected for Round Robin editing.");

    auto project = getProject();
    addCompatibleZonesToAnchorRoundRobinPool(project, *selectedZoneIndex);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::normalizeSelectedRoundRobinPool(const std::string& label)
{
    const auto selectedZone = getSelectedZone();
    if (!selectedZone.has_value() || !selectedZone->roundRobin.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin normalization rejected",
                                  "The selected zone is not part of a Round Robin pool.");

    auto project = getProject();
    auto memberIndices = collectRoundRobinPoolMemberIndices(project, selectedZone->roundRobin->poolId);
    if (memberIndices.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin normalization rejected",
                                  "The selected Round Robin pool could not be resolved.");

    normalizeRoundRobinPool(project, memberIndices, selectedZone->roundRobin->poolId);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::removeSelectedZoneFromRoundRobinPool(const std::string& label)
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin removal rejected",
                                  "No zone is currently selected for Round Robin editing.");

    auto project = getProject();
    auto& selectedZone = project.authoring.zones[*selectedZoneIndex];
    if (!selectedZone.roundRobin.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Round Robin removal rejected",
                                  "The selected zone is not part of a Round Robin pool.");

    auto remainingMembers = collectRoundRobinPoolMemberIndices(project, selectedZone.roundRobin->poolId);
    remainingMembers.erase(std::remove(remainingMembers.begin(), remainingMembers.end(), *selectedZoneIndex),
                           remainingMembers.end());
    const auto previousPoolId = selectedZone.roundRobin->poolId;
    clearRoundRobinAssignment(selectedZone);

    if (!remainingMembers.empty())
        normalizeRoundRobinPool(project, remainingMembers, previousPoolId);

    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::createRoundRobinPoolForSelectedGroup(const std::string& label)
{
    auto project = getProject();
    const auto anchorIndex = findSelectedGroupRoundRobinAnchorIndex(project);
    if (!anchorIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin pool creation rejected",
                                  "The selected group does not have an audition anchor zone available for Round Robin editing.");

    isolateMalformedRoundRobinPoolMembers(project, *anchorIndex);
    createRoundRobinPoolForAnchor(project, *anchorIndex);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::addCompatibleZonesToSelectedGroupRoundRobinPool(const std::string& label)
{
    auto project = getProject();
    const auto anchorIndex = findSelectedGroupRoundRobinAnchorIndex(project);
    if (!anchorIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin grouping rejected",
                                  "The selected group does not have an audition anchor zone available for Round Robin editing.");

    isolateMalformedRoundRobinPoolMembers(project, *anchorIndex);
    addCompatibleZonesToAnchorRoundRobinPool(project, *anchorIndex);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::normalizeSelectedGroupRoundRobinPool(const std::string& label)
{
    auto project = getProject();
    const auto anchorIndex = findSelectedGroupRoundRobinAnchorIndex(project);
    if (!anchorIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin normalization rejected",
                                  "The selected group does not have an audition anchor zone available for Round Robin editing.");

    isolateMalformedRoundRobinPoolMembers(project, *anchorIndex);
    const auto& anchorZone = project.authoring.zones[*anchorIndex];
    if (!anchorZone.roundRobin.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin normalization rejected",
                                  "The selected group's audition anchor is not part of a Round Robin pool.");

    auto memberIndices = collectRoundRobinPoolMemberIndices(project, anchorZone.roundRobin->poolId);
    if (memberIndices.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin normalization rejected",
                                  "The selected group's Round Robin pool could not be resolved.");

    normalizeRoundRobinPool(project, memberIndices, anchorZone.roundRobin->poolId);
    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::removeSelectedGroupAnchorFromRoundRobinPool(const std::string& label)
{
    auto project = getProject();
    const auto anchorIndex = findSelectedGroupRoundRobinAnchorIndex(project);
    if (!anchorIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin removal rejected",
                                  "The selected group does not have an audition anchor zone available for Round Robin editing.");

    isolateMalformedRoundRobinPoolMembers(project, *anchorIndex);
    auto& anchorZone = project.authoring.zones[*anchorIndex];
    if (!anchorZone.roundRobin.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin removal rejected",
                                  "The selected group's audition anchor is not part of a Round Robin pool.");

    auto remainingMembers = collectRoundRobinPoolMemberIndices(project, anchorZone.roundRobin->poolId);
    remainingMembers.erase(std::remove(remainingMembers.begin(), remainingMembers.end(), *anchorIndex),
                           remainingMembers.end());
    const auto previousPoolId = anchorZone.roundRobin->poolId;
    clearRoundRobinAssignment(anchorZone);

    if (!remainingMembers.empty())
        normalizeRoundRobinPool(project, remainingMembers, previousPoolId);

    alignSelectedGroupToSelectedZone(project);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteSelectedSample()
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Sample deletion rejected",
                                  "No sample is currently selected for deletion.");

    auto project = getProject();
    const auto deletedZone = project.authoring.zones[*selectedZoneIndex];
    const auto sampleSourceId = deletedZone.sampleSourceId;
    project.authoring.zones.erase(project.authoring.zones.begin()
                                  + static_cast<std::ptrdiff_t>(*selectedZoneIndex));

    for (auto& routingBus : project.authoring.routingBuses)
    {
        if (routingBus.inputSourceId != deletedZone.id)
            continue;

        const auto replacementZone = std::find_if(project.authoring.zones.begin(),
                                                  project.authoring.zones.end(),
                                                  [&](const RuntimeProjectZoneDefinition& zone)
                                                  {
                                                      return zone.groupId == deletedZone.groupId;
                                                  });
        routingBus.inputSourceId = replacementZone != project.authoring.zones.end()
            ? replacementZone->id
            : std::string { "master" };
    }

    const auto sourceStillUsed = std::any_of(project.authoring.zones.begin(),
                                             project.authoring.zones.end(),
                                             [&](const RuntimeProjectZoneDefinition& zone)
                                             {
                                                 return zone.sampleSourceId == sampleSourceId;
                                             });
    if (!sourceStillUsed)
    {
        project.sampleSources.erase(
            std::remove_if(project.sampleSources.begin(),
                           project.sampleSources.end(),
                           [&](const RuntimeProjectSampleSource& source)
                           {
                               return source.id == sampleSourceId;
                           }),
            project.sampleSources.end());
    }

    if (project.authoring.zones.empty())
    {
        project.authoring.selectedZoneId.clear();
    }
    else
    {
        const auto nextIndex = std::min(*selectedZoneIndex, project.authoring.zones.size() - 1);
        project.authoring.selectedZoneId = project.authoring.zones[nextIndex].id;
    }

    alignSelectedGroupToSelectedZone(project);

    std::vector<std::string> changedPaths {
        "authoring.zones",
        "authoring.selectedZoneId"
    };
    if (usesExplicitZoneGroupsSchema(project))
    {
        changedPaths.push_back("authoring.selectedGroupId");
        changedPaths.push_back("authoring.groups");
    }
    changedPaths.push_back("authoring.routingBuses");
    if (!sourceStillUsed)
        changedPaths.push_back("sampleSources");

    return documentController.commitSnapshot(project,
                                             "Delete selected sample",
                                             std::move(changedPaths));
}

RuntimeProjectDocumentActionResult AuthoringSession::appendImportedContent(
    std::vector<RuntimeProjectSampleSource> sampleSources,
    std::vector<RuntimeProjectZoneDefinition> zones,
    const std::string& label)
{
    return appendImportedContent(std::move(sampleSources), std::move(zones), {}, {}, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::appendImportedContent(
    std::vector<RuntimeProjectSampleSource> sampleSources,
    std::vector<RuntimeProjectZoneDefinition> zones,
    std::vector<std::string> projectNotes,
    std::vector<std::string> authoringNotes,
    const std::string& label)
{
    if (zones.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Authoring import rejected",
                                  "Imported content must include at least one zone.");

    auto project = getProject();
    const auto originalSampleSourceCount = project.sampleSources.size();
    const auto originalZoneCount = project.authoring.zones.size();

    project.sampleSources.insert(project.sampleSources.end(),
                                 std::make_move_iterator(sampleSources.begin()),
                                 std::make_move_iterator(sampleSources.end()));
    project.authoring.zones.insert(project.authoring.zones.end(),
                                   std::make_move_iterator(zones.begin()),
                                   std::make_move_iterator(zones.end()));
    project.notes.insert(project.notes.end(),
                         std::make_move_iterator(projectNotes.begin()),
                         std::make_move_iterator(projectNotes.end()));
    project.authoring.notes.insert(project.authoring.notes.end(),
                                   std::make_move_iterator(authoringNotes.begin()),
                                   std::make_move_iterator(authoringNotes.end()));
    project.authoring.selectedZoneId = project.authoring.zones[originalZoneCount].id;

    const auto requiresRoundRobinSchema = std::any_of(project.authoring.zones.begin(),
                                                      project.authoring.zones.end(),
                                                      [](const RuntimeProjectZoneDefinition& zone)
                                                      {
                                                          return zone.roundRobin.has_value();
                                                      });
    if (requiresRoundRobinSchema
        && (project.schemaVersion != 3 || project.authoring.schemaVersion != 2))
    {
        const auto migration = migrateRuntimeProjectToPhase3RoundRobinSchema(project);
        if (!migration.valid)
            return makeRejectedResult(getDocumentState(),
                                      "Authoring import rejected",
                                      migration.issues.empty()
                                          ? "Round Robin import could not migrate the project schema."
                                          : migration.issues.front());

        project = migration.project;
    }

    alignSelectedGroupToSelectedZone(project);

    std::vector<std::string> changedPaths {
        "sampleSources[" + std::to_string(originalSampleSourceCount) + "]",
        "authoring.zones[" + std::to_string(originalZoneCount) + "]",
        "authoring.selectedZoneId"
    };
    if (usesExplicitZoneGroupsSchema(project))
    {
        changedPaths.push_back("authoring.selectedGroupId");
        changedPaths.push_back("authoring.groups");
    }
    if (!projectNotes.empty())
        changedPaths.push_back("notes");
    if (!authoringNotes.empty())
        changedPaths.push_back("authoring.notes");

    return documentController.commitSnapshot(project, label, changedPaths);
}

RuntimeProjectDocumentActionResult AuthoringSession::updateMacro(std::size_t macroIndex,
                                                                 const RuntimeProjectMacroDefinition& macro,
                                                                 const std::string& label)
{
    if (macroIndex >= getProject().authoring.macros.size())
        return makeRejectedResult(getDocumentState(),
                                  "Macro edit rejected",
                                  "Macro index " + std::to_string(macroIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.macros[macroIndex] = macro;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.macros[" + std::to_string(macroIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::moveMacro(std::size_t macroIndex,
                                                               int direction,
                                                               const std::string& label)
{
    if (macroIndex >= getProject().authoring.macros.size())
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro index " + std::to_string(macroIndex) + " is out of range.");

    if (direction != -1 && direction != 1)
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro reordering only supports directions of -1 or 1.");

    const auto targetIndexSigned = static_cast<int>(macroIndex) + direction;
    if (targetIndexSigned < 0
        || targetIndexSigned >= static_cast<int>(getProject().authoring.macros.size()))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Macro reorder rejected",
                                  "Macro cannot move beyond the current authoring surface bounds.");
    }

    auto project = getProject();
    const auto targetIndex = static_cast<std::size_t>(targetIndexSigned);
    std::swap(project.authoring.macros[macroIndex], project.authoring.macros[targetIndex]);

    return documentController.commitSnapshot(project,
                                             label,
                                             {
                                                 "authoring.macros[" + std::to_string(macroIndex) + "]",
                                                 "authoring.macros[" + std::to_string(targetIndex) + "]"
                                             });
}

RuntimeProjectDocumentActionResult AuthoringSession::updateFxSlot(std::size_t fxSlotIndex,
                                                                  const RuntimeProjectFxSlotDefinition& fxSlot,
                                                                  const std::string& label)
{
    if (fxSlotIndex >= getProject().authoring.fxSlots.size())
        return makeRejectedResult(getDocumentState(),
                                  "FX slot edit rejected",
                                  "FX slot index " + std::to_string(fxSlotIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.fxSlots[fxSlotIndex] = fxSlot;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.fxSlots[" + std::to_string(fxSlotIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updateRoutingBus(std::size_t routingBusIndex,
                                                                      const RuntimeProjectRoutingBusDefinition& routingBus,
                                                                      const std::string& label)
{
    if (routingBusIndex >= getProject().authoring.routingBuses.size())
        return makeRejectedResult(getDocumentState(),
                                  "Routing edit rejected",
                                  "Routing bus index " + std::to_string(routingBusIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.routingBuses[routingBusIndex] = routingBus;
    return documentController.commitSnapshot(project,
                                             label,
                                             {"authoring.routingBuses[" + std::to_string(routingBusIndex) + "]"});
}

RuntimeProjectDocumentActionResult AuthoringSession::updatePerformanceBank(
    std::size_t performanceBankIndex,
    const RuntimeProjectPerformanceBankDefinition& performanceBank,
    const std::string& label)
{
    if (performanceBankIndex >= getProject().authoring.performanceBanks.size())
        return makeRejectedResult(getDocumentState(),
                                  "Performance-bank edit rejected",
                                  "Performance bank index " + std::to_string(performanceBankIndex) + " is out of range.");

    auto project = getProject();
    project.authoring.performanceBanks[performanceBankIndex] = performanceBank;
    project.authoring.selectedPerformanceBankId = performanceBank.id;
    return documentController.commitSnapshot(project,
                                             label,
                                             {
                                                 "authoring.performanceBanks[" + std::to_string(performanceBankIndex) + "]",
                                                 "authoring.selectedPerformanceBankId"
                                             });
}

RuntimeProjectDocumentActionResult AuthoringSession::undo()
{
    return documentController.undo();
}

RuntimeProjectDocumentActionResult AuthoringSession::redo()
{
    return documentController.redo();
}

void AuthoringSession::markSaved()
{
    documentController.markSaved();
}
} // namespace drs::engine
