#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoice.h"
#include "drs/engine/SamplerVoicePool.h"

#include <array>
#include <cmath>
#include <cstddef>
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
        throw std::runtime_error(message + " (actual=" + std::to_string(actual)
                                 + ", expected=" + std::to_string(expected) + ")");
}

void requireVector(const std::vector<float>& actual,
                   const std::vector<float>& expected,
                   const std::string& message)
{
    require(actual.size() == expected.size(), message + " size mismatch.");
    for (std::size_t index = 0; index < actual.size(); ++index)
        requireNear(actual[index], expected[index], message + " at frame " + std::to_string(index));
}

struct ModelOptions
{
    double sourceSampleRate = 48000.0;
    int rootKey = 60;
    std::uint64_t sampleStartFrame = 0;
    std::uint64_t sampleEndFrame = 0;
    bool loopEnabled = false;
    drs::engine::RegionLoopMode loopMode = drs::engine::RegionLoopMode::noLoop;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    double releaseSeconds = 0.0;
    double releaseShape = 0.0;
};

drs::engine::SamplerRenderModelPtr buildModel(std::vector<float> source,
                                              const ModelOptions& options = {})
{
    require(!source.empty(), "Lifecycle fixture requires source PCM.");
    const auto frameCount = source.size();

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 44;
    snapshot.contentDigest = "sprint4-lifecycle-snapshot";
    drs::engine::PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "lifecycle-zone";
    snapshotZone.sampleSourceId = "lifecycle-sample";
    snapshotZone.displayName = "Lifecycle Zone";
    snapshotZone.groupId = "lifecycle-group";
    snapshotZone.articulationId = "sustain";
    snapshotZone.rootKey = options.rootKey;
    snapshotZone.sampleStartFrame = options.sampleStartFrame;
    snapshotZone.sampleEndFrame = options.sampleEndFrame;
    snapshotZone.loopEnabled = options.loopEnabled;
    snapshotZone.loopMode = options.loopMode;
    snapshotZone.loopStartFrame = options.loopStartFrame;
    snapshotZone.loopEndFrame = options.loopEndFrame;
    snapshotZone.releaseSeconds = options.releaseSeconds;
    snapshotZone.releaseShape = options.releaseShape;
    snapshot.zones.push_back(std::move(snapshotZone));
    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "lifecycle-group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { "lifecycle-zone" };
    snapshotGroup.displayName = "Lifecycle Group";
    snapshot.groupRoutes.push_back(std::move(snapshotGroup));

    auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = { std::move(source) };
    drs::engine::PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "lifecycle-sample";
    sample.streamSampleId = "lifecycle-stream";
    sample.sampleRate = options.sourceSampleRate;
    sample.frameCount = frameCount;
    sample.channelCount = 1;
    sample.decodedSampleData = std::move(decoded);

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 4401;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint4-lifecycle-prepared";
    prepared.samples.push_back(std::move(sample));
    drs::engine::PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "lifecycle-zone";
    preparedZone.sampleSourceId = "lifecycle-sample";
    preparedZone.streamSampleId = "lifecycle-stream";
    preparedZone.rootKey = options.rootKey;
    preparedZone.sampleStartFrame = options.sampleStartFrame;
    preparedZone.sampleEndFrame = options.sampleEndFrame;
    preparedZone.loopEnabled = options.loopEnabled;
    preparedZone.loopMode = options.loopMode;
    preparedZone.loopStartFrame = options.loopStartFrame;
    preparedZone.loopEndFrame = options.loopEndFrame;
    preparedZone.releaseSeconds = options.releaseSeconds;
    preparedZone.releaseShape = options.releaseShape;
    prepared.zones.push_back(std::move(preparedZone));
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "lifecycle-group";
    preparedGroup.articulationIds = { "sustain" };
    preparedGroup.zoneIds = { "lifecycle-zone" };
    preparedGroup.displayName = "Lifecycle Group";
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::preview;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 4402;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));
    const auto result = drs::engine::buildSamplerRenderModel(payload);
    std::string findingSummary;
    for (const auto& finding : result.findings)
        findingSummary += " [" + finding.code + "] " + finding.message;
    require(result.built && result.model != nullptr,
            "Lifecycle fixture model should validate." + findingSummary);
    return result.model;
}

