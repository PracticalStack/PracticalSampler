#include "drs/engine/SamplerPlaybackContext.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace
{
constexpr float tolerance = 1.0e-6f;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(float actual, float expected, const std::string& message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message + " (actual=" + std::to_string(actual)
                                 + ", expected=" + std::to_string(expected) + ")");
}

struct ModelLifetime
{
    drs::engine::SamplerRenderModelPtr model;
    std::weak_ptr<const drs::engine::PlaybackActivationPayload> payload;
};

ModelLifetime buildModel(drs::engine::PlaybackActivationLane lane,
                         std::size_t revision,
                         float sampleValue)
{
    const auto laneName = lane == drs::engine::PlaybackActivationLane::preview
        ? "preview"
        : "performance";
    const auto suffix = laneName + std::string("-") + std::to_string(revision);

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = "context-snapshot-" + suffix;
    drs::engine::PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "context-zone-" + suffix;
    snapshotZone.sampleSourceId = "context-sample-" + suffix;
    snapshotZone.displayName = "Context Zone";
    snapshotZone.groupId = "context-group";
    snapshotZone.articulationId = "sustain";
    snapshotZone.rootKey = 60;
    snapshotZone.keyLow = 0;
    snapshotZone.keyHigh = 127;
    snapshotZone.velocityLow = 1;
    snapshotZone.velocityHigh = 127;
    snapshotZone.loopEnabled = true;
    snapshotZone.loopStartFrame = 0;
    snapshotZone.loopEndFrame = 4;
    snapshot.zones.push_back(std::move(snapshotZone));

    auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { { sampleValue, sampleValue, sampleValue, sampleValue } };
    drs::engine::PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "context-sample-" + suffix;
    sample.streamSampleId = "context-stream-" + suffix;
    sample.sampleRate = 48000.0;
    sample.frameCount = 4;
    sample.channelCount = 1;
    sample.decodedSampleData = std::move(decoded);

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 5000 + revision;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = "context-prepared-" + suffix;
    prepared.samples.push_back(std::move(sample));
    drs::engine::PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "context-zone-" + suffix;
    preparedZone.sampleSourceId = "context-sample-" + suffix;
    preparedZone.streamSampleId = "context-stream-" + suffix;
    preparedZone.preparedSampleIndex = 0;
    preparedZone.preparedStreamIndex = 0;
    preparedZone.rootKey = 60;
    preparedZone.keyLow = 0;
    preparedZone.keyHigh = 127;
    preparedZone.velocityLow = 1;
    preparedZone.velocityHigh = 127;
    preparedZone.loopEnabled = true;
    preparedZone.loopStartFrame = 0;
    preparedZone.loopEndFrame = 4;
    prepared.zones.push_back(std::move(preparedZone));

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = lane;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 6000 + revision;
    payload->lifecycleState = lane == drs::engine::PlaybackActivationLane::preview
        ? drs::engine::PlaybackSnapshotLifecycleState::ready
        : drs::engine::PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));

    ModelLifetime result;
    result.payload = payload;
    const auto build = drs::engine::buildSamplerRenderModel(payload);
    require(build.built && build.model != nullptr, "Playback-context fixture model should validate.");
    result.model = build.model;
    return result;
}

