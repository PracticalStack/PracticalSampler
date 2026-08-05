#include "drs/engine/AuthoringPreviewPreparation.h"

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"

#include <algorithm>
#include <numeric>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace drs::engine
{
namespace
{
void addError(AuthoringPreviewPreparationResult& result,
              std::string code,
              std::string path,
              std::string message)
{
    result.findings.push_back({ SamplerRenderModelFindingSeverity::error,
                                std::move(code), std::move(path), std::move(message) });
}

bool containsZoneId(const std::vector<std::string>& zoneIds, const std::string& zoneId)
{
    return std::find(zoneIds.begin(), zoneIds.end(), zoneId) != zoneIds.end();
}

void retainZoneId(std::vector<std::string>& zoneIds, const std::string& zoneId, bool& changed)
{
    if (zoneId.empty() || containsZoneId(zoneIds, zoneId))
        return;

    zoneIds.push_back(zoneId);
    changed = true;
}

void expandRetainedZoneDependencies(const ImmutablePreparedPlayback& prepared,
                                    std::vector<std::string>& retainedZoneIds)
{
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (const auto& zone : prepared.zones)
        {
            if (!containsZoneId(retainedZoneIds, zone.zoneId))
                continue;

            retainZoneId(retainedZoneIds, zone.velocityCrossfadeRuntime.fadeInNeighborZoneId, changed);
            retainZoneId(retainedZoneIds, zone.velocityCrossfadeRuntime.fadeOutNeighborZoneId, changed);

            if (!zone.roundRobin.has_value() || zone.roundRobin->poolId.empty())
                continue;

            for (const auto& candidate : prepared.zones)
            {
                if (!candidate.roundRobin.has_value()
                    || candidate.roundRobin->poolId != zone.roundRobin->poolId)
                {
                    continue;
                }

                retainZoneId(retainedZoneIds, candidate.zoneId, changed);
            }
        }
    }
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

CompiledPerformanceProgram remapPerformanceProgramForScopedZones(
    const CompiledPerformanceProgram& sourceProgram,
    const std::vector<std::size_t>& retainedZoneIndices,
    const std::unordered_map<std::size_t, std::size_t>& zoneIndexMap,
    const std::string& selectedArticulationId)
{
    auto scopedProgram = sourceProgram;
    scopedProgram.zoneArticulationIndices.clear();
    if (!sourceProgram.zoneArticulationIndices.empty())
    {
        scopedProgram.zoneArticulationIndices.reserve(retainedZoneIndices.size());
        for (const auto retainedZoneIndex : retainedZoneIndices)
        {
            if (retainedZoneIndex < sourceProgram.zoneArticulationIndices.size())
                scopedProgram.zoneArticulationIndices.push_back(
                    sourceProgram.zoneArticulationIndices[retainedZoneIndex]);
            else
            scopedProgram.zoneArticulationIndices.push_back(kInvalidPerformanceProgramIndex);
        }
    }
    if (!selectedArticulationId.empty())
    {
        const auto selectedStableId = stableIdHash(selectedArticulationId);
        for (std::size_t articulationIndex = 0;
             articulationIndex < scopedProgram.articulationStableIds.size();
             ++articulationIndex)
        {
            if (scopedProgram.articulationStableIds[articulationIndex] == selectedStableId)
            {
                scopedProgram.defaultArticulationIndex
                    = static_cast<std::uint32_t>(articulationIndex);
                break;
            }
        }
    }
    if (scopedProgram.defaultArticulationIndex >= scopedProgram.articulationCount)
    {
        for (const auto articulationIndex : scopedProgram.zoneArticulationIndices)
        {
            if (articulationIndex < scopedProgram.articulationCount)
            {
                scopedProgram.defaultArticulationIndex = articulationIndex;
                break;
            }
        }
    }

    scopedProgram.triggerRoutes.clear();
    scopedProgram.triggerRoutes.reserve(sourceProgram.triggerRoutes.size());
    for (const auto& route : sourceProgram.triggerRoutes)
    {
        const auto mappedZone = zoneIndexMap.find(route.zoneIndex);
        if (mappedZone == zoneIndexMap.end())
            continue;

        auto scopedRoute = route;
        scopedRoute.zoneIndex = static_cast<std::uint32_t>(mappedZone->second);
        scopedProgram.triggerRoutes.push_back(std::move(scopedRoute));
    }

    for (std::size_t event = 0, first = 0; event < scopedProgram.eventRanges.size(); ++event)
    {
        auto& range = scopedProgram.eventRanges[event];
        range.firstRoute = static_cast<std::uint32_t>(first);
        while (first < scopedProgram.triggerRoutes.size()
               && static_cast<std::size_t>(scopedProgram.triggerRoutes[first].event) == event)
        {
            ++first;
        }
        range.routeCount = static_cast<std::uint32_t>(first - range.firstRoute);
    }

    scopedProgram.retainedBytes = sizeof(CompiledPerformanceProgram)
        + scopedProgram.triggerRoutes.size() * sizeof(CompiledPerformanceTriggerRoute)
        + scopedProgram.roundRobinResets.size() * sizeof(CompiledPerformanceRoundRobinReset)
        + scopedProgram.articulationStableIds.size() * sizeof(std::uint64_t)
        + scopedProgram.exclusiveGroupStableIds.size() * sizeof(std::uint64_t)
        + scopedProgram.roundRobinPoolStableIds.size() * sizeof(std::uint64_t)
        + scopedProgram.zoneArticulationIndices.size() * sizeof(std::uint32_t);
    return scopedProgram;
}

template <typename Route>
void retainSelectedZones(std::vector<Route>& routes, const std::vector<std::string>& retainedZoneIds)
{
    for (auto iterator = routes.begin(); iterator != routes.end();)
    {
        iterator->zoneIds.erase(std::remove_if(iterator->zoneIds.begin(), iterator->zoneIds.end(),
                                              [&](const std::string& zoneId)
                                              {
                                                  return !containsZoneId(retainedZoneIds, zoneId);
                                              }),
                                iterator->zoneIds.end());
        if (iterator->zoneIds.empty())
            iterator = routes.erase(iterator);
        else
            ++iterator;
    }
}
} // namespace

AuthoringPreviewPreparationResult prepareAuthoringPreviewRenderModel(
    const PlaybackActivationPayloadPtr& preparedDraftPayload,
    const AuthoringPreviewRequest& request)
{
    AuthoringPreviewPreparationResult result;
    result.scope = request.identity.scope;
    if (preparedDraftPayload == nullptr)
    {
        addError(result, "preview-prepared-payload-missing", "payload",
                 "Preview requires a completed immutable prepared-draft payload.");
        return result;
    }
    if (preparedDraftPayload->lane != PlaybackActivationLane::preview)
    {
        addError(result, "preview-prepared-lane-invalid", "payload.lane",
                 "Authoring Preview can only consume the Preview activation lane.");
        return result;
    }
    if (preparedDraftPayload->revision != request.identity.draftRevision)
    {
        addError(result, "preview-prepared-revision-mismatch", "payload.revision",
                 "Prepared payload revision does not match the Preview request identity.");
        return result;
    }

    const auto fullValidation = buildSamplerRenderModel(preparedDraftPayload);
    if (!fullValidation.built || fullValidation.model == nullptr)
    {
        result.findings = fullValidation.findings;
        return result;
    }
    result.validatedZoneCount = fullValidation.model->getRoutes().size();

    if (request.identity.scope == AuthoringPreviewScope::currentDraft)
    {
        result.prepared = true;
        result.scopedPayload = preparedDraftPayload;
        result.model = fullValidation.model;
        result.retainedZoneCount = result.model->getRoutes().size();
        result.retainedSampleCount = result.model->getSamples().size();
        return result;
    }

    const auto selectedZoneScope = request.identity.scope == AuthoringPreviewScope::selectedZone;
    const auto selectedGroupScope = request.identity.scope == AuthoringPreviewScope::selectedGroup;

    if (selectedZoneScope && request.identity.selectedZoneId.empty())
    {
        addError(result, "preview-selected-zone-missing", "request.selectedZoneId",
                 "Selected-zone Preview requires an explicit selected-zone identity.");
        return result;
    }
    if (selectedGroupScope && request.identity.selectedGroupId.empty())
    {
        addError(result, "preview-selected-group-missing", "request.selectedGroupId",
                 "Selected-group Preview requires an explicit selected-group identity.");
        return result;
    }

    const auto& sourceSnapshot = *preparedDraftPayload->snapshot;
    const auto& sourcePrepared = *preparedDraftPayload->prepared;
    std::vector<std::string> retainedZoneIds;
    if (selectedZoneScope)
    {
        retainedZoneIds.push_back(request.identity.selectedZoneId);
    }
    else
    {
        for (const auto& zone : sourceSnapshot.zones)
        {
            if (zone.groupId == request.identity.selectedGroupId)
                retainedZoneIds.push_back(zone.id);
        }
    }
    expandRetainedZoneDependencies(sourcePrepared, retainedZoneIds);
    if (retainedZoneIds.empty())
    {
        addError(result,
                 selectedZoneScope ? "preview-selected-zone-unprepared"
                                   : "preview-selected-group-unprepared",
                 selectedZoneScope ? "request.selectedZoneId"
                                   : "request.selectedGroupId",
                 selectedZoneScope
                     ? "The selected zone is not present in the validated prepared draft."
                     : "The selected group is not present in the validated prepared draft.");
        return result;
    }

    const auto firstRetainedSnapshotZone = std::find_if(
        sourceSnapshot.zones.begin(),
        sourceSnapshot.zones.end(),
        [&](const PlaybackSnapshotZone& zone)
        {
            return containsZoneId(retainedZoneIds, zone.id);
        });
    if (firstRetainedSnapshotZone == sourceSnapshot.zones.end())
    {
        addError(result,
                 selectedZoneScope ? "preview-selected-zone-unprepared"
                                   : "preview-selected-group-unprepared",
                 selectedZoneScope ? "request.selectedZoneId"
                                   : "request.selectedGroupId",
                 "The requested Preview scope does not resolve to validated snapshot routes.");
        return result;
    }
    const auto scopedArticulationId = firstRetainedSnapshotZone->articulationId;

    auto scopedSnapshot = sourceSnapshot;
    scopedSnapshot.zones.erase(
        std::remove_if(scopedSnapshot.zones.begin(),
                       scopedSnapshot.zones.end(),
                       [&](const PlaybackSnapshotZone& zone)
                       {
                           return !containsZoneId(retainedZoneIds, zone.id);
                       }),
        scopedSnapshot.zones.end());
    scopedSnapshot.selectedZoneId = request.identity.selectedZoneId;
    scopedSnapshot.selectedGroupId = selectedGroupScope
        ? request.identity.selectedGroupId
        : firstRetainedSnapshotZone->groupId;
    scopedSnapshot.sampleIdentities.erase(
        std::remove_if(scopedSnapshot.sampleIdentities.begin(), scopedSnapshot.sampleIdentities.end(),
                       [&](const PlaybackSnapshotSampleIdentity& sample)
                       {
                           return std::none_of(scopedSnapshot.zones.begin(),
                                               scopedSnapshot.zones.end(),
                                               [&](const PlaybackSnapshotZone& zone)
                                               {
                                                   return zone.sampleSourceId == sample.sampleSourceId;
                                               });
                       }),
        scopedSnapshot.sampleIdentities.end());
    retainSelectedZones(scopedSnapshot.articulationRoutes, retainedZoneIds);
    retainSelectedZones(scopedSnapshot.groupRoutes, retainedZoneIds);
    scopedSnapshot.routingBuses.erase(
        std::remove_if(scopedSnapshot.routingBuses.begin(), scopedSnapshot.routingBuses.end(),
                       [&](const PlaybackSnapshotRoutingBusReference& bus)
                       {
                           if (bus.inputSourceId.rfind("zones/", 0) == 0)
                               return !containsZoneId(retainedZoneIds, bus.inputSourceId.substr(6));
                           if (bus.inputSourceId.rfind("groups/", 0) == 0)
                               return bus.inputSourceId.substr(7) != scopedSnapshot.selectedGroupId;
                           return false;
                       }),
        scopedSnapshot.routingBuses.end());
    scopedSnapshot.dspGraphDigest = computePlaybackSnapshotDspGraphDigest(scopedSnapshot);
    scopedSnapshot.contentDigest = computePlaybackSnapshotContentDigest(scopedSnapshot);

    auto scopedPrepared = sourcePrepared;
    scopedPrepared.selectedGroupId = scopedSnapshot.selectedGroupId;
    scopedPrepared.samples.clear();
    scopedPrepared.streams.clear();
    scopedPrepared.zones.clear();

    std::vector<std::size_t> retainedPreparedZoneIndices;
    std::unordered_map<std::size_t, std::size_t> sampleIndexMap;
    std::unordered_map<std::size_t, std::size_t> streamIndexMap;
    std::unordered_map<std::size_t, std::size_t> zoneIndexMap;
    std::unordered_map<std::size_t, std::size_t> ownershipIndexMap;
    std::vector<PreparedPlaybackOwnershipRecord> retainedOwnership;
    const auto retainOwnership = [&](std::size_t oldIndex) -> std::size_t
    {
        if (oldIndex >= sourcePrepared.ownershipRecords.size())
            return 0;
        if (const auto existing = ownershipIndexMap.find(oldIndex);
            existing != ownershipIndexMap.end())
        {
            return existing->second;
        }
        ownershipIndexMap.emplace(oldIndex, retainedOwnership.size());
        retainedOwnership.push_back(sourcePrepared.ownershipRecords[oldIndex]);
        return retainedOwnership.size() - 1;
    };

    for (std::size_t sourceZoneIndex = 0; sourceZoneIndex < sourcePrepared.zones.size(); ++sourceZoneIndex)
    {
        const auto& zone = sourcePrepared.zones[sourceZoneIndex];
        if (!containsZoneId(retainedZoneIds, zone.zoneId))
            continue;

        if (zone.preparedSampleIndex >= sourcePrepared.samples.size())
        {
            addError(result, "preview-selected-sample-index-invalid", "payload.prepared.zones",
                     "A retained Preview zone does not reference a retained prepared sample.");
            return result;
        }

        auto selectedPreparedZone = zone;
        if (const auto mappedSample = sampleIndexMap.find(zone.preparedSampleIndex);
            mappedSample != sampleIndexMap.end())
        {
            selectedPreparedZone.preparedSampleIndex = mappedSample->second;
        }
        else
        {
            auto sample = sourcePrepared.samples[zone.preparedSampleIndex];
            sample.ownershipRecordIndex = retainOwnership(sample.ownershipRecordIndex);
            selectedPreparedZone.preparedSampleIndex = scopedPrepared.samples.size();
            sampleIndexMap.emplace(zone.preparedSampleIndex, selectedPreparedZone.preparedSampleIndex);
            scopedPrepared.samples.push_back(std::move(sample));
        }

        if (zone.preparedStreamIndex < sourcePrepared.streams.size())
        {
            if (const auto mappedStream = streamIndexMap.find(zone.preparedStreamIndex);
                mappedStream != streamIndexMap.end())
            {
                selectedPreparedZone.preparedStreamIndex = mappedStream->second;
            }
            else
            {
                auto stream = sourcePrepared.streams[zone.preparedStreamIndex];
                stream.ownershipRecordIndex = retainOwnership(stream.ownershipRecordIndex);
                selectedPreparedZone.preparedStreamIndex = scopedPrepared.streams.size();
                streamIndexMap.emplace(zone.preparedStreamIndex, selectedPreparedZone.preparedStreamIndex);
                scopedPrepared.streams.push_back(std::move(stream));
            }
        }
        else
        {
            selectedPreparedZone.preparedStreamIndex = 0;
        }

        zoneIndexMap.emplace(sourceZoneIndex, scopedPrepared.zones.size());
        retainedPreparedZoneIndices.push_back(sourceZoneIndex);
        scopedPrepared.zones.push_back(std::move(selectedPreparedZone));
    }
    scopedPrepared.ownershipRecords = std::move(retainedOwnership);
    scopedPrepared.performanceProgram = remapPerformanceProgramForScopedZones(
        sourcePrepared.performanceProgram,
        retainedPreparedZoneIndices,
        zoneIndexMap,
        scopedArticulationId);
    scopedPrepared.snapshotContentDigest = scopedSnapshot.contentDigest;
    scopedPrepared.preparedContentDigest = computePreparedPlaybackContentDigest(scopedPrepared);

    auto scopedPayload = std::make_shared<PlaybackActivationPayload>(*preparedDraftPayload);
    scopedPayload->snapshotContentDigest = scopedSnapshot.contentDigest;
    scopedPayload->preparedContentDigest = scopedPrepared.preparedContentDigest;
    scopedPayload->retainedPreparedBytes = std::accumulate(
        scopedPrepared.ownershipRecords.begin(), scopedPrepared.ownershipRecords.end(),
        std::uint64_t { 0 },
        [](std::uint64_t total, const PreparedPlaybackOwnershipRecord& ownership)
        {
            return total + ownership.retainedBytes;
        });
    scopedPayload->snapshot
        = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(scopedSnapshot));
    scopedPayload->prepared
        = std::make_shared<const ImmutablePreparedPlayback>(std::move(scopedPrepared));

    SamplerRenderModelBuildOptions options;
    if (selectedZoneScope)
    {
        options.selectedZoneId = request.identity.selectedZoneId;
        options.retainedZoneIds = retainedZoneIds;
        options.auditionSelectedZone = true;
    }
    else if (selectedGroupScope)
    {
        options.selectedGroupId = request.identity.selectedGroupId;
    }
    auto scopedModel = buildSamplerRenderModel(scopedPayload, options);
    if (!scopedModel.built || scopedModel.model == nullptr)
    {
        result.findings = std::move(scopedModel.findings);
        return result;
    }

    result.prepared = true;
    result.scopedPayload = std::move(scopedPayload);
    result.model = std::move(scopedModel.model);
    result.retainedZoneCount = result.model->getRoutes().size();
    result.retainedSampleCount = result.model->getSamples().size();
    return result;
}
} // namespace drs::engine
