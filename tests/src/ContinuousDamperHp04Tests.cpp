#include "drs/engine/SamplerPlaybackContext.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#include <malloc.h>
#endif

namespace allocation_probe
{
thread_local bool enabled = false;
thread_local std::size_t allocations = 0;
thread_local std::size_t deallocations = 0;

void reset() noexcept
{
    allocations = 0;
    deallocations = 0;
}

void* allocate(const std::size_t size)
{
    if (enabled) ++allocations;
    if (auto* pointer = std::malloc(size == 0 ? 1 : size)) return pointer;
    throw std::bad_alloc {};
}

void* allocateAligned(const std::size_t size, const std::size_t alignment)
{
    if (enabled) ++allocations;
#if defined(_MSC_VER)
    if (auto* pointer = _aligned_malloc(size == 0 ? 1 : size, alignment)) return pointer;
#else
    void* pointer = nullptr;
    if (posix_memalign(&pointer, alignment, size == 0 ? 1 : size) == 0) return pointer;
#endif
    throw std::bad_alloc {};
}

void free(void* pointer) noexcept
{
    if (enabled && pointer != nullptr) ++deallocations;
    std::free(pointer);
}

void freeAligned(void* pointer) noexcept
{
    if (enabled && pointer != nullptr) ++deallocations;
#if defined(_MSC_VER)
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}
} // namespace allocation_probe

void* operator new(std::size_t size) { return allocation_probe::allocate(size); }
void* operator new[](std::size_t size) { return allocation_probe::allocate(size); }
void operator delete(void* pointer) noexcept { allocation_probe::free(pointer); }
void operator delete[](void* pointer) noexcept { allocation_probe::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { allocation_probe::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { allocation_probe::free(pointer); }
void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocation_probe::allocateAligned(size, static_cast<std::size_t>(alignment));
}
void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocation_probe::allocateAligned(size, static_cast<std::size_t>(alignment));
}
void operator delete(void* pointer, std::align_val_t) noexcept
{
    allocation_probe::freeAligned(pointer);
}
void operator delete[](void* pointer, std::align_val_t) noexcept
{
    allocation_probe::freeAligned(pointer);
}
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept
{
    allocation_probe::freeAligned(pointer);
}
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept
{
    allocation_probe::freeAligned(pointer);
}

