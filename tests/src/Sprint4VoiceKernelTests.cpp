#include "drs/engine/SamplerRenderModel.h"
#include "drs/engine/SamplerVoice.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr float renderTolerance = 1.0e-6f;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void requireNear(double actual, double expected, double tolerance, const std::string& message)
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
        if (std::abs(actual[index] - expected[index]) > renderTolerance)
            throw std::runtime_error(message + " at frame " + std::to_string(index)
                                     + " (actual=" + std::to_string(actual[index])
                                     + ", expected=" + std::to_string(expected[index]) + ")");
}

struct ModelOptions
{
    double sourceSampleRate = 48000.0;
    int rootKey = 60;
    double gainDb = 0.0;
    double pan = 0.0;
    double fineTuneCents = 0.0;
    double amplitudeVelocityTracking = 100.0;
    std::uint64_t sampleStartFrame = 0;
    std::uint64_t sampleEndFrame = 0;
};

drs::engine::SamplerRenderModelPtr buildModel(std::vector<std::vector<float>> channels,
                                              const ModelOptions& options = {},
                                              drs::engine::SampleDataSourcePtr dataSource = nullptr)
{
    require(!channels.empty() && !channels.front().empty(), "Voice test model requires PCM.");
    const auto frameCount = channels.front().size();
    for (const auto& channel : channels)
        require(channel.size() == frameCount, "Voice test PCM channels must have equal length.");

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 4;
    snapshot.contentDigest = "sprint4-voice-snapshot";
    drs::engine::PlaybackSnapshotZone snapshotZone;
    snapshotZone.id = "voice-zone";
    snapshotZone.sampleSourceId = "voice-sample";
    snapshotZone.displayName = "Voice Zone";
    snapshotZone.groupId = "voice-group";
    snapshotZone.articulationId = "sustain";
    snapshotZone.rootKey = options.rootKey;
    snapshotZone.gainDb = options.gainDb;
    snapshotZone.pan = options.pan;
    snapshotZone.fineTuneCents = options.fineTuneCents;
    snapshotZone.amplitudeVelocityTracking = options.amplitudeVelocityTracking;
    snapshotZone.sampleStartFrame = options.sampleStartFrame;
    snapshotZone.sampleEndFrame = options.sampleEndFrame;
    snapshot.zones.push_back(std::move(snapshotZone));
    drs::engine::PlaybackSnapshotGroupRoute snapshotGroup;
    snapshotGroup.groupId = "voice-group";
    snapshotGroup.articulationIds = { "sustain" };
    snapshotGroup.zoneIds = { "voice-zone" };
    snapshotGroup.displayName = "Voice Group";
    snapshot.groupRoutes.push_back(std::move(snapshotGroup));

    drs::engine::PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "voice-sample";
    sample.streamSampleId = "voice-stream";
    sample.sampleRate = options.sourceSampleRate;
    sample.frameCount = frameCount;
    sample.channelCount = static_cast<std::uint32_t>(channels.size());
    if (dataSource != nullptr)
        sample.dataSource = std::move(dataSource);
    else
    {
        auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
        decoded->normalizedChannels = std::move(channels);
        sample.decodedSampleData = std::move(decoded);
    }

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 401;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint4-voice-prepared";
    prepared.samples.push_back(std::move(sample));
    drs::engine::PreparedPlaybackZoneHandle preparedZone;
    preparedZone.zoneId = "voice-zone";
    preparedZone.sampleSourceId = "voice-sample";
    preparedZone.streamSampleId = "voice-stream";
    preparedZone.rootKey = options.rootKey;
    preparedZone.gainDb = options.gainDb;
    preparedZone.pan = options.pan;
    preparedZone.fineTuneCents = options.fineTuneCents;
    preparedZone.amplitudeVelocityTracking = options.amplitudeVelocityTracking;
    preparedZone.sampleStartFrame = options.sampleStartFrame;
    preparedZone.sampleEndFrame = options.sampleEndFrame;
    prepared.zones.push_back(std::move(preparedZone));
    drs::engine::PreparedPlaybackGroupRoute preparedGroup;
    preparedGroup.groupId = "voice-group";
    preparedGroup.articulationIds = { "sustain" };
    preparedGroup.zoneIds = { "voice-zone" };
    preparedGroup.displayName = "Voice Group";
    prepared.groupRoutes.push_back(std::move(preparedGroup));

    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::preview;
    payload->revision = snapshot.draftRevision;
    payload->snapshotBuildId = prepared.snapshotBuildId;
    payload->preparedBuildId = 402;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshot.contentDigest;
    payload->preparedContentDigest = prepared.preparedContentDigest;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(std::move(snapshot));
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(std::move(prepared));

    const auto result = drs::engine::buildSamplerRenderModel(payload);
    require(result.built && result.model != nullptr, "Voice test render model should validate.");
    return result.model;
}

