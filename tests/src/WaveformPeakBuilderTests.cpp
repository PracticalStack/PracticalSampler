#include "drs/engine/SampleImport.h"
#include "WavImportTestSupport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path getScratchDirectory()
{
    const auto path = fs::temp_directory_path() / "drs-waveform-peak-builder-tests";
    fs::create_directories(path);
    return path;
}

void writeAudioFile(const fs::path& filePath, const juce::AudioBuffer<float>& buffer)
{
    juce::WavAudioFormat wavFormat;
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open waveform policy fixture for writing.");
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(48000.0)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    auto writer = wavFormat.createWriterFor(output, options);
    require(writer != nullptr, "Could not create waveform policy fixture writer.");
    require(writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write waveform policy fixture.");
}

juce::AudioBuffer<float> buildChannelPolicyFixture()
{
    juce::AudioBuffer<float> buffer(2, 4);
    buffer.setSample(0, 0, -0.2f);
    buffer.setSample(0, 1, 0.2f);
    buffer.setSample(0, 2, 0.2f);
    buffer.setSample(0, 3, -0.2f);
    buffer.setSample(1, 0, -0.9f);
    buffer.setSample(1, 1, 0.9f);
    buffer.setSample(1, 2, 0.9f);
    buffer.setSample(1, 3, -0.9f);
    return buffer;
}

bool nearlyEqual(const float left, const float right, const float epsilon = 1.0e-4f)
{
    return std::abs(left - right) <= epsilon;
}

class CancelAfterFirstProgress final : public drs::engine::WaveformPeakBuildCallbacks
{
public:
    bool isCancellationRequested() const override
    {
        return cancelRequested;
    }

    void onProgress(const drs::engine::WaveformPeakBuildProgress& progress) const override
    {
        ++progressCallbackCount;
        lastFramesProcessed = progress.framesProcessed;
        cancelRequested = true;
    }

    mutable bool cancelRequested = false;
    mutable int progressCallbackCount = 0;
    mutable std::uint64_t lastFramesProcessed = 0;
};
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto policyFixturePath = scratchDirectory / "channel-policy.wav";
        writeAudioFile(policyFixturePath, buildChannelPolicyFixture());

        drs::engine::WaveformPeakBuildOptions options;
        options.displayPointCount = 1;
        const auto firstChannel = drs::engine::buildWaveformPeaks(
            policyFixturePath.generic_string(), {}, { 1, 4096, drs::engine::WaveformPeakChannelReduction::firstChannel });
        require(firstChannel.built, "First-channel waveform peaks should build for the policy fixture.");
        require(firstChannel.points.size() == 1, "Single-point waveform policy fixture should collapse to one point.");
        require(nearlyEqual(firstChannel.points.front().minValue, -0.2f)
                    && nearlyEqual(firstChannel.points.front().maxValue, 0.2f),
                "First-channel waveform reduction should follow channel 0 values.");

        const auto averaged = drs::engine::buildWaveformPeaks(
            policyFixturePath.generic_string(), {}, { 1, 4096, drs::engine::WaveformPeakChannelReduction::averageChannels });
        require(averaged.built, "Average-channel waveform peaks should build for the policy fixture.");
        require(nearlyEqual(averaged.points.front().minValue, -0.55f)
                    && nearlyEqual(averaged.points.front().maxValue, 0.55f),
                "Average-channel waveform reduction should average both channels per frame.");

        const auto extrema = drs::engine::buildWaveformPeaks(
            policyFixturePath.generic_string(), {}, { 1, 4096, drs::engine::WaveformPeakChannelReduction::channelExtrema });
        require(extrema.built, "Extrema waveform peaks should build for the policy fixture.");
        require(nearlyEqual(extrema.points.front().minValue, -0.9f)
                    && nearlyEqual(extrema.points.front().maxValue, 0.9f),
                "Channel-extrema waveform reduction should preserve the widest per-frame envelope.");

        drs::tests::DeterministicSampleImportHooks hooks;
        const auto syntheticPath = (scratchDirectory / "synthetic-chunked.wav").generic_string();
        hooks.addReaderFixture({ syntheticPath, "WAV file", 48000.0, 8192, 2, 32, true, {}, false });
        hooks.setFingerprintBytes(syntheticPath, std::string(16384, 'p'));

        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            drs::engine::resetSampleImportIoCounters();
            drs::engine::WaveformPeakBuildOptions chunkedOptions;
            chunkedOptions.displayPointCount = 64;
            chunkedOptions.chunkFrameCount = 256;
            const auto chunked = drs::engine::buildWaveformPeaks(syntheticPath, {}, chunkedOptions);
            require(chunked.built, "Chunked waveform peaks should build for the synthetic fixture.");
            require(chunked.points.size() == 64,
                    "Chunked waveform peaks should honor the requested display resolution.");
            const auto counters = drs::engine::getSampleImportIoCounters();
            require(counters.readerOpenCount == 1,
                    "Waveform peak building should record one reader-open attempt.");
            require(counters.fingerprintOpenCount == 1,
                    "Waveform peak building should record one fingerprint-stream open.");
            require(counters.fullFrameReadCount == 0,
                    "Waveform peak building must not perform a full-frame decode read.");
            require(counters.peakChunkReadCount > 0,
                    "Waveform peak building should record bounded chunk reads.");
        }

        {
            drs::engine::ScopedSampleImportHooksOverride scope(hooks);
            CancelAfterFirstProgress callbacks;
            drs::engine::resetSampleImportIoCounters();
            drs::engine::WaveformPeakBuildOptions cancelOptions;
            cancelOptions.displayPointCount = 64;
            cancelOptions.chunkFrameCount = 256;
            cancelOptions.callbacks = &callbacks;
            const auto canceled = drs::engine::buildWaveformPeaks(syntheticPath, {}, cancelOptions);
            require(!canceled.built && canceled.canceled,
                    "Waveform peak building should honor explicit cancellation.");
            require(callbacks.progressCallbackCount == 1 && callbacks.lastFramesProcessed > 0,
                    "Waveform peak cancellation coverage requires progress publication before canceling.");
            const auto counters = drs::engine::getSampleImportIoCounters();
            require(counters.fullFrameReadCount == 0,
                    "Canceled waveform peak building must not fall back to a full-frame decode.");
            require(counters.peakChunkReadCount == 1,
                    "Canceled waveform peak building should stop after the first chunk once cancellation is requested.");
        }

        std::cout << "Waveform peak builder tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Waveform peak builder tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
