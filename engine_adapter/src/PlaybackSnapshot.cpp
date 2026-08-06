#include "drs/engine/PlaybackSnapshot.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using ordered_json = nlohmann::ordered_json;

void addFinding(PlaybackSnapshotBuildResult& result,
                const PlaybackSnapshotFindingSeverity severity,
                const std::string& code,
                const std::string& path,
                const std::string& message)
{
    result.findings.push_back({ severity, code, path, message });
}

std::size_t countFindings(const PlaybackSnapshotBuildResult& result, PlaybackSnapshotFindingSeverity severity)
{
    return static_cast<std::size_t>(std::count_if(result.findings.begin(),
                                                  result.findings.end(),
                                                  [severity](const PlaybackSnapshotFinding& finding)
                                                  {
                                                      return finding.severity == severity;
                                                  }));
}

bool containsValue(const std::vector<std::string>& values, const std::string& value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

void retainZoneId(std::vector<std::string>& retainedZoneIds,
                  const std::string& zoneId,
                  bool& changed)
{
    if (zoneId.empty() || containsValue(retainedZoneIds, zoneId))
        return;

    retainedZoneIds.push_back(zoneId);
    changed = true;
}

bool keyRangesOverlap(const PlaybackSnapshotZone& left,
                      const PlaybackSnapshotZone& right) noexcept
{
    return left.keyLow <= right.keyHigh && right.keyLow <= left.keyHigh;
}

bool hasChokeDependency(const PlaybackSnapshotZone& left,
                        const PlaybackSnapshotZone& right)
{
    return (!right.exclusiveGroupId.empty()
            && containsValue(left.exclusiveTargetGroupIds, right.exclusiveGroupId))
        || (!left.exclusiveGroupId.empty()
            && containsValue(right.exclusiveTargetGroupIds, left.exclusiveGroupId));
}

void expandPreparationDependencies(const ImmutablePlaybackSnapshot& snapshot,
                                   std::vector<std::string>& retainedZoneIds)
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto& zone : snapshot.zones)
        {
            if (!containsValue(retainedZoneIds, zone.id))
                continue;

            retainZoneId(retainedZoneIds,
                         zone.velocityCrossfadeRuntime.fadeInNeighborZoneId,
                         changed);
            retainZoneId(retainedZoneIds,
                         zone.velocityCrossfadeRuntime.fadeOutNeighborZoneId,
                         changed);

            for (const auto& candidate : snapshot.zones)
            {
                const auto roundRobinPeer = zone.roundRobin.has_value()
                    && candidate.roundRobin.has_value()
                    && !zone.roundRobin->poolId.empty()
                    && candidate.roundRobin->poolId == zone.roundRobin->poolId;
                const auto releaseTrigger = candidate.performance.event == PerformanceEventKind::release
                    && candidate.articulationId == zone.articulationId
                    && keyRangesOverlap(candidate, zone);
                if (roundRobinPeer || releaseTrigger || hasChokeDependency(zone, candidate))
                    retainZoneId(retainedZoneIds, candidate.id, changed);
            }
        }
    }
}

CompiledPerformanceProgram remapPerformanceProgram(
    const CompiledPerformanceProgram& source,
    const std::vector<std::size_t>& retainedZoneIndices,
    const std::unordered_map<std::size_t, std::size_t>& zoneIndexMap)
{
    auto scoped = source;
    scoped.zoneArticulationIndices.clear();
    if (!source.zoneArticulationIndices.empty())
    {
        scoped.zoneArticulationIndices.reserve(retainedZoneIndices.size());
        for (const auto retainedIndex : retainedZoneIndices)
        {
            scoped.zoneArticulationIndices.push_back(
                retainedIndex < source.zoneArticulationIndices.size()
                    ? source.zoneArticulationIndices[retainedIndex]
                    : kInvalidPerformanceProgramIndex);
        }
    }

    scoped.triggerRoutes.clear();
    scoped.triggerRoutes.reserve(source.triggerRoutes.size());
    for (const auto& route : source.triggerRoutes)
    {
        const auto mapped = zoneIndexMap.find(route.zoneIndex);
        if (mapped == zoneIndexMap.end())
            continue;

        auto retainedRoute = route;
        retainedRoute.zoneIndex = static_cast<std::uint32_t>(mapped->second);
        scoped.triggerRoutes.push_back(retainedRoute);
    }

    for (std::size_t event = 0, first = 0; event < scoped.eventRanges.size(); ++event)
    {
        auto& range = scoped.eventRanges[event];
        range.firstRoute = static_cast<std::uint32_t>(first);
        while (first < scoped.triggerRoutes.size()
               && static_cast<std::size_t>(scoped.triggerRoutes[first].event) == event)
        {
            ++first;
        }
        range.routeCount = static_cast<std::uint32_t>(first - range.firstRoute);
    }

    scoped.retainedBytes = sizeof(CompiledPerformanceProgram)
        + scoped.triggerRoutes.size() * sizeof(CompiledPerformanceTriggerRoute)
        + scoped.roundRobinResets.size() * sizeof(CompiledPerformanceRoundRobinReset)
        + scoped.articulationStableIds.size() * sizeof(std::uint64_t)
        + scoped.exclusiveGroupStableIds.size() * sizeof(std::uint64_t)
        + scoped.roundRobinPoolStableIds.size() * sizeof(std::uint64_t)
        + scoped.zoneArticulationIndices.size() * sizeof(std::uint32_t);
    return scoped;
}

template <typename Route>
void retainScopedRoutes(std::vector<Route>& routes,
                        const std::vector<std::string>& retainedZoneIds)
{
    for (auto iterator = routes.begin(); iterator != routes.end();)
    {
        iterator->zoneIds.erase(
            std::remove_if(iterator->zoneIds.begin(),
                           iterator->zoneIds.end(),
                           [&](const std::string& zoneId)
                           {
                               return !containsValue(retainedZoneIds, zoneId);
                           }),
            iterator->zoneIds.end());
        if (iterator->zoneIds.empty())
            iterator = routes.erase(iterator);
        else
            ++iterator;
    }
}

std::string normalizeAssetPath(const std::string& contentRootPath, const std::string& candidatePath)
{
    const fs::path path(candidatePath);
    if (path.is_absolute() || contentRootPath.empty())
        return path.lexically_normal().generic_string();

    return (fs::path(contentRootPath) / path).lexically_normal().generic_string();
}

std::string computeFnv1a64Hex(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    std::ostringstream stream;
    stream << std::hex;
    stream.width(16);
    stream.fill('0');
    stream << hash;
    return stream.str();
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();
    for (const auto& value : values)
        array.push_back(value);
    return array;
}

std::string buildGroupRoutingSourceId(const std::string& groupId)
{
    return groupId.empty() ? std::string {} : "groups/" + groupId;
}

bool isGroupRoutingSourceId(std::string_view sourceId) noexcept
{
    return sourceId.rfind("groups/", 0) == 0;
}

std::string extractGroupIdFromRoutingSourceId(std::string_view sourceId)
{
    if (!isGroupRoutingSourceId(sourceId) || sourceId.size() <= std::string_view("groups/").size())
        return {};

    return std::string(sourceId.substr(std::string_view("groups/").size()));
}

std::string extractZoneIdFromRoutingSourceId(std::string_view sourceId)
{
    constexpr std::string_view prefix { "zones/" };
    return sourceId.rfind(prefix, 0) == 0 && sourceId.size() > prefix.size()
        ? std::string(sourceId.substr(prefix.size())) : std::string {};
}

ordered_json serializeGroupRoute(const PlaybackSnapshotGroupRoute& route, bool includeWorkspaceVisible)
{
    ordered_json routeObject;
    routeObject["groupId"] = route.groupId;
    routeObject["articulationIds"] = serializeStringArray(route.articulationIds);
    routeObject["zoneIds"] = serializeStringArray(route.zoneIds);
    routeObject["displayName"] = route.displayName;
    routeObject["displayOrder"] = route.displayOrder;
    routeObject["routingSourceId"] = route.routingSourceId;
    if (includeWorkspaceVisible)
        routeObject["workspaceVisible"] = route.workspaceVisible;
    routeObject["gainDb"] = route.gainDb;
    routeObject["pan"] = route.pan;
    routeObject["routingBusId"] = route.routingBusId;
    routeObject["auditionAnchorZoneId"] = route.auditionAnchorZoneId;
    return routeObject;
}

