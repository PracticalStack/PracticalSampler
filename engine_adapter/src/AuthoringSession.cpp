#include "drs/engine/AuthoringSession.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <unordered_set>
#include <utility>

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

std::optional<std::size_t> findMacroIndexById(const RuntimeProjectModel& project,
                                              const std::string& macroId)
{
    const auto iterator = std::find_if(project.authoring.macros.begin(),
                                       project.authoring.macros.end(),
                                       [&](const RuntimeProjectMacroDefinition& macro)
                                       {
                                           return macro.id == macroId;
                                       });
    if (iterator == project.authoring.macros.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.macros.begin(), iterator));
}

std::optional<std::size_t> findFxSlotIndexById(const RuntimeProjectModel& project,
                                               const std::string& fxSlotId)
{
    const auto iterator = std::find_if(project.authoring.fxSlots.begin(),
                                       project.authoring.fxSlots.end(),
                                       [&](const RuntimeProjectFxSlotDefinition& slot)
                                       {
                                           return slot.id == fxSlotId;
                                       });
    if (iterator == project.authoring.fxSlots.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.fxSlots.begin(), iterator));
}

std::optional<std::size_t> findRoutingBusIndexById(const RuntimeProjectModel& project,
                                                   const std::string& busId)
{
    const auto iterator = std::find_if(project.authoring.routingBuses.begin(),
                                       project.authoring.routingBuses.end(),
                                       [&](const RuntimeProjectRoutingBusDefinition& bus)
                                       {
                                           return bus.id == busId;
                                       });
    if (iterator == project.authoring.routingBuses.end())
        return std::nullopt;

    return static_cast<std::size_t>(std::distance(project.authoring.routingBuses.begin(), iterator));
}

std::optional<std::size_t> findUniqueFxSlotOwnerBusIndex(const RuntimeProjectModel& project,
                                                         const std::string& fxSlotId)
{
    std::optional<std::size_t> ownerIndex;
    for (std::size_t index = 0; index < project.authoring.routingBuses.size(); ++index)
    {
        const auto& slotIds = project.authoring.routingBuses[index].fxSlotIds;
        const auto occurrences = static_cast<std::size_t>(std::count(slotIds.begin(), slotIds.end(), fxSlotId));
        if (occurrences == 0)
            continue;
        if (occurrences != 1 || ownerIndex.has_value())
            return std::nullopt;
        ownerIndex = index;
    }
    return ownerIndex;
}

