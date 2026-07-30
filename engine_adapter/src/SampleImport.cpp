#include "drs/engine/SampleImport.h"

#include <juce_audio_formats/juce_audio_formats.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <unordered_map>

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

void addPolicyError(SampleImportPolicyReport& report, const std::string& error)
{
    report.errors.push_back(error);
}

void addPolicyWarning(SampleImportPolicyReport& report, const std::string& warning)
{
    report.warnings.push_back(warning);
}

void addFinding(std::vector<AuthoringImportFinding>& findings,
                AuthoringImportFindingSeverity severity,
                const std::string& code,
                const std::string& summary,
                const std::string& detail,
                bool requiresConfirmation = false,
                const std::vector<std::string>& relatedTokens = {})
{
    AuthoringImportFinding finding;
    finding.severity = severity;
    finding.code = code;
    finding.summary = summary;
    finding.detail = detail;
    finding.requiresConfirmation = requiresConfirmation;
    finding.relatedTokens = relatedTokens;
    findings.push_back(std::move(finding));
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

std::string computeFnv1a64Hex(const std::string& text)
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    constexpr std::uint64_t prime = 1099511628211ull;

    std::uint64_t hash = offsetBasis;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= prime;
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

std::string toLowerAscii(const std::string& text)
{
    std::string lowered = text;
    std::transform(lowered.begin(),
                   lowered.end(),
                   lowered.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(std::tolower(character));
                   });
    return lowered;
}

std::string slugify(const std::string& text)
{
    std::string slug;
    bool previousWasDash = false;

    for (const char character : text)
    {
        if (std::isalnum(static_cast<unsigned char>(character)) != 0)
        {
            slug.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
            previousWasDash = false;
            continue;
        }

        if (!previousWasDash)
        {
            slug.push_back('-');
            previousWasDash = true;
        }
    }

    while (!slug.empty() && slug.front() == '-')
        slug.erase(slug.begin());
    while (!slug.empty() && slug.back() == '-')
        slug.pop_back();

    return slug.empty() ? "sample" : slug;
}

std::string toDisplayName(const std::string& filenameStem)
{
    std::string displayName;
    bool previousWasSpace = false;

    for (const char character : filenameStem)
    {
        const auto isWordCharacter = std::isalnum(static_cast<unsigned char>(character)) != 0
            || character == '#'
            || character == 'b';

        if (isWordCharacter)
        {
            displayName.push_back(character);
            previousWasSpace = false;
        }
        else if (!previousWasSpace)
        {
            displayName.push_back(' ');
            previousWasSpace = true;
        }
    }

    while (!displayName.empty() && displayName.front() == ' ')
        displayName.erase(displayName.begin());
    while (!displayName.empty() && displayName.back() == ' ')
        displayName.pop_back();

    return displayName.empty() ? filenameStem : displayName;
}

std::vector<std::string> splitFilenameStem(const std::string& filenameStem)
{
    std::vector<std::string> tokens;
    std::string current;

    for (const char character : filenameStem)
    {
        const auto isSeparator = std::isspace(static_cast<unsigned char>(character)) != 0
            || character == '_'
            || character == '-'
            || character == '.'
            || character == '('
            || character == ')'
            || character == '['
            || character == ']';

        if (isSeparator)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }

            continue;
        }

        current.push_back(character);
    }

    if (!current.empty())
        tokens.push_back(current);

    return tokens;
}

std::optional<int> parseMidiNoteToken(const std::string& token);
std::optional<int> parseVelocityToken(const std::string& token);
std::optional<int> parseRoundRobinToken(const std::string& token);
std::optional<std::string> parseArticulationToken(const std::string& token);
std::string noteTokenCanonicalName(int midiNote);

struct ParsedFilenameContext
{
    std::vector<SampleFilenameToken> tokens;
    std::optional<int> detectedVelocity;
    std::optional<int> detectedRoundRobin;
    std::optional<std::string> detectedArticulation;
};