namespace
{
using namespace drs::engine;
constexpr double sampleRate = 48000.0;
constexpr float tolerance = 1.0e-6f;
constexpr float releaseTriggerLevel = 0.25f;

void require(const bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

void requireNear(const float actual, const float expected, const std::string& message)
{
    if (std::abs(actual - expected) > tolerance)
        throw std::runtime_error(message + " (actual=" + std::to_string(actual)
                                 + ", expected=" + std::to_string(expected) + ")");
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> channels;

    explicit StereoOutput(const std::size_t frames)
        : left(frames, 0.0f), right(frames, 0.0f), channels { left.data(), right.data() }
    {
    }

    SamplerAudioBufferView view()
    {
        return { channels.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

SamplerRenderEvent event(const SamplerRenderEventType type,
                         const std::uint32_t offset = 0,
                         const int note = 60)
{
    SamplerRenderEvent result;
    result.type = type;
    result.sampleOffset = offset;
    result.midiNote = static_cast<std::uint8_t>(note);
    result.velocity = type == SamplerRenderEventType::noteOn ? 1.0f : 0.0f;
    return result;
}

SamplerRenderEvent controller(const std::uint32_t offset, const int value)
{
    auto result = event(SamplerRenderEventType::controllerChange, offset, 0);
    result.controllerNumber = 64;
    result.controllerValue = static_cast<std::uint8_t>(value);
    return result;
}

SamplerRenderEvent hostCc64(const std::uint32_t offset, const int value)
{
    auto result = event(SamplerRenderEventType::sustainPedal, offset, 0);
    result.velocity = static_cast<float>(value) / 127.0f;
    result.controllerNumber = 64;
    result.controllerValue = static_cast<std::uint8_t>(value);
    return result;
}

template <std::size_t Size>
SamplerRenderEventView view(const std::array<SamplerRenderEvent, Size>& events)
{
    return { events.data(), events.size() };
}

SamplerRenderEventView noEvents()
{
    return { nullptr, 0 };
}

struct ModelFixture
{
    SamplerRenderModelPtr model;
};

ModelFixture buildModel(const std::size_t revision,
                        const PlaybackActivationLane lane = PlaybackActivationLane::performance)
{
    ContinuousDamperDefinition damper;
    damper.sustainControllerNumber = 90;
    damper.sustainThreshold = 0.5;
    damper.dynamicRelease = true;
    damper.releaseControllerNumber = 64;
    damper.releaseAmountSeconds = 0.2;
    damper.releaseCurveIndex = 11;
    for (std::size_t index = 0; index < damper.releaseCurve.size(); ++index)
        damper.releaseCurve[index] = static_cast<double>(index) / 127.0;

    CompiledPerformanceProgram program;
    program.articulationCount = 1;
    program.defaultArticulationIndex = 0;
    program.articulationStableIds = { 0x68703034ull };
    program.triggerRoutes = {
        { 0, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::noteOn, PerformanceSustainCondition::any,
          PerformancePitchSource::eventNote },
        { 1, 0, kInvalidPerformanceProgramIndex, 0, 0.0f,
          PerformanceEventKind::release, PerformanceSustainCondition::pedalUp,
          PerformancePitchSource::eventNote }
    };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::noteOn)] = { 0, 1 };
    program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::release)] = { 1, 1 };
    program.zoneArticulationIndices = { 0, 0 };

    ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = revision;
    snapshot.contentDigest = "hp04-snapshot-" + std::to_string(revision);
    snapshot.performanceProgram = program;

    PlaybackSnapshotZone main;
    main.id = "hp04-main-" + std::to_string(revision);
    main.sampleSourceId = "hp04-main-sample-" + std::to_string(revision);
    main.displayName = "HP-04 Main";
    main.groupId = "main";
    main.articulationId = "default";
    main.rootKey = 60;
    main.keyLow = 0;
    main.keyHigh = 127;
    main.loopEnabled = true;
    main.loopStartFrame = 0;
    main.loopEndFrame = 32768;
    main.releaseSeconds = 0.02;
    main.releaseShape = -10.3616;
    main.performance = { PerformanceEventKind::noteOn, PerformanceSustainCondition::any,
                         PerformancePitchSource::eventNote };
    main.damper = damper;
    snapshot.zones.push_back(main);

    PlaybackSnapshotZone release = main;
    release.id = "hp04-release-" + std::to_string(revision);
    release.sampleSourceId = "hp04-release-sample-" + std::to_string(revision);
    release.displayName = "HP-04 Release Trigger";
    release.loopEnabled = false;
    release.loopEndFrame = 0;
    release.triggerMode = ZoneTriggerMode::oneShot;
    release.performance = { PerformanceEventKind::release,
                            PerformanceSustainCondition::pedalUp,
                            PerformancePitchSource::eventNote };
    snapshot.zones.push_back(release);

    PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "main";
    snapshotGroup.articulationIds = { "default" };
    snapshotGroup.zoneIds = { main.id, release.id };
    snapshot.groupRoutes.push_back(snapshotGroup);

    ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 4000 + revision * 2;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = revision;
    prepared.preparedContentDigest = "hp04-prepared-" + std::to_string(revision);
    prepared.performanceProgram = program;

    const auto addSample = [&](const std::string& sourceId,
                               const std::string& streamId,
                               const std::size_t frames,
                               const float level)
    {
        auto decoded = std::make_shared<PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float>(frames, level) };
        PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sourceId;
        sample.streamSampleId = streamId;
        sample.sampleRate = sampleRate;
        sample.frameCount = frames;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));
    };
    addSample(main.sampleSourceId, "hp04-main-stream-" + std::to_string(revision), 32768, 1.0f);
    addSample(release.sampleSourceId, "hp04-release-stream-" + std::to_string(revision), 4096, 0.25f);

    const auto addPreparedZone = [&](const PlaybackSnapshotZone& source,
                                     const std::size_t sampleIndex)
    {
        PreparedPlaybackZoneHandle zone;
        zone.zoneId = source.id;
        zone.sampleSourceId = source.sampleSourceId;
        zone.streamSampleId = prepared.samples[sampleIndex].streamSampleId;
        zone.preparedSampleIndex = sampleIndex;
        zone.rootKey = source.rootKey;
        zone.keyLow = source.keyLow;
        zone.keyHigh = source.keyHigh;
        zone.loopEnabled = source.loopEnabled;
        zone.loopStartFrame = source.loopStartFrame;
        zone.loopEndFrame = source.loopEndFrame;
        zone.releaseSeconds = source.releaseSeconds;
        zone.releaseShape = source.releaseShape;
        zone.triggerMode = source.triggerMode;
        zone.damper = source.damper;
        prepared.zones.push_back(std::move(zone));
    };
    addPreparedZone(main, 0);
    addPreparedZone(release, 1);
    PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "main";
    preparedGroup.articulationIds = { "default" };
    preparedGroup.zoneIds = { main.id, release.id };
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<PlaybackActivationPayload>();
    payload->lane = lane;
    payload->revision = revision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = prepared.snapshotBuildId + 1;
    payload->lifecycleState = lane == PlaybackActivationLane::preview
        ? PlaybackSnapshotLifecycleState::ready
        : PlaybackSnapshotLifecycleState::active;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const ImmutablePreparedPlayback>(std::move(prepared));
    const auto built = buildSamplerRenderModel(payload);
    if (!built.built)
    {
        std::string details;
        for (const auto& finding : built.findings)
            details += finding.code + ": " + finding.message + " ";
        throw std::runtime_error("HP-04 model must build: " + details);
    }
    require(built.model != nullptr && built.model->usesContinuousDamper(),
            "HP-04 model must expose continuous damper behavior");
    return { built.model };
}

