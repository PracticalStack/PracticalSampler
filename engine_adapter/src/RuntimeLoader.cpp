#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/ControlLaw.h"
#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/PerformanceRuleContract.h"

#include "drs/engine/WorkspacePaths.generated.h"

#include <json/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;

template <typename TResult>
void addIssue(TResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::uint64_t computeFnv1a64(const std::string& text)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    return hash;
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

std::string readTextFile(const fs::path& filePath)
{
    std::ifstream input(filePath, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

fs::path resolveRelativePath(const fs::path& manifestPath, const std::string& rawPath)
{
    const fs::path candidate(rawPath);

    if (candidate.is_absolute())
        return candidate.lexically_normal();

    return (manifestPath.parent_path() / candidate).lexically_normal();
}

std::string toManifestRelativePath(const fs::path& manifestPath, const std::string& storedPath)
{
    const fs::path candidate(storedPath);

    if (!candidate.is_absolute())
        return candidate.generic_string();

    const auto relativePath = candidate.lexically_relative(manifestPath.parent_path());

    if (!relativePath.empty())
        return relativePath.generic_string();

    return candidate.generic_string();
}

template <typename TResult>
std::optional<fs::path> validateRequiredFile(TResult& result,
                                             const fs::path& manifestPath,
                                             const std::string& rawPath,
                                             const char* context)
{
    const auto resolvedPath = resolveRelativePath(manifestPath, rawPath);

    std::error_code errorCode;
    if (!fs::exists(resolvedPath, errorCode))
    {
        addIssue(result, std::string(context) + " does not exist: " + toDisplayPath(resolvedPath));
        return std::nullopt;
    }

    return resolvedPath;
}

template <typename TResult>
std::optional<fs::path> validateRequiredDirectory(TResult& result,
                                                  const fs::path& manifestPath,
                                                  const std::string& rawPath,
                                                  const char* context)
{
    const auto resolvedPath = resolveRelativePath(manifestPath, rawPath);

    std::error_code errorCode;
    if (!fs::exists(resolvedPath, errorCode) || !fs::is_directory(resolvedPath, errorCode))
    {
        addIssue(result, std::string(context) + " directory does not exist: " + toDisplayPath(resolvedPath));
        return std::nullopt;
    }

    return resolvedPath;
}

template <typename TResult, typename TValue>
std::optional<TValue> readRequired(const json& object,
                                   TResult& result,
                                   const char* propertyName,
                                   const char* context)
{
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return std::nullopt;
    }

    try
    {
        return iterator->get<TValue>();
    }
    catch (const json::exception&)
    {
        addIssue(result, std::string(context) + " has invalid type for field '" + propertyName + "'.");
        return std::nullopt;
    }
}

template <typename TResult, typename TValue>
std::optional<TValue> readOptional(const json& object,
                                   TResult& result,
                                   const char* propertyName,
                                   const char* context)
{
    const auto iterator = object.find(propertyName);

    if (iterator == object.end())
        return std::nullopt;

    try
    {
        return iterator->get<TValue>();
    }
    catch (const json::exception&)
    {
        addIssue(result, std::string(context) + " has invalid type for field '" + propertyName + "'.");
        return std::nullopt;
    }
}

template <typename TResult>
std::vector<std::string> readRequiredStringArray(const json& object,
                                                 TResult& result,
                                                 const char* propertyName,
                                                 const char* context)
{
    std::vector<std::string> values;

    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
    {
        addIssue(result, std::string(context) + " is missing required field '" + propertyName + "'.");
        return values;
    }

    if (!iterator->is_array())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an array.");
        return values;
    }

    values.reserve(iterator->size());
    for (const auto& entry : *iterator)
    {
        if (!entry.is_string())
        {
            addIssue(result, std::string(context) + " field '" + propertyName + "' must contain only strings.");
            continue;
        }

        values.push_back(entry.get<std::string>());
    }

    return values;
}

template <typename TResult>
std::vector<std::string> readOptionalStringArray(const json& object,
                                                 TResult& result,
                                                 const char* propertyName,
                                                 const char* context)
{
    std::vector<std::string> values;

    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
        return values;

    if (!iterator->is_array())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an array.");
        return values;
    }

    values.reserve(iterator->size());
    for (const auto& entry : *iterator)
    {
        if (!entry.is_string())
        {
            addIssue(result, std::string(context) + " field '" + propertyName + "' must contain only strings.");
            continue;
        }

        values.push_back(entry.get<std::string>());
    }

    return values;
}

bool isObjectArray(const json& value)
{
    return value.is_array()
        && std::all_of(value.begin(), value.end(), [](const auto& entry) { return entry.is_object(); });
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();

    for (const auto& value : values)
        array.push_back(value);

    return array;
}

template <typename TItem>
bool hasDuplicateIds(const std::vector<TItem>& items)
{
    std::unordered_set<std::string> ids;

    for (const auto& item : items)
    {
        if (item.id.empty())
            continue;

        if (!ids.insert(item.id).second)
            return true;
    }

    return false;
}

RuntimeProjectAuthoringState buildDefaultPhase2AuthoringState()
{
    RuntimeProjectAuthoringState authoring;
    authoring.schemaName = "drs.authoring";
    authoring.schemaVersion = 1;
    return authoring;
}

RuntimeProjectAuthoringState buildDefaultZoneGroupsAuthoringState()
{
    auto authoring = buildDefaultPhase2AuthoringState();
    authoring.schemaVersion = 3;
    return authoring;
}

std::vector<RuntimeProjectGroupDefinition> synthesizeProjectGroupsFromZones(
    const std::vector<RuntimeProjectZoneDefinition>& zones)
{
    std::vector<RuntimeProjectGroupDefinition> groups;
    std::unordered_set<std::string> groupIds;

    for (const auto& zone : zones)
    {
        if (zone.groupId.empty() || groupIds.count(zone.groupId))
            continue;

        RuntimeProjectGroupDefinition group;
        group.id = zone.groupId;
        group.displayName = zone.groupId;
        group.displayOrder = static_cast<int>(groups.size());
        group.workspaceVisible = true;
        group.gainDb = 0.0;
        group.pan = 0.0;
        group.auditionAnchorZoneId = zone.id;
        groups.push_back(std::move(group));
        groupIds.insert(zone.groupId);
    }

    return groups;
}

std::string resolveSelectedGroupIdFromSelectedZone(const RuntimeProjectAuthoringState& authoring)
{
    if (!authoring.selectedZoneId.empty())
    {
        const auto iterator = std::find_if(authoring.zones.begin(),
                                           authoring.zones.end(),
                                           [&](const RuntimeProjectZoneDefinition& zone)
                                           {
                                               return zone.id == authoring.selectedZoneId;
                                           });
        if (iterator != authoring.zones.end())
            return iterator->groupId;
    }

    if (!authoring.groups.empty())
        return authoring.groups.front().id;

    return {};
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
    ordered_json object;
    object["poolId"] = roundRobin.poolId;
    object["slotCount"] = roundRobin.slotCount;
    object["slotIndex"] = roundRobin.slotIndex;
    object["mode"] = toRoundRobinModeString(roundRobin.mode);
    return object;
}

template <typename TResult>
std::optional<RoundRobinDescriptor> readOptionalRoundRobin(const json& object,
                                                           TResult& result,
                                                           const char* propertyName,
                                                           const char* context)
{
    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
        return std::nullopt;

    if (!iterator->is_object())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an object.");
        return std::nullopt;
    }

    RoundRobinDescriptor descriptor;
    bool valid = true;

    if (const auto poolId = readRequired<TResult, std::string>(*iterator, result, "poolId", context))
        descriptor.poolId = *poolId;
    else
        valid = false;

    if (const auto slotCount = readRequired<TResult, int>(*iterator, result, "slotCount", context))
        descriptor.slotCount = *slotCount;
    else
        valid = false;

    if (const auto slotIndex = readRequired<TResult, int>(*iterator, result, "slotIndex", context))
        descriptor.slotIndex = *slotIndex;
    else
        valid = false;

    if (const auto mode = readRequired<TResult, std::string>(*iterator, result, "mode", context))
    {
        if (*mode == "sequential")
            descriptor.mode = RoundRobinMode::sequential;
        else if (*mode == "random")
            descriptor.mode = RoundRobinMode::random;
        else
        {
            addIssue(result, std::string(context) + " field '" + propertyName
                                 + ".mode' must be 'sequential' or 'random'.");
            valid = false;
        }
    }
    else
    {
        valid = false;
    }

    return valid ? std::optional<RoundRobinDescriptor>(descriptor) : std::nullopt;
}

template <typename TZone>
void applyRoundRobinDescriptor(TZone& zone, const RoundRobinDescriptor& roundRobin)
{
    zone.roundRobin = roundRobin;
    zone.roundRobinLength = roundRobin.slotCount;
    zone.roundRobinPosition = roundRobin.slotIndex;
}

template <typename TZone>
std::optional<RoundRobinDescriptor> synthesizeRoundRobinFromLegacyScalars(const TZone& zone)
{
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
    roundRobin.poolId = "legacy-rr-" + std::to_string(computeFnv1a64(stream.str()));
    roundRobin.slotCount = zone.roundRobinLength;
    roundRobin.slotIndex = zone.roundRobinPosition;
    roundRobin.mode = RoundRobinMode::sequential;
    return roundRobin;
}

template <typename TResult>
void validateRoundRobinDescriptor(TResult& result,
                                  const std::string& context,
                                  const std::optional<RoundRobinDescriptor>& roundRobin,
                                  int roundRobinLength,
                                  int roundRobinPosition,
                                  bool explicitObjectRequired,
                                  bool requireScalarMirror)
{
    if (roundRobinLength < 0)
        addIssue(result, context + " must not have a negative roundRobinLength.");

    if (roundRobinPosition < 0)
        addIssue(result, context + " must not have a negative roundRobinPosition.");

    if (roundRobinPosition > 0 && roundRobinLength <= 0)
        addIssue(result, context + " must set roundRobinLength when roundRobinPosition is present.");

    if (roundRobinLength > 0
        && (roundRobinPosition < 1 || roundRobinPosition > roundRobinLength))
    {
        addIssue(result, context + " has roundRobinPosition outside roundRobinLength.");
    }

    if (!roundRobin.has_value())
    {
        if (explicitObjectRequired && (roundRobinLength > 0 || roundRobinPosition > 0))
            addIssue(result, context + " must use the roundRobin object in the current schema.");

        return;
    }

    if (roundRobin->poolId.empty())
        addIssue(result, context + " roundRobin.poolId must not be empty.");

    if (roundRobin->slotCount <= 0)
        addIssue(result, context + " roundRobin.slotCount must be greater than zero.");

    if (roundRobin->slotCount > 0
        && (roundRobin->slotIndex < 1 || roundRobin->slotIndex > roundRobin->slotCount))
    {
        addIssue(result, context + " roundRobin.slotIndex must stay within roundRobin.slotCount.");
    }

    if (requireScalarMirror && roundRobinLength != roundRobin->slotCount)
        addIssue(result, context + " roundRobinLength must mirror roundRobin.slotCount.");

    if (requireScalarMirror && roundRobinPosition != roundRobin->slotIndex)
        addIssue(result, context + " roundRobinPosition must mirror roundRobin.slotIndex.");
}

ordered_json serializeMacroTargets(const std::vector<RuntimeProjectMacroTargetDefinition>& targets)
{
    ordered_json array = ordered_json::array();

    for (const auto& target : targets)
    {
        ordered_json targetObject;
        targetObject["parameterId"] = target.parameterId;
        targetObject["parameterPath"] = target.parameterPath;
        targetObject["role"] = target.role;
        if (!target.dspSlotId.empty() || !target.dspParameterId.empty())
        {
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
        }
        array.push_back(std::move(targetObject));
    }

    return array;
}

ordered_json serializeProjectMacros(const std::vector<RuntimeProjectMacroDefinition>& macros)
{
    ordered_json array = ordered_json::array();

    for (const auto& macro : macros)
    {
        ordered_json macroObject;
        macroObject["id"] = macro.id;
        macroObject["name"] = macro.name;
        macroObject["defaultValue"] = macro.defaultValue;
        macroObject["minValue"] = macro.minValue;
        macroObject["maxValue"] = macro.maxValue;
        macroObject["exposedInPerformance"] = macro.exposedInPerformance;
        macroObject["targets"] = serializeMacroTargets(macro.targets);
        array.push_back(std::move(macroObject));
    }

    return array;
}

ordered_json serializeVelocityCrossfade(const VelocityCrossfadeDescriptor& crossfade)
{
    ordered_json crossfadeObject;
    crossfadeObject["fadeInLowVelocity"] = crossfade.fadeInLowVelocity;
    crossfadeObject["fadeInHighVelocity"] = crossfade.fadeInHighVelocity;
    crossfadeObject["fadeOutLowVelocity"] = crossfade.fadeOutLowVelocity;
    crossfadeObject["fadeOutHighVelocity"] = crossfade.fadeOutHighVelocity;
    crossfadeObject["curve"] = "linear";
    return crossfadeObject;
}

ordered_json serializeVelocityCrossfadeRuntime(const VelocityCrossfadeRuntimeDescriptor& runtime)
{
    ordered_json runtimeObject;
    runtimeObject["effectiveLowVelocity"] = runtime.effectiveLowVelocity;
    runtimeObject["effectiveHighVelocity"] = runtime.effectiveHighVelocity;

    if (!runtime.fadeInNeighborZoneId.empty())
        runtimeObject["fadeInNeighborZoneId"] = runtime.fadeInNeighborZoneId;
    if (!runtime.fadeOutNeighborZoneId.empty())
        runtimeObject["fadeOutNeighborZoneId"] = runtime.fadeOutNeighborZoneId;
    if (runtime.fadeInOverlapLowVelocity > 0)
        runtimeObject["fadeInOverlapLowVelocity"] = runtime.fadeInOverlapLowVelocity;
    if (runtime.fadeInOverlapHighVelocity > 0)
        runtimeObject["fadeInOverlapHighVelocity"] = runtime.fadeInOverlapHighVelocity;
    if (runtime.fadeOutOverlapLowVelocity > 0)
        runtimeObject["fadeOutOverlapLowVelocity"] = runtime.fadeOutOverlapLowVelocity;
    if (runtime.fadeOutOverlapHighVelocity > 0)
        runtimeObject["fadeOutOverlapHighVelocity"] = runtime.fadeOutOverlapHighVelocity;

    return runtimeObject;
}

template <typename TResult>
std::optional<VelocityCrossfadeDescriptor> readOptionalVelocityCrossfade(const json& object,
                                                                         TResult& result,
                                                                         const char* propertyName,
                                                                         const char* context)
{
    const auto iterator = object.find(propertyName);
    if (iterator == object.end())
        return std::nullopt;

    if (!iterator->is_object())
    {
        addIssue(result, std::string(context) + " field '" + propertyName + "' must be an object.");
        return std::nullopt;
    }

    VelocityCrossfadeDescriptor descriptor;

    if (const auto fadeInLowVelocity =
            readOptional<TResult, int>(*iterator, result, "fadeInLowVelocity", context))
    {
        descriptor.fadeInLowVelocity = *fadeInLowVelocity;
    }

    if (const auto fadeInHighVelocity =
            readOptional<TResult, int>(*iterator, result, "fadeInHighVelocity", context))
    {
        descriptor.fadeInHighVelocity = *fadeInHighVelocity;
    }

    if (const auto fadeOutLowVelocity =
            readOptional<TResult, int>(*iterator, result, "fadeOutLowVelocity", context))
    {
        descriptor.fadeOutLowVelocity = *fadeOutLowVelocity;
    }

    if (const auto fadeOutHighVelocity =
            readOptional<TResult, int>(*iterator, result, "fadeOutHighVelocity", context))
    {
        descriptor.fadeOutHighVelocity = *fadeOutHighVelocity;
    }

    if (const auto curve = readOptional<TResult, std::string>(*iterator, result, "curve", context))
    {
        if (*curve != "linear")
            addIssue(result, std::string(context) + " field '" + propertyName + ".curve' must be 'linear'.");
    }

    return descriptor;
}

