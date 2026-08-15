#include "drs/engine/PerformancePublishPreparation.h"
#include "drs/engine/SampleDataSource.h"

#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace drs::engine
{
namespace
{
void addError(PerformancePublishPreparationResult& result,
              std::string code,
              std::string path,
              std::string message)
{
    result.findings.push_back({ PlaybackSnapshotFindingSeverity::error,
                                std::move(code), std::move(path), std::move(message) });
}

bool hasErrors(const std::vector<PlaybackSnapshotFinding>& findings)
{
    return std::any_of(findings.begin(), findings.end(), [](const auto& finding)
    {
        return finding.severity == PlaybackSnapshotFindingSeverity::error;
    });
}

bool zonesMatch(const PlaybackSnapshotZone& authored, const PreparedPlaybackZoneHandle& prepared)
{
    return authored.id == prepared.zoneId
        && authored.sampleSourceId == prepared.sampleSourceId
        && authored.rootKey == prepared.rootKey
        && authored.keyLow == prepared.keyLow
        && authored.keyHigh == prepared.keyHigh
        && authored.velocityLow == prepared.velocityLow
        && authored.velocityHigh == prepared.velocityHigh
        && authored.gainDb == prepared.gainDb
        && authored.pan == prepared.pan
        && authored.sampleStartFrame == prepared.sampleStartFrame
        && authored.loopEnabled == prepared.loopEnabled
        && authored.loopStartFrame == prepared.loopStartFrame
        && authored.loopEndFrame == prepared.loopEndFrame
        && authored.releaseSeconds == prepared.releaseSeconds
        && authored.releaseShape == prepared.releaseShape
        && authored.damper == prepared.damper
        && authored.roundRobinLength == prepared.roundRobinLength
        && authored.roundRobinPosition == prepared.roundRobinPosition
        && authored.triggerMode == prepared.triggerMode;
}

bool groupRoutesMatch(const PlaybackSnapshotGroupRoute& authored, const PreparedPlaybackGroupRoute& prepared)
{
    return authored.groupId == prepared.groupId
        && authored.articulationIds == prepared.articulationIds
        && authored.zoneIds == prepared.zoneIds
        && authored.displayName == prepared.displayName
        && authored.displayOrder == prepared.displayOrder
        && authored.routingSourceId == prepared.routingSourceId
        && authored.gainDb == prepared.gainDb
        && authored.pan == prepared.pan
        && authored.routingBusId == prepared.routingBusId
        && authored.auditionAnchorZoneId == prepared.auditionAnchorZoneId;
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
} // namespace

PerformancePublishPreparationResult validatePerformancePublishPreparationImpl(
    const PerformancePublishRequestIdentity& identity,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const PreparedPlaybackBuildResult& preparedResult,
    const bool recomputeDigests)
{
    PerformancePublishPreparationResult result;
    result.publishResult.identity = identity;
    result.publishResult.preparedBuildId = preparedResult.buildId;
    result.publishResult.preparedContentDigest = preparedResult.prepared.preparedContentDigest;
    result.publishResult.routeDigest = preparedResult.prepared.routeDigest;
    result.publishResult.sourceProvenanceDigest = preparedResult.prepared.sourceProvenanceDigest;
    result.publishResult.preparedMacroSchemaDigest = preparedResult.prepared.macroSchemaDigest;
    result.findings = snapshotResult.findings;
    result.findings.insert(result.findings.end(), preparedResult.findings.begin(), preparedResult.findings.end());

    if (!snapshotResult.built || !snapshotResult.activationEligible)
        addError(result, "publish-snapshot-ineligible", "snapshot",
                 "The immutable authored snapshot is incomplete or activation-ineligible.");
    if (!preparedResult.built || !preparedResult.activationEligible)
    {
        const auto canceled = preparedResult.lifecycleState == PlaybackSnapshotLifecycleState::canceled;
        addError(result,
                 canceled ? "publish-preparation-canceled" : "publish-prepared-result-ineligible",
                 "prepared",
                 canceled ? "Full-project preparation was canceled; no partial payload is eligible."
                          : "The worker did not produce a complete activation-eligible project result.");
    }

    const auto& snapshot = snapshotResult.snapshot;
    const auto& prepared = preparedResult.prepared;
    if (identity.requestId == 0 || identity.projectGeneration == 0)
        addError(result, "publish-identity-invalid", "identity", "Publish identity is incomplete.");
    if (identity.draftRevision != snapshotResult.requestedDraftRevision
        || identity.draftRevision != snapshot.draftRevision
        || identity.draftRevision != preparedResult.requestedDraftRevision
        || identity.draftRevision != prepared.draftRevision)
        addError(result, "publish-revision-mismatch", "identity.draftRevision",
                 "Snapshot and prepared project must belong to the exact requested draft revision.");
    if (snapshotResult.buildId == 0 || preparedResult.snapshotBuildId != snapshotResult.buildId
        || prepared.snapshotBuildId != snapshotResult.buildId)
        addError(result, "publish-snapshot-build-mismatch", "prepared.snapshotBuildId",
                 "Prepared content is not linked to the exact immutable snapshot build.");

    const auto authoredDigest = recomputeDigests
        ? computePlaybackSnapshotContentDigest(snapshot) : snapshot.contentDigest;
    if (snapshot.contentDigest.empty() || identity.authoredContentDigest != snapshot.contentDigest
        || authoredDigest != snapshot.contentDigest)
        addError(result, "publish-authored-digest-mismatch", "snapshot.contentDigest",
                 "The authored project digest does not match the requested immutable snapshot.");
    if (prepared.snapshotContentDigest != snapshot.contentDigest)
        addError(result, "publish-prepared-snapshot-digest-mismatch", "prepared.snapshotContentDigest",
                 "Prepared content was produced from a different authored snapshot.");
    const auto dspGraphDigest = recomputeDigests
        ? computePlaybackSnapshotDspGraphDigest(snapshot) : snapshot.dspGraphDigest;
    if (snapshot.dspGraphDigest.empty() || snapshot.dspGraphDigest != dspGraphDigest
        || prepared.snapshotDspGraphDigest != dspGraphDigest || prepared.dspGraphDigest != dspGraphDigest)
    {
        addError(result, "publish-dsp-graph-digest-mismatch", "prepared.dspGraphDigest",
                 "Prepared content was not produced from the exact immutable DSP graph identity.");
    }
    if (prepared.preparedContentDigest.empty()
        || (recomputeDigests
            && prepared.preparedContentDigest != computePreparedPlaybackContentDigest(prepared)))
        addError(result, "publish-prepared-digest-mismatch", "prepared.preparedContentDigest",
                 "Prepared content digest is missing or does not describe the worker result.");

    const auto macroDigest = recomputeDigests
        ? computePlaybackSnapshotMacroSchemaDigest(snapshot) : prepared.macroSchemaDigest;
    if (identity.macroSchemaDigest.empty() || prepared.macroSchemaDigest != macroDigest
        || prepared.macroSchemaDigest != identity.macroSchemaDigest)
        addError(result, "publish-macro-schema-digest-mismatch", "prepared.macroSchemaDigest",
                 "Prepared macro schema does not match the requested immutable macro schema.");
    if (prepared.routeDigest.empty()
        || (recomputeDigests
            && prepared.routeDigest != computePreparedPlaybackRouteDigest(snapshot, prepared)))
        addError(result, "publish-route-digest-mismatch", "prepared.routeDigest",
                 "Prepared routing topology digest is missing or inconsistent.");
    if (prepared.sourceProvenanceDigest.empty()
        || (recomputeDigests
            && prepared.sourceProvenanceDigest != computePreparedPlaybackSourceProvenanceDigest(prepared)))
        addError(result, "publish-source-provenance-digest-mismatch", "prepared.sourceProvenanceDigest",
                 "Prepared source provenance digest is missing or inconsistent.");

    if (snapshot.sampleIdentities.size() != prepared.samples.size()
        || prepared.samples.size() != prepared.streams.size()
        || prepared.samples.size() != prepared.ownershipRecords.size())
        addError(result, "publish-source-coverage-incomplete", "prepared.samples",
                 "Every authored source must have exactly one sample, stream, and ownership handle.");
    if (snapshot.zones.size() != prepared.zones.size())
        addError(result, "publish-zone-coverage-incomplete", "prepared.zones",
                 "Every authored zone must have exactly one immutable prepared route.");
    if (snapshot.groupRoutes.size() != prepared.groupRoutes.size())
        addError(result, "publish-group-coverage-incomplete", "prepared.groupRoutes",
                 "Every authored group route must have exactly one immutable prepared group route.");

    std::unordered_map<std::string, const PreparedPlaybackSampleHandle*> samples;
    for (std::size_t index = 0; index < prepared.samples.size(); ++index)
    {
        const auto& sample = prepared.samples[index];
        const auto path = "prepared.samples[" + std::to_string(index) + "]";
        if (sample.sampleSourceId.empty() || !samples.emplace(sample.sampleSourceId, &sample).second)
            addError(result, "publish-prepared-source-identity-invalid", path + ".sampleSourceId",
                     "Prepared sample source ids must be non-empty and unique.");
        if (sample.canonicalSourceIdentity.empty() || sample.sourceFingerprintHex.empty()
            || sample.formatName.empty() || sample.frameCount == 0 || sample.channelCount == 0
            || (sample.decodedSampleData == nullptr && sample.dataSource == nullptr))
            addError(result, "publish-source-provenance-incomplete", path,
                     "Prepared source provenance and a resident or paged immutable source must be complete.");
        else if (sample.dataSource != nullptr
                 && (!validateSampleDataSourceDescriptor(sample.dataSource->descriptor()).valid
                     || sample.dataSource->descriptor().frameCount != sample.frameCount
                     || sample.dataSource->descriptor().channelCount != sample.channelCount))
            addError(result, "publish-paged-source-incomplete", path + ".dataSource",
                     "Paged source descriptor and immutable prepared sample dimensions must match.");
        else if (sample.dataSource == nullptr
                 && (sample.decodedSampleData->normalizedChannels.size() != sample.channelCount
                     || std::any_of(sample.decodedSampleData->normalizedChannels.begin(),
                                    sample.decodedSampleData->normalizedChannels.end(),
                                    [&](const auto& channel) { return channel.size() < sample.frameCount; })))
            addError(result, "publish-decoded-source-incomplete", path + ".decodedSampleData",
                     "Decoded channel and frame coverage must match immutable source metadata.");
    }
    for (std::size_t index = 0; index < snapshot.sampleIdentities.size(); ++index)
    {
        const auto& authored = snapshot.sampleIdentities[index];
        const auto found = samples.find(authored.sampleSourceId);
        if (found == samples.end())
            addError(result, "publish-authored-source-not-prepared",
                     "snapshot.sampleIdentities[" + std::to_string(index) + "]",
                     "An authored sample source is absent from the prepared project.");
        else if (found->second->canonicalSourcePath != authored.sourcePath
                 || found->second->canonicalSourceIdentity
                    != authored.sampleSourceId + "|" + authored.sourcePath)
            addError(result, "publish-authored-source-provenance-mismatch",
                     "snapshot.sampleIdentities[" + std::to_string(index) + "].sourcePath",
                     "Prepared canonical source identity does not match the authored source.");
    }

    std::unordered_map<std::string, const PreparedPlaybackStreamHandle*> streams;
    for (std::size_t index = 0; index < prepared.streams.size(); ++index)
    {
        const auto& stream = prepared.streams[index];
        if (stream.sampleSourceId.empty() || !streams.emplace(stream.sampleSourceId, &stream).second)
            addError(result, "publish-prepared-stream-identity-invalid",
                     "prepared.streams[" + std::to_string(index) + "].sampleSourceId",
                     "Prepared stream source ids must be non-empty and unique.");
    }

    std::unordered_set<std::string> authoredZoneIds;
    std::unordered_map<std::string, const PlaybackSnapshotZone*> authoredZones;
    authoredZones.reserve(snapshot.zones.size());
    for (const auto& zone : snapshot.zones)
    {
        authoredZoneIds.insert(zone.id);
        authoredZones.emplace(zone.id, &zone);
    }
    std::unordered_set<std::string> routedArticulationZones;
    std::unordered_set<std::string> articulationIds;
    for (std::size_t index = 0; index < snapshot.articulationRoutes.size(); ++index)
    {
        const auto& route = snapshot.articulationRoutes[index];
        const auto path = "snapshot.articulationRoutes[" + std::to_string(index) + "]";
        if (route.articulationId.empty() || !articulationIds.insert(route.articulationId).second)
            addError(result, "publish-articulation-route-identity-invalid", path + ".articulationId",
                     "Articulation route ids must be non-empty and unique.");
        for (const auto& zoneId : route.zoneIds)
        {
            const auto authored = authoredZones.find(zoneId);
            if (authored == authoredZones.end()
                || authored->second->articulationId != route.articulationId
                || !routedArticulationZones.insert(zoneId).second)
                addError(result, "publish-articulation-route-invalid", path + ".zoneIds",
                         "Every zone must occur once under its authored articulation route.");
        }
    }
    if (routedArticulationZones != authoredZoneIds)
        addError(result, "publish-articulation-coverage-incomplete", "snapshot.articulationRoutes",
                 "Articulation routes must cover the complete authored zone set exactly once.");

    std::unordered_set<std::string> routedGroupZones;
    std::unordered_set<std::string> groupIds;
    for (std::size_t index = 0; index < snapshot.groupRoutes.size(); ++index)
    {
        const auto& route = snapshot.groupRoutes[index];
        const auto path = "snapshot.groupRoutes[" + std::to_string(index) + "]";
        if (route.groupId.empty() || !groupIds.insert(route.groupId).second)
            addError(result, "publish-group-route-identity-invalid", path + ".groupId",
                     "Group route ids must be non-empty and unique.");
        for (const auto& zoneId : route.zoneIds)
        {
            const auto authored = authoredZones.find(zoneId);
            if (authored == authoredZones.end()
                || authored->second->groupId != route.groupId
                || !routedGroupZones.insert(zoneId).second)
                addError(result, "publish-group-route-invalid", path + ".zoneIds",
                         "Every zone must occur once under its authored group route.");
        }
    }
    if (routedGroupZones != authoredZoneIds)
        addError(result, "publish-group-coverage-incomplete", "snapshot.groupRoutes",
                 "Group routes must cover the complete authored zone set exactly once.");

    std::unordered_map<std::string, const PreparedPlaybackGroupRoute*> preparedGroupRoutes;
    for (std::size_t index = 0; index < prepared.groupRoutes.size(); ++index)
    {
        const auto& route = prepared.groupRoutes[index];
        const auto path = "prepared.groupRoutes[" + std::to_string(index) + "]";
        if (route.groupId.empty() || !preparedGroupRoutes.emplace(route.groupId, &route).second)
        {
            addError(result, "publish-prepared-group-route-identity-invalid", path + ".groupId",
                     "Prepared group route ids must be non-empty and unique.");
        }
    }
    for (std::size_t index = 0; index < snapshot.groupRoutes.size(); ++index)
    {
        const auto& route = snapshot.groupRoutes[index];
        const auto found = preparedGroupRoutes.find(route.groupId);
        if (found == preparedGroupRoutes.end() || !groupRoutesMatch(route, *found->second))
        {
            addError(result, "publish-prepared-group-route-mismatch",
                     "snapshot.groupRoutes[" + std::to_string(index) + "]",
                     "Prepared group metadata or membership does not match the authored immutable snapshot.");
        }
    }

    std::unordered_set<std::string> fxSlotIds;
    for (const auto& slot : snapshot.fxSlots)
        fxSlotIds.insert(slot.id);
    std::unordered_set<std::string> busIds;
    std::unordered_map<std::string, const PlaybackSnapshotRoutingBusReference*> busesById;
    for (std::size_t index = 0; index < snapshot.routingBuses.size(); ++index)
    {
        const auto& bus = snapshot.routingBuses[index];
        const auto path = "snapshot.routingBuses[" + std::to_string(index) + "]";
        if (bus.id.empty() || !busIds.insert(bus.id).second)
            addError(result, "publish-routing-bus-identity-invalid", path + ".id",
                     "Routing bus ids must be non-empty and unique.");
        else
            busesById.emplace(bus.id, &bus);

        if (bus.inputSourceId.empty())
        {
            addError(result, "publish-routing-input-source-missing", path + ".inputSourceId",
                     "Routing buses must declare an immutable input source.");
        }
        else if (bus.inputSourceId != "master" && !authoredZoneIds.count(bus.inputSourceId))
        {
            const auto groupId = extractGroupIdFromRoutingSourceId(bus.inputSourceId);
            if (groupId.empty() || !groupIds.count(groupId))
            {
                addError(result, "publish-routing-input-source-invalid", path + ".inputSourceId",
                         "Routing buses may only source audio from master, authored zones, or authored groups.");
            }
        }

        for (const auto& fxSlotId : bus.fxSlotIds)
            if (!fxSlotIds.count(fxSlotId))
                addError(result, "publish-routing-fx-slot-invalid", path + ".fxSlotIds",
                         "Routing buses may only reference captured FX slots.");
    }

    std::unordered_set<std::string> claimedGroupBusIds;
    for (std::size_t index = 0; index < snapshot.groupRoutes.size(); ++index)
    {
        const auto& route = snapshot.groupRoutes[index];
        const auto path = "snapshot.groupRoutes[" + std::to_string(index) + "]";
        if (route.routingBusId.empty())
            continue;

        const auto bus = busesById.find(route.routingBusId);
        if (bus == busesById.end())
        {
            addError(result, "publish-group-routing-bus-invalid", path + ".routingBusId",
                     "Group routes may only reference captured routing buses.");
            continue;
        }

        if (bus->second->inputSourceId != route.routingSourceId)
        {
            addError(result, "publish-group-routing-bus-mismatch", path + ".routingBusId",
                     "Group routes must own a routing bus sourced from their own groups/<groupId> source.");
        }
        else if (!claimedGroupBusIds.insert(route.routingBusId).second)
        {
            addError(result, "publish-group-routing-bus-duplicate", path + ".routingBusId",
                     "Routing buses must not be claimed by more than one group route.");
        }
    }

    for (std::size_t index = 0; index < snapshot.routingBuses.size(); ++index)
    {
        const auto& bus = snapshot.routingBuses[index];
        const auto groupId = extractGroupIdFromRoutingSourceId(bus.inputSourceId);
        if (!groupId.empty() && !claimedGroupBusIds.count(bus.id))
        {
            addError(result, "publish-orphaned-group-routing-bus",
                     "snapshot.routingBuses[" + std::to_string(index) + "].inputSourceId",
                     "Group-targeted routing buses must be claimed by exactly one group route.");
        }
    }

    std::unordered_map<std::string, const PreparedPlaybackZoneHandle*> zones;
    for (std::size_t index = 0; index < prepared.zones.size(); ++index)
    {
        const auto& zone = prepared.zones[index];
        const auto path = "prepared.zones[" + std::to_string(index) + "]";
        if (zone.zoneId.empty() || !zones.emplace(zone.zoneId, &zone).second)
            addError(result, "publish-prepared-zone-identity-invalid", path + ".zoneId",
                     "Prepared zone ids must be non-empty and unique.");
        if (zone.preparedSampleIndex >= prepared.samples.size()
            || zone.preparedStreamIndex >= prepared.streams.size())
        {
            addError(result, "publish-prepared-zone-handle-invalid", path,
                     "Prepared zone sample and stream indices must refer to immutable handles.");
            continue;
        }
        const auto& sample = prepared.samples[zone.preparedSampleIndex];
        const auto& stream = prepared.streams[zone.preparedStreamIndex];
        if (sample.sampleSourceId != zone.sampleSourceId || stream.sampleSourceId != zone.sampleSourceId
            || sample.streamSampleId != zone.streamSampleId || stream.streamSampleId != zone.streamSampleId)
            addError(result, "publish-prepared-zone-binding-mismatch", path,
                     "Prepared zone bindings do not agree with their sample and stream handles.");
        if (zone.sampleStartFrame >= sample.frameCount
            || (zone.loopEnabled && (zone.loopStartFrame >= zone.loopEndFrame
                                     || zone.loopEndFrame > sample.frameCount)))
            addError(result, "publish-prepared-zone-range-invalid", path,
                     "Prepared sample start and loop ranges must fit the decoded source.");
    }
    for (std::size_t index = 0; index < snapshot.zones.size(); ++index)
    {
        const auto& authored = snapshot.zones[index];
        const auto found = zones.find(authored.id);
        if (found == zones.end() || !zonesMatch(authored, *found->second))
            addError(result, "publish-authored-zone-not-prepared",
                     "snapshot.zones[" + std::to_string(index) + "]",
                     "An authored zone is absent from or differs from the prepared topology.");
    }

    result.completeProject = !hasErrors(result.findings);
    result.activationEligible = result.completeProject;
    result.publishResult.completeProject = result.completeProject;
    result.publishResult.activationEligible = result.activationEligible;
    for (const auto& finding : result.findings)
    {
        PerformancePublishFindingSeverity severity = finding.severity == PlaybackSnapshotFindingSeverity::error
            ? PerformancePublishFindingSeverity::error
            : PerformancePublishFindingSeverity::warning;
        result.publishResult.findings.push_back({ severity, finding.code, finding.path, finding.message });
    }
    return result;
}

PerformancePublishPreparationResult validatePerformancePublishPreparation(
    const PerformancePublishRequestIdentity& identity,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const PreparedPlaybackBuildResult& preparedResult)
{
    return validatePerformancePublishPreparationImpl(
        identity, snapshotResult, preparedResult, true);
}

PerformancePublishPreparationResult validateExactPreviewReuseForPerformance(
    const PerformancePublishRequestIdentity& identity,
    const PlaybackSnapshotBuildResult& snapshotResult,
    const PreparedPlaybackBuildResult& preparedResult)
{
    return validatePerformancePublishPreparationImpl(
        identity, snapshotResult, preparedResult, false);
}
} // namespace drs::engine
