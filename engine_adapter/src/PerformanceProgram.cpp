#include "drs/engine/PerformanceProgram.h"

#include <json/json.hpp>

#include <algorithm>
#include <unordered_map>

namespace drs::engine
{
namespace
{
std::size_t eventIndex(PerformanceEventKind event) noexcept
{
    return static_cast<std::size_t>(event);
}

std::uint64_t stableIdHash(std::string_view text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash;
}

template <typename Value, typename ReadId>
std::vector<std::string> sortedIds(const std::vector<Value>& values, const ReadId& readId)
{
    std::vector<std::string> ids;
    ids.reserve(values.size());
    for (const auto& value : values)
        if (!readId(value).empty())
            ids.push_back(readId(value));
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::unordered_map<std::string, std::uint32_t> makeIndex(const std::vector<std::string>& ids)
{
    std::unordered_map<std::string, std::uint32_t> index;
    index.reserve(ids.size());
    for (std::size_t item = 0; item < ids.size(); ++item)
        index.emplace(ids[item], static_cast<std::uint32_t>(item));
    return index;
}
} // namespace

CompiledPerformanceProgramResult compilePerformanceProgram(const RuntimeProjectAuthoringState& authoring)
{
    CompiledPerformanceProgramResult result;
    for (auto& activation : result.program.activationByMidiNote)
        activation.articulationIndex = kInvalidPerformanceProgramIndex;

    const auto articulationIds = sortedIds(authoring.articulations, [](const auto& item) -> const std::string& { return item.id; });
    const auto articulationIndex = makeIndex(articulationIds);
    result.program.articulationCount = static_cast<std::uint32_t>(articulationIds.size());
    result.program.articulationStableIds.reserve(articulationIds.size());
    for (const auto& id : articulationIds)
        result.program.articulationStableIds.push_back(stableIdHash(id));
    for (const auto& articulation : authoring.articulations)
    {
        if (!articulation.isDefault) continue;
        const auto index = articulationIndex.find(articulation.id);
        if (index != articulationIndex.end()) result.program.defaultArticulationIndex = index->second;
        break;
    }

    result.program.zoneArticulationIndices.resize(authoring.zones.size(), kInvalidPerformanceProgramIndex);
    for (std::size_t zoneIndex = 0; zoneIndex < authoring.zones.size(); ++zoneIndex)
    {
        const auto articulation = articulationIndex.find(authoring.zones[zoneIndex].articulationId);
        if (articulation != articulationIndex.end())
            result.program.zoneArticulationIndices[zoneIndex] = articulation->second;
    }

    std::vector<std::string> groupIds;
    for (const auto& zone : authoring.zones)
    {
        if (!zone.exclusiveGroupId.empty()) groupIds.push_back(zone.exclusiveGroupId);
        groupIds.insert(groupIds.end(), zone.exclusiveTargetGroupIds.begin(), zone.exclusiveTargetGroupIds.end());
    }
    std::sort(groupIds.begin(), groupIds.end());
    groupIds.erase(std::unique(groupIds.begin(), groupIds.end()), groupIds.end());
    const auto groupIndex = makeIndex(groupIds);
    result.program.exclusiveGroupCount = static_cast<std::uint32_t>(groupIds.size());
    result.program.exclusiveGroupStableIds.reserve(groupIds.size());
    for (const auto& id : groupIds)
        result.program.exclusiveGroupStableIds.push_back(stableIdHash(id));

    std::vector<std::string> poolIds;
    for (const auto& zone : authoring.zones)
        if (zone.roundRobin.has_value() && !zone.roundRobin->poolId.empty())
            poolIds.push_back(zone.roundRobin->poolId);
    std::sort(poolIds.begin(), poolIds.end());
    poolIds.erase(std::unique(poolIds.begin(), poolIds.end()), poolIds.end());
    const auto poolIndex = makeIndex(poolIds);
    result.program.roundRobinPoolCount = static_cast<std::uint32_t>(poolIds.size());

    for (const auto& articulation : authoring.articulations)
    {
        if (!articulation.activation.has_value()) continue;
        const auto index = articulationIndex.find(articulation.id);
        if (index == articulationIndex.end())
        {
            result.issues.push_back("Performance compiler could not resolve articulation '" + articulation.id + "'.");
            continue;
        }
        const auto note = articulation.activation->midiNote;
        if (note < 0 || note > 127 || result.program.activationByMidiNote[static_cast<std::size_t>(note)].articulationIndex != kInvalidPerformanceProgramIndex)
        {
            result.issues.push_back("Performance compiler found an invalid or duplicate activation note.");
            continue;
        }
        result.program.activationByMidiNote[static_cast<std::size_t>(note)] = { index->second, articulation.activation->consume };
    }

    std::vector<std::size_t> zoneOrder(authoring.zones.size());
    for (std::size_t index = 0; index < zoneOrder.size(); ++index) zoneOrder[index] = index;
    std::sort(zoneOrder.begin(), zoneOrder.end(), [&](const auto left, const auto right)
    {
        const auto& leftZone = authoring.zones[left];
        const auto& rightZone = authoring.zones[right];
        if (leftZone.performance.event != rightZone.performance.event)
            return eventIndex(leftZone.performance.event) < eventIndex(rightZone.performance.event);
        return leftZone.id < rightZone.id;
    });
    result.program.triggerRoutes.reserve(zoneOrder.size());
    for (const auto zoneIndex : zoneOrder)
    {
        const auto& zone = authoring.zones[zoneIndex];
        const auto articulation = articulationIndex.find(zone.articulationId);
        if (articulation == articulationIndex.end())
        {
            result.issues.push_back("Performance compiler could not resolve zone articulation '" + zone.articulationId + "'.");
            continue;
        }
        CompiledPerformanceTriggerRoute route;
        route.zoneIndex = static_cast<std::uint32_t>(zoneIndex);
        route.articulationIndex = articulation->second;
        route.event = zone.performance.event;
        route.sustain = zone.performance.sustain;
        route.pitchSource = zone.performance.pitchSource;
        route.chokeReleaseSeconds = static_cast<float>(zone.chokeReleaseSeconds.value_or(0.0));
        if (!zone.exclusiveGroupId.empty())
        {
            const auto group = groupIndex.find(zone.exclusiveGroupId);
            if (group == groupIndex.end()) result.issues.push_back("Performance compiler could not resolve exclusive group.");
            else route.exclusiveGroupIndex = group->second;
        }
        for (const auto& targetId : zone.exclusiveTargetGroupIds)
        {
            const auto target = groupIndex.find(targetId);
            if (target == groupIndex.end() || target->second >= 64)
                result.issues.push_back("Performance compiler could not resolve choke target '" + targetId + "'.");
            else route.chokeTargetMask |= (std::uint64_t { 1 } << target->second);
        }
        result.program.triggerRoutes.push_back(route);
    }

    for (std::size_t event = 0, first = 0; event < result.program.eventRanges.size(); ++event)
    {
        auto& range = result.program.eventRanges[event];
        range.firstRoute = static_cast<std::uint32_t>(first);
        while (first < result.program.triggerRoutes.size()
               && eventIndex(result.program.triggerRoutes[first].event) == event)
            ++first;
        range.routeCount = static_cast<std::uint32_t>(first - range.firstRoute);
    }

    for (const auto& reset : authoring.roundRobinResetRules)
    {
        CompiledPerformanceRoundRobinReset action;
        action.event = reset.event;
        if (!reset.targetAll)
        {
            const auto pool = poolIndex.find(reset.targetPoolId);
            if (pool == poolIndex.end())
            {
                result.issues.push_back("Performance compiler could not resolve Round Robin pool '" + reset.targetPoolId + "'.");
                continue;
            }
            action.targetPoolIndex = pool->second;
        }
        result.program.roundRobinResets.push_back(action);
    }

    result.program.retainedBytes = sizeof(CompiledPerformanceProgram)
        + result.program.triggerRoutes.size() * sizeof(CompiledPerformanceTriggerRoute)
        + result.program.roundRobinResets.size() * sizeof(CompiledPerformanceRoundRobinReset)
        + result.program.articulationStableIds.size() * sizeof(std::uint64_t)
        + result.program.exclusiveGroupStableIds.size() * sizeof(std::uint64_t)
        + result.program.zoneArticulationIndices.size() * sizeof(std::uint32_t);
    result.compiled = result.issues.empty();
    return result;
}

std::string serializeCompiledPerformanceProgram(const CompiledPerformanceProgram& program)
{
    nlohmann::ordered_json root;
    root["articulationCount"] = program.articulationCount;
    root["defaultArticulationIndex"] = program.defaultArticulationIndex;
    root["exclusiveGroupCount"] = program.exclusiveGroupCount;
    root["roundRobinPoolCount"] = program.roundRobinPoolCount;
    root["retainedBytes"] = program.retainedBytes;
    nlohmann::ordered_json ranges = nlohmann::ordered_json::array();
    for (const auto& range : program.eventRanges) ranges.push_back({ { "firstRoute", range.firstRoute }, { "routeCount", range.routeCount } });
    root["eventRanges"] = std::move(ranges);
    nlohmann::ordered_json activations = nlohmann::ordered_json::array();
    for (std::size_t note = 0; note < program.activationByMidiNote.size(); ++note)
    {
        const auto& activation = program.activationByMidiNote[note];
        if (activation.articulationIndex != kInvalidPerformanceProgramIndex)
            activations.push_back({ { "midiNote", note }, { "articulationIndex", activation.articulationIndex }, { "consume", activation.consume } });
    }
    root["activations"] = std::move(activations);
    root["articulationStableIds"] = program.articulationStableIds;
    root["exclusiveGroupStableIds"] = program.exclusiveGroupStableIds;
    root["zoneArticulationIndices"] = program.zoneArticulationIndices;
    nlohmann::ordered_json routes = nlohmann::ordered_json::array();
    for (const auto& route : program.triggerRoutes)
        routes.push_back({ { "zoneIndex", route.zoneIndex }, { "articulationIndex", route.articulationIndex },
                           { "exclusiveGroupIndex", route.exclusiveGroupIndex }, { "chokeTargetMask", route.chokeTargetMask },
                           { "chokeReleaseSeconds", route.chokeReleaseSeconds }, { "event", static_cast<int>(route.event) },
                           { "sustain", static_cast<int>(route.sustain) }, { "pitchSource", static_cast<int>(route.pitchSource) } });
    root["triggerRoutes"] = std::move(routes);
    nlohmann::ordered_json resets = nlohmann::ordered_json::array();
    for (const auto& reset : program.roundRobinResets)
        resets.push_back({ { "event", static_cast<int>(reset.event) }, { "targetPoolIndex", reset.targetPoolIndex } });
    root["roundRobinResets"] = std::move(resets);
    return root.dump();
}
} // namespace drs::engine
