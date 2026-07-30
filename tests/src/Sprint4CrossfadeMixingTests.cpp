#include "drs/engine/SamplerPlaybackContext.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
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
    {
        throw std::runtime_error(message + " (actual=" + std::to_string(actual)
                                 + ", expected=" + std::to_string(expected) + ")");
    }
}

struct RouteSpec
{
    std::string zoneId;
    float sampleValue = 1.0f;
    int velocityLow = 1;
    int velocityHigh = 127;
    drs::engine::VelocityCrossfadeDescriptor crossfade;
    drs::engine::VelocityCrossfadeRuntimeDescriptor crossfadeRuntime;
    int roundRobinLength = 0;
    int roundRobinPosition = 0;
};

drs::engine::VelocityCrossfadeRuntimeDescriptor makeFadeOutRuntime(
    int effectiveLow,
    int effectiveHigh,
    const std::string& upperZoneId,
    int overlapLow,
    int overlapHigh)
{
    drs::engine::VelocityCrossfadeRuntimeDescriptor runtime;
    runtime.effectiveLowVelocity = effectiveLow;
    runtime.effectiveHighVelocity = effectiveHigh;
    runtime.fadeOutNeighborZoneId = upperZoneId;
    runtime.fadeOutOverlapLowVelocity = overlapLow;
    runtime.fadeOutOverlapHighVelocity = overlapHigh;
    return runtime;
}

drs::engine::VelocityCrossfadeRuntimeDescriptor makeFadeInRuntime(
    int effectiveLow,
    int effectiveHigh,
    const std::string& lowerZoneId,
    int overlapLow,
    int overlapHigh)
{
    drs::engine::VelocityCrossfadeRuntimeDescriptor runtime;
    runtime.effectiveLowVelocity = effectiveLow;
    runtime.effectiveHighVelocity = effectiveHigh;
    runtime.fadeInNeighborZoneId = lowerZoneId;
    runtime.fadeInOverlapLowVelocity = overlapLow;
    runtime.fadeInOverlapHighVelocity = overlapHigh;
    return runtime;
}

drs::engine::SamplerRenderModelPtr buildModel(const std::vector<RouteSpec>& specs,
                                              std::size_t frameCount = 1)
{
    require(!specs.empty(), "Crossfade mixing fixtures require at least one route.");

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 1;
    snapshot.contentDigest = "sprint4-crossfade-mixing-snapshot";

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 9101;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint4-crossfade-mixing-prepared";

    for (std::size_t index = 0; index < specs.size(); ++index)
    {
        const auto& spec = specs[index];
        const auto sampleId = "crossfade-sample-" + std::to_string(index + 1);
        const auto streamId = "crossfade-stream-" + std::to_string(index + 1);

        drs::engine::PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = spec.zoneId;
        snapshotZone.sampleSourceId = sampleId;
        snapshotZone.displayName = spec.zoneId;
        snapshotZone.groupId = "crossfade-group";
        snapshotZone.articulationId = "sustain";
        snapshotZone.rootKey = 60;
        snapshotZone.keyLow = 60;
        snapshotZone.keyHigh = 60;
        snapshotZone.velocityLow = spec.velocityLow;
        snapshotZone.velocityHigh = spec.velocityHigh;
        snapshotZone.velocityCrossfade = spec.crossfade;
        snapshotZone.velocityCrossfadeRuntime = spec.crossfadeRuntime;
        snapshotZone.roundRobinLength = spec.roundRobinLength;
        snapshotZone.roundRobinPosition = spec.roundRobinPosition;
        snapshot.zones.push_back(std::move(snapshotZone));

        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float>(frameCount, spec.sampleValue) };

        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = streamId;
        sample.sampleRate = 48000.0;
        sample.frameCount = frameCount;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        drs::engine::PreparedPlaybackZoneHandle preparedZone;
        preparedZone.zoneId = spec.zoneId;
        preparedZone.sampleSourceId = sampleId;
        preparedZone.streamSampleId = streamId;
        preparedZone.preparedSampleIndex = index;
        preparedZone.preparedStreamIndex = index;
        preparedZone.rootKey = 60;
        preparedZone.keyLow = 60;
        preparedZone.keyHigh = 60;
        preparedZone.velocityLow = spec.velocityLow;
        preparedZone.velocityHigh = spec.velocityHigh;
        preparedZone.velocityCrossfade = spec.crossfade;
        preparedZone.velocityCrossfadeRuntime = spec.crossfadeRuntime;
        preparedZone.roundRobinLength = spec.roundRobinLength;
        preparedZone.roundRobinPosition = spec.roundRobinPosition;
        prepared.zones.push_back(std::move(preparedZone));
    }

    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "crossfade-group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.displayName = "Crossfade Group";
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.displayName = snapshotGroup.displayName;
    for (const auto& spec : specs)
    {
        snapshotGroup.zoneIds.push_back(spec.zoneId);
        preparedGroup.zoneIds.push_back(spec.zoneId);
    }
    snapshot.groupRoutes.push_back(std::move(snapshotGroup));
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::preview;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 9201;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->retainedPreparedBytes = specs.size() * frameCount * sizeof(float);
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));

    const auto result = drs::engine::buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr,
            "Crossfade mixing fixture model should validate.");
    return result.model;
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