ParsedFilenameContext parseFilenameContext(const std::string& filenameStem)
{
    ParsedFilenameContext context;
    const auto rawTokens = splitFilenameStem(filenameStem);

    for (const auto& rawToken : rawTokens)
    {
        SampleFilenameToken token;
        token.text = rawToken;
        token.normalizedText = toLowerAscii(rawToken);
        token.kind = SampleFilenameTokenKind::text;

        if (const auto rootKey = parseMidiNoteToken(rawToken); rootKey.has_value())
        {
            token.kind = SampleFilenameTokenKind::rootNote;
            token.numericValue = *rootKey;
            token.canonicalValue = noteTokenCanonicalName(*rootKey);
        }
        else if (const auto velocity = parseVelocityToken(rawToken); velocity.has_value())
        {
            token.kind = SampleFilenameTokenKind::velocity;
            token.numericValue = *velocity;
            token.canonicalValue = std::to_string(*velocity);
            if (!context.detectedVelocity.has_value())
                context.detectedVelocity = *velocity;
        }
        else if (const auto roundRobin = parseRoundRobinToken(rawToken); roundRobin.has_value())
        {
            token.kind = SampleFilenameTokenKind::roundRobin;
            token.numericValue = *roundRobin;
            token.canonicalValue = std::to_string(*roundRobin);
            if (!context.detectedRoundRobin.has_value())
                context.detectedRoundRobin = *roundRobin;
        }
        else if (const auto articulation = parseArticulationToken(rawToken); articulation.has_value())
        {
            token.kind = SampleFilenameTokenKind::articulation;
            token.canonicalValue = *articulation;
            context.detectedArticulation = *articulation;
        }

        context.tokens.push_back(std::move(token));
    }

    return context;
}

std::string buildRoundRobinGroupSignature(const std::vector<SampleFilenameToken>& tokens)
{
    std::ostringstream stream;
    for (const auto& token : tokens)
    {
        if (token.kind == SampleFilenameTokenKind::roundRobin)
            continue;

        switch (token.kind)
        {
            case SampleFilenameTokenKind::rootNote:
                stream << "root:" << token.canonicalValue;
                break;
            case SampleFilenameTokenKind::velocity:
                stream << "velocity:" << token.canonicalValue;
                break;
            case SampleFilenameTokenKind::articulation:
                stream << "articulation:" << token.canonicalValue;
                break;
            case SampleFilenameTokenKind::text:
            case SampleFilenameTokenKind::unknown:
                stream << "text:" << token.normalizedText;
                break;
            case SampleFilenameTokenKind::roundRobin:
                break;
        }

        stream << "|";
    }

    return stream.str();
}

void applyRoundRobinDescriptor(RuntimeProjectZoneDefinition& zone,
                               const std::string& poolId,
                               int slotCount,
                               int slotIndex)
{
    zone.roundRobin = RoundRobinDescriptor {
        poolId,
        slotCount,
        slotIndex,
        RoundRobinMode::sequential
    };
    zone.roundRobinLength = slotCount;
    zone.roundRobinPosition = slotIndex;
}

