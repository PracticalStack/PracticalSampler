#include "WavImportTestSupport.h"

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace drs::tests
{
namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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

void writeAudioFile(const fs::path& filePath,
                    juce::AudioFormat& format,
                    const juce::AudioBuffer<float>& buffer,
                    const juce::StringPairArray& metadata)
{
    auto fileOutput = std::make_unique<juce::FileOutputStream>(juce::File(filePath.generic_string()));
    require(fileOutput->openedOk(), "Could not open output file for writing: " + filePath.generic_string());
    std::unique_ptr<juce::OutputStream> output = std::move(fileOutput);

    juce::AudioFormatWriterOptions options;
    options = options.withSampleRate(48000.0)
        .withNumChannels(buffer.getNumChannels())
        .withBitsPerSample(24);

    for (const auto& key : metadata.getAllKeys())
        options = options.withMetadata(key, metadata[key]);

    auto writerOwner = format.createWriterFor(output, options);
    require(writerOwner != nullptr, "Could not create audio writer for: " + filePath.generic_string());
    require(writerOwner->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples()),
            "Could not write audio samples to: " + filePath.generic_string());
}

class SyntheticAudioFormatReader final : public juce::AudioFormatReader
{
public:
    SyntheticAudioFormatReader(const SyntheticSampleReaderFixture& nextFixture,
                               const OperationPauseGate* nextReadGate)
        : juce::AudioFormatReader(nullptr, nextFixture.formatName),
          fixture(nextFixture),
          readGate(nextReadGate)
    {
        sampleRate = fixture.sampleRate;
        bitsPerSample = static_cast<int>(fixture.bitsPerSample);
        usesFloatingPointData = fixture.usesFloatingPointData;
        lengthInSamples = fixture.frameCount;
        numChannels = fixture.channelCount;
        metadataValues = fixture.metadata;
    }

    bool readSamples(int* const* destChannels,
                     int numDestChannels,
                     int startOffsetInDestBuffer,
                     juce::int64 startSampleInFile,
                     int numSamples) override
    {
        if (readGate != nullptr)
            readGate->waitIfArmed();

        clearSamplesBeyondAvailableLength(destChannels,
                                          numDestChannels,
                                          startOffsetInDestBuffer,
                                          startSampleInFile,
                                          numSamples,
                                          lengthInSamples);

        if (fixture.failRead)
            return false;

        if (numSamples <= 0)
            return true;

        for (int channelIndex = 0; channelIndex < numDestChannels; ++channelIndex)
        {
            static_assert(sizeof(int) == sizeof(float),
                          "Int and float size must match in order for pointer arithmetic to work correctly");

            auto* dest = reinterpret_cast<float*>(destChannels[channelIndex]);
            if (dest == nullptr)
                continue;

            dest += startOffsetInDestBuffer;
            for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
            {
                const auto absoluteSample = static_cast<float>(startSampleInFile + sampleIndex);
                const auto phase = absoluteSample / 97.0f;
                const auto amplitude = 0.2f + (0.1f * static_cast<float>(channelIndex));
                dest[sampleIndex] = std::sin(phase + (0.15f * static_cast<float>(channelIndex))) * amplitude;
            }
        }

        return true;
    }

private:
    SyntheticSampleReaderFixture fixture;
    const OperationPauseGate* readGate = nullptr;
};
} // namespace