drs::engine::SamplerVoiceStartRequest startRequest(int note = 60,
                                                    double outputSampleRate = 48000.0,
                                                    std::uint64_t voiceId = 1)
{
    drs::engine::SamplerVoiceStartRequest request;
    request.voiceId = voiceId;
    request.routeIndex = 0;
    request.sourceMidiNote = note;
    request.effectiveMidiNote = note;
    request.effectiveVelocity = 127;
    request.outputSampleRate = outputSampleRate;
    return request;
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

drs::engine::SamplerRenderEvent noteOn(std::uint32_t offset, int note = 60)
{
    return { drs::engine::SamplerRenderEventType::noteOn,
             offset,
             static_cast<std::uint8_t>(note),
             1.0f };
}

drs::engine::SamplerRenderEvent noteOff(std::uint32_t offset, int note = 60)
{
    return { drs::engine::SamplerRenderEventType::noteOff,
             offset,
             static_cast<std::uint8_t>(note),
             0.0f };
}

drs::engine::SamplerRenderEvent resetEvent(std::uint32_t offset)
{
    return { drs::engine::SamplerRenderEventType::reset, offset, 0, 0.0f };
}

void runLoopBoundaryMatrix()
{
    ModelOptions loop;
    loop.loopEnabled = true;
    loop.loopStartFrame = 2;
    loop.loopEndFrame = 5;
    const auto model = buildModel({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }, loop);
    drs::engine::SamplerVoice voice;
    require(voice.start(*model, startRequest()), "Unity loop voice should start.");
    StereoOutput output(10);
    const auto result = voice.render(output.view(), 0, 10);
    require(result.mixedFrameCount == 10 && !result.voiceFinished && voice.isActive(),
            "Enabled loop must remain active after repeated wraps.");
    requireVector(output.left,
                  { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 2.0f, 3.0f, 4.0f, 2.0f, 3.0f },
                  "Unity loop sequence changed");

    loop.sampleStartFrame = 2;
    const auto fractionalModel = buildModel({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }, loop);
    drs::engine::SamplerVoice fractional;
    require(fractional.start(*fractionalModel, startRequest(60, 96000.0)),
            "Fractional loop voice should start inside the loop.");
    StereoOutput fractionalOutput(6);
    fractional.render(fractionalOutput.view(), 0, 6);
    requireVector(fractionalOutput.left,
                  { 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 3.0f },
                  "Fractional loop-boundary interpolation changed");

    ModelOptions multipleWraps;
    multipleWraps.sampleStartFrame = 1;
    multipleWraps.loopEnabled = true;
    multipleWraps.loopStartFrame = 1;
    multipleWraps.loopEndFrame = 3;
    const auto multipleModel = buildModel({ 0.0f, 1.0f, 2.0f, 3.0f }, multipleWraps);
    drs::engine::SamplerVoice multiple;
    require(multiple.start(*multipleModel, startRequest(84)), "Multiple-wrap voice should start.");
    StereoOutput multipleOutput(4);
    multiple.render(multipleOutput.view(), 0, 4);
    requireVector(multipleOutput.left, { 1.0f, 1.0f, 1.0f, 1.0f },
                  "Multiple wraps in one increment changed");

    ModelOptions shortLoop;
    shortLoop.sampleStartFrame = 2;
    shortLoop.loopEnabled = true;
    shortLoop.loopStartFrame = 2;
    shortLoop.loopEndFrame = 3;
    const auto shortModel = buildModel({ 0.0f, 1.0f, 2.0f, 3.0f }, shortLoop);
    drs::engine::SamplerVoice shortVoice;
    require(shortVoice.start(*shortModel, startRequest()), "One-frame loop should start.");
    StereoOutput shortOutput(4);
    shortVoice.render(shortOutput.view(), 0, 4);
    requireVector(shortOutput.left, { 2.0f, 2.0f, 2.0f, 2.0f },
                  "One-frame loop traversal changed");

    ModelOptions afterLoop;
    afterLoop.sampleStartFrame = 5;
    afterLoop.loopEnabled = false;
    afterLoop.loopStartFrame = 1;
    afterLoop.loopEndFrame = 4;
    const auto afterModel = buildModel({ 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f }, afterLoop);
    drs::engine::SamplerVoice afterVoice;
    require(afterVoice.start(*afterModel, startRequest()), "Natural-tail start voice should start.");
    StereoOutput afterOutput(4);
    const auto afterResult = afterVoice.render(afterOutput.view(), 0, 4);
    require(afterResult.mixedFrameCount == 3 && afterResult.voiceFinished,
            "A non-looping playback region must play its natural tail without wrapping.");
    requireVector(afterOutput.left, { 5.0f, 6.0f, 7.0f, 0.0f },
                  "Post-loop natural tail changed");
}

void runPlaybackRegionMatrix()
{
    ModelOptions bounded;
    bounded.sampleEndFrame = 3;
    const auto boundedModel = buildModel({ 0.0f, 1.0f, 2.0f, 90.0f, 100.0f }, bounded);
    drs::engine::SamplerVoice boundedVoice;
    require(boundedVoice.start(*boundedModel, startRequest()),
            "A non-empty authored playback region should start.");
    StereoOutput boundedOutput(5);
    const auto boundedResult = boundedVoice.render(boundedOutput.view(), 0, 5);
    require(boundedResult.mixedFrameCount == 3 && boundedResult.voiceFinished,
            "A non-looping voice must finish at the authored exclusive end.");
    requireVector(boundedOutput.left, { 0.0f, 1.0f, 2.0f, 0.0f, 0.0f },
                  "Authored playback end changed");

    ModelOptions oneFrame;
    oneFrame.sampleEndFrame = 1;
    const auto oneFrameModel = buildModel({ 7.0f, 80.0f }, oneFrame);
    drs::engine::SamplerVoice oneFrameVoice;
    require(oneFrameVoice.start(*oneFrameModel, startRequest()),
            "The maximum-short legal playback region should start.");
    StereoOutput oneFrameOutput(2);
    const auto oneFrameResult = oneFrameVoice.render(oneFrameOutput.view(), 0, 2);
    require(oneFrameResult.mixedFrameCount == 1 && oneFrameResult.voiceFinished,
            "An exclusive end of one must render exactly one frame.");
    requireVector(oneFrameOutput.left, { 7.0f, 0.0f }, "One-frame playback region changed");

    ModelOptions interpolated;
    interpolated.sampleEndFrame = 2;
    const auto interpolatedModel = buildModel({ 1.0f, 1.0f, 100.0f }, interpolated);
    drs::engine::SamplerVoice interpolatedVoice;
    require(interpolatedVoice.start(*interpolatedModel, startRequest(60, 96000.0)),
            "Fractional bounded voice should start.");
    StereoOutput interpolatedOutput(5);
    const auto interpolatedResult = interpolatedVoice.render(interpolatedOutput.view(), 0, 5);
    require(interpolatedResult.mixedFrameCount == 4 && interpolatedResult.voiceFinished,
            "Fractional playback must finish at the authored boundary.");
    requireVector(interpolatedOutput.left, { 1.0f, 1.0f, 1.0f, 1.0f, 0.0f },
                  "Interpolation read beyond the authored playback end");

    ModelOptions containedLoop;
    containedLoop.sampleStartFrame = 1;
    containedLoop.sampleEndFrame = 5;
    containedLoop.loopEnabled = true;
    containedLoop.loopStartFrame = 2;
    containedLoop.loopEndFrame = 4;
    const auto containedLoopModel = buildModel(
        { 0.0f, 1.0f, 2.0f, 3.0f, 90.0f, 100.0f }, containedLoop);
    drs::engine::SamplerVoice containedLoopVoice;
    require(containedLoopVoice.start(*containedLoopModel, startRequest()),
            "A loop contained by the playback region should start.");
    StereoOutput containedLoopOutput(7);
    containedLoopVoice.render(containedLoopOutput.view(), 0, 7);
    requireVector(containedLoopOutput.left, { 1.0f, 2.0f, 3.0f, 2.0f, 3.0f, 2.0f, 3.0f },
                  "Contained playback-region loop changed");

    auto invalidLoop = containedLoop;
    invalidLoop.loopEndFrame = 6;
    bool rejectedInvalidLoop = false;
    try
    {
        static_cast<void>(buildModel(
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }, invalidLoop));
    }
    catch (const std::runtime_error& error)
    {
        rejectedInvalidLoop = std::string(error.what()).find("render-model-loop-range-invalid")
            != std::string::npos;
    }
    require(rejectedInvalidLoop,
            "A loop outside the authored playback region must be rejected before render.");

    ModelOptions beyondPhysicalEnd;
    beyondPhysicalEnd.sampleEndFrame = 7;
    bool rejectedBeyondPhysicalEnd = false;
    try
    {
        static_cast<void>(buildModel(
            { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }, beyondPhysicalEnd));
    }
    catch (const std::runtime_error& error)
    {
        rejectedBeyondPhysicalEnd = std::string(error.what()).find(
            "render-model-end-frame-invalid") != std::string::npos;
    }
    require(rejectedBeyondPhysicalEnd,
            "An authored playback end beyond the physical source must be rejected before render.");

    const auto auditionModel = buildModel(
        { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f }, {});
    auto auditionStart = startRequest();
    auditionStart.hasPlaybackRegionOverride = true;
    auditionStart.playbackStartFrameOverride = 2;
    auditionStart.playbackEndFrameExclusiveOverride = 5;
    drs::engine::SamplerVoice auditionVoice;
    require(auditionVoice.start(*auditionModel, auditionStart),
            "A valid temporary audition range should start without changing the render model.");
    StereoOutput auditionOutput(5);
    const auto auditionResult = auditionVoice.render(auditionOutput.view(), 0, 5);
    require(auditionResult.mixedFrameCount == 3 && auditionResult.voiceFinished,
            "Temporary selection audition must finish at its exclusive event boundary.");
    requireVector(auditionOutput.left, { 2.0f, 3.0f, 4.0f, 0.0f, 0.0f },
                  "Temporary audition range changed");

    auditionStart.playbackStartFrameOverride = 1;
    auditionStart.playbackEndFrameExclusiveOverride = 5;
    auditionStart.loopOverrideEnabled = true;
    auditionStart.loopStartFrameOverride = 2;
    auditionStart.loopEndFrameExclusiveOverride = 4;
    drs::engine::SamplerVoice loopAuditionVoice;
    require(loopAuditionVoice.start(*auditionModel, auditionStart),
            "A contained temporary loop audition should start.");
    StereoOutput loopAuditionOutput(7);
    loopAuditionVoice.render(loopAuditionOutput.view(), 0, 7);
    requireVector(loopAuditionOutput.left, { 1.0f, 2.0f, 3.0f, 2.0f, 3.0f, 2.0f, 3.0f },
                  "Temporary loop audition changed");
}