SamplerVoiceSlotSnapshot findVoice(const SamplerVoicePool& pool,
                                   const std::uint64_t voiceId)
{
    for (std::size_t index = 0; index < SamplerVoicePool::capacity; ++index)
    {
        const auto voice = pool.getSlotSnapshot(index);
        if (voice.voiceId == voiceId) return voice;
    }
    return {};
}

SamplerVoiceSlotSnapshot findReleasingMainVoice(const SamplerVoicePool& pool)
{
    for (std::size_t index = 0; index < SamplerVoicePool::capacity; ++index)
    {
        const auto voice = pool.getSlotSnapshot(index);
        if (voice.state == SamplerVoiceSlotState::releasing && voice.routeIndex == 0)
            return voice;
    }
    return {};
}

void activate(SamplerPlaybackContext& context, const SamplerRenderModelPtr& model)
{
    require(context.prepare(sampleRate) && context.stageActivation(model),
            "HP-04 playback context must prepare and stage");
    StereoOutput output(1);
    const auto result = context.renderBlock(output.view(), noEvents());
    require(result.accepted && result.activationApplied,
            "HP-04 playback context must activate at the block boundary");
}

void verifyRepedalCatchAndDenseBurst(const ModelFixture& fixture)
{
    SamplerPlaybackContext context(PlaybackActivationLane::performance);
    activate(context, fixture.model);

    const std::array attack { controller(0, 127), event(SamplerRenderEventType::noteOn) };
    StereoOutput attackOutput(1);
    require(context.renderBlock(attackOutput.view(), view(attack)).voicePool.render.startedVoiceCount == 1,
            "The source attack must allocate exactly one voice");
    const auto source = findVoice(context.getVoicePool(), 1);
    require(source.state == SamplerVoiceSlotState::active && source.triggerId == 1
                && source.routeIndex == 0,
            "The source voice must retain its physical trigger identity");

    const std::array off { event(SamplerRenderEventType::noteOff) };
    StereoOutput offOutput(1);
    const auto released = context.renderBlock(offOutput.view(), view(off));
    require(released.voicePool.render.releasedVoiceCount == 1
                && released.voicePool.render.startedVoiceCount == 1,
            "One physical key-up must release the source and start one release-trigger sample");
    auto voice = findVoice(context.getVoicePool(), 1);
    require(voice.state == SamplerVoiceSlotState::releasing && voice.triggerId == source.triggerId,
            "Effective release must not replace the source voice or trigger identity");

    StereoOutput decay(64);
    context.renderBlock(decay.view(), noEvents());
    const auto beforeDamp = findVoice(context.getVoicePool(), 1).releaseEnvelopeLevel;
    const std::array damp { controller(0, 32) };
    StereoOutput dampOutput(1);
    const auto damped = context.renderBlock(dampOutput.view(), view(damp));
    require(damped.voicePool.dynamicReleaseUpdateCount == 1
                && damped.voicePool.repedalCatchCount == 0,
            "A falling CC64 value must damp the remaining release without counting a catch");
    requireNear(dampOutput.left.front() - releaseTriggerLevel, beforeDamp,
                "Damping must preserve the exact envelope level at the update sample");

    StereoOutput moreDecay(64);
    context.renderBlock(moreDecay.view(), noEvents());
    const auto beforeCatch = findVoice(context.getVoicePool(), 1).releaseEnvelopeLevel;
    const std::array catchEvent { controller(0, 110) };
    StereoOutput catchOutput(1);
    const auto caught = context.renderBlock(catchOutput.view(), view(catchEvent));
    voice = findVoice(context.getVoicePool(), 1);
    require(caught.voicePool.dynamicReleaseUpdateCount == 1
                && caught.voicePool.repedalCatchCount == 1
                && voice.voiceId == source.voiceId && voice.triggerId == source.triggerId
                && voice.activationGeneration == source.activationGeneration
                && voice.routeIndex == source.routeIndex && voice.repedalCatchCount == 1,
            "Repedaling must catch the same still-audible source voice in place");
    requireNear(catchOutput.left.front() - releaseTriggerLevel, beforeCatch,
                "Repedal catch must preserve the remaining energy without an upward jump");

    const auto beforeBurst = voice.releaseEnvelopeLevel;
    const std::array burst {
        controller(0, 0), controller(0, 127), controller(0, 20), controller(0, 110)
    };
    StereoOutput burstOutput(1);
    allocation_probe::reset();
    allocation_probe::enabled = true;
    const auto burstResult = context.renderBlock(burstOutput.view(), view(burst));
    allocation_probe::enabled = false;
    voice = findVoice(context.getVoicePool(), 1);
    require(burstResult.voicePool.dynamicReleaseUpdateCount == 4
                && burstResult.voicePool.repedalCatchCount == 2
                && burstResult.voicePool.render.startedVoiceCount == 0
                && burstResult.voicePool.render.releasedVoiceCount == 0
                && allocation_probe::allocations == 0 && allocation_probe::deallocations == 0,
            "A same-offset repedal staircase must remain bounded and allocation-free");
    requireNear(burstOutput.left.front() - releaseTriggerLevel, beforeBurst,
                "Multiple same-offset updates must preserve the pre-burst envelope level");
    require(voice.dynamicReleaseUpdateCount == 6 && voice.repedalCatchCount == 3
                && voice.voiceId == source.voiceId && voice.triggerId == source.triggerId,
            "Fast 127-to-0-to-127 repedaling must not change voice identity");

    const auto snapshot = context.getSnapshot();
    require(snapshot.semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOff)] == 1
                && snapshot.semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::release)] == 1
                && snapshot.counters.dynamicReleaseUpdateCount == 6
                && snapshot.counters.repedalCatchCount == 3,
            "Repedal updates must not duplicate physical or effective release semantics");
}

