#include "drs/engine/SamplerRenderModel.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace drs::engine
{
namespace
{
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

const PlaybackSnapshotZone* findSnapshotZone(const ImmutablePlaybackSnapshot& snapshot,
                                             const std::string& zoneId)
{
    const auto iterator = std::find_if(snapshot.zones.begin(),
                                       snapshot.zones.end(),
                                       [&](const PlaybackSnapshotZone& zone)
                                       {
                                           return zone.id == zoneId;
                                       });
    return iterator == snapshot.zones.end() ? nullptr : &(*iterator);
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
        && snapshotZone.gainDb == preparedZone.gainDb
        && snapshotZone.pan == preparedZone.pan
        && snapshotZone.sampleStartFrame == preparedZone.sampleStartFrame
        && snapshotZone.loopEnabled == preparedZone.loopEnabled
        && snapshotZone.loopStartFrame == preparedZone.loopStartFrame
        && snapshotZone.loopEndFrame == preparedZone.loopEndFrame
        && snapshotZone.triggerMode == preparedZone.triggerMode;
}

bool hasDuplicateSnapshotZoneId(const ImmutablePlaybackSnapshot& snapshot,
                                std::size_t index)
{
    for (std::size_t other = 0; other < index; ++other)
        if (snapshot.zones[other].id == snapshot.zones[index].id)
            return true;
    return false;
}

bool hasDuplicatePreparedZoneId(const ImmutablePreparedPlayback& prepared,
                                std::size_t index)
{
    for (std::size_t other = 0; other < index; ++other)
        if (prepared.zones[other].zoneId == prepared.zones[index].zoneId)
            return true;
    return false;
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

    for (std::size_t index = 0; index < snapshot.zones.size(); ++index)
    {
        const auto& zone = snapshot.zones[index];
        if (zone.id.empty())
            addError(result, "render-model-snapshot-zone-id-missing",
                     "payload.snapshot.zones[" + std::to_string(index) + "].id",
                     "Every snapshot route requires a stable zone identity.");
        else if (hasDuplicateSnapshotZoneId(snapshot, index))
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
        if (sample.decodedSampleData == nullptr)
        {
            addError(result, "render-model-decoded-data-missing", path + ".decodedSampleData",
                     "Prepared decoded PCM must be retained before renderer model construction.");
            continue;
        }

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

    for (std::size_t index = 0; index < prepared.zones.size(); ++index)
    {
        const auto& zone = prepared.zones[index];
        const auto path = "payload.prepared.zones[" + std::to_string(index) + "]";
        if (zone.zoneId.empty())
            addError(result, "render-model-zone-id-missing", path + ".zoneId",
                     "Every prepared route requires a stable zone identity.");
        else if (hasDuplicatePreparedZoneId(prepared, index))
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
        if (zone.sampleStartFrame >= sample.frameCount)
            addError(result, "render-model-start-frame-invalid", path + ".sampleStartFrame",
                     "Zone start frame must address retained decoded PCM.");
        if (zone.loopEnabled
            && (zone.loopStartFrame >= zone.loopEndFrame || zone.loopEndFrame > sample.frameCount))
            addError(result, "render-model-loop-range-invalid", path + ".loopStartFrame",
                     "Enabled loops require an ordered half-open range inside retained PCM.");

        const auto* snapshotZone = findSnapshotZone(snapshot, zone.zoneId);
        if (snapshotZone == nullptr)
            addError(result, "render-model-snapshot-route-missing", path + ".zoneId",
                     "Prepared route has no matching immutable snapshot route.");
        else if (!sameTopology(*snapshotZone, zone))
            addError(result, "render-model-route-topology-mismatch", path,
                     "Snapshot and prepared route topology must agree before rendering.");
    }

    const auto routeSelected = [&](const PreparedPlaybackZoneHandle& zone)
    {
        const auto* snapshotZone = findSnapshotZone(snapshot, zone.zoneId);
        return snapshotZone != nullptr
            && (options.selectedZoneId.empty() || snapshotZone->id == options.selectedZoneId)
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
    model->midiNoteOffset = options.midiNoteOffset;
    model->fixedVelocity = options.fixedVelocity;
    model->retainedActivationPayload = payload;
    model->samples.reserve(prepared.samples.size());
    for (std::size_t index = 0; index < prepared.samples.size(); ++index)
    {
        const auto& sample = prepared.samples[index];
        model->samples.push_back({ index,
                                   sample.sampleSourceId,
                                   sample.streamSampleId,
                                   sample.sampleRate,
                                   sample.frameCount,
                                   sample.channelCount,
                                   sample.decodedSampleData });
    }

    model->routes.reserve(selectedRouteCount);
    for (std::size_t index = 0; index < prepared.zones.size(); ++index)
    {
        const auto& zone = prepared.zones[index];
        if (!routeSelected(zone))
            continue;
        model->routes.push_back({ index,
                                  zone.preparedSampleIndex,
                                  zone.zoneId,
                                  zone.sampleSourceId,
                                  zone.rootKey,
                                  options.auditionSelectedZone ? 0 : zone.keyLow,
                                  options.auditionSelectedZone ? 127 : zone.keyHigh,
                                  options.auditionSelectedZone ? 1 : zone.velocityLow,
                                  options.auditionSelectedZone ? 127 : zone.velocityHigh,
                                  zone.gainDb,
                                  zone.pan,
                                  zone.sampleStartFrame,
                                  zone.loopEnabled,
                                  zone.loopStartFrame,
                                  zone.loopEndFrame,
                                  zone.triggerMode });
    }

    result.built = true;
    result.model = std::move(model);
    return result;
}
} // namespace drs::engine