ordered_json serializeVelocityCrossfade(const VelocityCrossfadeDescriptor& crossfade)
{
    return {
        { "fadeInLowVelocity", crossfade.fadeInLowVelocity },
        { "fadeInHighVelocity", crossfade.fadeInHighVelocity },
        { "fadeOutLowVelocity", crossfade.fadeOutLowVelocity },
        { "fadeOutHighVelocity", crossfade.fadeOutHighVelocity },
        { "curve", "linear" }
    };
}

ordered_json serializeVelocityCrossfadeRuntime(const VelocityCrossfadeRuntimeDescriptor& runtime)
{
    ordered_json value;
    value["effectiveLowVelocity"] = runtime.effectiveLowVelocity;
    value["effectiveHighVelocity"] = runtime.effectiveHighVelocity;

    if (!runtime.fadeInNeighborZoneId.empty())
        value["fadeInNeighborZoneId"] = runtime.fadeInNeighborZoneId;
    if (!runtime.fadeOutNeighborZoneId.empty())
        value["fadeOutNeighborZoneId"] = runtime.fadeOutNeighborZoneId;
    if (runtime.fadeInOverlapLowVelocity > 0)
        value["fadeInOverlapLowVelocity"] = runtime.fadeInOverlapLowVelocity;
    if (runtime.fadeInOverlapHighVelocity > 0)
        value["fadeInOverlapHighVelocity"] = runtime.fadeInOverlapHighVelocity;
    if (runtime.fadeOutOverlapLowVelocity > 0)
        value["fadeOutOverlapLowVelocity"] = runtime.fadeOutOverlapLowVelocity;
    if (runtime.fadeOutOverlapHighVelocity > 0)
        value["fadeOutOverlapHighVelocity"] = runtime.fadeOutOverlapHighVelocity;

    return value;
}

std::string toRoundRobinModeString(RoundRobinMode mode)
{
    switch (mode)
    {
        case RoundRobinMode::sequential:
            return "sequential";
        case RoundRobinMode::random:
            return "random";
    }

    return "sequential";
}

ordered_json serializeRoundRobin(const RoundRobinDescriptor& roundRobin)
{
    ordered_json value;
    value["poolId"] = roundRobin.poolId;
    value["slotCount"] = roundRobin.slotCount;
    value["slotIndex"] = roundRobin.slotIndex;
    value["mode"] = toRoundRobinModeString(roundRobin.mode);
    return value;
}

std::optional<RoundRobinDescriptor> materializeRoundRobinDescriptor(
    const RuntimeProjectZoneDefinition& zone)
{
    if (zone.roundRobin.has_value())
        return zone.roundRobin;

    if (zone.roundRobinLength <= 0 || zone.roundRobinPosition <= 0)
        return std::nullopt;

    std::ostringstream stream;
    stream << zone.groupId
           << "|"
           << zone.articulationId
           << "|"
           << zone.rootKey
           << "|"
           << zone.keyLow
           << "|"
           << zone.keyHigh
           << "|"
           << zone.roundRobinLength
           << "|"
           << static_cast<int>(zone.triggerMode);

    RoundRobinDescriptor roundRobin;
    roundRobin.poolId = "legacy-rr-" + computeFnv1a64Hex(stream.str());
    roundRobin.slotCount = zone.roundRobinLength;
    roundRobin.slotIndex = zone.roundRobinPosition;
    roundRobin.mode = RoundRobinMode::sequential;
    return roundRobin;
}

std::string buildCrossfadeTopologyMessage(const std::string& zoneId,
                                          VelocityCrossfadeTopologyIssue issue)
{
    switch (issue)
    {
        case VelocityCrossfadeTopologyIssue::none:
            return {};
        case VelocityCrossfadeTopologyIssue::fadeInMissingPartner:
            return "Zone '" + zoneId + "' must resolve exactly one lower crossfade partner for velocityCrossfade fade-in.";
        case VelocityCrossfadeTopologyIssue::fadeInAmbiguousPartner:
            return "Zone '" + zoneId + "' matched multiple lower crossfade partners for velocityCrossfade fade-in.";
        case VelocityCrossfadeTopologyIssue::fadeOutMissingPartner:
            return "Zone '" + zoneId + "' must resolve exactly one upper crossfade partner for velocityCrossfade fade-out.";
        case VelocityCrossfadeTopologyIssue::fadeOutAmbiguousPartner:
            return "Zone '" + zoneId + "' matched multiple upper crossfade partners for velocityCrossfade fade-out.";
        case VelocityCrossfadeTopologyIssue::roundRobinDuplicateSlot:
            return "Zone '" + zoneId + "' duplicates a Round Robin slot within one crossfade layer.";
        case VelocityCrossfadeTopologyIssue::roundRobinIncompletePool:
            return "Zone '" + zoneId + "' belongs to a Round Robin pool with incomplete slot coverage.";
        case VelocityCrossfadeTopologyIssue::roundRobinMixedSlotCount:
            return "Zone '" + zoneId + "' belongs to a Round Robin pool with mixed slot counts.";
    }

    return "Zone '" + zoneId + "' produced an unknown velocityCrossfade topology issue.";
}

std::vector<VelocityCrossfadeRuntimeDescriptor> buildSnapshotCrossfadeRuntimeDescriptors(
    const RuntimeProjectModel& project)
{
    std::vector<VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(project.authoring.zones.size());

    for (const auto& zone : project.authoring.zones)
    {
        const auto roundRobin = materializeRoundRobinDescriptor(zone);
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = computeVelocityCrossfadePairingKey(zone.articulationId,
                                                                      zone.rootKey,
                                                                      zone.keyLow,
                                                                      zone.keyHigh,
                                                                      static_cast<int>(zone.triggerMode));
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = roundRobin.has_value() ? roundRobin->poolId : std::string {};
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        topologyZones.push_back(topologyZone);
    }

    const auto runtimeTopology = buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones);
    std::vector<VelocityCrossfadeRuntimeDescriptor> descriptors(project.authoring.zones.size());

    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        const auto& zone = project.authoring.zones[index];
        if (!hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            continue;

        const auto& topology = runtimeTopology[index];
        auto& descriptor = descriptors[index];
        descriptor.effectiveLowVelocity = topology.effectiveLowVelocity;
        descriptor.effectiveHighVelocity = topology.effectiveHighVelocity;
        descriptor.fadeInOverlapLowVelocity = topology.fadeInOverlapLowVelocity;
        descriptor.fadeInOverlapHighVelocity = topology.fadeInOverlapHighVelocity;
        descriptor.fadeOutOverlapLowVelocity = topology.fadeOutOverlapLowVelocity;
        descriptor.fadeOutOverlapHighVelocity = topology.fadeOutOverlapHighVelocity;

        if (topology.fadeInNeighborZoneIndex >= 0)
        {
            descriptor.fadeInNeighborZoneId =
                project.authoring.zones[static_cast<std::size_t>(topology.fadeInNeighborZoneIndex)].id;
        }

        if (topology.fadeOutNeighborZoneIndex >= 0)
        {
            descriptor.fadeOutNeighborZoneId =
                project.authoring.zones[static_cast<std::size_t>(topology.fadeOutNeighborZoneIndex)].id;
        }
    }

    return descriptors;
}

