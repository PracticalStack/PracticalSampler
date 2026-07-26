#include "drs/engine/RuntimeCompiler.h"

#include <json/json.hpp>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;
using ordered_json = nlohmann::ordered_json;

void addIssue(RuntimeCompileResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

void addWarning(RuntimeCompileResult& result, const std::string& warning)
{
    result.warnings.push_back(warning);
}

std::uint64_t alignUp(std::uint64_t value, std::uint64_t alignment)
{
    if (alignment == 0)
        return value;

    const auto remainder = value % alignment;
    return remainder == 0 ? value : value + (alignment - remainder);
}

ordered_json serializeStringArray(const std::vector<std::string>& values)
{
    ordered_json array = ordered_json::array();

    for (const auto& value : values)
        array.push_back(value);

    return array;
}

std::string toContainerRelativePath(const fs::path& containerPath, const std::string& storedPath)
{
    const fs::path candidate(storedPath);

    if (!candidate.is_absolute())
        return candidate.generic_string();

    const auto relativePath = candidate.lexically_relative(containerPath.parent_path());

    if (!relativePath.empty())
        return relativePath.generic_string();

    return candidate.generic_string();
}

std::uint64_t computeFnv1a64(std::string_view text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    return hash;
}

std::uint64_t buildCrossfadePairingKey(const RuntimeCompileZoneDefinition& zone)
{
    std::ostringstream stream;
    stream << zone.articulationId
           << "|" << zone.rootKey
           << "|" << zone.keyLow
           << "|" << zone.keyHigh
           << "|" << zone.roundRobinLength
           << "|" << zone.roundRobinPosition;
    return computeFnv1a64(stream.str());
}

std::optional<RoundRobinDescriptor> materializeRoundRobinDescriptor(const RuntimeCompileZoneDefinition& zone)
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
           << "|0";

    RoundRobinDescriptor roundRobin;
    roundRobin.poolId = "legacy-rr-" + std::to_string(computeFnv1a64(stream.str()));
    roundRobin.slotCount = zone.roundRobinLength;
    roundRobin.slotIndex = zone.roundRobinPosition;
    roundRobin.mode = RoundRobinMode::sequential;
    return roundRobin;
}

std::string buildCrossfadeTopologyIssue(const std::string& zoneId,
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
    }

    return "Zone '" + zoneId + "' produced an unknown velocityCrossfade topology issue.";
}

void populateCrossfadeRuntimeDescriptors(const RuntimeCompilePlan& plan,
                                         std::vector<RuntimeZoneDefinition>& zones)
{
    std::vector<VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(plan.zones.size());

    for (const auto& zone : plan.zones)
    {
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildCrossfadePairingKey(zone);
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        topologyZones.push_back(topologyZone);
    }

    const auto runtimeTopology = buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones);
    for (std::size_t index = 0; index < zones.size() && index < runtimeTopology.size(); ++index)
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
} // namespace