ModelLifetime buildRoundRobinModel(drs::engine::PlaybackActivationLane lane,
                                   std::size_t revision,
                                   float slotOneValue,
                                   float slotTwoValue)
{
    const auto laneName = lane == drs::engine::PlaybackActivationLane::preview
        ? "preview"
        : "performance";
    const auto suffix = laneName + std::string("-rr-") + std::to_string(revision);

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = "context-round-robin-snapshot-" + suffix;

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 7000 + revision;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = "context-round-robin-prepared-" + suffix;

    const std::array slotValues { slotOneValue, slotTwoValue };
    for (std::size_t index = 0; index < slotValues.size(); ++index)
    {
        const auto slotNumber = static_cast<int>(index + 1);
        const auto slotSuffix = suffix + "-slot-" + std::to_string(slotNumber);

        drs::engine::PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = "context-rr-zone-" + slotSuffix;
        snapshotZone.sampleSourceId = "context-rr-sample-" + slotSuffix;
        snapshotZone.displayName = "Context RR Zone";
        snapshotZone.groupId = "context-group";
        snapshotZone.articulationId = "sustain";
        snapshotZone.rootKey = 60;
        snapshotZone.keyLow = 60;
        snapshotZone.keyHigh = 60;
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        snapshotZone.roundRobin = drs::engine::RoundRobinDescriptor {
            std::string("context-rr-pool-") + laneName,
            2,
            slotNumber,
            drs::engine::RoundRobinMode::sequential
        };
        snapshotZone.roundRobinLength = 2;
        snapshotZone.roundRobinPosition = slotNumber;
        snapshot.zones.push_back(std::move(snapshotZone));

        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { { slotValues[index] } };
        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = "context-rr-sample-" + slotSuffix;
        sample.streamSampleId = "context-rr-stream-" + slotSuffix;
        sample.sampleRate = 48000.0;
        sample.frameCount = 1;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        drs::engine::PreparedPlaybackZoneHandle preparedZone;
        preparedZone.zoneId = "context-rr-zone-" + slotSuffix;
        preparedZone.sampleSourceId = "context-rr-sample-" + slotSuffix;
        preparedZone.streamSampleId = "context-rr-stream-" + slotSuffix;
        preparedZone.preparedSampleIndex = index;
        preparedZone.preparedStreamIndex = index;
        preparedZone.rootKey = 60;
        preparedZone.keyLow = 60;
        preparedZone.keyHigh = 60;
        preparedZone.velocityLow = 1;
        preparedZone.velocityHigh = 127;
        preparedZone.roundRobin = drs::engine::RoundRobinDescriptor {
            std::string("context-rr-pool-") + laneName,
            2,
            slotNumber,
            drs::engine::RoundRobinMode::sequential
        };
        preparedZone.roundRobinLength = 2;
        preparedZone.roundRobinPosition = slotNumber;
        prepared.zones.push_back(std::move(preparedZone));
    }

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = lane;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 8000 + revision;
    payload->lifecycleState = lane == drs::engine::PlaybackActivationLane::preview
        ? drs::engine::PlaybackSnapshotLifecycleState::ready
        : drs::engine::PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));

    ModelLifetime result;
    result.payload = payload;
    const auto build = drs::engine::buildSamplerRenderModel(payload);
    require(build.built && build.model != nullptr, "Playback-context RR fixture model should validate.");
    result.model = build.model;
    return result;
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> pointers;

    explicit StereoOutput(std::size_t frames)
        : left(frames, 0.0f), right(frames, 0.0f), pointers { left.data(), right.data() }
    {
    }

    drs::engine::SamplerAudioBufferView view()
    {
        return { pointers.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

drs::engine::SamplerRenderEvent noteOn(std::uint32_t offset, int note)
{
    return { drs::engine::SamplerRenderEventType::noteOn,
             offset,
             static_cast<std::uint8_t>(note),
             1.0f };
}

drs::engine::SamplerRenderEvent noteOff(std::uint32_t offset, int note)
{
    return { drs::engine::SamplerRenderEventType::noteOff,
             offset,
             static_cast<std::uint8_t>(note),
             0.0f };
}

template <std::size_t Size>
drs::engine::SamplerRenderEventView eventView(const std::array<drs::engine::SamplerRenderEvent, Size>& events)
{
    return { events.data(), events.size() };
}

drs::engine::SamplerRenderEventView noEvents()
{
    return { nullptr, 0 };
}

float renderSingleFrameNote(drs::engine::SamplerPlaybackContext& context, int note)
{
    const std::array events { noteOn(0, note) };
    StereoOutput output(1);
    const auto result = context.renderBlock(output.view(), eventView(events));
    require(result.accepted, "Single-frame context render should be accepted.");
    return output.left.front();
}

void runIndependentContextMatrix()
{
    drs::engine::SamplerPlaybackContext preview(drs::engine::PlaybackActivationLane::preview);
    drs::engine::SamplerPlaybackContext performance(drs::engine::PlaybackActivationLane::performance);
    require(preview.prepare(48000.0) && performance.prepare(48000.0),
            "Both playback contexts should prepare independently.");

    auto previewModel = buildModel(drs::engine::PlaybackActivationLane::preview, 1, 1.0f);
    auto performanceModel = buildModel(drs::engine::PlaybackActivationLane::performance, 10, 2.0f);
    require(preview.stageActivation(previewModel.model)
                && performance.stageActivation(performanceModel.model),
            "Preview and Performance should accept their own immutable models.");
    require(!preview.stageActivation(performanceModel.model)
                && !performance.stageActivation(previewModel.model),
            "A context must reject an activation from the other lane.");

    const std::array events { noteOn(0, 60) };
    StereoOutput previewOutput(8);
    StereoOutput performanceOutput(8);
    const auto previewResult = preview.renderBlock(previewOutput.view(), eventView(events));
    const auto performanceResult = performance.renderBlock(performanceOutput.view(), eventView(events));
    require(previewResult.accepted && performanceResult.accepted
                && previewResult.activationApplied && performanceResult.activationApplied,
            "Both contexts should apply and render their activations at their block boundary.");
    requireNear(previewOutput.left.front(), 0.25f,
                "Preview must render only Preview PCM and gain state.");
    requireNear(performanceOutput.left.front(), 0.5f,
                "Performance must render only Performance PCM and gain state.");

    preview.resetAtBlockBoundary();
    StereoOutput previewAfterReset(4);
    StereoOutput performanceAfterReset(4);
    require(preview.renderBlock(previewAfterReset.view(), noEvents()).accepted
                && performance.renderBlock(performanceAfterReset.view(), noEvents()).accepted,
            "Both contexts should remain renderable after Preview reset.");
    requireNear(previewAfterReset.left.front(), 0.0f,
                "Preview reset must silence only Preview voices.");
    requireNear(performanceAfterReset.left.front(), 0.5f,
                "Preview reset must not alter Performance note ownership.");

    const auto previewSnapshot = preview.getSnapshot();
    const auto performanceSnapshot = performance.getSnapshot();
    require(previewSnapshot.activeVoiceCount == 0
                && performanceSnapshot.activeVoiceCount == 1
                && previewSnapshot.counters.startedVoiceCount == 1
                && performanceSnapshot.counters.startedVoiceCount == 1,
            "Voice pools, note ownership, and counters must be context-local.");
}

void runConcurrentRenderMatrix()
{
    drs::engine::SamplerPlaybackContext preview(drs::engine::PlaybackActivationLane::preview);
    drs::engine::SamplerPlaybackContext performance(drs::engine::PlaybackActivationLane::performance);
    auto previewModel = buildModel(drs::engine::PlaybackActivationLane::preview, 11, 1.0f);
    auto performanceModel = buildModel(drs::engine::PlaybackActivationLane::performance, 12, 2.0f);
    require(preview.prepare(48000.0) && performance.prepare(48000.0)
                && preview.stageActivation(previewModel.model)
                && performance.stageActivation(performanceModel.model),
            "Concurrent contexts should prepare and stage independently.");

    const std::array events { noteOn(0, 60) };
    StereoOutput previewOutput(256);
    StereoOutput performanceOutput(256);
    bool previewAccepted = false;
    bool performanceAccepted = false;
    std::thread previewThread([&]
    {
        previewAccepted = preview.renderBlock(previewOutput.view(), eventView(events)).accepted;
    });
    std::thread performanceThread([&]
    {
        performanceAccepted = performance.renderBlock(performanceOutput.view(), eventView(events)).accepted;
    });
    previewThread.join();
    performanceThread.join();

    require(previewAccepted && performanceAccepted,
            "Preview and Performance should render concurrently through the shared core.");
    for (std::size_t frame = 0; frame < previewOutput.left.size(); ++frame)
    {
        requireNear(previewOutput.left[frame], 0.25f,
                    "Concurrent Preview output leaked Performance state.");
        requireNear(performanceOutput.left[frame], 0.5f,
                    "Concurrent Performance output leaked Preview state.");
    }
}

void runBlockBoundaryAndLifetimeMatrix()
{
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "Lifetime context should prepare.");

    auto oldLifetime = buildModel(drs::engine::PlaybackActivationLane::preview, 20, 1.0f);
    const auto oldModelUseCount = oldLifetime.model.use_count();
    require(context.stageActivation(oldLifetime.model), "Old activation should stage.");
    require(context.getActiveRenderModel() == nullptr
                && context.getSnapshot().hasPendingActivation,
            "Staging must not expose an activation before a block boundary.");

    const std::array oldNote { noteOn(0, 60) };
    StereoOutput firstBlock(4);
    require(context.renderBlock(firstBlock.view(), eventView(oldNote)).activationApplied,
            "First render boundary should activate the staged model.");
    require(oldLifetime.model.use_count() == oldModelUseCount + 1,
            "Audio activation must not copy the slot's shared model ownership.");

    auto newLifetime = buildModel(drs::engine::PlaybackActivationLane::preview, 21, 2.0f);
    require(context.stageActivation(newLifetime.model), "Replacement activation should stage.");
    require(context.getSnapshot().activeRevision == 20,
            "The active revision must remain stable until the next block.");

    const std::array newNote { noteOn(0, 61) };
    StereoOutput replacementBlock(4);
    const auto replacement = context.renderBlock(replacementBlock.view(), eventView(newNote));
    require(replacement.accepted && replacement.activationApplied
                && context.getSnapshot().activeRevision == 21,
            "Replacement must become visible exactly at the following block boundary.");
    requireNear(replacementBlock.left.front(), 0.75f,
                "Old and new activation voices must render together without model aliasing.");

    oldLifetime.model.reset();
    require(!oldLifetime.payload.expired() && context.serviceRetirements() == 0,
            "A retired activation must stay leased while its old voice is live.");

    const std::array releaseOld { noteOff(0, 60) };
    StereoOutput releaseBlock(drs::engine::SamplerVoice::compatibilityReleaseSampleCount);
    const auto release = context.renderBlock(releaseBlock.view(), eventView(releaseOld));
    require(release.accepted && release.voicePool.render.releasedVoiceCount == 1
                && context.getSnapshot().activeVoiceCount == 1,
            "Only the old activation's matching note should finish its release.");
    require(!oldLifetime.payload.expired(),
            "Audio completion must return only a retirement token, not release the payload.");
    require(context.serviceRetirements() == 1 && oldLifetime.payload.expired(),
            "Message-owned retirement drain must perform the final old payload release.");
    require(context.getSnapshot().counters.reclaimedActivationCount == 1,
            "Retirement diagnostics should count message-owned reclamation.");
}

void runRestartCloseAndDrainMatrix()
{
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::performance);
    require(context.prepare(48000.0), "Restart context should prepare.");
    auto lifetime = buildModel(drs::engine::PlaybackActivationLane::performance, 30, 3.0f);
    require(context.stageActivation(lifetime.model), "Performance activation should stage.");
    const std::array start { noteOn(0, 64) };
    StereoOutput startBlock(2);
    require(context.renderBlock(startBlock.view(), eventView(start)).accepted,
            "Performance voice should start before restart.");

    require(context.prepare(44100.0), "Device restart should accept the new sample rate.");
    auto snapshot = context.getSnapshot();
    require(snapshot.hasActiveActivation && snapshot.activeRevision == 30
                && snapshot.activeVoiceCount == 0,
            "Device restart must stop voices while preserving the active activation lease.");
    StereoOutput restartedBlock(2);
    require(context.renderBlock(restartedBlock.view(), eventView(start)).accepted
                && context.getSnapshot().activeVoiceCount == 1,
            "The preserved activation should start fresh voices after restart.");

    lifetime.model.reset();
    context.closeAtBlockBoundary();
    snapshot = context.getSnapshot();
    require(!snapshot.hasActiveActivation && !snapshot.hasPendingActivation
                && snapshot.activeVoiceCount == 0
                && !lifetime.payload.expired(),
            "Close must detach audio state while deferring final payload reclamation.");
    require(context.serviceRetirements() == 1 && lifetime.payload.expired(),
            "Close retirement should drain exactly once on the message-owned path.");
    require(context.serviceRetirements() == 0,
            "Repeated retirement drain must be idempotent.");
}

