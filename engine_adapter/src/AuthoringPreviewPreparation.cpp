#include "drs/engine/AuthoringPreviewPreparation.h"

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"

#include <algorithm>
#include <numeric>
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

template <typename Route>
void retainSelectedZone(std::vector<Route>& routes, const std::string& selectedZoneId)
{
    for (auto iterator = routes.begin(); iterator != routes.end();)
    {
        iterator->zoneIds.erase(std::remove_if(iterator->zoneIds.begin(), iterator->zoneIds.end(),
                                              [&](const std::string& zoneId)
                                              {
                                                  return zoneId != selectedZoneId;
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

    if (request.identity.selectedZoneId.empty())
    {
        addError(result, "preview-selected-zone-missing", "request.selectedZoneId",
                 "Selected-zone Preview requires an explicit selected-zone identity.");
        return result;
    }

    const auto& sourceSnapshot = *preparedDraftPayload->snapshot;
    const auto& sourcePrepared = *preparedDraftPayload->prepared;
    const auto snapshotZone = std::find_if(sourceSnapshot.zones.begin(), sourceSnapshot.zones.end(),
                                           [&](const PlaybackSnapshotZone& zone)
                                           {
                                               return zone.id == request.identity.selectedZoneId;
                                           });
    const auto preparedZone = std::find_if(sourcePrepared.zones.begin(), sourcePrepared.zones.end(),
                                           [&](const PreparedPlaybackZoneHandle& zone)
                                           {
                                               return zone.zoneId == request.identity.selectedZoneId;
                                           });
    if (snapshotZone == sourceSnapshot.zones.end() || preparedZone == sourcePrepared.zones.end())
    {
        addError(result, "preview-selected-zone-unprepared", "request.selectedZoneId",
                 "The selected zone is not present in the validated prepared draft.");
        return result;
    }

    const auto oldSampleIndex = preparedZone->preparedSampleIndex;
    const auto oldStreamIndex = preparedZone->preparedStreamIndex;
    if (oldSampleIndex >= sourcePrepared.samples.size())
    {
        addError(result, "preview-selected-sample-index-invalid", "payload.prepared.zones",
                 "The selected zone does not reference a retained prepared sample.");
        return result;
    }

    auto scopedSnapshot = sourceSnapshot;
    scopedSnapshot.zones = { *snapshotZone };
    scopedSnapshot.selectedZoneId = request.identity.selectedZoneId;
    scopedSnapshot.sampleIdentities.erase(
        std::remove_if(scopedSnapshot.sampleIdentities.begin(), scopedSnapshot.sampleIdentities.end(),
                       [&](const PlaybackSnapshotSampleIdentity& sample)
                       {
                           return sample.sampleSourceId != snapshotZone->sampleSourceId;
                       }),
        scopedSnapshot.sampleIdentities.end());
    retainSelectedZone(scopedSnapshot.articulationRoutes, request.identity.selectedZoneId);
    retainSelectedZone(scopedSnapshot.groupRoutes, request.identity.selectedZoneId);
    scopedSnapshot.contentDigest = computePlaybackSnapshotContentDigest(scopedSnapshot);

    auto scopedPrepared = sourcePrepared;
    const auto selectedSample = sourcePrepared.samples[oldSampleIndex];
    scopedPrepared.samples = { selectedSample };
    scopedPrepared.streams.clear();
    if (oldStreamIndex < sourcePrepared.streams.size())
        scopedPrepared.streams.push_back(sourcePrepared.streams[oldStreamIndex]);

    std::vector<PreparedPlaybackOwnershipRecord> selectedOwnership;
    std::unordered_map<std::size_t, std::size_t> ownershipIndexMap;
    const auto retainOwnership = [&](std::size_t oldIndex)
    {
        if (oldIndex >= sourcePrepared.ownershipRecords.size()
            || ownershipIndexMap.find(oldIndex) != ownershipIndexMap.end())
            return;
        ownershipIndexMap.emplace(oldIndex, selectedOwnership.size());
        selectedOwnership.push_back(sourcePrepared.ownershipRecords[oldIndex]);
    };
    retainOwnership(selectedSample.ownershipRecordIndex);
    if (!scopedPrepared.streams.empty())
        retainOwnership(scopedPrepared.streams.front().ownershipRecordIndex);
    scopedPrepared.ownershipRecords = std::move(selectedOwnership);
    if (const auto mapped = ownershipIndexMap.find(scopedPrepared.samples.front().ownershipRecordIndex);
        mapped != ownershipIndexMap.end())
        scopedPrepared.samples.front().ownershipRecordIndex = mapped->second;
    if (!scopedPrepared.streams.empty())
    {
        if (const auto mapped = ownershipIndexMap.find(scopedPrepared.streams.front().ownershipRecordIndex);
            mapped != ownershipIndexMap.end())
            scopedPrepared.streams.front().ownershipRecordIndex = mapped->second;
    }

    auto selectedPreparedZone = *preparedZone;
    selectedPreparedZone.preparedSampleIndex = 0;
    selectedPreparedZone.preparedStreamIndex = 0;
    scopedPrepared.zones = { std::move(selectedPreparedZone) };
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
    options.selectedZoneId = request.identity.selectedZoneId;
    options.auditionSelectedZone = true;
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
