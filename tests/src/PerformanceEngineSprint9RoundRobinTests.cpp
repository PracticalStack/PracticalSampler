#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PerformanceLaneState.h"
#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

std::uint64_t fnv1a(const std::string_view value)
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : value)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }
    return hash;
}

SamplerRenderModelPtr roundRobinModel()
{
    constexpr std::string_view poolId = "rr-main";
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 9;
    snapshot.contentDigest = "sprint9-rr-snapshot";
    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 901;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint9-rr-prepared";

    for (std::size_t index = 0; index < 3; ++index)
    {
        const auto id = std::string("rr-") + std::to_string(index + 1);
        const auto sampleId = id + "-sample";
        const RoundRobinDescriptor descriptor { std::string(poolId), 3, static_cast<int>(index + 1), RoundRobinMode::sequential };
        PlaybackSnapshotZone snapshotZone;
        snapshotZone.id = id;
        snapshotZone.sampleSourceId = sampleId;
        snapshotZone.displayName = id;
        snapshotZone.groupId = "rr";
        snapshotZone.articulationId = "kit";
        snapshotZone.rootKey = 60;
        snapshotZone.keyLow = 60;
        snapshotZone.keyHigh = 60;
        snapshotZone.velocityLow = 1;
        snapshotZone.velocityHigh = 127;
        snapshotZone.roundRobin = descriptor;
        snapshotZone.roundRobinLength = 3;
        snapshotZone.roundRobinPosition = static_cast<int>(index + 1);
        snapshot.zones.push_back(std::move(snapshotZone));

        auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float> { 0.2f + static_cast<float>(index) * 0.2f } };
        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = sampleId + "-stream";
        sample.sampleRate = 48000.0;
        sample.frameCount = 1;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));

        PreparedPlaybackZoneHandle zone;
        zone.zoneId = id;
        zone.sampleSourceId = sampleId;
        zone.streamSampleId = sampleId + "-stream";
        zone.preparedSampleIndex = index;
        zone.preparedStreamIndex = index;
        zone.rootKey = 60;
        zone.keyLow = 60;
        zone.keyHigh = 60;
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        zone.roundRobin = descriptor;
        zone.roundRobinLength = 3;
        zone.roundRobinPosition = static_cast<int>(index + 1);
        prepared.zones.push_back(std::move(zone));
    }

    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "rr";
    snapshotGroup.articulationIds = { "kit" };
    snapshotGroup.zoneIds = { "rr-1", "rr-2", "rr-3" };
    snapshot.groupRoutes.push_back(snapshotGroup);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = snapshotGroup.groupId;
    preparedGroup.articulationIds = snapshotGroup.articulationIds;
    preparedGroup.zoneIds = snapshotGroup.zoneIds;
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    prepared.performanceProgram.articulationCount = 1;
    prepared.performanceProgram.defaultArticulationIndex = 0;
    prepared.performanceProgram.articulationStableIds = { 0x6b6974ull };
    prepared.performanceProgram.roundRobinPoolCount = 1;
    prepared.performanceProgram.roundRobinPoolStableIds = { fnv1a(poolId) };
    prepared.performanceProgram.zoneArticulationIndices = { 0, 0, 0 };
    prepared.performanceProgram.triggerRoutes = {
        { 0, 0, kInvalidPerformanceProgramIndex, 0, 0.0f, PerformanceEventKind::noteOn,
          PerformanceSustainCondition::any, PerformancePitchSource::eventNote },
        { 1, 0, kInvalidPerformanceProgramIndex, 0, 0.0f, PerformanceEventKind::noteOn,
          PerformanceSustainCondition::any, PerformancePitchSource::eventNote },
        { 2, 0, kInvalidPerformanceProgramIndex, 0, 0.0f, PerformanceEventKind::noteOn,
          PerformanceSustainCondition::any, PerformancePitchSource::eventNote }
    };
    prepared.performanceProgram.eventRanges[static_cast<std::size_t>(PerformanceEventKind::noteOn)] = { 0, 3 };
    prepared.performanceProgram.roundRobinResets = {
        { RoundRobinResetEvent::pedalUp, 0 },
        { RoundRobinResetEvent::articulationChange, kInvalidPerformanceProgramIndex }
    };

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 902;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr, "Round Robin reset fixture must validate.");
    return result.model;
}