void inferRoundRobinPoolFromSiblings(const fs::path& samplePath,
                                     const ParsedFilenameContext& context,
                                     RuntimeProjectZoneDefinition& zone,
                                     std::vector<AuthoringImportFinding>& findings)
{
    if (!context.detectedRoundRobin.has_value())
        return;

    const auto parentPath = samplePath.parent_path();
    if (parentPath.empty() || !fs::exists(parentPath))
        return;

    const auto groupSignature = buildRoundRobinGroupSignature(context.tokens);
    std::set<int> uniqueSlots;
    std::unordered_map<int, int> slotCounts;

    for (const auto& entry : fs::directory_iterator(parentPath))
    {
        if (!entry.is_regular_file())
            continue;

        const auto siblingPath = entry.path();
        if (toLowerAscii(siblingPath.extension().generic_string())
            != toLowerAscii(samplePath.extension().generic_string()))
        {
            continue;
        }

        const auto siblingContext = parseFilenameContext(siblingPath.stem().generic_string());
        if (!siblingContext.detectedRoundRobin.has_value())
            continue;

        if (buildRoundRobinGroupSignature(siblingContext.tokens) != groupSignature)
            continue;

        uniqueSlots.insert(*siblingContext.detectedRoundRobin);
        ++slotCounts[*siblingContext.detectedRoundRobin];
    }

    if (uniqueSlots.empty())
        return;

    std::vector<std::string> relatedTokens;
    relatedTokens.reserve(uniqueSlots.size());
    for (const auto slot : uniqueSlots)
        relatedTokens.push_back("rr" + std::to_string(slot));

    const auto highestSlot = *uniqueSlots.rbegin();
    const auto hasDuplicateSlot = std::any_of(slotCounts.begin(),
                                              slotCounts.end(),
                                              [](const auto& entry)
                                              {
                                                  return entry.second > 1;
                                              });
    const bool contiguous = *uniqueSlots.begin() == 1
        && static_cast<int>(uniqueSlots.size()) == highestSlot;

    if (hasDuplicateSlot)
    {
        addFinding(findings,
                   AuthoringImportFindingSeverity::warning,
                   "round_robin.conflicting_group",
                   "Round-robin slot mapping conflicts across sibling files",
                   "Sibling files inferred multiple samples for the same round-robin slot. Confirm or regroup these files before accepting the draft zone.",
                   true,
                   relatedTokens);
        return;
    }

    if (!contiguous || highestSlot <= 1)
    {
        addFinding(findings,
                   AuthoringImportFindingSeverity::warning,
                   "round_robin.sparse_slots",
                   "Round-robin sibling pool is incomplete",
                   "Sibling files exposed round-robin slots that do not form a contiguous 1-based pool, so the importer left the draft zone ungrouped for review.",
                   true,
                   relatedTokens);
        return;
    }

    applyRoundRobinDescriptor(zone,
                              "rr-import-" + computeFnv1a64Hex(parentPath.lexically_normal().generic_string()
                                                               + "|" + groupSignature),
                              highestSlot,
                              *context.detectedRoundRobin);
    addFinding(findings,
               AuthoringImportFindingSeverity::info,
               "round_robin.inferred",
               "Inferred round-robin pool from sibling files",
               "Sibling files resolved a " + std::to_string(highestSlot)
                   + "-slot sequential round-robin pool for this draft zone.",
               false,
               relatedTokens);
}

std::optional<int> parseMidiNoteToken(const std::string& token)
{
    if (token.size() < 2 || token.size() > 4)
        return std::nullopt;

    const auto lowered = toLowerAscii(token);
    const char noteLetter = lowered[0];

    int semitone = 0;
    switch (noteLetter)
    {
    case 'c': semitone = 0; break;
    case 'd': semitone = 2; break;
    case 'e': semitone = 4; break;
    case 'f': semitone = 5; break;
    case 'g': semitone = 7; break;
    case 'a': semitone = 9; break;
    case 'b': semitone = 11; break;
    default:
        return std::nullopt;
    }

    std::size_t octaveStart = 1;
    if (lowered.size() > 2 && (lowered[1] == '#' || lowered[1] == 'b'))
    {
        semitone += lowered[1] == '#' ? 1 : -1;
        octaveStart = 2;
    }

    if (octaveStart >= lowered.size())
        return std::nullopt;

    const auto octaveText = lowered.substr(octaveStart);
    const bool numeric = !octaveText.empty()
        && std::all_of(octaveText.begin(),
                       octaveText.end(),
                       [](char character)
                       {
                           return std::isdigit(static_cast<unsigned char>(character)) != 0
                               || character == '-';
                       });
    if (!numeric)
        return std::nullopt;

    int octave = 0;
    try
    {
        octave = std::stoi(octaveText);
    }
    catch (...)
    {
        return std::nullopt;
    }

    const int midiNote = ((octave + 1) * 12) + semitone;
    if (midiNote < 0 || midiNote > 127)
        return std::nullopt;

    return midiNote;
}

std::optional<int> parseVelocityToken(const std::string& token)
{
    const auto lowered = toLowerAscii(token);

    static const std::unordered_map<std::string, int> dynamicLevels {
        {"pp", 24},
        {"p", 40},
        {"mp", 56},
        {"mf", 88},
        {"f", 108},
        {"ff", 124}
    };

    if (const auto iterator = dynamicLevels.find(lowered); iterator != dynamicLevels.end())
        return iterator->second;

    const auto hasPrefix = [&](const std::string& prefix)
    {
        return lowered.rfind(prefix, 0) == 0 && lowered.size() > prefix.size();
    };

    std::string digits;
    if (hasPrefix("vel"))
        digits = lowered.substr(3);
    else if (hasPrefix("velocity"))
        digits = lowered.substr(8);
    else if (hasPrefix("v"))
        digits = lowered.substr(1);
    else
        return std::nullopt;

    if (!std::all_of(digits.begin(), digits.end(), [](char character) { return std::isdigit(static_cast<unsigned char>(character)) != 0; }))
        return std::nullopt;

    const int velocity = std::stoi(digits);
    if (velocity < 1 || velocity > 127)
        return std::nullopt;

    return velocity;
}

