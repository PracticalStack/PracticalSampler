#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct ImportedSampleMetadata
{
    std::string sourcePath;
    std::string formatName;
    std::string sourceChecksumHex;
    std::string channelLayout;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    std::uint32_t bitsPerSample = 0;
    bool usesFloatingPointData = false;
    double durationSeconds = 0.0;
    bool rootMidiNotePresent = false;
    int rootMidiNote = 60;
    bool loopRangePresent = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
};

struct ImportedSampleData
{
    ImportedSampleMetadata metadata;
    std::vector<std::vector<float>> normalizedChannels;
};

struct SampleImportPolicyReport
{
    bool accepted = false;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
};

struct SampleImportResult
{
    bool fileFound = false;
    bool imported = false;
    std::string sourcePath;
    std::string state;
    std::vector<std::string> warnings;
    std::vector<std::string> issues;
    ImportedSampleData sample;
};

SampleImportPolicyReport evaluatePhase1SamplePolicy(const ImportedSampleMetadata& metadata,
                                                    const std::string& contentRootPath = {});
SampleImportResult importSampleFile(const std::string& samplePath);
} // namespace drs::engine
