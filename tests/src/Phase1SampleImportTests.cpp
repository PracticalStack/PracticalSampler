#include "drs/engine/SampleImport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path getScratchDirectory()
{
    auto path = fs::temp_directory_path() / "drs-phase1-sample-import-tests";
    fs::create_directories(path);
    return path;
}

juce::AudioBuffer<float> buildReferenceBuffer()
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(2, frameCount);

    for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
    {
        const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
        buffer.setSample(0, sampleIndex, std::sin(phase * juce::MathConstants<float>::twoPi));
        buffer.setSample(1, sampleIndex, std::cos(phase * juce::MathConstants<float>::twoPi) * 0.5f);
    }

    return buffer;
}

juce::AudioBuffer<float> buildMultiChannelBuffer(int channelCount)
{
    constexpr int frameCount = 480;
    juce::AudioBuffer<float> buffer(channelCount, frameCount);

    for (int channelIndex = 0; channelIndex < channelCount; ++channelIndex)
    {
        for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
        {
            const auto phase = static_cast<float>(sampleIndex) / static_cast<float>(frameCount);
            buffer.setSample(channelIndex,
                             sampleIndex,
                             std::sin((phase * juce::MathConstants<float>::twoPi) + (0.2f * static_cast<float>(channelIndex))) * 0.25f);
        }
    }

    return buffer;
}

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    const juce::StringPairArray& metadata,
                    double sampleRate = 48000.0)
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open output file for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(sampleRate)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    for (const auto& key : metadata.getAllKeys())
        options = options.withMetadata(key, metadata[key]);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr, "Could not create audio writer for: " + filePath.generic_string());

    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write audio samples to: " + filePath.generic_string());
}

void requireEquivalentWaveforms(const drs::engine::SampleImportResult& left,
                                const drs::engine::SampleImportResult& right)
{
    require(left.sample.normalizedChannels.size() == right.sample.normalizedChannels.size(),
            "Imported channel counts did not match between WAV and FLAC.");

    for (std::size_t channelIndex = 0; channelIndex < left.sample.normalizedChannels.size(); ++channelIndex)
    {
        const auto& leftChannel = left.sample.normalizedChannels[channelIndex];
        const auto& rightChannel = right.sample.normalizedChannels[channelIndex];
        require(leftChannel.size() == rightChannel.size(),
                "Imported frame counts did not match between WAV and FLAC.");

        for (std::size_t sampleIndex = 0; sampleIndex < leftChannel.size(); ++sampleIndex)
        {
            const auto delta = std::abs(leftChannel[sampleIndex] - rightChannel[sampleIndex]);
            require(delta < 1.0e-4f, "Normalized sample data diverged unexpectedly between WAV and FLAC imports.");
        }
    }
}

void requireAnyContains(const std::vector<std::string>& messages,
                        const std::string& needle,
                        const std::string& failureMessage)
{
    const auto containsNeedle = std::any_of(messages.begin(),
                                            messages.end(),
                                            [&](const std::string& message)
                                            {
                                                return message.find(needle) != std::string::npos;
                                            });
    require(containsNeedle, failureMessage);
}
} // namespace

