#include "drs/engine/SampleImport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cctype>
#include <limits>
#include <memory>
#include <sstream>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

void addIssue(SampleImportResult& result, const std::string& issue)
{
    result.issues.push_back(issue);
}

void addWarning(SampleImportResult& result, const std::string& warning)
{
    result.warnings.push_back(warning);
}

std::string toDisplayPath(const fs::path& path)
{
    return path.lexically_normal().generic_string();
}

std::string computeFnv1aChecksumHex(const fs::path& path)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::ifstream input(path, std::ios::binary);
    if (!input.good())
        return {};

    std::uint64_t hash = offsetBasis;
    char buffer[4096];

    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
    {
        for (std::streamsize index = 0; index < input.gcount(); ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= prime;
        }
    }

    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

juce::AudioFormatManager& getAudioFormatManager()
{
    static juce::AudioFormatManager manager;
    static const bool initialized = []()
    {
        manager.registerBasicFormats();
        return true;
    }();

    juce::ignoreUnused(initialized);
    return manager;
}

bool tryReadIntMetadata(const juce::StringPairArray& metadataValues,
                        const juce::String& key,
                        int& value)
{
    if (!metadataValues.containsKey(key))
        return false;

    value = metadataValues[key].getIntValue();
    return true;
}

void populateOptionalMetadata(const juce::AudioFormatReader& reader, ImportedSampleMetadata& metadata)
{
    int rootMidiNote = 0;
    if (tryReadIntMetadata(reader.metadataValues, "MidiUnityNote", rootMidiNote))
    {
        metadata.rootMidiNotePresent = true;
        metadata.rootMidiNote = rootMidiNote;
    }

    int numSampleLoops = 0;
    if (tryReadIntMetadata(reader.metadataValues, "NumSampleLoops", numSampleLoops) && numSampleLoops > 0)
    {
        int loopStart = 0;
        int loopEnd = 0;

        if (tryReadIntMetadata(reader.metadataValues, "Loop0Start", loopStart)
            && tryReadIntMetadata(reader.metadataValues, "Loop0End", loopEnd)
            && loopStart <= loopEnd)
        {
            metadata.loopRangePresent = true;
            metadata.loopStartFrame = static_cast<std::uint64_t>(loopStart);
            metadata.loopEndFrame = static_cast<std::uint64_t>(loopEnd);
        }
    }
}

void addPolicyError(SampleImportPolicyReport& report, const std::string& error)
{
    report.errors.push_back(error);
}

void addPolicyWarning(SampleImportPolicyReport& report, const std::string& warning)
{
    report.warnings.push_back(warning);
}

bool isPhase1SupportedSampleRate(double sampleRate)
{
    return sampleRate == 44100.0 || sampleRate == 48000.0;
}

bool isPhase1SupportedChannelCount(std::uint32_t channelCount)
{
    return channelCount == 1 || channelCount == 2;
}

bool hasOnlyPhase1PortableFilenameCharacters(const std::string& filenameStem)
{
    for (const char character : filenameStem)
    {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0)
            continue;

        if (character == '_' || character == '-')
            continue;

        return false;
    }

    return true;
}

bool pathLivesWithinDirectory(const fs::path& candidatePath, const fs::path& rootPath)
{
    const auto relative = candidatePath.lexically_relative(rootPath);
    if (relative.empty())
        return false;

    const auto relativeText = relative.generic_string();
    return relativeText != ".." && relativeText.rfind("../", 0) != 0;
}
} // namespace

SampleImportPolicyReport evaluatePhase1SamplePolicy(const ImportedSampleMetadata& metadata,
                                                    const std::string& contentRootPath)
{
    SampleImportPolicyReport report;
    report.state = "Phase 1 sample policy not evaluated";

    const fs::path sourcePath(metadata.sourcePath);
    const auto formatName = metadata.formatName;

    if (formatName != "WAV file" && formatName != "FLAC file")
    {
        addPolicyError(report,
                       "Phase 1 only supports WAV and FLAC source assets; importer decoded unsupported format '"
                           + formatName + "'.");
    }

    if (!isPhase1SupportedSampleRate(metadata.sampleRate))
    {
        addPolicyError(report,
                       "Phase 1 only supports 44100 Hz and 48000 Hz source assets; sample reported "
                           + std::to_string(static_cast<int>(metadata.sampleRate)) + " Hz.");
    }

    if (!isPhase1SupportedChannelCount(metadata.channelCount))
    {
        addPolicyError(report,
                       "Phase 1 only supports mono and stereo source assets; sample reported "
                           + std::to_string(metadata.channelCount) + " channels.");
    }

    const auto filenameStem = sourcePath.stem().generic_string();
    if (filenameStem.empty())
    {
        addPolicyError(report, "Phase 1 source assets must have a non-empty filename stem.");
    }
    else if (!hasOnlyPhase1PortableFilenameCharacters(filenameStem))
    {
        addPolicyWarning(report,
                         "Phase 1 recommends portable sample names using only letters, digits, underscores, and hyphens; '"
                             + filenameStem + "' may be harder to reuse across tools.");
    }

    if (!contentRootPath.empty())
    {
        const fs::path contentRoot(contentRootPath);
        const auto normalizedContentRoot = contentRoot.lexically_normal();
        const auto normalizedSourcePath = sourcePath.lexically_normal();
        const auto samplesRoot = (normalizedContentRoot / "Samples").lexically_normal();

        if (!normalizedSourcePath.is_absolute())
        {
            addPolicyError(report,
                           "Compile-time sample sources must resolve to absolute paths before Phase 1 layout policy can run.");
        }
        else if (!pathLivesWithinDirectory(normalizedSourcePath, normalizedContentRoot))
        {
            addPolicyError(report,
                           "Phase 1 compile inputs must live under the configured content root: "
                               + normalizedContentRoot.generic_string());
        }
        else if (!pathLivesWithinDirectory(normalizedSourcePath, samplesRoot))
        {
            addPolicyError(report,
                           "Phase 1 compile inputs must live under the content-root Samples directory: "
                               + samplesRoot.generic_string());
        }
    }

    report.accepted = report.errors.empty();
    report.state = report.accepted ? "Phase 1 sample policy accepted" : "Phase 1 sample policy rejected";
    return report;
}