drs::engine::SamplerVoiceStartRequest makeStart(int note = 60,
                                                 int velocity = 127,
                                                 double outputSampleRate = 48000.0)
{
    drs::engine::SamplerVoiceStartRequest request;
    request.voiceId = 1;
    request.routeIndex = 0;
    request.sourceMidiNote = note;
    request.effectiveMidiNote = note;
    request.effectiveVelocity = velocity;
    request.outputSampleRate = outputSampleRate;
    return request;
}

struct StereoOutput
{
    std::vector<float> left;
    std::vector<float> right;
    std::array<float*, 2> pointers;

    explicit StereoOutput(std::size_t frames, float initial = 0.0f)
        : left(frames, initial), right(frames, initial), pointers { left.data(), right.data() }
    {
    }

    drs::engine::SamplerAudioBufferView view()
    {
        return { pointers.data(), 2, static_cast<std::uint32_t>(left.size()) };
    }
};

class IntentRecordingSource final : public drs::engine::ISampleDataSource
{
public:
    explicit IntentRecordingSource(
        std::shared_ptr<drs::engine::DeterministicFakePagedSampleDataSource> source)
        : backing(std::move(source))
    {
    }

    const drs::engine::SampleDataSourceDescriptor& descriptor() const noexcept override
    {
        return backing->descriptor();
    }

    drs::engine::SampleFrameView acquireFrameView(
        std::uint64_t firstFrame, std::uint32_t frames) const noexcept override
    {
        return backing->acquireFrameView(firstFrame, frames);
    }