ordered_json serializeSnapshot(const ImmutablePlaybackSnapshot& snapshot, bool includeDigest)
{
    ordered_json root;
    root["schemaName"] = snapshot.schemaName;
    root["schemaVersion"] = snapshot.schemaVersion;
    root["projectId"] = snapshot.projectId;
    root["displayName"] = snapshot.displayName;
    root["sourceProjectSchemaName"] = snapshot.sourceProjectSchemaName;
    root["sourceProjectSchemaVersion"] = snapshot.sourceProjectSchemaVersion;
    root["sourceAuthoringSchemaName"] = snapshot.sourceAuthoringSchemaName;
    root["sourceAuthoringSchemaVersion"] = snapshot.sourceAuthoringSchemaVersion;
    root["draftRevision"] = snapshot.draftRevision;
    root["selectedZoneId"] = snapshot.selectedZoneId;
    root["selectedGroupId"] = snapshot.selectedGroupId;
    root["selectedPerformanceBankId"] = snapshot.selectedPerformanceBankId;
    root["masterGainDb"] = snapshot.masterGainDb;

    if (includeDigest)
        root["contentDigest"] = snapshot.contentDigest;
    if (includeDigest)
        root["dspGraphDigest"] = snapshot.dspGraphDigest;

    ordered_json sampleIdentities = ordered_json::array();
    for (const auto& sample : snapshot.sampleIdentities)
    {
        ordered_json sampleObject;
        sampleObject["sampleSourceId"] = sample.sampleSourceId;
        sampleObject["sourcePath"] = sample.sourcePath;
        sampleObject["role"] = sample.role;
        sampleIdentities.push_back(std::move(sampleObject));
    }
    root["sampleIdentities"] = std::move(sampleIdentities);

    ordered_json macros = ordered_json::array();
    for (const auto& macro : snapshot.macroDefaults)
    {
        ordered_json macroObject;
        macroObject["id"] = macro.id;
        macroObject["name"] = macro.name;
        macroObject["defaultValue"] = macro.defaultValue;
        macroObject["minValue"] = macro.minValue;
        macroObject["maxValue"] = macro.maxValue;
        macroObject["exposedInPerformance"] = macro.exposedInPerformance;

        ordered_json targets = ordered_json::array();
        for (const auto& target : macro.targets)
        {
            ordered_json targetObject;
            targetObject["parameterId"] = target.parameterId;
            targetObject["parameterPath"] = target.parameterPath;
            targetObject["role"] = target.role;
            targetObject["dspSlotId"] = target.dspSlotId;
            targetObject["dspParameterId"] = target.dspParameterId;
            targetObject["sourceMinimum"] = target.sourceMinimum;
            targetObject["sourceMaximum"] = target.sourceMaximum;
            targetObject["destinationMinimum"] = target.destinationMinimum;
            targetObject["destinationMaximum"] = target.destinationMaximum;
            if (!target.controlLaw.id.empty())
            {
                targetObject["controlLaw"] = {
                    { "id", target.controlLaw.id },
                    { "version", target.controlLaw.version }
                };
            }
            targetObject["curve"] = target.curve;
            targets.push_back(std::move(targetObject));
        }

        macroObject["targets"] = std::move(targets);
        macros.push_back(std::move(macroObject));
    }
    root["macroDefaults"] = std::move(macros);

    ordered_json fxSlots = ordered_json::array();
    for (const auto& fxSlot : snapshot.fxSlots)
    {
        ordered_json fxObject;
        fxObject["id"] = fxSlot.id;
        fxObject["displayName"] = fxSlot.displayName;
        fxObject["effectType"] = fxSlot.effectType;
        fxObject["bypassed"] = fxSlot.bypassed;
        fxObject["effectVersion"] = fxSlot.effectVersion;
        fxObject["unavailable"] = fxSlot.unavailable;
        fxObject["legacyInert"] = fxSlot.legacyInert;
        fxObject["catalogResolved"] = fxSlot.catalogResolved;
        fxObject["costStateBytes"] = fxSlot.cost.stateBytes;
        fxObject["costScratchBytes"] = fxSlot.cost.scratchBytes;
        fxObject["costMaximumTailFrames"] = fxSlot.cost.maximumTailFrames;
        fxObject["costUnits"] = fxSlot.cost.costUnits;
        ordered_json parameters = ordered_json::array();
        for (const auto& parameter : fxSlot.parameters)
            parameters.push_back({ { "id", parameter.id }, { "value", parameter.value } });
        fxObject["parameters"] = std::move(parameters);
        fxSlots.push_back(std::move(fxObject));
    }
    root["fxSlots"] = std::move(fxSlots);

    ordered_json routingBuses = ordered_json::array();
    for (const auto& routingBus : snapshot.routingBuses)
    {
        ordered_json busObject;
        busObject["id"] = routingBus.id;
        busObject["displayName"] = routingBus.displayName;
        busObject["inputSourceId"] = routingBus.inputSourceId;
        busObject["fxSlotIds"] = serializeStringArray(routingBus.fxSlotIds);
        busObject["chainBypassed"] = routingBus.chainBypassed;
        routingBuses.push_back(std::move(busObject));
    }
    root["routingBuses"] = std::move(routingBuses);

    ordered_json articulationRoutes = ordered_json::array();
    for (const auto& route : snapshot.articulationRoutes)
    {
        ordered_json routeObject;
        routeObject["articulationId"] = route.articulationId;
        routeObject["zoneIds"] = serializeStringArray(route.zoneIds);
        articulationRoutes.push_back(std::move(routeObject));
    }
    root["articulationRoutes"] = std::move(articulationRoutes);

    ordered_json articulationDefinitions = ordered_json::array();
    for (const auto& articulation : snapshot.articulationDefinitions)
    {
        ordered_json value;
        value["id"] = articulation.id;
        value["displayName"] = articulation.displayName;
        value["isDefault"] = articulation.isDefault;
        value["displayOrder"] = articulation.displayOrder;
        if (articulation.activation.has_value())
        {
            value["activation"] = {
                { "event", static_cast<int>(articulation.activation->event) },
                { "midiNote", articulation.activation->midiNote },
                { "mode", static_cast<int>(articulation.activation->mode) },
                { "consume", articulation.activation->consume }
            };
        }
        articulationDefinitions.push_back(std::move(value));
    }
    root["articulationDefinitions"] = std::move(articulationDefinitions);

    ordered_json groupRoutes = ordered_json::array();
    for (const auto& route : snapshot.groupRoutes)
        groupRoutes.push_back(serializeGroupRoute(route, includeDigest));
    root["groupRoutes"] = std::move(groupRoutes);

    ordered_json zones = ordered_json::array();
    for (const auto& zone : snapshot.zones)
    {
        ordered_json zoneObject;
        zoneObject["id"] = zone.id;
        zoneObject["sampleSourceId"] = zone.sampleSourceId;
        zoneObject["displayName"] = zone.displayName;
        zoneObject["groupId"] = zone.groupId;
        zoneObject["articulationId"] = zone.articulationId;
        zoneObject["rootKey"] = zone.rootKey;
        zoneObject["keyLow"] = zone.keyLow;
        zoneObject["keyHigh"] = zone.keyHigh;
        zoneObject["velocityLow"] = zone.velocityLow;
        zoneObject["velocityHigh"] = zone.velocityHigh;
        if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            zoneObject["velocityCrossfade"] = serializeVelocityCrossfade(zone.velocityCrossfade);
        if (hasAnyVelocityCrossfadeRuntimeValue(zone.velocityCrossfadeRuntime))
            zoneObject["velocityCrossfadeRuntime"] = serializeVelocityCrossfadeRuntime(zone.velocityCrossfadeRuntime);
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        zoneObject["releaseSeconds"] = zone.releaseSeconds;
        if (zone.roundRobin.has_value())
            zoneObject["roundRobin"] = serializeRoundRobin(*zone.roundRobin);
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            zoneObject["triggerMode"] = "one-shot";
        zoneObject["performance"] = {
            { "event", static_cast<int>(zone.performance.event) },
            { "sustain", static_cast<int>(zone.performance.sustain) },
            { "pitchSource", static_cast<int>(zone.performance.pitchSource) }
        };
        if (!zone.exclusiveGroupId.empty()) zoneObject["exclusiveGroupId"] = zone.exclusiveGroupId;
        if (!zone.exclusiveTargetGroupIds.empty()) zoneObject["exclusiveTargetGroupIds"] = serializeStringArray(zone.exclusiveTargetGroupIds);
        if (zone.chokeReleaseSeconds.has_value()) zoneObject["chokeReleaseSeconds"] = *zone.chokeReleaseSeconds;
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);
    ordered_json resetRules = ordered_json::array();
    for (const auto& rule : snapshot.roundRobinResetRules)
        resetRules.push_back({ { "event", static_cast<int>(rule.event) }, { "targetAll", rule.targetAll },
                               { "targetPoolId", rule.targetPoolId } });
    root["roundRobinResetRules"] = std::move(resetRules);
    root["performanceProgram"] = nlohmann::ordered_json::parse(serializeCompiledPerformanceProgram(snapshot.performanceProgram));
    root["notes"] = serializeStringArray(snapshot.notes);
    return root;
}

