#include "drs/engine/SamplerRenderModel.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace drs::engine
{
namespace
{
std::uint64_t stableSourceGeneration(std::string_view identity) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : identity)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash == 0 ? 1 : hash;
}

void addError(SamplerRenderModelBuildResult& result,
              std::string code,
              std::string path,
              std::string message)
{
    result.findings.push_back({ SamplerRenderModelFindingSeverity::error,
                                std::move(code),
                                std::move(path),
                                std::move(message) });
}

bool containsZoneId(const std::vector<std::string>& zoneIds, const std::string& zoneId)
{
    return std::find(zoneIds.begin(), zoneIds.end(), zoneId) != zoneIds.end();
}

std::uint32_t resolveSelectedArticulationIndex(const SamplerRenderModelBuildOptions& options,
                                               const std::vector<SamplerRenderRoute>& routes,
                                               const CompiledPerformanceProgram& program) noexcept
{
    if (options.selectedArticulationId.empty())
        return program.defaultArticulationIndex;

    const auto selectedRoute = std::find_if(
        routes.begin(),
        routes.end(),
        [](const SamplerRenderRoute& route)
        {
            return route.performanceArticulationIndex < kInvalidPerformanceProgramIndex;
        });
    return selectedRoute != routes.end()
        ? selectedRoute->performanceArticulationIndex
        : program.defaultArticulationIndex;
}

double combineRouteGainDb(double zoneGainDb, double groupGainDb, double masterGainDb) noexcept
{
    return zoneGainDb + groupGainDb + masterGainDb;
}

