#include "drs/engine/PerformanceLaneState.h"
#include "drs/engine/DraftPlaybackContract.h"
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

SamplerRenderEvent event(const SamplerRenderEventType type,
                         const std::uint32_t offset,
                         const std::uint8_t note = 0,
                         const float velocity = 1.0f)
{
    return { type, offset, note, velocity };
}

CompiledPerformanceProgram pedalProgram()
{
    CompiledPerformanceProgram program;
    program.articulationCount = 1;
    program.defaultArticulationIndex = 0;
    program.articulationStableIds = { 0x706564616cull };
    // Ordered by semantic event: release, pedal-down, pedal-up.
    program.triggerRoutes = {
        { 0, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::release, PerformanceSustainCondition::pedalUp, PerformancePitchSource::eventNote },
        { 1, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::pedalDown, PerformanceSustainCondition::pedalDown, PerformancePitchSource::fixedRoot },
        { 2, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::pedalUp, PerformanceSustainCondition::pedalUp, PerformancePitchSource::fixedRoot }
    };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::release)] = { 0, 1 };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::pedalDown)] = { 1, 1 };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::pedalUp)] = { 2, 1 };
    return program;
}

SamplerRenderModelPtr pedalRenderModel()
{
    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 7;
    snapshot.contentDigest = "sprint7-pedal-snapshot";
    PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "pedal-noise";
    snapshotZone.sampleSourceId = "pedal-sample";
    snapshotZone.displayName = "Pedal Noise";
    snapshotZone.groupId = "pedal";
    snapshotZone.articulationId = "sustain";
    snapshotZone.rootKey = 36;
    snapshotZone.keyLow = 36;
    snapshotZone.keyHigh = 36;
    snapshotZone.velocityLow = 1;
    snapshotZone.velocityHigh = 127;
    snapshotZone.triggerMode = ZoneTriggerMode::oneShot;
    snapshot.zones.push_back(snapshotZone);
    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "pedal";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { "pedal-noise" };
    snapshot.groupRoutes.push_back(snapshotGroup);

    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 701;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint7-pedal-prepared";
    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(64, 0.75f) };
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "pedal-sample";
    sample.streamSampleId = "pedal-stream";
    sample.sampleRate = 48000.0;
    sample.frameCount = 64;
    sample.channelCount = 1;
    sample.decodedSampleData = std::move(decoded);
    prepared.samples.push_back(std::move(sample));
    PreparedPlaybackZoneHandle zone;
    zone.zoneId = "pedal-noise";
    zone.sampleSourceId = "pedal-sample";
    zone.streamSampleId = "pedal-stream";
    zone.preparedSampleIndex = 0;
    zone.preparedStreamIndex = 0;
    zone.rootKey = 36;
    zone.keyLow = 36;
    zone.keyHigh = 36;
    zone.velocityLow = 1;
    zone.velocityHigh = 127;
    zone.triggerMode = ZoneTriggerMode::oneShot;
    prepared.zones.push_back(std::move(zone));
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "pedal";
    preparedGroup.articulationIds = { "sustain" };
    preparedGroup.zoneIds = { "pedal-noise" };
    prepared.groupRoutes.push_back(std::move(preparedGroup));
    prepared.performanceProgram = pedalProgram();
    prepared.performanceProgram.triggerRoutes = {
        { 0, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::pedalDown, PerformanceSustainCondition::pedalDown, PerformancePitchSource::fixedRoot }
    };
    prepared.performanceProgram.eventRanges = {};
    prepared.performanceProgram.eventRanges[static_cast<std::size_t>(PerformanceEventKind::pedalDown)] = { 0, 1 };
    prepared.performanceProgram.zoneArticulationIndices = { 0 };

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 702;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = buildSamplerRenderModel(payload);
    if (!result.built)
    {
        std::string details;
        for (const auto& finding : result.findings)
            details += finding.code + ": " + finding.message + " ";
        throw std::runtime_error("Pedal render fixture must validate: " + details);
    }
    require(result.built && result.model != nullptr, "Pedal render fixture must validate.");
    return result.model;
}

