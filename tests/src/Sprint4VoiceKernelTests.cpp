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
    std::uint64_t sampleStartFrame = 0;
};

drs::engine::SamplerRenderModelPtr buildModel(std::vector<std::vector<float>> channels,
                                              const ModelOptions& options = {})
{
    require(!channels.empty() && !channels.front().empty(), "Voice test model requires PCM.");
    const auto frameCount = channels.front().size();
    for (const auto& channel : channels)
        require(channel.size() == frameCount, "Voice test PCM channels must have equal length.");

    drs::engine::ImmutablePlaybackSnapshot snapshot;
    snapshot.draftRevision = 4;
    snapshot.contentDigest = "sprint4-voice-snapshot";
    snapshot.zones.push_back({ "voice-zone",
                               "voice-sample",
                               "Voice Zone",
                               "voice-group",
                               "sustain",
                               options.rootKey,
                               0,
                               127,
                               1,
                               127,
                               options.gainDb,
                               options.pan,
                               options.sampleStartFrame,
                               false,
                               0,
                               0 });

    auto decoded = std::make_shared<drs::engine::PreparedPlaybackDecodedSampleData>();
    decoded->normalizedChannels = std::move(channels);
    drs::engine::PreparedPlaybackSampleHandle sample;
    sample.sampleSourceId = "voice-sample";
    sample.streamSampleId = "voice-stream";
    sample.sampleRate = options.sourceSampleRate;
    sample.frameCount = frameCount;
    sample.channelCount = static_cast<std::uint32_t>(decoded->normalizedChannels.size());
    sample.decodedSampleData = std::move(decoded);

    drs::engine::ImmutablePreparedPlayback prepared;
    prepared.snapshotBuildId = 401;
    prepared.snapshotContentDigest = snapshot.contentDigest;
    prepared.draftRevision = snapshot.draftRevision;
    prepared.preparedContentDigest = "sprint4-voice-prepared";
    prepared.samples.push_back(std::move(sample));
    prepared.zones.push_back({ "voice-zone",
                               "voice-sample",
                               "voice-stream",
                               0,
                               0,
                               options.rootKey,
                               0,
                               127,
                               1,
                               127,
                               options.gainDb,
                               options.pan,
                               options.sampleStartFrame,
                               false,
                               0,
                               0 });

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
    return { 1, 0, note, note, velocity, outputSampleRate };
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
    requireNear(voice.getBaseGain(), 0.25 * 64.0 / 127.0, 1.0e-7,
                "Velocity/gain compatibility formula changed.");
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
    requireVector(output.left, { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f },
                  "Legacy center left vector changed");
    requireVector(output.right, { 1.0f, 0.75f, 0.5f, 0.25f, 0.0f },
                  "Legacy center right vector changed");

    require(voice.render(output.view(), 0, 5).voiceFinished,
            "A completed voice should remain deterministically finished.");
    requireVector(output.left, { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f },
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
    requireVector(impulseOutput.left, { 0.25f, 0.0f, 0.0f, 0.0f },
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
    requireVector(upOutput.left, { 0.0f, 0.5f, 1.0f, 1.5f, 2.0f },
                  "Octave-up interpolation vector changed");
    requireVector(upOutput.right, upOutput.left, "Mono octave-up should duplicate to stereo");

    drs::engine::SamplerVoice down;
    require(down.start(*model, makeStart(48)), "Octave-down voice should start.");
    StereoOutput downOutput(6);
    const auto downResult = down.render(downOutput.view(), 0, 6);
    require(downResult.accepted && !downResult.voiceFinished,
            "Octave-down voice should retain remaining sample frames.");
    requireVector(downOutput.left, { 0.0f, 0.125f, 0.25f, 0.375f, 0.5f, 0.625f },
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
    requireVector(resampledOutput.left, { 0.0f, 0.125f, 0.25f, 0.375f },
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
    requireNear(minimumOutput.left[0], 0.25 / 127.0, renderTolerance,
                "Minimum velocity scale changed.");

    ModelOptions minusSix;
    minusSix.gainDb = -6.020599913279624;
    const auto quietModel = buildModel({ constant }, minusSix);
    drs::engine::SamplerVoice quiet;
    require(quiet.start(*quietModel, makeStart()), "-6.02 dB voice should start.");
    StereoOutput quietOutput(1);
    quiet.render(quietOutput.view(), 0, 1);
    requireNear(quietOutput.left[0], 0.125, renderTolerance,
                "Zone dB conversion changed.");

    ModelOptions hardLeft;
    hardLeft.pan = -1.0;
    const auto leftModel = buildModel({ constant }, hardLeft);
    drs::engine::SamplerVoice leftVoice;
    require(leftVoice.start(*leftModel, makeStart()), "Hard-left voice should start.");
    StereoOutput leftOutput(1);
    leftVoice.render(leftOutput.view(), 0, 1);
    requireNear(leftOutput.left[0], 0.25, renderTolerance, "Hard-left signal changed.");
    requireNear(leftOutput.right[0], 0.0, renderTolerance, "Hard-left opposite channel must mute.");

    ModelOptions hardRight;
    hardRight.pan = 1.0;
    const auto rightModel = buildModel({ constant }, hardRight);
    drs::engine::SamplerVoice rightVoice;
    require(rightVoice.start(*rightModel, makeStart()), "Hard-right voice should start.");
    StereoOutput rightOutput(1);
    rightVoice.render(rightOutput.view(), 0, 1);
    requireNear(rightOutput.left[0], 0.0, renderTolerance, "Hard-right opposite channel must mute.");
    requireNear(rightOutput.right[0], 0.25, renderTolerance, "Hard-right signal changed.");

    ModelOptions halfRight;
    halfRight.pan = 0.5;
    const auto halfRightModel = buildModel({ constant }, halfRight);
    drs::engine::SamplerVoice halfRightVoice;
    require(halfRightVoice.start(*halfRightModel, makeStart()), "Half-right voice should start.");
    StereoOutput halfRightOutput(1);
    halfRightVoice.render(halfRightOutput.view(), 0, 1);
    requireNear(halfRightOutput.left[0], 0.125, renderTolerance, "Linear left balance changed.");
    requireNear(halfRightOutput.right[0], 0.25, renderTolerance, "Linear right balance changed.");
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
    requireVector(output.left, { 0.5f, 0.5f, 1.0f, 1.25f, 1.5f, 0.5f },
                  "Start-offset/additive output changed");
    requireVector(output.right, output.left, "Mono start-offset output should duplicate");

    const auto oneFrameModel = buildModel({ { 0.8f } });
    drs::engine::SamplerVoice oneFrameVoice;
    require(oneFrameVoice.start(*oneFrameModel, makeStart()), "One-frame voice should start.");
    StereoOutput oneFrameOutput(2);
    const auto oneFrameResult = oneFrameVoice.render(oneFrameOutput.view(), 0, 2);
    require(oneFrameResult.mixedFrameCount == 1 && oneFrameResult.voiceFinished,
            "One-frame source must render its final frame once.");
    requireVector(oneFrameOutput.left, { 0.2f, 0.0f }, "One-frame final sample changed");

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
        std::cout << "Sprint 4.2 deterministic voice-kernel matrix passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << std::endl;
        return 1;
    }
}