double combineGroupPan(double zonePan, double groupPan) noexcept
{
    return std::clamp(zonePan + groupPan, -1.0, 1.0);
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

std::uint64_t buildCrossfadePairingKey(const PreparedPlaybackZoneHandle& zone)
{
    std::ostringstream stream;
    stream << zone.rootKey
           << "|" << zone.keyLow
           << "|" << zone.keyHigh
           << "|" << static_cast<int>(zone.triggerMode);
    return computeFnv1a64(stream.str());
}

std::string resolveRoundRobinPoolId(const PreparedPlaybackZoneHandle& zone)
{
    return zone.roundRobin.has_value() ? zone.roundRobin->poolId : std::string {};
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

bool sameTopology(const PlaybackSnapshotZone& snapshotZone,
                  const PreparedPlaybackZoneHandle& preparedZone)
{
    return snapshotZone.sampleSourceId == preparedZone.sampleSourceId
        && snapshotZone.rootKey == preparedZone.rootKey
        && snapshotZone.keyLow == preparedZone.keyLow
        && snapshotZone.keyHigh == preparedZone.keyHigh
        && snapshotZone.velocityLow == preparedZone.velocityLow
        && snapshotZone.velocityHigh == preparedZone.velocityHigh
        && snapshotZone.velocityCrossfade.fadeInLowVelocity == preparedZone.velocityCrossfade.fadeInLowVelocity
        && snapshotZone.velocityCrossfade.fadeInHighVelocity == preparedZone.velocityCrossfade.fadeInHighVelocity
        && snapshotZone.velocityCrossfade.fadeOutLowVelocity == preparedZone.velocityCrossfade.fadeOutLowVelocity
        && snapshotZone.velocityCrossfade.fadeOutHighVelocity == preparedZone.velocityCrossfade.fadeOutHighVelocity
        && snapshotZone.velocityCrossfadeRuntime.effectiveLowVelocity
            == preparedZone.velocityCrossfadeRuntime.effectiveLowVelocity
        && snapshotZone.velocityCrossfadeRuntime.effectiveHighVelocity
            == preparedZone.velocityCrossfadeRuntime.effectiveHighVelocity
        && snapshotZone.velocityCrossfadeRuntime.fadeInNeighborZoneId
            == preparedZone.velocityCrossfadeRuntime.fadeInNeighborZoneId
        && snapshotZone.velocityCrossfadeRuntime.fadeOutNeighborZoneId
            == preparedZone.velocityCrossfadeRuntime.fadeOutNeighborZoneId
        && snapshotZone.velocityCrossfadeRuntime.fadeInOverlapLowVelocity
            == preparedZone.velocityCrossfadeRuntime.fadeInOverlapLowVelocity
        && snapshotZone.velocityCrossfadeRuntime.fadeInOverlapHighVelocity
            == preparedZone.velocityCrossfadeRuntime.fadeInOverlapHighVelocity
        && snapshotZone.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity
            == preparedZone.velocityCrossfadeRuntime.fadeOutOverlapLowVelocity
        && snapshotZone.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity
            == preparedZone.velocityCrossfadeRuntime.fadeOutOverlapHighVelocity
        && snapshotZone.gainDb == preparedZone.gainDb
        && snapshotZone.pan == preparedZone.pan
        && snapshotZone.fineTuneCents == preparedZone.fineTuneCents
        && snapshotZone.amplitudeVelocityTracking == preparedZone.amplitudeVelocityTracking
        && snapshotZone.controllerConditions == preparedZone.controllerConditions
        && snapshotZone.damper == preparedZone.damper
        && snapshotZone.sampleStartFrame == preparedZone.sampleStartFrame
        && snapshotZone.loopEnabled == preparedZone.loopEnabled
        && snapshotZone.loopStartFrame == preparedZone.loopStartFrame
        && snapshotZone.loopEndFrame == preparedZone.loopEndFrame
        && snapshotZone.loopMode == preparedZone.loopMode
        && snapshotZone.sampleEndFrame == preparedZone.sampleEndFrame
        && snapshotZone.releaseSeconds == preparedZone.releaseSeconds
        && snapshotZone.releaseShape == preparedZone.releaseShape
        && snapshotZone.roundRobin == preparedZone.roundRobin
        && snapshotZone.roundRobinLength == preparedZone.roundRobinLength
        && snapshotZone.roundRobinPosition == preparedZone.roundRobinPosition
        && snapshotZone.triggerMode == preparedZone.triggerMode;
}

} // namespace

bool SamplerAudioBufferView::isValid() const noexcept
{
    if (channels == nullptr || channelCount == 0 || frameCount == 0)
        return false;

    for (std::uint32_t channel = 0; channel < channelCount; ++channel)
        if (channels[channel] == nullptr)
            return false;
    return true;
}

SamplerRenderModelBuildResult buildSamplerRenderModel(
    const PlaybackActivationPayloadPtr& payload,
    const SamplerRenderModelBuildOptions& options)
{
    SamplerRenderModelBuildResult result;
    if (payload == nullptr)
    {
        addError(result, "render-model-payload-missing", "payload", "An activation payload is required.");
        return result;
    }

    if (!payload->activationEligible)
        addError(result, "render-model-payload-ineligible", "payload.activationEligible",
                 "The activation payload is not renderer eligible.");
    if (payload->snapshot == nullptr)
        addError(result, "render-model-snapshot-missing", "payload.snapshot",
                 "The activation payload does not retain an immutable snapshot.");
    if (payload->prepared == nullptr)
        addError(result, "render-model-prepared-missing", "payload.prepared",
                 "The activation payload does not retain immutable prepared resources.");

    const auto expectedLifecycle = payload->lane == PlaybackActivationLane::performance
        ? PlaybackSnapshotLifecycleState::active
        : PlaybackSnapshotLifecycleState::ready;
    if (payload->lifecycleState != expectedLifecycle)
        addError(result, "render-model-lifecycle-invalid", "payload.lifecycleState",
                 "The activation payload lifecycle does not match its renderer lane.");

    if (payload->snapshot == nullptr || payload->prepared == nullptr)
        return result;

    const auto& snapshot = *payload->snapshot;
    const auto& prepared = *payload->prepared;

    if (options.midiNoteOffset < -127 || options.midiNoteOffset > 127)
        addError(result, "render-model-note-offset-invalid", "options.midiNoteOffset",
                 "MIDI note offset must remain inside -127 through 127 semitones.");
    if (options.fixedVelocity < 0 || options.fixedVelocity > 127)
        addError(result, "render-model-fixed-velocity-invalid", "options.fixedVelocity",
                 "Fixed velocity must be zero (use event velocity) or 1 through 127.");

    if (payload->snapshotBuildId == 0 || payload->preparedBuildId == 0)
        addError(result, "render-model-build-identity-missing", "payload",
                 "Snapshot and prepared build identities must both be non-zero.");
    if (snapshot.draftRevision != payload->revision || prepared.draftRevision != payload->revision)
        addError(result, "render-model-revision-mismatch", "payload.revision",
                 "Payload, snapshot, and prepared revisions must agree.");
    if (prepared.snapshotBuildId != payload->snapshotBuildId)
        addError(result, "render-model-snapshot-build-mismatch", "payload.snapshotBuildId",
                 "Prepared resources do not belong to the payload snapshot build.");
    if (payload->snapshotContentDigest.empty()
        || payload->snapshotContentDigest != snapshot.contentDigest
        || payload->snapshotContentDigest != prepared.snapshotContentDigest)
        addError(result, "render-model-snapshot-digest-mismatch", "payload.snapshotContentDigest",
                 "Payload, snapshot, and prepared snapshot digests must agree and be non-empty.");
    if (payload->preparedContentDigest.empty()
        || payload->preparedContentDigest != prepared.preparedContentDigest)
        addError(result, "render-model-prepared-digest-mismatch", "payload.preparedContentDigest",
                 "Payload and prepared content digests must agree and be non-empty.");

    if (prepared.samples.empty())
        addError(result, "render-model-samples-empty", "payload.prepared.samples",
                 "At least one prepared sample is required.");
    if (prepared.zones.empty())
        addError(result, "render-model-routes-empty", "payload.prepared.zones",
                 "At least one prepared zone route is required.");
    if (snapshot.zones.size() != prepared.zones.size())
        addError(result, "render-model-route-count-mismatch", "payload.prepared.zones",
                 "Snapshot and prepared route counts must agree.");
    if (prepared.performanceProgram.retainedBytes != 0
        && prepared.performanceProgram.retainedBytes
        != sizeof(CompiledPerformanceProgram)
            + prepared.performanceProgram.triggerRoutes.size() * sizeof(CompiledPerformanceTriggerRoute)
            + prepared.performanceProgram.roundRobinResets.size() * sizeof(CompiledPerformanceRoundRobinReset)
            + prepared.performanceProgram.articulationStableIds.size() * sizeof(std::uint64_t)
            + prepared.performanceProgram.exclusiveGroupStableIds.size() * sizeof(std::uint64_t)
            + prepared.performanceProgram.roundRobinPoolStableIds.size() * sizeof(std::uint64_t)
            + prepared.performanceProgram.zoneArticulationIndices.size() * sizeof(std::uint32_t))
    {
        addError(result, "render-model-performance-program-size-invalid", "payload.prepared.performanceProgram",
                 "Prepared performance-program memory accounting does not match its retained numeric records.");
    }
    for (const auto& route : prepared.performanceProgram.triggerRoutes)
    {
        if (route.zoneIndex >= prepared.zones.size()
            || route.articulationIndex >= prepared.performanceProgram.articulationCount)
        {
            addError(result, "render-model-performance-program-index-invalid", "payload.prepared.performanceProgram",
                     "Prepared performance program contains an out-of-range numeric route index.");
            break;
        }
    }
    const auto& zoneArticulationIndices = prepared.performanceProgram.zoneArticulationIndices;
    if (!zoneArticulationIndices.empty() && zoneArticulationIndices.size() != prepared.zones.size())
        addError(result, "render-model-performance-zone-map-size-invalid", "payload.prepared.performanceProgram",
                 "A non-legacy performance program must map every prepared zone to an articulation.");
    for (const auto articulationIndex : zoneArticulationIndices)
    {
        if (articulationIndex >= prepared.performanceProgram.articulationCount)
        {
            addError(result, "render-model-performance-zone-map-index-invalid", "payload.prepared.performanceProgram",
                     "Prepared performance program contains an out-of-range zone articulation index.");
            break;
        }
    }

    std::unordered_map<std::string, const PlaybackSnapshotZone*> snapshotZonesById;
    snapshotZonesById.reserve(snapshot.zones.size());
    for (std::size_t index = 0; index < snapshot.zones.size(); ++index)
    {
        const auto& zone = snapshot.zones[index];
        if (zone.id.empty())
            addError(result, "render-model-snapshot-zone-id-missing",
                     "payload.snapshot.zones[" + std::to_string(index) + "].id",
                     "Every snapshot route requires a stable zone identity.");
        else if (!snapshotZonesById.emplace(zone.id, &zone).second)
            addError(result, "render-model-snapshot-zone-id-duplicate",
                     "payload.snapshot.zones[" + std::to_string(index) + "].id",
                     "Snapshot route identities must be unique.");
    }

    for (std::size_t index = 0; index < prepared.samples.size(); ++index)
    {
        const auto& sample = prepared.samples[index];
        const auto path = "payload.prepared.samples[" + std::to_string(index) + "]";
        if (sample.sampleSourceId.empty())
            addError(result, "render-model-sample-id-missing", path + ".sampleSourceId",
                     "Every prepared sample requires a source identity.");
        if (!std::isfinite(sample.sampleRate) || sample.sampleRate <= 0.0)
            addError(result, "render-model-sample-rate-invalid", path + ".sampleRate",
                     "Prepared sample rate must be finite and positive.");
        if (sample.frameCount == 0)
            addError(result, "render-model-frame-count-invalid", path + ".frameCount",
                     "Prepared samples must contain at least one frame.");
        if (sample.channelCount < 1 || sample.channelCount > 2)
            addError(result, "render-model-channel-count-unsupported", path + ".channelCount",
                     "Sprint 4 renderer inputs support mono or stereo prepared samples.");
        if (sample.decodedSampleData == nullptr && sample.dataSource == nullptr)
        {
            addError(result, "render-model-sample-source-missing", path + ".dataSource",
                     "Prepared samples require either bounded resident PCM or a common sample data source.");
            continue;
        }
        if (sample.dataSource != nullptr)
        {
            const auto validation = validateSampleDataSourceDescriptor(sample.dataSource->descriptor());
            if (!validation.valid
                || sample.dataSource->descriptor().frameCount != sample.frameCount
                || sample.dataSource->descriptor().channelCount != sample.channelCount)
                addError(result, "render-model-source-descriptor-invalid", path + ".dataSource",
                         "Prepared source descriptor must validate and match sample dimensions.");
        }
        if (sample.decodedSampleData != nullptr)
        {
            const auto& channels = sample.decodedSampleData->normalizedChannels;
            if (channels.size() != sample.channelCount)
                addError(result, "render-model-channel-layout-mismatch", path + ".decodedSampleData",
                         "Decoded PCM channel storage must match prepared channel metadata.");
            for (std::size_t channel = 0; channel < channels.size(); ++channel)
                if (channels[channel].size() < sample.frameCount)
                    addError(result, "render-model-channel-frames-truncated",
                             path + ".decodedSampleData.channels[" + std::to_string(channel) + "]",
                             "Decoded PCM storage is shorter than the declared frame count.");
        }
    }

    std::unordered_map<std::string, const PlaybackSnapshotGroupRoute*> snapshotGroupsById;
    std::unordered_map<std::string, std::unordered_set<std::string>> snapshotGroupZoneIds;
    snapshotGroupsById.reserve(snapshot.groupRoutes.size());
    snapshotGroupZoneIds.reserve(snapshot.groupRoutes.size());
    for (const auto& group : snapshot.groupRoutes)
    {
        snapshotGroupsById.emplace(group.groupId, &group);
        snapshotGroupZoneIds.emplace(
            group.groupId,
            std::unordered_set<std::string>(group.zoneIds.begin(), group.zoneIds.end()));
    }
    std::unordered_map<std::string, const PreparedPlaybackGroupRoute*> preparedGroupsById;
    std::unordered_map<std::string, std::unordered_set<std::string>> preparedGroupZoneIds;
    preparedGroupsById.reserve(prepared.groupRoutes.size());
    preparedGroupZoneIds.reserve(prepared.groupRoutes.size());
    for (const auto& group : prepared.groupRoutes)
    {
        preparedGroupsById.emplace(group.groupId, &group);
        preparedGroupZoneIds.emplace(
            group.groupId,
            std::unordered_set<std::string>(group.zoneIds.begin(), group.zoneIds.end()));
    }

    std::unordered_set<std::string> preparedZoneIds;
    preparedZoneIds.reserve(prepared.zones.size());
    for (std::size_t index = 0; index < prepared.zones.size(); ++index)
    {
        const auto& zone = prepared.zones[index];
        const auto path = "payload.prepared.zones[" + std::to_string(index) + "]";
        if (zone.zoneId.empty())
            addError(result, "render-model-zone-id-missing", path + ".zoneId",
                     "Every prepared route requires a stable zone identity.");
        else if (!preparedZoneIds.insert(zone.zoneId).second)
            addError(result, "render-model-zone-id-duplicate", path + ".zoneId",
                     "Prepared route identities must be unique.");

        if (zone.preparedSampleIndex >= prepared.samples.size())
        {
            addError(result, "render-model-sample-index-invalid", path + ".preparedSampleIndex",
                     "Prepared route sample index is out of range.");
            continue;
        }

        const auto& sample = prepared.samples[zone.preparedSampleIndex];
        if (zone.sampleSourceId.empty() || zone.sampleSourceId != sample.sampleSourceId)
            addError(result, "render-model-zone-sample-mismatch", path + ".sampleSourceId",
                     "Prepared route source identity must match its sample handle.");
        if (zone.streamSampleId != sample.streamSampleId)
            addError(result, "render-model-zone-stream-mismatch", path + ".streamSampleId",
                     "Prepared route stream identity must match its sample handle.");
        if (zone.rootKey < 0 || zone.rootKey > 127)
            addError(result, "render-model-root-key-invalid", path + ".rootKey",
                     "Root key must be inside the MIDI note range.");
        if (zone.keyLow < 0 || zone.keyHigh > 127 || zone.keyLow > zone.keyHigh)
            addError(result, "render-model-key-range-invalid", path + ".keyLow",
                     "Key range must be ordered inside the MIDI note range.");
        if (zone.velocityLow < 1 || zone.velocityHigh > 127 || zone.velocityLow > zone.velocityHigh)
            addError(result, "render-model-velocity-range-invalid", path + ".velocityLow",
                     "Velocity range must be ordered inside 1 through 127.");
        if (!std::isfinite(zone.gainDb))
            addError(result, "render-model-gain-invalid", path + ".gainDb",
                     "Zone gain must be finite.");
        if (!std::isfinite(zone.pan) || zone.pan < -1.0 || zone.pan > 1.0)
            addError(result, "render-model-pan-invalid", path + ".pan",
                     "Zone pan must be finite and normalized to -1 through 1.");
        const auto playbackEndFrame = resolveSampleEndFrame(zone.sampleEndFrame, sample.frameCount);
        if (zone.sampleEndFrame > sample.frameCount)
            addError(result, "render-model-end-frame-invalid", path + ".sampleEndFrame",
                     "Authored playback end must not exceed the retained source frame count.");
        if (zone.sampleStartFrame >= playbackEndFrame)
            addError(result, "render-model-start-frame-invalid", path + ".sampleStartFrame",
                     "Zone start frame must precede the resolved exclusive playback end.");
        if (zone.loopEnabled
            && (zone.loopStartFrame < zone.sampleStartFrame
                || zone.loopStartFrame >= zone.loopEndFrame
                || zone.loopEndFrame > playbackEndFrame))
            addError(result, "render-model-loop-range-invalid", path + ".loopStartFrame",
                     "Enabled loops require an ordered half-open range inside the playback region.");

        const auto snapshotZoneEntry = snapshotZonesById.find(zone.zoneId);
        const auto* snapshotZone = snapshotZoneEntry == snapshotZonesById.end()
            ? nullptr : snapshotZoneEntry->second;
        if (snapshotZone == nullptr)
            addError(result, "render-model-snapshot-route-missing", path + ".zoneId",
                     "Prepared route has no matching immutable snapshot route.");
        else if (!sameTopology(*snapshotZone, zone))
            addError(result, "render-model-route-topology-mismatch", path,
                     "Snapshot and prepared route topology must agree before rendering.");
        else
        {
            const auto snapshotGroupEntry = snapshotGroupsById.find(snapshotZone->groupId);
            const auto* snapshotGroupRoute = snapshotGroupEntry == snapshotGroupsById.end()
                ? nullptr : snapshotGroupEntry->second;
            if (snapshotGroupRoute == nullptr)
            {
                addError(result, "render-model-group-route-missing", path + ".zoneId",
                         "Prepared routes must resolve through an authored group route.");
            }
            else
            {
                const auto snapshotMembers = snapshotGroupZoneIds.find(snapshotGroupRoute->groupId);
                if (snapshotMembers == snapshotGroupZoneIds.end()
                    || !snapshotMembers->second.count(zone.zoneId))
                {
                    addError(result, "render-model-group-membership-invalid", path + ".zoneId",
                             "Authored group routes must contain each routed zone exactly where its groupId points.");
                }

                const auto preparedGroupEntry = preparedGroupsById.find(snapshotGroupRoute->groupId);
                const auto* preparedGroupRoute = preparedGroupEntry == preparedGroupsById.end()
                    ? nullptr : preparedGroupEntry->second;
                if (preparedGroupRoute == nullptr)
                {
                    addError(result, "render-model-prepared-group-route-missing", path + ".zoneId",
                             "Prepared content must retain the immutable group route for every rendered zone.");
                }
                else if (!preparedGroupZoneIds[preparedGroupRoute->groupId].count(zone.zoneId)
                         || preparedGroupRoute->gainDb != snapshotGroupRoute->gainDb
                         || preparedGroupRoute->pan != snapshotGroupRoute->pan
                         || preparedGroupRoute->routingBusId != snapshotGroupRoute->routingBusId
                         || preparedGroupRoute->routingSourceId != snapshotGroupRoute->routingSourceId)
                {
                    addError(result, "render-model-prepared-group-route-mismatch", path + ".zoneId",
                             "Prepared group routing metadata must match the immutable authored group route.");
                }
            }
        }
    }

    if (!std::isfinite(snapshot.masterGainDb))
        addError(result, "render-model-master-gain-invalid", "payload.snapshot.masterGainDb",
                 "Snapshot master gain must be finite.");
    if (!std::isfinite(prepared.masterGainDb))
        addError(result, "render-model-prepared-master-gain-invalid", "payload.prepared.masterGainDb",
                 "Prepared master gain must be finite.");
    if (snapshot.masterGainDb != prepared.masterGainDb)
        addError(result, "render-model-master-gain-mismatch", "payload.prepared.masterGainDb",
                 "Prepared content must retain the immutable snapshot master gain.");

    std::vector<VelocityCrossfadeTopologyZoneDefinition> crossfadeTopologyZones;
    crossfadeTopologyZones.reserve(prepared.zones.size());
    for (const auto& zone : prepared.zones)
    {
        VelocityCrossfadeTopologyZoneDefinition topologyZone;
        topologyZone.pairingKey = buildCrossfadePairingKey(zone);
        topologyZone.velocityLow = zone.velocityLow;
        topologyZone.velocityHigh = zone.velocityHigh;
        topologyZone.roundRobinPoolId = resolveRoundRobinPoolId(zone);
        topologyZone.roundRobinLength = zone.roundRobinLength;
        topologyZone.roundRobinPosition = zone.roundRobinPosition;
        topologyZone.crossfade = zone.velocityCrossfade;
        crossfadeTopologyZones.push_back(topologyZone);
    }

    std::vector<VelocityCrossfadeTopologyFinding> crossfadeTopologyFindings;
    buildFirstPassVelocityCrossfadeRuntimeTopology(crossfadeTopologyZones, &crossfadeTopologyFindings);
    for (const auto& finding : crossfadeTopologyFindings)
    {
        if (finding.zoneIndex >= prepared.zones.size())
            continue;

        const auto& zone = prepared.zones[finding.zoneIndex];
        addError(result,
                 "render-model-velocity-crossfade-topology-invalid",
                 "payload.prepared.zones[" + std::to_string(finding.zoneIndex) + "].velocityCrossfade",
                 buildCrossfadeTopologyMessage(zone.zoneId, finding.issue));
    }

    const auto routeSelected = [&](const PreparedPlaybackZoneHandle& zone)
    {
        const auto snapshotZoneEntry = snapshotZonesById.find(zone.zoneId);
        const auto* snapshotZone = snapshotZoneEntry == snapshotZonesById.end()
            ? nullptr : snapshotZoneEntry->second;
        return snapshotZone != nullptr
            && (options.selectedZoneId.empty()
                || (!options.retainedZoneIds.empty()
                        ? containsZoneId(options.retainedZoneIds, snapshotZone->id)
                        : snapshotZone->id == options.selectedZoneId))
            && (options.selectedGroupId.empty() || snapshotZone->groupId == options.selectedGroupId)
            && (options.selectedArticulationId.empty()
                || snapshotZone->articulationId == options.selectedArticulationId);
    };
    const auto selectedRouteCount = static_cast<std::size_t>(std::count_if(
        prepared.zones.begin(), prepared.zones.end(), routeSelected));
    if (selectedRouteCount == 0 && !prepared.zones.empty())
        addError(result, "render-model-route-selection-empty", "options",
                 "The normalized zone/articulation selection contains no prepared routes.");

    const auto errorCount = std::count_if(result.findings.begin(),
                                          result.findings.end(),
                                          [](const SamplerRenderModelFinding& finding)
                                          {
                                              return finding.severity == SamplerRenderModelFindingSeverity::error;
                                          });
    if (errorCount != 0)
        return result;

    auto model = std::shared_ptr<SamplerRenderModel>(new SamplerRenderModel());
    model->lane = payload->lane;
    model->revision = payload->revision;
    model->snapshotBuildId = payload->snapshotBuildId;
    model->preparedBuildId = payload->preparedBuildId;
    model->snapshotContentDigest = payload->snapshotContentDigest;
    model->preparedContentDigest = payload->preparedContentDigest;
    model->performanceProgram = prepared.performanceProgram;
    model->midiNoteOffset = options.midiNoteOffset;
    model->fixedVelocity = options.fixedVelocity;
    model->retainedActivationPayload = payload;
    model->samples.reserve(prepared.samples.size());
    for (std::size_t index = 0; index < prepared.samples.size(); ++index)
    {
        const auto& sample = prepared.samples[index];
        SampleDataSourceDescriptor descriptor;
        SampleDataSourcePtr dataSource = sample.dataSource;
        if (dataSource != nullptr)
        {
            descriptor = dataSource->descriptor();
        }
        else
        {
            descriptor.kind = SampleDataSourceKind::resident;
            descriptor.sourceId = sample.sampleSourceId;
            descriptor.canonicalSourceIdentity = sample.canonicalSourceIdentity.empty()
                ? sample.sampleSourceId : sample.canonicalSourceIdentity;
            descriptor.provenanceIdentity = sample.sourceFingerprintHex.empty()
                ? "prepared-build:" + std::to_string(payload->preparedBuildId)
                : sample.sourceFingerprintHex;
            descriptor.formatName = sample.formatName.empty() ? "decoded-float32" : sample.formatName;
            descriptor.channelLayout = sample.channelLayout.empty()
                ? (sample.channelCount == 1 ? "mono" : "stereo") : sample.channelLayout;
            descriptor.checksumHex = sample.sourceFingerprintHex;
            descriptor.generation = stableSourceGeneration(descriptor.provenanceIdentity);
            descriptor.sampleRate = sample.sampleRate;
            descriptor.frameCount = sample.frameCount;
            descriptor.channelCount = sample.channelCount;
            descriptor.bytesPerFrame = static_cast<std::uint64_t>(sample.channelCount) * sizeof(float);
            descriptor.dataSizeBytes = descriptor.frameCount * descriptor.bytesPerFrame;
            dataSource = std::make_shared<ResidentSampleDataSource>(descriptor,
                                                                   sample.decodedSampleData);
        }
        SamplerRenderSample renderSample;
        renderSample.preparedSampleIndex = index;
        renderSample.sampleSourceId = sample.sampleSourceId;
        renderSample.streamSampleId = sample.streamSampleId;
        renderSample.sampleRate = sample.sampleRate;
        renderSample.frameCount = sample.frameCount;
        renderSample.channelCount = sample.channelCount;
        renderSample.sourceDescriptor = std::move(descriptor);
        renderSample.dataSource = std::move(dataSource);
        renderSample.decodedSampleData = sample.decodedSampleData;
        model->samples.push_back(std::move(renderSample));
    }

    model->routes.reserve(selectedRouteCount);
    std::vector<const CompiledPerformanceTriggerRoute*> triggerRoutesByZone(
        prepared.zones.size(), nullptr);
    for (const auto& triggerRoute : prepared.performanceProgram.triggerRoutes)
        if (triggerRoute.zoneIndex < triggerRoutesByZone.size())
            triggerRoutesByZone[triggerRoute.zoneIndex] = &triggerRoute;
    for (std::size_t index = 0; index < prepared.zones.size(); ++index)
    {
        const auto& zone = prepared.zones[index];
        if (!routeSelected(zone))
            continue;
        const auto snapshotZoneEntry = snapshotZonesById.find(zone.zoneId);
        const auto* snapshotZone = snapshotZoneEntry == snapshotZonesById.end()
            ? nullptr : snapshotZoneEntry->second;
        const auto snapshotGroupEntry = snapshotZone == nullptr
            ? snapshotGroupsById.end() : snapshotGroupsById.find(snapshotZone->groupId);
        const auto* snapshotGroupRoute = snapshotGroupEntry == snapshotGroupsById.end()
            ? nullptr : snapshotGroupEntry->second;
        const auto gainDb = combineRouteGainDb(zone.gainDb,
                                               snapshotGroupRoute == nullptr ? 0.0 : snapshotGroupRoute->gainDb,
                                               snapshot.masterGainDb);
        const auto pan = snapshotGroupRoute == nullptr
            ? zone.pan
            : combineGroupPan(zone.pan, snapshotGroupRoute->pan);
        const auto* triggerRoute = triggerRoutesByZone[index];
        model->routes.push_back({ index,
                                  zone.preparedSampleIndex,
                                  zone.zoneId,
                                  zone.sampleSourceId,
                                  zone.rootKey,
                                  options.auditionSelectedZone ? 0 : zone.keyLow,
                                  options.auditionSelectedZone ? 127 : zone.keyHigh,
                                  options.auditionSelectedZone ? 1 : zone.velocityLow,
                                  options.auditionSelectedZone ? 127 : zone.velocityHigh,
                                  gainDb,
                                  pan,
                                  zone.sampleStartFrame,
                                  zone.loopEnabled,
                                  zone.loopStartFrame,
                                  zone.loopEndFrame,
                                  zone.releaseSeconds,
                                  zone.releaseShape,
                                  zone.roundRobin,
                                  zone.roundRobinLength,
                                  zone.roundRobinPosition,
                                  zone.triggerMode,
                                  options.auditionSelectedZone ? VelocityCrossfadeDescriptor {} : zone.velocityCrossfade,
                                  options.auditionSelectedZone
                                      ? VelocityCrossfadeRuntimeDescriptor {}
                                      : zone.velocityCrossfadeRuntime,
                                  index < prepared.performanceProgram.zoneArticulationIndices.size()
                                      ? prepared.performanceProgram.zoneArticulationIndices[index]
                                      : kInvalidPerformanceProgramIndex,
                                  triggerRoute == nullptr
                                      ? PerformanceEventKind::noteOn : triggerRoute->event,
                                  triggerRoute == nullptr
                                      ? PerformanceSustainCondition::any : triggerRoute->sustain,
                                  triggerRoute == nullptr
                                      ? PerformancePitchSource::eventNote : triggerRoute->pitchSource,
                                  triggerRoute == nullptr
                                      || triggerRoute->exclusiveGroupIndex >= prepared.performanceProgram.exclusiveGroupStableIds.size()
                                      ? 0 : prepared.performanceProgram.exclusiveGroupStableIds[triggerRoute->exclusiveGroupIndex],
                                  triggerRoute == nullptr
                                      ? 0 : triggerRoute->chokeTargetMask,
                                  triggerRoute == nullptr
                                      ? 0.0f : triggerRoute->chokeReleaseSeconds,
                                  zone.fineTuneCents,
                                  zone.amplitudeVelocityTracking,
                                  zone.controllerConditions,
                                  zone.damper });
        model->routes.back().loopMode = zone.loopMode;
        model->routes.back().sampleEndFrame = zone.sampleEndFrame;
    }
    for (const auto& route : model->routes)
    {
        if (route.performanceEvent != PerformanceEventKind::noteOn)
            continue;
        model->continuousDamperEnabled = route.damper.dynamicRelease
            || route.damper.sustainControllerNumber != legacySustainControllerNumber
            || route.damper.sustainThreshold != legacySustainThreshold;
        model->sustainControllerNumber = route.damper.sustainControllerNumber;
        model->sustainThreshold = route.damper.sustainThreshold;
        if (model->continuousDamperEnabled)
            break;
    }
    model->performanceProgram.defaultArticulationIndex = resolveSelectedArticulationIndex(
        options,
        model->routes,
        model->performanceProgram);

    result.built = true;
    result.model = std::move(model);
    return result;
}
} // namespace drs::engine
