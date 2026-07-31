#pragma once

#include "drs/engine/SampleImport.h"

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <iosfwd>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include <juce_audio_formats/juce_audio_formats.h>

namespace drs::tests
{
struct GeneratedWavImportBatchCorpus
{
    std::filesystem::path cleanSiblingOnePath;
    std::filesystem::path cleanPath;
    std::filesystem::path cleanSiblingThreePath;
    std::filesystem::path ambiguousPath;
    std::filesystem::path conflictPath;
    std::filesystem::path policyWarningPath;
    std::filesystem::path canceledPath;
    std::filesystem::path unsupportedPath;
    std::filesystem::path missingPath;
    std::filesystem::path sparseOnePath;
    std::filesystem::path sparseThreePath;
};

GeneratedWavImportBatchCorpus createGeneratedWavImportBatchCorpus(const std::filesystem::path& root);

class OperationPauseGate
{
public:
    void arm();
    void release();
    bool waitUntilBlocked(std::chrono::milliseconds timeout);
    void waitIfArmed() const;

private:
    mutable std::mutex mutex;
    mutable std::condition_variable condition;
    mutable bool armed = false;
    mutable bool blocked = false;
    mutable bool released = false;
};

struct SyntheticSampleReaderFixture
{
    std::string samplePath;
    std::string formatName = "WAV file";
    double sampleRate = 48000.0;
    std::int64_t frameCount = 480;
    std::uint32_t channelCount = 2;
    std::uint32_t bitsPerSample = 32;
    bool usesFloatingPointData = true;
    juce::StringPairArray metadata;
    bool failRead = false;
};

class DeterministicSampleImportHooks final : public engine::SampleImportHooks
{
public:
    OperationPauseGate& copyGate() noexcept { return copyGateState; }
    OperationPauseGate& fingerprintGate() noexcept { return fingerprintGateState; }
    OperationPauseGate& readGate() noexcept { return readGateState; }

    void addReaderFixture(SyntheticSampleReaderFixture fixture);
    void setFingerprintBytes(const std::string& samplePath, std::string bytes);
    void failCopyFor(const std::string& sourcePath);

    bool fileExists(const std::string& samplePath) const override;
    bool copyFile(const std::string& sourcePath, const std::string& destinationPath) const override;
    std::unique_ptr<std::istream> openFingerprintStream(const std::string& samplePath) const override;
    std::unique_ptr<juce::AudioFormatReader> createAudioReader(const std::string& samplePath) const override;

private:
    OperationPauseGate copyGateState;
    OperationPauseGate fingerprintGateState;
    OperationPauseGate readGateState;
    std::unordered_map<std::string, SyntheticSampleReaderFixture> readerFixtures;
    std::unordered_map<std::string, std::string> fingerprintBytes;
    std::set<std::string> failingCopySources;
};
} // namespace drs::tests