RuntimeCompileResult compileRuntimeInstrument(const RuntimeCompilePlan& plan)
{
    RuntimeCompileResult result;
    result.state = "Compile not attempted";
    result.pageSizeBytes = plan.pageSizeBytes;
    result.containerId = plan.instrumentId;

    if (plan.pageSizeBytes == 0)
        addIssue(result, "Compile plan pageSizeBytes must be greater than zero.");

    if (plan.sampleSources.empty())
        addIssue(result, "Compile plan must provide at least one sample source.");

    if (plan.zones.empty())
        addIssue(result, "Compile plan must provide at least one zone.");

    if (plan.articulations.empty())
        addIssue(result, "Compile plan must provide at least one articulation.");

    if (plan.groups.empty())
        addIssue(result, "Compile plan must provide at least one group.");

    if (!result.issues.empty())
    {
        result.state = "Compile plan invalid";
        return result;
    }

    std::unordered_map<std::string, RuntimeCompileSourceDefinition> sourceById;
    sourceById.reserve(plan.sampleSources.size());

    result.project.schemaName = "drs.project";
    result.project.schemaVersion = 1;
    result.project.projectId = plan.projectId;
    result.project.displayName = plan.projectDisplayName;
    result.project.contentRootPath = plan.contentRootPath;
    result.project.defaultInstrumentManifestPath = plan.outputInstrumentPath;
    result.project.notes = plan.projectNotes;

    for (const auto& sampleSource : plan.sampleSources)
    {
        if (sampleSource.id.empty())
        {
            addIssue(result, "Compile source definitions must have non-empty ids.");
            continue;
        }

        if (sampleSource.sourcePath.empty())
        {
            addIssue(result, "Compile source '" + sampleSource.id + "' must have a non-empty source path.");
            continue;
        }

        if (sampleSource.metadata.frameCount == 0 || sampleSource.metadata.channelCount == 0 || sampleSource.metadata.sampleRate <= 0.0)
        {
            addIssue(result, "Compile source '" + sampleSource.id + "' must carry imported sample metadata before compilation.");
            continue;
        }

        sourceById.emplace(sampleSource.id, sampleSource);

        const auto policyReport = evaluatePhase1SamplePolicy(sampleSource.metadata, plan.contentRootPath);
        for (const auto& warning : policyReport.warnings)
            addWarning(result, "Source '" + sampleSource.id + "': " + warning);

        for (const auto& error : policyReport.errors)
            addIssue(result, "Source '" + sampleSource.id + "': " + error);

        RuntimeProjectSampleSource projectSource;
        projectSource.id = sampleSource.id;
        projectSource.path = sampleSource.sourcePath;
        projectSource.role = sampleSource.role;
        result.project.sampleSources.push_back(std::move(projectSource));
    }

    result.instrument.schemaName = "drs.instrument";
    result.instrument.schemaVersion = 2;
    result.instrument.instrumentId = plan.instrumentId;
    result.instrument.displayName = plan.instrumentDisplayName;
    result.instrument.sourceProjectPath = plan.outputProjectPath;
    result.instrument.compiledStreamAssetPath = plan.outputStreamPath;
    result.instrument.defaultLoadProfile = plan.defaultLoadProfile;
    result.instrument.macros = plan.macros;
    result.instrument.articulations = plan.articulations;
    result.instrument.groups = plan.groups;
    result.instrument.validationNotes = plan.instrumentValidationNotes;

    std::unordered_map<std::string, bool> articulationIds;
    for (const auto& articulation : plan.articulations)
        articulationIds.emplace(articulation.id, true);

    std::unordered_map<std::string, bool> groupIds;
    for (const auto& group : plan.groups)
        groupIds.emplace(group.id, true);

    std::unordered_map<std::string, std::uint64_t> maxPrefetchBySourceId;
    maxPrefetchBySourceId.reserve(plan.sampleSources.size());
    std::vector<VelocityCrossfadeTopologyZoneDefinition> crossfadeTopologyZones;
    crossfadeTopologyZones.reserve(plan.zones.size());

    for (const auto& zonePlan : plan.zones)
    {
        const auto roundRobin = materializeRoundRobinDescriptor(zonePlan);
        const auto sourceIterator = sourceById.find(zonePlan.sourceId);
        if (sourceIterator == sourceById.end())
        {
            addIssue(result, "Zone '" + zonePlan.id + "' references unknown source '" + zonePlan.sourceId + "'.");
            continue;
        }

        if (!groupIds.count(zonePlan.groupId))
            addIssue(result, "Zone '" + zonePlan.id + "' references unknown group '" + zonePlan.groupId + "'.");

        if (!articulationIds.count(zonePlan.articulationId))
            addIssue(result, "Zone '" + zonePlan.id + "' references unknown articulation '" + zonePlan.articulationId + "'.");

        if (zonePlan.keyLow > zonePlan.keyHigh)
            addIssue(result, "Zone '" + zonePlan.id + "' has keyLow greater than keyHigh.");

        if (zonePlan.velocityLow > zonePlan.velocityHigh)
            addIssue(result, "Zone '" + zonePlan.id + "' has velocityLow greater than velocityHigh.");
        if (zonePlan.roundRobinLength < 0 || zonePlan.roundRobinPosition < 0)
            addIssue(result, "Zone '" + zonePlan.id + "' must not have negative round-robin metadata.");
        if (zonePlan.roundRobinPosition > 0 && zonePlan.roundRobinLength <= 0)
            addIssue(result, "Zone '" + zonePlan.id + "' must define roundRobinLength when roundRobinPosition is present.");
        if (zonePlan.roundRobinLength > 0
            && (zonePlan.roundRobinPosition < 1 || zonePlan.roundRobinPosition > zonePlan.roundRobinLength))
        {
            addIssue(result, "Zone '" + zonePlan.id + "' has roundRobinPosition outside roundRobinLength.");
        }
        if (roundRobin.has_value())
        {
            if (roundRobin->poolId.empty())
                addIssue(result, "Zone '" + zonePlan.id + "' must provide a non-empty Round Robin poolId.");
            if (roundRobin->slotCount != zonePlan.roundRobinLength
                || roundRobin->slotIndex != zonePlan.roundRobinPosition)
            {
                addIssue(result, "Zone '" + zonePlan.id + "' must keep roundRobin slot data aligned with the scalar Round Robin fields.");
            }
        }
        if (hasAnyVelocityCrossfadeValue(zonePlan.velocityCrossfade))
        {
            const VelocityCrossfadeZoneDefinition crossfadeZone {
                zonePlan.velocityLow,
                zonePlan.velocityHigh,
                zonePlan.velocityCrossfade
            };
            const auto crossfadeIssue = validateFirstPassVelocityCrossfadeZone(crossfadeZone);
            if (crossfadeIssue != VelocityCrossfadeZoneIssue::none)
                addIssue(result, "Zone '" + zonePlan.id + "' carries unsupported velocityCrossfade metadata in the compile plan.");
        }

        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildCrossfadePairingKey(zonePlan);
        topologyZone.velocityLow = zonePlan.velocityLow;
        topologyZone.velocityHigh = zonePlan.velocityHigh;
        topologyZone.roundRobinLength = zonePlan.roundRobinLength;
        topologyZone.roundRobinPosition = zonePlan.roundRobinPosition;
        topologyZone.crossfade = zonePlan.velocityCrossfade;
        crossfadeTopologyZones.push_back(topologyZone);

        maxPrefetchBySourceId[zonePlan.sourceId] = std::max(maxPrefetchBySourceId[zonePlan.sourceId],
                                                            zonePlan.prefetchBytes);
    }

    std::vector<VelocityCrossfadeTopologyFinding> crossfadeTopologyFindings;
    buildFirstPassVelocityCrossfadeRuntimeTopology(crossfadeTopologyZones, &crossfadeTopologyFindings);
    for (const auto& finding : crossfadeTopologyFindings)
    {
        if (finding.zoneIndex >= plan.zones.size())
            continue;

        addIssue(result, buildCrossfadeTopologyIssue(plan.zones[finding.zoneIndex].id, finding.issue));
    }

    if (!result.issues.empty())
    {
        result.state = "Compile plan invalid";
        return result;
    }

    std::unordered_map<std::string, std::uint64_t> streamOffsetBySourceId;
    streamOffsetBySourceId.reserve(maxPrefetchBySourceId.size());

    std::uint64_t nextOffsetBytes = 0;
    for (const auto& sampleSource : plan.sampleSources)
    {
        const auto prefetchIterator = maxPrefetchBySourceId.find(sampleSource.id);
        if (prefetchIterator == maxPrefetchBySourceId.end())
            continue;

        const auto payloadSizeBytes = sampleSource.metadata.frameCount
            * static_cast<std::uint64_t>(sampleSource.metadata.channelCount)
            * sizeof(float);
        const auto clampedPrefetchBytes = std::min(prefetchIterator->second, payloadSizeBytes);

        CompiledStreamSampleDefinition streamSample;
        streamSample.sampleId = sampleSource.id;
        streamSample.sourcePath = sampleSource.sourcePath;
        streamSample.sourceChecksumHex = sampleSource.metadata.sourceChecksumHex;
        streamSample.formatName = sampleSource.metadata.formatName;
        streamSample.role = sampleSource.role;
        streamSample.channelLayout = sampleSource.metadata.channelLayout;
        streamSample.sampleRate = sampleSource.metadata.sampleRate;
        streamSample.frameCount = sampleSource.metadata.frameCount;
        streamSample.channelCount = sampleSource.metadata.channelCount;
        streamSample.payloadOffsetBytes = nextOffsetBytes;
        streamSample.payloadSizeBytes = payloadSizeBytes;
        streamSample.prefetchBytes = clampedPrefetchBytes;
        streamSample.rootMidiNotePresent = sampleSource.metadata.rootMidiNotePresent;
        streamSample.rootMidiNote = sampleSource.metadata.rootMidiNote;
        streamSample.loopRangePresent = sampleSource.metadata.loopRangePresent;
        streamSample.loopStartFrame = sampleSource.metadata.loopStartFrame;
        streamSample.loopEndFrame = sampleSource.metadata.loopEndFrame;

        auto streamedOffsetBytes = clampedPrefetchBytes;
        std::uint32_t pageIndex = 0;
        while (streamedOffsetBytes < payloadSizeBytes)
        {
            CompiledStreamPageDefinition page;
            page.pageIndex = pageIndex++;
            page.offsetBytes = nextOffsetBytes + streamedOffsetBytes;
            page.sizeBytes = std::min(plan.pageSizeBytes, payloadSizeBytes - streamedOffsetBytes);
            streamSample.pages.push_back(std::move(page));
            streamedOffsetBytes += plan.pageSizeBytes;
        }

        streamOffsetBySourceId[sampleSource.id] = nextOffsetBytes;
        result.totalPayloadBytes += payloadSizeBytes;
        result.streamSamples.push_back(std::move(streamSample));
        nextOffsetBytes += alignUp(payloadSizeBytes, plan.pageSizeBytes);
    }

    for (const auto& zonePlan : plan.zones)
    {
        const auto& source = sourceById.at(zonePlan.sourceId);
        const auto payloadSizeBytes = source.metadata.frameCount
            * static_cast<std::uint64_t>(source.metadata.channelCount)
            * sizeof(float);
        const auto clampedPrefetchBytes = std::min(zonePlan.prefetchBytes, payloadSizeBytes);
        const auto roundRobin = materializeRoundRobinDescriptor(zonePlan);

        RuntimeZoneDefinition zone;
        zone.id = zonePlan.id;
        zone.groupId = zonePlan.groupId;
        zone.articulationId = zonePlan.articulationId;
        zone.samplePath = source.sourcePath;
        zone.streamAssetPath = plan.outputStreamPath;
        zone.rootKey = zonePlan.rootKey;
        zone.keyLow = zonePlan.keyLow;
        zone.keyHigh = zonePlan.keyHigh;
        zone.velocityLow = zonePlan.velocityLow;
        zone.velocityHigh = zonePlan.velocityHigh;
        zone.velocityCrossfade = zonePlan.velocityCrossfade;
        zone.streamOffsetBytes = streamOffsetBySourceId.at(zonePlan.sourceId);
        zone.prefetchBytes = clampedPrefetchBytes;
        zone.roundRobin = roundRobin;
        zone.roundRobinLength = zonePlan.roundRobinLength;
        zone.roundRobinPosition = zonePlan.roundRobinPosition;
        result.instrument.zones.push_back(std::move(zone));
    }

    populateCrossfadeRuntimeDescriptors(plan, result.instrument.zones);

    result.compiled = true;
    result.state = "Compile completed";
    return result;
}