VelocityCrossfadeZoneDefinition buildVelocityCrossfadeValidationZone(int velocityLow,
                                                                    int velocityHigh,
                                                                    const VelocityCrossfadeDescriptor& crossfade)
{
    VelocityCrossfadeZoneDefinition zone;
    zone.velocityLow = velocityLow;
    zone.velocityHigh = velocityHigh;
    zone.crossfade = crossfade;
    return zone;
}

std::string buildVelocityCrossfadeIssue(const std::string& context,
                                        VelocityCrossfadeZoneIssue issue)
{
    switch (issue)
    {
        case VelocityCrossfadeZoneIssue::none:
            return {};
        case VelocityCrossfadeZoneIssue::velocityRangeInvalid:
            return context + " must keep velocityLow/velocityHigh within 1-127 and ordered low-to-high for crossfade support.";
        case VelocityCrossfadeZoneIssue::unsupportedCurve:
            return context + " must use the linear velocityCrossfade curve.";
        case VelocityCrossfadeZoneIssue::fadeInPartial:
            return context + " must define both velocityCrossfade.fadeInLowVelocity and fadeInHighVelocity when fade-in metadata is present.";
        case VelocityCrossfadeZoneIssue::fadeInOutOfRange:
            return context + " must anchor velocityCrossfade fade-in to velocityLow and keep it inside the zone velocity window.";
        case VelocityCrossfadeZoneIssue::fadeInInverted:
            return context + " must keep velocityCrossfade fadeInLowVelocity lower than fadeInHighVelocity.";
        case VelocityCrossfadeZoneIssue::fadeOutPartial:
            return context + " must define both velocityCrossfade.fadeOutLowVelocity and fadeOutHighVelocity when fade-out metadata is present.";
        case VelocityCrossfadeZoneIssue::fadeOutOutOfRange:
            return context + " must anchor velocityCrossfade fade-out to velocityHigh and keep it inside the zone velocity window.";
        case VelocityCrossfadeZoneIssue::fadeOutInverted:
            return context + " must keep velocityCrossfade.fadeOutLowVelocity lower than fadeOutHighVelocity.";
        case VelocityCrossfadeZoneIssue::fadeWindowsOverlap:
            return context + " must keep velocityCrossfade fade-in and fade-out windows disjoint within one zone.";
    }

    return context + " contains an unknown velocityCrossfade validation issue.";
}

std::string buildVelocityCrossfadeTopologyIssue(const std::string& context,
                                                VelocityCrossfadeTopologyIssue issue)
{
    switch (issue)
    {
        case VelocityCrossfadeTopologyIssue::none:
            return {};
        case VelocityCrossfadeTopologyIssue::fadeInMissingPartner:
            return context + " must resolve exactly one lower crossfade partner for velocityCrossfade fade-in.";
        case VelocityCrossfadeTopologyIssue::fadeInAmbiguousPartner:
            return context + " matched multiple lower crossfade partners for velocityCrossfade fade-in.";
        case VelocityCrossfadeTopologyIssue::fadeOutMissingPartner:
            return context + " must resolve exactly one upper crossfade partner for velocityCrossfade fade-out.";
        case VelocityCrossfadeTopologyIssue::fadeOutAmbiguousPartner:
            return context + " matched multiple upper crossfade partners for velocityCrossfade fade-out.";
        case VelocityCrossfadeTopologyIssue::roundRobinDuplicateSlot:
            return context + " duplicates a Round Robin slot within one crossfade layer.";
        case VelocityCrossfadeTopologyIssue::roundRobinIncompletePool:
            return context + " belongs to a Round Robin pool with incomplete slot coverage.";
        case VelocityCrossfadeTopologyIssue::roundRobinMixedSlotCount:
            return context + " belongs to a Round Robin pool with mixed slot counts.";
    }

    return context + " contains an unknown velocityCrossfade topology issue.";
}

template <typename TZone>
std::uint64_t buildVelocityCrossfadePairingKey(const TZone& zone)
{
    return computeVelocityCrossfadePairingKey(zone.articulationId,
                                              zone.rootKey,
                                              zone.keyLow,
                                              zone.keyHigh,
                                              static_cast<int>(zone.triggerMode));
}

template <typename TZone>
std::string resolveVelocityCrossfadeRoundRobinPoolId(const TZone& zone)
{
    return zone.roundRobin.has_value() ? zone.roundRobin->poolId : std::string {};
}

template <typename TZone>
std::vector<VelocityCrossfadeTopologyFinding> collectVelocityCrossfadeTopologyFindings(
    const std::vector<TZone>& zones)
{
    std::vector<VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(zones.size());

    for (const auto& zone : zones)
    {
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildVelocityCrossfadePairingKey(zone);
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = resolveVelocityCrossfadeRoundRobinPoolId(zone);
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        topologyZones.push_back(topologyZone);
    }

    std::vector<VelocityCrossfadeTopologyFinding> findings;
    buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones, &findings);
    return findings;
}

void populateVelocityCrossfadeRuntimeDescriptors(std::vector<RuntimeZoneDefinition>& zones)
{
    std::vector<VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(zones.size());

    for (const auto& zone : zones)
    {
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildVelocityCrossfadePairingKey(zone);
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = resolveVelocityCrossfadeRoundRobinPoolId(zone);
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        topologyZones.push_back(topologyZone);
    }

    const auto runtimeTopology = buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones);
    for (std::size_t index = 0; index < zones.size(); ++index)
    {
        auto& zone = zones[index];
        zone.velocityCrossfadeRuntime = {};
        if (!hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            continue;

        const auto& topology = runtimeTopology[index];
        zone.velocityCrossfadeRuntime.effectiveLowVelocity = topology.effectiveLowVelocity;
        zone.velocityCrossfadeRuntime.effectiveHighVelocity = topology.effectiveHighVelocity;
        zone.velocityCrossfadeRuntime.fadeInOverlapLowVelocity = topology.fadeInOverlapLowVelocity;
        zone.velocityCrossfadeRuntime.fadeInOverlapHighVelocity = topology.fadeInOverlapHighVelocity;
        zone.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity = topology.fadeOutOverlapLowVelocity;
        zone.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity = topology.fadeOutOverlapHighVelocity;

        if (topology.fadeInNeighborZoneIndex >= 0)
        {
            zone.velocityCrossfadeRuntime.fadeInNeighborZoneId =
                zones[static_cast<std::size_t>(topology.fadeInNeighborZoneIndex)].id;
        }

        if (topology.fadeOutNeighborZoneIndex >= 0)
        {
            zone.velocityCrossfadeRuntime.fadeOutNeighborZoneId =
                zones[static_cast<std::size_t>(topology.fadeOutNeighborZoneIndex)].id;
        }
    }
}

ordered_json serializeProjectZones(const std::vector<RuntimeProjectZoneDefinition>& zones,
                                   bool useExplicitRoundRobin,
                                   bool usePerformanceRules)
{
    ordered_json array = ordered_json::array();

    for (const auto& zone : zones)
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
        zoneObject["gainDb"] = zone.gainDb;
        zoneObject["pan"] = zone.pan;
        zoneObject["sampleStartFrame"] = zone.sampleStartFrame;
        zoneObject["loopEnabled"] = zone.loopEnabled;
        zoneObject["loopStartFrame"] = zone.loopStartFrame;
        zoneObject["loopEndFrame"] = zone.loopEndFrame;
        zoneObject["releaseSeconds"] = zone.releaseSeconds;
        if (useExplicitRoundRobin)
        {
            if (zone.roundRobin.has_value())
                zoneObject["roundRobin"] = serializeRoundRobin(*zone.roundRobin);
        }
        else
        {
            zoneObject["roundRobinLength"] = zone.roundRobinLength;
            zoneObject["roundRobinPosition"] = zone.roundRobinPosition;
        }
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            zoneObject["triggerMode"] = "one-shot";
        if (usePerformanceRules)
        {
            zoneObject["performance"] = {
                { "event", performanceEventKindId(zone.performance.event) },
                { "sustain", performanceSustainConditionId(zone.performance.sustain) },
                { "pitchSource", performancePitchSourceId(zone.performance.pitchSource) }
            };
            if (!zone.exclusiveGroupId.empty())
                zoneObject["exclusiveGroupId"] = zone.exclusiveGroupId;
            if (!zone.exclusiveTargetGroupIds.empty())
                zoneObject["exclusiveTargetGroupIds"] = serializeStringArray(zone.exclusiveTargetGroupIds);
            if (zone.chokeReleaseSeconds.has_value())
                zoneObject["chokeReleaseSeconds"] = *zone.chokeReleaseSeconds;
        }
        array.push_back(std::move(zoneObject));
    }

    return array;
}

ordered_json serializeProjectGroups(const std::vector<RuntimeProjectGroupDefinition>& groups)
{
    ordered_json array = ordered_json::array();

    for (const auto& group : groups)
    {
        ordered_json groupObject;
        groupObject["id"] = group.id;
        groupObject["displayName"] = group.displayName;
        groupObject["displayOrder"] = group.displayOrder;
        groupObject["workspaceVisible"] = group.workspaceVisible;
        groupObject["gainDb"] = group.gainDb;
        groupObject["pan"] = group.pan;
        if (!group.routingBusId.empty())
            groupObject["routingBusId"] = group.routingBusId;
        if (!group.auditionAnchorZoneId.empty())
            groupObject["auditionAnchorZoneId"] = group.auditionAnchorZoneId;
        array.push_back(std::move(groupObject));
    }

    return array;
}

ordered_json serializeProjectArticulations(
    const std::vector<RuntimeProjectArticulationDefinition>& articulations)
{
    ordered_json array = ordered_json::array();
    for (const auto& articulation : articulations)
    {
        ordered_json articulationObject;
        articulationObject["id"] = articulation.id;
        articulationObject["displayName"] = articulation.displayName;
        articulationObject["isDefault"] = articulation.isDefault;
        articulationObject["displayOrder"] = articulation.displayOrder;
        if (articulation.activation.has_value())
        {
            const auto& activation = *articulation.activation;
            articulationObject["activation"] = {
                { "event", performanceEventKindId(activation.event) },
                { "midiNote", activation.midiNote },
                { "mode", articulationActivationModeId(activation.mode) },
                { "consume", activation.consume }
            };
        }
        array.push_back(std::move(articulationObject));
    }
    return array;
}

ordered_json serializeRoundRobinResetRules(
    const std::vector<RuntimeProjectRoundRobinResetRuleDefinition>& rules)
{
    ordered_json array = ordered_json::array();
    for (const auto& rule : rules)
    {
        ordered_json object;
        object["event"] = roundRobinResetEventId(rule.event);
        if (rule.targetAll)
            object["target"] = "all";
        else
            object["targetPoolId"] = rule.targetPoolId;
        array.push_back(std::move(object));
    }
    return array;
}

ordered_json serializeFxSlots(const std::vector<RuntimeProjectFxSlotDefinition>& fxSlots, const bool dspSchema)
{
    ordered_json array = ordered_json::array();

    for (const auto& fxSlot : fxSlots)
    {
        ordered_json fxObject;
        fxObject["id"] = fxSlot.id;
        fxObject["displayName"] = fxSlot.displayName;
        fxObject["effectType"] = fxSlot.effectType;
        fxObject["bypassed"] = fxSlot.bypassed;
        if (dspSchema)
        {
            fxObject["effectVersion"] = fxSlot.effectVersion;
            fxObject["legacyInert"] = fxSlot.legacyInert;
            ordered_json parameters = ordered_json::array();
            for (const auto& parameter : fxSlot.parameters)
                parameters.push_back({ { "id", parameter.id }, { "value", parameter.value } });
            fxObject["parameters"] = std::move(parameters);
        }
        array.push_back(std::move(fxObject));
    }

    return array;
}

ordered_json serializeRoutingBuses(const std::vector<RuntimeProjectRoutingBusDefinition>& routingBuses, const bool dspSchema)
{
    ordered_json array = ordered_json::array();

    for (const auto& bus : routingBuses)
    {
        ordered_json busObject;
        busObject["id"] = bus.id;
        busObject["displayName"] = bus.displayName;
        busObject["inputSourceId"] = bus.inputSourceId;
        busObject["fxSlotIds"] = serializeStringArray(bus.fxSlotIds);
        if (dspSchema)
            busObject["chainBypassed"] = bus.chainBypassed;
        array.push_back(std::move(busObject));
    }

    return array;
}

ordered_json serializeTriggerSlots(const std::vector<RuntimeProjectTriggerSlotDefinition>& triggerSlots)
{
    ordered_json array = ordered_json::array();

    for (const auto& slot : triggerSlots)
    {
        ordered_json slotObject;
        slotObject["id"] = slot.id;
        slotObject["displayName"] = slot.displayName;
        slotObject["triggerEvent"] = slot.triggerEvent;
        slotObject["targetArticulationId"] = slot.targetArticulationId;
        if (!slot.phraseAssetId.empty())
            slotObject["phraseAssetId"] = slot.phraseAssetId;
        if (!slot.chordMode.empty())
            slotObject["chordMode"] = slot.chordMode;
        array.push_back(std::move(slotObject));
    }

    return array;
}

ordered_json serializePhraseNotes(const std::vector<RuntimeProjectPhraseNoteDefinition>& notes)
{
    ordered_json array = ordered_json::array();

    for (const auto& note : notes)
    {
        ordered_json noteObject;
        noteObject["midiNote"] = note.midiNote;
        noteObject["velocity"] = note.velocity;
        noteObject["startBeat"] = note.startBeat;
        noteObject["durationBeats"] = note.durationBeats;
        array.push_back(std::move(noteObject));
    }

    return array;
}

ordered_json serializePhraseAssets(const std::vector<RuntimeProjectPhraseAssetDefinition>& phraseAssets)
{
    ordered_json array = ordered_json::array();

    for (const auto& phraseAsset : phraseAssets)
    {
        ordered_json phraseObject;
        phraseObject["id"] = phraseAsset.id;
        phraseObject["displayName"] = phraseAsset.displayName;
        phraseObject["sourcePath"] = phraseAsset.sourcePath;
        phraseObject["ticksPerQuarter"] = phraseAsset.ticksPerQuarter;
        phraseObject["lengthBeats"] = phraseAsset.lengthBeats;
        phraseObject["chordHint"] = phraseAsset.chordHint;
        phraseObject["normalizationState"] = phraseAsset.normalizationState;
        phraseObject["issues"] = serializeStringArray(phraseAsset.issues);
        phraseObject["notes"] = serializePhraseNotes(phraseAsset.notes);
        array.push_back(std::move(phraseObject));
    }

    return array;
}

ordered_json serializePerformanceBanks(const std::vector<RuntimeProjectPerformanceBankDefinition>& banks)
{
    ordered_json array = ordered_json::array();

    for (const auto& bank : banks)
    {
        ordered_json bankObject;
        bankObject["id"] = bank.id;
        bankObject["displayName"] = bank.displayName;
        bankObject["triggerSlots"] = serializeTriggerSlots(bank.triggerSlots);
        bankObject["phraseAssets"] = serializePhraseAssets(bank.phraseAssets);
        bankObject["notes"] = serializeStringArray(bank.notes);
        array.push_back(std::move(bankObject));
    }

    return array;
}
} // namespace

std::string getPhase1RuntimeRootPath()
{
    return generated::workspacePhase1RuntimeRoot;
}

std::string getPhase1ReferenceCorpusIndexPath()
{
    return generated::workspacePhase1ReferenceCorpusIndex;
}