    bool publishPageIntent(std::uint64_t firstFrame,
                           drs::engine::SamplePageRequestPriority,
                           std::uint64_t) const noexcept override
    {
        lastRequestedFrame.store(firstFrame, std::memory_order_relaxed);
        publicationCount.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    std::shared_ptr<drs::engine::DeterministicFakePagedSampleDataSource> backing;
    mutable std::atomic<std::uint64_t> publicationCount {0};
    mutable std::atomic<std::uint64_t> lastRequestedFrame {0};
};

void runStartAndFormulaContract()
{
    static_assert(noexcept(std::declval<drs::engine::SamplerVoice&>().start(
                      std::declval<const drs::engine::SamplerRenderModel&>(),
                      std::declval<const drs::engine::SamplerVoiceStartRequest&>())),
                  "Voice start must remain noexcept.");
    static_assert(noexcept(std::declval<drs::engine::SamplerVoice&>().render(
                      std::declval<drs::engine::SamplerAudioBufferView>(), 0, 0)),
                  "Voice render must remain noexcept.");

    const auto model = buildModel({ { 0.0f, 0.5f, 1.0f, 0.5f } });
    drs::engine::SamplerVoice voice;
    const auto start = makeStart(72, 64, 96000.0);
    require(voice.start(*model, start), "A valid route should initialize the voice kernel.");
    require(voice.isActive()
                && voice.getVoiceId() == 1
                && voice.getRouteIndex() == 0
                && voice.getSourceMidiNote() == 72
                && voice.getEffectiveMidiNote() == 72
                && voice.getEffectiveVelocity() == 64,
            "Voice identity must be stable primitive state.");
    requireNear(voice.getIncrementFrames(), 1.0, 1.0e-12,
                "Octave-up at half source/output rate should produce unity increment.");
    requireNear(voice.getBaseGain(), 64.0 / 127.0, 1.0e-7,
                "Velocity/gain formula changed.");
    requireNear(voice.getPositionFrames(), 0.0, 0.0, "Voice should start at the route offset.");
    requireNear(voice.getPanGains().left, 1.0, 0.0, "Center left balance changed.");
    requireNear(voice.getPanGains().right, 1.0, 0.0, "Center right balance changed.");

    auto invalid = makeStart();
    invalid.voiceId = 0;
    require(!voice.start(*model, invalid)
                && voice.getLifecycleState() == drs::engine::SamplerVoiceLifecycleState::idle,
            "Zero voice identity must be rejected and reset state.");
    invalid = makeStart();
    invalid.routeIndex = 4;
    require(!voice.start(*model, invalid), "Out-of-range route must be rejected.");
    invalid = makeStart();
    invalid.effectiveVelocity = 0;
    require(!voice.start(*model, invalid), "Velocity outside 1-127 must be rejected.");
    invalid = makeStart();
    invalid.effectiveMidiNote = 128;
    require(!voice.start(*model, invalid), "MIDI note outside 0-127 must be rejected.");
    invalid = makeStart();
    invalid.outputSampleRate = 0.0;
    require(!voice.start(*model, invalid), "Non-positive output sample rate must be rejected.");
}

void runLegacyCenterReferenceVectors()
{
    // Captured from the legacy renderBlockRange formula at center pan, unity pitch, gain 0 dB,
    // and velocity 127. These values are the approved extraction baseline.
    const auto model = buildModel({ { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
                                    { 4.0f, 3.0f, 2.0f, 1.0f, 0.0f } });
    drs::engine::SamplerVoice voice;
    require(voice.start(*model, makeStart()), "Legacy vector voice should start.");
    StereoOutput output(5);
    const auto result = voice.render(output.view(), 0, 5);
    require(result.accepted && result.mixedFrameCount == 5 && result.voiceFinished,
            "Legacy vector should render through the final frame exactly once.");
    requireVector(output.left, { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
                  "Legacy center left vector changed");
    requireVector(output.right, { 4.0f, 3.0f, 2.0f, 1.0f, 0.0f },
                  "Legacy center right vector changed");

    require(voice.render(output.view(), 0, 5).voiceFinished,
            "A completed voice should remain deterministically finished.");
    requireVector(output.left, { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f },
                  "Finished voice must not render twice");
}

void runSilenceAndImpulseMatrix()
{
    const auto silentModel = buildModel({ { 0.0f, 0.0f, 0.0f, 0.0f } });
    drs::engine::SamplerVoice silentVoice;
    require(silentVoice.start(*silentModel, makeStart()), "Silent voice should start.");
    StereoOutput silentOutput(4);
    const auto silentResult = silentVoice.render(silentOutput.view(), 0, 4);
    require(silentResult.mixedFrameCount == 4 && silentResult.voiceFinished,
            "Silent source must still advance and finish deterministically.");
    requireVector(silentOutput.left, { 0.0f, 0.0f, 0.0f, 0.0f },
                  "Silent left vector changed");
    requireVector(silentOutput.right, silentOutput.left, "Silent mono duplication changed");

    const auto impulseModel = buildModel({ { 1.0f, 0.0f, 0.0f, 0.0f } });
    drs::engine::SamplerVoice impulseVoice;
    require(impulseVoice.start(*impulseModel, makeStart()), "Impulse voice should start.");
    StereoOutput impulseOutput(4);
    impulseVoice.render(impulseOutput.view(), 0, 4);
    requireVector(impulseOutput.left, { 1.0f, 0.0f, 0.0f, 0.0f },
                  "Impulse response vector changed");
    requireVector(impulseOutput.right, impulseOutput.left, "Impulse mono duplication changed");
}

void runPitchAndInterpolationMatrix()
{
    const std::vector<float> ramp { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f };
    const auto model = buildModel({ ramp });

    drs::engine::SamplerVoice up;
    require(up.start(*model, makeStart(72)), "Octave-up voice should start.");
    StereoOutput upOutput(5);
    require(up.render(upOutput.view(), 0, 5).voiceFinished,
            "Octave-up voice should reach sample end.");
    requireVector(upOutput.left, { 0.0f, 2.0f, 4.0f, 6.0f, 8.0f },
                  "Octave-up interpolation vector changed");
    requireVector(upOutput.right, upOutput.left, "Mono octave-up should duplicate to stereo");

    drs::engine::SamplerVoice down;
    require(down.start(*model, makeStart(48)), "Octave-down voice should start.");
    StereoOutput downOutput(6);
    const auto downResult = down.render(downOutput.view(), 0, 6);
    require(downResult.accepted && !downResult.voiceFinished,
            "Octave-down voice should retain remaining sample frames.");
    requireVector(downOutput.left, { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f, 2.5f },
                  "Octave-down fractional interpolation vector changed");

    ModelOptions halfRate;
    halfRate.sourceSampleRate = 24000.0;
    const auto halfRateModel = buildModel({ ramp }, halfRate);
    drs::engine::SamplerVoice resampled;
    require(resampled.start(*halfRateModel, makeStart()), "Half-rate voice should start.");
    requireNear(resampled.getIncrementFrames(), 0.5, 1.0e-12,
                "Source/output sample-rate ratio changed.");
    StereoOutput resampledOutput(4);
    resampled.render(resampledOutput.view(), 0, 4);
    requireVector(resampledOutput.left, { 0.0f, 0.5f, 1.0f, 1.5f },
                  "Sample-rate interpolation vector changed");
}

void runGainVelocityAndPanMatrix()
{
    const std::vector<float> constant(4, 1.0f);

    drs::engine::SamplerVoice minimumVelocity;
    const auto centerModel = buildModel({ constant });
    require(minimumVelocity.start(*centerModel, makeStart(60, 1)), "Minimum velocity should start.");
    StereoOutput minimumOutput(1);
    minimumVelocity.render(minimumOutput.view(), 0, 1);
    requireNear(minimumOutput.left[0], 1.0 / 127.0, renderTolerance,
                "Minimum velocity scale changed.");

    ModelOptions minusSix;
    minusSix.gainDb = -6.020599913279624;
    const auto quietModel = buildModel({ constant }, minusSix);
    drs::engine::SamplerVoice quiet;
    require(quiet.start(*quietModel, makeStart()), "-6.02 dB voice should start.");
    StereoOutput quietOutput(1);
    quiet.render(quietOutput.view(), 0, 1);
    requireNear(quietOutput.left[0], 0.5, renderTolerance,
                "Zone dB conversion changed.");

    ModelOptions hardLeft;
    hardLeft.pan = -1.0;
    const auto leftModel = buildModel({ constant }, hardLeft);
    drs::engine::SamplerVoice leftVoice;
    require(leftVoice.start(*leftModel, makeStart()), "Hard-left voice should start.");
    StereoOutput leftOutput(1);
    leftVoice.render(leftOutput.view(), 0, 1);
    requireNear(leftOutput.left[0], 1.0, renderTolerance, "Hard-left signal changed.");
    requireNear(leftOutput.right[0], 0.0, renderTolerance, "Hard-left opposite channel must mute.");

    ModelOptions hardRight;
    hardRight.pan = 1.0;
    const auto rightModel = buildModel({ constant }, hardRight);
    drs::engine::SamplerVoice rightVoice;
    require(rightVoice.start(*rightModel, makeStart()), "Hard-right voice should start.");
    StereoOutput rightOutput(1);
    rightVoice.render(rightOutput.view(), 0, 1);
    requireNear(rightOutput.left[0], 0.0, renderTolerance, "Hard-right opposite channel must mute.");
    requireNear(rightOutput.right[0], 1.0, renderTolerance, "Hard-right signal changed.");

    ModelOptions halfRight;
    halfRight.pan = 0.5;
    const auto halfRightModel = buildModel({ constant }, halfRight);
    drs::engine::SamplerVoice halfRightVoice;
    require(halfRightVoice.start(*halfRightModel, makeStart()), "Half-right voice should start.");
    StereoOutput halfRightOutput(1);
    halfRightVoice.render(halfRightOutput.view(), 0, 1);
    requireNear(halfRightOutput.left[0], 0.5, renderTolerance, "Linear left balance changed.");
    requireNear(halfRightOutput.right[0], 1.0, renderTolerance, "Linear right balance changed.");

    ModelOptions untracked;
    untracked.amplitudeVelocityTracking = 0.0;
    const auto untrackedModel = buildModel({ constant }, untracked);
    drs::engine::SamplerVoice untrackedVoice;
    require(untrackedVoice.start(*untrackedModel, makeStart(60, 1)),
            "Velocity-independent voice should start.");
    requireNear(untrackedVoice.getBaseGain(), 1.0, renderTolerance,
                "amp_veltrack=0 must produce velocity-independent unity gain.");

    ModelOptions halfTracked;
    halfTracked.amplitudeVelocityTracking = 50.0;
    const auto halfTrackedModel = buildModel({ constant }, halfTracked);
    drs::engine::SamplerVoice halfTrackedVoice;
    require(halfTrackedVoice.start(*halfTrackedModel, makeStart(60, 32)),
            "Partially velocity-tracked voice should start.");
    requireNear(halfTrackedVoice.getBaseGain(), std::sqrt(32.0 / 127.0), renderTolerance,
                "amp_veltrack=50 must use the documented native square-root law.");

    ModelOptions tuned;
    tuned.fineTuneCents = 100.0;
    const auto tunedModel = buildModel({ constant }, tuned);
    drs::engine::SamplerVoice tunedVoice;
    require(tunedVoice.start(*tunedModel, makeStart()), "+100-cent voice should start.");
    requireNear(tunedVoice.getIncrementFrames(), std::pow(2.0, 1.0 / 12.0), 1.0e-12,
                "Fine tuning must apply cents in the playback pitch ratio.");
}

void runOffsetAccumulationAndFinalFrameMatrix()
{
    ModelOptions offset;
    offset.sampleStartFrame = 2;
    const auto offsetModel = buildModel({ { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f } }, offset);
    drs::engine::SamplerVoice offsetVoice;
    require(offsetVoice.start(*offsetModel, makeStart()), "Offset voice should start.");
    StereoOutput output(6, 0.5f);
    const auto result = offsetVoice.render(output.view(), 2, 3);
    require(result.mixedFrameCount == 3 && result.voiceFinished,
            "Offset render should consume frames 2 through 4.");
    requireVector(output.left, { 0.5f, 0.5f, 2.5f, 3.5f, 4.5f, 0.5f },
                  "Start-offset/additive output changed");
    requireVector(output.right, output.left, "Mono start-offset output should duplicate");

    const auto oneFrameModel = buildModel({ { 0.8f } });
    drs::engine::SamplerVoice oneFrameVoice;
    require(oneFrameVoice.start(*oneFrameModel, makeStart()), "One-frame voice should start.");
    StereoOutput oneFrameOutput(2);
    const auto oneFrameResult = oneFrameVoice.render(oneFrameOutput.view(), 0, 2);
    require(oneFrameResult.mixedFrameCount == 1 && oneFrameResult.voiceFinished,
            "One-frame source must render its final frame once.");
    requireVector(oneFrameOutput.left, { 0.8f, 0.0f }, "One-frame final sample changed");

    drs::engine::SamplerVoice invalidRange;
    require(invalidRange.start(*oneFrameModel, makeStart()), "Invalid-range voice should start.");
    StereoOutput invalidOutput(2);
    const auto rejected = invalidRange.render(invalidOutput.view(), 1, 2);
    require(!rejected.accepted && invalidRange.isActive(),
            "Invalid output range must not advance or finish the voice.");
}

void runPartitionInvarianceMatrix()
{
    std::vector<float> source(32);
    for (std::size_t index = 0; index < source.size(); ++index)
        source[index] = static_cast<float>(index) / 31.0f;
    const auto model = buildModel({ source });
    const auto start = makeStart(61, 93);

    drs::engine::SamplerVoice contiguous;
    drs::engine::SamplerVoice partitioned;
    require(contiguous.start(*model, start) && partitioned.start(*model, start),
            "Partition comparison voices should start.");
    StereoOutput contiguousOutput(12);
    StereoOutput partitionedOutput(12);
    contiguous.render(contiguousOutput.view(), 0, 12);
    partitioned.render(partitionedOutput.view(), 0, 3);
    partitioned.render(partitionedOutput.view(), 3, 4);
    partitioned.render(partitionedOutput.view(), 7, 5);

    requireVector(partitionedOutput.left, contiguousOutput.left,
                  "Equivalent block partition changed left output");
    requireVector(partitionedOutput.right, contiguousOutput.right,
                  "Equivalent block partition changed right output");
    requireNear(partitioned.getPositionFrames(), contiguous.getPositionFrames(), 1.0e-12,
                "Equivalent block partition changed cursor position.");
    require(partitioned.getLifecycleState() == contiguous.getLifecycleState(),
            "Equivalent block partition changed lifecycle state.");
}

drs::engine::SampleDataSourceDescriptor makePagedDescriptor(const std::uint64_t frameCount)
{
    drs::engine::SampleDataSourceDescriptor descriptor;
    descriptor.kind = drs::engine::SampleDataSourceKind::deterministicFake;
    descriptor.sourceId = "voice-sample";
    descriptor.canonicalSourceIdentity = "synthetic://voice-paged";
    descriptor.provenanceIdentity = "voice-paged-generation-1";
    descriptor.formatName = "float32";
    descriptor.channelLayout = "mono";
    descriptor.generation = 91;
    descriptor.sampleRate = 48000.0;
    descriptor.frameCount = frameCount;
    descriptor.channelCount = 1;
    descriptor.bytesPerFrame = sizeof(float);
    descriptor.dataSizeBytes = frameCount * sizeof(float);
    descriptor.headSizeBytes = 4 * sizeof(float);
    descriptor.pageSizeBytes = 4 * sizeof(float);
    return descriptor;
}

void runPagedBoundaryAndUnderrunMatrix()
{
    std::vector<float> source(12);
    for (std::size_t index = 0; index < source.size(); ++index)
        source[index] = static_cast<float>(index) / 11.0f;
    const auto residentModel = buildModel({ source });
    auto readySource = std::make_shared<drs::engine::DeterministicFakePagedSampleDataSource>(
        makePagedDescriptor(source.size()), std::vector<std::vector<float>> { source },
        4, 4, std::vector<bool> { true, true });
    auto recordingSource = std::make_shared<IntentRecordingSource>(readySource);
    const auto pagedModel = buildModel({ source }, {}, recordingSource);

    drs::engine::SamplerVoice resident;
    drs::engine::SamplerVoice paged;
    require(resident.start(*residentModel, makeStart())
                && paged.start(*pagedModel, makeStart()),
            "Resident and paged parity voices should start.");
    StereoOutput residentOutput(source.size());
    StereoOutput pagedOutput(source.size());
    const auto residentResult = resident.render(
        residentOutput.view(), 0, static_cast<std::uint32_t>(source.size()));
    const auto pagedResult = paged.render(
        pagedOutput.view(), 0, static_cast<std::uint32_t>(source.size()));
    requireVector(pagedOutput.left, residentOutput.left,
                  "Ready pages changed resident output across head/page boundaries");
    require(residentResult.mixedFrameCount == pagedResult.mixedFrameCount
                && pagedResult.pageMissCount == 0
                && pagedResult.underrunFrameCount == 0,
            "Ready paged rendering must retain resident completion and zero-miss diagnostics.");
    require(recordingSource->publicationCount.load(std::memory_order_relaxed) == 1
                && recordingSource->lastRequestedFrame.load(std::memory_order_relaxed) == 11,
            "The voice must publish one bounded proactive look-ahead intent before leaving its head.");

    auto boundedReadySource = std::make_shared<drs::engine::DeterministicFakePagedSampleDataSource>(
        makePagedDescriptor(source.size()), std::vector<std::vector<float>> { source },
        4, 4, std::vector<bool> { true, true });
    auto boundedRecordingSource = std::make_shared<IntentRecordingSource>(boundedReadySource);
    ModelOptions boundedOptions;
    boundedOptions.sampleEndFrame = 7;
    const auto boundedPagedModel = buildModel(
        { source }, boundedOptions, boundedRecordingSource);
    drs::engine::SamplerVoice boundedPaged;
    require(boundedPaged.start(*boundedPagedModel, makeStart()),
            "Bounded paged voice should start.");
    StereoOutput boundedPagedOutput(8);
    const auto boundedPagedResult = boundedPaged.render(boundedPagedOutput.view(), 0, 8);
    require(boundedPagedResult.mixedFrameCount == 7 && boundedPagedResult.voiceFinished
                && boundedRecordingSource->publicationCount.load(std::memory_order_relaxed) == 1
                && boundedRecordingSource->lastRequestedFrame.load(std::memory_order_relaxed) == 6,
            "Paged look-ahead and completion must clamp to the authored exclusive end.");

    auto recoveringSource = std::make_shared<drs::engine::DeterministicFakePagedSampleDataSource>(
        makePagedDescriptor(source.size()), std::vector<std::vector<float>> { source },
        4, 4, std::vector<bool> { false, true });
    const auto recoveringModel = buildModel({ source }, {}, recoveringSource);
    drs::engine::SamplerVoice recovering;
    require(recovering.start(*recoveringModel, makeStart()),
            "Paged underrun voice should start from its resident head.");
    StereoOutput recoveringOutput(8);
    const auto missing = recovering.render(recoveringOutput.view(), 0, 6);
    require(missing.pageMissCount == 3 && missing.underrunFrameCount == 3
                && missing.recoveryCount == 0
                && recovering.getPositionFrames() == 6.0,
            "Missing pages must render bounded silence while advancing musical time.");
    recoveringSource->setPageReady(0, true);
    const auto resumed = recovering.render(recoveringOutput.view(), 6, 2);
    require(resumed.recoveryCount == 1 && resumed.pageMissCount == 0
                && recoveringOutput.left[6] == source[6]
                && recoveringOutput.left[7] == source[7],
            "A newly ready page must resume at the current musical position without replay.");

    drs::engine::SamplePageIntentRing ring;
    for (std::size_t index = 0; index < drs::engine::SamplePageIntentRing::capacity; ++index)
        require(ring.push({ 91, index, drs::engine::SamplePageRequestPriority::lookAhead, 7 }),
                "The fixed intent ring must accept exactly its advertised capacity.");
    require(!ring.push({ 91, 999, drs::engine::SamplePageRequestPriority::imminent, 7 })
                && ring.metrics().droppedCount == 1
                && ring.metrics().maximumDepth == drs::engine::SamplePageIntentRing::capacity,
            "Intent saturation must drop deterministically without blocking or allocating.");
    drs::engine::SamplePageIntent intent;
    for (std::size_t index = 0; index < drs::engine::SamplePageIntentRing::capacity; ++index)
        require(ring.pop(intent) && intent.pageIndex == index,
                "The SPSC intent ring must preserve FIFO ordering.");
    require(!ring.pop(intent) && ring.metrics().consumedCount
                == drs::engine::SamplePageIntentRing::capacity,
            "The drained intent ring must report bounded publication/consumption metrics.");
}
} // namespace

int main()
{
    try
    {
        runStartAndFormulaContract();
        runLegacyCenterReferenceVectors();
        runSilenceAndImpulseMatrix();
        runPitchAndInterpolationMatrix();
        runGainVelocityAndPanMatrix();
        runOffsetAccumulationAndFinalFrameMatrix();
        runPartitionInvarianceMatrix();
        runPagedBoundaryAndUnderrunMatrix();
        std::cout << "Sprint 4.2 deterministic voice-kernel matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