ordered_json serializeDspGraph(const ImmutablePlaybackSnapshot& snapshot)
{
    ordered_json root;
    ordered_json slots = ordered_json::array();
    for (const auto& slot : snapshot.fxSlots)
    {
        ordered_json value;
        value["id"] = slot.id;
        value["effectType"] = slot.effectType;
        value["effectVersion"] = slot.effectVersion;
        value["bypassed"] = slot.bypassed;
        value["unavailable"] = slot.unavailable;
        value["legacyInert"] = slot.legacyInert;
        ordered_json parameters = ordered_json::array();
        for (const auto& parameter : slot.parameters)
            parameters.push_back({ { "id", parameter.id }, { "value", parameter.value } });
        value["parameters"] = std::move(parameters);
        slots.push_back(std::move(value));
    }
    root["slots"] = std::move(slots);
    ordered_json chains = ordered_json::array();
    for (const auto& bus : snapshot.routingBuses)
        chains.push_back({ { "id", bus.id }, { "inputSourceId", bus.inputSourceId },
                           { "fxSlotIds", serializeStringArray(bus.fxSlotIds) },
                           { "chainBypassed", bus.chainBypassed } });
    root["chains"] = std::move(chains);
    return root;
}
} // namespace

PlaybackSnapshotBuildRequest PlaybackSnapshotBuilder::requestBuild(std::size_t draftRevision,
                                                                  bool activationRequested)
{
    PlaybackSnapshotBuildRequest request;
    request.accepted = true;
    request.buildId = nextBuildId++;
    request.cancellationId = request.buildId;
    request.requestedDraftRevision = draftRevision;
    request.activationRequested = activationRequested;
    request.lifecycleState = PlaybackSnapshotLifecycleState::preparing;
    request.state = "Snapshot build queued";
    return request;
}