void runReleaseLawMatrix()
{
    ModelOptions loop;
    loop.loopEnabled = true;
    loop.loopStartFrame = 0;
    loop.loopEndFrame = 16;
    const auto model = buildModel(std::vector<float>(16, 1.0f), loop);
    drs::engine::SamplerVoice voice;
    require(voice.start(*model, startRequest()) && voice.beginRelease(),
            "Looping voice should enter compatibility release.");
    require(voice.isReleasing()
                && voice.getReleaseSamplesTotal() == 2048
                && voice.getReleaseSamplesRemaining() == 2048,
            "Compatibility release length changed.");
    StereoOutput output(drs::engine::SamplerVoice::compatibilityReleaseSampleCount);
    const auto result = voice.render(output.view(), 0,
                                     drs::engine::SamplerVoice::compatibilityReleaseSampleCount);
    require(result.mixedFrameCount == 2048 && result.voiceFinished
                && voice.getReleaseSamplesRemaining() == 0,
            "Release must finish after exactly 2,048 rendered samples.");
    requireNear(output.left.front(), 1.0f, "First release sample must remain unity.");
    requireNear(output.left[1], 2047.0f / 2048.0f,
                "Second release sample changed.");
    requireNear(output.left.back(), 1.0f / 2048.0f,
                "Final release sample changed.");

    drs::engine::SamplerVoice idempotent;
    require(idempotent.start(*model, startRequest()) && idempotent.beginRelease(),
            "Idempotent release voice should start.");
    StereoOutput prefix(10);
    idempotent.render(prefix.view(), 0, 10);
    require(idempotent.getReleaseSamplesRemaining() == 2038 && !idempotent.beginRelease()
                && idempotent.getReleaseSamplesRemaining() == 2038,
            "Repeated note-off must not restart the release envelope.");

    ModelOptions natural = loop;
    natural.sourceSampleRate = 8.0;
    natural.releaseSeconds = 1.0;
    natural.releaseShape = drs::engine::sfzDefaultReleaseShape;
    const auto naturalModel = buildModel(std::vector<float>(16, 1.0f), natural);
    drs::engine::SamplerVoice naturalVoice;
    require(naturalVoice.start(*naturalModel, startRequest(60, 8.0))
                && naturalVoice.beginRelease(),
            "Natural release voice should start.");
    StereoOutput naturalOutput(5);
    naturalVoice.render(naturalOutput.view(), 0, 5);
    require(naturalOutput.left[4] < 0.01f,
            "The SFZ natural curve should decay well below a linear envelope by its midpoint.");

    ModelOptions slow = natural;
    slow.releaseShape = drs::engine::slowReleaseShape;
    const auto slowModel = buildModel(std::vector<float>(16, 1.0f), slow);
    drs::engine::SamplerVoice slowVoice;
    require(slowVoice.start(*slowModel, startRequest(60, 8.0))
                && slowVoice.beginRelease(),
            "Slow release voice should start.");
    StereoOutput slowOutput(5);
    slowVoice.render(slowOutput.view(), 0, 5);
    require(slowOutput.left[4] > 0.9f,
            "The positive slow curve should retain more level than a linear envelope at its midpoint.");
}