int main()
{
    try
    {
        const auto scratchDirectory = getScratchDirectory();
        const auto wavPath = scratchDirectory / "equivalent-source.wav";
        const auto flacPath = scratchDirectory / "equivalent-source.flac";
        const auto aiffPath = scratchDirectory / "equivalent-source.aiff";
        const auto highRatePath = scratchDirectory / "high-rate-source.wav";
        const auto surroundPath = scratchDirectory / "surround-source.wav";
        const auto awkwardNamePath = scratchDirectory / "Bad Name!.wav";
        const auto unsupportedPath = scratchDirectory / "unsupported-source.txt";

        const auto buffer = buildReferenceBuffer();
        const auto surroundBuffer = buildMultiChannelBuffer(4);

        juce::StringPairArray wavMetadata;
        wavMetadata.set("MidiUnityNote", "69");
        wavMetadata.set("NumSampleLoops", "1");
        wavMetadata.set("Loop0Start", "64");
        wavMetadata.set("Loop0End", "192");

        juce::WavAudioFormat wavFormat;
        writeAudioFile(wavPath, wavFormat, buffer, wavMetadata);

        juce::FlacAudioFormat flacFormat;
        writeAudioFile(flacPath, flacFormat, buffer, {});

        juce::AiffAudioFormat aiffFormat;
        writeAudioFile(aiffPath, aiffFormat, buffer, {});

        writeAudioFile(highRatePath, wavFormat, buffer, {}, 96000.0);
        writeAudioFile(surroundPath, wavFormat, surroundBuffer, {});
        writeAudioFile(awkwardNamePath, wavFormat, buffer, {});

        {
            juce::FileOutputStream unsupportedOutput(juce::File(unsupportedPath.generic_string()));
            require(unsupportedOutput.openedOk(), "Could not create unsupported test fixture.");
            unsupportedOutput.writeText("not audio", false, false, nullptr);
        }

        const auto wavResult = drs::engine::importSampleFile(wavPath.generic_string());
        require(wavResult.imported, "WAV fixture import should succeed.");
        require(wavResult.sample.metadata.formatName == "WAV file", "WAV fixture format name changed unexpectedly.");
        require(wavResult.sample.metadata.sampleRate == 48000.0, "WAV fixture sample rate changed unexpectedly.");
        require(wavResult.sample.metadata.channelCount == 2, "WAV fixture channel count changed unexpectedly.");
        require(wavResult.sample.metadata.frameCount == 480, "WAV fixture frame count changed unexpectedly.");
        require(wavResult.sample.metadata.durationSeconds > 0.009 && wavResult.sample.metadata.durationSeconds < 0.011,
                "WAV fixture duration changed unexpectedly.");
        require(wavResult.sample.metadata.rootMidiNotePresent, "WAV fixture should expose a root midi note.");
        require(wavResult.sample.metadata.rootMidiNote == 69, "WAV fixture root midi note changed unexpectedly.");
        require(wavResult.sample.metadata.loopRangePresent, "WAV fixture should expose a loop range.");
        require(wavResult.sample.metadata.loopStartFrame == 64, "WAV fixture loop start changed unexpectedly.");
        require(wavResult.sample.metadata.loopEndFrame == 192, "WAV fixture loop end changed unexpectedly.");
        require(!wavResult.sample.metadata.sourceChecksumHex.empty(), "WAV fixture checksum must be populated.");
        require(wavResult.warnings.empty(), "WAV fixture should not trigger policy warnings.");

        const auto flacResult = drs::engine::importSampleFile(flacPath.generic_string());
        require(flacResult.imported, "FLAC fixture import should succeed.");
        require(flacResult.sample.metadata.formatName == "FLAC file", "FLAC fixture format name changed unexpectedly.");
        require(flacResult.sample.metadata.sampleRate == wavResult.sample.metadata.sampleRate,
                "FLAC fixture sample rate should match the WAV fixture.");
        require(flacResult.sample.metadata.channelCount == wavResult.sample.metadata.channelCount,
                "FLAC fixture channel count should match the WAV fixture.");
        require(flacResult.sample.metadata.frameCount == wavResult.sample.metadata.frameCount,
                "FLAC fixture frame count should match the WAV fixture.");
        require(!flacResult.sample.metadata.sourceChecksumHex.empty(), "FLAC fixture checksum must be populated.");
        require(flacResult.warnings.empty(), "FLAC fixture should not trigger policy warnings.");

        requireEquivalentWaveforms(wavResult, flacResult);

        const auto awkwardNameResult = drs::engine::importSampleFile(awkwardNamePath.generic_string());
        require(awkwardNameResult.imported, "Awkward-but-decodable WAV fixture should still import.");
        require(!awkwardNameResult.warnings.empty(), "Awkward sample names should trigger a policy warning.");
        requireAnyContains(awkwardNameResult.warnings,
                           "portable sample names",
                           "Awkward sample-name warning should explain the naming policy.");

        const auto aiffResult = drs::engine::importSampleFile(aiffPath.generic_string());
        require(!aiffResult.imported, "AIFF fixture should be rejected by the Phase 1 format policy.");
        requireAnyContains(aiffResult.issues,
                           "WAV and FLAC",
                           "AIFF policy rejection should explain the supported Phase 1 formats.");

        const auto highRateResult = drs::engine::importSampleFile(highRatePath.generic_string());
        require(!highRateResult.imported, "96 kHz fixture should be rejected by the Phase 1 sample-rate policy.");
        requireAnyContains(highRateResult.issues,
                           "44100 Hz and 48000 Hz",
                           "Sample-rate policy rejection should explain the supported Phase 1 rates.");

        const auto surroundResult = drs::engine::importSampleFile(surroundPath.generic_string());
        require(!surroundResult.imported, "Four-channel fixture should be rejected by the Phase 1 channel-count policy.");
        requireAnyContains(surroundResult.issues,
                           "mono and stereo",
                           "Channel-count policy rejection should explain the supported Phase 1 channel layouts.");

        const auto missingResult = drs::engine::importSampleFile((scratchDirectory / "missing.wav").generic_string());
        require(!missingResult.imported, "Missing file import should fail.");
        require(!missingResult.issues.empty(), "Missing file import should report an actionable issue.");

        const auto unsupportedResult = drs::engine::importSampleFile(unsupportedPath.generic_string());
        require(!unsupportedResult.imported, "Unsupported file import should fail.");
        require(!unsupportedResult.issues.empty(), "Unsupported file import should report an actionable issue.");

        std::cout << "Phase 1 sample import tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 sample import tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