drs::engine::SamplerRenderEvent noteOn(std::uint32_t offset, int velocity)
{
    return { drs::engine::SamplerRenderEventType::noteOn,
             offset,
             60,
             static_cast<float>(velocity) / 127.0f };
}

template <std::size_t Size>
drs::engine::SamplerRenderEventView eventView(
    const std::array<drs::engine::SamplerRenderEvent, Size>& events)
{
    return { events.data(), events.size() };
}

float expectedOutput(int velocity, float sampleValue = 1.0f, float gainMultiplier = 1.0f)
{
    return 0.25f * static_cast<float>(velocity) / 127.0f * sampleValue * gainMultiplier;
}

std::vector<RouteSpec> makeCrossfadePair(bool includeUpperRuntime = true)
{
    RouteSpec lower;
    lower.zoneId = "crossfade-lower";
    lower.velocityLow = 1;
    lower.velocityHigh = 60;
    lower.crossfade.fadeOutLowVelocity = 25;
    lower.crossfade.fadeOutHighVelocity = 60;
    lower.crossfadeRuntime = makeFadeOutRuntime(1, 60, "crossfade-upper", 25, 60);

    RouteSpec upper;
    upper.zoneId = "crossfade-upper";
    upper.velocityLow = 25;
    upper.velocityHigh = 127;
    upper.crossfade.fadeInLowVelocity = 25;
    upper.crossfade.fadeInHighVelocity = 60;
    if (includeUpperRuntime)
        upper.crossfadeRuntime = makeFadeInRuntime(25, 127, "crossfade-lower", 25, 60);

    return { lower, upper };
}

void runCrossfadeGainMatrix()
{
    const auto model = buildModel(makeCrossfadePair());

    const auto verifyVelocity = [&](int velocity,
                                    std::uint32_t expectedStartedVoices,
                                    std::uint32_t expectedCrossfadeVoices,
                                    std::uint32_t expectedOverlapHits,
                                    float expectedSample)
    {
        drs::engine::SamplerVoicePool pool;
        require(pool.prepare(*model, 48000.0), "Crossfade gain pool should prepare.");
        const std::array events { noteOn(0, velocity) };
        StereoOutput output(1);
        const auto result = pool.renderBlock(output.view(), eventView(events));
        require(result.accepted, "Crossfade gain render should be accepted.");
        require(result.render.startedVoiceCount == expectedStartedVoices,
                "Crossfade gain routing started an unexpected number of voices.");
        require(result.render.crossfadeStartedVoiceCount == expectedCrossfadeVoices,
                "Crossfade gain routing crossfade voice count changed unexpectedly.");
        require(result.render.crossfadeOverlapHitCount == expectedOverlapHits,
                "Crossfade gain overlap-hit accounting changed unexpectedly.");
        require(result.render.crossfadeFallbackCount == 0,
                "Valid runtime-paired crossfade routes should not fall back.");
        requireNear(output.left.front(), expectedSample,
                    "Crossfade gain output changed unexpectedly.");
        requireNear(output.right.front(), expectedSample,
                    "Crossfade gain stereo duplication changed unexpectedly.");
    };

    verifyVelocity(25, 1, 1, 0, expectedOutput(25));
    verifyVelocity(43, 2, 2, 1, expectedOutput(43));
    verifyVelocity(60, 1, 1, 0, expectedOutput(60));
}

