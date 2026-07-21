#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoicePool.h"

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

void* allocate(std::size_t size)
{
    if (enabled)
        ++allocations;
    if (auto* pointer = std::malloc(size == 0 ? 1 : size))
        return pointer;
    throw std::bad_alloc {};
}

void* allocateAligned(std::size_t size, std::size_t alignment)
{
    if (enabled)
        ++allocations;
#if defined(_MSC_VER)
    if (auto* pointer = _aligned_malloc(size == 0 ? 1 : size, alignment))
        return pointer;
#else
    void* pointer = nullptr;
    if (posix_memalign(&pointer, alignment, size == 0 ? 1 : size) == 0)
        return pointer;
#endif
    throw std::bad_alloc {};
}

void free(void* pointer) noexcept
{
    if (enabled && pointer != nullptr)
        ++deallocations;
    std::free(pointer);
}

void freeAligned(void* pointer) noexcept
{
    if (enabled && pointer != nullptr)
        ++deallocations;
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
void operator delete(void* pointer, std::align_val_t) noexcept { allocation_probe::freeAligned(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { allocation_probe::freeAligned(pointer); }
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
constexpr float tolerance = 1.0e-6f;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireVector(const std::vector<float>& actual,
                   const std::vector<float>& expected,
                   const std::string& message)
{
    require(actual.size() == expected.size(), message + " size mismatch.");
    for (std::size_t index = 0; index < actual.size(); ++index)
        if (std::abs(actual[index] - expected[index]) > tolerance)
            throw std::runtime_error(message + " at frame " + std::to_string(index));
}

drs::engine::SamplerRenderModelPtr buildModel(std::size_t frameCount = 4096,
                                              int keyLow = 0,
                                              int keyHigh = 127,
                                              std::size_t layerCount = 1,
                                              drs::engine::ZoneTriggerMode triggerMode = drs::engine::ZoneTriggerMode::gated)
{
    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 43;
    snapshot.contentDigest = "sprint4-pool-snapshot";

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 4301;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint4-pool-prepared";
    for (std::size_t layerIndex = 0; layerIndex < layerCount; ++layerIndex)
    {
        const auto suffix = std::to_string(layerIndex + 1);
        const auto zoneId = "pool-zone-" + suffix;
        const auto sampleId = "pool-sample-" + suffix;
        const auto streamId = "pool-stream-" + suffix;
        snapshot.zones.push_back({ zoneId,
                                   sampleId,
                                   "Pool Zone " + suffix,
                                   "pool-group",
                                   "sustain",
                                   60,
                                   keyLow,
                                   keyHigh,
                                   1,
                                   127,
                                   0.0,
                                   0.0,
                                   0,
                                   false,
                                   0,
                                   0,
                                   triggerMode });

        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = { std::vector<float>(frameCount, 1.0f) };
        drs::engine::PreparedPlaybackSampleHandle sample;
        sample.sampleSourceId = sampleId;
        sample.streamSampleId = streamId;
        sample.sampleRate = 48000.0;
        sample.frameCount = frameCount;
        sample.channelCount = 1;
        sample.decodedSampleData = std::move(decoded);
        prepared.samples.push_back(std::move(sample));
        prepared.zones.push_back({ zoneId,
                                   sampleId,
                                   streamId,
                                   layerIndex,
                                   layerIndex,
                                   60,
                                   keyLow,
                                   keyHigh,
                                   1,
                                   127,
                                   0.0,
                                   0.0,
                                   0,
                                   false,
                                   0,
                                   0,
                                   triggerMode });
    }

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::preview;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 4302;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = drs::engine::buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr, "Voice-pool fixture model should validate.");
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

drs::engine::SamplerRenderEvent noteOn(std::uint32_t offset, int note = 60, float velocity = 1.0f)
{
    return { drs::engine::SamplerRenderEventType::noteOn,
             offset,
             static_cast<std::uint8_t>(note),
             velocity };
}

drs::engine::SamplerRenderEvent noteOff(std::uint32_t offset, int note = 60)
{
    return { drs::engine::SamplerRenderEventType::noteOff,
             offset,
             static_cast<std::uint8_t>(note),
             0.0f };
}

drs::engine::SamplerRenderEvent command(drs::engine::SamplerRenderEventType type,
                                        std::uint32_t offset)
{
    return { type, offset, 0, 0.0f };
}

bool containsVoiceId(const drs::engine::SamplerVoicePool& pool, std::uint64_t voiceId)
{
    for (std::size_t index = 0; index < drs::engine::SamplerVoicePool::capacity; ++index)
        if (pool.getSlotSnapshot(index).voiceId == voiceId)
            return true;
    return false;
}

void runEventBlockContract()
{
    static_assert(drs::engine::SamplerEventBlock::capacity == 128,
                  "Sprint 4 event capacity changed.");
    static_assert(drs::engine::SamplerVoicePool::capacity == 24,
                  "Sprint 4 per-context voice capacity changed.");

    drs::engine::SamplerEventBlock block;
    require(block.push(noteOn(8, 60))
                && block.push(noteOn(2, 61))
                && block.push(noteOff(8, 60))
                && block.push(noteOn(4, 62)),
            "Bounded event block should accept in-capacity events.");
    const auto view = block.view();
    require(view.size == 4
                && view[0].sampleOffset == 2
                && view[1].sampleOffset == 4
                && view[2].sampleOffset == 8
                && view[2].type == drs::engine::SamplerRenderEventType::noteOn
                && view[3].sampleOffset == 8
                && view[3].type == drs::engine::SamplerRenderEventType::noteOff,
            "Event block must stable-sort offsets while preserving equal-offset insertion order.");

    block.clear();
    for (std::size_t index = 0; index < drs::engine::SamplerEventBlock::capacity; ++index)
        require(block.push(noteOff(static_cast<std::uint32_t>(index % 32))),
                "Event block should accept exactly 128 events.");
    require(!block.push(noteOff(0)) && block.size() == 128 && block.droppedEventCount() == 1,
            "The 129th event must be rejected with a deterministic drop count.");
    block.clear();
    require(block.size() == 0 && block.droppedEventCount() == 0,
            "Event-block clear must restore bounded scratch state.");
}

void runSampleAccurateTimingMatrix()
{
    const auto model = buildModel();
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Voice pool should prepare with a valid model.");
    drs::engine::SamplerEventBlock events;
    events.push(noteOn(3));
    StereoOutput output(8);
    const auto result = pool.renderBlock(output.view(), events.view());
    require(result.accepted
                && result.render.renderedFrameCount == 8
                && result.render.consumedEventCount == 1
                && result.render.startedVoiceCount == 1
                && result.activeVoiceCount == 1,
            "Sample-accurate note-on should start one active voice.");
    requireVector(output.left, { 0.0f, 0.0f, 0.0f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f },
                  "Note-on sample offset changed");
    requireVector(output.right, output.left, "Mono scheduled voice should duplicate to stereo");

    drs::engine::SamplerVoicePool invalidPool;
    require(invalidPool.prepare(*model, 48000.0), "Invalid event matrix pool should prepare.");
    const std::array<drs::engine::SamplerRenderEvent, 2> unsorted { noteOn(4), noteOff(2) };
    StereoOutput invalidOutput(8);
    require(!invalidPool.renderBlock(invalidOutput.view(), { unsorted.data(), unsorted.size() }).accepted
                && invalidPool.activeVoiceCount() == 0,
            "Unsorted raw event views must be rejected without partial state changes.");
    const std::array<drs::engine::SamplerRenderEvent, 1> outside { noteOn(8) };
    require(!invalidPool.renderBlock(invalidOutput.view(), { outside.data(), outside.size() }).accepted,
            "Event offsets at or beyond the block end must be rejected.");
}

void runNoteOwnershipAndCommandMatrix()
{
    const auto model = buildModel();
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Ownership matrix pool should prepare.");

    drs::engine::SamplerEventBlock sameOffset;
    sameOffset.push(noteOn(0));
    sameOffset.push(noteOff(0));
    StereoOutput firstOutput(4);
    auto result = pool.renderBlock(firstOutput.view(), sameOffset.view());
    require(result.activeVoiceCount == 0 && result.releasingVoiceCount == 1
                && result.render.releasedVoiceCount == 1,
            "Equal-offset note-on then note-off must preserve insertion order.");

    pool.resetVoices();
    sameOffset.clear();
    sameOffset.push(noteOff(0));
    sameOffset.push(noteOn(0));
    StereoOutput secondOutput(4);
    result = pool.renderBlock(secondOutput.view(), sameOffset.view());
    require(result.activeVoiceCount == 1 && result.releasingVoiceCount == 0,
            "Equal-offset note-off then note-on must preserve insertion order.");

    pool.resetVoices();
    drs::engine::SamplerEventBlock repeated;
    repeated.push(noteOn(0, 60));
    repeated.push(noteOn(2, 60));
    repeated.push(noteOff(4, 60));
    StereoOutput repeatedOutput(6);
    result = pool.renderBlock(repeatedOutput.view(), repeated.view());
    require(result.render.startedVoiceCount == 2
                && result.render.releasedVoiceCount == 2
                && result.activeVoiceCount == 0
                && result.releasingVoiceCount == 2,
            "Repeated-note ownership must release every matching active voice.");
    requireVector(repeatedOutput.left,
                  { 0.25f, 0.25f, 0.5f, 0.5f, 0.5f,
                    0.5f * 2047.0f / 2048.0f },
                  "Repeated-note scheduling changed");

    pool.resetVoices();
    drs::engine::SamplerEventBlock allOff;
    allOff.push(noteOn(0, 60));
    allOff.push(noteOn(0, 64));
    allOff.push(command(drs::engine::SamplerRenderEventType::allNotesOff, 2));
    StereoOutput allOffOutput(4);
    result = pool.renderBlock(allOffOutput.view(), allOff.view());
    require(result.render.releasedVoiceCount == 2 && result.releasingVoiceCount == 2,
            "Ordinary all-notes-off must release every active voice.");

    pool.resetVoices();
    drs::engine::SamplerEventBlock reset;
    reset.push(noteOn(0));
    reset.push(command(drs::engine::SamplerRenderEventType::reset, 4));
    StereoOutput resetOutput(8);
    result = pool.renderBlock(resetOutput.view(), reset.view());
    require(result.resetVoiceCount == 1
                && result.activeVoiceCount == 0
                && result.releasingVoiceCount == 0,
            "Emergency reset must immediately return all slots to free.");
    requireVector(resetOutput.left, { 0.25f, 0.25f, 0.25f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f },
                  "Emergency reset sample boundary changed");
}

void runOverlappingLayerMatrix()
{
    const auto model = buildModel(4096, 48, 72, 2);
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Overlapping-layer pool should prepare.");

    drs::engine::SamplerEventBlock events;
    events.push(noteOn(0, 60));
    StereoOutput output(4);
    auto result = pool.renderBlock(output.view(), events.view());
    require(result.render.startedVoiceCount == 2 && result.activeVoiceCount == 2,
            "One note must start every sample whose key and velocity ranges overlap the trigger.");
    requireVector(output.left, { 0.5f, 0.5f, 0.5f, 0.5f },
                  "Overlapping sample layers should be mixed together");

    events.clear();
    events.push(noteOff(0, 60));
    StereoOutput releaseOutput(1);
    result = pool.renderBlock(releaseOutput.view(), events.view());
    require(result.render.releasedVoiceCount == 2 && result.releasingVoiceCount == 2,
            "Note-off must release every voice started by an overlapping layered trigger.");
}

void runTriggerModeMatrix()
{
    const auto model = buildModel(8, 0, 127, 1, drs::engine::ZoneTriggerMode::oneShot);
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "One-shot voice pool should prepare.");

    drs::engine::SamplerEventBlock events;
    events.push(noteOn(0));
    events.push(noteOff(2));
    StereoOutput firstOutput(4);
    auto result = pool.renderBlock(firstOutput.view(), events.view());
    require(result.render.startedVoiceCount == 1
                && result.render.releasedVoiceCount == 0
                && result.activeVoiceCount == 1
                && result.releasingVoiceCount == 0,
            "One-shot voices must ignore matching note-off events.");
    requireVector(firstOutput.left, { 0.25f, 0.25f, 0.25f, 0.25f },
                  "One-shot playback should continue after note-off");

    events.clear();
    StereoOutput naturalEndOutput(4);
    result = pool.renderBlock(naturalEndOutput.view(), events.view());
    require(result.render.completedVoiceCount == 1 && result.finishedVoiceCount == 1,
            "One-shot voices must still stop at the sample's natural end.");

    pool.resetVoices();
    events.push(noteOn(0));
    events.push(command(drs::engine::SamplerRenderEventType::allNotesOff, 1));
    StereoOutput allOffOutput(2);
    result = pool.renderBlock(allOffOutput.view(), events.view());
    require(result.render.releasedVoiceCount == 1 && result.releasingVoiceCount == 1,
            "All-notes-off must remain able to release one-shot voices.");
}

void runCapacityAndStealMatrix()
{
    const auto model = buildModel();
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Capacity matrix pool should prepare.");
    drs::engine::SamplerEventBlock events;
    for (std::size_t index = 0; index < drs::engine::SamplerVoicePool::capacity; ++index)
        events.push(noteOn(0, 60));
    StereoOutput output(1);
    auto result = pool.renderBlock(output.view(), events.view());
    require(result.render.startedVoiceCount == 24
                && result.render.stolenVoiceCount == 0
                && result.activeVoiceCount == 24,
            "The first 24 notes must fill the fixed pool without stealing.");

    events.clear();
    events.push(noteOn(0, 61));
    StereoOutput stealOutput(1);
    result = pool.renderBlock(stealOutput.view(), events.view());
    require(result.render.stolenVoiceCount == 1
                && result.activeVoiceCount == 24
                && !containsVoiceId(pool, 1)
                && containsVoiceId(pool, 25),
            "The 25th note must deterministically steal the oldest active voice.");

    pool.resetVoices();
    events.clear();
    for (std::size_t index = 0; index < drs::engine::SamplerVoicePool::capacity; ++index)
        events.push(noteOn(0, 60));
    StereoOutput refillOutput(1);
    pool.renderBlock(refillOutput.view(), events.view());
    events.clear();
    events.push(noteOff(0, 60));
    events.push(noteOn(0, 62));
    StereoOutput releaseStealOutput(1);
    result = pool.renderBlock(releaseStealOutput.view(), events.view());
    require(result.render.releasedVoiceCount == 24
                && result.render.stolenVoiceCount == 1
                && result.activeVoiceCount == 1
                && result.releasingVoiceCount == 23,
            "A new note must steal the oldest releasing voice before any active voice.");

    const auto oneFrameModel = buildModel(1);
    drs::engine::SamplerVoicePool finishedPool;
    require(finishedPool.prepare(*oneFrameModel, 48000.0), "Finished-slot pool should prepare.");
    events.clear();
    events.push(noteOn(0));
    StereoOutput oneFrameOutput(1);
    result = finishedPool.renderBlock(oneFrameOutput.view(), events.view());
    require(result.finishedVoiceCount == 1, "One-frame voice should leave an explicit finished slot.");
    events.clear();
    events.push(noteOn(0));
    StereoOutput reuseOutput(1);
    result = finishedPool.renderBlock(reuseOutput.view(), events.view());
    require(result.render.stolenVoiceCount == 0 && result.finishedVoiceCount == 1,
            "Finished slot reuse must not count as stealing.");
}

void runOverflowDropAndRealtimeMatrix()
{
    const auto model = buildModel(4096, 48, 72);
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Overflow matrix pool should prepare.");
    drs::engine::SamplerEventBlock events;

    allocation_probe::reset();
    allocation_probe::enabled = true;
    for (std::size_t index = 0; index < drs::engine::SamplerEventBlock::capacity; ++index)
        events.push(noteOn(static_cast<std::uint32_t>(index % 32), 60));
    const auto overflowAccepted = events.push(noteOn(0, 60));
    allocation_probe::enabled = false;
    require(!overflowAccepted && events.droppedEventCount() == 1
                && allocation_probe::allocations == 0 && allocation_probe::deallocations == 0,
            "Bounded event admission must not allocate, release, or grow at 128/129 pressure.");

    StereoOutput output(64);
    allocation_probe::reset();
    allocation_probe::enabled = true;
    const auto result = pool.renderBlock(output.view(), events.view());
    allocation_probe::enabled = false;
    require(result.accepted
                && result.render.consumedEventCount == 128
                && result.activeVoiceCount == 24
                && allocation_probe::allocations == 0
                && allocation_probe::deallocations == 0,
            "Maximum bounded scheduling/render must perform no allocation, release, or capacity growth.");

    drs::engine::SamplerEventBlock invalid;
    invalid.push(noteOn(0, 127));
    invalid.push(noteOn(1, 60, 0.0f));
    StereoOutput invalidOutput(4);
    const auto invalidResult = pool.renderBlock(invalidOutput.view(), invalid.view());
    require(invalidResult.render.droppedEventCount == 2,
            "No-route and invalid-velocity notes must report deterministic drops.");
}
} // namespace

int main()
{
    try
    {
        runEventBlockContract();
        runSampleAccurateTimingMatrix();
        runNoteOwnershipAndCommandMatrix();
        runOverlappingLayerMatrix();
        runTriggerModeMatrix();
        runCapacityAndStealMatrix();
        runOverflowDropAndRealtimeMatrix();
        std::cout << "Sprint 4.3 fixed voice-pool and sample-accurate event matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
