#include "shared/authoring/StructureViewModels.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <sstream>
#include <functional>

namespace drs::app::authoring
{
namespace
{
struct RangeAccumulator
{
    bool hasValue = false;
    int low = 127;
    int high = 0;

    void include(const int nextLow, const int nextHigh) noexcept
    {
        hasValue = true;
        low = std::min(low, nextLow);
        high = std::max(high, nextHigh);
    }
};

std::string rangeText(const int low, const int high)
{
    return std::to_string(low) + "–" + std::to_string(high);
}

bool selected(const AuthoringStructureSelection& selection,
              const StructureSelectionKind kind,
              const std::string& id)
{
    return selection.getKind() == kind && selection.contains(id);
}

bool primary(const AuthoringStructureSelection& selection,
             const StructureSelectionKind kind,
             const std::string& id)
{
    return selection.getKind() == kind && selection.getPrimaryId() == id;
}

bool matchesSearch(const std::string& value, const std::string& query)
{
    if (query.empty()) return true;
    auto lower = [](std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](const unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
        return text;
    };
    return lower(value).find(lower(query)) != std::string::npos;
}

bool keepOverlap(const StructureOverlapInfo& info, const StructureViewProjectionOptions& options)
{
    if (options.showExactStacksOnly)
        return info.kind == StructureOverlapKind::exactStack || info.kind == StructureOverlapKind::exactKeyStack;
    if (options.showPotentialCollisionsOnly)
        return info.kind == StructureOverlapKind::potentialCollision || info.hasPotentialCollision;
    if (options.showOverlapsOnly) return info.kind != StructureOverlapKind::none;
    return true;
}
} // namespace

void StructureOverlapCache::refreshIfNeeded(const drs::engine::RuntimeProjectModel& project)
{
    std::size_t nextFingerprint = std::hash<std::string> {}(project.projectId);
    const auto mix = [&](const std::size_t value)
    {
        nextFingerprint ^= value + 0x9e3779b9u + (nextFingerprint << 6) + (nextFingerprint >> 2);
    };
    for (const auto& layer : project.authoring.layers)
    {
        mix(std::hash<std::string> {}(layer.id));
        mix(static_cast<std::size_t>(layer.displayOrder));
        mix(static_cast<std::size_t>(layer.workspaceVisible));
    }
    for (const auto& group : project.authoring.groups)
    {
        mix(std::hash<std::string> {}(group.id));
        mix(std::hash<std::string> {}(group.layerId));
        mix(static_cast<std::size_t>(group.displayOrder));
        mix(static_cast<std::size_t>(group.workspaceVisible));
    }
    for (const auto& zone : project.authoring.zones)
    {
        mix(std::hash<std::string> {}(zone.id));
        mix(std::hash<std::string> {}(zone.groupId));
        mix(static_cast<std::size_t>(zone.keyLow * 131 + zone.keyHigh * 17
                                     + zone.velocityLow * 7 + zone.velocityHigh));
        mix(std::hash<std::string> {}(zone.articulationId));
        mix(static_cast<std::size_t>(zone.triggerMode));
        mix(static_cast<std::size_t>(zone.performance.event));
        mix(static_cast<std::size_t>(zone.performance.sustain));
        mix(static_cast<std::size_t>(zone.performance.pitchSource));
        mix(static_cast<std::size_t>(zone.performance.triggerControllerNumber.value_or(-1)));
        mix(static_cast<std::size_t>(zone.roundRobinLength * 31 + zone.roundRobinPosition));
        mix(static_cast<std::size_t>(zone.velocityCrossfade.fadeInLowVelocity * 17
                                     + zone.velocityCrossfade.fadeInHighVelocity * 19
                                     + zone.velocityCrossfade.fadeOutLowVelocity * 23
                                     + zone.velocityCrossfade.fadeOutHighVelocity * 29
                                     + static_cast<int>(zone.velocityCrossfade.curve)));
    }
    if (nextFingerprint == fingerprint && info.size() == project.authoring.zones.size())
        return;
    fingerprint = nextFingerprint;
    info = analyzeStructureOverlaps(project.authoring.zones);
}

StructureHierarchyViewModel buildStructureHierarchyViewModel(
    const drs::engine::RuntimeProjectModel& project,
    const AuthoringStructureSelection& selection,
    const std::string& primaryLayerId,
    const std::string& primaryGroupId,
    const StructureViewProjectionOptions& options,
    const StructureOverlapCache* overlapCache)
{
    StructureHierarchyViewModel result;
    result.layers.reserve(project.authoring.layers.size());
    result.groups.reserve(project.authoring.groups.size());

    for (const auto& layer : project.authoring.layers)
    {
        int groupCount = 0;
        int zoneCount = 0;
        RangeAccumulator range;
        for (const auto& group : project.authoring.groups)
        {
            if (group.layerId != layer.id)
                continue;

            ++groupCount;
            for (const auto& zone : project.authoring.zones)
            {
                if (zone.groupId == group.id)
                {
                    ++zoneCount;
                    range.include(zone.keyLow, zone.keyHigh);
                }
            }
        }

        auto row = StructureLayerRowViewModel {};
        row.id = layer.id;
        row.title = layer.displayName.empty() ? layer.id : layer.displayName;
        row.displayOrder = layer.displayOrder;
        row.groupCount = groupCount;
        row.zoneCount = zoneCount;
        row.keyLow = range.hasValue ? range.low : 0;
        row.keyHigh = range.hasValue ? range.high : 127;
        row.workspaceVisible = layer.workspaceVisible;
        row.routingBusId = layer.routingBusId;
        row.auditionAnchorId = layer.auditionAnchorGroupId;
        row.accessibilityText = row.title + ", " + row.statusText + ", key range " + rangeText(row.keyLow, row.keyHigh);
        if ((options.visibleOnly && !row.workspaceVisible)
            || !matchesSearch(row.title, options.searchText))
            continue;
        row.selected = selected(selection, StructureSelectionKind::layer, layer.id);
        row.primary = primary(selection, StructureSelectionKind::layer, layer.id);
        row.statusText = std::to_string(groupCount) + " groups · " + std::to_string(zoneCount) + " zones"
            + (layer.workspaceVisible ? " · visible" : " · hidden");
        result.layers.push_back(std::move(row));
    }

    for (const auto& group : project.authoring.groups)
    {
        if (!primaryLayerId.empty() && group.layerId != primaryLayerId)
            continue;

        int zoneCount = 0;
        RangeAccumulator range;
        for (const auto& zone : project.authoring.zones)
        {
            if (zone.groupId == group.id)
            {
                ++zoneCount;
                range.include(zone.keyLow, zone.keyHigh);
            }
        }

        auto row = StructureGroupRowViewModel {};
        row.id = group.id;
        row.layerId = group.layerId;
        row.title = group.displayName.empty() ? group.id : group.displayName;
        row.displayOrder = group.displayOrder;
        row.zoneCount = zoneCount;
        row.keyLow = range.hasValue ? range.low : 0;
        row.keyHigh = range.hasValue ? range.high : 127;
        row.workspaceVisible = group.workspaceVisible;
        row.routingBusId = group.routingBusId;
        row.auditionAnchorId = group.auditionAnchorZoneId;
        row.accessibilityText = row.title + ", " + row.statusText + ", key range " + rangeText(row.keyLow, row.keyHigh);
        if ((options.visibleOnly && !row.workspaceVisible)
            || !matchesSearch(row.title, options.searchText))
            continue;
        row.selected = selected(selection, StructureSelectionKind::group, group.id);
        row.primary = primary(selection, StructureSelectionKind::group, group.id);
        row.statusText = std::to_string(zoneCount) + " zones"
            + (group.workspaceVisible ? " · visible" : " · hidden");
        result.groups.push_back(std::move(row));
    }

    std::unordered_map<std::string, StructureOverlapInfo> overlapById;
    const auto diagnosticsRequested = options.showOverlapsOnly
        || options.showPotentialCollisionsOnly
        || options.showExactStacksOnly
        || options.sortMode == StructureSortMode::diagnostic
        || options.includeDiagnostics;
    if (diagnosticsRequested)
    {
        if (overlapCache != nullptr)
        {
            const auto& overlapInfo = overlapCache->getInfo();
            for (std::size_t index = 0; index < project.authoring.zones.size() && index < overlapInfo.size(); ++index)
                overlapById.emplace(project.authoring.zones[index].id, overlapInfo[index]);
        }
        else
        {
            const auto overlapInfo = analyzeStructureOverlaps(project.authoring.zones);
            for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
                overlapById.emplace(project.authoring.zones[index].id, overlapInfo[index]);
        }
    }

    if (!primaryGroupId.empty())
    {
        for (const auto& zone : project.authoring.zones)
        {
            if (zone.groupId != primaryGroupId)
                continue;

            auto row = StructureZoneRowViewModel {};
            row.id = zone.id;
            row.groupId = zone.groupId;
            row.title = zone.displayName.empty() ? zone.id : zone.displayName;
            row.sampleSourceId = zone.sampleSourceId;
            row.articulationId = zone.articulationId;
            row.rootKey = zone.rootKey;
            row.keyLow = zone.keyLow;
            row.keyHigh = zone.keyHigh;
            row.velocityLow = zone.velocityLow;
            row.velocityHigh = zone.velocityHigh;
            row.keyStartNormalized = std::clamp(static_cast<float>(zone.keyLow) / 128.0f, 0.0f, 1.0f);
            row.keyEndNormalized = std::clamp(static_cast<float>(zone.keyHigh + 1) / 128.0f, 0.0f, 1.0f);
            row.rootNormalized = std::clamp(static_cast<float>(zone.rootKey) / 127.0f, 0.0f, 1.0f);
            row.roundRobinPosition = zone.roundRobin.has_value()
                ? zone.roundRobin->slotIndex : zone.roundRobinPosition;
            row.roundRobinLength = zone.roundRobin.has_value()
                ? zone.roundRobin->slotCount : zone.roundRobinLength;
            row.triggerMode = zone.triggerMode;
            row.performanceEvent = zone.performance.event;
            row.hasVelocityCrossfade = drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade);
            row.accessibilityText = row.title + ", key range " + rangeText(row.keyLow, row.keyHigh)
                + ", velocity " + rangeText(row.velocityLow, row.velocityHigh)
                + ", articulation " + row.articulationId;
            row.selected = selected(selection, StructureSelectionKind::zone, zone.id);
            row.primary = primary(selection, StructureSelectionKind::zone, zone.id);
            if (const auto diagnostic = overlapById.find(zone.id); diagnostic != overlapById.end())
            {
                row.overlapKind = diagnostic->second.kind;
                row.overlapCount = diagnostic->second.overlapCount;
                row.hasPotentialCollision = diagnostic->second.hasPotentialCollision;
                row.overlapReason = diagnostic->second.reason;
            }
            StructureOverlapInfo rowDiagnostic;
            rowDiagnostic.kind = row.overlapKind;
            rowDiagnostic.hasPotentialCollision = row.hasPotentialCollision;
            if (!matchesSearch(row.title, options.searchText) || !keepOverlap(rowDiagnostic, options))
                continue;
            if ((!options.articulationFilter.empty() && row.articulationId != options.articulationFilter)
                || (options.performanceEventFilter.has_value()
                    && row.performanceEvent != *options.performanceEventFilter))
                continue;
            result.zones.push_back(std::move(row));
        }
    }