std::string serializePrototypeStreamContainer(const RuntimeCompileResult& result, const std::string& containerPath)
{
    const fs::path containerFsPath(containerPath);
    ordered_json root;
    root["schemaName"] = "drs.streamContainer";
    root["schemaVersion"] = 1;
    root["containerId"] = result.containerId;
    root["pageSizeBytes"] = result.pageSizeBytes;
    root["sampleCount"] = result.streamSamples.size();
    root["payloadEncoding"] = "f32-interleaved-little-endian";
    root["totalPayloadBytes"] = result.totalPayloadBytes;

    ordered_json samples = ordered_json::array();
    for (const auto& sample : result.streamSamples)
    {
        ordered_json sampleObject;
        sampleObject["sampleId"] = sample.sampleId;
        sampleObject["sourcePath"] = toContainerRelativePath(containerFsPath, sample.sourcePath);
        sampleObject["sourceChecksumHex"] = sample.sourceChecksumHex;
        sampleObject["formatName"] = sample.formatName;
        sampleObject["role"] = sample.role;
        sampleObject["channelLayout"] = sample.channelLayout;
        sampleObject["sampleRate"] = sample.sampleRate;
        sampleObject["frameCount"] = sample.frameCount;
        sampleObject["channelCount"] = sample.channelCount;
        sampleObject["payloadOffsetBytes"] = sample.payloadOffsetBytes;
        sampleObject["payloadSizeBytes"] = sample.payloadSizeBytes;
        sampleObject["prefetchBytes"] = sample.prefetchBytes;

        if (sample.rootMidiNotePresent)
            sampleObject["rootMidiNote"] = sample.rootMidiNote;

        if (sample.loopRangePresent)
        {
            sampleObject["loopStartFrame"] = sample.loopStartFrame;
            sampleObject["loopEndFrame"] = sample.loopEndFrame;
        }

        ordered_json pages = ordered_json::array();
        for (const auto& page : sample.pages)
        {
            ordered_json pageObject;
            pageObject["pageIndex"] = page.pageIndex;
            pageObject["offsetBytes"] = page.offsetBytes;
            pageObject["sizeBytes"] = page.sizeBytes;
            pages.push_back(std::move(pageObject));
        }

        sampleObject["pages"] = std::move(pages);
        samples.push_back(std::move(sampleObject));
    }

    root["samples"] = std::move(samples);
    root["notes"] = serializeStringArray({
        "Sprint 2 compiler-emitted prototype stream-container descriptor.",
        "Carries deterministic payload offsets, prefetch heads, and page-table placeholders for the reference instrument."
    });

    return root.dump(2) + "\n";
}
} // namespace drs::engine