std::string getPhase1ReferenceBenchmarkScenePath()
{
    return generated::workspacePhase1ReferenceBenchmarkScene;
}

std::string getPhase1ReferenceBaselinePath()
{
    return generated::workspacePhase1ReferenceBaseline;
}

std::string getPhase1ReferenceProjectManifestPath()
{
    return generated::workspacePhase1ReferenceProject;
}

std::string getPhase1ReferenceInstrumentManifestPath()
{
    return generated::workspacePhase1ReferenceManifest;
}

std::string getPhase1ReferencePackageManifestPath()
{
    return generated::workspacePhase1ReferencePackageManifest;
}

std::string getPhase2RuntimeRootPath()
{
    return generated::workspacePhase2RuntimeRoot;
}

std::string getPhase2ReferenceProjectManifestPath()
{
    return generated::workspacePhase2ReferenceProject;
}

RuntimeProjectLoadResult parseRuntimeProjectManifest(const std::string& rawText,
                                                     const std::string& manifestPath,
                                                     const bool validateReferencedPaths)
{
    RuntimeProjectLoadResult result;
    result.manifestPath = manifestPath;
    result.state = "Project parse not attempted";
    result.manifestFound = true;

    const fs::path manifestFsPath(manifestPath);
    if (rawText.empty())
    {
        result.state = "Project unreadable";
        addIssue(result, "Project JSON text was empty.");
        return result;
    }

    json root;
    try
    {
        root = json::parse(rawText);
    }
    catch (const json::exception& exception)
    {
        result.state = "Project parse failed";
        addIssue(result, "Project JSON parse failed: " + std::string(exception.what()));
        return result;
    }

    if (!root.is_object())
    {
        result.state = "Project root invalid";
        addIssue(result, "Project root must be a JSON object.");
        return result;
    }

    auto& project = result.project;

    if (const auto schemaName = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "schemaName", "Project"))
        project.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<RuntimeProjectLoadResult, int>(root, result, "schemaVersion", "Project"))
        project.schemaVersion = *schemaVersion;

    if (const auto projectId = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "projectId", "Project"))
        project.projectId = *projectId;

    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "displayName", "Project"))
        project.displayName = *displayName;

    if (const auto contentRoot = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "contentRoot", "Project"))
    {
        const auto resolved = validateReferencedPaths
            ? validateRequiredDirectory(result, manifestFsPath, *contentRoot, "Project content root")
            : std::optional<fs::path> { resolveRelativePath(manifestFsPath, *contentRoot) };
        project.contentRootPath = resolved ? toDisplayPath(*resolved) : *contentRoot;
    }

    if (const auto defaultInstrument = readRequired<RuntimeProjectLoadResult, std::string>(root, result, "defaultInstrumentManifest", "Project"))
    {
        const auto resolved = validateReferencedPaths
            ? validateRequiredFile(result, manifestFsPath, *defaultInstrument, "Default instrument manifest")
            : std::optional<fs::path> { resolveRelativePath(manifestFsPath, *defaultInstrument) };
        project.defaultInstrumentManifestPath = resolved ? toDisplayPath(*resolved) : *defaultInstrument;
    }

    const auto sampleSourcesIterator = root.find("sampleSources");
    if (sampleSourcesIterator == root.end() || !isObjectArray(*sampleSourcesIterator))
    {
        addIssue(result, "Project field 'sampleSources' must be an array of objects.");
    }
    else
    {
        project.sampleSources.reserve(sampleSourcesIterator->size());

        for (std::size_t index = 0; index < sampleSourcesIterator->size(); ++index)
        {
            const auto& sampleObject = sampleSourcesIterator->at(index);
            const auto context = "SampleSource[" + std::to_string(index) + "]";
            RuntimeProjectSampleSource sampleSource;

            if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "id", context.c_str()))
                sampleSource.id = *id;

            if (const auto path = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "path", context.c_str()))
            {
                const auto resolved = validateReferencedPaths
                    ? validateRequiredFile(result, manifestFsPath, *path, "Sample source")
                    : std::optional<fs::path> { resolveRelativePath(manifestFsPath, *path) };
                sampleSource.path = resolved ? toDisplayPath(*resolved) : *path;
            }

            if (const auto role = readRequired<RuntimeProjectLoadResult, std::string>(sampleObject, result, "role", context.c_str()))
                sampleSource.role = *role;

            project.sampleSources.push_back(std::move(sampleSource));
        }
    }

    project.notes = readRequiredStringArray(root, result, "notes", "Project");

    if (project.schemaVersion >= 2 && project.schemaVersion <= 6)
    {
        const auto authoringIterator = root.find("authoring");
        if (authoringIterator == root.end() || !authoringIterator->is_object())
        {
            addIssue(result, "Project authoring schemas require an 'authoring' object.");
        }
        else
        {
            auto& authoring = project.authoring;

            if (const auto schemaName = readRequired<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "schemaName", "Project authoring"))
                authoring.schemaName = *schemaName;

            if (const auto schemaVersion = readRequired<RuntimeProjectLoadResult, int>(*authoringIterator, result, "schemaVersion", "Project authoring"))
                authoring.schemaVersion = *schemaVersion;

            if (const auto selectedZoneId = readOptional<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "selectedZoneId", "Project authoring"))
                authoring.selectedZoneId = *selectedZoneId;

            if (const auto selectedGroupId = readOptional<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "selectedGroupId", "Project authoring"))
                authoring.selectedGroupId = *selectedGroupId;

            if (const auto selectedPerformanceBankId = readOptional<RuntimeProjectLoadResult, std::string>(*authoringIterator, result, "selectedPerformanceBankId", "Project authoring"))
                authoring.selectedPerformanceBankId = *selectedPerformanceBankId;

            if (project.schemaVersion >= 6 && authoring.schemaVersion >= 5)
            {
                const auto articulationsIterator = authoringIterator->find("articulations");
                if (articulationsIterator == authoringIterator->end() || !isObjectArray(*articulationsIterator))
                {
                    addIssue(result, "Project authoring field 'articulations' must be an array of objects.");
                }
                else
                {
                    authoring.articulations.reserve(articulationsIterator->size());
                    for (std::size_t index = 0; index < articulationsIterator->size(); ++index)
                    {
                        const auto& articulationObject = articulationsIterator->at(index);
                        const auto context = "ProjectArticulation[" + std::to_string(index) + "]";
                        RuntimeProjectArticulationDefinition articulation;
                        if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(articulationObject, result, "id", context.c_str()))
                            articulation.id = *id;
                        if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(articulationObject, result, "displayName", context.c_str()))
                            articulation.displayName = *displayName;
                        if (const auto isDefault = readRequired<RuntimeProjectLoadResult, bool>(articulationObject, result, "isDefault", context.c_str()))
                            articulation.isDefault = *isDefault;
                        if (const auto displayOrder = readRequired<RuntimeProjectLoadResult, int>(articulationObject, result, "displayOrder", context.c_str()))
                            articulation.displayOrder = *displayOrder;
                        const auto activationIterator = articulationObject.find("activation");
                        if (activationIterator != articulationObject.end())
                        {
                            if (!activationIterator->is_object())
                            {
                                addIssue(result, context + " field 'activation' must be an object.");
                            }
                            else
                            {
                                RuntimeProjectArticulationActivationDefinition activation;
                                if (const auto event = readRequired<RuntimeProjectLoadResult, std::string>(*activationIterator, result, "event", context.c_str()))
                                    if (!parsePerformanceEventKind(*event, activation.event))
                                        addIssue(result, context + " activation field 'event' is unsupported.");
                                if (const auto midiNote = readRequired<RuntimeProjectLoadResult, int>(*activationIterator, result, "midiNote", context.c_str())) activation.midiNote = *midiNote;
                                if (const auto mode = readRequired<RuntimeProjectLoadResult, std::string>(*activationIterator, result, "mode", context.c_str()))
                                    if (!parseArticulationActivationMode(*mode, activation.mode))
                                        addIssue(result, context + " activation field 'mode' is unsupported.");
                                if (const auto consume = readRequired<RuntimeProjectLoadResult, bool>(*activationIterator, result, "consume", context.c_str())) activation.consume = *consume;
                                articulation.activation = std::move(activation);
                            }
                        }
                        authoring.articulations.push_back(std::move(articulation));
                    }
                }
            }

            const auto zonesIterator = authoringIterator->find("zones");
            if (zonesIterator == authoringIterator->end() || !isObjectArray(*zonesIterator))
            {
                addIssue(result, "Project authoring field 'zones' must be an array of objects.");
            }
            else
            {
                authoring.zones.reserve(zonesIterator->size());

                for (std::size_t index = 0; index < zonesIterator->size(); ++index)
                {
                    const auto& zoneObject = zonesIterator->at(index);
                    const auto context = "ProjectZone[" + std::to_string(index) + "]";
                    RuntimeProjectZoneDefinition zone;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "id", context.c_str()))
                        zone.id = *id;
                    if (const auto sampleSourceId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "sampleSourceId", context.c_str()))
                        zone.sampleSourceId = *sampleSourceId;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "displayName", context.c_str()))
                        zone.displayName = *displayName;
                    if (const auto groupId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "groupId", context.c_str()))
                        zone.groupId = *groupId;
                    if (const auto articulationId = readRequired<RuntimeProjectLoadResult, std::string>(zoneObject, result, "articulationId", context.c_str()))
                        zone.articulationId = *articulationId;
                    if (const auto rootKey = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "rootKey", context.c_str()))
                        zone.rootKey = *rootKey;
                    if (const auto keyLow = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "keyLow", context.c_str()))
                        zone.keyLow = *keyLow;
                    if (const auto keyHigh = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "keyHigh", context.c_str()))
                        zone.keyHigh = *keyHigh;
                    if (const auto velocityLow = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "velocityLow", context.c_str()))
                        zone.velocityLow = *velocityLow;
                    if (const auto velocityHigh = readRequired<RuntimeProjectLoadResult, int>(zoneObject, result, "velocityHigh", context.c_str()))
                        zone.velocityHigh = *velocityHigh;
                    if (const auto velocityCrossfade =
                            readOptionalVelocityCrossfade(zoneObject, result, "velocityCrossfade", context.c_str()))
                    {
                        zone.velocityCrossfade = *velocityCrossfade;
                    }
                    if (const auto gainDb = readRequired<RuntimeProjectLoadResult, double>(zoneObject, result, "gainDb", context.c_str()))
                        zone.gainDb = *gainDb;
                    if (const auto pan = readRequired<RuntimeProjectLoadResult, double>(zoneObject, result, "pan", context.c_str()))
                        zone.pan = *pan;
                    if (const auto sampleStartFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "sampleStartFrame", context.c_str()))
                        zone.sampleStartFrame = *sampleStartFrame;
                    if (const auto loopEnabled = readRequired<RuntimeProjectLoadResult, bool>(zoneObject, result, "loopEnabled", context.c_str()))
                        zone.loopEnabled = *loopEnabled;
                    if (const auto loopStartFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "loopStartFrame", context.c_str()))
                        zone.loopStartFrame = *loopStartFrame;
                    if (const auto loopEndFrame = readRequired<RuntimeProjectLoadResult, std::uint64_t>(zoneObject, result, "loopEndFrame", context.c_str()))
                        zone.loopEndFrame = *loopEndFrame;
                    if (const auto releaseSeconds = readOptional<RuntimeProjectLoadResult, double>(zoneObject, result, "releaseSeconds", context.c_str()))
                        zone.releaseSeconds = *releaseSeconds;

                    const auto explicitRoundRobin = readOptionalRoundRobin(zoneObject, result, "roundRobin", context.c_str());
                    const auto hasExplicitRoundRobinField = zoneObject.find("roundRobin") != zoneObject.end();
                    if (hasExplicitRoundRobinField)
                    {
                        if (explicitRoundRobin.has_value())
                            applyRoundRobinDescriptor(zone, *explicitRoundRobin);

                        if (project.schemaVersion >= 3
                            && authoring.schemaVersion >= 2
                            && (zoneObject.find("roundRobinLength") != zoneObject.end()
                                || zoneObject.find("roundRobinPosition") != zoneObject.end()))
                        {
                            addIssue(result, context + " must not mix roundRobin scalars with the roundRobin object in the current schema.");
                        }
                    }
                    else
                    {
                        if (const auto roundRobinLength = readOptional<RuntimeProjectLoadResult, int>(zoneObject, result, "roundRobinLength", context.c_str()))
                            zone.roundRobinLength = *roundRobinLength;
                        if (const auto roundRobinPosition = readOptional<RuntimeProjectLoadResult, int>(zoneObject, result, "roundRobinPosition", context.c_str()))
                            zone.roundRobinPosition = *roundRobinPosition;

                        if (project.schemaVersion >= 3
                            && authoring.schemaVersion >= 2
                            && (zone.roundRobinLength > 0 || zone.roundRobinPosition > 0))
                        {
                            addIssue(result, context + " must use the roundRobin object in schemaVersion 3 files.");
                        }

                    }

                    if (const auto triggerMode = readOptional<RuntimeProjectLoadResult, std::string>(zoneObject, result, "triggerMode", context.c_str()))
                    {
                        if (*triggerMode == "gated")
                            zone.triggerMode = ZoneTriggerMode::gated;
                        else if (*triggerMode == "one-shot")
                            zone.triggerMode = ZoneTriggerMode::oneShot;
                        else
                            addIssue(result, context + " field 'triggerMode' must be 'gated' or 'one-shot'.");
                    }

                    const auto performanceIterator = zoneObject.find("performance");
                    if (performanceIterator != zoneObject.end())
                    {
                        if (!performanceIterator->is_object())
                        {
                            addIssue(result, context + " field 'performance' must be an object.");
                        }
                        else
                        {
                            if (const auto event = readRequired<RuntimeProjectLoadResult, std::string>(*performanceIterator, result, "event", context.c_str()))
                                if (!parsePerformanceEventKind(*event, zone.performance.event)) addIssue(result, context + " performance field 'event' is unsupported.");
                            if (const auto sustain = readRequired<RuntimeProjectLoadResult, std::string>(*performanceIterator, result, "sustain", context.c_str()))
                                if (!parsePerformanceSustainCondition(*sustain, zone.performance.sustain)) addIssue(result, context + " performance field 'sustain' is unsupported.");
                            if (const auto pitchSource = readRequired<RuntimeProjectLoadResult, std::string>(*performanceIterator, result, "pitchSource", context.c_str()))
                                if (!parsePerformancePitchSource(*pitchSource, zone.performance.pitchSource)) addIssue(result, context + " performance field 'pitchSource' is unsupported.");
                        }
                    }
                    if (const auto exclusiveGroupId = readOptional<RuntimeProjectLoadResult, std::string>(zoneObject, result, "exclusiveGroupId", context.c_str()))
                        zone.exclusiveGroupId = *exclusiveGroupId;
                    if (zoneObject.find("exclusiveTargetGroupIds") != zoneObject.end())
                        zone.exclusiveTargetGroupIds = readRequiredStringArray(zoneObject, result, "exclusiveTargetGroupIds", context.c_str());
                    if (const auto chokeReleaseSeconds = readOptional<RuntimeProjectLoadResult, double>(zoneObject, result, "chokeReleaseSeconds", context.c_str()))
                        zone.chokeReleaseSeconds = *chokeReleaseSeconds;

                    if (!hasExplicitRoundRobinField)
                    {
                        if (const auto synthesizedRoundRobin = synthesizeRoundRobinFromLegacyScalars(zone))
                            applyRoundRobinDescriptor(zone, *synthesizedRoundRobin);
                    }

                    authoring.zones.push_back(std::move(zone));
                }
            }

            if (project.schemaVersion >= 6 && authoring.schemaVersion >= 5)
            {
                const auto resetRulesIterator = authoringIterator->find("roundRobinResetRules");
                if (resetRulesIterator != authoringIterator->end())
                {
                    if (!isObjectArray(*resetRulesIterator))
                    {
                        addIssue(result, "Project authoring field 'roundRobinResetRules' must be an array of objects.");
                    }
                    else
                    {
                        authoring.roundRobinResetRules.reserve(resetRulesIterator->size());
                        for (std::size_t index = 0; index < resetRulesIterator->size(); ++index)
                        {
                            const auto& ruleObject = resetRulesIterator->at(index);
                            const auto context = "RoundRobinResetRule[" + std::to_string(index) + "]";
                            RuntimeProjectRoundRobinResetRuleDefinition rule;
                            if (const auto event = readRequired<RuntimeProjectLoadResult, std::string>(ruleObject, result, "event", context.c_str()))
                                if (!parseRoundRobinResetEvent(*event, rule.event)) addIssue(result, context + " field 'event' is unsupported.");
                            const auto target = readOptional<RuntimeProjectLoadResult, std::string>(ruleObject, result, "target", context.c_str());
                            const auto targetPoolId = readOptional<RuntimeProjectLoadResult, std::string>(ruleObject, result, "targetPoolId", context.c_str());
                            if (target.has_value())
                            {
                                rule.targetAll = *target == "all";
                                if (!rule.targetAll) addIssue(result, context + " field 'target' must be 'all'.");
                            }
                            if (targetPoolId.has_value())
                            {
                                rule.targetPoolId = *targetPoolId;
                                rule.targetAll = false;
                            }
                            authoring.roundRobinResetRules.push_back(std::move(rule));
                        }
                    }
                }
            }

            if (project.schemaVersion >= 4 && authoring.schemaVersion >= 3)
            {
                const auto groupsIterator = authoringIterator->find("groups");
                if (groupsIterator == authoringIterator->end() || !isObjectArray(*groupsIterator))
                {
                    addIssue(result, "Project authoring field 'groups' must be an array of objects.");
                }
                else
                {
                    authoring.groups.reserve(groupsIterator->size());

                    for (std::size_t index = 0; index < groupsIterator->size(); ++index)
                    {
                        const auto& groupObject = groupsIterator->at(index);
                        const auto context = "ProjectGroup[" + std::to_string(index) + "]";
                        RuntimeProjectGroupDefinition group;

                        if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(groupObject, result, "id", context.c_str()))
                            group.id = *id;
                        if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(groupObject, result, "displayName", context.c_str()))
                            group.displayName = *displayName;
                        if (const auto displayOrder = readRequired<RuntimeProjectLoadResult, int>(groupObject, result, "displayOrder", context.c_str()))
                            group.displayOrder = *displayOrder;
                        if (const auto workspaceVisible = readRequired<RuntimeProjectLoadResult, bool>(groupObject, result, "workspaceVisible", context.c_str()))
                            group.workspaceVisible = *workspaceVisible;
                        if (const auto gainDb = readRequired<RuntimeProjectLoadResult, double>(groupObject, result, "gainDb", context.c_str()))
                            group.gainDb = *gainDb;
                        if (const auto pan = readRequired<RuntimeProjectLoadResult, double>(groupObject, result, "pan", context.c_str()))
                            group.pan = *pan;
                        if (const auto routingBusId = readOptional<RuntimeProjectLoadResult, std::string>(groupObject, result, "routingBusId", context.c_str()))
                            group.routingBusId = *routingBusId;
                        if (const auto auditionAnchorZoneId = readOptional<RuntimeProjectLoadResult, std::string>(groupObject, result, "auditionAnchorZoneId", context.c_str()))
                            group.auditionAnchorZoneId = *auditionAnchorZoneId;

                        authoring.groups.push_back(std::move(group));
                    }
                }
            }

            const auto macrosIterator = authoringIterator->find("macros");
            if (macrosIterator == authoringIterator->end() || !isObjectArray(*macrosIterator))
            {
                addIssue(result, "Project authoring field 'macros' must be an array of objects.");
            }
            else
            {
                authoring.macros.reserve(macrosIterator->size());

                for (std::size_t index = 0; index < macrosIterator->size(); ++index)
                {
                    const auto& macroObject = macrosIterator->at(index);
                    const auto context = "ProjectMacro[" + std::to_string(index) + "]";
                    RuntimeProjectMacroDefinition macro;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(macroObject, result, "id", context.c_str()))
                        macro.id = *id;
                    if (const auto name = readRequired<RuntimeProjectLoadResult, std::string>(macroObject, result, "name", context.c_str()))
                        macro.name = *name;
                    if (const auto defaultValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "defaultValue", context.c_str()))
                        macro.defaultValue = *defaultValue;
                    if (const auto minValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "minValue", context.c_str()))
                        macro.minValue = *minValue;
                    if (const auto maxValue = readRequired<RuntimeProjectLoadResult, double>(macroObject, result, "maxValue", context.c_str()))
                        macro.maxValue = *maxValue;
                    const auto exposedInPerformance = readOptional<RuntimeProjectLoadResult, bool>(
                        macroObject, result, "exposedInPerformance", context.c_str());
                    if (exposedInPerformance.has_value())
                        macro.exposedInPerformance = *exposedInPerformance;
                    else
                        macro.exposedInPerformance = true;

                    const auto targetsIterator = macroObject.find("targets");
                    if (targetsIterator == macroObject.end() || !isObjectArray(*targetsIterator))
                    {
                        addIssue(result, context + " field 'targets' must be an array of objects.");
                    }
                    else
                    {
                        macro.targets.reserve(targetsIterator->size());

                        for (std::size_t targetIndex = 0; targetIndex < targetsIterator->size(); ++targetIndex)
                        {
                            const auto& targetObject = targetsIterator->at(targetIndex);
                            const auto targetContext = context + ".Target[" + std::to_string(targetIndex) + "]";
                            RuntimeProjectMacroTargetDefinition target;

                            if (const auto parameterId = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "parameterId", targetContext.c_str()))
                                target.parameterId = *parameterId;
                            if (const auto parameterPath = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "parameterPath", targetContext.c_str()))
                                target.parameterPath = *parameterPath;
                            if (const auto role = readRequired<RuntimeProjectLoadResult, std::string>(targetObject, result, "role", targetContext.c_str()))
                                target.role = *role;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, std::string>(targetObject, result, "dspSlotId", targetContext.c_str())) target.dspSlotId = *value;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, std::string>(targetObject, result, "dspParameterId", targetContext.c_str())) target.dspParameterId = *value;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, double>(targetObject, result, "sourceMinimum", targetContext.c_str())) target.sourceMinimum = *value;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, double>(targetObject, result, "sourceMaximum", targetContext.c_str())) target.sourceMaximum = *value;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, double>(targetObject, result, "destinationMinimum", targetContext.c_str())) target.destinationMinimum = *value;
                            if (const auto value = readOptional<RuntimeProjectLoadResult, double>(targetObject, result, "destinationMaximum", targetContext.c_str())) target.destinationMaximum = *value;
                            if (const auto controlLaw = targetObject.find("controlLaw"); controlLaw != targetObject.end())
                            {
                                if (!controlLaw->is_object())
                                {
                                    addIssue(result, targetContext + " field 'controlLaw' must be an object.");
                                }
                                else
                                {
                                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(
                                            *controlLaw, result, "id", (targetContext + ".controlLaw").c_str()))
                                        target.controlLaw.id = *id;
                                    if (const auto version = readRequired<RuntimeProjectLoadResult, std::uint32_t>(
                                            *controlLaw, result, "version", (targetContext + ".controlLaw").c_str()))
                                        target.controlLaw.version = *version;
                                }
                            }
                            if (const auto value = readOptional<RuntimeProjectLoadResult, std::string>(targetObject, result, "curve", targetContext.c_str())) target.curve = *value;

                            macro.targets.push_back(std::move(target));
                        }
                    }

                    authoring.macros.push_back(std::move(macro));
                }
            }

            const auto fxSlotsIterator = authoringIterator->find("fxSlots");
            if (fxSlotsIterator == authoringIterator->end() || !isObjectArray(*fxSlotsIterator))
            {
                addIssue(result, "Project authoring field 'fxSlots' must be an array of objects.");
            }
            else
            {
                authoring.fxSlots.reserve(fxSlotsIterator->size());

                for (std::size_t index = 0; index < fxSlotsIterator->size(); ++index)
                {
                    const auto& fxObject = fxSlotsIterator->at(index);
                    const auto context = "ProjectFxSlot[" + std::to_string(index) + "]";
                    RuntimeProjectFxSlotDefinition fxSlot;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "id", context.c_str()))
                        fxSlot.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "displayName", context.c_str()))
                        fxSlot.displayName = *displayName;
                    if (const auto effectType = readRequired<RuntimeProjectLoadResult, std::string>(fxObject, result, "effectType", context.c_str()))
                        fxSlot.effectType = *effectType;
                    if (const auto bypassed = readRequired<RuntimeProjectLoadResult, bool>(fxObject, result, "bypassed", context.c_str()))
                        fxSlot.bypassed = *bypassed;
                    if (project.schemaVersion >= 5)
                    {
                        if (const auto version = readRequired<RuntimeProjectLoadResult, std::uint32_t>(fxObject, result, "effectVersion", context.c_str()))
                            fxSlot.effectVersion = *version;
                        if (const auto legacyInert = readRequired<RuntimeProjectLoadResult, bool>(fxObject, result, "legacyInert", context.c_str())) fxSlot.legacyInert = *legacyInert;
                        const auto parameters = fxObject.find("parameters");
                        if (parameters == fxObject.end() || !isObjectArray(*parameters))
                            addIssue(result, context + " field 'parameters' must be an array of objects.");
                        else for (std::size_t parameterIndex = 0; parameterIndex < parameters->size(); ++parameterIndex)
                        {
                            const auto& parameter = parameters->at(parameterIndex);
                            RuntimeProjectFxSlotDefinition::ParameterValue value;
                            const auto parameterContext = context + ".parameters[" + std::to_string(parameterIndex) + "]";
                            if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(parameter, result, "id", parameterContext.c_str())) value.id = *id;
                            if (const auto numeric = readRequired<RuntimeProjectLoadResult, double>(parameter, result, "value", parameterContext.c_str())) value.value = *numeric;
                            fxSlot.parameters.push_back(std::move(value));
                        }
                        if (findCuratedDspEffect(fxSlot.effectType, fxSlot.effectVersion) == nullptr)
                        {
                            fxSlot.unavailable = true;
                            result.warnings.push_back(context + " effect '" + fxSlot.effectType
                                + "' version " + std::to_string(fxSlot.effectVersion)
                                + " is unavailable and will be bypassed at runtime.");
                        }
                    }

                    authoring.fxSlots.push_back(std::move(fxSlot));
                }
            }

            const auto routingBusesIterator = authoringIterator->find("routingBuses");
            if (routingBusesIterator == authoringIterator->end() || !isObjectArray(*routingBusesIterator))
            {
                addIssue(result, "Project authoring field 'routingBuses' must be an array of objects.");
            }
            else
            {
                authoring.routingBuses.reserve(routingBusesIterator->size());

                for (std::size_t index = 0; index < routingBusesIterator->size(); ++index)
                {
                    const auto& busObject = routingBusesIterator->at(index);
                    const auto context = "ProjectRoutingBus[" + std::to_string(index) + "]";
                    RuntimeProjectRoutingBusDefinition bus;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "id", context.c_str()))
                        bus.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "displayName", context.c_str()))
                        bus.displayName = *displayName;
                    if (const auto inputSourceId = readRequired<RuntimeProjectLoadResult, std::string>(busObject, result, "inputSourceId", context.c_str()))
                    {
                        bus.inputSourceId = *inputSourceId;
                        if (project.schemaVersion >= 5 && bus.inputSourceId != "master"
                            && bus.inputSourceId.rfind("zones/", 0) != 0
                            && bus.inputSourceId.rfind("groups/", 0) != 0)
                            bus.inputSourceId = "zones/" + bus.inputSourceId;
                    }
                    bus.fxSlotIds = readRequiredStringArray(busObject, result, "fxSlotIds", context.c_str());
                    if (project.schemaVersion >= 5)
                        if (const auto bypassed = readRequired<RuntimeProjectLoadResult, bool>(busObject, result, "chainBypassed", context.c_str())) bus.chainBypassed = *bypassed;

                    authoring.routingBuses.push_back(std::move(bus));
                }
            }

            const auto performanceBanksIterator = authoringIterator->find("performanceBanks");
            if (performanceBanksIterator == authoringIterator->end() || !isObjectArray(*performanceBanksIterator))
            {
                addIssue(result, "Project authoring field 'performanceBanks' must be an array of objects.");
            }
            else
            {
                authoring.performanceBanks.reserve(performanceBanksIterator->size());

                for (std::size_t index = 0; index < performanceBanksIterator->size(); ++index)
                {
                    const auto& bankObject = performanceBanksIterator->at(index);
                    const auto context = "ProjectPerformanceBank[" + std::to_string(index) + "]";
                    RuntimeProjectPerformanceBankDefinition bank;

                    if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(bankObject, result, "id", context.c_str()))
                        bank.id = *id;
                    if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(bankObject, result, "displayName", context.c_str()))
                        bank.displayName = *displayName;

                    const auto triggerSlotsIterator = bankObject.find("triggerSlots");
                    if (triggerSlotsIterator == bankObject.end() || !isObjectArray(*triggerSlotsIterator))
                    {
                        addIssue(result, context + " field 'triggerSlots' must be an array of objects.");
                    }
                    else
                    {
                        bank.triggerSlots.reserve(triggerSlotsIterator->size());

                        for (std::size_t triggerIndex = 0; triggerIndex < triggerSlotsIterator->size(); ++triggerIndex)
                        {
                            const auto& slotObject = triggerSlotsIterator->at(triggerIndex);
                            const auto slotContext = context + ".TriggerSlot[" + std::to_string(triggerIndex) + "]";
                            RuntimeProjectTriggerSlotDefinition slot;

                            if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "id", slotContext.c_str()))
                                slot.id = *id;
                            if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "displayName", slotContext.c_str()))
                                slot.displayName = *displayName;
                            if (const auto triggerEvent = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "triggerEvent", slotContext.c_str()))
                                slot.triggerEvent = *triggerEvent;
                            if (const auto targetArticulationId = readRequired<RuntimeProjectLoadResult, std::string>(slotObject, result, "targetArticulationId", slotContext.c_str()))
                                slot.targetArticulationId = *targetArticulationId;
                            if (const auto phraseAssetId = readOptional<RuntimeProjectLoadResult, std::string>(slotObject, result, "phraseAssetId", slotContext.c_str()))
                                slot.phraseAssetId = *phraseAssetId;
                            if (const auto chordMode = readOptional<RuntimeProjectLoadResult, std::string>(slotObject, result, "chordMode", slotContext.c_str()))
                                slot.chordMode = *chordMode;

                            bank.triggerSlots.push_back(std::move(slot));
                        }
                    }

                    const auto phraseAssetsIterator = bankObject.find("phraseAssets");
                    if (phraseAssetsIterator != bankObject.end())
                    {
                        if (!isObjectArray(*phraseAssetsIterator))
                        {
                            addIssue(result, context + " field 'phraseAssets' must be an array of objects.");
                        }
                        else
                        {
                            bank.phraseAssets.reserve(phraseAssetsIterator->size());

                            for (std::size_t phraseIndex = 0; phraseIndex < phraseAssetsIterator->size(); ++phraseIndex)
                            {
                                const auto& phraseObject = phraseAssetsIterator->at(phraseIndex);
                                const auto phraseContext = context + ".PhraseAsset[" + std::to_string(phraseIndex) + "]";
                                RuntimeProjectPhraseAssetDefinition phraseAsset;

                                if (const auto id = readRequired<RuntimeProjectLoadResult, std::string>(phraseObject, result, "id", phraseContext.c_str()))
                                    phraseAsset.id = *id;
                                if (const auto displayName = readRequired<RuntimeProjectLoadResult, std::string>(phraseObject, result, "displayName", phraseContext.c_str()))
                                    phraseAsset.displayName = *displayName;
                                if (const auto sourcePath = readRequired<RuntimeProjectLoadResult, std::string>(phraseObject, result, "sourcePath", phraseContext.c_str()))
                                    phraseAsset.sourcePath = *sourcePath;
                                if (const auto ticksPerQuarter = readRequired<RuntimeProjectLoadResult, int>(phraseObject, result, "ticksPerQuarter", phraseContext.c_str()))
                                    phraseAsset.ticksPerQuarter = *ticksPerQuarter;
                                if (const auto lengthBeats = readRequired<RuntimeProjectLoadResult, double>(phraseObject, result, "lengthBeats", phraseContext.c_str()))
                                    phraseAsset.lengthBeats = *lengthBeats;
                                if (const auto chordHint = readRequired<RuntimeProjectLoadResult, std::string>(phraseObject, result, "chordHint", phraseContext.c_str()))
                                    phraseAsset.chordHint = *chordHint;
                                if (const auto normalizationState = readRequired<RuntimeProjectLoadResult, std::string>(phraseObject, result, "normalizationState", phraseContext.c_str()))
                                    phraseAsset.normalizationState = *normalizationState;
                                phraseAsset.issues = readRequiredStringArray(phraseObject, result, "issues", phraseContext.c_str());

                                const auto notesIterator = phraseObject.find("notes");
                                if (notesIterator == phraseObject.end() || !isObjectArray(*notesIterator))
                                {
                                    addIssue(result, phraseContext + " field 'notes' must be an array of objects.");
                                }
                                else
                                {
                                    phraseAsset.notes.reserve(notesIterator->size());

                                    for (std::size_t noteIndex = 0; noteIndex < notesIterator->size(); ++noteIndex)
                                    {
                                        const auto& noteObject = notesIterator->at(noteIndex);
                                        const auto noteContext = phraseContext + ".Note[" + std::to_string(noteIndex) + "]";
                                        RuntimeProjectPhraseNoteDefinition phraseNote;

                                        if (const auto midiNote = readRequired<RuntimeProjectLoadResult, int>(noteObject, result, "midiNote", noteContext.c_str()))
                                            phraseNote.midiNote = *midiNote;
                                        if (const auto velocity = readRequired<RuntimeProjectLoadResult, int>(noteObject, result, "velocity", noteContext.c_str()))
                                            phraseNote.velocity = *velocity;
                                        if (const auto startBeat = readRequired<RuntimeProjectLoadResult, double>(noteObject, result, "startBeat", noteContext.c_str()))
                                            phraseNote.startBeat = *startBeat;
                                        if (const auto durationBeats = readRequired<RuntimeProjectLoadResult, double>(noteObject, result, "durationBeats", noteContext.c_str()))
                                            phraseNote.durationBeats = *durationBeats;

                                        phraseAsset.notes.push_back(std::move(phraseNote));
                                    }
                                }

                                bank.phraseAssets.push_back(std::move(phraseAsset));
                            }
                        }
                    }

                    bank.notes = readRequiredStringArray(bankObject, result, "notes", context.c_str());
                    authoring.performanceBanks.push_back(std::move(bank));
                }
            }

            authoring.notes = readRequiredStringArray(*authoringIterator, result, "notes", "Project authoring");
        }
    }

    const auto validation = validateRuntimeProjectModel(project);
    result.issues.insert(result.issues.end(), validation.issues.begin(), validation.issues.end());

    result.loaded = result.issues.empty();
    result.state = result.loaded ? "Project loaded" : "Project invalid";
    return result;
}

