#include "drs/engine/PerformanceLaneState.h"
#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoicePool.h"

#include <algorithm>
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
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

struct ModelFixture
{
    SamplerRenderModelPtr model;
    std::shared_ptr<const PreparedPlaybackDecodedSampleData> decoded;
};

ModelFixture buildModel(const std::size_t revision,
                        const int curveIndex,
                        const bool reverseCurve = false)
{
    ContinuousDamperDefinition damper;
    damper.sustainControllerNumber = 90;
    damper.sustainThreshold = 0.5;
    damper.dynamicRelease = true;
    damper.releaseControllerNumber = 64;
    damper.releaseAmountSeconds = 0.1;
    damper.releaseCurveIndex = curveIndex;
    for (std::size_t index = 0; index < damper.releaseCurve.size(); ++index)
    {
        const auto normalized = static_cast<double>(index) / 127.0;
        damper.releaseCurve[index] = reverseCurve ? 1.0 - normalized : normalized;
    }

    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = "hp03-snapshot-" + std::to_string(revision);
    PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "hp03-zone-" + std::to_string(revision);
    snapshotZone.sampleSourceId = "hp03-sample-" + std::to_string(revision);
    snapshotZone.displayName = "HP-03 Loop";
    snapshotZone.groupId = "main";
    snapshotZone.rootKey = 60;
    snapshotZone.keyLow = 0;
    snapshotZone.keyHigh = 127;
    snapshotZone.loopEnabled = true;
    snapshotZone.loopStartFrame = 0;
    snapshotZone.loopEndFrame = 4096;
    snapshotZone.releaseSeconds = 0.01;
    snapshotZone.releaseShape = -10.3616;
    snapshotZone.damper = damper;
    snapshot.zones.push_back(snapshotZone);
    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "main";
    snapshotGroup.zoneIds = { snapshotZone.id };
    snapshot.groupRoutes.push_back(snapshotGroup);

    auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::vector<float>(4096, 1.0f) };

    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 3000 + revision * 2;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = "hp03-prepared-" + std::to_string(revision);
    PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = snapshotZone.sampleSourceId;
    sample.streamSampleId = "hp03-stream-" + std::to_string(revision);
    sample.sampleRate = 1000.0;
    sample.frameCount = 4096;
    sample.channelCount = 1;
    sample.decodedSampleData = decoded;
    prepared.samples.push_back(sample);
    PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = snapshotZone.id;
    preparedZone.sampleSourceId = snapshotZone.sampleSourceId;
    preparedZone.streamSampleId = sample.streamSampleId;
    preparedZone.rootKey = 60;
    preparedZone.keyLow = 0;
    preparedZone.keyHigh = 127;
    preparedZone.loopEnabled = true;
    preparedZone.loopStartFrame = 0;
    preparedZone.loopEndFrame = 4096;
    preparedZone.releaseSeconds = snapshotZone.releaseSeconds;
    preparedZone.releaseShape = snapshotZone.releaseShape;
    preparedZone.damper = damper;
    prepared.zones.push_back(preparedZone);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "main";
    preparedGroup.zoneIds = { preparedZone.zoneId };
    prepared.groupRoutes.push_back(preparedGroup);

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = PlaybackActivationLane::performance;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = prepared.snapshotBuildId + 1;
    payload->lifecycleState = PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto built = buildSamplerRenderModel(payload);
    require(built.built && built.model != nullptr, "HP-03 model must build");
    require(built.model->usesContinuousDamper()
                && built.model->getSustainControllerNumber() == 90
                && built.model->getSustainThreshold() == 0.5,
            "Render model must expose the focused sustain control");
    return { built.model, decoded };
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> channels;

    explicit StereoOutput(const std::size_t frames)
        : left(frames, 0.0f), right(frames, 0.0f),
          channels { left.data(), right.data() }
    {
    }

    SamplerAudioBufferView view()
    {
        return { channels.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

SamplerRenderEvent note(const SamplerRenderEventType type,
                        const std::uint32_t offset,
                        const int midiNote)
{
    SamplerRenderEvent event;
    event.type = type;
    event.sampleOffset = offset;
    event.midiNote = static_cast<std::uint8_t>(midiNote);
    event.velocity = type == SamplerRenderEventType::noteOn ? 1.0f : 0.0f;
    return event;
}

SamplerRenderEvent controller(const std::uint32_t offset,
                              const int number,
                              const int value)
{
    SamplerRenderEvent event;
    event.type = SamplerRenderEventType::controllerChange;
    event.sampleOffset = offset;
    event.controllerNumber = static_cast<std::uint8_t>(number);
    event.controllerValue = static_cast<std::uint8_t>(value);
    return event;
}

template <std::size_t Size>
SamplerRenderEventView view(const std::array<SamplerRenderEvent, Size>& events)
{
    return { events.data(), events.size() };
}

SamplerVoiceSlotSnapshot findVoice(const SamplerVoicePool& pool, const int noteValue)
{
    for (std::size_t index = 0; index < SamplerVoicePool::capacity; ++index)
    {
        const auto slot = pool.getSlotSnapshot(index);
        if ((slot.state == SamplerVoiceSlotState::active
             || slot.state == SamplerVoiceSlotState::releasing)
            && slot.sourceMidiNote == noteValue)
            return slot;
    }
    return {};
}

void verifyExactControllerAndReassignedSustain()
{
    CompiledPerformanceProgram program;
    PerformanceLaneState state;
    PerformanceActionScratch scratch;
    state.migrateProgram(program, 41, true, 90, 0.5);

    SamplerRenderEvent rawCc64;
    rawCc64.type = SamplerRenderEventType::sustainPedal;
    rawCc64.sampleOffset = 7;
    rawCc64.velocity = 62.0f / 127.0f;
    rawCc64.controllerNumber = 64;
    rawCc64.controllerValue = 62;
    require(state.normalize(rawCc64, 41, program, scratch)
                && scratch.size() == 1
                && scratch.view()[0].type == SamplerRenderEventType::controllerChange
                && scratch.view()[0].controllerValue == 62
                && state.getSnapshot().controllerValues[64] == 62
                && !state.getSnapshot().pedalDown,
            "Continuous CC64 must remain exact and must not toggle CC90 sustain");

    scratch.clear();
    require(state.normalize(controller(9, 90, 1), 41, program, scratch)
                && scratch.size() == 2
                && scratch.view()[0].type == SamplerRenderEventType::controllerChange
                && scratch.view()[1].type == SamplerRenderEventType::pedalDown
                && state.getSnapshot().pedalDown,
            "CC90 threshold crossing must emit one binary pedal-down edge");
    const auto pedalDownCount = state.getSnapshot().semanticEventCounts[
        static_cast<std::size_t>(PerformanceEventKind::pedalDown)];
    scratch.clear();
    require(state.normalize(controller(10, 90, 1), 41, program, scratch)
                && scratch.size() == 1
                && state.getSnapshot().semanticEventCounts[
                    static_cast<std::size_t>(PerformanceEventKind::pedalDown)] == pedalDownCount,
            "Repeated intermediate sustain values must not duplicate pedal mechanics");

    state.migrateProgram(program, 42, true, 90, 0.5);
    require(state.getSnapshot().controllerValues[64] == 62
                && state.getSnapshot().controllerValues[90] == 1
                && state.getSnapshot().pedalDown,
            "Continuous controller state must survive activation migration exactly");
}

void verifyDynamicReleaseAndContinuity(const ModelFixture& fixture)
{
    SamplerVoicePool pool;
    require(pool.prepare(*fixture.model, 1000.0, 41), "HP-03 pool must prepare");
    const std::array start {
        controller(0, 64, 32), note(SamplerRenderEventType::noteOn, 0, 60),
        note(SamplerRenderEventType::noteOff, 2, 60)
    };
    StereoOutput first(3);
    const auto started = pool.renderBlock(first.view(), view(start));
    auto voice = findVoice(pool, 60);
    const auto expected32 = static_cast<std::uint32_t>(std::llround(
        (0.01 + 0.1 * (32.0 / 127.0)) * 1000.0));
    require(started.accepted && started.render.releasedVoiceCount == 1
                && voice.state == SamplerVoiceSlotState::releasing
                && voice.releaseSamplesTotal == expected32,
            "Physical note-off must start the curve-selected dynamic release");

    const auto levelBeforeUpdate = voice.releaseEnvelopeLevel;
    const std::array update { controller(0, 64, 64) };
    StereoOutput updated(1);
    const auto updateResult = pool.renderBlock(updated.view(), view(update));
    voice = findVoice(pool, 60);
    const auto expected64 = static_cast<std::uint32_t>(std::llround(
        (0.01 + 0.1 * (64.0 / 127.0)) * 1000.0));
    require(updateResult.dynamicReleaseUpdateCount == 1
                && voice.releaseSamplesTotal == expected64
                && voice.releaseSamplesRemaining == expected64 - 1
                && voice.dynamicReleaseUpdateCount == 1,
            "A sample-positioned CC64 change must replace only the future release duration");
    require(std::abs(updated.left.front() - levelBeforeUpdate) < 1.0e-6f,
            "Dynamic release update must preserve the current envelope level exactly");

    const auto remainingBeforeRepeat = voice.releaseSamplesRemaining;
    StereoOutput repeated(1);
    const auto repeatedResult = pool.renderBlock(repeated.view(), view(update));
    voice = findVoice(pool, 60);
    require(repeatedResult.dynamicReleaseUpdateCount == 0
                && voice.dynamicReleaseUpdateCount == 1
                && voice.releaseSamplesRemaining == remainingBeforeRepeat - 1,
            "A repeated controller value must not restart the release segment");
}

std::uint32_t measureTailSamples(const ModelFixture& fixture, const int controllerValue)
{
    constexpr double ladderSampleRate = 48000.0;
    SamplerVoicePool pool;
    require(pool.prepare(*fixture.model, ladderSampleRate, 70 + controllerValue),
            "Tail-ladder pool must prepare");
    const std::array events {
        controller(0, 64, controllerValue),
        note(SamplerRenderEventType::noteOn, 0, 60),
        note(SamplerRenderEventType::noteOff, 1, 60)
    };
    StereoOutput output(2);
    pool.renderBlock(output.view(), view(events));
    return findVoice(pool, 60).releaseSamplesTotal;
}

void verifyMonotonicTailLadder(const ModelFixture& fixture)
{
    constexpr double ladderSampleRate = 48000.0;
    constexpr std::array values { 0, 20, 32, 42, 54, 62, 63, 64, 127 };
    std::array<std::uint32_t, values.size()> tails {};
    for (std::size_t index = 0; index < values.size(); ++index)
        tails[index] = measureTailSamples(fixture, values[index]);
    require(std::is_sorted(tails.begin(), tails.end())
                && std::adjacent_find(tails.begin(), tails.end()) == tails.end(),
            "Linear curve points must produce strictly monotonic offline tail lengths");
    const auto expectedBoundaryDelta = static_cast<std::uint32_t>(std::llround(
        (0.01 + 0.1 * (64.0 / 127.0)) * ladderSampleRate))
        - static_cast<std::uint32_t>(std::llround(
            (0.01 + 0.1 * (63.0 / 127.0)) * ladderSampleRate));
    require(tails[7] - tails[6] == expectedBoundaryDelta,
            "The 63-to-64 boundary must contain only the authored curve step");
}

void verifySustainRouting(const ModelFixture& fixture)
{
    SamplerVoicePool immediate;
    require(immediate.prepare(*fixture.model, 1000.0, 51), "Immediate pool must prepare");
    const std::array cc64Only {
        controller(0, 64, 127), note(SamplerRenderEventType::noteOn, 0, 60),
        note(SamplerRenderEventType::noteOff, 1, 60)
    };
    StereoOutput immediateOutput(2);
    immediate.renderBlock(immediateOutput.view(), view(cc64Only));
    require(findVoice(immediate, 60).state == SamplerVoiceSlotState::releasing,
            "CC64 must not defer note-off when binary sustain is assigned to CC90");

    SamplerVoicePool deferred;
    require(deferred.prepare(*fixture.model, 1000.0, 52), "Deferred pool must prepare");
    const std::array held {
        controller(0, 90, 1), note(SamplerRenderEventType::noteOn, 0, 61),
        note(SamplerRenderEventType::noteOff, 1, 61)
    };
    StereoOutput heldOutput(2);
    deferred.renderBlock(heldOutput.view(), view(held));
    require(findVoice(deferred, 61).state == SamplerVoiceSlotState::active
                && deferred.sustainDeferredVoiceCount() == 1,
            "CC90 down must defer effective release");
    const std::array unrelated { controller(0, 64, 20) };
    StereoOutput unrelatedOutput(1);
    deferred.renderBlock(unrelatedOutput.view(), view(unrelated));
    require(findVoice(deferred, 61).state == SamplerVoiceSlotState::active,
            "Release-control CC64 must not release a CC90-deferred voice");
    const std::array up { controller(0, 90, 0) };
    StereoOutput upOutput(1);
    const auto released = deferred.renderBlock(upOutput.view(), view(up));
    require(released.render.releasedVoiceCount == 1
                && findVoice(deferred, 61).state == SamplerVoiceSlotState::releasing,
            "CC90 up must release the deferred voice exactly once");
}

void verifyGenerationOwnedCurve(const ModelFixture& first,
                                const ModelFixture& second)
{
    SamplerVoicePool pool;
    require(pool.prepare(*first.model, 1000.0, 41), "Generation 41 must prepare");
    const std::array start {
        controller(0, 64, 32), note(SamplerRenderEventType::noteOn, 0, 60),
        note(SamplerRenderEventType::noteOff, 1, 60)
    };
    StereoOutput startOutput(2);
    pool.renderBlock(startOutput.view(), view(start));
    require(pool.activateModel(*second.model, 1000.0, 42), "Generation 42 must activate");
    const std::array update { controller(0, 64, 64) };
    StereoOutput updateOutput(1);
    pool.renderBlock(updateOutput.view(), view(update));
    const auto oldVoice = findVoice(pool, 60);
    const auto expectedOld = static_cast<std::uint32_t>(std::llround(
        (0.01 + 0.1 * (64.0 / 127.0)) * 1000.0));
    require(oldVoice.activationGeneration == 41 && oldVoice.damperCurveIndex == 11
                && oldVoice.releaseSamplesTotal == expectedOld,
            "Retired voice must update from its generation-41 curve");

    const std::array newNote {
        note(SamplerRenderEventType::noteOn, 0, 64),
        note(SamplerRenderEventType::noteOff, 1, 64)
    };
    StereoOutput newOutput(2);
    pool.renderBlock(newOutput.view(), view(newNote));
    const auto newVoice = findVoice(pool, 64);
    const auto expectedNew = static_cast<std::uint32_t>(std::llround(
        (0.01 + 0.1 * (1.0 - 64.0 / 127.0)) * 1000.0));
    require(newVoice.activationGeneration == 42 && newVoice.damperCurveIndex == 17
                && newVoice.releaseSamplesTotal == expectedNew,
            "New voice must start from the active generation-42 curve and retained CC64");
}
} // namespace

int main()
{
    try
    {
        verifyExactControllerAndReassignedSustain();
        const auto first = buildModel(41, 11, false);
        const auto second = buildModel(42, 17, true);
        verifyDynamicReleaseAndContinuity(first);
        verifyMonotonicTailLadder(first);
        verifySustainRouting(first);
        verifyGenerationOwnedCurve(first, second);
        std::cout << "Continuous damper HP-03 realtime release tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Continuous damper HP-03 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
