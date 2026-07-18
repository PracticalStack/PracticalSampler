#include "drs/engine/PlaybackSnapshot.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <unordered_map>

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
    root["selectedPerformanceBankId"] = snapshot.selectedPerformanceBankId;

    if (includeDigest)
        root["contentDigest"] = snapshot.contentDigest;

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

        ordered_json targets = ordered_json::array();
        for (const auto& target : macro.targets)
        {
            ordered_json targetObject;
            targetObject["parameterId"] = target.parameterId;
            targetObject["parameterPath"] = target.parameterPath;
            targetObject["role"] = target.role;
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

    ordered_json groupRoutes = ordered_json::array();
    for (const auto& route : snapshot.groupRoutes)
    {
        ordered_json routeObject;
        routeObject["groupId"] = route.groupId;
        routeObject["articulationIds"] = serializeStringArray(route.articulationIds);
        routeObject["zoneIds"] = serializeStringArray(route.zoneIds);
        groupRoutes.push_back(std::move(routeObject));
    }
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
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);
    root["notes"] = serializeStringArray(snapshot.notes);
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
    result.snapshot.selectedPerformanceBankId = project.authoring.selectedPerformanceBankId;
    result.snapshot.notes = project.notes;
    result.snapshot.notes.insert(result.snapshot.notes.end(),
                                 project.authoring.notes.begin(),
                                 project.authoring.notes.end());

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
        snapshotMacro.targets.reserve(macro.targets.size());

        for (const auto& target : macro.targets)
            snapshotMacro.targets.push_back({ target.parameterId, target.parameterPath, target.role });

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

        result.snapshot.fxSlots.push_back({ fxSlot.id, fxSlot.displayName, fxSlot.effectType, fxSlot.bypassed });
    }

    result.snapshot.routingBuses.reserve(project.authoring.routingBuses.size());
    for (std::size_t index = 0; index < project.authoring.routingBuses.size(); ++index)
    {
        const auto& routingBus = project.authoring.routingBuses[index];
        const auto path = "authoring.routingBuses[" + std::to_string(index) + "]";

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
            routingBus.fxSlotIds
        });
    }

    std::unordered_map<std::string, std::size_t> articulationRouteIndices;
    std::unordered_map<std::string, std::size_t> groupRouteIndices;
    std::unordered_map<std::string, std::size_t> zoneIndices;
    result.snapshot.zones.reserve(project.authoring.zones.size());

    for (std::size_t index = 0; index < project.authoring.zones.size(); ++index)
    {
        const auto& zone = project.authoring.zones[index];
        const auto path = "authoring.zones[" + std::to_string(index) + "]";

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

        if (zone.loopEnabled && zone.loopEndFrame < zone.loopStartFrame)
            addFinding(result, PlaybackSnapshotFindingSeverity::error, "invalid-zone-loop-range", path,
                       "Loop-enabled zones must not declare loopEndFrame before loopStartFrame.");

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
            zone.gainDb,
            zone.pan,
            zone.sampleStartFrame,
            zone.loopEnabled,
            zone.loopStartFrame,
            zone.loopEndFrame
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
                PlaybackSnapshotGroupRoute route;
                route.groupId = zone.groupId;
                route.zoneIds.push_back(zone.id);
                if (!zone.articulationId.empty())
                    route.articulationIds.push_back(zone.articulationId);
                groupRouteIndices.emplace(zone.groupId, result.snapshot.groupRoutes.size());
                result.snapshot.groupRoutes.push_back(std::move(route));
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

    if (!project.authoring.selectedZoneId.empty() && !zoneIndices.count(project.authoring.selectedZoneId))
    {
        addFinding(result, PlaybackSnapshotFindingSeverity::warning, "unknown-selected-zone", "authoring.selectedZoneId",
                   "Selected zone '" + project.authoring.selectedZoneId + "' does not exist in the snapshot zone set.");
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

std::string serializeImmutablePlaybackSnapshot(const ImmutablePlaybackSnapshot& snapshot)
{
    return serializeSnapshot(snapshot, true).dump(2) + "\n";
}
} // namespace drs::engine