std::optional<int> parseRoundRobinToken(const std::string& token)
{
    const auto lowered = toLowerAscii(token);

    const auto hasPrefix = [&](const std::string& prefix)
    {
        return lowered.rfind(prefix, 0) == 0 && lowered.size() > prefix.size();
    };

    std::string digits;
    if (hasPrefix("rr"))
        digits = lowered.substr(2);
    else if (hasPrefix("take"))
        digits = lowered.substr(4);
    else if (hasPrefix("roundrobin"))
        digits = lowered.substr(10);
    else
        return std::nullopt;

    if (!std::all_of(digits.begin(), digits.end(), [](char character) { return std::isdigit(static_cast<unsigned char>(character)) != 0; }))
        return std::nullopt;

    const int roundRobinIndex = std::stoi(digits);
    return roundRobinIndex > 0 ? std::optional<int>(roundRobinIndex) : std::nullopt;
}

std::optional<std::string> parseArticulationToken(const std::string& token)
{
    const auto lowered = toLowerAscii(token);

    static const std::unordered_map<std::string, std::string> articulationAliases {
        {"sus", "sustain"},
        {"sustain", "sustain"},
        {"leg", "legato"},
        {"legato", "legato"},
        {"stac", "staccato"},
        {"staccato", "staccato"},
        {"spic", "spiccato"},
        {"spiccato", "spiccato"},
        {"pizz", "pizzicato"},
        {"pizzicato", "pizzicato"},
        {"lead", "lead"},
        {"pad", "pad"}
    };

    const auto iterator = articulationAliases.find(lowered);
    if (iterator == articulationAliases.end())
        return std::nullopt;

    return iterator->second;
}

std::string noteTokenCanonicalName(int midiNote)
{
    static const char* noteNames[] = {
        "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
    };

    const auto noteIndex = midiNote % 12;
    const auto octave = (midiNote / 12) - 1;
    return std::string(noteNames[noteIndex]) + std::to_string(octave);
}

std::pair<int, int> velocityBucketRange(int velocity)
{
    const int zeroBased = std::max(0, velocity - 1);
    const int bucket = zeroBased / 32;
    const int low = (bucket * 32) + 1;
    const int high = std::min(low + 31, 127);
    return { low, high };
}

AuthoringImportItemState resolvePostImportState(const AuthoringImportQueueItem& item)
{
    if (!item.importResult.imported)
        return AuthoringImportItemState::failed;

    const auto hasWarnings = std::any_of(item.findings.begin(),
                                         item.findings.end(),
                                         [](const AuthoringImportFinding& finding)
                                         {
                                             return finding.severity == AuthoringImportFindingSeverity::warning
                                                 || finding.severity == AuthoringImportFindingSeverity::error
                                                 || finding.requiresConfirmation;
                                         });
    return hasWarnings ? AuthoringImportItemState::warning : AuthoringImportItemState::inferred;
}