void runTypedLoopModeMatrix()
{
    ModelOptions continuous;
    continuous.sampleStartFrame = 3;
    continuous.loopEnabled = true;
    continuous.loopMode = drs::engine::RegionLoopMode::loopContinuous;
    continuous.loopStartFrame = 2;
    continuous.loopEndFrame = 5;
    const auto continuousModel = buildModel(
        { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f }, continuous);
    drs::engine::SamplerVoice continuousVoice;
    require(continuousVoice.start(*continuousModel, startRequest())
                && continuousVoice.isLoopActive()
                && continuousVoice.beginRelease()
                && continuousVoice.isLoopActive(),
            "loop_continuous must keep wrapping after note-off while its release runs.");
    StereoOutput continuousOutput(16);
    const auto continuousResult = continuousVoice.render(continuousOutput.view(), 0, 16);
    require(!continuousResult.voiceFinished && continuousVoice.isReleasing(),
            "loop_continuous should not run into the natural sample end during release.");

    auto sustain = continuous;
    sustain.loopMode = drs::engine::RegionLoopMode::loopSustain;
    const auto sustainModel = buildModel(
        { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f }, sustain);
    drs::engine::SamplerVoice sustainVoice;
    require(sustainVoice.start(*sustainModel, startRequest())
                && sustainVoice.isLoopActive()
                && sustainVoice.beginRelease()
                && !sustainVoice.isLoopActive(),
            "loop_sustain must stop wrapping at note-off so playback can enter the tail.");
    StereoOutput sustainOutput(16);
    const auto sustainResult = sustainVoice.render(sustainOutput.view(), 0, 16);
    require(sustainResult.voiceFinished,
            "loop_sustain should reach the natural source end after leaving its loop.");

    ModelOptions oneShot;
    oneShot.loopMode = drs::engine::RegionLoopMode::oneShot;
    const auto oneShotModel = buildModel({ 1.0f, 1.0f, 1.0f, 1.0f }, oneShot);
    drs::engine::SamplerVoice oneShotVoice;
    require(oneShotVoice.start(*oneShotModel, startRequest())
                && oneShotVoice.ignoresNoteOff()
                && !oneShotVoice.isLoopActive(),
            "one_shot must ignore note-off without enabling a repeating loop.");

    ModelOptions noLoop;
    const auto noLoopModel = buildModel({ 1.0f, 1.0f, 1.0f, 1.0f }, noLoop);
    drs::engine::SamplerVoice noLoopVoice;
    require(noLoopVoice.start(*noLoopModel, startRequest())
                && !noLoopVoice.ignoresNoteOff()
                && !noLoopVoice.isLoopActive(),
            "no_loop must retain ordinary gated note-off behavior.");
}