void verifyRepeatedPitchReleaseUniqueness(const ModelFixture& fixture)
{
    SamplerPlaybackContext context(PlaybackActivationLane::performance);
    activate(context, fixture.model);
    for (int cycle = 0; cycle < 2; ++cycle)
    {
        const std::array on { event(SamplerRenderEventType::noteOn) };
        StereoOutput onOutput(1);
        require(context.renderBlock(onOutput.view(), view(on)).voicePool.render.startedVoiceCount == 1,
                "Each repeated-pitch attack must start one source voice");
        const std::array off { event(SamplerRenderEventType::noteOff) };
        StereoOutput offOutput(1);
        const auto result = context.renderBlock(offOutput.view(), view(off));
        require(result.voicePool.render.releasedVoiceCount == 1
                    && result.voicePool.render.startedVoiceCount == 1,
                "Each repeated-pitch key-up must start at most one release-trigger sample");
        const std::array repedal { controller(0, 20), controller(0, 110) };
        StereoOutput repedalOutput(1);
        const auto repedalResult = context.renderBlock(repedalOutput.view(), view(repedal));
        require(repedalResult.voicePool.render.startedVoiceCount == 0
                    && repedalResult.voicePool.render.releasedVoiceCount == 0,
                "Repedaling repeated pitches must not allocate release-trigger samples");
    }
    const auto snapshot = context.getSnapshot();
    require(snapshot.semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::noteOff)] == 2
                && snapshot.semanticEventCounts[static_cast<std::size_t>(PerformanceEventKind::release)] == 2,
            "Two sequential gestures on one pitch must own exactly two physical and effective releases");
}