void refreshQueueMetrics(AuthoringImportQueue& queue)
{
    auto& metrics = queue.metrics;
    metrics.totalItemCount = queue.items.size();
    metrics.pendingCount = 0;
    metrics.processedCount = 0;
    metrics.warningItemCount = 0;
    metrics.failedItemCount = 0;
    metrics.canceledItemCount = 0;
    metrics.acceptedItemCount = 0;

    for (const auto& item : queue.items)
    {
        switch (item.state)
        {
        case AuthoringImportItemState::pending:
            ++metrics.pendingCount;
            break;
        case AuthoringImportItemState::parsing:
            ++metrics.pendingCount;
            break;
        case AuthoringImportItemState::inferred:
            ++metrics.processedCount;
            break;
        case AuthoringImportItemState::warning:
            ++metrics.processedCount;
            ++metrics.warningItemCount;
            break;
        case AuthoringImportItemState::failed:
            ++metrics.processedCount;
            ++metrics.failedItemCount;
            break;
        case AuthoringImportItemState::canceled:
            ++metrics.canceledItemCount;
            break;
        case AuthoringImportItemState::accepted:
            ++metrics.processedCount;
            ++metrics.acceptedItemCount;
            break;
        }
    }

    metrics.state = metrics.pendingCount == 0 ? "Authoring import queue drained" : "Authoring import queue active";
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

SampleSourceFingerprintResult fingerprintSampleSourceFile(const std::string& samplePath)
{
    SampleSourceFingerprintResult result;
    result.sourcePath = samplePath;
    result.state = "Source fingerprint not attempted";

    const fs::path sampleFsPath(samplePath);
    if (!fs::exists(sampleFsPath))
    {
        result.state = "Sample missing";
        result.issues.push_back("Sample file was not found at " + samplePath + ".");
        return result;
    }

    result.fileFound = true;
    result.fingerprintHex = computeFnv1aChecksumHex(sampleFsPath);
    if (result.fingerprintHex.empty())
    {
        result.state = "Sample fingerprint failed";
        result.issues.push_back("Sample file could not be read while computing its source fingerprint: "
                                + toDisplayPath(sampleFsPath));
        return result;
    }

    result.fingerprinted = true;
    result.state = "Source fingerprint ready";
    return result;
}

SampleImportResult importSampleFile(const std::string& samplePath,
                                    const std::string& knownFingerprintHex)
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

    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        getAudioFormatManager().createReaderFor(juce::File(sampleFsPath.generic_string())));
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
    metadata.sourceChecksumHex = knownFingerprintHex.empty()
        ? computeFnv1aChecksumHex(sampleFsPath)
        : knownFingerprintHex;
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

ParsedSampleFilenameHeuristics parseSampleFilenameHeuristics(const std::string& samplePath,
                                                             const ImportedSampleMetadata* metadata)
{
    ParsedSampleFilenameHeuristics result;

    const fs::path sourcePath(samplePath);
    const auto filenameStem = sourcePath.stem().generic_string();
    const auto context = parseFilenameContext(filenameStem);
    result.tokens = context.tokens;

    auto& suggestion = result.suggestedZone;
    suggestion.suggested = true;
    suggestion.sourceSampleId = slugify(filenameStem);
    suggestion.zone.id = slugify(filenameStem);
    suggestion.zone.sampleSourceId = suggestion.sourceSampleId;
    suggestion.zone.displayName = toDisplayName(filenameStem);
    suggestion.zone.groupId = context.detectedArticulation.value_or("default-group");
    suggestion.zone.articulationId = context.detectedArticulation.value_or("default");
    suggestion.zone.gainDb = 0.0;
    suggestion.zone.pan = 0.0;
    suggestion.zone.sampleStartFrame = 0;

    if (metadata != nullptr && metadata->loopRangePresent)
    {
        suggestion.zone.loopEnabled = true;
        suggestion.zone.loopStartFrame = metadata->loopStartFrame;
        suggestion.zone.loopEndFrame = metadata->loopEndFrame;
    }

    if (context.detectedArticulation.has_value())
    {
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "articulation.detected",
                   "Detected articulation token",
                   "Filename token inferred articulation '" + *context.detectedArticulation + "'.",
                   false,
                   {*context.detectedArticulation});
    }

    if (context.detectedRoundRobin.has_value())
    {
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "round_robin.detected",
                   "Detected round-robin token",
                   "Filename token inferred round-robin slot " + std::to_string(*context.detectedRoundRobin) + ".",
                   false,
                   {"rr" + std::to_string(*context.detectedRoundRobin)});
        inferRoundRobinPoolFromSiblings(sourcePath, context, suggestion.zone, result.findings);
    }

    const auto rootKeyInference = inferSampleRootKey(samplePath, metadata);
    suggestion.rootKeySource = rootKeyInference.source;
    if (rootKeyInference.resolved)
    {
        suggestion.zone.rootKey = rootKeyInference.rootKey;
        suggestion.zone.keyLow = rootKeyInference.rootKey;
        suggestion.zone.keyHigh = rootKeyInference.rootKey;
    }
    else
    {
        suggestion.zone.rootKey = rootKeyInference.rootKey;
        suggestion.zone.keyLow = 0;
        suggestion.zone.keyHigh = 127;
    }

    result.findings.insert(result.findings.end(),
                           rootKeyInference.findings.begin(),
                           rootKeyInference.findings.end());

    if (context.detectedVelocity.has_value())
    {
        const auto [velocityLow, velocityHigh] = velocityBucketRange(*context.detectedVelocity);
        suggestion.zone.velocityLow = velocityLow;
        suggestion.zone.velocityHigh = velocityHigh;
        suggestion.velocitySource = "filename";
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "velocity.detected",
                   "Detected velocity layer token",
                   "Filename token inferred a velocity layer centered on " + std::to_string(*context.detectedVelocity)
                       + ", mapped to the range " + std::to_string(velocityLow) + "-" + std::to_string(velocityHigh) + ".");
    }
    else
    {
        suggestion.zone.velocityLow = 1;
        suggestion.zone.velocityHigh = 127;
        suggestion.velocitySource = "default";
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "velocity.default",
                   "No velocity token detected",
                   "Filename did not expose a velocity layer token, so the suggestion kept a full 1-127 range.");
    }

    return result;
}