PlaybackSnapshotBuildResult PlaybackSnapshotBuilder::buildSnapshot(const PlaybackSnapshotBuildRequest& request,
                                                                  const RuntimeProjectModel& project) const
{
    PlaybackSnapshotBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = request.accepted
        ? PlaybackSnapshotLifecycleState::preparing
        : PlaybackSnapshotLifecycleState::idle;
    result.state = request.accepted ? "Snapshot build in progress" : "Snapshot build request rejected";

    const auto startTime = Clock::now();
    if (!request.accepted)
    {
        result.buildDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
        return result;
    }

    result.snapshot.schemaName = "drs.playbackSnapshot";
    result.snapshot.schemaVersion = 1;
    result.snapshot.projectId = project.projectId;
    result.snapshot.displayName = project.displayName;
    result.snapshot.sourceProjectSchemaName = project.schemaName;
    result.snapshot.sourceProjectSchemaVersion = project.schemaVersion;
    result.snapshot.sourceAuthoringSchemaName = project.authoring.schemaName;
    result.snapshot.sourceAuthoringSchemaVersion = project.authoring.schemaVersion;
    result.snapshot.draftRevision = request.requestedDraftRevision;
    result.snapshot.selectedZoneId = project.authoring.selectedZoneId;
    result.snapshot.selectedGroupId = project.authoring.selectedGroupId;
    result.snapshot.selectedPerformanceBankId = project.authoring.selectedPerformanceBankId;
    result.snapshot.masterGainDb = project.authoring.masterGainDb;
    result.snapshot.notes = project.notes;
    result.snapshot.notes.insert(result.snapshot.notes.end(),
                                 project.authoring.notes.begin(),
                                 project.authoring.notes.end());
    result.snapshot.articulationDefinitions.reserve(project.authoring.articulations.size());
    for (const auto& articulation : project.authoring.articulations)
        result.snapshot.articulationDefinitions.push_back({ articulation.id, articulation.displayName,
                                                             articulation.isDefault, articulation.displayOrder,
                                                             articulation.activation });
    result.snapshot.roundRobinResetRules = project.authoring.roundRobinResetRules;

    std::unordered_map<std::string, std::size_t> sampleIndices;
    result.snapshot.sampleIdentities.reserve(project.sampleSources.size());
    for (std::size_t index = 0; index < project.sampleSources.size(); ++index)
    {
        const auto& sampleSource = project.sampleSources[index];
        const auto path = "sampleSources[" + std::to_string(index) + "]";

        if (sampleSource.id.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-sample-source-id", path + ".id",
                       "Sample source ids must be non-empty.");
        else if (!sampleIndices.emplace(sampleSource.id, result.snapshot.sampleIdentities.size()).second)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-sample-source-id", path + ".id",
                       "Sample source id '" + sampleSource.id + "' is duplicated.");

        const auto normalizedPath = normalizeAssetPath(project.contentRootPath, sampleSource.path);
        if (sampleSource.path.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-sample-source-path", path + ".path",
                       "Sample source paths must be non-empty.");
        else if (!fs::exists(fs::path(normalizedPath)))
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-sample-source-asset", path + ".path",
                       "Sample source asset '" + normalizedPath + "' does not exist.");

        PlaybackSnapshotSampleIdentity identity;
        identity.sampleSourceId = sampleSource.id;
        identity.sourcePath = normalizedPath;
        identity.role = sampleSource.role;
        result.snapshot.sampleIdentities.push_back(std::move(identity));
    }

    std::unordered_map<std::string, std::size_t> macroIndices;
    result.snapshot.macroDefaults.reserve(project.authoring.macros.size());
    for (std::size_t index = 0; index < project.authoring.macros.size(); ++index)
    {
        const auto& macro = project.authoring.macros[index];
        const auto path = "authoring.macros[" + std::to_string(index) + "]";

        if (macro.id.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-macro-id", path + ".id",
                       "Macro ids must be non-empty.");
        else if (!macroIndices.emplace(macro.id, index).second)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-macro-id", path + ".id",
                       "Macro id '" + macro.id + "' is duplicated.");

        if (macro.minValue > macro.maxValue)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-macro-range", path,
                       "Macro minValue must not exceed maxValue.");

        if (macro.defaultValue < macro.minValue || macro.defaultValue > macro.maxValue)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "macro-default-out-of-range", path + ".defaultValue",
                       "Macro defaultValue must stay within the declared min/max range.");

        PlaybackSnapshotMacroDefault snapshotMacro;
        snapshotMacro.id = macro.id;
        snapshotMacro.name = macro.name;
        snapshotMacro.defaultValue = macro.defaultValue;
        snapshotMacro.minValue = macro.minValue;
        snapshotMacro.maxValue = macro.maxValue;
        snapshotMacro.exposedInPerformance = macro.exposedInPerformance;
        snapshotMacro.targets.reserve(macro.targets.size());

        for (const auto& target : macro.targets)
            snapshotMacro.targets.push_back({ target.parameterId, target.parameterPath, target.role,
                                              target.dspSlotId, target.dspParameterId,
                                              target.sourceMinimum, target.sourceMaximum,
                                              target.destinationMinimum, target.destinationMaximum,
                                              target.curve, target.controlLaw });

        result.snapshot.macroDefaults.push_back(std::move(snapshotMacro));
    }

    std::unordered_map<std::string, std::size_t> fxSlotIndices;
    result.snapshot.fxSlots.reserve(project.authoring.fxSlots.size());
    for (std::size_t index = 0; index < project.authoring.fxSlots.size(); ++index)
    {
        const auto& fxSlot = project.authoring.fxSlots[index];
        const auto path = "authoring.fxSlots[" + std::to_string(index) + "]";

        if (fxSlot.id.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-fx-slot-id", path + ".id",
                       "FX slot ids must be non-empty.");
        else if (!fxSlotIndices.emplace(fxSlot.id, index).second)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-fx-slot-id", path + ".id",
                       "FX slot id '" + fxSlot.id + "' is duplicated.");

        PlaybackSnapshotFxSlotReference snapshotSlot;
        snapshotSlot.id = fxSlot.id;
        snapshotSlot.displayName = fxSlot.displayName;
        snapshotSlot.effectType = fxSlot.effectType;
        snapshotSlot.bypassed = fxSlot.bypassed;
        snapshotSlot.effectVersion = fxSlot.effectVersion;
        snapshotSlot.unavailable = fxSlot.unavailable;
        snapshotSlot.legacyInert = fxSlot.legacyInert;
        if (const auto* catalogEffect = findCuratedDspEffect(fxSlot.effectType, fxSlot.effectVersion))
        {
            snapshotSlot.catalogResolved = true;
            snapshotSlot.supportedScopes = catalogEffect->supportedScopes;
            snapshotSlot.stateClass = catalogEffect->stateClass;
            snapshotSlot.cost = catalogEffect->cost;
        }
        snapshotSlot.parameters.reserve(fxSlot.parameters.size());
        for (const auto& parameter : fxSlot.parameters)
            snapshotSlot.parameters.push_back({ parameter.id, parameter.value });
        result.snapshot.fxSlots.push_back(std::move(snapshotSlot));
    }

    const auto usesExplicitGroupDefinitions = project.schemaVersion >= 4
        && project.authoring.schemaVersion >= 3;

    std::unordered_set<std::string> authoredZoneIds;
    authoredZoneIds.reserve(project.authoring.zones.size());
    for (const auto& zone : project.authoring.zones)
    {
        if (!zone.id.empty())
            authoredZoneIds.insert(zone.id);
    }

    std::unordered_set<std::string> authoredGroupIds;
    if (usesExplicitGroupDefinitions)
    {
        authoredGroupIds.reserve(project.authoring.groups.size());
        for (const auto& group : project.authoring.groups)
        {
            if (!group.id.empty())
                authoredGroupIds.insert(group.id);
        }
    }

    std::unordered_map<std::string, std::size_t> routingBusIndices;
    result.snapshot.routingBuses.reserve(project.authoring.routingBuses.size());
    for (std::size_t index = 0; index < project.authoring.routingBuses.size(); ++index)
    {
        const auto& routingBus = project.authoring.routingBuses[index];
        const auto path = "authoring.routingBuses[" + std::to_string(index) + "]";

        if (routingBus.id.empty())
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-routing-bus-id", path + ".id",
                       "Routing bus ids must be non-empty.");
        }
        else if (!routingBusIndices.emplace(routingBus.id, index).second)
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-routing-bus-id", path + ".id",
                       "Routing bus id '" + routingBus.id + "' is duplicated.");
        }

        if (routingBus.inputSourceId.empty())
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "missing-routing-input-source",
                       path + ".inputSourceId",
                       "Routing buses must declare an input source.");
        }
        else if (routingBus.inputSourceId != "master" && !authoredZoneIds.count(routingBus.inputSourceId)
                 && !authoredZoneIds.count(extractZoneIdFromRoutingSourceId(routingBus.inputSourceId)))
        {
            const auto groupId = extractGroupIdFromRoutingSourceId(routingBus.inputSourceId);
            if (!groupId.empty() && usesExplicitGroupDefinitions && authoredGroupIds.count(groupId))
            {
                // Group input sources are legal in Sprint 4 when they resolve to an authored group.
            }
            else
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           groupId.empty() ? "unknown-routing-input-source" : "unknown-group-routing-input-source",
                           path + ".inputSourceId",
                           groupId.empty()
                               ? "Routing bus '" + routingBus.id + "' references unknown input source '"
                                   + routingBus.inputSourceId + "'."
                               : "Routing bus '" + routingBus.id + "' references unknown group input source '"
                                   + routingBus.inputSourceId + "'.");
            }
        }

        for (const auto& fxSlotId : routingBus.fxSlotIds)
        {
            if (!fxSlotId.empty() && !fxSlotIndices.count(fxSlotId))
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "unknown-routing-fx-slot",
                           path + ".fxSlotIds",
                           "Routing bus '" + routingBus.id + "' references unknown FX slot '" + fxSlotId + "'.");
            }
        }

        result.snapshot.routingBuses.push_back({
            routingBus.id,
            routingBus.displayName,
            routingBus.inputSourceId,
            routingBus.fxSlotIds,
            routingBus.chainBypassed
        });
    }

    if (project.schemaVersion >= 5)
    {
        std::unordered_set<std::string> canonicalDspSources;
        std::unordered_map<std::string, std::size_t> dspSlotOwnerCounts;
        for (std::size_t index = 0; index < result.snapshot.routingBuses.size(); ++index)
        {
        auto& bus = result.snapshot.routingBuses[index];
        const auto path = "authoring.routingBuses[" + std::to_string(index) + "]";
        CuratedDspScope scope = CuratedDspScope::instrument;
        if (bus.inputSourceId != "master")
        {
            auto zoneId = extractZoneIdFromRoutingSourceId(bus.inputSourceId);
            if (zoneId.empty() && authoredZoneIds.count(bus.inputSourceId))
                zoneId = bus.inputSourceId;
            const auto groupId = extractGroupIdFromRoutingSourceId(bus.inputSourceId);
            if (!zoneId.empty() && authoredZoneIds.count(zoneId))
            {
                bus.inputSourceId = "zones/" + zoneId;
                scope = CuratedDspScope::zone;
            }
            else if (!groupId.empty() && authoredGroupIds.count(groupId))
            {
                bus.inputSourceId = "groups/" + groupId;
                scope = CuratedDspScope::group;
            }
            else
            {
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-invalid-owner-source",
                           path + ".inputSourceId", "DSP chain owner source cannot be canonically resolved.");
            }
        }
        if (!canonicalDspSources.insert(bus.inputSourceId).second)
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-duplicate-owner-source",
                       path + ".inputSourceId", "Every canonical DSP owner source must have exactly one chain.");
        }

        for (const auto& slotId : bus.fxSlotIds)
        {
            const auto slotIndex = fxSlotIndices.find(slotId);
            if (slotIndex == fxSlotIndices.end())
                continue;
            ++dspSlotOwnerCounts[slotId];
            const auto& slot = result.snapshot.fxSlots[slotIndex->second];
            const auto slotPath = "authoring.fxSlots[" + std::to_string(slotIndex->second) + "]";
            if (!slot.catalogResolved)
            {
                if (!slot.unavailable && !slot.legacyInert)
                    addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-unknown-catalog-version",
                               slotPath, "An unresolved DSP effect must be explicitly unavailable or legacy-inert.");
                continue;
            }
            if (std::find(slot.supportedScopes.begin(), slot.supportedScopes.end(), scope)
                == slot.supportedScopes.end())
            {
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-unsupported-scope",
                           slotPath, "The resolved DSP catalog effect is unsupported at this chain owner scope.");
            }
            std::unordered_set<std::string> parameterIds;
            const auto* catalogEffect = findCuratedDspEffect(slot.effectType, slot.effectVersion);
            for (const auto& parameter : slot.parameters)
            {
                if (!parameterIds.insert(parameter.id).second)
                    addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-duplicate-parameter",
                               slotPath + ".parameters", "DSP parameter IDs must be unique in a snapshot slot.");
                const auto descriptor = std::find_if(catalogEffect->parameters.begin(), catalogEffect->parameters.end(),
                                                     [&](const CuratedDspParameterDescriptor& candidate)
                                                     { return candidate.id == parameter.id; });
                if (descriptor == catalogEffect->parameters.end() || !std::isfinite(parameter.value)
                    || parameter.value < descriptor->minimum || parameter.value > descriptor->maximum)
                {
                    addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-invalid-parameter",
                               slotPath + ".parameters." + parameter.id,
                               "Catalog DSP parameters must be known, finite, and within their versioned range.");
                }
            }
        }
        }
        for (const auto& slot : result.snapshot.fxSlots)
            if (dspSlotOwnerCounts[slot.id] != 1)
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "snapshot-dsp-slot-owner-count",
                           "authoring.fxSlots", "Every DSP slot must have exactly one canonical chain owner.");
    }

    std::unordered_map<std::string, std::size_t> articulationRouteIndices;
    std::unordered_map<std::string, std::size_t> groupRouteIndices;
    std::unordered_map<std::string, std::size_t> zoneIndices;
    if (usesExplicitGroupDefinitions)
    {
        result.snapshot.groupRoutes.reserve(project.authoring.groups.size());
        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            const auto& group = project.authoring.groups[index];
            const auto path = "authoring.groups[" + std::to_string(index) + "]";

            if (group.id.empty())
            {
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-group-id", path + ".id",
                           "Group ids must be non-empty.");
            }
            else if (!groupRouteIndices.emplace(group.id, result.snapshot.groupRoutes.size()).second)
            {
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-group-id", path + ".id",
                           "Group id '" + group.id + "' is duplicated.");
            }

            if (group.displayName.empty())
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "missing-group-display-name",
                           path + ".displayName",
                           "Groups must declare a display name.");
            }

            if (group.displayOrder < 0)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "invalid-group-display-order",
                           path + ".displayOrder",
                           "Group displayOrder must not be negative.");
            }

            if (!group.routingBusId.empty() && !routingBusIndices.count(group.routingBusId))
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "unknown-group-routing-bus",
                           path + ".routingBusId",
                           "Group '" + group.id + "' references unknown routing bus '" + group.routingBusId + "'.");
            }

            PlaybackSnapshotGroupRoute route;
            route.groupId = group.id;
            route.displayName = group.displayName;
            route.displayOrder = group.displayOrder;
            route.routingSourceId = buildGroupRoutingSourceId(group.id);
            route.workspaceVisible = group.workspaceVisible;
            route.gainDb = group.gainDb;
            route.pan = group.pan;
            route.routingBusId = group.routingBusId;
            route.auditionAnchorZoneId = group.auditionAnchorZoneId;
            result.snapshot.groupRoutes.push_back(std::move(route));
        }

        std::unordered_set<std::string> claimedRoutingBusIds;
        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            const auto& group = project.authoring.groups[index];
            const auto path = "authoring.groups[" + std::to_string(index) + "]";
            if (group.routingBusId.empty())
                continue;

            const auto busIndex = routingBusIndices.find(group.routingBusId);
            if (busIndex == routingBusIndices.end())
                continue;

            const auto& bus = project.authoring.routingBuses[busIndex->second];
            const auto expectedSourceId = buildGroupRoutingSourceId(group.id);
            if (bus.inputSourceId != expectedSourceId)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "mismatched-group-routing-bus-source",
                           path + ".routingBusId",
                           "Group '" + group.id + "' must reference a routing bus sourced from '"
                               + expectedSourceId + "'.");
            }
            else if (!claimedRoutingBusIds.insert(group.routingBusId).second)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "duplicate-group-routing-bus-assignment",
                           path + ".routingBusId",
                           "Routing bus '" + group.routingBusId
                               + "' is already assigned to another group route.");
            }
        }

        for (std::size_t index = 0; index < project.authoring.routingBuses.size(); ++index)
        {
            const auto& routingBus = project.authoring.routingBuses[index];
            const auto groupId = extractGroupIdFromRoutingSourceId(routingBus.inputSourceId);
            if (groupId.empty())
                continue;

            if (!claimedRoutingBusIds.count(routingBus.id))
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "orphaned-group-routing-bus",
                           "authoring.routingBuses[" + std::to_string(index) + "].inputSourceId",
                           "Routing bus '" + routingBus.id + "' targets group source '"
                               + routingBus.inputSourceId
                               + "' but no authored group claims that bus.");
            }
        }
    }

    if (usesExplicitGroupDefinitions && !project.authoring.zones.empty() && project.authoring.groups.empty())
    {
        addFinding(result,
                   PlaybackSnapshotFindingSeverity::error,
                   "missing-group-definitions",
                   "authoring.groups",
                   "Projects with playable zones must carry explicit authored groups in the immutable snapshot build.");
    }

    result.snapshot.zones.reserve(project.authoring.zones.size());
    const auto crossfadeRuntimeDescriptors = buildSnapshotCrossfadeRuntimeDescriptors(project);
    std::vector<VelocityCrossfadeTopologyZoneDefinition> crossfadeTopologyZones;
    crossfadeTopologyZones.reserve(project.authoring.zones.size());
    for (const auto& zone : project.authoring.zones)
    {
        const auto roundRobin = materializeRoundRobinDescriptor(zone);
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = computeVelocityCrossfadePairingKey(zone.articulationId,
                                                                      zone.rootKey,
                                                                      zone.keyLow,
                                                                      zone.keyHigh,
                                                                      static_cast<int>(zone.triggerMode));
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = roundRobin.has_value() ? roundRobin->poolId : std::string {};
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        crossfadeTopologyZones.push_back(topologyZone);
    }
    std::vector<VelocityCrossfadeTopologyFinding> crossfadeTopologyFindings;
    buildFirstPassVelocityCrossfadeRuntimeTopology(crossfadeTopologyZones, &crossfadeTopologyFindings);

    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        const auto& zone = project.authoring.zones[index];
        const auto path = "authoring.zones[" + std::to_string(index) + "]";
        const auto roundRobin = materializeRoundRobinDescriptor(zone);

        if (zone.id.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-zone-id", path + ".id",
                       "Zone ids must be non-empty.");
        else if (!zoneIndices.emplace(zone.id, index).second)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "duplicate-zone-id", path + ".id",
                       "Zone id '" + zone.id + "' is duplicated.");

        if (zone.sampleSourceId.empty())
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-zone-sample-source", path + ".sampleSourceId",
                       "Zones must reference a sample source.");
        }
        else if (!sampleIndices.count(zone.sampleSourceId))
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "unknown-zone-sample-source", path + ".sampleSourceId",
                       "Zone '" + zone.id + "' references unknown sample source '" + zone.sampleSourceId + "'.");
        }

        if (zone.groupId.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-zone-group-id", path + ".groupId",
                       "Zones must declare a group id.");

        if (zone.articulationId.empty())
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-zone-articulation-id", path + ".articulationId",
                       "Zones must declare an articulation id.");

        if (zone.keyLow > zone.keyHigh)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-key-range", path,
                       "Zone keyLow must not exceed keyHigh.");

        if (zone.velocityLow > zone.velocityHigh)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-velocity-range", path,
                       "Zone velocityLow must not exceed velocityHigh.");
        if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
        {
            const VelocityCrossfadeZoneDefinition crossfadeZone {
                zone.velocityLow,
                zone.velocityHigh,
                zone.velocityCrossfade
            };
            const auto crossfadeIssue = validateFirstPassVelocityCrossfadeZone(crossfadeZone);
            if (crossfadeIssue != VelocityCrossfadeZoneIssue::none)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "invalid-zone-velocity-crossfade",
                           path + ".velocityCrossfade",
                           "Zone '" + zone.id + "' carries unsupported velocityCrossfade metadata in the playback snapshot build.");
            }
        }

        if (zone.loopEnabled && zone.loopEndFrame < zone.loopStartFrame)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-loop-range", path,
                       "Loop-enabled zones must not declare loopEndFrame before loopStartFrame.");
        if (zone.releaseSeconds < 0.0)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-release", path,
                       "Zone releaseSeconds must not be negative.");
        if (zone.roundRobinLength < 0 || zone.roundRobinPosition < 0)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-round-robin", path,
                       "Zone round-robin metadata must not be negative.");
        if (zone.roundRobinPosition > 0 && zone.roundRobinLength <= 0)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "missing-zone-round-robin-length", path,
                       "Zone roundRobinPosition requires a positive roundRobinLength.");
        if (zone.roundRobinLength > 0
            && (zone.roundRobinPosition < 1 || zone.roundRobinPosition > zone.roundRobinLength))
        {
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-round-robin-position", path,
                       "Zone roundRobinPosition must stay within roundRobinLength.");
        }
        if (roundRobin.has_value())
        {
            if (roundRobin->poolId.empty())
            {
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-round-robin-pool-id", path,
                           "Zone Round Robin poolId must be non-empty when Round Robin metadata is present.");
            }
            if (roundRobin->slotCount != zone.roundRobinLength
                || roundRobin->slotIndex != zone.roundRobinPosition)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "invalid-zone-round-robin-shape",
                           path,
                           "Zone Round Robin descriptor must mirror the scalar Round Robin slot metadata.");
            }
        }

        result.snapshot.zones.push_back({
            zone.id,
            zone.sampleSourceId,
            zone.displayName,
            zone.groupId,
            zone.articulationId,
            zone.rootKey,
            zone.keyLow,
            zone.keyHigh,
            zone.velocityLow,
            zone.velocityHigh,
            zone.velocityCrossfade,
            crossfadeRuntimeDescriptors[index],
            zone.gainDb,
            zone.pan,
            zone.sampleStartFrame,
            zone.loopEnabled,
            zone.loopStartFrame,
            zone.loopEndFrame,
            zone.releaseSeconds,
            roundRobin,
            zone.roundRobinLength,
            zone.roundRobinPosition,
            zone.triggerMode,
            zone.performance,
            zone.exclusiveGroupId,
            zone.exclusiveTargetGroupIds,
            zone.chokeReleaseSeconds
        });

        if (!zone.articulationId.empty())
        {
            const auto articulationIterator = articulationRouteIndices.find(zone.articulationId);
            if (articulationIterator == articulationRouteIndices.end())
            {
                PlaybackSnapshotArticulationRoute route;
                route.articulationId = zone.articulationId;
                route.zoneIds.push_back(zone.id);
                articulationRouteIndices.emplace(zone.articulationId, result.snapshot.articulationRoutes.size());
                result.snapshot.articulationRoutes.push_back(std::move(route));
            }
            else
            {
                result.snapshot.articulationRoutes[articulationIterator->second].zoneIds.push_back(zone.id);
            }
        }

        if (!zone.groupId.empty())
        {
            const auto groupIterator = groupRouteIndices.find(zone.groupId);
            if (groupIterator == groupRouteIndices.end())
            {
                if (usesExplicitGroupDefinitions)
                {
                    addFinding(result,
                               PlaybackSnapshotFindingSeverity::error,
                               "unknown-zone-group-reference",
                               path + ".groupId",
                               "Zone '" + zone.id + "' references unknown authored group '" + zone.groupId + "'.");
                }
                else
                {
                    PlaybackSnapshotGroupRoute route;
                    route.groupId = zone.groupId;
                    route.displayName = zone.groupId;
                    route.displayOrder = static_cast<int>(result.snapshot.groupRoutes.size());
                    route.routingSourceId = buildGroupRoutingSourceId(zone.groupId);
                    route.zoneIds.push_back(zone.id);
                    if (!zone.articulationId.empty())
                        route.articulationIds.push_back(zone.articulationId);
                    groupRouteIndices.emplace(zone.groupId, result.snapshot.groupRoutes.size());
                    result.snapshot.groupRoutes.push_back(std::move(route));
                }
            }
            else
            {
                auto& route = result.snapshot.groupRoutes[groupIterator->second];
                route.zoneIds.push_back(zone.id);
                if (!zone.articulationId.empty() && !containsValue(route.articulationIds, zone.articulationId))
                    route.articulationIds.push_back(zone.articulationId);
            }
        }
    }

    for (const auto& finding : crossfadeTopologyFindings)
    {
        if (finding.zoneIndex >= project.authoring.zones.size())
            continue;

        const auto& zone = project.authoring.zones[finding.zoneIndex];
        const auto path = "authoring.zones[" + std::to_string(finding.zoneIndex) + "].velocityCrossfade";
        addFinding(result,
                   PlaybackSnapshotFindingSeverity::error,
                   "invalid-zone-velocity-crossfade-topology",
                   path,
                   buildCrossfadeTopologyMessage(zone.id, finding.issue));
    }

    std::unordered_set<std::string> routedGroupZoneIds;
    for (std::size_t index = 0; index < result.snapshot.groupRoutes.size(); ++index)
    {
        const auto& route = result.snapshot.groupRoutes[index];
        const auto path = usesExplicitGroupDefinitions
            ? "authoring.groups[" + std::to_string(index) + "]"
            : "snapshot.groupRoutes[" + std::to_string(index) + "]";

        for (const auto& zoneId : route.zoneIds)
        {
            if (!routedGroupZoneIds.insert(zoneId).second)
            {
                addFinding(result,
                           PlaybackSnapshotFindingSeverity::error,
                           "duplicate-group-zone-coverage",
                           path + ".id",
                           "Zone '" + zoneId + "' appears more than once across authored group routes.");
            }
        }

        if (usesExplicitGroupDefinitions
            && !route.auditionAnchorZoneId.empty()
            && !containsValue(route.zoneIds, route.auditionAnchorZoneId))
        {
            addFinding(result,
                       PlaybackSnapshotFindingSeverity::error,
                       "invalid-group-audition-anchor",
                       path + ".auditionAnchorZoneId",
                       "Group '" + route.groupId + "' audition anchor '" + route.auditionAnchorZoneId
                           + "' must reference a zone assigned to that group.");
        }
    }

    if (!project.authoring.selectedZoneId.empty() && !zoneIndices.count(project.authoring.selectedZoneId))
    {
        addFinding(result, PlaybackSnapshotFindingSeverity::warning, "unknown-selected-zone", "authoring.selectedZoneId",
                   "Selected zone '" + project.authoring.selectedZoneId + "' does not exist in the snapshot zone set.");
    }

    if (usesExplicitGroupDefinitions
        && !project.authoring.selectedGroupId.empty()
        && !groupRouteIndices.count(project.authoring.selectedGroupId))
    {
        addFinding(result, PlaybackSnapshotFindingSeverity::warning, "unknown-selected-group", "authoring.selectedGroupId",
                   "Selected group '" + project.authoring.selectedGroupId + "' does not exist in the snapshot group set.");
    }

    if (result.snapshot.zones.empty())
    {
        addFinding(result, PlaybackSnapshotFindingSeverity::error, "no-playable-zones", "authoring.zones",
                   "Snapshot requires at least one playable zone before it can become activation-eligible.");
    }

    if (result.snapshot.sampleIdentities.empty())
    {
        addFinding(result, PlaybackSnapshotFindingSeverity::error, "no-sample-identities", "sampleSources",
                   "Snapshot requires at least one sample identity before it can become activation-eligible.");
    }

    if (project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5)
    {
        const auto compilation = compilePerformanceProgram(project.authoring);
        if (!compilation.compiled)
        {
            for (const auto& issue : compilation.issues)
                addFinding(result, PlaybackSnapshotFindingSeverity::error, "performance-program-compile-failed",
                           "authoring", issue);
        }
        else
        {
            result.snapshot.performanceProgram = compilation.program;
        }
    }

    const auto errorCount = countFindings(result, PlaybackSnapshotFindingSeverity::error);
    result.built = errorCount == 0;
    result.activationEligible = errorCount == 0;
    result.lifecycleState = errorCount == 0
        ? PlaybackSnapshotLifecycleState::ready
        : PlaybackSnapshotLifecycleState::failed;
    result.state = errorCount == 0
        ? "Immutable playback snapshot ready"
        : "Immutable playback snapshot failed validation";

    if (result.activationEligible)
    {
        result.snapshot.dspGraphDigest = computePlaybackSnapshotDspGraphDigest(result.snapshot);
        result.snapshot.contentDigest = "fnv1a64:" + computeFnv1a64Hex(serializeSnapshot(result.snapshot, false).dump());
    }

    result.buildDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
    return result;
}