void runReleasePartitionInvariance()
{
    ModelOptions loop;
    loop.loopEnabled = true;
    loop.loopStartFrame = 1;
    loop.loopEndFrame = 5;
    const auto model = buildModel({ 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }, loop);
    drs::engine::SamplerVoice contiguous;
    drs::engine::SamplerVoice partitioned;
    require(contiguous.start(*model, startRequest(61))
                && partitioned.start(*model, startRequest(61))
                && contiguous.beginRelease() && partitioned.beginRelease(),
            "Partition release voices should start.");

    StereoOutput contiguousOutput(2048);
    StereoOutput partitionedOutput(2048);
    contiguous.render(contiguousOutput.view(), 0, 2048);
    partitioned.render(partitionedOutput.view(), 0, 1);
    partitioned.render(partitionedOutput.view(), 1, 31);
    partitioned.render(partitionedOutput.view(), 32, 256);
    partitioned.render(partitionedOutput.view(), 288, 512);
    partitioned.render(partitionedOutput.view(), 800, 1248);

    requireVector(partitionedOutput.left, contiguousOutput.left,
                  "Release/loop output changed across block partitions");
    require(partitioned.getLifecycleState() == contiguous.getLifecycleState()
                && partitioned.getReleaseSamplesRemaining() == 0
                && contiguous.getReleaseSamplesRemaining() == 0,
            "Release lifecycle changed across block partitions.");
}