SampleRootKeyInferenceResult inferSampleRootKey(const std::string& samplePath,
                                                const ImportedSampleMetadata* metadata)
{
    SampleRootKeyInferenceResult result;

    const fs::path sourcePath(samplePath);
    const auto rawTokens = splitFilenameStem(sourcePath.stem().generic_string());

    std::optional<int> detectedRootKey;
    for (const auto& rawToken : rawTokens)
    {
        if (const auto rootKey = parseMidiNoteToken(rawToken); rootKey.has_value())
        {
            detectedRootKey = *rootKey;
            break;
        }
    }

    if (detectedRootKey.has_value())
    {
        result.resolved = true;
        result.rootKey = *detectedRootKey;
        result.source = "filename";
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "root_key.filename",
                   "Detected root key from filename",
                   "Filename token inferred root key '" + noteTokenCanonicalName(*detectedRootKey) + "'.",
                   false,
                   {noteTokenCanonicalName(*detectedRootKey)});
    }
    else if (metadata != nullptr && metadata->rootMidiNotePresent)
    {
        result.resolved = true;
        result.rootKey = metadata->rootMidiNote;
        result.source = "metadata";
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::info,
                   "root_key.metadata",
                   "Used embedded root key metadata",
                   "Sample metadata supplied root key '" + noteTokenCanonicalName(metadata->rootMidiNote) + "'.");
    }
    else
    {
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::warning,
                   "root_key.ambiguous",
                   "Root key needs confirmation",
                   "Filename and embedded metadata did not expose a clear root key, so the suggestion kept a full-range draft zone.",
                   true);
    }

    if (metadata != nullptr && metadata->rootMidiNotePresent && detectedRootKey.has_value()
        && metadata->rootMidiNote != *detectedRootKey)
    {
        addFinding(result.findings,
                   AuthoringImportFindingSeverity::warning,
                   "root_key.conflict",
                   "Filename and metadata disagree on root key",
                   "Filename suggested '" + noteTokenCanonicalName(*detectedRootKey)
                       + "' while embedded metadata suggested '"
                       + noteTokenCanonicalName(metadata->rootMidiNote)
                       + "'. The filename inference stayed active, but this item needs confirmation.",
                   true,
                   {noteTokenCanonicalName(*detectedRootKey), noteTokenCanonicalName(metadata->rootMidiNote)});
    }

    return result;
}

AuthoringImportQueue createAuthoringImportQueue(const std::vector<std::string>& samplePaths,
                                                const std::string& contentRootPath)
{
    AuthoringImportQueue queue;
    queue.contentRootPath = contentRootPath;
    queue.items.reserve(samplePaths.size());

    for (std::size_t index = 0; index < samplePaths.size(); ++index)
    {
        const fs::path samplePath(samplePaths[index]);
        AuthoringImportQueueItem item;
        item.id = "import-item-" + std::to_string(index + 1) + "-" + slugify(samplePath.stem().generic_string());
        item.sourcePath = samplePath.generic_string();
        queue.items.push_back(std::move(item));
    }

    refreshQueueMetrics(queue);
    return queue;
}

