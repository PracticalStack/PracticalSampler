#include "shared/authoring/ScopedZoneProjection.h"

#include <algorithm>
#include <cctype>

namespace drs::app::authoring
{
namespace
{
bool matches(const std::string& value, const std::string& query)
{
    if (query.empty()) return true;
    auto lower = [](std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(),
                       [](const unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    };
    return lower(value).find(lower(query)) != std::string::npos;
}

const drs::engine::RuntimeProjectGroupDefinition* groupFor(
    const drs::engine::RuntimeProjectModel& project, const std::string& id)
{
    const auto it = std::find_if(project.authoring.groups.begin(), project.authoring.groups.end(),
                                 [&](const auto& group) { return group.id == id; });
    return it == project.authoring.groups.end() ? nullptr : &*it;
}

const drs::engine::RuntimeProjectLayerDefinition* layerFor(
    const drs::engine::RuntimeProjectModel& project, const std::string& id)
{
    const auto it = std::find_if(project.authoring.layers.begin(), project.authoring.layers.end(),
                                 [&](const auto& layer) { return layer.id == id; });
    return it == project.authoring.layers.end() ? nullptr : &*it;
}

bool selected(const AuthoringStructureSelection& selection, const std::string& id)
{
    return selection.getKind() == StructureSelectionKind::zone && selection.contains(id);
}

drs::engine::AuthoringZoneSummary summarize(const drs::engine::RuntimeProjectZoneDefinition& zone,
                                            const AuthoringStructureSelection& selection)
{
    drs::engine::AuthoringZoneSummary result;
    result.id = zone.id;
    result.displayName = zone.displayName;
    result.sampleSourceId = zone.sampleSourceId;
    result.articulationId = zone.articulationId;
    result.groupId = zone.groupId;
    result.rootKey = zone.rootKey;
    result.keyLow = zone.keyLow;
    result.keyHigh = zone.keyHigh;
    result.velocityLow = zone.velocityLow;
    result.velocityHigh = zone.velocityHigh;
    result.velocityCrossfade = zone.velocityCrossfade;
    result.gainDb = zone.gainDb;
    result.pan = zone.pan;
    result.loopEnabled = zone.loopEnabled;
    result.loopMode = zone.loopMode;
    result.sampleEndFrame = zone.sampleEndFrame;
    result.roundRobin = zone.roundRobin;
    result.roundRobinLength = zone.roundRobinLength;
    result.roundRobinPosition = zone.roundRobinPosition;
    result.triggerMode = zone.triggerMode;
    result.selected = selection.getPrimaryId() == zone.id && selected(selection, zone.id);
    result.additionallySelected = selected(selection, zone.id) && !result.selected;
    return result;
}
} // namespace

ScopedZoneProjection buildScopedZoneProjection(
    const drs::engine::RuntimeProjectModel& project,
    const StructureScope& scope,
    const AuthoringStructureSelection& selection,
    const ScopedZoneProjectionOptions& options)
{
    ScopedZoneProjection result;
    result.scope = isValidStructureScope(project, scope) ? scope : makeInstrumentStructureScope();
    for (const auto& zone : project.authoring.zones)
    {
        const auto* group = groupFor(project, zone.groupId);
        const bool inScope = result.scope.kind == StructureScopeKind::instrument
            || (result.scope.kind == StructureScopeKind::group && group != nullptr && group->id == result.scope.id)
            || (result.scope.kind == StructureScopeKind::layer && group != nullptr && group->layerId == result.scope.id);
        if (!inScope)
            continue;
        ++result.totalInScope;
        const auto* layer = group == nullptr ? nullptr : layerFor(project, group->layerId);
        const bool hidden = group == nullptr || layer == nullptr || !group->workspaceVisible || !layer->workspaceVisible;
        if (hidden)
            ++result.hiddenCount;
        if (hidden && !options.includeHiddenContainers && !(options.includeSelectedHidden && selected(selection, zone.id)))
            continue;
        if (!matches(zone.displayName.empty() ? zone.id : zone.displayName, options.searchText)
            || (!options.articulationFilter.empty() && zone.articulationId != options.articulationFilter)
            || (options.performanceEventFilter.has_value() && zone.performance.event != *options.performanceEventFilter))
            continue;
        result.zones.push_back(summarize(zone, selection));
    }
    return result;
}
} // namespace drs::app::authoring
