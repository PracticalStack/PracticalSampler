#include "shared/authoring/StructureScope.h"

#include <algorithm>

namespace drs::app::authoring
{
namespace
{
const drs::engine::RuntimeProjectGroupDefinition* findGroup(
    const drs::engine::RuntimeProjectModel& project, const std::string& id)
{
    const auto it = std::find_if(project.authoring.groups.begin(), project.authoring.groups.end(),
                                 [&](const auto& candidate) { return candidate.id == id; });
    return it == project.authoring.groups.end() ? nullptr : &*it;
}

const drs::engine::RuntimeProjectLayerDefinition* findLayer(
    const drs::engine::RuntimeProjectModel& project, const std::string& id)
{
    const auto it = std::find_if(project.authoring.layers.begin(), project.authoring.layers.end(),
                                 [&](const auto& candidate) { return candidate.id == id; });
    return it == project.authoring.layers.end() ? nullptr : &*it;
}

const drs::engine::RuntimeProjectZoneDefinition* findZone(
    const drs::engine::RuntimeProjectModel& project, const std::string& id)
{
    const auto it = std::find_if(project.authoring.zones.begin(), project.authoring.zones.end(),
                                 [&](const auto& candidate) { return candidate.id == id; });
    return it == project.authoring.zones.end() ? nullptr : &*it;
}
} // namespace

StructureScope makeInstrumentStructureScope()
{
    return { StructureScopeKind::instrument, kInstrumentStructureId };
}

bool isValidStructureScope(const drs::engine::RuntimeProjectModel& project,
                           const StructureScope& scope) noexcept
{
    if (scope.kind == StructureScopeKind::instrument)
        return scope.id == kInstrumentStructureId;
    if (scope.kind == StructureScopeKind::layer)
        return findLayer(project, scope.id) != nullptr;
    if (const auto* group = findGroup(project, scope.id); group != nullptr)
        return findLayer(project, group->layerId) != nullptr;
    return false;
}

StructureScope deriveStructureScope(const drs::engine::RuntimeProjectModel& project,
                                    const AuthoringStructureSelection& selection)
{
    if (selection.getKind() == StructureSelectionKind::layer && !selection.isEmpty())
        if (findLayer(project, selection.getPrimaryId()) != nullptr)
            return { StructureScopeKind::layer, selection.getPrimaryId() };

    if (selection.getKind() == StructureSelectionKind::group && !selection.isEmpty())
        if (const auto* group = findGroup(project, selection.getPrimaryId()); group != nullptr
            && findLayer(project, group->layerId) != nullptr)
            return { StructureScopeKind::group, selection.getPrimaryId() };

    if (selection.getKind() == StructureSelectionKind::zone && !selection.isEmpty())
    {
        const auto* first = findZone(project, selection.getPrimaryId());
        if (first != nullptr)
        {
            const auto* group = findGroup(project, first->groupId);
            if (group != nullptr && findLayer(project, group->layerId) != nullptr)
            {
                bool sameGroup = true;
                bool sameLayer = true;
                for (const auto& id : selection.getIds())
                {
                    const auto* zone = findZone(project, id);
                    const auto* candidateGroup = zone == nullptr ? nullptr : findGroup(project, zone->groupId);
                    if (zone == nullptr || candidateGroup == nullptr || zone->groupId != first->groupId)
                        sameGroup = false;
                    if (zone == nullptr || candidateGroup == nullptr || candidateGroup->layerId != group->layerId)
                        sameLayer = false;
                }
                if (sameGroup)
                    return { StructureScopeKind::group, group->id };
                if (sameLayer)
                    return { StructureScopeKind::layer, group->layerId };
            }
        }
    }
    return makeInstrumentStructureScope();
}

StructureScope reconcileStructureScope(const drs::engine::RuntimeProjectModel& project,
                                       const StructureScope& previous)
{
    if (isValidStructureScope(project, previous))
        return previous;
    if (previous.kind == StructureScopeKind::group)
    {
        // The group may have been deleted; no sibling is a safe guess. Fall
        // back to its surviving layer only when a caller has kept that context
        // in the authored model (otherwise the instrument root is deterministic).
        return makeInstrumentStructureScope();
    }
    return makeInstrumentStructureScope();
}

std::string structureScopeName(const drs::engine::RuntimeProjectModel& project,
                               const StructureScope& scope)
{
    if (scope.kind == StructureScopeKind::instrument)
        return project.displayName.empty() ? "Instrument" : project.displayName;
    if (scope.kind == StructureScopeKind::layer)
    {
        if (const auto* layer = findLayer(project, scope.id); layer != nullptr)
            return layer->displayName.empty() ? layer->id : layer->displayName;
    }
    else if (const auto* group = findGroup(project, scope.id); group != nullptr)
    {
        return group->displayName.empty() ? group->id : group->displayName;
    }
    return "Instrument";
}

std::vector<std::string> zoneIdsInStructureScope(const drs::engine::RuntimeProjectModel& project,
                                                  const StructureScope& scope)
{
    std::vector<std::string> result;
    for (const auto& zone : project.authoring.zones)
    {
        const auto* group = findGroup(project, zone.groupId);
        const bool included = scope.kind == StructureScopeKind::instrument
            || (scope.kind == StructureScopeKind::group && group != nullptr && group->id == scope.id)
            || (scope.kind == StructureScopeKind::layer && group != nullptr && group->layerId == scope.id);
        if (included)
            result.push_back(zone.id);
    }
    return result;
}
} // namespace drs::app::authoring