PlaybackSnapshotBuildResult PlaybackSnapshotBuilder::cancelBuild(const PlaybackSnapshotBuildRequest& request,
                                                                const std::string& state) const
{
    PlaybackSnapshotBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = request.cancellationId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = PlaybackSnapshotLifecycleState::canceled;
    result.state = state;
    return result;
}

PlaybackSnapshotBuildResult PlaybackSnapshotBuilder::supersedeBuild(const PlaybackSnapshotBuildRequest& request,
                                                                    std::uint64_t replacementBuildId,
                                                                    const std::string& state) const
{
    PlaybackSnapshotBuildResult result;
    result.buildId = request.buildId;
    result.cancellationId = replacementBuildId;
    result.requestedDraftRevision = request.requestedDraftRevision;
    result.activationRequested = request.activationRequested;
    result.lifecycleState = PlaybackSnapshotLifecycleState::superseded;
    result.state = state;
    return result;
}

std::string toString(PlaybackSnapshotLifecycleState state)
{
    switch (state)
    {
    case PlaybackSnapshotLifecycleState::idle:
        return "Idle";
    case PlaybackSnapshotLifecycleState::preparing:
        return "Preparing";
    case PlaybackSnapshotLifecycleState::ready:
        return "Ready";
    case PlaybackSnapshotLifecycleState::activating:
        return "Activating";
    case PlaybackSnapshotLifecycleState::active:
        return "Active";
    case PlaybackSnapshotLifecycleState::failed:
        return "Failed";
    case PlaybackSnapshotLifecycleState::superseded:
        return "Superseded";
    case PlaybackSnapshotLifecycleState::canceled:
        return "Canceled";
    }

    return "Unknown";
}