GeneratedWavImportBatchCorpus createGeneratedWavImportBatchCorpus(const fs::path& root)
{
    fs::create_directories(root);

    GeneratedWavImportBatchCorpus corpus;
    corpus.cleanSiblingOnePath = root / "Pad_Sustain_C4_vel064_rr1.wav";
    corpus.cleanPath = root / "Pad_Sustain_C4_vel064_rr2.wav";
    corpus.cleanSiblingThreePath = root / "Pad_Sustain_C4_vel064_rr3.wav";
    corpus.ambiguousPath = root / "MysteryTexture.wav";
    corpus.conflictPath = root / "Lead_A4.wav";
    corpus.policyWarningPath = root / "Shimmer_Bad Name!.wav";
    corpus.canceledPath = root / "Canceled_C3.wav";
    corpus.unsupportedPath = root / "Unsupported.txt";
    corpus.missingPath = root / "Missing_C4.wav";
    corpus.sparseOnePath = root / "Brush_Sustain_D4_vel096_rr1.wav";
    corpus.sparseThreePath = root / "Brush_Sustain_D4_vel096_rr3.wav";

    const auto buffer = buildReferenceBuffer();
    juce::WavAudioFormat wavFormat;

    juce::StringPairArray matchingMetadata;
    matchingMetadata.set("MidiUnityNote", "60");
    matchingMetadata.set("NumSampleLoops", "1");
    matchingMetadata.set("Loop0Start", "64");
    matchingMetadata.set("Loop0End", "192");

    juce::StringPairArray conflictingMetadata;
    conflictingMetadata.set("MidiUnityNote", "60");

    writeAudioFile(corpus.cleanSiblingOnePath, wavFormat, buffer, matchingMetadata);
    writeAudioFile(corpus.cleanPath, wavFormat, buffer, matchingMetadata);
    writeAudioFile(corpus.cleanSiblingThreePath, wavFormat, buffer, matchingMetadata);
    writeAudioFile(corpus.ambiguousPath, wavFormat, buffer, {});
    writeAudioFile(corpus.conflictPath, wavFormat, buffer, conflictingMetadata);
    writeAudioFile(corpus.policyWarningPath, wavFormat, buffer, {});
    writeAudioFile(corpus.canceledPath, wavFormat, buffer, matchingMetadata);
    writeAudioFile(corpus.sparseOnePath, wavFormat, buffer, matchingMetadata);
    writeAudioFile(corpus.sparseThreePath, wavFormat, buffer, matchingMetadata);

    {
        juce::FileOutputStream unsupportedOutput(juce::File(corpus.unsupportedPath.generic_string()));
        require(unsupportedOutput.openedOk(), "Could not create unsupported text fixture.");
        unsupportedOutput.writeText("not audio", false, false, nullptr);
    }

    return corpus;
}

void OperationPauseGate::arm()
{
    std::lock_guard<std::mutex> lock(mutex);
    armed = true;
    blocked = false;
    released = false;
}

void OperationPauseGate::release()
{
    std::lock_guard<std::mutex> lock(mutex);
    released = true;
    condition.notify_all();
}

bool OperationPauseGate::waitUntilBlocked(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> lock(mutex);
    return condition.wait_for(lock, timeout, [&] { return blocked; });
}

void OperationPauseGate::waitIfArmed() const
{
    std::unique_lock<std::mutex> lock(mutex);
    if (!armed)
        return;

    blocked = true;
    condition.notify_all();
    condition.wait(lock, [&] { return released; });
    armed = false;
}

void DeterministicSampleImportHooks::addReaderFixture(SyntheticSampleReaderFixture fixture)
{
    readerFixtures[fixture.samplePath] = std::move(fixture);
}

void DeterministicSampleImportHooks::setFingerprintBytes(const std::string& samplePath, std::string bytes)
{
    fingerprintBytes[samplePath] = std::move(bytes);
}

void DeterministicSampleImportHooks::failCopyFor(const std::string& sourcePath)
{
    failingCopySources.insert(sourcePath);
}

bool DeterministicSampleImportHooks::fileExists(const std::string& samplePath) const
{
    return readerFixtures.count(samplePath) > 0
        || fingerprintBytes.count(samplePath) > 0
        || engine::SampleImportHooks::fileExists(samplePath);
}

bool DeterministicSampleImportHooks::copyFile(const std::string& sourcePath, const std::string& destinationPath) const
{
    copyGateState.waitIfArmed();
    if (failingCopySources.count(sourcePath) > 0)
        return false;

    return engine::SampleImportHooks::copyFile(sourcePath, destinationPath);
}

std::unique_ptr<std::istream> DeterministicSampleImportHooks::openFingerprintStream(const std::string& samplePath) const
{
    fingerprintGateState.waitIfArmed();

    if (const auto iterator = fingerprintBytes.find(samplePath); iterator != fingerprintBytes.end())
        return std::make_unique<std::istringstream>(iterator->second);

    return engine::SampleImportHooks::openFingerprintStream(samplePath);
}

std::unique_ptr<juce::AudioFormatReader> DeterministicSampleImportHooks::createAudioReader(const std::string& samplePath) const
{
    if (const auto iterator = readerFixtures.find(samplePath); iterator != readerFixtures.end())
    {
        return std::make_unique<SyntheticAudioFormatReader>(iterator->second, &readGateState);
    }

    return engine::SampleImportHooks::createAudioReader(samplePath);
}
} // namespace drs::tests