const CuratedDspParameterDescriptor* findCatalogParameter(const RuntimeProjectFxSlotDefinition& slot,
                                                          const std::string& parameterId)
{
    const auto* effect = findCuratedDspEffect(slot.effectType, slot.effectVersion);
    if (effect == nullptr)
        return nullptr;
    const auto iterator = std::find_if(effect->parameters.begin(), effect->parameters.end(),
                                       [&](const CuratedDspParameterDescriptor& parameter)
                                       {
                                           return parameter.id == parameterId;
                                       });
    return iterator == effect->parameters.end() ? nullptr : &*iterator;
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

std::string slugifyIdentifier(const std::string& text, std::string_view fallback)
{
    std::string slug;
    slug.reserve(text.size());

    bool previousWasSeparator = false;
    for (const auto character : text)
    {
        const auto ascii = static_cast<unsigned char>(character);
        if (std::isalnum(ascii) != 0)
        {
            slug.push_back(static_cast<char>(std::tolower(ascii)));
            previousWasSeparator = false;
            continue;
        }

        if (!slug.empty() && !previousWasSeparator)
        {
            slug.push_back('-');
            previousWasSeparator = true;
        }
    }

    while (!slug.empty() && slug.front() == '-')
        slug.erase(slug.begin());
    while (!slug.empty() && slug.back() == '-')
        slug.pop_back();

    return slug.empty() ? std::string(fallback) : slug;
}

std::string buildDefaultMacroName(const RuntimeProjectModel& project)
{
    return "Macro " + std::to_string(project.authoring.macros.size() + 1);
}

std::string makeUniqueMacroId(const RuntimeProjectModel& project,
                              const std::string& preferredBase,
                              const std::string& ignoredMacroId = {})
{
    std::unordered_set<std::string> usedIds;
    usedIds.reserve(project.authoring.macros.size());
    for (const auto& macro : project.authoring.macros)
    {
        if (!ignoredMacroId.empty() && macro.id == ignoredMacroId)
            continue;
        usedIds.insert(macro.id);
    }

    const auto baseId = slugifyIdentifier(preferredBase, "macro");
    if (!usedIds.count(baseId))
        return baseId;

    auto suffix = 2;
    for (;; ++suffix)
    {
        const auto candidate = baseId + "-" + std::to_string(suffix);
        if (!usedIds.count(candidate))
            return candidate;
    }
}

std::string buildDuplicateMacroName(const RuntimeProjectModel& project,
                                    const RuntimeProjectMacroDefinition& source)
{
    const auto baseName = source.name.empty() ? "Macro" : source.name;
    const auto preferredName = baseName + " Copy";

    std::unordered_set<std::string> usedNames;
    usedNames.reserve(project.authoring.macros.size());
    for (const auto& macro : project.authoring.macros)
        usedNames.insert(macro.name);

    if (!usedNames.count(preferredName))
        return preferredName;

    auto suffix = 2;
    for (;; ++suffix)
    {
        const auto candidate = preferredName + " " + std::to_string(suffix);
        if (!usedNames.count(candidate))
            return candidate;
    }
}

std::optional<std::string> validateMacroDefinition(const RuntimeProjectModel& project,
                                                   const RuntimeProjectMacroDefinition& macro,
                                                   std::optional<std::size_t> editedMacroIndex = std::nullopt)
{
    if (macro.id.empty())
        return "Macro ids must be non-empty.";
    if (macro.name.empty())
        return "Macro names must be non-empty.";

    const auto finite = std::isfinite(macro.minValue)
        && std::isfinite(macro.maxValue)
        && std::isfinite(macro.defaultValue);
    if (!finite)
        return "Macro ranges and default values must be finite.";
    if (macro.minValue > macro.maxValue)
        return "Macro minValue must not exceed maxValue.";
    if (macro.defaultValue < macro.minValue || macro.defaultValue > macro.maxValue)
        return "Macro defaultValue must stay within the declared min/max range.";

    for (std::size_t index = 0; index < project.authoring.macros.size(); ++index)
    {
        if (editedMacroIndex.has_value() && *editedMacroIndex == index)
            continue;
        if (project.authoring.macros[index].id == macro.id)
            return "Macro id '" + macro.id + "' already exists.";
    }

    for (const auto& target : macro.targets)
    {
        if (target.parameterId.empty())
            return "Macro '" + macro.id + "' contains a target without parameterId.";
        if (target.parameterPath.empty())
            return "Macro '" + macro.id + "' contains a target without parameterPath.";
        if (target.role.empty())
            return "Macro '" + macro.id + "' contains a target without role.";

        const auto hasDspIdentity = !target.dspSlotId.empty() || !target.dspParameterId.empty();
        if (!hasDspIdentity)
            continue;

        const auto finiteTargetRange = std::isfinite(target.sourceMinimum)
            && std::isfinite(target.sourceMaximum)
            && std::isfinite(target.destinationMinimum)
            && std::isfinite(target.destinationMaximum);
        const auto validCurve = target.curve == "linear" || target.curve == "logarithmic";
        const auto validLogRange = target.curve != "logarithmic"
            || (target.destinationMinimum > 0.0 && target.destinationMaximum > 0.0);
        const auto completeControlLaw = (target.controlLaw.id.empty() && target.controlLaw.version == 0)
            || (!target.controlLaw.id.empty() && target.controlLaw.version != 0);
        CompiledControlLaw compiledLaw;
        const auto knownControlLawIsCompatible = target.controlLaw.id.empty()
            || target.controlLaw.version != 1
            || compileControlLaw(target.controlLaw.id, target.destinationMinimum,
                                 target.destinationMaximum, compiledLaw);
        if (target.dspSlotId.empty()
            || target.dspParameterId.empty()
            || !finiteTargetRange
            || target.sourceMinimum >= target.sourceMaximum
            || target.destinationMinimum > target.destinationMaximum
            || !validCurve
            || !validLogRange
            || !completeControlLaw
            || !knownControlLawIsCompatible)
        {
            return "Macro '" + macro.id + "' contains an invalid structured DSP target.";
        }
    }

    return std::nullopt;
}

bool resolveNewDspTargetControlLaw(const RuntimeProjectModel& project,
                                   RuntimeProjectMacroTargetDefinition& target)
{
    if (!target.controlLaw.id.empty() || target.dspSlotId.empty() || target.dspParameterId.empty())
        return !target.controlLaw.id.empty();
    const auto slotIndex = findFxSlotIndexById(project, target.dspSlotId);
    if (!slotIndex.has_value()) return false;
    const auto* parameter = findCatalogParameter(project.authoring.fxSlots[*slotIndex], target.dspParameterId);
    if (parameter == nullptr) return false;
    const auto resolution = resolveCuratedDspControlLaw({ parameter, target.role, {} });
    if (!resolution.resolved) return false;
    target.controlLaw.id = std::string(resolution.controlLawId);
    target.controlLaw.version = 1;
    target.destinationMinimum = resolution.minimum;
    target.destinationMaximum = resolution.maximum;
    return true;
}

void resolveNewDspTargetControlLaws(const RuntimeProjectModel& project,
                                    RuntimeProjectMacroDefinition& macro)
{
    for (auto& target : macro.targets)
        resolveNewDspTargetControlLaw(project, target);
}

bool isLegacyMixerGainTarget(const RuntimeProjectModel& project,
                             const RuntimeProjectMacroTargetDefinition& target)
{
    if (!target.controlLaw.id.empty() || target.role != "mix"
        || target.dspSlotId.empty() || target.dspParameterId != "gainDb")
        return false;
    const auto slotIndex = findFxSlotIndexById(project, target.dspSlotId);
    return slotIndex.has_value()
        && project.authoring.fxSlots[*slotIndex].effectType == "drs.gain"
        && project.authoring.fxSlots[*slotIndex].effectVersion == 1;
}

RuntimeProjectMacroDefinition normalizeCreatedMacro(const RuntimeProjectModel& project,
                                                    RuntimeProjectMacroDefinition macro)
{
    if (macro.name.empty())
        macro.name = buildDefaultMacroName(project);

    if (macro.id.empty())
        macro.id = makeUniqueMacroId(project, macro.name);

    resolveNewDspTargetControlLaws(project, macro);

    return macro;
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
                               int slotIndex,
                               RoundRobinMode mode = RoundRobinMode::sequential)
{
    zone.roundRobin = RoundRobinDescriptor {
        poolId,
        slotCount,
        slotIndex,
        mode
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

std::string allocateRoundRobinPoolId(const RuntimeProjectModel& project);

void normalizeRoundRobinPool(RuntimeProjectModel& project,
                             std::vector<std::size_t> memberIndices,
                             const std::string& poolId,
                             std::optional<RoundRobinMode> modeOverride = std::nullopt)
{
    auto mode = modeOverride.value_or(RoundRobinMode::sequential);
    if (!modeOverride.has_value())
    {
        const auto existingMode = std::find_if(memberIndices.begin(),
                                               memberIndices.end(),
                                               [&](const std::size_t memberIndex)
                                               {
                                                   return project.authoring.zones[memberIndex].roundRobin.has_value();
                                               });
        if (existingMode != memberIndices.end())
            mode = project.authoring.zones[*existingMode].roundRobin->mode;
    }

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
                                  static_cast<int>(ordinal) + 1,
                                  mode);
}

std::vector<std::size_t> collectGroupMemberIndices(const RuntimeProjectModel& project,
                                                   const std::string& groupId)
{
    std::vector<std::size_t> indices;
    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
        if (project.authoring.zones[index].groupId == groupId)
            indices.push_back(index);
    return indices;
}

std::vector<std::vector<std::size_t>> partitionGroupRoundRobinCandidates(
    const RuntimeProjectModel& project,
    const std::string& groupId)
{
    std::vector<std::vector<std::size_t>> partitions;
    for (const auto memberIndex : collectGroupMemberIndices(project, groupId))
    {
        const auto partition = std::find_if(partitions.begin(),
                                            partitions.end(),
                                            [&](const auto& candidate)
                                            {
                                                return !candidate.empty()
                                                    && isRoundRobinGroupingCompatible(
                                                        project.authoring.zones[candidate.front()],
                                                        project.authoring.zones[memberIndex]);
                                            });
        if (partition != partitions.end())
            partition->push_back(memberIndex);
        else
            partitions.push_back({ memberIndex });
    }
    return partitions;
}

AuthoringGroupRoundRobinStatus assessGroupRoundRobin(const RuntimeProjectModel& project,
                                                     const std::string& groupId)
{
    AuthoringGroupRoundRobinStatus status;
    const auto partitions = partitionGroupRoundRobinCandidates(project, groupId);
    if (partitions.empty())
    {
        status.state = "Round Robin requires at least two compatible zones.";
        return status;
    }

    for (const auto& partition : partitions)
    {
        if (partition.size() >= 2)
            continue;
        for (const auto zoneIndex : partition)
            status.incompatibleZoneIds.push_back(project.authoring.zones[zoneIndex].id);
    }
    status.eligible = status.incompatibleZoneIds.empty();
    if (!status.eligible)
    {
        status.state = "Every mapping in the group must have at least two exact-match zones.";
        return status;
    }

    std::optional<RoundRobinMode> groupMode;
    for (const auto& partition : partitions)
    {
        std::string poolId;
        std::vector<bool> occupiedSlots(partition.size(), false);
        for (const auto zoneIndex : partition)
        {
            const auto& zone = project.authoring.zones[zoneIndex];
            if (!zone.roundRobin.has_value())
            {
                status.state = "Round Robin is off.";
                return status;
            }

            const auto& roundRobin = *zone.roundRobin;
            if (poolId.empty())
                poolId = roundRobin.poolId;
            if (!groupMode.has_value())
                groupMode = roundRobin.mode;
            if (roundRobin.poolId.empty()
                || roundRobin.poolId != poolId
                || roundRobin.slotCount != static_cast<int>(partition.size())
                || roundRobin.slotIndex < 1
                || roundRobin.slotIndex > static_cast<int>(partition.size())
                || roundRobin.mode != *groupMode
                || occupiedSlots[static_cast<std::size_t>(roundRobin.slotIndex - 1)])
            {
                status.state = "Round Robin metadata is incomplete or inconsistent.";
                return status;
            }
            occupiedSlots[static_cast<std::size_t>(roundRobin.slotIndex - 1)] = true;
        }

        for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
        {
            if (std::find(partition.begin(), partition.end(), index) != partition.end())
                continue;
            const auto& zone = project.authoring.zones[index];
            if (zone.roundRobin.has_value() && zone.roundRobin->poolId == poolId)
            {
                status.state = "A Round Robin pool crosses group or mapping boundaries.";
                return status;
            }
        }
    }

    status.enabled = true;
    status.mode = groupMode.value_or(RoundRobinMode::sequential);
    status.state = status.mode == RoundRobinMode::random
        ? "Round Robin is on in random mode."
        : "Round Robin is on in cycle mode.";
    return status;
}

void clearGroupRoundRobin(RuntimeProjectModel& project, const std::string& groupId)
{
    for (auto& zone : project.authoring.zones)
        if (zone.groupId == groupId)
            clearRoundRobinAssignment(zone);
}

void enableGroupRoundRobin(RuntimeProjectModel& project,
                           const std::string& groupId,
                           RoundRobinMode mode)
{
    clearGroupRoundRobin(project, groupId);
    for (auto partition : partitionGroupRoundRobinCandidates(project, groupId))
        normalizeRoundRobinPool(project,
                                std::move(partition),
                                allocateRoundRobinPoolId(project),
                                mode);
}

void reconcilePreviouslyEnabledGroup(RuntimeProjectModel& project,
                                     const std::string& groupId,
                                     RoundRobinMode mode)
{
    const auto status = assessGroupRoundRobin(project, groupId);
    if (status.eligible)
        enableGroupRoundRobin(project, groupId, mode);
    else
        clearGroupRoundRobin(project, groupId);
}

RuntimeProjectModel prepareAuthoringProject(RuntimeProjectModel project)
{
    if (!usesExplicitZoneGroupsSchema(project))
        return project;

    ensureExplicitZoneGroups(project);
    for (const auto& group : project.authoring.groups)
    {
        const auto hasRoundRobinMetadata = std::any_of(
            project.authoring.zones.begin(),
            project.authoring.zones.end(),
            [&](const RuntimeProjectZoneDefinition& zone)
            {
                return zone.groupId == group.id && zone.roundRobin.has_value();
            });
        if (hasRoundRobinMetadata && !assessGroupRoundRobin(project, group.id).enabled)
            clearGroupRoundRobin(project, group.id);
    }
    return project;
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

std::optional<std::size_t> findSelectedGroupPreviewAnchorIndex(const RuntimeProjectModel& project)
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
    : documentController(prepareAuthoringProject(std::move(project)))
{
    recoverDspSelection();
    recoverMacroSelection();
}

const RuntimeProjectModel& AuthoringSession::getProject() const
{
    return documentController.getProject();
}

const RuntimeProjectDocumentState& AuthoringSession::getDocumentState() const
{
    return documentController.getDocumentState();
}

RuntimeProjectDocumentCheckpoint AuthoringSession::exportCheckpoint() const
{
    return documentController.exportCheckpoint();
}

RuntimeProjectDocumentActionResult AuthoringSession::restoreCheckpoint(
    RuntimeProjectDocumentCheckpoint checkpoint,
    RuntimeProjectDocumentCheckpointConstraints constraints)
{
    auto result = documentController.restoreCheckpoint(std::move(checkpoint), std::move(constraints));
    if (result.applied)
    {
        recoverDspSelection();
        recoverMacroSelection();
    }
    return result;
}

void AuthoringSession::replaceProject(RuntimeProjectModel project)
{
    documentController = RuntimeProjectDocumentController(prepareAuthoringProject(std::move(project)));
    recoverDspSelection();
    recoverMacroSelection();
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

std::optional<RuntimeProjectMacroDefinition> AuthoringSession::getSelectedMacro() const
{
    const auto selectedMacroIndex = getSelectedMacroIndex();
    if (!selectedMacroIndex.has_value())
        return std::nullopt;

    return getProject().authoring.macros[*selectedMacroIndex];
}

std::optional<std::size_t> AuthoringSession::getSelectedMacroIndex() const
{
    if (selectedMacroId.empty())
        return std::nullopt;

    return findMacroIndexById(getProject(), selectedMacroId);
}

AuthoringGroupRoundRobinStatus AuthoringSession::getSelectedGroupRoundRobinStatus() const
{
    const auto selectedGroup = getSelectedGroup();
    if (!selectedGroup.has_value())
    {
        AuthoringGroupRoundRobinStatus status;
        status.state = "Select a group to configure Round Robin.";
        return status;
    }

    return assessGroupRoundRobin(getProject(), selectedGroup->id);
}

AuthoringDspSelection AuthoringSession::getDspSelection() const
{
    return dspSelection;
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

    const auto anchorIndex = findSelectedGroupPreviewAnchorIndex(getProject());
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

RuntimeProjectDocumentActionResult AuthoringSession::selectDspSlot(const std::string& fxSlotId)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    const auto ownerIndex = findUniqueFxSlotOwnerBusIndex(getProject(), fxSlotId);
    if (!slotIndex.has_value() || !ownerIndex.has_value())
        return makeRejectedResult(getDocumentState(), "DSP selection rejected", "FX slot must exist with exactly one owner.");
    dspSelection.fxSlotId = fxSlotId;
    dspSelection.routingBusId = getProject().authoring.routingBuses[*ownerIndex].id;
    auto result = RuntimeProjectDocumentActionResult {};
    result.applied = true;
    result.state = "DSP slot selected";
    result.documentState = getDocumentState();
    return result;
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
    const auto previousGroupId = project.authoring.zones[*selectedZoneIndex].groupId;
    const auto previousGroupStatus = assessGroupRoundRobin(project, previousGroupId);
    const auto destinationGroupStatus = zone.groupId == previousGroupId
        ? previousGroupStatus
        : assessGroupRoundRobin(project, zone.groupId);
    project.authoring.zones[*selectedZoneIndex] = zone;
    if (previousGroupStatus.enabled)
        reconcilePreviouslyEnabledGroup(project, previousGroupId, previousGroupStatus.mode);
    if (zone.groupId != previousGroupId && destinationGroupStatus.enabled)
        reconcilePreviouslyEnabledGroup(project, zone.groupId, destinationGroupStatus.mode);
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

RuntimeProjectDocumentActionResult AuthoringSession::selectMacro(const std::string& macroId)
{
    if (!findMacroIndexById(getProject(), macroId).has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Macro selection rejected",
                                  "Macro '" + macroId + "' does not exist in the current authoring project.");

    selectedMacroId = macroId;
    RuntimeProjectDocumentActionResult result;
    result.applied = true;
    result.state = "Macro selected";
    result.documentState = getDocumentState();
    return result;
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
    return reassignZonesToGroup({ zoneId }, groupId, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::reassignZonesToGroup(const std::vector<std::string>& zoneIds,
                                                                          const std::string& groupId,
                                                                          const std::string& label)
{
    auto project = getProject();
    if (!usesExplicitZoneGroupsSchema(project))
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "This project schema does not support explicit authored groups.");
    if (zoneIds.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Select at least one zone before assigning it to a group.");
    if (groupId.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Zones must always belong to a non-empty target group id.");

    const auto targetGroupIndex = findGroupIndexById(project, groupId);
    if (!targetGroupIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  "Target group '" + groupId + "' does not exist in the current authoring project.");

    std::vector<std::size_t> zoneIndices;
    zoneIndices.reserve(zoneIds.size());
    for (const auto& zoneId : zoneIds)
    {
        const auto zoneIndex = findZoneIndexById(project, zoneId);
        if (!zoneIndex.has_value())
            return makeRejectedResult(getDocumentState(),
                                      "Zone reassignment rejected",
                                      "Zone '" + zoneId + "' does not exist in the current authoring project.");

        if (std::find(zoneIndices.begin(), zoneIndices.end(), *zoneIndex) == zoneIndices.end())
            zoneIndices.push_back(*zoneIndex);
    }

    std::vector<std::string> previousGroupIds;
    std::vector<std::pair<std::string, RoundRobinMode>> enabledGroups;
    const auto rememberEnabledGroup = [&](const std::string& candidateGroupId)
    {
        if (std::find_if(enabledGroups.begin(),
                         enabledGroups.end(),
                         [&](const auto& entry) { return entry.first == candidateGroupId; })
            != enabledGroups.end())
        {
            return;
        }
        const auto status = assessGroupRoundRobin(project, candidateGroupId);
        if (status.enabled)
            enabledGroups.push_back({ candidateGroupId, status.mode });
    };
    rememberEnabledGroup(groupId);
    previousGroupIds.reserve(zoneIndices.size());
    bool anyChanged = false;
    bool selectedZoneMoved = false;
    for (const auto zoneIndex : zoneIndices)
    {
        auto& zone = project.authoring.zones[zoneIndex];
        previousGroupIds.push_back(zone.groupId);
        rememberEnabledGroup(zone.groupId);
        if (zone.groupId == groupId)
            continue;

        zone.groupId = groupId;
        anyChanged = true;
        selectedZoneMoved = selectedZoneMoved || project.authoring.selectedZoneId == zone.id;
    }

    if (!anyChanged)
    {
        return makeRejectedResult(getDocumentState(),
                                  "Zone reassignment rejected",
                                  zoneIndices.size() == 1
                                      ? "Zone '" + zoneIds.front() + "' is already assigned to group '" + groupId + "'."
                                      : "Every selected zone is already assigned to group '" + groupId + "'.");
    }

    ensureExplicitZoneGroups(project);
    for (const auto& [enabledGroupId, mode] : enabledGroups)
        reconcilePreviouslyEnabledGroup(project, enabledGroupId, mode);

    std::vector<std::string> changedPaths {
        "authoring.zones",
        "authoring.groups"
    };
    if (selectedZoneMoved)
    {
        project.authoring.selectedGroupId = groupId;
        changedPaths.push_back("authoring.selectedGroupId");
    }
    else if (project.authoring.selectedGroupId != groupId)
    {
        for (const auto& previousGroupId : previousGroupIds)
        {
            if (project.authoring.selectedGroupId == previousGroupId
                && !groupHasMembers(project, previousGroupId))
            {
                applyFallbackGroupSelection(project, groupId);
                changedPaths.push_back("authoring.selectedGroupId");
                changedPaths.push_back("authoring.selectedZoneId");
                break;
            }
        }
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
    const auto status = getSelectedGroupRoundRobinStatus();
    return setSelectedGroupRoundRobinEnabled(
        true,
        status.enabled ? status.mode : RoundRobinMode::sequential,
        label);
}

RuntimeProjectDocumentActionResult AuthoringSession::addCompatibleZonesToSelectedGroupRoundRobinPool(const std::string& label)
{
    const auto status = getSelectedGroupRoundRobinStatus();
    return setSelectedGroupRoundRobinEnabled(
        true,
        status.enabled ? status.mode : RoundRobinMode::sequential,
        label);
}

RuntimeProjectDocumentActionResult AuthoringSession::normalizeSelectedGroupRoundRobinPool(const std::string& label)
{
    const auto status = getSelectedGroupRoundRobinStatus();
    if (!status.enabled)
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin normalization rejected",
                                  "Round Robin is not enabled for the selected group.");
    return setSelectedGroupRoundRobinEnabled(true, status.mode, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::removeSelectedGroupAnchorFromRoundRobinPool(const std::string& label)
{
    return setSelectedGroupRoundRobinEnabled(false, RoundRobinMode::sequential, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::setSelectedGroupRoundRobinEnabled(
    bool enabled,
    RoundRobinMode mode,
    const std::string& label)
{
    const auto selectedGroup = getSelectedGroup();
    if (!selectedGroup.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin toggle rejected",
                                  "Select a group before changing Round Robin.");

    auto project = getProject();
    const auto status = assessGroupRoundRobin(project, selectedGroup->id);
    if (!enabled)
    {
        if (!status.enabled)
            return makeRejectedResult(getDocumentState(),
                                      "Group Round Robin toggle rejected",
                                      "Round Robin is already off for group '" + selectedGroup->displayName + "'.");

        clearGroupRoundRobin(project, selectedGroup->id);
        return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
    }

    if (!status.eligible)
    {
        auto issue = status.state;
        if (!status.incompatibleZoneIds.empty())
        {
            issue += " Remove or correct these zone(s): ";
            for (std::size_t index = 0; index < status.incompatibleZoneIds.size(); ++index)
            {
                if (index > 0)
                    issue += ", ";
                issue += status.incompatibleZoneIds[index];
            }
            issue += ".";
        }
        return makeRejectedResult(getDocumentState(), "Group Round Robin toggle rejected", issue);
    }

    enableGroupRoundRobin(project, selectedGroup->id, mode);
    return documentController.commitSnapshot(project, label, buildRoundRobinChangedPaths(project));
}

RuntimeProjectDocumentActionResult AuthoringSession::setSelectedGroupRoundRobinMode(
    RoundRobinMode mode,
    const std::string& label)
{
    const auto status = getSelectedGroupRoundRobinStatus();
    if (!status.enabled)
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin mode change rejected",
                                  "Enable Round Robin for the selected group before changing its mode.");

    if (status.mode == mode)
        return makeRejectedResult(getDocumentState(),
                                  "Group Round Robin mode change rejected",
                                  "The selected group already uses that Round Robin mode.");

    return setSelectedGroupRoundRobinEnabled(true, mode, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteSelectedSample()
{
    const auto selectedZoneIndex = findSelectedZoneIndex(getProject());
    if (!selectedZoneIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Sample deletion rejected",
                                  "No sample is currently selected for deletion.");

    return deleteZones({ getProject().authoring.zones[*selectedZoneIndex].id },
                       "Delete selected sample");
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteZones(const std::vector<std::string>& zoneIds,
                                                                 const std::string& label)
{
    if (zoneIds.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Zone deletion rejected",
                                  "At least one zone must be specified for deletion.");

    auto project = getProject();
    std::vector<std::string> normalizedZoneIds;
    normalizedZoneIds.reserve(zoneIds.size());
    for (const auto& zoneId : zoneIds)
    {
        if (!findZoneIndexById(project, zoneId).has_value())
            continue;

        if (std::find(normalizedZoneIds.begin(), normalizedZoneIds.end(), zoneId)
            == normalizedZoneIds.end())
        {
            normalizedZoneIds.push_back(zoneId);
        }
    }

    if (normalizedZoneIds.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Zone deletion rejected",
                                  "None of the requested zones could be resolved for deletion.");

    std::vector<std::pair<std::string, RoundRobinMode>> enabledGroups;
    for (const auto& zoneId : normalizedZoneIds)
    {
        const auto zoneIndex = findZoneIndexById(project, zoneId);
        if (!zoneIndex.has_value())
            continue;
        const auto& groupId = project.authoring.zones[*zoneIndex].groupId;
        if (std::find_if(enabledGroups.begin(),
                         enabledGroups.end(),
                         [&](const auto& entry) { return entry.first == groupId; })
            != enabledGroups.end())
        {
            continue;
        }
        const auto status = assessGroupRoundRobin(project, groupId);
        if (status.enabled)
            enabledGroups.push_back({ groupId, status.mode });
    }

    const auto selectedZoneIndex = findSelectedZoneIndex(project);
    const auto selectedZoneId = project.authoring.selectedZoneId;
    std::vector<std::size_t> deletedZoneIndices;
    std::vector<RuntimeProjectZoneDefinition> deletedZones;
    deletedZoneIndices.reserve(normalizedZoneIds.size());
    deletedZones.reserve(normalizedZoneIds.size());

    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        const auto& zone = project.authoring.zones[index];
        if (std::find(normalizedZoneIds.begin(), normalizedZoneIds.end(), zone.id)
            == normalizedZoneIds.end())
        {
            continue;
        }

        deletedZoneIndices.push_back(index);
        deletedZones.push_back(zone);
    }

    project.authoring.zones.erase(
        std::remove_if(project.authoring.zones.begin(),
                       project.authoring.zones.end(),
                       [&](const RuntimeProjectZoneDefinition& zone)
                       {
                           return std::find(normalizedZoneIds.begin(),
                                            normalizedZoneIds.end(),
                                            zone.id) != normalizedZoneIds.end();
                       }),
        project.authoring.zones.end());

    std::unordered_set<std::string> deletedRoutingSlotIds;
    for (const auto& deletedZone : deletedZones)
    {
        for (auto busIterator = project.authoring.routingBuses.begin();
             busIterator != project.authoring.routingBuses.end();)
        {
            if (busIterator->inputSourceId != deletedZone.id)
            {
                ++busIterator;
                continue;
            }

            const auto replacementZone = std::find_if(project.authoring.zones.begin(),
                                                      project.authoring.zones.end(),
                                                      [&](const RuntimeProjectZoneDefinition& zone)
                                                      {
                                                          return zone.groupId == deletedZone.groupId;
                                                      });
            if (replacementZone != project.authoring.zones.end())
            {
                busIterator->inputSourceId = replacementZone->id;
                ++busIterator;
                continue;
            }

            // A source-owned chain cannot fall back to master: master may already have
            // its own chain, and every source and slot has exactly one owner. Retire the
            // orphaned chain together with its inserts when its final source zone goes away.
            deletedRoutingSlotIds.insert(busIterator->fxSlotIds.begin(), busIterator->fxSlotIds.end());
            busIterator = project.authoring.routingBuses.erase(busIterator);
        }
    }

    if (!deletedRoutingSlotIds.empty())
    {
        project.authoring.fxSlots.erase(
            std::remove_if(project.authoring.fxSlots.begin(),
                           project.authoring.fxSlots.end(),
                           [&](const RuntimeProjectFxSlotDefinition& slot)
                           {
                               return deletedRoutingSlotIds.count(slot.id) != 0;
                           }),
            project.authoring.fxSlots.end());
    }

    std::vector<std::string> affectedRoundRobinPoolIds;
    for (const auto& deletedZone : deletedZones)
    {
        if (!deletedZone.roundRobin.has_value())
            continue;

        const auto& poolId = deletedZone.roundRobin->poolId;
        if (std::find(affectedRoundRobinPoolIds.begin(), affectedRoundRobinPoolIds.end(), poolId)
            == affectedRoundRobinPoolIds.end())
        {
            affectedRoundRobinPoolIds.push_back(poolId);
        }
    }

    for (const auto& poolId : affectedRoundRobinPoolIds)
    {
        auto remainingMembers = collectRoundRobinPoolMemberIndices(project, poolId);
        if (!remainingMembers.empty())
            normalizeRoundRobinPool(project, std::move(remainingMembers), poolId);
    }
    for (const auto& [enabledGroupId, mode] : enabledGroups)
        reconcilePreviouslyEnabledGroup(project, enabledGroupId, mode);

    std::vector<std::string> deletedSampleSourceIds;
    for (const auto& deletedZone : deletedZones)
    {
        if (std::find(deletedSampleSourceIds.begin(),
                      deletedSampleSourceIds.end(),
                      deletedZone.sampleSourceId) == deletedSampleSourceIds.end())
        {
            deletedSampleSourceIds.push_back(deletedZone.sampleSourceId);
        }
    }

    const auto sampleSourceCountBeforeCleanup = project.sampleSources.size();
    project.sampleSources.erase(
        std::remove_if(project.sampleSources.begin(),
                       project.sampleSources.end(),
                       [&](const RuntimeProjectSampleSource& source)
                       {
                           if (std::find(deletedSampleSourceIds.begin(),
                                         deletedSampleSourceIds.end(),
                                         source.id) == deletedSampleSourceIds.end())
                           {
                               return false;
                           }

                           return std::none_of(project.authoring.zones.begin(),
                                               project.authoring.zones.end(),
                                               [&](const RuntimeProjectZoneDefinition& zone)
                                               {
                                                   return zone.sampleSourceId == source.id;
                                               });
                       }),
        project.sampleSources.end());

    const auto selectedZoneSurvived = !selectedZoneId.empty()
        && std::find(normalizedZoneIds.begin(), normalizedZoneIds.end(), selectedZoneId)
            == normalizedZoneIds.end()
        && findZoneIndexById(project, selectedZoneId).has_value();

    if (selectedZoneSurvived)
    {
        project.authoring.selectedZoneId = selectedZoneId;
    }
    else if (project.authoring.zones.empty())
    {
        project.authoring.selectedZoneId.clear();
    }
    else
    {
        const auto fallbackIndex = selectedZoneIndex.has_value()
            ? *selectedZoneIndex
            : (deletedZoneIndices.empty() ? std::size_t {} : deletedZoneIndices.front());
        const auto nextIndex = std::min(fallbackIndex, project.authoring.zones.size() - 1);
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
    if (project.sampleSources.size() != sampleSourceCountBeforeCleanup)
        changedPaths.push_back("sampleSources");

    return documentController.commitSnapshot(project, label, std::move(changedPaths));
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
    const std::string& label,
    const bool reconcileInferredRoundRobin)
{
    if (zones.empty())
        return makeRejectedResult(getDocumentState(),
                                  "Authoring import rejected",
                                  "Imported content must include at least one zone.");

    auto project = getProject();
    std::vector<std::pair<std::string, RoundRobinMode>> enabledGroups;
    for (const auto& group : project.authoring.groups)
    {
        const auto status = assessGroupRoundRobin(project, group.id);
        if (status.enabled)
            enabledGroups.push_back({ group.id, status.mode });
    }

    const auto originalSampleSourceCount = project.sampleSources.size();
    const auto originalZoneCount = project.authoring.zones.size();

    project.sampleSources.insert(project.sampleSources.end(),
                                 std::make_move_iterator(sampleSources.begin()),
                                 std::make_move_iterator(sampleSources.end()));
    project.authoring.zones.insert(project.authoring.zones.end(),
                                   std::make_move_iterator(zones.begin()),
                                   std::make_move_iterator(zones.end()));
    if (reconcileInferredRoundRobin)
        reconcileBatchInferredRoundRobinDescriptors(project.authoring.zones);
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
    if (requiresRoundRobinSchema && project.schemaVersion < 3)
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
    for (const auto& [enabledGroupId, mode] : enabledGroups)
        reconcilePreviouslyEnabledGroup(project, enabledGroupId, mode);

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

RuntimeProjectDocumentActionResult AuthoringSession::createMacro(const RuntimeProjectMacroDefinition& macro,
                                                                 const std::string& label)
{
    auto project = getProject();
    auto createdMacro = normalizeCreatedMacro(project, macro);
    if (const auto validationIssue = validateMacroDefinition(project, createdMacro))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Macro creation rejected",
                                  *validationIssue);
    }

    project.authoring.macros.push_back(std::move(createdMacro));
    const auto result = documentController.commitSnapshot(project, label, { "authoring.macros" });
    if (result.applied)
        selectedMacroId = project.authoring.macros.back().id;
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::duplicateMacro(const std::string& macroId,
                                                                    const std::string& label)
{
    auto project = getProject();
    const auto sourceIndex = findMacroIndexById(project, macroId);
    if (!sourceIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Macro duplication rejected",
                                  "Macro '" + macroId + "' does not exist in the current authoring project.");

    auto duplicate = project.authoring.macros[*sourceIndex];
    duplicate.name = buildDuplicateMacroName(project, duplicate);
    duplicate.id = makeUniqueMacroId(project, duplicate.id + "-copy");
    if (const auto validationIssue = validateMacroDefinition(project, duplicate))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Macro duplication rejected",
                                  *validationIssue);
    }

    project.authoring.macros.push_back(std::move(duplicate));
    const auto result = documentController.commitSnapshot(project, label, { "authoring.macros" });
    if (result.applied)
        selectedMacroId = project.authoring.macros.back().id;
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteMacro(const std::string& macroId,
                                                                 const std::string& label)
{
    auto project = getProject();
    const auto macroIndex = findMacroIndexById(project, macroId);
    if (!macroIndex.has_value())
        return makeRejectedResult(getDocumentState(),
                                  "Macro deletion rejected",
                                  "Macro '" + macroId + "' does not exist in the current authoring project.");

    std::string fallbackSelectedMacroId;
    if (selectedMacroId == macroId)
    {
        const auto nextIndex = *macroIndex + 1;
        if (nextIndex < project.authoring.macros.size())
            fallbackSelectedMacroId = project.authoring.macros[nextIndex].id;
        else if (*macroIndex > 0)
            fallbackSelectedMacroId = project.authoring.macros[*macroIndex - 1].id;
    }

    project.authoring.macros.erase(project.authoring.macros.begin()
                                   + static_cast<std::ptrdiff_t>(*macroIndex));
    const auto result = documentController.commitSnapshot(project, label, { "authoring.macros" });
    if (result.applied)
    {
        selectedMacroId = fallbackSelectedMacroId;
        recoverMacroSelection();
    }
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::updateMacro(std::size_t macroIndex,
                                                                 const RuntimeProjectMacroDefinition& macro,
                                                                 const std::string& label)
{
    if (macroIndex >= getProject().authoring.macros.size())
        return makeRejectedResult(getDocumentState(),
                                  "Macro edit rejected",
                                  "Macro index " + std::to_string(macroIndex) + " is out of range.");

    if (macro.id != getProject().authoring.macros[macroIndex].id)
        return makeRejectedResult(getDocumentState(),
                                  "Macro edit rejected",
                                  "Macro ids are immutable once created.");

    auto updatedMacro = macro;
    const auto& previousMacro = getProject().authoring.macros[macroIndex];
    for (std::size_t targetIndex = 0; targetIndex < updatedMacro.targets.size(); ++targetIndex)
    {
        auto& target = updatedMacro.targets[targetIndex];
        const auto hasPreviousTarget = targetIndex < previousMacro.targets.size();
        const auto sameStructuredIdentity = hasPreviousTarget
            && target.dspSlotId == previousMacro.targets[targetIndex].dspSlotId
            && target.dspParameterId == previousMacro.targets[targetIndex].dspParameterId;
        if (target.controlLaw.id.empty() && hasPreviousTarget && sameStructuredIdentity
            && !previousMacro.targets[targetIndex].controlLaw.id.empty())
            target.controlLaw = previousMacro.targets[targetIndex].controlLaw;
        else if (target.controlLaw.id.empty() && !sameStructuredIdentity)
            resolveNewDspTargetControlLaw(getProject(), target);
    }

    if (const auto validationIssue = validateMacroDefinition(getProject(), updatedMacro, macroIndex))
    {
        return makeRejectedResult(getDocumentState(),
                                  "Macro edit rejected",
                                  *validationIssue);
    }

    auto project = getProject();
    project.authoring.macros[macroIndex] = std::move(updatedMacro);
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

AuthoringMixerTaperUpgradePreview AuthoringSession::previewMixerTaperUpgrade() const
{
    AuthoringMixerTaperUpgradePreview preview;
    const auto& project = getProject();
    for (const auto& macro : project.authoring.macros)
    {
        for (const auto& target : macro.targets)
        {
            if (!isLegacyMixerGainTarget(project, target)) continue;
            preview.affectedMacroIds.push_back(macro.id);
            preview.affectedTargetPaths.push_back(target.parameterPath);
        }
    }
    return preview;
}

RuntimeProjectDocumentActionResult AuthoringSession::upgradeMixerTaper(const std::string& label)
{
    auto project = getProject();
    auto affectedCount = std::size_t { 0 };
    for (auto& macro : project.authoring.macros)
    {
        for (auto& target : macro.targets)
        {
            if (!isLegacyMixerGainTarget(project, target)) continue;
            target.controlLaw.id = std::string(controlLawMixerGainV1);
            target.controlLaw.version = 1;
            target.destinationMinimum = -96.0;
            target.destinationMaximum = 6.0;
            target.curve = "linear"; // retained for pre-law compatibility readers.
            ++affectedCount;
        }
    }
    if (affectedCount == 0)
        return makeRejectedResult(getDocumentState(),
                                  "Mixer taper upgrade rejected",
                                  "No legacy group or bus gain targets are eligible for the mixer taper upgrade.");
    return documentController.commitSnapshot(project, label, { "authoring.macros" });
}

RuntimeProjectDocumentActionResult AuthoringSession::createFxSlot(
    const RuntimeProjectFxSlotDefinition& fxSlot,
    const std::string& ownerBusId,
    const std::string& label)
{
    if (fxSlot.id.empty())
        return makeRejectedResult(getDocumentState(), "FX slot creation rejected", "FX slot id must not be empty.");
    if (findFxSlotIndexById(getProject(), fxSlot.id).has_value())
        return makeRejectedResult(getDocumentState(), "FX slot creation rejected", "FX slot id already exists: " + fxSlot.id);
    const auto ownerIndex = findRoutingBusIndexById(getProject(), ownerBusId);
    if (!ownerIndex.has_value())
        return makeRejectedResult(getDocumentState(), "FX slot creation rejected", "FX slot owner bus does not exist: " + ownerBusId);

    auto project = getProject();
    project.authoring.fxSlots.push_back(fxSlot);
    project.authoring.routingBuses[*ownerIndex].fxSlotIds.push_back(fxSlot.id);
    auto result = documentController.commitSnapshot(project, label,
                                                     { "authoring.fxSlots", "authoring.routingBuses["
                                                         + std::to_string(*ownerIndex) + "].fxSlotIds" });
    if (result.applied) dspSelection = { fxSlot.id, ownerBusId };
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::duplicateFxSlot(const std::string& fxSlotId,
                                                                      const std::string& duplicateId,
                                                                      const std::string& label)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    if (!slotIndex.has_value())
        return makeRejectedResult(getDocumentState(), "FX slot duplication rejected", "FX slot does not exist: " + fxSlotId);
    if (duplicateId.empty() || findFxSlotIndexById(getProject(), duplicateId).has_value())
        return makeRejectedResult(getDocumentState(), "FX slot duplication rejected", "Duplicate FX slot id must be non-empty and unique.");
    const auto ownerIndex = findUniqueFxSlotOwnerBusIndex(getProject(), fxSlotId);
    if (!ownerIndex.has_value())
        return makeRejectedResult(getDocumentState(), "FX slot duplication rejected", "FX slot must have exactly one owner before duplication.");

    auto project = getProject();
    auto duplicate = project.authoring.fxSlots[*slotIndex];
    duplicate.id = duplicateId;
    project.authoring.fxSlots.insert(project.authoring.fxSlots.begin() + static_cast<std::ptrdiff_t>(*slotIndex + 1),
                                     std::move(duplicate));
    auto& ownerSlots = project.authoring.routingBuses[*ownerIndex].fxSlotIds;
    const auto ownerPosition = std::find(ownerSlots.begin(), ownerSlots.end(), fxSlotId);
    ownerSlots.insert(ownerPosition + 1, duplicateId);
    auto result = documentController.commitSnapshot(project, label,
                                                     { "authoring.fxSlots", "authoring.routingBuses["
                                                         + std::to_string(*ownerIndex) + "].fxSlotIds" });
    if (result.applied) dspSelection = { duplicateId, project.authoring.routingBuses[*ownerIndex].id };
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteFxSlot(const std::string& fxSlotId,
                                                                   const std::string& label)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    const auto ownerIndex = findUniqueFxSlotOwnerBusIndex(getProject(), fxSlotId);
    if (!slotIndex.has_value() || !ownerIndex.has_value())
        return makeRejectedResult(getDocumentState(), "FX slot deletion rejected", "FX slot must exist with exactly one owner.");

    auto project = getProject();
    project.authoring.fxSlots.erase(project.authoring.fxSlots.begin() + static_cast<std::ptrdiff_t>(*slotIndex));
    auto& ownerSlots = project.authoring.routingBuses[*ownerIndex].fxSlotIds;
    ownerSlots.erase(std::find(ownerSlots.begin(), ownerSlots.end(), fxSlotId));
    auto result = documentController.commitSnapshot(project, label,
                                                     { "authoring.fxSlots", "authoring.routingBuses["
                                                         + std::to_string(*ownerIndex) + "].fxSlotIds" });
    if (result.applied) recoverDspSelection();
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::moveFxSlot(const std::string& fxSlotId,
                                                                 int direction,
                                                                 const std::string& label)
{
    const auto ownerIndex = findUniqueFxSlotOwnerBusIndex(getProject(), fxSlotId);
    if (!ownerIndex.has_value() || (direction != -1 && direction != 1))
        return makeRejectedResult(getDocumentState(), "FX slot reorder rejected", "FX slot and direction must be valid.");
    const auto& ownerSlots = getProject().authoring.routingBuses[*ownerIndex].fxSlotIds;
    const auto position = static_cast<int>(std::distance(ownerSlots.begin(),
                                                          std::find(ownerSlots.begin(), ownerSlots.end(), fxSlotId)));
    const auto target = position + direction;
    if (target < 0 || target >= static_cast<int>(ownerSlots.size()))
        return makeRejectedResult(getDocumentState(), "FX slot reorder rejected", "FX slot cannot move beyond the list bounds.");

    auto project = getProject();
    auto& mutableOwnerSlots = project.authoring.routingBuses[*ownerIndex].fxSlotIds;
    std::swap(mutableOwnerSlots[static_cast<std::size_t>(position)],
              mutableOwnerSlots[static_cast<std::size_t>(target)]);
    return documentController.commitSnapshot(project, label,
                                             { "authoring.routingBuses[" + std::to_string(*ownerIndex)
                                                 + "].fxSlotIds" });
}

RuntimeProjectDocumentActionResult AuthoringSession::moveFxSlotToBus(const std::string& fxSlotId,
                                                                      const std::string& destinationBusId,
                                                                      const std::string& label)
{
    const auto sourceIndex = findUniqueFxSlotOwnerBusIndex(getProject(), fxSlotId);
    const auto destinationIndex = findRoutingBusIndexById(getProject(), destinationBusId);
    if (!sourceIndex.has_value() || !destinationIndex.has_value() || *sourceIndex == *destinationIndex)
        return makeRejectedResult(getDocumentState(), "FX slot move rejected",
                                  "FX slot must have one distinct existing source and destination owner.");

    auto project = getProject();
    auto& sourceSlots = project.authoring.routingBuses[*sourceIndex].fxSlotIds;
    sourceSlots.erase(std::find(sourceSlots.begin(), sourceSlots.end(), fxSlotId));
    project.authoring.routingBuses[*destinationIndex].fxSlotIds.push_back(fxSlotId);
    auto result = documentController.commitSnapshot(project, label,
                                                     { "authoring.routingBuses[" + std::to_string(*sourceIndex)
                                                         + "].fxSlotIds", "authoring.routingBuses["
                                                         + std::to_string(*destinationIndex) + "].fxSlotIds" });
    if (result.applied && dspSelection.fxSlotId == fxSlotId) dspSelection.routingBusId = destinationBusId;
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::createRoutingBus(
    const RuntimeProjectRoutingBusDefinition& routingBus,
    const std::string& label)
{
    if (routingBus.id.empty() || routingBus.inputSourceId.empty())
        return makeRejectedResult(getDocumentState(), "Routing chain creation rejected",
                                  "Routing chain id and canonical input source must not be empty.");
    if (findRoutingBusIndexById(getProject(), routingBus.id).has_value())
        return makeRejectedResult(getDocumentState(), "Routing chain creation rejected",
                                  "Routing chain id already exists: " + routingBus.id);

    auto project = getProject();
    std::vector<std::string> changedPaths { "authoring.routingBuses" };
    constexpr std::string_view groupPrefix { "groups/" };
    if (routingBus.inputSourceId.rfind(groupPrefix.data(), 0) == 0)
    {
        const auto groupId = routingBus.inputSourceId.substr(groupPrefix.size());
        const auto groupIndex = findGroupIndexById(project, groupId);
        if (!groupIndex.has_value())
            return makeRejectedResult(getDocumentState(), "Routing chain creation rejected",
                                      "Group-scoped chain requires an existing group: " + groupId);
        if (!project.authoring.groups[*groupIndex].routingBusId.empty())
            return makeRejectedResult(getDocumentState(), "Routing chain creation rejected",
                                      "Group '" + groupId + "' already owns routing bus '"
                                          + project.authoring.groups[*groupIndex].routingBusId + "'.");
        project.authoring.groups[*groupIndex].routingBusId = routingBus.id;
        changedPaths.push_back("authoring.groups[" + std::to_string(*groupIndex) + "].routingBusId");
    }
    project.authoring.routingBuses.push_back(routingBus);
    return documentController.commitSnapshot(project, label, std::move(changedPaths));
}

RuntimeProjectDocumentActionResult AuthoringSession::deleteRoutingBus(const std::string& busId,
                                                                       const std::string& label)
{
    const auto busIndex = findRoutingBusIndexById(getProject(), busId);
    if (!busIndex.has_value())
        return makeRejectedResult(getDocumentState(), "Routing chain deletion rejected", "Routing bus does not exist: " + busId);

    auto project = getProject();
    const auto removedSlotIds = project.authoring.routingBuses[*busIndex].fxSlotIds;
    project.authoring.fxSlots.erase(std::remove_if(project.authoring.fxSlots.begin(),
                                                   project.authoring.fxSlots.end(),
                                                   [&](const RuntimeProjectFxSlotDefinition& slot)
                                                   {
                                                       return std::find(removedSlotIds.begin(), removedSlotIds.end(), slot.id)
                                                           != removedSlotIds.end();
                                                   }),
                                    project.authoring.fxSlots.end());
    project.authoring.routingBuses.erase(project.authoring.routingBuses.begin()
                                         + static_cast<std::ptrdiff_t>(*busIndex));
    for (auto& group : project.authoring.groups)
        if (group.routingBusId == busId)
            group.routingBusId.clear();

    auto result = documentController.commitSnapshot(project, label,
                                                     { "authoring.routingBuses", "authoring.fxSlots", "authoring.groups" });
    if (result.applied) recoverDspSelection();
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::setRoutingBusChainBypassed(
    const std::string& busId,
    bool bypassed,
    const std::string& label)
{
    const auto busIndex = findRoutingBusIndexById(getProject(), busId);
    if (!busIndex.has_value())
        return makeRejectedResult(getDocumentState(), "Routing bypass rejected", "Routing bus does not exist: " + busId);

    auto project = getProject();
    project.authoring.routingBuses[*busIndex].chainBypassed = bypassed;
    return documentController.commitSnapshot(project, label,
                                             { "authoring.routingBuses[" + std::to_string(*busIndex)
                                                 + "].chainBypassed" });
}

RuntimeProjectDocumentActionResult AuthoringSession::setFxSlotParameter(const std::string& fxSlotId,
                                                                         const std::string& parameterId,
                                                                         double value,
                                                                         const std::string& label)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    if (!slotIndex.has_value() || parameterId.empty() || !std::isfinite(value))
        return makeRejectedResult(getDocumentState(), "DSP parameter edit rejected",
                                  "FX slot, parameter id, and finite value are required.");
    const auto& slot = getProject().authoring.fxSlots[*slotIndex];
    const auto* descriptor = findCatalogParameter(slot, parameterId);
    if (findCuratedDspEffect(slot.effectType, slot.effectVersion) != nullptr
        && (descriptor == nullptr || value < descriptor->minimum || value > descriptor->maximum))
    {
        return makeRejectedResult(getDocumentState(), "DSP parameter edit rejected",
                                  "Known catalog parameter is unknown or outside its allowed range.");
    }

    auto project = getProject();
    auto& parameters = project.authoring.fxSlots[*slotIndex].parameters;
    const auto parameter = std::find_if(parameters.begin(), parameters.end(),
                                        [&](const RuntimeProjectFxSlotDefinition::ParameterValue& entry)
                                        { return entry.id == parameterId; });
    if (parameter == parameters.end())
        parameters.push_back({ parameterId, value });
    else
        parameter->value = value;
    return documentController.commitSnapshot(project, label,
                                             { "authoring.fxSlots[" + std::to_string(*slotIndex)
                                                 + "].parameters." + parameterId });
}

RuntimeProjectDocumentActionResult AuthoringSession::resetFxSlotParameterToDefault(
    const std::string& fxSlotId,
    const std::string& parameterId,
    const std::string& label)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    if (!slotIndex.has_value())
        return makeRejectedResult(getDocumentState(), "DSP parameter reset rejected", "FX slot does not exist: " + fxSlotId);
    const auto* descriptor = findCatalogParameter(getProject().authoring.fxSlots[*slotIndex], parameterId);
    if (descriptor == nullptr)
        return makeRejectedResult(getDocumentState(), "DSP parameter reset rejected",
                                  "Only known catalog parameters have an authored default.");
    return setFxSlotParameter(fxSlotId, parameterId, descriptor->defaultValue, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::beginFxSlotParameterGesture(
    const std::string& fxSlotId,
    const std::string& parameterId)
{
    const auto slotIndex = findFxSlotIndexById(getProject(), fxSlotId);
    if (pendingDspParameterGesture.has_value() || !slotIndex.has_value() || parameterId.empty())
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected",
                                  "A gesture requires one existing slot, parameter id, and no active gesture.");
    const auto& parameters = getProject().authoring.fxSlots[*slotIndex].parameters;
    const auto parameter = std::find_if(parameters.begin(), parameters.end(),
                                        [&](const RuntimeProjectFxSlotDefinition::ParameterValue& entry)
                                        { return entry.id == parameterId; });
    const auto* descriptor = findCatalogParameter(getProject().authoring.fxSlots[*slotIndex], parameterId);
    if (parameter == parameters.end() && descriptor == nullptr)
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected",
                                  "A gesture may only edit an existing or catalog-defined parameter.");
    pendingDspParameterGesture = { fxSlotId, parameterId,
                                   parameter == parameters.end() ? descriptor->defaultValue : parameter->value };
    auto result = RuntimeProjectDocumentActionResult {};
    result.applied = true;
    result.state = "DSP parameter gesture started";
    result.documentState = getDocumentState();
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::updateFxSlotParameterGesture(double value)
{
    if (!pendingDspParameterGesture.has_value())
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected", "No parameter gesture is active.");
    const auto slotIndex = findFxSlotIndexById(getProject(), pendingDspParameterGesture->fxSlotId);
    const auto* descriptor = slotIndex.has_value()
        ? findCatalogParameter(getProject().authoring.fxSlots[*slotIndex], pendingDspParameterGesture->parameterId)
        : nullptr;
    if (!std::isfinite(value) || (descriptor != nullptr && (value < descriptor->minimum || value > descriptor->maximum)))
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected", "Gesture value is not valid for its parameter.");
    pendingDspParameterGesture->value = value;
    if (dspParameterGesturePreviewListener)
        dspParameterGesturePreviewListener(pendingDspParameterGesture->fxSlotId,
                                           pendingDspParameterGesture->parameterId,
                                           value);
    auto result = RuntimeProjectDocumentActionResult {};
    result.applied = true;
    result.state = "DSP parameter gesture preview updated";
    result.documentState = getDocumentState();
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::commitFxSlotParameterGesture(const std::string& label)
{
    if (!pendingDspParameterGesture.has_value())
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected", "No parameter gesture is active.");
    const auto gesture = *pendingDspParameterGesture;
    pendingDspParameterGesture.reset();
    return setFxSlotParameter(gesture.fxSlotId, gesture.parameterId, gesture.value, label);
}

RuntimeProjectDocumentActionResult AuthoringSession::cancelFxSlotParameterGesture()
{
    if (!pendingDspParameterGesture.has_value())
        return makeRejectedResult(getDocumentState(), "DSP parameter gesture rejected", "No parameter gesture is active.");
    pendingDspParameterGesture.reset();
    auto result = RuntimeProjectDocumentActionResult {};
    result.applied = true;
    result.state = "DSP parameter gesture cancelled";
    result.documentState = getDocumentState();
    return result;
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
    auto result = documentController.undo();
    if (result.applied)
    {
        recoverDspSelection();
        recoverMacroSelection();
    }
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::redo()
{
    auto result = documentController.redo();
    if (result.applied)
    {
        recoverDspSelection();
        recoverMacroSelection();
    }
    return result;
}

RuntimeProjectDocumentActionResult AuthoringSession::applyProjectMigration(
    RuntimeProjectModel migratedProject)
{
    auto result = documentController.commitSnapshot(
        migratedProject,
        "Upgrade project to curated DSP schema",
        { "schemaVersion", "authoring.schemaVersion", "authoring.fxSlots", "authoring.routingBuses" });
    if (result.applied)
    {
        recoverDspSelection();
        recoverMacroSelection();
    }
    return result;
}

void AuthoringSession::recoverDspSelection()
{
    if (!dspSelection.fxSlotId.empty())
    {
        if (const auto owner = findUniqueFxSlotOwnerBusIndex(getProject(), dspSelection.fxSlotId); owner.has_value())
        {
            dspSelection.routingBusId = getProject().authoring.routingBuses[*owner].id;
            return;
        }
        dspSelection.fxSlotId.clear();
    }

    const auto bus = findRoutingBusIndexById(getProject(), dspSelection.routingBusId);
    if (!bus.has_value())
        dspSelection.routingBusId = getProject().authoring.routingBuses.empty()
            ? std::string {} : getProject().authoring.routingBuses.front().id;
    const auto selectedBus = findRoutingBusIndexById(getProject(), dspSelection.routingBusId);
    if (selectedBus.has_value() && !getProject().authoring.routingBuses[*selectedBus].fxSlotIds.empty())
    {
        dspSelection.fxSlotId = getProject().authoring.routingBuses[*selectedBus].fxSlotIds.front();
        return;
    }
    for (const auto& candidateBus : getProject().authoring.routingBuses)
    {
        if (!candidateBus.fxSlotIds.empty())
        {
            dspSelection.routingBusId = candidateBus.id;
            dspSelection.fxSlotId = candidateBus.fxSlotIds.front();
            return;
        }
    }
}

void AuthoringSession::recoverMacroSelection()
{
    if (!selectedMacroId.empty() && findMacroIndexById(getProject(), selectedMacroId).has_value())
        return;

    selectedMacroId = getProject().authoring.macros.empty()
        ? std::string {}
        : getProject().authoring.macros.front().id;
}

void AuthoringSession::markSaved()
{
    documentController.markSaved();
}
} // namespace drs::engine