void runPoolLifecycleMatrix()
{
    ModelOptions loop;
    loop.loopEnabled = true;
    loop.loopStartFrame = 0;
    loop.loopEndFrame = 8;
    const auto loopModel = buildModel(std::vector<float>(8, 1.0f), loop);
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*loopModel, 48000.0), "Lifecycle pool should prepare.");
    drs::engine::SamplerEventBlock events;
    events.push(noteOn(1));
    events.push(noteOff(3));
    StereoOutput output(6);
    auto result = pool.renderBlock(output.view(), events.view());
    require(result.render.releasedVoiceCount == 1 && result.releasingVoiceCount == 1,
            "Note-off must enter release at its exact event boundary.");
    requireVector(output.left,
                  { 0.0f, 1.0f, 1.0f, 1.0f,
                    2047.0f / 2048.0f,
                    2046.0f / 2048.0f },
                  "Pool release boundary changed");

    events.clear();
    StereoOutput tail(2045);
    result = pool.renderBlock(tail.view(), events.view());
    require(result.render.completedVoiceCount == 1
                && result.finishedVoiceCount == 1
                && result.releasingVoiceCount == 0,
            "Release completion must reclaim the slot into explicit finished state.");

    const auto naturalModel = buildModel({ 1.0f, 1.0f, 1.0f });
    drs::engine::SamplerVoicePool naturalPool;
    require(naturalPool.prepare(*naturalModel, 48000.0), "Natural-end pool should prepare.");
    events.push(noteOn(0));
    StereoOutput naturalOutput(4);
    result = naturalPool.renderBlock(naturalOutput.view(), events.view());
    require(result.render.completedVoiceCount == 1 && result.finishedVoiceCount == 1,
            "Natural sample end must produce one completed finished slot.");
    requireVector(naturalOutput.left, { 1.0f, 1.0f, 1.0f, 0.0f },
                  "Natural sample completion changed");
}