RuntimeProjectLoadResult loadRuntimeProjectManifest(const std::string& manifestPath)
{
    RuntimeProjectLoadResult result;
    result.manifestPath = manifestPath;
    result.state = "Project load not attempted";

    const fs::path manifestFsPath(manifestPath);
    std::error_code errorCode;

    if (!fs::exists(manifestFsPath, errorCode))
    {
        result.state = "Project missing";
        addIssue(result, "Project file was not found at " + manifestPath + ".");
        return result;
    }

    const auto rawText = readTextFile(manifestFsPath);
    if (rawText.empty())
    {
        result.manifestFound = true;
        result.state = "Project unreadable";
        addIssue(result, "Project file was empty or unreadable.");
        return result;
    }

    return parseRuntimeProjectManifest(rawText, manifestPath, true);
}

RuntimeProjectLoadResult loadPhase1ReferenceProjectManifest()
{
    return loadRuntimeProjectManifest(getPhase1ReferenceProjectManifestPath());
}

RuntimeProjectLoadResult loadPhase2ReferenceProjectManifest()
{
    return loadRuntimeProjectManifest(getPhase2ReferenceProjectManifestPath());
}

RuntimeProjectValidationResult validateRuntimeProjectModel(const RuntimeProjectModel& project)
{
    RuntimeProjectValidationResult result;
    result.state = "Project validation failed";

    if (project.schemaName != "drs.project")
        addIssue(result, "Project schemaName must be 'drs.project'.");

    if (project.schemaVersion != 1 && project.schemaVersion != 2 && project.schemaVersion != 3
        && project.schemaVersion != 4 && project.schemaVersion != 5 && project.schemaVersion != 6)
    {
        addIssue(result, "Project schemaVersion must be 1, 2, 3, 4, 5, or 6.");
    }

    if (project.projectId.empty())
        addIssue(result, "Project projectId must not be empty.");

    if (project.displayName.empty())
        addIssue(result, "Project displayName must not be empty.");

    if (project.contentRootPath.empty())
        addIssue(result, "Project contentRootPath must not be empty.");

    if (project.defaultInstrumentManifestPath.empty())
        addIssue(result, "Project defaultInstrumentManifestPath must not be empty.");

    if (project.schemaVersion == 1 && project.sampleSources.empty())
        addIssue(result, "Project must declare at least one sample source.");

    {
        std::unordered_set<std::string> sampleSourceIds;
        for (const auto& sampleSource : project.sampleSources)
        {
            if (sampleSource.id.empty())
                addIssue(result, "Project sample sources must have non-empty ids.");
            else if (!sampleSourceIds.insert(sampleSource.id).second)
                addIssue(result, "Project sample source ids must be unique; duplicate '" + sampleSource.id + "'.");

            if (sampleSource.path.empty())
                addIssue(result, "Project sample source '" + sampleSource.id + "' must have a non-empty path.");

            if (sampleSource.role.empty())
                addIssue(result, "Project sample source '" + sampleSource.id + "' must have a non-empty role.");
        }
    }

    if (project.schemaVersion >= 2 && project.schemaVersion <= 6)
    {
        const auto& authoring = project.authoring;
        const auto explicitRoundRobinRequired = project.schemaVersion >= 3;
        const auto explicitGroupsRequired = project.schemaVersion >= 4;

        if (authoring.schemaName != "drs.authoring")
            addIssue(result, "Project authoring schemaName must be 'drs.authoring'.");

        if (project.schemaVersion == 2 && authoring.schemaVersion != 1)
            addIssue(result, "Project authoring schemaVersion must be 1 for schemaVersion 2 projects.");

        if (project.schemaVersion == 3 && authoring.schemaVersion != 2)
            addIssue(result, "Project authoring schemaVersion must be 2 for schemaVersion 3 projects.");

        if (project.schemaVersion == 4 && authoring.schemaVersion != 3)
            addIssue(result, "Project authoring schemaVersion must be 3 for schemaVersion 4 projects.");
        if (project.schemaVersion == 5 && authoring.schemaVersion != 4)
            addIssue(result, "Project authoring schemaVersion must be 4 for schemaVersion 5 projects.");
        if (project.schemaVersion == 6 && authoring.schemaVersion != 5)
            addIssue(result, "Project authoring schemaVersion must be 5 for schemaVersion 6 projects.");

        if (hasDuplicateIds(authoring.zones))
            addIssue(result, "Project authoring zone ids must be unique.");

        const auto explicitArticulationsRequired = project.schemaVersion >= 6;
        if (explicitArticulationsRequired && hasDuplicateIds(authoring.articulations))
            addIssue(result, "Project authoring articulation ids must be unique.");

        if (explicitGroupsRequired && hasDuplicateIds(authoring.groups))
            addIssue(result, "Project authoring group ids must be unique.");

        if (hasDuplicateIds(authoring.macros))
            addIssue(result, "Project authoring macro ids must be unique.");

        if (hasDuplicateIds(authoring.fxSlots))
            addIssue(result, "Project authoring FX slot ids must be unique.");

        if (hasDuplicateIds(authoring.routingBuses))
            addIssue(result, "Project authoring routing bus ids must be unique.");

        if (hasDuplicateIds(authoring.performanceBanks))
            addIssue(result, "Project authoring performance bank ids must be unique.");

        std::unordered_set<std::string> sampleSourceIds;
        for (const auto& sampleSource : project.sampleSources)
            sampleSourceIds.insert(sampleSource.id);

        std::unordered_map<std::string, std::string> zoneGroupIds;
        std::unordered_set<std::string> groupIds;
        if (explicitGroupsRequired)
        {
            for (const auto& group : authoring.groups)
            {
                if (group.id.empty())
                    addIssue(result, "Project groups must have non-empty ids.");
                else
                    groupIds.insert(group.id);

                if (group.displayName.empty())
                    addIssue(result, "Project group '" + group.id + "' must have a displayName.");

                if (group.displayOrder < 0)
                    addIssue(result, "Project group '" + group.id + "' must not have a negative displayOrder.");
            }
        }

        std::unordered_set<std::string> zoneIds;
        std::unordered_set<std::string> articulationIds;
        std::size_t defaultArticulationCount = 0;
        if (explicitArticulationsRequired)
        {
            if (authoring.articulations.empty())
                addIssue(result, "Project authoring schema 5 must declare at least one articulation.");
            for (const auto& articulation : authoring.articulations)
            {
                if (articulation.id.empty())
                    addIssue(result, "Project articulations must have non-empty ids.");
                else
                    articulationIds.insert(articulation.id);
                if (articulation.displayName.empty())
                    addIssue(result, "Project articulation '" + articulation.id + "' must have a displayName.");
                if (articulation.displayOrder < 0)
                    addIssue(result, "Project articulation '" + articulation.id + "' must not have a negative displayOrder.");
                if (articulation.isDefault)
                    ++defaultArticulationCount;
            }
            if (defaultArticulationCount != 1)
                addIssue(result, "Project authoring schema 5 must declare exactly one default articulation.");
            if (authoring.articulations.size() > 64)
                addIssue(result, "Project authoring articulation count exceeds the 64-item limit.");
        }
        for (const auto& zone : authoring.zones)
        {
            if (!zone.id.empty())
            {
                zoneIds.insert(zone.id);
                zoneGroupIds.emplace(zone.id, zone.groupId);
            }

            if (zone.sampleSourceId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must reference a sampleSourceId.");
            else if (!sampleSourceIds.count(zone.sampleSourceId))
                addIssue(result, "Project zone '" + zone.id + "' references unknown sampleSourceId '" + zone.sampleSourceId + "'.");

            if (zone.displayName.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have a displayName.");

            if (zone.groupId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have a groupId.");
            else if (explicitGroupsRequired && !groupIds.count(zone.groupId))
                addIssue(result, "Project zone '" + zone.id + "' references unknown groupId '" + zone.groupId + "'.");

            if (zone.articulationId.empty())
                addIssue(result, "Project zone '" + zone.id + "' must have an articulationId.");
            else if (explicitArticulationsRequired && !articulationIds.count(zone.articulationId))
                addIssue(result, "Project zone '" + zone.id + "' references unknown articulationId '" + zone.articulationId + "'.");

            if (zone.keyLow > zone.keyHigh)
                addIssue(result, "Project zone '" + zone.id + "' has keyLow greater than keyHigh.");

            if (zone.velocityLow > zone.velocityHigh)
                addIssue(result, "Project zone '" + zone.id + "' has velocityLow greater than velocityHigh.");

            if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            {
                const auto crossfadeIssue = validateFirstPassVelocityCrossfadeZone(
                    buildVelocityCrossfadeValidationZone(zone.velocityLow, zone.velocityHigh, zone.velocityCrossfade));
                if (crossfadeIssue != VelocityCrossfadeZoneIssue::none)
                    addIssue(result, buildVelocityCrossfadeIssue("Project zone '" + zone.id + "'", crossfadeIssue));
            }

            if (zone.loopEnabled && zone.loopStartFrame > zone.loopEndFrame)
                addIssue(result, "Project zone '" + zone.id + "' has loopStartFrame greater than loopEndFrame.");

            if (zone.releaseSeconds < 0.0)
                addIssue(result, "Project zone '" + zone.id + "' must not have a negative releaseSeconds.");

            validateRoundRobinDescriptor(result,
                                         "Project zone '" + zone.id + "'",
                                         zone.roundRobin,
                                         zone.roundRobinLength,
                                         zone.roundRobinPosition,
                                         explicitRoundRobinRequired,
                                         explicitRoundRobinRequired);
        }

        for (const auto& finding : collectVelocityCrossfadeTopologyFindings(authoring.zones))
        {
            if (finding.zoneIndex >= authoring.zones.size())
                continue;

            addIssue(result,
                     buildVelocityCrossfadeTopologyIssue("Project zone '" + authoring.zones[finding.zoneIndex].id + "'",
                                                         finding.issue));
        }

        if (!authoring.selectedZoneId.empty() && !zoneIds.count(authoring.selectedZoneId))
            addIssue(result, "Project authoring selectedZoneId references unknown zone '" + authoring.selectedZoneId + "'.");

        if (explicitGroupsRequired && !authoring.selectedGroupId.empty() && !groupIds.count(authoring.selectedGroupId))
            addIssue(result,
                     "Project authoring selectedGroupId references unknown group '" + authoring.selectedGroupId + "'.");

        std::unordered_set<std::string> fxSlotIds;
        std::size_t totalDspParameterCount = 0;
        for (const auto& fxSlot : authoring.fxSlots)
        {
            if (fxSlot.id.empty())
                addIssue(result, "Project FX slots must have non-empty ids.");
            else
                fxSlotIds.insert(fxSlot.id);

            if (fxSlot.displayName.empty())
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have a displayName.");

            if (fxSlot.effectType.empty())
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have an effectType.");

            if (project.schemaVersion >= 5 && fxSlot.effectVersion == 0)
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have a non-zero effectVersion.");

            std::unordered_set<std::string> parameterIds;
            for (const auto& parameter : fxSlot.parameters)
            {
                ++totalDspParameterCount;
                if (parameter.id.empty())
                    addIssue(result, "Project FX slot '" + fxSlot.id + "' contains a parameter without id.");
                else if (!parameterIds.insert(parameter.id).second)
                    addIssue(result, "Project FX slot '" + fxSlot.id + "' contains duplicate parameter id '" + parameter.id + "'.");
                if (!std::isfinite(parameter.value))
                    addIssue(result, "Project FX slot '" + fxSlot.id + "' parameter '" + parameter.id + "' must be finite.");
            }
        }
        if (totalDspParameterCount > 1024)
            addIssue(result, "Project authored DSP parameter count exceeds the 1024-item limit.");

        for (const auto& macro : authoring.macros)
        {
            if (macro.id.empty())
                addIssue(result, "Project macros must have non-empty ids.");

            if (macro.name.empty())
                addIssue(result, "Project macro '" + macro.id + "' must have a name.");

            if (!std::isfinite(macro.minValue) || !std::isfinite(macro.maxValue) || !std::isfinite(macro.defaultValue))
                addIssue(result, "Project macro '" + macro.id + "' must have finite minValue, maxValue, and defaultValue.");

            if (macro.minValue > macro.maxValue)
                addIssue(result, "Project macro '" + macro.id + "' has minValue greater than maxValue.");

            if (macro.defaultValue < macro.minValue || macro.defaultValue > macro.maxValue)
                addIssue(result, "Project macro '" + macro.id + "' has defaultValue outside minValue/maxValue.");

            for (const auto& target : macro.targets)
            {
                if (target.parameterId.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without parameterId.");

                if (target.parameterPath.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without parameterPath.");

                if (target.role.empty())
                    addIssue(result, "Project macro '" + macro.id + "' contains a target without role.");
                const auto hasDspIdentity = !target.dspSlotId.empty() || !target.dspParameterId.empty();
                if (hasDspIdentity && (target.dspSlotId.empty() || target.dspParameterId.empty()
                    || !std::isfinite(target.sourceMinimum) || !std::isfinite(target.sourceMaximum)
                    || !std::isfinite(target.destinationMinimum) || !std::isfinite(target.destinationMaximum)
                    || target.sourceMinimum >= target.sourceMaximum || target.destinationMinimum > target.destinationMaximum
                    || (target.curve != "linear" && target.curve != "logarithmic")
                    || (target.curve == "logarithmic"
                        && (target.destinationMinimum <= 0.0 || target.destinationMaximum <= 0.0))))
                    addIssue(result, "Project macro '" + macro.id + "' contains an invalid structured DSP target.");
                if (hasDspIdentity && ((!target.controlLaw.id.empty() && target.controlLaw.version == 0)
                    || (target.controlLaw.id.empty() && target.controlLaw.version != 0)))
                    addIssue(result, "Project macro '" + macro.id + "' contains an incomplete control-law identity.");
                if (hasDspIdentity && !target.controlLaw.id.empty()
                    && target.controlLaw.version == 1)
                {
                    CompiledControlLaw compiledLaw;
                    if (!compileControlLaw(target.controlLaw.id, target.destinationMinimum,
                                           target.destinationMaximum, compiledLaw))
                        addIssue(result, "Project macro '" + macro.id + "' contains an unsupported or incompatible control law.");
                }
            }
        }

        std::unordered_set<std::string> performanceBankIds;
        for (const auto& bank : authoring.performanceBanks)
        {
            if (!bank.id.empty())
                performanceBankIds.insert(bank.id);

            if (bank.displayName.empty())
                addIssue(result, "Project performance bank '" + bank.id + "' must have a displayName.");

            std::unordered_set<std::string> phraseAssetIds;
            for (const auto& phraseAsset : bank.phraseAssets)
            {
                if (phraseAsset.id.empty())
                    addIssue(result, "Project performance bank '" + bank.id + "' contains a phrase asset without id.");
                else if (!phraseAssetIds.insert(phraseAsset.id).second)
                    addIssue(result, "Project performance bank '" + bank.id + "' contains duplicate phrase asset id '" + phraseAsset.id + "'.");

                if (phraseAsset.displayName.empty())
                    addIssue(result, "Project phrase asset '" + phraseAsset.id + "' must have a displayName.");

                if (phraseAsset.sourcePath.empty())
                    addIssue(result, "Project phrase asset '" + phraseAsset.id + "' must record its sourcePath.");

                if (phraseAsset.ticksPerQuarter <= 0)
                    addIssue(result, "Project phrase asset '" + phraseAsset.id + "' must have a positive ticksPerQuarter.");

                if (phraseAsset.lengthBeats < 0.0)
                    addIssue(result, "Project phrase asset '" + phraseAsset.id + "' cannot have a negative lengthBeats.");

                for (const auto& phraseNote : phraseAsset.notes)
                {
                    if (phraseNote.midiNote < 0 || phraseNote.midiNote > 127)
                        addIssue(result, "Project phrase asset '" + phraseAsset.id + "' contains a midiNote outside 0-127.");

                    if (phraseNote.velocity < 1 || phraseNote.velocity > 127)
                        addIssue(result, "Project phrase asset '" + phraseAsset.id + "' contains a velocity outside 1-127.");

                    if (phraseNote.startBeat < 0.0)
                        addIssue(result, "Project phrase asset '" + phraseAsset.id + "' contains a negative startBeat.");

                    if (phraseNote.durationBeats <= 0.0)
                        addIssue(result, "Project phrase asset '" + phraseAsset.id + "' contains a non-positive durationBeats.");
                }
            }

            std::unordered_set<std::string> triggerSlotIds;
            for (const auto& triggerSlot : bank.triggerSlots)
            {
                if (triggerSlot.id.empty())
                    addIssue(result, "Project performance bank '" + bank.id + "' contains a trigger slot without id.");
                else if (!triggerSlotIds.insert(triggerSlot.id).second)
                    addIssue(result, "Project performance bank '" + bank.id + "' contains duplicate trigger slot id '" + triggerSlot.id + "'.");

                if (triggerSlot.displayName.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a displayName.");

                if (triggerSlot.triggerEvent.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a triggerEvent.");

                if (triggerSlot.targetArticulationId.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must have a targetArticulationId.");
                else if (explicitArticulationsRequired && !articulationIds.count(triggerSlot.targetArticulationId))
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' references unknown targetArticulationId '" + triggerSlot.targetArticulationId + "'.");

                if (triggerSlot.triggerEvent == "phrase-trigger" && triggerSlot.phraseAssetId.empty())
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' must reference a phraseAssetId when triggerEvent is 'phrase-trigger'.");

                if (!triggerSlot.phraseAssetId.empty() && !phraseAssetIds.count(triggerSlot.phraseAssetId))
                    addIssue(result, "Project trigger slot '" + triggerSlot.id + "' references unknown phraseAssetId '" + triggerSlot.phraseAssetId + "'.");
            }
        }

        if (!authoring.selectedPerformanceBankId.empty() && !performanceBankIds.count(authoring.selectedPerformanceBankId))
            addIssue(result,
                     "Project authoring selectedPerformanceBankId references unknown bank '"
                         + authoring.selectedPerformanceBankId + "'.");

        std::unordered_set<std::string> routingBusIds;
        std::unordered_set<std::string> groupOwnedRoutingBusIds;
        std::unordered_map<std::string, std::size_t> fxSlotOwnerCounts;
        std::unordered_set<std::string> routingSourceOwners;
        for (const auto& bus : authoring.routingBuses)
        {
            if (bus.id.empty())
                addIssue(result, "Project routing buses must have non-empty ids.");
            else
                routingBusIds.insert(bus.id);

            if (bus.displayName.empty())
                addIssue(result, "Project routing bus '" + bus.id + "' must have a displayName.");

            if (bus.inputSourceId.empty())
                addIssue(result, "Project routing bus '" + bus.id + "' must have an inputSourceId.");
            else if (!routingSourceOwners.insert(bus.inputSourceId).second)
                addIssue(result, "Project routing source '" + bus.inputSourceId + "' must have exactly one chain owner.");
            else if (bus.inputSourceId != "master" && !zoneIds.count(bus.inputSourceId)
                     && !zoneIds.count(extractZoneIdFromRoutingSourceId(bus.inputSourceId)))
            {
                const auto groupId = extractGroupIdFromRoutingSourceId(bus.inputSourceId);
                if (groupId.empty() || !explicitGroupsRequired || !groupIds.count(groupId))
                {
                    addIssue(result,
                             "Project routing bus '" + bus.id + "' references unknown inputSourceId '"
                                 + bus.inputSourceId + "'.");
                }
            }

            for (const auto& fxSlotId : bus.fxSlotIds)
            {
                if (!fxSlotIds.count(fxSlotId))
                    addIssue(result, "Project routing bus '" + bus.id + "' references unknown FX slot '" + fxSlotId + "'.");
                else
                    ++fxSlotOwnerCounts[fxSlotId];
            }
        }

        for (const auto& fxSlot : authoring.fxSlots)
            if (fxSlotOwnerCounts[fxSlot.id] != 1)
                addIssue(result, "Project FX slot '" + fxSlot.id + "' must have exactly one chain owner.");

        if (explicitGroupsRequired)
        {
            for (const auto& group : authoring.groups)
            {
                if (!group.routingBusId.empty() && !routingBusIds.count(group.routingBusId))
                {
                    addIssue(result,
                             "Project group '" + group.id + "' references unknown routingBusId '"
                                 + group.routingBusId + "'.");
                }
                else if (!group.routingBusId.empty())
                {
                    const auto busIterator = std::find_if(authoring.routingBuses.begin(),
                                                          authoring.routingBuses.end(),
                                                          [&](const auto& bus)
                                                          {
                                                              return bus.id == group.routingBusId;
                                                          });
                    if (busIterator != authoring.routingBuses.end())
                    {
                        const auto expectedInputSourceId = "groups/" + group.id;
                        if (busIterator->inputSourceId != expectedInputSourceId)
                        {
                            addIssue(result,
                                     "Project group '" + group.id + "' must reference a routingBusId whose inputSourceId is '"
                                         + expectedInputSourceId + "'.");
                        }
                        else if (!groupOwnedRoutingBusIds.insert(group.routingBusId).second)
                        {
                            addIssue(result,
                                     "Project routingBusId '" + group.routingBusId
                                         + "' must not be assigned to more than one group.");
                        }
                    }
                }

                if (!group.auditionAnchorZoneId.empty())
                {
                    const auto zoneIterator = zoneGroupIds.find(group.auditionAnchorZoneId);
                    if (zoneIterator == zoneGroupIds.end())
                    {
                        addIssue(result,
                                 "Project group '" + group.id + "' references unknown auditionAnchorZoneId '"
                                     + group.auditionAnchorZoneId + "'.");
                    }
                    else if (zoneIterator->second != group.id)
                    {
                        addIssue(result,
                                 "Project group '" + group.id + "' auditionAnchorZoneId '"
                                     + group.auditionAnchorZoneId + "' must belong to the same group.");
                    }
                }
            }

            for (const auto& bus : authoring.routingBuses)
            {
                const auto groupId = extractGroupIdFromRoutingSourceId(bus.inputSourceId);
                if (!groupId.empty() && !groupOwnedRoutingBusIds.count(bus.id))
                {
                    addIssue(result,
                             "Project routing bus '" + bus.id + "' targets group source '"
                                 + bus.inputSourceId + "' but no group claims that routingBusId.");
                }
            }
        }
    }

    if (project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5)
    {
        const auto performanceRules = validatePerformanceRuleDeclarations(project.authoring);
        for (const auto& finding : performanceRules.findings)
            addIssue(result, "[" + finding.code + "] " + finding.path + ": " + finding.message
                           + " Repair: " + finding.repair);
    }

    if (result.issues.empty())
    {
        result.valid = true;
        result.state = "Project validated";
    }

    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToPhase2Authoring(const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    result.state = "Project migration failed";

    if (project.schemaVersion == 2)
    {
        result.project = project;
        const auto validation = validateRuntimeProjectModel(result.project);
        result.issues = validation.issues;
        result.valid = validation.valid;
        result.state = validation.valid ? "Project already uses the Phase 2 authoring schema" : validation.state;
        return result;
    }

    if (project.schemaVersion != 1)
    {
        addIssue(result, "Only Project schemaVersion 1 can be migrated into the Phase 2 authoring schema.");
        return result;
    }

    result.project = project;
    result.project.schemaVersion = 2;
    result.project.authoring = buildDefaultPhase2AuthoringState();
    result.project.authoring.notes = {
        "Migrated from the Phase 1 runtime project manifest into the Phase 2 authoring schema.",
        "Authoring zones, macro targets, routing, and performance-bank placeholders can now be edited and saved in-project."
    };

    const auto validation = validateRuntimeProjectModel(result.project);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid ? "Project migrated to the Phase 2 authoring schema" : validation.state;
    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToPhase3RoundRobinSchema(const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    result.state = "Project round-robin migration failed";

    if (project.schemaVersion == 3 && project.authoring.schemaVersion == 2)
    {
        result.project = project;
        const auto validation = validateRuntimeProjectModel(result.project);
        result.issues = validation.issues;
        result.valid = validation.valid;
        result.state = validation.valid
            ? "Project already uses the Phase 3 Round Robin schema"
            : validation.state;
        return result;
    }

    if (project.schemaVersion != 2 || project.authoring.schemaVersion != 1)
    {
        addIssue(result, "Only Project schemaVersion 2 with authoring schemaVersion 1 can be migrated into the Phase 3 Round Robin schema.");
        return result;
    }

    result.project = project;
    result.project.schemaVersion = 3;
    result.project.authoring.schemaVersion = 2;

    for (auto& zone : result.project.authoring.zones)
    {
        if (zone.roundRobin.has_value())
        {
            applyRoundRobinDescriptor(zone, *zone.roundRobin);
            continue;
        }

        if (const auto synthesizedRoundRobin = synthesizeRoundRobinFromLegacyScalars(zone))
            applyRoundRobinDescriptor(zone, *synthesizedRoundRobin);
    }

    const auto validation = validateRuntimeProjectModel(result.project);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid
        ? "Project migrated to the Phase 3 Round Robin schema"
        : validation.state;
    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToZoneGroupsSchema(const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    result.state = "Project zone-group migration failed";

    if (project.schemaVersion == 4 && project.authoring.schemaVersion == 3)
    {
        result.project = project;
        const auto validation = validateRuntimeProjectModel(result.project);
        result.issues = validation.issues;
        result.valid = validation.valid;
        result.state = validation.valid
            ? "Project already uses the Zone Groups schema"
            : validation.state;
        return result;
    }

    RuntimeProjectModel migratedProject = project;

    if (migratedProject.schemaVersion == 1)
    {
        const auto phase2Migration = migrateRuntimeProjectToPhase2Authoring(migratedProject);
        if (!phase2Migration.valid)
        {
            result.issues = phase2Migration.issues;
            return result;
        }

        migratedProject = phase2Migration.project;
    }

    if (migratedProject.schemaVersion == 2 && migratedProject.authoring.schemaVersion == 1)
    {
        const auto phase3Migration = migrateRuntimeProjectToPhase3RoundRobinSchema(migratedProject);
        if (!phase3Migration.valid)
        {
            result.issues = phase3Migration.issues;
            return result;
        }

        migratedProject = phase3Migration.project;
    }

    if (migratedProject.schemaVersion != 3 || migratedProject.authoring.schemaVersion != 2)
    {
        addIssue(result,
                 "Only Project schemaVersion 3 with authoring schemaVersion 2 can be migrated into the Zone Groups schema.");
        return result;
    }

    migratedProject.schemaVersion = 4;
    migratedProject.authoring.schemaVersion = 3;
    migratedProject.authoring.groups = synthesizeProjectGroupsFromZones(migratedProject.authoring.zones);
    migratedProject.authoring.selectedGroupId =
        resolveSelectedGroupIdFromSelectedZone(migratedProject.authoring);

    const auto validation = validateRuntimeProjectModel(migratedProject);
    result.project = std::move(migratedProject);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid
        ? "Project migrated to the Zone Groups schema"
        : validation.state;
    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToCuratedDspSchema(const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    auto migrated = project;
    if (migrated.schemaVersion < 4)
    {
        const auto groups = migrateRuntimeProjectToZoneGroupsSchema(migrated);
        if (!groups.valid) return groups;
        migrated = groups.project;
    }
    if (migrated.schemaVersion != 4 || migrated.authoring.schemaVersion != 3)
    {
        addIssue(result, "Only schema-4 / authoring-3 projects can migrate to curated DSP schema 5.");
        return result;
    }
    for (auto& slot : migrated.authoring.fxSlots)
    {
        const auto alreadyUsesCuratedIdentity = slot.effectType.rfind("drs.", 0) == 0;
        if (slot.effectType == "delay") slot.effectType = "drs.stereoDelay";
        else if (slot.effectType == "reverb") slot.effectType = "drs.algorithmicReverb";
        else if (slot.effectType == "saturator") slot.effectType = "drs.saturator";
        else if (slot.effectType == "gain") slot.effectType = "drs.gain";
        else if (slot.effectType == "eq") slot.effectType = "drs.compactEq";
        else if (slot.effectType == "chorus") slot.effectType = "drs.chorus";
        slot.effectVersion = 1;
        slot.parameters.clear();
        if (alreadyUsesCuratedIdentity)
        {
            if (const auto* descriptor = findCuratedDspEffect(slot.effectType, slot.effectVersion))
            {
                for (const auto& parameter : descriptor->parameters)
                    slot.parameters.push_back({ std::string(parameter.id), parameter.defaultValue });
                slot.legacyInert = false;
                slot.unavailable = false;
            }
            else
            {
                slot.legacyInert = true;
                slot.bypassed = true;
            }
        }
        else
        {
            slot.legacyInert = true;
            slot.bypassed = true;
        }
    }
    for (auto& bus : migrated.authoring.routingBuses)
        if (bus.inputSourceId != "master" && bus.inputSourceId.rfind("groups/", 0) != 0 && bus.inputSourceId.rfind("zones/", 0) != 0)
            bus.inputSourceId = "zones/" + bus.inputSourceId;
    migrated.schemaVersion = 5;
    migrated.authoring.schemaVersion = 4;
    const auto validation = validateRuntimeProjectModel(migrated);
    result.project = std::move(migrated);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid ? "Project migrated to curated DSP schema" : validation.state;
    return result;
}

RuntimeProjectMigrationResult migrateRuntimeProjectToPerformanceArticulationSchema(
    const RuntimeProjectModel& project)
{
    RuntimeProjectMigrationResult result;
    result.state = "Project articulation migration failed";

    if (project.schemaVersion == 6 && project.authoring.schemaVersion == 5)
    {
        result.project = project;
        const auto validation = validateRuntimeProjectModel(result.project);
        result.issues = validation.issues;
        result.valid = validation.valid;
        result.state = validation.valid ? "Project already uses the articulation schema" : validation.state;
        return result;
    }
    if (project.schemaVersion != 5 || project.authoring.schemaVersion != 4)
    {
        addIssue(result, "Only Project schemaVersion 5 with authoring schemaVersion 4 can migrate to the articulation schema.");
        return result;
    }

    auto migrated = project;
    migrated.schemaVersion = 6;
    migrated.authoring.schemaVersion = 5;
    migrated.authoring.articulations.clear();
    std::unordered_set<std::string> seenIds;
    const auto selectedZone = std::find_if(migrated.authoring.zones.begin(), migrated.authoring.zones.end(),
                                           [&](const RuntimeProjectZoneDefinition& zone)
                                           {
                                               return zone.id == migrated.authoring.selectedZoneId;
                                           });
    const auto preferredDefaultId = selectedZone != migrated.authoring.zones.end()
        && !selectedZone->articulationId.empty() ? selectedZone->articulationId : std::string {};

    for (auto& zone : migrated.authoring.zones)
    {
        if (zone.articulationId.empty())
            zone.articulationId = "legacy-default";
        if (!seenIds.insert(zone.articulationId).second)
            continue;
        RuntimeProjectArticulationDefinition articulation;
        articulation.id = zone.articulationId;
        articulation.displayName = zone.articulationId;
        if (!articulation.displayName.empty())
        {
            articulation.displayName.front() = static_cast<char>(std::toupper(
                static_cast<unsigned char>(articulation.displayName.front())));
            std::replace(articulation.displayName.begin(), articulation.displayName.end(), '-', ' ');
            std::replace(articulation.displayName.begin(), articulation.displayName.end(), '_', ' ');
        }
        articulation.displayOrder = static_cast<int>(migrated.authoring.articulations.size());
        migrated.authoring.articulations.push_back(std::move(articulation));
    }
    if (!migrated.authoring.articulations.empty())
    {
        const auto defaultIterator = std::find_if(migrated.authoring.articulations.begin(), migrated.authoring.articulations.end(),
                                                  [&](const RuntimeProjectArticulationDefinition& articulation)
                                                  {
                                                      return articulation.id == preferredDefaultId;
                                                  });
        (defaultIterator != migrated.authoring.articulations.end()
            ? defaultIterator : migrated.authoring.articulations.begin())->isDefault = true;
    }

    const auto validation = validateRuntimeProjectModel(migrated);
    result.project = std::move(migrated);
    result.issues = validation.issues;
    result.valid = validation.valid;
    result.migrated = validation.valid;
    result.state = validation.valid ? "Project migrated to the articulation schema" : validation.state;
    return result;
}

RuntimeManifestLoadResult loadRuntimeInstrumentManifest(const std::string& manifestPath)
{
    const auto startTime = std::chrono::steady_clock::now();
    RuntimeManifestLoadResult result;
    result.manifestPath = manifestPath;
    result.state = "Manifest load not attempted";

    const fs::path manifestFsPath(manifestPath);
    std::error_code errorCode;

    if (!fs::exists(manifestFsPath, errorCode))
    {
        result.state = "Manifest missing";
        addIssue(result, "Manifest file was not found at " + manifestPath + ".");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    result.manifestFound = true;

    const auto rawText = readTextFile(manifestFsPath);
    if (rawText.empty())
    {
        result.state = "Manifest unreadable";
        addIssue(result, "Manifest file was empty or unreadable.");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    result.metrics.manifestSizeBytes = static_cast<std::uint64_t>(rawText.size());

    json root;
    try
    {
        root = json::parse(rawText);
    }
    catch (const json::exception& exception)
    {
        result.state = "Manifest parse failed";
        addIssue(result, "Manifest JSON parse failed: " + std::string(exception.what()));
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    if (!root.is_object())
    {
        result.state = "Manifest root invalid";
        addIssue(result, "Manifest root must be a JSON object.");
        result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
        return result;
    }

    auto& instrument = result.instrument;

    if (const auto schemaName = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "schemaName", "Manifest"))
        instrument.schemaName = *schemaName;

    if (const auto schemaVersion = readRequired<RuntimeManifestLoadResult, int>(root, result, "schemaVersion", "Manifest"))
        instrument.schemaVersion = *schemaVersion;

    if (const auto instrumentId = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "instrumentId", "Manifest"))
        instrument.instrumentId = *instrumentId;

    if (const auto displayName = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "displayName", "Manifest"))
        instrument.displayName = *displayName;

    if (const auto sourceProjectPath = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "sourceProject", "Manifest"))
    {
        const auto resolved = validateRequiredFile(result, manifestFsPath, *sourceProjectPath, "Source project");
        instrument.sourceProjectPath = resolved ? toDisplayPath(*resolved) : *sourceProjectPath;
        result.metrics.sourceProjectResolved = resolved.has_value();
    }

    if (const auto compiledStreamAsset = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "compiledStreamAsset", "Manifest"))
    {
        const auto resolved = validateRequiredFile(result, manifestFsPath, *compiledStreamAsset, "Compiled stream asset");
        instrument.compiledStreamAssetPath = resolved ? toDisplayPath(*resolved) : *compiledStreamAsset;
        result.metrics.usesStreaming = resolved.has_value();
        result.metrics.compiledStreamAssetResolved = resolved.has_value();
    }

    if (const auto loadProfile = readRequired<RuntimeManifestLoadResult, std::string>(root, result, "defaultLoadProfile", "Manifest"))
        instrument.defaultLoadProfile = *loadProfile;

    std::unordered_set<std::string> articulationIds;
    bool defaultArticulationFound = false;

    const auto articulationsIterator = root.find("articulations");
    if (articulationsIterator == root.end() || !isObjectArray(*articulationsIterator))
    {
        addIssue(result, "Manifest field 'articulations' must be an array of objects.");
    }
    else
    {
        instrument.articulations.reserve(articulationsIterator->size());

        for (std::size_t index = 0; index < articulationsIterator->size(); ++index)
        {
            const auto& articulationObject = articulationsIterator->at(index);
            const auto context = "Articulation[" + std::to_string(index) + "]";
            RuntimeArticulationDefinition articulation;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(articulationObject, result, "id", context.c_str()))
                articulation.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(articulationObject, result, "name", context.c_str()))
                articulation.name = *name;

            articulation.isDefault = articulationObject.value("isDefault", false);
            if (instrument.schemaVersion >= 3 && articulationObject.contains("activation"))
            {
                const auto& activationObject = articulationObject["activation"];
                if (!activationObject.is_object())
                {
                    addIssue(result, context + " field 'activation' must be an object.");
                }
                else
                {
                    RuntimeProjectArticulationActivationDefinition activation;
                    const auto event = readRequired<RuntimeManifestLoadResult, std::string>(activationObject, result, "event", context.c_str());
                    const auto note = readRequired<RuntimeManifestLoadResult, int>(activationObject, result, "midiNote", context.c_str());
                    const auto mode = readRequired<RuntimeManifestLoadResult, std::string>(activationObject, result, "mode", context.c_str());
                    if (event && !parsePerformanceEventKind(*event, activation.event)) addIssue(result, context + " activation event is unsupported.");
                    if (note) activation.midiNote = *note;
                    if (mode && !parseArticulationActivationMode(*mode, activation.mode)) addIssue(result, context + " activation mode is unsupported.");
                    activation.consume = activationObject.value("consume", true);
                    articulation.activation = activation;
                }
            }
            defaultArticulationFound = defaultArticulationFound || articulation.isDefault;

            if (!articulation.id.empty())
                articulationIds.insert(articulation.id);

            instrument.articulations.push_back(std::move(articulation));
        }
    }

    if (!defaultArticulationFound)
        addIssue(result, "Manifest must declare one default articulation for the Sprint 1 reference load path.");

    std::unordered_set<std::string> groupIds;

    const auto groupsIterator = root.find("groups");
    if (groupsIterator == root.end() || !isObjectArray(*groupsIterator))
    {
        addIssue(result, "Manifest field 'groups' must be an array of objects.");
    }
    else
    {
        instrument.groups.reserve(groupsIterator->size());

        for (std::size_t index = 0; index < groupsIterator->size(); ++index)
        {
            const auto& groupObject = groupsIterator->at(index);
            const auto context = "Group[" + std::to_string(index) + "]";
            RuntimeGroupDefinition group;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(groupObject, result, "id", context.c_str()))
                group.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(groupObject, result, "name", context.c_str()))
                group.name = *name;

            group.articulationIds = readRequiredStringArray(groupObject, result, "articulationIds", context.c_str());

            for (const auto& articulationId : group.articulationIds)
            {
                if (!articulationIds.count(articulationId))
                    addIssue(result, context + " references unknown articulation '" + articulationId + "'.");
            }

            if (!group.id.empty())
                groupIds.insert(group.id);

            instrument.groups.push_back(std::move(group));
        }
    }

    const auto macrosIterator = root.find("macros");
    if (macrosIterator == root.end() || !isObjectArray(*macrosIterator))
    {
        addIssue(result, "Manifest field 'macros' must be an array of objects.");
    }
    else
    {
        instrument.macros.reserve(macrosIterator->size());

        for (std::size_t index = 0; index < macrosIterator->size(); ++index)
        {
            const auto& macroObject = macrosIterator->at(index);
            const auto context = "Macro[" + std::to_string(index) + "]";
            RuntimeMacroDefinition macro;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(macroObject, result, "id", context.c_str()))
                macro.id = *id;

            if (const auto name = readRequired<RuntimeManifestLoadResult, std::string>(macroObject, result, "name", context.c_str()))
                macro.name = *name;

            if (const auto defaultValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "defaultValue", context.c_str()))
                macro.defaultValue = *defaultValue;

            if (const auto minValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "minValue", context.c_str()))
                macro.minValue = *minValue;

            if (const auto maxValue = readRequired<RuntimeManifestLoadResult, double>(macroObject, result, "maxValue", context.c_str()))
                macro.maxValue = *maxValue;

            if (macro.minValue > macro.maxValue)
                addIssue(result, context + " has minValue greater than maxValue.");

            instrument.macros.push_back(std::move(macro));
        }
    }

    const auto zonesIterator = root.find("zones");
    if (zonesIterator == root.end() || !isObjectArray(*zonesIterator))
    {
        addIssue(result, "Manifest field 'zones' must be an array of objects.");
    }
    else
    {
        instrument.zones.reserve(zonesIterator->size());

        for (std::size_t index = 0; index < zonesIterator->size(); ++index)
        {
            const auto& zoneObject = zonesIterator->at(index);
            const auto context = "Zone[" + std::to_string(index) + "]";
            RuntimeZoneDefinition zone;

            if (const auto id = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "id", context.c_str()))
                zone.id = *id;

            if (const auto groupId = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "groupId", context.c_str()))
                zone.groupId = *groupId;

            if (const auto articulationId = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "articulationId", context.c_str()))
                zone.articulationId = *articulationId;

            if (const auto samplePath = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "samplePath", context.c_str()))
            {
                const auto resolvedSamplePath = validateRequiredFile(result, manifestFsPath, *samplePath, "Zone sample");
                zone.samplePath = resolvedSamplePath ? toDisplayPath(*resolvedSamplePath) : *samplePath;
            }

            if (const auto streamAssetPath = readRequired<RuntimeManifestLoadResult, std::string>(zoneObject, result, "streamAssetPath", context.c_str()))
            {
                const auto resolvedStreamPath = validateRequiredFile(result, manifestFsPath, *streamAssetPath, "Zone stream asset");
                zone.streamAssetPath = resolvedStreamPath ? toDisplayPath(*resolvedStreamPath) : *streamAssetPath;
            }

            if (const auto rootKey = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "rootKey", context.c_str()))
                zone.rootKey = *rootKey;

            if (const auto keyLow = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "keyLow", context.c_str()))
                zone.keyLow = *keyLow;

            if (const auto keyHigh = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "keyHigh", context.c_str()))
                zone.keyHigh = *keyHigh;

            if (const auto velocityLow = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "velocityLow", context.c_str()))
                zone.velocityLow = *velocityLow;

            if (const auto velocityHigh = readRequired<RuntimeManifestLoadResult, int>(zoneObject, result, "velocityHigh", context.c_str()))
                zone.velocityHigh = *velocityHigh;
            if (const auto velocityCrossfade =
                    readOptionalVelocityCrossfade(zoneObject, result, "velocityCrossfade", context.c_str()))
            {
                zone.velocityCrossfade = *velocityCrossfade;
            }

            if (const auto streamOffsetBytes = readRequired<RuntimeManifestLoadResult, std::uint64_t>(zoneObject, result, "streamOffsetBytes", context.c_str()))
                zone.streamOffsetBytes = *streamOffsetBytes;

            if (const auto prefetchBytes = readRequired<RuntimeManifestLoadResult, std::uint64_t>(zoneObject, result, "prefetchBytes", context.c_str()))
                zone.prefetchBytes = *prefetchBytes;
            if (const auto releaseSeconds = readOptional<RuntimeManifestLoadResult, double>(zoneObject, result, "releaseSeconds", context.c_str()))
                zone.releaseSeconds = *releaseSeconds;

            const auto explicitRoundRobin = readOptionalRoundRobin(zoneObject, result, "roundRobin", context.c_str());
            const auto hasExplicitRoundRobinField = zoneObject.find("roundRobin") != zoneObject.end();
            if (hasExplicitRoundRobinField)
            {
                if (explicitRoundRobin.has_value())
                    applyRoundRobinDescriptor(zone, *explicitRoundRobin);

                if (instrument.schemaVersion >= 2
                    && (zoneObject.find("roundRobinLength") != zoneObject.end()
                        || zoneObject.find("roundRobinPosition") != zoneObject.end()))
                {
                    addIssue(result, context + " must not mix roundRobin scalars with the roundRobin object in the current schema.");
                }
            }
            else
            {
                if (const auto roundRobinLength = readOptional<RuntimeManifestLoadResult, int>(zoneObject, result, "roundRobinLength", context.c_str()))
                    zone.roundRobinLength = *roundRobinLength;
                if (const auto roundRobinPosition = readOptional<RuntimeManifestLoadResult, int>(zoneObject, result, "roundRobinPosition", context.c_str()))
                    zone.roundRobinPosition = *roundRobinPosition;

                if (instrument.schemaVersion >= 2
                    && (zone.roundRobinLength > 0 || zone.roundRobinPosition > 0))
                {
                    addIssue(result, context + " must use the roundRobin object in schemaVersion 2 manifests.");
                }
            }
            if (const auto triggerMode = readOptional<RuntimeManifestLoadResult, std::string>(zoneObject, result, "triggerMode", context.c_str()))
            {
                if (*triggerMode == "gated")
                    zone.triggerMode = ZoneTriggerMode::gated;
                else if (*triggerMode == "one-shot")
                    zone.triggerMode = ZoneTriggerMode::oneShot;
                else
                    addIssue(result, context + " field 'triggerMode' must be 'gated' or 'one-shot'.");
            }
            if (instrument.schemaVersion >= 3 && zoneObject.contains("performance"))
            {
                const auto& performance = zoneObject["performance"];
                if (!performance.is_object())
                {
                    addIssue(result, context + " field 'performance' must be an object.");
                }
                else
                {
                    const auto event = readRequired<RuntimeManifestLoadResult, std::string>(performance, result, "event", context.c_str());
                    const auto sustain = readRequired<RuntimeManifestLoadResult, std::string>(performance, result, "sustain", context.c_str());
                    const auto pitch = readRequired<RuntimeManifestLoadResult, std::string>(performance, result, "pitchSource", context.c_str());
                    if (event && !parsePerformanceEventKind(*event, zone.performance.event)) addIssue(result, context + " performance event is unsupported.");
                    if (sustain && !parsePerformanceSustainCondition(*sustain, zone.performance.sustain)) addIssue(result, context + " performance sustain is unsupported.");
                    if (pitch && !parsePerformancePitchSource(*pitch, zone.performance.pitchSource)) addIssue(result, context + " performance pitchSource is unsupported.");
                }
            }
            if (instrument.schemaVersion >= 3)
            {
                if (const auto group = readOptional<RuntimeManifestLoadResult, std::string>(zoneObject, result, "exclusiveGroupId", context.c_str())) zone.exclusiveGroupId = *group;
                if (zoneObject.contains("exclusiveTargetGroupIds")) zone.exclusiveTargetGroupIds = readRequiredStringArray(zoneObject, result, "exclusiveTargetGroupIds", context.c_str());
                if (const auto release = readOptional<RuntimeManifestLoadResult, double>(zoneObject, result, "chokeReleaseSeconds", context.c_str())) zone.chokeReleaseSeconds = *release;
            }

            if (!hasExplicitRoundRobinField)
            {
                if (const auto synthesizedRoundRobin = synthesizeRoundRobinFromLegacyScalars(zone))
                    applyRoundRobinDescriptor(zone, *synthesizedRoundRobin);
            }

            if (!groupIds.count(zone.groupId))
                addIssue(result, context + " references unknown group '" + zone.groupId + "'.");

            if (!articulationIds.count(zone.articulationId))
                addIssue(result, context + " references unknown articulation '" + zone.articulationId + "'.");

            if (zone.keyLow > zone.keyHigh)
                addIssue(result, context + " has keyLow greater than keyHigh.");

            if (zone.releaseSeconds < 0.0)
                addIssue(result, context + " must not have a negative releaseSeconds.");

            validateRoundRobinDescriptor(result,
                                         context,
                                         zone.roundRobin,
                                         zone.roundRobinLength,
                                         zone.roundRobinPosition,
                                         instrument.schemaVersion >= 2,
                                         instrument.schemaVersion >= 2);

            if (zone.velocityLow > zone.velocityHigh)
                addIssue(result, context + " has velocityLow greater than velocityHigh.");

            if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            {
                const auto crossfadeIssue = validateFirstPassVelocityCrossfadeZone(
                    buildVelocityCrossfadeValidationZone(zone.velocityLow, zone.velocityHigh, zone.velocityCrossfade));
                if (crossfadeIssue != VelocityCrossfadeZoneIssue::none)
                    addIssue(result, buildVelocityCrossfadeIssue(context, crossfadeIssue));
            }

            result.metrics.totalPrefetchBytes += zone.prefetchBytes;
            instrument.zones.push_back(std::move(zone));
        }
    }

    for (const auto& finding : collectVelocityCrossfadeTopologyFindings(instrument.zones))
    {
        if (finding.zoneIndex >= instrument.zones.size())
            continue;

        addIssue(result,
                 buildVelocityCrossfadeTopologyIssue("Zone '" + instrument.zones[finding.zoneIndex].id + "'",
                                                     finding.issue));
    }

    populateVelocityCrossfadeRuntimeDescriptors(instrument.zones);

    if (instrument.schemaVersion >= 3 && root.contains("roundRobinResetRules"))
    {
        const auto& resetRules = root["roundRobinResetRules"];
        if (!isObjectArray(resetRules))
        {
            addIssue(result, "Manifest field 'roundRobinResetRules' must be an array of objects.");
        }
        else
        {
            for (std::size_t index = 0; index < resetRules.size(); ++index)
            {
                const auto& value = resetRules[index];
                RuntimeProjectRoundRobinResetRuleDefinition rule;
                const auto context = "RoundRobinResetRule[" + std::to_string(index) + "]";
                const auto event = readRequired<RuntimeManifestLoadResult, std::string>(value, result, "event", context.c_str());
                if (event && !parseRoundRobinResetEvent(*event, rule.event)) addIssue(result, context + " event is unsupported.");
                rule.targetAll = value.value("targetAll", true);
                if (const auto pool = readOptional<RuntimeManifestLoadResult, std::string>(value, result, "targetPoolId", context.c_str())) rule.targetPoolId = *pool;
                instrument.roundRobinResetRules.push_back(std::move(rule));
            }
        }
    }

    instrument.validationNotes = readRequiredStringArray(root, result, "validationNotes", "Manifest");

    result.metrics.macroCount = instrument.macros.size();
    result.metrics.articulationCount = instrument.articulations.size();
    result.metrics.groupCount = instrument.groups.size();
    result.metrics.zoneCount = instrument.zones.size();
    result.metrics.referencedSampleCount = instrument.zones.size();

    if (instrument.schemaName != "drs.instrument")
        addIssue(result, "Manifest schemaName must be 'drs.instrument' for the Sprint 1 loader.");

    if (instrument.schemaVersion != 1 && instrument.schemaVersion != 2 && instrument.schemaVersion != 3)
        addIssue(result, "Manifest schemaVersion must be 1, 2, or 3.");

    if (instrument.zones.empty())
        addIssue(result, "Manifest must declare at least one zone.");

    if (instrument.groups.empty())
        addIssue(result, "Manifest must declare at least one group.");

    if (instrument.articulations.empty())
        addIssue(result, "Manifest must declare at least one articulation.");

    if (!result.metrics.usesStreaming)
        addIssue(result, "Compiled stream asset must exist so the Sprint 1 loader can prove the stream-container seam.");

    result.loaded = result.issues.empty();
    result.state = result.loaded ? "Reference manifest loaded" : "Reference manifest invalid";
    result.metrics.loadDurationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
    return result;
}