void verifyFinishedAndStolenVoicesStayGone(const ModelFixture& fixture)
{
    SamplerVoicePool finished;
    require(finished.prepare(*fixture.model, sampleRate, 81), "Finished pool must prepare");
    const std::array start {
        controller(0, 0), event(SamplerRenderEventType::noteOn),
        event(SamplerRenderEventType::noteOff, 1)
    };
    StereoOutput startOutput(2);
    finished.renderBlock(startOutput.view(), view(start));
    const auto finishedVoiceId = findReleasingMainVoice(finished).voiceId;
    StereoOutput finishOutput(1200);
    finished.renderBlock(finishOutput.view(), noEvents());
    require(findVoice(finished, finishedVoiceId).state == SamplerVoiceSlotState::finished,
            "The short CC0 release must reach the finished lifecycle");
    const std::array revive { controller(0, 127) };
    StereoOutput reviveOutput(1);
    const auto reviveResult = finished.renderBlock(reviveOutput.view(), view(revive));
    require(reviveResult.dynamicReleaseUpdateCount == 0 && reviveResult.repedalCatchCount == 0
                && findVoice(finished, finishedVoiceId).state == SamplerVoiceSlotState::finished,
            "Repedaling must not revive a completed voice");

    SamplerVoicePool stealing;
    require(stealing.prepare(*fixture.model, sampleRate, 82), "Stealing pool must prepare");
    SamplerEventBlock fill;
    fill.push(controller(0, 32));
    for (std::size_t index = 0; index < SamplerVoicePool::capacity; ++index)
        fill.push(event(SamplerRenderEventType::noteOn));
    fill.push(event(SamplerRenderEventType::noteOff, 1));
    StereoOutput fillOutput(2);
    const auto filled = stealing.renderBlock(fillOutput.view(), fill.view());
    require(filled.render.startedVoiceCount == SamplerVoicePool::capacity
                && filled.render.releasedVoiceCount == SamplerVoicePool::capacity,
            "Steal fixture must fill the bounded pool with releasing voices");
    const std::array replacement { event(SamplerRenderEventType::noteOn, 0, 62) };
    StereoOutput replacementOutput(1);
    const auto replacementResult = stealing.renderBlock(replacementOutput.view(), view(replacement));
    require(replacementResult.render.stolenVoiceCount == 1
                && replacementResult.render.releasingVoiceStealCount == 1
                && findVoice(stealing, 1).voiceId == 0
                && findVoice(stealing, SamplerVoicePool::capacity + 1).voiceId
                    == SamplerVoicePool::capacity + 1,
            "The oldest releasing voice must be replaced deterministically");
    const std::array catchRemaining { controller(0, 110) };
    StereoOutput catchRemainingOutput(1);
    allocation_probe::reset();
    allocation_probe::enabled = true;
    const auto caught = stealing.renderBlock(catchRemainingOutput.view(), view(catchRemaining));
    allocation_probe::enabled = false;
    require(caught.dynamicReleaseUpdateCount == SamplerVoicePool::capacity - 1
                && caught.repedalCatchCount == SamplerVoicePool::capacity - 1
                && findVoice(stealing, 1).voiceId == 0
                && findVoice(stealing, SamplerVoicePool::capacity + 1).voiceId
                    == SamplerVoicePool::capacity + 1
                && allocation_probe::allocations == 0 && allocation_probe::deallocations == 0,
            "A stolen voice must stay gone while every remaining tail is caught in place");
}