std::string toString(PlaybackSnapshotFindingSeverity severity)
{
    switch (severity)
    {
    case PlaybackSnapshotFindingSeverity::warning:
        return "warning";
    case PlaybackSnapshotFindingSeverity::error:
        return "error";
    }

    return "unknown";
}

std::string toString(const PlaybackPreparationScope scope)
{
    switch (scope)
    {
    case PlaybackPreparationScope::currentDraft:
        return "CurrentDraft";
    case PlaybackPreparationScope::selectedZone:
        return "SelectedZone";
    case PlaybackPreparationScope::selectedGroup:
        return "SelectedGroup";
    }

    return "CurrentDraft";
}

PlaybackSnapshotBuildResult scopePlaybackSnapshotForPreparation(
    const PlaybackSnapshotBuildResult& source,
    const PlaybackPreparationScopeRequest& request)
{
    auto result = source;
    result.preparationScope = request.scope;
    result.preparationSelectedZoneId = request.selectedZoneId;
    result.preparationSelectedGroupId = request.selectedGroupId;
    result.unscopedZoneCount = source.snapshot.zones.size();
    result.unscopedSampleCount = source.snapshot.sampleIdentities.size();
    result.retainedZoneCount = result.unscopedZoneCount;
    result.retainedSampleCount = result.unscopedSampleCount;

    if (!source.built || !source.activationEligible
        || request.scope == PlaybackPreparationScope::currentDraft)
    {
        return result;
    }

    std::vector<std::string> retainedZoneIds;
    if (request.scope == PlaybackPreparationScope::selectedZone)
    {
        if (!request.selectedZoneId.empty())
            retainedZoneIds.push_back(request.selectedZoneId);
    }
    else
    {
        for (const auto& zone : source.snapshot.zones)
        {
            if (zone.groupId == request.selectedGroupId)
                retainedZoneIds.push_back(zone.id);
        }
    }

    const auto requestedZoneExists = request.scope != PlaybackPreparationScope::selectedZone
        || std::any_of(source.snapshot.zones.begin(),
                       source.snapshot.zones.end(),
                       [&](const PlaybackSnapshotZone& zone)
                       {
                           return zone.id == request.selectedZoneId;
                       });
    if (retainedZoneIds.empty() || !requestedZoneExists)
    {
        result.built = false;
        result.activationEligible = false;
        result.lifecycleState = PlaybackSnapshotLifecycleState::failed;
        result.state = request.scope == PlaybackPreparationScope::selectedZone
            ? "Selected-zone preparation scope was not found"
            : "Selected-group preparation scope was not found";
        addFinding(result,
                   PlaybackSnapshotFindingSeverity::error,
                   request.scope == PlaybackPreparationScope::selectedZone
                       ? "snapshot-preparation-zone-missing"
                       : "snapshot-preparation-group-missing",
                   request.scope == PlaybackPreparationScope::selectedZone
                       ? "preparationScope.selectedZoneId"
                       : "preparationScope.selectedGroupId",
                   "The requested Preview preparation scope does not exist in the immutable snapshot.");
        result.retainedZoneCount = 0;
        result.retainedSampleCount = 0;
        return result;
    }

    expandPreparationDependencies(source.snapshot, retainedZoneIds);

    std::vector<std::size_t> retainedZoneIndices;
    std::unordered_map<std::size_t, std::size_t> zoneIndexMap;
    retainedZoneIndices.reserve(retainedZoneIds.size());
    for (std::size_t index = 0; index < source.snapshot.zones.size(); ++index)
    {
        if (!containsValue(retainedZoneIds, source.snapshot.zones[index].id))
            continue;
        zoneIndexMap.emplace(index, retainedZoneIndices.size());
        retainedZoneIndices.push_back(index);
    }

    auto& scoped = result.snapshot;
    scoped.zones.erase(
        std::remove_if(scoped.zones.begin(),
                       scoped.zones.end(),
                       [&](const PlaybackSnapshotZone& zone)
                       {
                           return !containsValue(retainedZoneIds, zone.id);
                       }),
        scoped.zones.end());
    scoped.sampleIdentities.erase(
        std::remove_if(scoped.sampleIdentities.begin(),
                       scoped.sampleIdentities.end(),
                       [&](const PlaybackSnapshotSampleIdentity& sample)
                       {
                           return std::none_of(scoped.zones.begin(),
                                               scoped.zones.end(),
                                               [&](const PlaybackSnapshotZone& zone)
                                               {
                                                   return zone.sampleSourceId == sample.sampleSourceId;
                                               });
                       }),
        scoped.sampleIdentities.end());
    retainScopedRoutes(scoped.articulationRoutes, retainedZoneIds);
    retainScopedRoutes(scoped.groupRoutes, retainedZoneIds);
    scoped.routingBuses.erase(
        std::remove_if(scoped.routingBuses.begin(),
                       scoped.routingBuses.end(),
                       [&](const PlaybackSnapshotRoutingBusReference& bus)
                       {
                           if (bus.inputSourceId.rfind("zones/", 0) == 0)
                               return !containsValue(retainedZoneIds, bus.inputSourceId.substr(6));
                           if (bus.inputSourceId.rfind("groups/", 0) == 0)
                           {
                               return std::none_of(scoped.zones.begin(),
                                                   scoped.zones.end(),
                                                   [&](const PlaybackSnapshotZone& zone)
                                                   {
                                                       return zone.groupId == bus.inputSourceId.substr(7);
                                                   });
                           }
                           return false;
                       }),
        scoped.routingBuses.end());
    scoped.performanceProgram = remapPerformanceProgram(source.snapshot.performanceProgram,
                                                        retainedZoneIndices,
                                                        zoneIndexMap);
    scoped.selectedZoneId = request.selectedZoneId;
    if (request.scope == PlaybackPreparationScope::selectedGroup)
        scoped.selectedGroupId = request.selectedGroupId;
    else if (!scoped.zones.empty())
        scoped.selectedGroupId = scoped.zones.front().groupId;
    scoped.dspGraphDigest = computePlaybackSnapshotDspGraphDigest(scoped);
    scoped.contentDigest = computePlaybackSnapshotContentDigest(scoped);
    result.retainedZoneCount = scoped.zones.size();
    result.retainedSampleCount = scoped.sampleIdentities.size();
    result.state = "Playback snapshot scoped for " + toString(request.scope) + " preparation";
    return result;
}

std::string serializeImmutablePlaybackSnapshot(const ImmutablePlaybackSnapshot& snapshot)
{
    return serializeSnapshot(snapshot, true).dump(2) + "\n";
}

std::string computePlaybackSnapshotContentDigest(const ImmutablePlaybackSnapshot& snapshot)
{
    return "fnv1a64:" + computeFnv1a64Hex(serializeSnapshot(snapshot, false).dump());
}

std::string computePlaybackSnapshotDspGraphDigest(const ImmutablePlaybackSnapshot& snapshot)
{
    return "fnv1a64:" + computeFnv1a64Hex(serializeDspGraph(snapshot).dump());
}
} // namespace drs::engine