void runStealAndRepeatedResetMatrix()
{
    ModelOptions loop;
    loop.loopEnabled = true;
    loop.loopStartFrame = 0;
    loop.loopEndFrame = 32;
    const auto model = buildModel(std::vector<float>(32, 1.0f), loop);
    const auto modelUseCount = model.use_count();
    drs::engine::SamplerVoicePool pool;
    require(pool.prepare(*model, 48000.0), "Steal/reset pool should prepare.");
    drs::engine::SamplerEventBlock events;
    for (std::size_t index = 0; index < drs::engine::SamplerVoicePool::capacity; ++index)
        events.push(noteOn(0, 60));
    StereoOutput fillOutput(1);
    pool.renderBlock(fillOutput.view(), events.view());

    events.clear();
    events.push(noteOff(0, 60));
    events.push(noteOn(0, 62));
    StereoOutput stealOutput(1);
    auto result = pool.renderBlock(stealOutput.view(), events.view());
    require(result.render.releasedVoiceCount == drs::engine::SamplerVoicePool::capacity
                && result.render.stolenVoiceCount == 1
                && result.activeVoiceCount == 1
                && result.releasingVoiceCount
                    == drs::engine::SamplerVoicePool::capacity - 1,
            "Stealing during release must choose one oldest releasing voice.");

    events.clear();
    events.push(resetEvent(2));
    events.push(resetEvent(2));
    StereoOutput resetOutput(5);
    result = pool.renderBlock(resetOutput.view(), events.view());
    require(result.resetVoiceCount == drs::engine::SamplerVoicePool::capacity
                && result.activeVoiceCount == 0
                && result.releasingVoiceCount == 0
                && pool.finishedVoiceCount() == 0,
            "Repeated emergency reset must be idempotent after the first clear.");
    require(model.use_count() == modelUseCount,
            "Steal/reset must not copy or release immutable model ownership on audio.");

    events.clear();
    StereoOutput silentOutput(4);
    result = pool.renderBlock(silentOutput.view(), events.view());
    require(result.accepted && result.render.completedVoiceCount == 0,
            "Post-reset empty render should remain stable.");
    requireVector(silentOutput.left, { 0.0f, 0.0f, 0.0f, 0.0f },
                  "Post-reset output must remain silent");
}
} // namespace

int main()
{
    try
    {
        runPlaybackRegionMatrix();
        runLoopBoundaryMatrix();
        runReleaseLawMatrix();
        runTypedLoopModeMatrix();
        runReleasePartitionInvariance();
        runPoolLifecycleMatrix();
        runStealAndRepeatedResetMatrix();
        std::cout << "Sprint 4.4 loop, release, and voice-lifecycle matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