void runRoundRobinLaneAndGenerationMatrix()
{
    drs::engine::SamplerPlaybackContext preview(drs::engine::PlaybackActivationLane::preview);
    drs::engine::SamplerPlaybackContext performance(drs::engine::PlaybackActivationLane::performance);
    require(preview.prepare(48000.0) && performance.prepare(48000.0),
            "RR playback contexts should prepare.");

    auto previewModel = buildRoundRobinModel(drs::engine::PlaybackActivationLane::preview, 40, 1.0f, 2.0f);
    auto performanceModel = buildRoundRobinModel(drs::engine::PlaybackActivationLane::performance, 41, 1.0f, 2.0f);
    require(preview.stageActivation(previewModel.model)
                && performance.stageActivation(performanceModel.model),
            "RR activations should stage for both lanes.");

    requireNear(renderSingleFrameNote(preview, 60), 0.25f,
                "Preview RR should start on slot 1.");
    requireNear(renderSingleFrameNote(preview, 60), 0.5f,
                "Preview RR should advance to slot 2.");
    requireNear(renderSingleFrameNote(performance, 60), 0.25f,
                "Performance RR must keep its own slot counter.");
    auto previewSnapshot = preview.getSnapshot();
    auto performanceSnapshot = performance.getSnapshot();
    require(previewSnapshot.counters.roundRobinPoolHitCount == 2
                && previewSnapshot.counters.roundRobinPoolMissCount == 0
                && previewSnapshot.counters.roundRobinFallbackCount == 0
                && performanceSnapshot.counters.roundRobinPoolHitCount == 1,
            "RR counters must remain lane-local and visible in playback diagnostics.");

    auto replacementModel = buildRoundRobinModel(drs::engine::PlaybackActivationLane::preview, 42, 1.0f, 2.0f);
    require(preview.stageActivation(replacementModel.model),
            "Replacement preview RR activation should stage.");
    requireNear(renderSingleFrameNote(preview, 60), 0.25f,
                "A new activation generation should reset the RR slot sequence.");
    previewSnapshot = preview.getSnapshot();
    require(previewSnapshot.counters.roundRobinPoolHitCount == 3
                && previewSnapshot.counters.roundRobinPoolMissCount == 0
                && previewSnapshot.counters.roundRobinFallbackCount == 0,
            "RR counters should continue across generations without introducing false misses.");
}
} // namespace

int main()
{
    try
    {
        runIndependentContextMatrix();
        runConcurrentRenderMatrix();
        runBlockBoundaryAndLifetimeMatrix();
        runRestartCloseAndDrainMatrix();
        runRoundRobinLaneAndGenerationMatrix();
        std::cout << "Sprint 4.5 playback-context isolation and activation-lifetime matrix passed."
                  << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