void runCrossfadeFallbackMatrix()
{
    const auto model = buildModel(makeCrossfadePair(false));
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Crossfade fallback pool should prepare.");

    const std::array events { noteOn(0, 43) };
    StereoOutput output(1);
    const auto result = pool.renderBlock(output.view(), eventView(events));
    require(result.accepted, "Crossfade fallback render should be accepted.");
    require(result.render.startedVoiceCount == 2,
            "Crossfade fallback should preserve the legacy full-layer start count.");
    require(result.render.crossfadeStartedVoiceCount == 0,
            "Legacy fallback should not report crossfade-weighted starts.");
    require(result.render.crossfadeOverlapHitCount == 0,
            "Legacy fallback should not report crossfade overlap hits.");
    require(result.render.crossfadeFallbackCount == 1,
            "Missing runtime pairing should record one crossfade fallback.");
    requireNear(output.left.front(), expectedOutput(43, 1.0f, 2.0f),
                "Crossfade fallback should retain the legacy doubled overlap gain.");
}

void runRoundRobinPairMatrix()
{
    auto routes = makeCrossfadePair();
    routes[0].sampleValue = 1.0f;
    routes[0].roundRobinLength = 2;
    routes[0].roundRobinPosition = 1;
    routes[1].sampleValue = 1.0f;
    routes[1].roundRobinLength = 2;
    routes[1].roundRobinPosition = 1;

    auto secondPair = makeCrossfadePair();
    secondPair[0].zoneId = "crossfade-lower-rr2";
    secondPair[0].sampleValue = 2.0f;
    secondPair[0].roundRobinLength = 2;
    secondPair[0].roundRobinPosition = 2;
    secondPair[0].crossfadeRuntime = makeFadeOutRuntime(1, 60, "crossfade-upper-rr2", 25, 60);
    secondPair[1].zoneId = "crossfade-upper-rr2";
    secondPair[1].sampleValue = 2.0f;
    secondPair[1].roundRobinLength = 2;
    secondPair[1].roundRobinPosition = 2;
    secondPair[1].crossfadeRuntime = makeFadeInRuntime(25, 127, "crossfade-lower-rr2", 25, 60);

    routes.insert(routes.end(), secondPair.begin(), secondPair.end());

    const auto model = buildModel(routes);
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Round-robin crossfade pool should prepare.");

    const std::array events { noteOn(0, 43), noteOn(1, 43) };
    StereoOutput output(2);
    const auto result = pool.renderBlock(output.view(), eventView(events));
    require(result.accepted, "Round-robin crossfade render should be accepted.");
    require(result.render.startedVoiceCount == 4
                && result.render.crossfadeStartedVoiceCount == 4
                && result.render.crossfadeOverlapHitCount == 2
                && result.render.crossfadeFallbackCount == 0,
            "Round-robin crossfade counters changed unexpectedly.");
    requireNear(output.left[0], expectedOutput(43, 1.0f),
                "First note-on should resolve to the first round-robin pair.");
    requireNear(output.left[1], expectedOutput(43, 2.0f),
                "Second note-on should advance to the second round-robin pair.");
}

void runPlaybackContextCounterMatrix()
{
    const auto model = buildModel(makeCrossfadePair());
    drs::engine::SamplerPlaybackContext context(drs::engine::PlaybackActivationLane::preview);
    require(context.prepare(48000.0), "Crossfade playback context should prepare.");
    require(context.stageActivation(model), "Crossfade playback context should accept staging.");

    const std::array events { noteOn(0, 43) };
    StereoOutput output(1);
    const auto render = context.renderBlock(output.view(), eventView(events));
    require(render.accepted && render.activationApplied,
            "Crossfade playback context should apply and render the staged activation.");

    const auto snapshot = context.getSnapshot();
    require(snapshot.counters.crossfadeStartedVoiceCount == 2,
            "Playback-context snapshot crossfade-start counter changed unexpectedly.");
    require(snapshot.counters.crossfadeOverlapHitCount == 1,
            "Playback-context snapshot overlap counter changed unexpectedly.");
    require(snapshot.counters.crossfadeFallbackCount == 0,
            "Playback-context snapshot fallback counter changed unexpectedly.");
}
} // namespace

int main()
{
    try
    {
        runCrossfadeGainMatrix();
        runCrossfadeFallbackMatrix();
        runRoundRobinPairMatrix();
        runPlaybackContextCounterMatrix();
        std::cout << "Sprint 4 crossfade mixing tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 4 crossfade mixing tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
