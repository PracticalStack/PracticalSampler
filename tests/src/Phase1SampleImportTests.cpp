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

class NullReaderHooks final : public drs::engine::SampleImportHooks
{
public:
    bool fileExists(const std::string&) const override
    {
        return true;
    }

    std::unique_ptr<juce::AudioFormatReader> createAudioReader(const std::string&) const override
    {
        return {};
    }
};

class FailingCopyHooks final : public drs::engine::SampleImportHooks
{
public:
    bool copyFile(const std::string&, const std::string&) const override
    {
        return false;
    }
};
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

        drs::engine::resetSampleImportIoCounters();
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

        const auto wavIoCounters = drs::engine::getSampleImportIoCounters();
        require(wavIoCounters.readerOpenCount == 1, "Import instrumentation should record one reader-open attempt.");
        require(wavIoCounters.fingerprintOpenCount == 1,
                "Import instrumentation should record one fingerprint-stream open.");
        require(wavIoCounters.bytesReadCount > 0,
                "Import instrumentation should record source bytes read while fingerprinting.");
        require(wavIoCounters.fullFrameReadCount == 1,
                "Import instrumentation should record one full-frame decode read.");
        require(wavIoCounters.copyCount == 0,
                "Direct sample import should not record any copy operations.");
        require(wavIoCounters.peakChunkReadCount == 0,
                "Direct sample import should not report waveform peak chunk reads.");

        drs::engine::resetSampleImportIoCounters();
        const auto wavInspection = drs::engine::inspectSampleFile(wavPath.generic_string());
        require(wavInspection.inspected, "WAV fixture inspection should succeed.");
        require(wavInspection.accepted, "WAV fixture inspection should satisfy the Phase 1 policy.");
        require(wavInspection.metadata.formatName == wavResult.sample.metadata.formatName,
                "Metadata-only inspection should preserve the detected format name.");
        require(wavInspection.metadata.sampleRate == wavResult.sample.metadata.sampleRate,
                "Metadata-only inspection should preserve the detected sample rate.");
        require(wavInspection.metadata.channelCount == wavResult.sample.metadata.channelCount,
                "Metadata-only inspection should preserve the detected channel count.");
        require(wavInspection.metadata.frameCount == wavResult.sample.metadata.frameCount,
                "Metadata-only inspection should preserve the detected frame count.");
        require(wavInspection.metadata.rootMidiNotePresent,
                "Metadata-only inspection should preserve the embedded root midi note.");
        require(wavInspection.metadata.rootMidiNote == 69,
                "Metadata-only inspection root midi note changed unexpectedly.");
        require(wavInspection.metadata.loopRangePresent,
                "Metadata-only inspection should preserve the loop range.");
        require(wavInspection.metadata.loopStartFrame == 64,
                "Metadata-only inspection loop start changed unexpectedly.");
        require(wavInspection.metadata.loopEndFrame == 192,
                "Metadata-only inspection loop end changed unexpectedly.");
        require(wavInspection.warnings.empty(),
                "Metadata-only inspection should not invent policy warnings for the WAV fixture.");

        const auto wavInspectionIoCounters = drs::engine::getSampleImportIoCounters();
        require(wavInspectionIoCounters.readerOpenCount == 1,
                "Metadata-only inspection should record one reader-open attempt.");
        require(wavInspectionIoCounters.fingerprintOpenCount == 1,
                "Metadata-only inspection should record one fingerprint-stream open.");
        require(wavInspectionIoCounters.bytesReadCount > 0,
                "Metadata-only inspection should record source bytes read while fingerprinting.");
        require(wavInspectionIoCounters.fullFrameReadCount == 0,
                "Metadata-only inspection must not perform a full-frame decode read.");
        require(wavInspectionIoCounters.copyCount == 0,
                "Metadata-only inspection should not record copy operations.");
        require(wavInspectionIoCounters.peakChunkReadCount == 0,
                "Metadata-only inspection should not report waveform peak chunk reads.");

        const auto wavInspectionHeuristics = drs::engine::parseSampleFilenameHeuristics(
            wavPath.generic_string(),
            &wavInspection.metadata);
        require(wavInspectionHeuristics.suggestedZone.zone.rootKey == 69,
                "Filename heuristics should continue to use inspected metadata for the root key.");
        require(wavInspectionHeuristics.suggestedZone.zone.loopEnabled,
                "Filename heuristics should continue to use inspected metadata for loop visibility.");
        require(wavInspectionHeuristics.suggestedZone.zone.loopStartFrame == 64,
                "Filename heuristics loop-start projection changed unexpectedly.");
        require(wavInspectionHeuristics.suggestedZone.zone.loopEndFrame == 192,
                "Filename heuristics loop-end projection changed unexpectedly.");

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

        drs::engine::resetSampleImportIoCounters();
        const auto awkwardInspection = drs::engine::inspectSampleFile(awkwardNamePath.generic_string());
        require(awkwardInspection.inspected,
                "Awkward-but-decodable WAV fixture should still support metadata-only inspection.");
        require(awkwardInspection.accepted,
                "Awkward-but-decodable WAV fixture should remain policy-accepted with warnings.");
        require(!awkwardInspection.warnings.empty(),
                "Metadata-only inspection should preserve the awkward-name policy warning.");
        requireAnyContains(awkwardInspection.warnings,
                           "portable sample names",
                           "Metadata-only inspection warning should explain the naming policy.");
        require(drs::engine::getSampleImportIoCounters().fullFrameReadCount == 0,
                "Metadata-only inspection warning paths must not perform a full-frame decode.");

        const auto aiffResult = drs::engine::importSampleFile(aiffPath.generic_string());
        require(!aiffResult.imported, "AIFF fixture should be rejected by the Phase 1 format policy.");
        requireAnyContains(aiffResult.issues,
                           "WAV and FLAC",
                           "AIFF policy rejection should explain the supported Phase 1 formats.");

        drs::engine::resetSampleImportIoCounters();
        const auto aiffInspection = drs::engine::inspectSampleFile(aiffPath.generic_string());
        require(aiffInspection.inspected,
                "AIFF fixture should still yield metadata-only inspection facts before policy rejection.");
        require(!aiffInspection.accepted,
                "AIFF fixture should remain rejected by the Phase 1 format policy.");
        require(aiffInspection.metadata.formatName == "AIFF file",
                "Metadata-only inspection should preserve the AIFF format name.");
        requireAnyContains(aiffInspection.issues,
                           "WAV and FLAC",
                           "Metadata-only inspection should preserve the supported-format policy rejection.");
        require(drs::engine::getSampleImportIoCounters().fullFrameReadCount == 0,
                "Rejected metadata-only inspection must not perform a full-frame decode.");

        const auto highRateResult = drs::engine::importSampleFile(highRatePath.generic_string());
        require(highRateResult.imported, "96 kHz fixture should import and remain usable under the sample-rate warning policy.");
        requireAnyContains(highRateResult.warnings,
                           "prefers 44100 Hz or 48000 Hz",
                           "Sample-rate policy warning should explain the preferred Phase 1 rates.");

        drs::engine::resetSampleImportIoCounters();
        const auto highRateInspection = drs::engine::inspectSampleFile(highRatePath.generic_string());
        require(highRateInspection.inspected,
                "96 kHz fixture should still yield metadata-only inspection facts under the warning-only sample-rate policy.");
        require(highRateInspection.accepted,
                "96 kHz fixture should remain accepted by metadata-only inspection.");
        require(highRateInspection.metadata.sampleRate == 96000.0,
                "Metadata-only inspection should preserve the detected unusual sample rate.");
        requireAnyContains(highRateInspection.warnings,
                           "prefers 44100 Hz or 48000 Hz",
                           "Metadata-only inspection should surface the preferred sample-rate warning.");
        require(drs::engine::getSampleImportIoCounters().fullFrameReadCount == 0,
                "Metadata-only inspection warning paths must not perform a full-frame decode.");

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

        const auto copiedPath = scratchDirectory / "copied-source.wav";
        fs::remove(copiedPath);
        drs::engine::resetSampleImportIoCounters();
        require(drs::engine::copySampleFileForImport(wavPath.generic_string(), copiedPath.generic_string()),
                "The counted sample-copy helper should preserve the current copy behavior.");
        const auto copyCounters = drs::engine::getSampleImportIoCounters();
        require(copyCounters.copyCount == 1,
                "Import instrumentation should record one copy attempt.");
        require(fs::exists(copiedPath),
                "The counted sample-copy helper should still materialize the destination file.");

        drs::engine::resetSampleImportIoCounters();
        drs::engine::recordWaveformPeakChunkRead(256, 2);
        const auto peakCounters = drs::engine::getSampleImportIoCounters();
        require(peakCounters.peakChunkReadCount == 1,
                "Peak-chunk instrumentation should record bounded waveform chunk reads.");

        {
            NullReaderHooks nullReaderHooks;
            drs::engine::ScopedSampleImportHooksOverride scope(nullReaderHooks);
            drs::engine::resetSampleImportIoCounters();
            const auto injectedReaderResult = drs::engine::importSampleFile(wavPath.generic_string());
            require(!injectedReaderResult.imported,
                    "A null-reader seam should let tests force the unsupported-format path.");
            require(injectedReaderResult.state == "Sample format unsupported",
                    "A null-reader seam should preserve the unsupported-format disposition.");
            const auto injectedReaderCounters = drs::engine::getSampleImportIoCounters();
            require(injectedReaderCounters.readerOpenCount == 1,
                    "The injected reader seam should still record the attempted reader open.");
            require(injectedReaderCounters.fullFrameReadCount == 0,
                    "The injected reader seam should avoid any full-frame decode read.");
        }

        {
            FailingCopyHooks failingCopyHooks;
            drs::engine::ScopedSampleImportHooksOverride scope(failingCopyHooks);
            drs::engine::resetSampleImportIoCounters();
            require(!drs::engine::copySampleFileForImport(wavPath.generic_string(),
                                                          (scratchDirectory / "copy-failure.wav").generic_string()),
                    "An injected copy seam should let tests force copy failure deterministically.");
            const auto failingCopyCounters = drs::engine::getSampleImportIoCounters();
            require(failingCopyCounters.copyCount == 1,
                    "The injected copy seam should still record the attempted copy.");
        }

        std::cout << "Phase 1 sample import tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 sample import tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