void verifyTransitionOnlyPedalSamples()
{
    const auto program = pedalProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 71);

    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 4, 0, 1.0f), 71, program, scratch),
            "CC64 pedal-down must normalize.");
    require(scratch.size() == 2 && scratch.view()[0].type == SamplerRenderEventType::pedalDown
                && scratch.view()[0].sustainPedalDown
                && scratch.view()[1].type == SamplerRenderEventType::noteOn
                && scratch.view()[1].performanceEvent == PerformanceEventKind::pedalDown
                && scratch.view()[1].sustainPedalDown && scratch.view()[1].midiNote == 0
                && scratch.view()[1].velocity == 1.0f && scratch.view()[1].articulationIndex == 0,
            "Pedal-down must apply state first, then emit one fixed-root trigger at deterministic velocity.");

    scratch.clear();
    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 5, 0, 1.0f), 71, program, scratch)
                && scratch.size() == 0,
            "Repeated high CC64 values must not re-trigger pedal noise.");

    require(state.normalize(event(SamplerRenderEventType::sustainPedal, 9, 0, 0.0f), 71, program, scratch),
            "CC64 pedal-up must normalize.");
    require(scratch.size() == 2 && scratch.view()[0].type == SamplerRenderEventType::pedalUp
                && !scratch.view()[0].sustainPedalDown
                && scratch.view()[1].type == SamplerRenderEventType::noteOn
                && scratch.view()[1].performanceEvent == PerformanceEventKind::pedalUp
                && !scratch.view()[1].sustainPedalDown && scratch.view()[1].velocity == 1.0f,
            "Pedal-up must emit one fixed-root trigger after the state transition.");
}

void verifyPedalUpReleasePrecedenceAndBurstBudget()
{
    const auto program = pedalProgram();
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 72);
    state.normalize(event(SamplerRenderEventType::pedalDown, 0), 72, program, scratch);
    scratch.clear();
    for (std::uint8_t note = 36; note < 44; ++note)
    {
        state.normalize(event(SamplerRenderEventType::noteOn, 1, note, 0.7f), 72, program, scratch);
        scratch.clear();
        state.normalize(event(SamplerRenderEventType::noteOff, 2, note, 0.0f), 72, program, scratch);
        scratch.clear();
    }

    require(state.normalize(event(SamplerRenderEventType::pedalUp, 17, 0, 0.0f), 72, program, scratch),
            "Pedal-up burst must normalize atomically.");
    require(scratch.size() == 10 && scratch.view()[0].type == SamplerRenderEventType::pedalUp
                && scratch.view()[1].performanceEvent == PerformanceEventKind::release
                && scratch.view()[8].performanceEvent == PerformanceEventKind::release
                && scratch.view()[9].performanceEvent == PerformanceEventKind::pedalUp
                && !scratch.view()[9].sustainPedalDown
                && state.getSnapshot().heldNoteCount == 0 && state.getSnapshot().actionOverflowCount == 0,
            "Pedal-up must release every deferred source before its pedal-up sample without partial fan-out.");
}

void verifyFixedRootVoiceRouting()
{
    const auto model = pedalRenderModel();
    SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0, 73), "Pedal voice pool must prepare.");
    std::array<float, 8> audio {};
    float* channels[] { audio.data() };
    auto trigger = event(SamplerRenderEventType::noteOn, 0, 0, 1.0f);
    trigger.articulationIndex = 0;
    trigger.performanceEvent = PerformanceEventKind::pedalDown;
    trigger.sustainPedalDown = true;
    const auto result = pool.renderBlock({ channels, 1, static_cast<std::uint32_t>(audio.size()) }, { &trigger, 1 });
    require(result.render.startedVoiceCount == 1 && pool.getSlotSnapshot(0).sourceMidiNote == 36
                && std::abs(audio[0] - 0.75f) < 0.0001f,
            "A fixed-root pedal route must start as an untransposed root-key one-shot, not MIDI note zero.");
}
} // namespace

int main()
{
    try
    {
        verifyTransitionOnlyPedalSamples();
        verifyPedalUpReleasePrecedenceAndBurstBudget();
        verifyFixedRootVoiceRouting();
        std::cout << "Performance-engine Sprint 7 pedal tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 7 pedal tests failed: " << exception.what() << '\n';
        return 1;
    }
}