SampleImportResult importSampleFile(const std::string& samplePath)
{
    SampleImportResult result;
    result.sourcePath = samplePath;
    result.state = "Sample import not attempted";

    const fs::path sampleFsPath(samplePath);
    if (!fs::exists(sampleFsPath))
    {
        result.state = "Sample missing";
        addIssue(result, "Sample file was not found at " + samplePath + ".");
        return result;
    }

    result.fileFound = true;

    auto reader = std::unique_ptr<juce::AudioFormatReader>(getAudioFormatManager().createReaderFor(juce::File(sampleFsPath.generic_string())));
    if (reader == nullptr)
    {
        result.state = "Sample format unsupported";
        addIssue(result, "Sample file could not be decoded as a supported audio format: " + toDisplayPath(sampleFsPath));
        return result;
    }

    if (reader->lengthInSamples < 0)
    {
        result.state = "Sample length invalid";
        addIssue(result, "Decoded sample reported a negative frame length.");
        return result;
    }

    if (reader->lengthInSamples > static_cast<juce::int64>(std::numeric_limits<int>::max()))
    {
        result.state = "Sample too large";
        addIssue(result, "Decoded sample exceeds the current importer frame limit.");
        return result;
    }

    if (reader->numChannels == 0)
    {
        result.state = "Sample channel count invalid";
        addIssue(result, "Decoded sample reported zero channels.");
        return result;
    }

    auto& metadata = result.sample.metadata;
    metadata.sourcePath = toDisplayPath(sampleFsPath);
    metadata.formatName = reader->getFormatName().toStdString();
    metadata.sourceChecksumHex = computeFnv1aChecksumHex(sampleFsPath);
    metadata.channelLayout = reader->getChannelLayout().getDescription().toStdString();
    metadata.sampleRate = reader->sampleRate;
    metadata.frameCount = static_cast<std::uint64_t>(reader->lengthInSamples);
    metadata.channelCount = static_cast<std::uint32_t>(reader->numChannels);
    metadata.bitsPerSample = reader->bitsPerSample;
    metadata.usesFloatingPointData = reader->usesFloatingPointData;
    metadata.durationSeconds = metadata.sampleRate > 0.0
        ? static_cast<double>(metadata.frameCount) / metadata.sampleRate
        : 0.0;

    populateOptionalMetadata(*reader, metadata);

    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                    static_cast<int>(reader->lengthInSamples));
    buffer.clear();

    if (!reader->read(&buffer,
                      0,
                      static_cast<int>(reader->lengthInSamples),
                      0,
                      true,
                      true))
    {
        result.state = "Sample read failed";
        addIssue(result, "Decoded sample reader failed while loading sample frames.");
        return result;
    }

    result.sample.normalizedChannels.resize(buffer.getNumChannels());
    for (int channelIndex = 0; channelIndex < buffer.getNumChannels(); ++channelIndex)
    {
        auto& channel = result.sample.normalizedChannels[static_cast<std::size_t>(channelIndex)];
        channel.assign(buffer.getReadPointer(channelIndex),
                       buffer.getReadPointer(channelIndex) + buffer.getNumSamples());
    }

    result.imported = true;
    const auto policyReport = evaluatePhase1SamplePolicy(metadata);
    for (const auto& warning : policyReport.warnings)
        addWarning(result, warning);

    if (!policyReport.accepted)
    {
        result.imported = false;
        result.state = policyReport.state;
        for (const auto& error : policyReport.errors)
            addIssue(result, error);
        return result;
    }

    result.state = result.warnings.empty() ? "Sample imported" : "Sample imported with warnings";
    return result;
}
} // namespace drs::engine