    if (options.sortMode != StructureSortMode::authoredOrder)
    {
        if (options.sortMode == StructureSortMode::name)
        {
            std::sort(result.layers.begin(), result.layers.end(), [](const auto& left, const auto& right) { return left.title < right.title; });
            std::sort(result.groups.begin(), result.groups.end(), [](const auto& left, const auto& right) { return left.title < right.title; });
            std::sort(result.zones.begin(), result.zones.end(), [](const auto& left, const auto& right) { return left.title < right.title; });
        }
        else if (options.sortMode == StructureSortMode::keyLow)
            std::sort(result.zones.begin(), result.zones.end(), [](const auto& left, const auto& right) { return left.keyLow < right.keyLow; });
        else if (options.sortMode == StructureSortMode::rootKey)
            std::sort(result.zones.begin(), result.zones.end(), [](const auto& left, const auto& right) { return left.rootKey < right.rootKey; });
        else if (options.sortMode == StructureSortMode::velocityLow)
            std::sort(result.zones.begin(), result.zones.end(), [](const auto& left, const auto& right) { return left.velocityLow < right.velocityLow; });
        else if (options.sortMode == StructureSortMode::diagnostic)
            std::sort(result.zones.begin(), result.zones.end(), [](const auto& left, const auto& right) { return left.overlapKind > right.overlapKind; });
    }

    return result;
}
} // namespace drs::app::authoring
