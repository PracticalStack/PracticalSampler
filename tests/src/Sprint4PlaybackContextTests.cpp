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

    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "context-group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { snapshot.zones.front().id };
    snapshotGroup.displayName = "Context Group";
    snapshot.groupRoutes.push_back(snapshotGroup);
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.zoneIds = { prepared.zones.front().zoneId };
    preparedGroup.displayName = snapshotGroup.displayName;
    prepared.groupRoutes.push_back(std::move(preparedGroup));

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

    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "context-group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.displayName = "Context Group";
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.displayName = snapshotGroup.displayName;
    for (const auto& zone : snapshot.zones)
        snapshotGroup.zoneIds.push_back(zone.id);
    for (const auto& zone : prepared.zones)
        preparedGroup.zoneIds.push_back(zone.zoneId);
    snapshot.groupRoutes.push_back(std::move(snapshotGroup));
    prepared.groupRoutes.push_back(std::move(preparedGroup));

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

void runDspGenerationActivationMatrix()
{
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "DSP-generation context should prepare.");
    auto lifetime = buildModel(drs::engine::PlaybackActivationLane::preview, 99, 1.0f);
    drs::engine::ImmutableDspGraphPlan plan;
    plan.planDigest = "fnv1a64:context-dsp-generation";
    plan.directFastPath = false;
    const auto zoneId = lifetime.model->getRoutes().front().zoneId;
    plan.parameters = {
        { "gainDb", 6.0 }, { "polarity", 0.0 }, { "mute", 0.0 },
        { "gainDb", -6.0 }, { "polarity", 0.0 }, { "mute", 0.0 },
        { "gainDb", 6.0 }, { "polarity", 0.0 }, { "mute", 0.0 }
    };
    plan.nodes = {
        { drs::engine::DspGraphOwnerKind::zone, zoneId, "zones/" + zoneId, "groups/context-group",
          "zone-gain", "drs.gain", 1, 0, 3 },
        { drs::engine::DspGraphOwnerKind::group, "context-group", "groups/context-group", "master",
          "group-gain", "drs.gain", 1, 3, 3 },
        { drs::engine::DspGraphOwnerKind::master, "master", "master", "output",
          "instrument-gain", "drs.gain", 1, 6, 3 }
    };
    plan.scratchBytes = 8;
    plan.stateBytes = 16;
    plan.delayMemoryBytes = 4;
    std::string failure;
    auto generation = drs::engine::createDspRenderGeneration(lifetime.model, plan, 512, &failure);
    require(generation != nullptr && failure.empty(),
            "DSP generation must preallocate graph state and scratch off audio.");
    require(generation->getRouteDestinationNodeIndex(0) == 0,
            "Each immutable sampler route must receive its numeric master graph destination off audio.");
    require(context.stageActivation(lifetime.model, generation),
            "Activation slots must retain a DSP generation that matches their immutable sampler model.");
    StereoOutput output(4);
    const std::array gainNotes { noteOn(0, 60), noteOn(0, 60) };
    require(context.renderBlock(output.view(), eventView(gainNotes)).activationApplied
                && context.getActiveDspGeneration() == generation.get(),
            "The audio block boundary must exchange only the prepared slot's generation pointer.");
    requireNear(output.left.front(), 0.9976312f,
                "Overlapping voices must traverse zone, group, and master Gain once at each aggregation point.");
    require(!context.publishDspControl(generation->getControlGenerationIdentity() + 1, 6, 0.0)
                && context.publishDspControl(generation->getControlGenerationIdentity(), 6, 0.0),
            "A stale generation must not publish a control value, while the active numeric control may.");
    context.resetAtBlockBoundary();
    StereoOutput updatedOutput(4);
    require(context.renderBlock(updatedOutput.view(), eventView(gainNotes)).accepted,
            "A live Gain control update must render without rebuilding the active graph.");
    require(updatedOutput.left[1] < 0.9976312f && updatedOutput.left[1] > 0.5f,
            "A linear Gain control update must begin a click-free ramp on the next callback block.");
    for (std::size_t block = 1; block < 120; ++block)
    {
        StereoOutput smoothingOutput(4);
        require(context.renderBlock(smoothingOutput.view(), noEvents()).accepted,
                "A smoothing ramp must remain renderable across callback partitions.");
        if (block == 119)
            requireNear(smoothingOutput.left.back(), 0.5f,
                        "A numeric master Gain control update must reach its target after 10 ms.");
    }
    require(!context.publishDspNodeBypass(generation->getControlGenerationIdentity() + 1, 1, true)
                && context.publishDspNodeBypass(generation->getControlGenerationIdentity(), 1, true),
            "A stale generation must not bypass a node, while the active node may crossfade to bypass.");
    StereoOutput bypassTransition(4);
    require(context.renderBlock(bypassTransition.view(), noEvents()).accepted
                && bypassTransition.left[1] > 0.5f && bypassTransition.left[1] < 0.9976312f,
            "A node bypass must start as a click-free wet/dry transition rather than a hard switch.");
    for (std::size_t block = 1; block < 60; ++block)
    {
        StereoOutput bypassOutput(4);
        require(context.renderBlock(bypassOutput.view(), noEvents()).accepted,
                "A bypass transition must remain renderable across callback partitions.");
        if (block == 59)
            requireNear(bypassOutput.left.back(), 0.9976312f,
                        "A bypassed group Gain must become transparent after its 5 ms crossfade.");
    }
    const auto snapshot = context.getSnapshot();
    require(snapshot.activeDspScratchBytes == 12296 && snapshot.activeDspStateBytes == 16
                && snapshot.activeDspDelayMemoryBytes == 4,
            "Realtime diagnostics must expose primitive DSP generation resource totals.");

    drs::engine::SamplerPlaybackContext performance(drs::engine::PlaybackActivationLane::performance);
    auto performanceLifetime = buildModel(drs::engine::PlaybackActivationLane::performance, 101, 1.0f);
    auto performancePlan = plan;
    const auto performanceZoneId = performanceLifetime.model->getRoutes().front().zoneId;
    performancePlan.nodes.front().ownerId = performanceZoneId;
    performancePlan.nodes.front().inputSourceId = "zones/" + performanceZoneId;
    auto performanceGeneration = drs::engine::createDspRenderGeneration(
        performanceLifetime.model, performancePlan, 512, &failure);
    require(performance.prepare(48000.0) && performanceGeneration != nullptr
                && performance.stageActivation(performanceLifetime.model, performanceGeneration),
            "Performance must own a distinct prepared Gain generation.");
    StereoOutput performanceOutput(4);
    require(performance.renderBlock(performanceOutput.view(), eventView(gainNotes)).accepted,
            "Performance must render the same instrument Gain core.");
    requireNear(performanceOutput.left.front(), 0.9976312f,
                "Preview and Performance must produce the same Gain result with separate state.");
    require(performance.getActiveDspGeneration() != generation.get(),
            "Preview and Performance must never share mutable Gain-generation state.");

    context.resetAtBlockBoundary();
    generation->setTailFramesRemaining(1);
    auto replacement = buildModel(drs::engine::PlaybackActivationLane::preview, 100, 1.0f);
    require(context.stageActivation(replacement.model),
            "A replacement model should stage while the prior generation retains a tail.");
    require(context.renderBlock(output.view(), noEvents()).activationApplied,
            "The replacement should activate at a later block boundary.");
    require(context.serviceRetirements() >= 1,
            "A retired tail must render and drain on audio before its generation becomes reclaimable off audio.");
    const auto tailReclaimedSnapshot = context.getSnapshot();
    require(tailReclaimedSnapshot.retiredActivationBacklog == 0
                && tailReclaimedSnapshot.retiredActivationPayloadBytes == 0,
            "Tail reclamation must return retained activation ownership accounting to baseline.");

    drs::engine::SamplerPlaybackContext pressure(drs::engine::PlaybackActivationLane::preview);
    require(pressure.prepare(48000.0), "Tail-pressure context should prepare.");
    std::vector<ModelLifetime> pressureLifetimes;
    std::vector<std::shared_ptr<drs::engine::DspRenderGeneration>> pressureGenerations;
    for (std::size_t index = 0; index < drs::engine::SamplerPlaybackContext::activationSlotCapacity; ++index)
    {
        auto lifetime = buildModel(drs::engine::PlaybackActivationLane::preview, 200 + index, 1.0f);
        auto pressurePlan = plan;
        const auto pressureZoneId = lifetime.model->getRoutes().front().zoneId;
        pressurePlan.nodes.front().ownerId = pressureZoneId;
        pressurePlan.nodes.front().inputSourceId = "zones/" + pressureZoneId;
        auto pressureGeneration = drs::engine::createDspRenderGeneration(lifetime.model, pressurePlan, 512, &failure);
        require(pressureGeneration != nullptr && pressure.stageActivation(lifetime.model, pressureGeneration),
                "Each pre-pressure activation must stage its matching DSP generation.");
        if (!pressureGenerations.empty()) pressureGenerations.back()->setTailFramesRemaining(48000);
        StereoOutput pressureOutput(512);
        require(pressure.renderBlock(pressureOutput.view(), noEvents()).accepted,
                "Each pre-pressure activation must cross the audio boundary.");
        pressureLifetimes.push_back(std::move(lifetime));
        pressureGenerations.push_back(std::move(pressureGeneration));
    }
    auto pressuredLifetime = buildModel(drs::engine::PlaybackActivationLane::preview, 300, 1.0f);
    auto pressuredPlan = plan;
    const auto pressuredZoneId = pressuredLifetime.model->getRoutes().front().zoneId;
    pressuredPlan.nodes.front().ownerId = pressuredZoneId;
    pressuredPlan.nodes.front().inputSourceId = "zones/" + pressuredZoneId;
    auto pressuredGeneration = drs::engine::createDspRenderGeneration(
        pressuredLifetime.model, pressuredPlan, 512, &failure);
    require(pressuredGeneration != nullptr
                && !pressure.stageActivation(pressuredLifetime.model, pressuredGeneration),
            "An exhausted activation pool must request a bounded fade before accepting another generation.");
    StereoOutput pressureFadeOutput(512);
    require(pressure.renderBlock(pressureFadeOutput.view(), noEvents()).accepted
                && pressure.serviceRetirements() >= 1
                && pressure.stageActivation(pressuredLifetime.model, pressuredGeneration),
            "The oldest retired tail must fade and reclaim a slot for the deferred activation.");
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
        runDspGenerationActivationMatrix();
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