SamplerRenderEvent noteOn()
{
    SamplerRenderEvent event { SamplerRenderEventType::noteOn, 0, 60, 1.0f };
    event.articulationIndex = 0;
    return event;
}

float renderOne(SamplerVoicePool& pool, const SamplerRenderEvent& event)
{
    std::array<float, 1> audio {};
    float* channels[] { audio.data() };
    const auto result = pool.renderBlock({ channels, 1, 1 }, { &event, 1 });
    require(result.render.startedVoiceCount == 1, "Round Robin trigger must start one selected slot.");
    return audio[0];
}

void verifyTargetedAndAllPoolResets()
{
    const auto model = roundRobinModel();
    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0, 91), "Round Robin reset pool must prepare.");

    const auto first = renderOne(pool, noteOn());
    const auto second = renderOne(pool, noteOn());
    require(std::abs(first - 0.05f) < 0.0001f && std::abs(second - 0.10f) < 0.0001f,
            "Sequential Round Robin must advance from slot 1 to slot 2.");

    std::array<float, 1> silence {};
    float* channels[] { silence.data() };
    SamplerRenderEvent pedalDown { SamplerRenderEventType::pedalDown, 0, 0, 1.0f };
    pool.renderBlock({ channels, 1, 1 }, { &pedalDown, 1 });
    SamplerRenderEvent pedalUp { SamplerRenderEventType::pedalUp, 0, 0, 0.0f };
    pool.renderBlock({ channels, 1, 1 }, { &pedalUp, 1 });
    require(std::abs(renderOne(pool, noteOn()) - 0.05f) < 0.0001f,
            "A targeted pedal-up reset must restore its pool to slot 1 before the next trigger.");

    renderOne(pool, noteOn());
    SamplerRenderEvent articulationReset { SamplerRenderEventType::roundRobinReset, 0, 0, 0.0f };
    articulationReset.roundRobinResetEvent = RoundRobinResetEvent::articulationChange;
    pool.renderBlock({ channels, 1, 1 }, { &articulationReset, 1 });
    require(std::abs(renderOne(pool, noteOn()) - 0.05f) < 0.0001f,
            "An articulation-change all-pools reset must precede its same-offset trigger selection.");
}

void verifyArticulationChangeResetControl()
{
    CompiledPerformanceProgram program;
    program.articulationCount = 2;
    program.defaultArticulationIndex = 0;
    program.articulationStableIds = { 0x7375737461696eull, 0x737461636361746full };
    program.activationByMidiNote[14] = { 1, true };
    program.roundRobinResets = { { RoundRobinResetEvent::articulationChange, kInvalidPerformanceProgramIndex } };
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 92);
    SamplerRenderEvent keySwitch { SamplerRenderEventType::noteOn, 7, 14, 1.0f };
    require(state.normalize(keySwitch, 92, program, scratch)
                && scratch.size() == 1 && scratch.view()[0].type == SamplerRenderEventType::roundRobinReset
                && scratch.view()[0].roundRobinResetEvent == RoundRobinResetEvent::articulationChange
                && state.getSnapshot().selectedArticulationIndex == 1,
            "A consumed articulation switch must emit its authored Round Robin reset before playable routing.");
}
} // namespace

int main()
{
    try
    {
        verifyTargetedAndAllPoolResets();
        verifyArticulationChangeResetControl();
        std::cout << "Performance-engine Sprint 9 Round Robin tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 9 Round Robin tests failed: " << exception.what() << '\n';
        return 1;
    }
}