AuthoringImportProcessResult processNextAuthoringImportQueueItem(AuthoringImportQueue& queue)
{
    using Clock = std::chrono::steady_clock;
    const auto startTime = Clock::now();

    AuthoringImportProcessResult result;
    result.state = "Authoring import queue idle";

    auto iterator = std::find_if(queue.items.begin(),
                                 queue.items.end(),
                                 [](const AuthoringImportQueueItem& item)
                                 {
                                     return item.state == AuthoringImportItemState::pending;
                                 });
    if (iterator == queue.items.end())
    {
        result.queueDrained = true;
        result.state = "Authoring import queue drained";
        refreshQueueMetrics(queue);
        return result;
    }

    auto& item = *iterator;
    item.state = AuthoringImportItemState::parsing;
    item.importResult = importSampleFile(item.sourcePath);
    item.filenameTokens.clear();
    item.findings.clear();
    item.suggestedZone = {};

    result.processed = true;
    result.itemId = item.id;

    if (!item.importResult.imported)
    {
        item.state = AuthoringImportItemState::failed;
        result.state = "Authoring import failed";
        result.issues = item.importResult.issues;
    }
    else
    {
        for (const auto& warning : item.importResult.warnings)
        {
            addFinding(item.findings,
                       AuthoringImportFindingSeverity::warning,
                       "import.policy_warning",
                       "Source sample triggered a portability warning",
                       warning,
                       false);
        }

        const auto heuristics = parseSampleFilenameHeuristics(item.sourcePath, &item.importResult.sample.metadata);
        item.filenameTokens = heuristics.tokens;
        item.findings.insert(item.findings.end(), heuristics.findings.begin(), heuristics.findings.end());
        item.suggestedZone = heuristics.suggestedZone;
        item.suggestedZone.zone.sampleSourceId = slugify(fs::path(item.sourcePath).stem().generic_string());
        item.suggestedZone.sourceSampleId = item.suggestedZone.zone.sampleSourceId;
        item.state = resolvePostImportState(item);
        result.state = item.state == AuthoringImportItemState::warning
            ? "Authoring import completed with findings"
            : "Authoring import inferred draft zone";
    }

    result.durationMicros = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - startTime).count());
    result.queueDrained = std::none_of(queue.items.begin(),
                                       queue.items.end(),
                                       [](const AuthoringImportQueueItem& queueItem)
                                       {
                                           return queueItem.state == AuthoringImportItemState::pending;
                                       });
    queue.metrics.lastProcessDurationMicros = result.durationMicros;
    queue.metrics.maxProcessDurationMicros = std::max(queue.metrics.maxProcessDurationMicros, result.durationMicros);
    queue.metrics.lastProcessedItemId = result.itemId;
    const auto previousProcessedCount = queue.metrics.processedCount;
    const auto previousAverage = queue.metrics.averageProcessDurationMicros;
    const auto nextProcessedCount = previousProcessedCount + 1;
    queue.metrics.averageProcessDurationMicros = nextProcessedCount > 0
        ? static_cast<std::uint64_t>(((previousAverage * previousProcessedCount) + result.durationMicros) / nextProcessedCount)
        : result.durationMicros;
    refreshQueueMetrics(queue);
    return result;
}

bool cancelAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId)
{
    const auto iterator = std::find_if(queue.items.begin(),
                                       queue.items.end(),
                                       [&](const AuthoringImportQueueItem& item)
                                       {
                                           return item.id == itemId;
                                       });
    if (iterator == queue.items.end() || iterator->state != AuthoringImportItemState::pending)
        return false;

    iterator->state = AuthoringImportItemState::canceled;
    refreshQueueMetrics(queue);
    return true;
}

bool acceptAuthoringImportQueueItem(AuthoringImportQueue& queue, const std::string& itemId)
{
    const auto iterator = std::find_if(queue.items.begin(),
                                       queue.items.end(),
                                       [&](const AuthoringImportQueueItem& item)
                                       {
                                           return item.id == itemId;
                                       });
    if (iterator == queue.items.end())
        return false;

    if (iterator->state != AuthoringImportItemState::inferred
        && iterator->state != AuthoringImportItemState::warning)
    {
        return false;
    }

    iterator->state = AuthoringImportItemState::accepted;
    refreshQueueMetrics(queue);
    return true;
}
} // namespace drs::engine