RuntimeManifestLoadResult loadPhase1ReferenceInstrumentManifest()
{
    return loadRuntimeInstrumentManifest(getPhase1ReferenceInstrumentManifestPath());
}

std::string serializeRuntimeProjectManifest(const RuntimeProjectModel& project, const std::string& manifestPath)
{
    const fs::path manifestFsPath(manifestPath);
    ordered_json root;
    root["schemaName"] = project.schemaName;
    root["schemaVersion"] = project.schemaVersion;
    root["projectId"] = project.projectId;
    root["displayName"] = project.displayName;
    root["contentRoot"] = toManifestRelativePath(manifestFsPath, project.contentRootPath);
    root["defaultInstrumentManifest"] = toManifestRelativePath(manifestFsPath, project.defaultInstrumentManifestPath);

    ordered_json sampleSources = ordered_json::array();
    for (const auto& sampleSource : project.sampleSources)
    {
        ordered_json sample;
        sample["id"] = sampleSource.id;
        sample["path"] = toManifestRelativePath(manifestFsPath, sampleSource.path);
        sample["role"] = sampleSource.role;
        sampleSources.push_back(std::move(sample));
    }

    root["sampleSources"] = std::move(sampleSources);

    if (project.schemaVersion >= 2)
    {
        ordered_json authoring;
        authoring["schemaName"] = project.authoring.schemaName;
        authoring["schemaVersion"] = project.authoring.schemaVersion;
        authoring["selectedZoneId"] = project.authoring.selectedZoneId;
        if (project.schemaVersion >= 4)
            authoring["selectedGroupId"] = project.authoring.selectedGroupId;
        authoring["selectedPerformanceBankId"] = project.authoring.selectedPerformanceBankId;
        if (project.schemaVersion >= 6)
        {
            authoring["articulations"] = serializeProjectArticulations(project.authoring.articulations);
            authoring["roundRobinResetRules"] = serializeRoundRobinResetRules(project.authoring.roundRobinResetRules);
        }
        authoring["zones"] = serializeProjectZones(project.authoring.zones, project.schemaVersion >= 3,
                                                     project.schemaVersion >= 6);
        if (project.schemaVersion >= 4)
            authoring["groups"] = serializeProjectGroups(project.authoring.groups);
        authoring["macros"] = serializeProjectMacros(project.authoring.macros);
        authoring["fxSlots"] = serializeFxSlots(project.authoring.fxSlots, project.schemaVersion >= 5);
        authoring["routingBuses"] = serializeRoutingBuses(project.authoring.routingBuses, project.schemaVersion >= 5);
        authoring["performanceBanks"] = serializePerformanceBanks(project.authoring.performanceBanks);
        authoring["notes"] = serializeStringArray(project.authoring.notes);
        root["authoring"] = std::move(authoring);
    }

    root["notes"] = serializeStringArray(project.notes);
    return root.dump(2) + "\n";
}