void verifyPanicActivationAndCloseBoundaries(const ModelFixture& first,
                                             const ModelFixture& second)
{
    SamplerVoicePool pool;
    require(pool.prepare(*first.model, sampleRate, 91), "Boundary pool must prepare");
    const std::array allOff {
        controller(0, 32), event(SamplerRenderEventType::noteOn),
        event(SamplerRenderEventType::allNotesOff, 1)
    };
    StereoOutput allOffOutput(2);
    require(pool.renderBlock(allOffOutput.view(), view(allOff)).render.releasedVoiceCount == 1,
            "All-notes-off must enter the existing release lifecycle");
    const std::array catchAllOff { controller(0, 110) };
    StereoOutput catchAllOffOutput(1);
    require(pool.renderBlock(catchAllOffOutput.view(), view(catchAllOff)).repedalCatchCount == 1,
            "An all-notes-off tail may be caught while it remains audible");
    const std::array reset { event(SamplerRenderEventType::reset) };
    StereoOutput resetOutput(1);
    const auto resetResult = pool.renderBlock(resetOutput.view(), view(reset));
    require(resetResult.resetVoiceCount == 1 && resetResult.activeVoiceCount == 0
                && resetResult.releasingVoiceCount == 0,
            "All-sound-off/reset must end a caught tail immediately");
    StereoOutput afterResetOutput(1);
    const auto afterReset = pool.renderBlock(afterResetOutput.view(), view(catchAllOff));
    require(afterReset.dynamicReleaseUpdateCount == 0 && afterReset.repedalCatchCount == 0,
            "A panic-reset voice must never become catchable again");

    SamplerPlaybackContext context(PlaybackActivationLane::performance);
    activate(context, first.model);
    const std::array start {
        controller(0, 20), event(SamplerRenderEventType::noteOn),
        event(SamplerRenderEventType::noteOff, 1)
    };
    StereoOutput startOutput(2);
    context.renderBlock(startOutput.view(), view(start));
    const auto old = findReleasingMainVoice(context.getVoicePool());
    require(context.stageActivation(second.model), "Replacement generation must stage");
    const std::array catchOld { controller(0, 110) };
    StereoOutput replacementOutput(1);
    const auto replacement = context.renderBlock(replacementOutput.view(), view(catchOld));
    const auto retained = findVoice(context.getVoicePool(), old.voiceId);
    require(replacement.activationApplied && replacement.voicePool.repedalCatchCount == 1
                && retained.voiceId == old.voiceId && retained.triggerId == old.triggerId
                && retained.activationGeneration == old.activationGeneration
                && retained.modelRevision == first.model->getRevision(),
            "Project replacement must catch an old-generation tail with its original identity");

    context.resetAtBlockBoundary();
    auto snapshot = context.getSnapshot();
    require(snapshot.activeVoiceCount == 0 && snapshot.releasingVoiceCount == 0,
            "The transport-discontinuity reset boundary must terminate caught voices");
    StereoOutput postTransportOutput(1);
    const auto postTransport = context.renderBlock(postTransportOutput.view(), view(catchOld));
    require(postTransport.voicePool.dynamicReleaseUpdateCount == 0,
            "A transport reset must leave no catchable tail");

    const std::array restart {
        hostCc64(0, 20), event(SamplerRenderEventType::noteOn),
        event(SamplerRenderEventType::noteOff, 1)
    };
    StereoOutput restartOutput(2);
    require(context.renderBlock(restartOutput.view(), view(restart)).voicePool.render.releasedVoiceCount == 1,
            "Transport reset must restore the active continuous-damper program and defaults");
    const std::array restartCatch { hostCc64(0, 110) };
    StereoOutput restartCatchOutput(1);
    require(context.renderBlock(restartCatchOutput.view(), view(restartCatch)).voicePool.repedalCatchCount == 1,
            "Host CC64 repedaling must resume immediately after a transport reset");
    context.closeAtBlockBoundary();
    snapshot = context.getSnapshot();
    require(!snapshot.hasActiveActivation && snapshot.activeVoiceCount == 0
                && snapshot.releasingVoiceCount == 0,
            "Package close must detach the model and terminate every voice");
}

void verifyPreviewPerformanceIsolation()
{
    const auto previewModel = buildModel(103, PlaybackActivationLane::preview);
    const auto performanceModel = buildModel(104, PlaybackActivationLane::performance);
    SamplerPlaybackContext preview(PlaybackActivationLane::preview);
    SamplerPlaybackContext performance(PlaybackActivationLane::performance);
    activate(preview, previewModel.model);
    activate(performance, performanceModel.model);
    const std::array start {
        controller(0, 20), event(SamplerRenderEventType::noteOn),
        event(SamplerRenderEventType::noteOff, 1)
    };
    StereoOutput previewStart(2);
    StereoOutput performanceStart(2);
    preview.renderBlock(previewStart.view(), view(start));
    performance.renderBlock(performanceStart.view(), view(start));
    const std::array catchEvent { controller(0, 110) };
    StereoOutput performanceCatch(1);
    require(performance.renderBlock(performanceCatch.view(), view(catchEvent)).voicePool.repedalCatchCount == 1
                && performance.getSnapshot().counters.repedalCatchCount == 1
                && preview.getSnapshot().counters.repedalCatchCount == 0,
            "Preview and Performance must own independent repedal state and diagnostics");
}
} // namespace

int main()
{
    try
    {
        const auto first = buildModel(101);
        const auto second = buildModel(102);
        verifyRepedalCatchAndDenseBurst(first);
        verifyRepeatedPitchReleaseUniqueness(first);
        verifyFinishedAndStolenVoicesStayGone(first);
        verifyPanicActivationAndCloseBoundaries(first, second);
        verifyPreviewPerformanceIsolation();
        std::cout << "Continuous damper HP-04 repedal lifecycle tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        allocation_probe::enabled = false;
        std::cerr << "Continuous damper HP-04 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