std::string serializeRuntimeInstrumentManifest(const RuntimeInstrumentModel& instrument, const std::string& manifestPath)
{
    const fs::path manifestFsPath(manifestPath);
    ordered_json root;
    root["schemaName"] = instrument.schemaName;
    root["schemaVersion"] = instrument.schemaVersion;
    root["instrumentId"] = instrument.instrumentId;
    root["displayName"] = instrument.displayName;
    root["sourceProject"] = toManifestRelativePath(manifestFsPath, instrument.sourceProjectPath);
    root["compiledStreamAsset"] = toManifestRelativePath(manifestFsPath, instrument.compiledStreamAssetPath);
    root["defaultLoadProfile"] = instrument.defaultLoadProfile;

    ordered_json macros = ordered_json::array();
    for (const auto& macro : instrument.macros)
    {
        ordered_json macroObject;
        macroObject["id"] = macro.id;
        macroObject["name"] = macro.name;
        macroObject["defaultValue"] = macro.defaultValue;
        macroObject["minValue"] = macro.minValue;
        macroObject["maxValue"] = macro.maxValue;
        macros.push_back(std::move(macroObject));
    }
    root["macros"] = std::move(macros);

    ordered_json articulations = ordered_json::array();
    for (const auto& articulation : instrument.articulations)
    {
        ordered_json articulationObject;
        articulationObject["id"] = articulation.id;
        articulationObject["name"] = articulation.name;
        articulationObject["isDefault"] = articulation.isDefault;
        if (instrument.schemaVersion >= 3 && articulation.activation.has_value())
        {
            articulationObject["activation"] = {
                { "event", performanceEventKindId(articulation.activation->event) },
                { "midiNote", articulation.activation->midiNote },
                { "mode", articulationActivationModeId(articulation.activation->mode) },
                { "consume", articulation.activation->consume }
            };
        }
        articulations.push_back(std::move(articulationObject));
    }
    root["articulations"] = std::move(articulations);

    ordered_json groups = ordered_json::array();
    for (const auto& group : instrument.groups)
    {
        ordered_json groupObject;
        groupObject["id"] = group.id;
        groupObject["name"] = group.name;
        groupObject["articulationIds"] = serializeStringArray(group.articulationIds);
        groups.push_back(std::move(groupObject));
    }
    root["groups"] = std::move(groups);

    ordered_json zones = ordered_json::array();
    for (const auto& zone : instrument.zones)
    {
        ordered_json zoneObject;
        zoneObject["id"] = zone.id;
        zoneObject["groupId"] = zone.groupId;
        zoneObject["articulationId"] = zone.articulationId;
        zoneObject["samplePath"] = toManifestRelativePath(manifestFsPath, zone.samplePath);
        zoneObject["streamAssetPath"] = toManifestRelativePath(manifestFsPath, zone.streamAssetPath);
        zoneObject["rootKey"] = zone.rootKey;
        zoneObject["keyLow"] = zone.keyLow;
        zoneObject["keyHigh"] = zone.keyHigh;
        zoneObject["velocityLow"] = zone.velocityLow;
        zoneObject["velocityHigh"] = zone.velocityHigh;
        if (hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
            zoneObject["velocityCrossfade"] = serializeVelocityCrossfade(zone.velocityCrossfade);
        if (hasAnyVelocityCrossfadeRuntimeValue(zone.velocityCrossfadeRuntime))
            zoneObject["velocityCrossfadeRuntime"] = serializeVelocityCrossfadeRuntime(zone.velocityCrossfadeRuntime);
        zoneObject["streamOffsetBytes"] = zone.streamOffsetBytes;
        zoneObject["prefetchBytes"] = zone.prefetchBytes;
        zoneObject["releaseSeconds"] = zone.releaseSeconds;
        if (instrument.schemaVersion >= 2)
        {
            if (zone.roundRobin.has_value())
                zoneObject["roundRobin"] = serializeRoundRobin(*zone.roundRobin);
        }
        else
        {
            zoneObject["roundRobinLength"] = zone.roundRobinLength;
            zoneObject["roundRobinPosition"] = zone.roundRobinPosition;
        }
        if (zone.triggerMode == ZoneTriggerMode::oneShot)
            zoneObject["triggerMode"] = "one-shot";
        if (instrument.schemaVersion >= 3)
        {
            zoneObject["performance"] = {
                { "event", performanceEventKindId(zone.performance.event) },
                { "sustain", performanceSustainConditionId(zone.performance.sustain) },
                { "pitchSource", performancePitchSourceId(zone.performance.pitchSource) }
            };
            if (!zone.exclusiveGroupId.empty()) zoneObject["exclusiveGroupId"] = zone.exclusiveGroupId;
            if (!zone.exclusiveTargetGroupIds.empty()) zoneObject["exclusiveTargetGroupIds"] = serializeStringArray(zone.exclusiveTargetGroupIds);
            if (zone.chokeReleaseSeconds.has_value()) zoneObject["chokeReleaseSeconds"] = *zone.chokeReleaseSeconds;
        }
        zones.push_back(std::move(zoneObject));
    }
    root["zones"] = std::move(zones);

    if (instrument.schemaVersion >= 3)
    {
        ordered_json resetRules = ordered_json::array();
        for (const auto& rule : instrument.roundRobinResetRules)
        {
            ordered_json value;
            value["event"] = roundRobinResetEventId(rule.event);
            value["targetAll"] = rule.targetAll;
            if (!rule.targetPoolId.empty()) value["targetPoolId"] = rule.targetPoolId;
            resetRules.push_back(std::move(value));
        }
        root["roundRobinResetRules"] = std::move(resetRules);
    }

    root["validationNotes"] = serializeStringArray(instrument.validationNotes);
    return root.dump(2) + "\n";
}
} // namespace drs::engine
