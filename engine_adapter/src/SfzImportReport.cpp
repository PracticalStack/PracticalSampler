#include "drs/engine/SfzImportReport.h"
#include "drs/engine/VelocityCrossfade.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <sstream>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

struct OpcodeClassification
{
    SfzImportSupportDisposition disposition = SfzImportSupportDisposition::converted;
    std::string nativeTarget;
    std::string rationale;
    std::string findingCode;
    std::string findingSummary;
    std::string findingDetail;
};

struct SupportKey
{
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string opcodeName;

    bool operator<(const SupportKey& other) const noexcept
    {
        if (scope != other.scope)
            return scope < other.scope;

        return opcodeName < other.opcodeName;
    }
};

struct CrossfadeOpcodeKey
{
    std::string sourcePath;
    std::size_t lineNumber = 0;
    std::size_t columnNumber = 0;
    SfzOpcodeScope scope = SfzOpcodeScope::unknown;
    std::string opcodeName;

    bool operator<(const CrossfadeOpcodeKey& other) const noexcept
    {
        if (sourcePath != other.sourcePath)
            return sourcePath < other.sourcePath;
        if (lineNumber != other.lineNumber)
            return lineNumber < other.lineNumber;
        if (columnNumber != other.columnNumber)
            return columnNumber < other.columnNumber;
        if (scope != other.scope)
            return scope < other.scope;
        return opcodeName < other.opcodeName;
    }
};

struct CrossfadeRegionTopology
{
    VelocityCrossfadeTopologyZoneDefinition topologyZone;
    std::vector<CrossfadeOpcodeKey> ownerOpcodes;
    VelocityCrossfadeZoneIssue zoneIssue = VelocityCrossfadeZoneIssue::none;
};

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

bool isCurveValueOpcode(const std::string& opcodeName)
{
    if (opcodeName.size() < 2 || opcodeName.front() != 'v')
        return false;

    return std::all_of(opcodeName.begin() + 1,
                       opcodeName.end(),
                       [](unsigned char character)
                       {
                           return std::isdigit(character) != 0;
                       });
}

bool isVelocityCrossfadeOpcode(const std::string& opcodeName)
{
    return opcodeName == "xfin_lovel" || opcodeName == "xfin_hivel"
        || opcodeName == "xfout_lovel" || opcodeName == "xfout_hivel";
}

bool isSequentialRoundRobinOpcode(const std::string& opcodeName)
{
    return opcodeName == "seq_length" || opcodeName == "seq_position";
}

std::optional<int> parseIntValue(const std::string& text)
{
    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<int> parseMidiNoteValue(const std::string& text)
{
    if (text.empty())
        return std::nullopt;

    const auto lowered = toLowerAscii(text);
    const bool numeric = std::all_of(lowered.begin(),
                                     lowered.end(),
                                     [](char character)
                                     {
                                         return std::isdigit(static_cast<unsigned char>(character)) != 0
                                             || character == '-';
                                     });
    if (numeric)
    {
        const auto value = parseIntValue(lowered);
        if (value.has_value() && *value >= 0 && *value <= 127)
            return value;
        return std::nullopt;
    }

    if (lowered.size() < 2 || lowered.size() > 4)
        return std::nullopt;

    int semitone = 0;
    switch (lowered[0])
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

    const auto octaveText = lowered.substr(octaveStart);
    if (octaveText.empty())
        return std::nullopt;

    const auto octave = parseIntValue(octaveText);
    if (!octave.has_value())
        return std::nullopt;

    const auto midiNote = ((*octave + 1) * 12) + semitone;
    if (midiNote >= 0 && midiNote <= 127)
        return midiNote;

    return std::nullopt;
}

std::uint64_t computeFnv1a64(const std::string& text) noexcept
{
    std::uint64_t hash = 14695981039346656037ull;
    for (const auto character : text)
    {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ull;
    }

    return hash;
}

std::uint64_t buildCrossfadePairingKey(const std::string& articulationId,
                                       int rootKey,
                                       int keyLow,
                                       int keyHigh,
                                       int roundRobinLength,
                                       int roundRobinPosition)
{
    (void) roundRobinLength;
    (void) roundRobinPosition;
    std::ostringstream stream;
    stream << articulationId
           << "|" << rootKey
           << "|" << keyLow
           << "|" << keyHigh;
    return computeFnv1a64(stream.str());
}

std::string buildArticulationId(const SfzNormalizedSection& section)
{
    if (const auto* trigger = findEffectiveOpcode(section, "trigger"))
    {
        const auto lowered = toLowerAscii(trigger->value);
        if (lowered == "release")
            return "release";
        if (lowered == "legato")
            return "legato";
    }

    return "sustain";
}

std::string buildRoundRobinPoolSignature(const SfzNormalizedSection& section,
                                         int rootKey,
                                         int keyLow,
                                         int keyHigh,
                                         int velocityLow,
                                         int velocityHigh)
{
    std::ostringstream stream;
    stream << buildArticulationId(section)
           << "|" << rootKey
           << "|" << keyLow
           << "|" << keyHigh
           << "|" << velocityLow
           << "|" << velocityHigh;
    return stream.str();
}

std::string buildCrossfadeRoundRobinPoolSignature(const SfzNormalizedSection& section,
                                                  int rootKey,
                                                  int keyLow,
                                                  int keyHigh)
{
    std::ostringstream stream;
    stream << buildArticulationId(section)
           << "|" << rootKey
           << "|" << keyLow
           << "|" << keyHigh;
    return stream.str();
}

std::size_t dispositionRank(const SfzImportSupportDisposition disposition) noexcept
{
    switch (disposition)
    {
        case SfzImportSupportDisposition::converted:
            return 0;
        case SfzImportSupportDisposition::approximated:
            return 1;
        case SfzImportSupportDisposition::reportedOnly:
            return 2;
        case SfzImportSupportDisposition::blocking:
            return 3;
    }

    return 0;
}

std::size_t countParsedOpcodes(const SfzParsedDocument& document) noexcept
{
    auto count = std::size_t { 0 };
    for (const auto& section : document.sections)
        count += section.opcodes.size();

    return count;
}

std::string findEffectiveSampleReference(const SfzNormalizedSection& section)
{
    const auto* sample = findEffectiveOpcode(section, "sample");
    if (sample == nullptr)
        return {};

    fs::path resolvedBase = fs::path(sample->location.sourcePath).parent_path();
    if (const auto* prefix = findEffectiveOpcode(section, "prefix_sfz_path");
        prefix != nullptr && !prefix->value.empty())
    {
        resolvedBase /= fs::path(prefix->value);
    }

    const fs::path samplePath(sample->value);
    return samplePath.is_absolute()
        ? samplePath.lexically_normal().generic_string()
        : (resolvedBase / samplePath).lexically_normal().generic_string();
}

std::string resolveSamplePathForSection(const SfzNormalizedSection& section,
                                        const SfzResolvedOpcode& opcode)
{
    fs::path resolvedBase = fs::path(opcode.location.sourcePath).parent_path();
    if (const auto* prefix = findEffectiveOpcode(section, "prefix_sfz_path");
        prefix != nullptr && !prefix->value.empty())
    {
        resolvedBase /= fs::path(prefix->value);
    }

    const fs::path samplePath(opcode.value);
    return samplePath.is_absolute()
        ? samplePath.lexically_normal().generic_string()
        : (resolvedBase / samplePath).lexically_normal().generic_string();
}

OpcodeClassification classifySampleOpcode(const SfzNormalizedSection& section,
                                          const SfzResolvedOpcode& opcode)
{
    const auto resolvedSamplePath = resolveSamplePathForSection(section, opcode);
    if (!fs::exists(resolvedSamplePath))
    {
        return { SfzImportSupportDisposition::blocking,
                 "zone.samplePath",
                 "Sample file resolution is required before any native zone can be created.",
                 "sfz.sample.missing",
                 "Referenced sample file is missing",
                 "The importer could not resolve sample '" + opcode.value + "' for this region." };
    }

    return { SfzImportSupportDisposition::converted,
             "zone.samplePath",
             "Relative sample references can map directly into native zone source paths." };
}

CrossfadeOpcodeKey makeCrossfadeOpcodeKey(const SfzResolvedOpcode& opcode)
{
    return {
        opcode.location.sourcePath,
        opcode.location.lineNumber,
        opcode.location.columnNumber,
        opcode.location.scope,
        opcode.name
    };
}

std::vector<CrossfadeOpcodeKey> collectCrossfadeOwnerKeys(const SfzNormalizedSection& section)
{
    std::set<CrossfadeOpcodeKey> uniqueKeys;
    for (const auto& opcode : section.effectiveOpcodes)
    {
        if (isVelocityCrossfadeOpcode(toLowerAscii(opcode.name)))
            uniqueKeys.insert(makeCrossfadeOpcodeKey(opcode));
    }

    return { uniqueKeys.begin(), uniqueKeys.end() };
}

std::string buildVelocityCrossfadeIssueDetail(VelocityCrossfadeZoneIssue issue)
{
    switch (issue)
    {
        case VelocityCrossfadeZoneIssue::none:
            return {};
        case VelocityCrossfadeZoneIssue::velocityRangeInvalid:
            return "The imported velocity bounds do not resolve to an ordered 1-127 range.";
        case VelocityCrossfadeZoneIssue::unsupportedCurve:
            return "Only the first-pass linear crossfade curve is currently supported.";
        case VelocityCrossfadeZoneIssue::fadeInPartial:
            return "Fade-in metadata must provide both xfin_lovel and xfin_hivel.";
        case VelocityCrossfadeZoneIssue::fadeInOutOfRange:
            return "Fade-in boundaries must begin at the owning layer's velocityLow and stay within range.";
        case VelocityCrossfadeZoneIssue::fadeInInverted:
            return "Fade-in boundaries must rise from low to high velocity.";
        case VelocityCrossfadeZoneIssue::fadeOutPartial:
            return "Fade-out metadata must provide both xfout_lovel and xfout_hivel.";
        case VelocityCrossfadeZoneIssue::fadeOutOutOfRange:
            return "Fade-out boundaries must end at the owning layer's velocityHigh and stay within range.";
        case VelocityCrossfadeZoneIssue::fadeOutInverted:
            return "Fade-out boundaries must rise from low to high velocity.";
        case VelocityCrossfadeZoneIssue::fadeWindowsOverlap:
            return "Fade-in and fade-out windows on the same layer must not touch or overlap.";
    }

    return "The imported velocity-crossfade shape is outside the supported first-pass contract.";
}

std::string buildVelocityCrossfadeTopologyDetail(VelocityCrossfadeTopologyIssue issue)
{
    switch (issue)
    {
        case VelocityCrossfadeTopologyIssue::none:
            return {};
        case VelocityCrossfadeTopologyIssue::fadeInMissingPartner:
            return "The imported fade-in layer did not resolve a single lower adjacent partner.";
        case VelocityCrossfadeTopologyIssue::fadeInAmbiguousPartner:
            return "The imported fade-in layer resolved multiple lower adjacent partners.";
        case VelocityCrossfadeTopologyIssue::fadeOutMissingPartner:
            return "The imported fade-out layer did not resolve a single upper adjacent partner.";
        case VelocityCrossfadeTopologyIssue::fadeOutAmbiguousPartner:
            return "The imported fade-out layer resolved multiple upper adjacent partners.";
    }

    return "The imported velocity-crossfade topology is outside the supported first-pass contract.";
}

OpcodeClassification makeSupportedVelocityCrossfadeClassification()
{
    return {
        SfzImportSupportDisposition::converted,
        "zone.velocityCrossfade",
        "Linear adjacent velocity-crossfade boundaries map into native metadata and now play back with deterministic gains."
    };
}

OpcodeClassification makeUnsupportedVelocityCrossfadeClassification(const std::string& detail)
{
    return {
        SfzImportSupportDisposition::approximated,
        "zone.velocityCrossfade",
        "Velocity-crossfade metadata can be preserved, but this shape falls outside the supported first-pass playback contract.",
        "sfz.velocity_crossfade.approximated",
        "Velocity crossfade requires review",
        detail.empty()
            ? "This SFZ uses velocity-crossfade boundaries that are not yet guaranteed to play back as authored."
            : "This SFZ uses velocity-crossfade boundaries that are not yet guaranteed to play back as authored. "
                + detail
    };
}

OpcodeClassification makeUnsupportedRoundRobinClassification(const std::string& findingCode,
                                                             const std::string& findingSummary,
                                                             const std::string& detail)
{
    return {
        SfzImportSupportDisposition::reportedOnly,
        "report.roundRobin",
        "Sequential round-robin metadata is recognized, but this pool needs creator review before it can be converted into a native Round Robin object.",
        findingCode,
        findingSummary,
        detail
    };
}

std::vector<CrossfadeOpcodeKey> collectSequentialRoundRobinOwnerKeys(const SfzNormalizedSection& section)
{
    std::set<CrossfadeOpcodeKey> uniqueKeys;
    for (const auto& opcode : section.effectiveOpcodes)
    {
        if (isSequentialRoundRobinOpcode(toLowerAscii(opcode.name)))
            uniqueKeys.insert(makeCrossfadeOpcodeKey(opcode));
    }

    return { uniqueKeys.begin(), uniqueKeys.end() };
}

void assignRoundRobinClassification(std::map<CrossfadeOpcodeKey, OpcodeClassification>& classifications,
                                    const std::vector<CrossfadeOpcodeKey>& ownerKeys,
                                    const OpcodeClassification& classification)
{
    for (const auto& ownerKey : ownerKeys)
    {
        auto existing = classifications.find(ownerKey);
        if (existing == classifications.end()
            || dispositionRank(classification.disposition) > dispositionRank(existing->second.disposition))
        {
            classifications[ownerKey] = classification;
        }
    }
}

std::map<CrossfadeOpcodeKey, OpcodeClassification> buildSequentialRoundRobinOpcodeClassifications(
    const SfzNormalizedDocument& document)
{
    struct SequentialRoundRobinRegion
    {
        std::string poolSignature;
        int roundRobinLength = 0;
        int roundRobinPosition = 0;
        std::vector<CrossfadeOpcodeKey> ownerKeys;
    };

    std::vector<SequentialRoundRobinRegion> regions;
    std::map<CrossfadeOpcodeKey, OpcodeClassification> classifications;

    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;

        auto ownerKeys = collectSequentialRoundRobinOwnerKeys(section);
        if (ownerKeys.empty())
            continue;

        const auto rootKey = parseMidiNoteValue(findEffectiveOpcode(section, "pitch_keycenter") != nullptr
                                                    ? findEffectiveOpcode(section, "pitch_keycenter")->value
                                                    : "60")
                                 .value_or(60);
        const auto keyLow = parseMidiNoteValue(findEffectiveOpcode(section, "lokey") != nullptr
                                                   ? findEffectiveOpcode(section, "lokey")->value
                                                   : std::to_string(rootKey))
                                .value_or(rootKey);
        const auto keyHigh = parseMidiNoteValue(findEffectiveOpcode(section, "hikey") != nullptr
                                                    ? findEffectiveOpcode(section, "hikey")->value
                                                    : std::to_string(rootKey))
                                 .value_or(rootKey);
        auto velocityLow = parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                                             ? findEffectiveOpcode(section, "lovel")->value
                                             : "1")
                               .value_or(1);
        auto velocityHigh = parseIntValue(findEffectiveOpcode(section, "hivel") != nullptr
                                              ? findEffectiveOpcode(section, "hivel")->value
                                              : "127")
                                .value_or(127);
        if (const auto fadeInLowVelocity = parseIntValue(findEffectiveOpcode(section, "xfin_lovel") != nullptr
                                                             ? findEffectiveOpcode(section, "xfin_lovel")->value
                                                             : "0");
            fadeInLowVelocity.value_or(0) > 0)
        {
            velocityLow = *fadeInLowVelocity;
        }
        if (const auto fadeOutHighVelocity = parseIntValue(findEffectiveOpcode(section, "xfout_hivel") != nullptr
                                                               ? findEffectiveOpcode(section, "xfout_hivel")->value
                                                               : "0");
            fadeOutHighVelocity.value_or(0) > 0)
        {
            velocityHigh = *fadeOutHighVelocity;
        }

        SequentialRoundRobinRegion region;
        region.roundRobinLength = parseIntValue(findEffectiveOpcode(section, "seq_length") != nullptr
                                                    ? findEffectiveOpcode(section, "seq_length")->value
                                                    : "0")
                                      .value_or(0);
        region.roundRobinPosition = parseIntValue(findEffectiveOpcode(section, "seq_position") != nullptr
                                                      ? findEffectiveOpcode(section, "seq_position")->value
                                                      : "0")
                                        .value_or(0);
        region.poolSignature = buildRoundRobinPoolSignature(section,
                                                            rootKey,
                                                            keyLow,
                                                            keyHigh,
                                                            velocityLow,
                                                            velocityHigh);
        region.ownerKeys = std::move(ownerKeys);

        if (region.roundRobinLength <= 0
            || region.roundRobinPosition <= 0
            || region.roundRobinPosition > region.roundRobinLength)
        {
            assignRoundRobinClassification(
                classifications,
                region.ownerKeys,
                makeUnsupportedRoundRobinClassification(
                    "sfz.round_robin.slot_contract.reported",
                    "Sequential round-robin slot metadata requires review",
                    "The importer only converts sequential round-robin regions when seq_length and seq_position form a positive in-range slot contract."));
        }

        regions.push_back(std::move(region));
    }

    std::map<std::string, std::vector<std::size_t>> regionsByPoolSignature;
    for (std::size_t index = 0; index < regions.size(); ++index)
    {
        if (regions[index].roundRobinLength > 0
            && regions[index].roundRobinPosition > 0
            && regions[index].roundRobinPosition <= regions[index].roundRobinLength)
        {
            regionsByPoolSignature[regions[index].poolSignature].push_back(index);
        }
    }

    for (const auto& [_, regionIndices] : regionsByPoolSignature)
    {
        std::set<int> uniqueLengths;
        std::set<int> uniquePositions;
        std::map<int, int> positionCounts;
        std::vector<CrossfadeOpcodeKey> ownerKeys;

        for (const auto regionIndex : regionIndices)
        {
            const auto& region = regions[regionIndex];
            uniqueLengths.insert(region.roundRobinLength);
            uniquePositions.insert(region.roundRobinPosition);
            ++positionCounts[region.roundRobinPosition];
            ownerKeys.insert(ownerKeys.end(), region.ownerKeys.begin(), region.ownerKeys.end());
        }

        if (uniqueLengths.size() > 1)
        {
            assignRoundRobinClassification(
                classifications,
                ownerKeys,
                makeUnsupportedRoundRobinClassification(
                    "sfz.round_robin.mixed_lengths.reported",
                    "Sequential round-robin pool declares mixed lengths",
                    "Regions that share the same articulation, key, and velocity window must agree on one seq_length value before the importer can build a native Round Robin pool."));
            continue;
        }

        const auto duplicateSlot = std::find_if(positionCounts.begin(),
                                                positionCounts.end(),
                                                [](const auto& entry)
                                                {
                                                    return entry.second > 1;
                                                });
        if (duplicateSlot != positionCounts.end())
        {
            assignRoundRobinClassification(
                classifications,
                ownerKeys,
                makeUnsupportedRoundRobinClassification(
                    "sfz.round_robin.conflicting_group.reported",
                    "Sequential round-robin pool maps multiple regions to one slot",
                    "Regions that share the same articulation, key, and velocity window must not reuse the same seq_position when building a native Round Robin pool."));
            continue;
        }

        const auto expectedLength = *uniqueLengths.begin();
        const auto contiguous = !uniquePositions.empty()
            && *uniquePositions.begin() == 1
            && static_cast<int>(uniquePositions.size()) == expectedLength;
        if (!contiguous)
        {
            assignRoundRobinClassification(
                classifications,
                ownerKeys,
                makeUnsupportedRoundRobinClassification(
                    "sfz.round_robin.sparse_slots.reported",
                    "Sequential round-robin pool has sparse slot coverage",
                    "The importer only converts sequential round-robin pools when every slot from 1 through seq_length is present exactly once within the pool."));
        }
    }

    return classifications;
}

std::map<CrossfadeOpcodeKey, OpcodeClassification> buildVelocityCrossfadeOpcodeClassifications(
    const SfzNormalizedDocument& document)
{
    std::vector<CrossfadeRegionTopology> regions;
    regions.reserve(document.sections.size());

    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;

        auto ownerOpcodes = collectCrossfadeOwnerKeys(section);
        if (ownerOpcodes.empty())
            continue;

        CrossfadeRegionTopology region;
        region.ownerOpcodes = std::move(ownerOpcodes);
        region.topologyZone.crossfade = {};
        region.topologyZone.crossfade.fadeInLowVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfin_lovel") != nullptr
                              ? findEffectiveOpcode(section, "xfin_lovel")->value
                              : "0")
                .value_or(0);
        region.topologyZone.crossfade.fadeInHighVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfin_hivel") != nullptr
                              ? findEffectiveOpcode(section, "xfin_hivel")->value
                              : "0")
                .value_or(0);
        region.topologyZone.crossfade.fadeOutLowVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfout_lovel") != nullptr
                              ? findEffectiveOpcode(section, "xfout_lovel")->value
                              : "0")
                .value_or(0);
        region.topologyZone.crossfade.fadeOutHighVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfout_hivel") != nullptr
                              ? findEffectiveOpcode(section, "xfout_hivel")->value
                              : "0")
                .value_or(0);

        const auto rootKey = parseMidiNoteValue(findEffectiveOpcode(section, "pitch_keycenter") != nullptr
                                                    ? findEffectiveOpcode(section, "pitch_keycenter")->value
                                                    : "60")
                                 .value_or(60);
        const auto keyLow = parseMidiNoteValue(findEffectiveOpcode(section, "lokey") != nullptr
                                                   ? findEffectiveOpcode(section, "lokey")->value
                                                   : std::to_string(rootKey))
                                .value_or(rootKey);
        const auto keyHigh = parseMidiNoteValue(findEffectiveOpcode(section, "hikey") != nullptr
                                                    ? findEffectiveOpcode(section, "hikey")->value
                                                    : std::to_string(rootKey))
                                 .value_or(rootKey);
        auto velocityLow = parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                                             ? findEffectiveOpcode(section, "lovel")->value
                                             : "1")
                               .value_or(1);
        auto velocityHigh = parseIntValue(findEffectiveOpcode(section, "hivel") != nullptr
                                              ? findEffectiveOpcode(section, "hivel")->value
                                              : "127")
                                .value_or(127);
        if (region.topologyZone.crossfade.fadeInLowVelocity > 0)
            velocityLow = region.topologyZone.crossfade.fadeInLowVelocity;
        if (region.topologyZone.crossfade.fadeOutHighVelocity > 0)
            velocityHigh = region.topologyZone.crossfade.fadeOutHighVelocity;

        region.topologyZone.velocityLow = velocityLow;
        region.topologyZone.velocityHigh = velocityHigh;
        region.topologyZone.roundRobinLength = parseIntValue(findEffectiveOpcode(section, "seq_length") != nullptr
                                                                 ? findEffectiveOpcode(section, "seq_length")->value
                                                                 : "0")
                                                   .value_or(0);
        region.topologyZone.roundRobinPosition = parseIntValue(findEffectiveOpcode(section, "seq_position") != nullptr
                                                                   ? findEffectiveOpcode(section, "seq_position")->value
                                                                   : "0")
                                                     .value_or(0);
        if (region.topologyZone.roundRobinLength > 0 && region.topologyZone.roundRobinPosition > 0)
        {
            region.topologyZone.roundRobinPoolId = buildCrossfadeRoundRobinPoolSignature(section,
                                                                                         rootKey,
                                                                                         keyLow,
                                                                                         keyHigh);
        }
        region.topologyZone.pairingKey = buildCrossfadePairingKey(buildArticulationId(section),
                                                                  rootKey,
                                                                  keyLow,
                                                                  keyHigh,
                                                                  region.topologyZone.roundRobinLength,
                                                                  region.topologyZone.roundRobinPosition);

        const VelocityCrossfadeZoneDefinition validationZone {
            region.topologyZone.velocityLow,
            region.topologyZone.velocityHigh,
            region.topologyZone.crossfade
        };
        region.zoneIssue = validateFirstPassVelocityCrossfadeZone(validationZone);
        regions.push_back(std::move(region));
    }

    std::vector<VelocityCrossfadeTopologyZoneDefinition> topologyZones;
    topologyZones.reserve(regions.size());
    for (const auto& region : regions)
        topologyZones.push_back(region.topologyZone);

    std::vector<VelocityCrossfadeTopologyFinding> topologyFindings;
    buildFirstPassVelocityCrossfadeRuntimeTopology(topologyZones, &topologyFindings);

    std::map<std::size_t, VelocityCrossfadeTopologyIssue> topologyIssuesByRegion;
    for (const auto& finding : topologyFindings)
    {
        if (finding.issue == VelocityCrossfadeTopologyIssue::none)
            continue;

        const auto existing = topologyIssuesByRegion.find(finding.zoneIndex);
        if (existing == topologyIssuesByRegion.end()
            || static_cast<int>(finding.issue) < static_cast<int>(existing->second))
        {
            topologyIssuesByRegion[finding.zoneIndex] = finding.issue;
        }
    }

    std::map<CrossfadeOpcodeKey, OpcodeClassification> classifications;
    for (std::size_t regionIndex = 0; regionIndex < regions.size(); ++regionIndex)
    {
        OpcodeClassification classification;
        if (regions[regionIndex].zoneIssue != VelocityCrossfadeZoneIssue::none)
        {
            classification = makeUnsupportedVelocityCrossfadeClassification(
                buildVelocityCrossfadeIssueDetail(regions[regionIndex].zoneIssue));
        }
        else if (const auto topologyIssue = topologyIssuesByRegion.find(regionIndex);
                 topologyIssue != topologyIssuesByRegion.end())
        {
            classification = makeUnsupportedVelocityCrossfadeClassification(
                buildVelocityCrossfadeTopologyDetail(topologyIssue->second));
        }
        else
        {
            classification = makeSupportedVelocityCrossfadeClassification();
        }

        for (const auto& ownerKey : regions[regionIndex].ownerOpcodes)
        {
            auto existing = classifications.find(ownerKey);
            if (existing == classifications.end()
                || dispositionRank(classification.disposition) > dispositionRank(existing->second.disposition))
            {
                classifications[ownerKey] = classification;
            }
        }
    }

    return classifications;
}

void incrementDispositionCount(SfzImportReportSummary& summary,
                               const SfzImportSupportDisposition disposition) noexcept
{
    switch (disposition)
    {
        case SfzImportSupportDisposition::converted:
            ++summary.convertedOpcodeCount;
            break;
        case SfzImportSupportDisposition::approximated:
            ++summary.approximatedOpcodeCount;
            break;
        case SfzImportSupportDisposition::reportedOnly:
            ++summary.reportedOnlyOpcodeCount;
            break;
        case SfzImportSupportDisposition::blocking:
            ++summary.blockingOpcodeCount;
            break;
    }
}

void accumulateFindingSeverities(SfzImportReportSummary& summary,
                                 const std::vector<SfzImportFinding>& findings) noexcept
{
    for (const auto& finding : findings)
    {
        switch (finding.severity)
        {
            case SfzImportFindingSeverity::information:
                ++summary.informationFindingCount;
                break;
            case SfzImportFindingSeverity::warning:
                ++summary.warningFindingCount;
                break;
            case SfzImportFindingSeverity::error:
                ++summary.errorFindingCount;
                break;
        }
    }
}

void addClassificationFinding(std::vector<SfzImportFinding>& findings,
                              const OpcodeClassification& classification,
                              const SfzResolvedOpcode& opcode,
                              const std::string& sampleReference)
{
    if (classification.disposition == SfzImportSupportDisposition::converted)
        return;

    SfzImportFinding finding;
    finding.severity = classification.disposition == SfzImportSupportDisposition::blocking
        ? SfzImportFindingSeverity::error
        : SfzImportFindingSeverity::warning;
    finding.disposition = classification.disposition;
    finding.code = classification.findingCode;
    finding.summary = classification.findingSummary;
    finding.detail = classification.findingDetail;
    if (!sampleReference.empty())
        finding.detail += " Context sample: '" + sampleReference + "'.";
    finding.location = opcode.location;
    findings.push_back(std::move(finding));
}

void updateSupportSummary(std::map<SupportKey, SfzImportOpcodeSupportSummary>& summaries,
                          const SfzResolvedOpcode& opcode,
                          const OpcodeClassification& classification)
{
    const SupportKey key { opcode.location.scope, opcode.name };
    auto& summary = summaries[key];
    if (summary.occurrenceCount == 0)
    {
        summary.scope = opcode.location.scope;
        summary.opcodeName = opcode.name;
        summary.disposition = classification.disposition;
        summary.nativeTarget = classification.nativeTarget;
        summary.rationale = classification.rationale;
    }
    else if (dispositionRank(classification.disposition) > dispositionRank(summary.disposition))
    {
        summary.disposition = classification.disposition;
        summary.nativeTarget = classification.nativeTarget;
        summary.rationale = classification.rationale;
    }

    ++summary.occurrenceCount;
}

OpcodeClassification classifyOpcode(const SfzResolvedOpcode& opcode)
{
    const auto opcodeName = toLowerAscii(opcode.name);

    if (opcodeName == "lokey")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.keyRange.lowNote",
                 "Lower key bounds map directly into native zone key ranges." };
    }

    if (opcodeName == "hikey")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.keyRange.highNote",
                 "Upper key bounds map directly into native zone key ranges." };
    }

    if (opcodeName == "pitch_keycenter")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.rootKey",
                 "Pitch centers map directly into native root-key metadata." };
    }

    if (opcodeName == "lovel")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.velocityRange.lowVelocity",
                 "Lower velocity bounds map directly into native velocity ranges." };
    }

    if (opcodeName == "hivel")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.velocityRange.highVelocity",
                 "Upper velocity bounds map directly into native velocity ranges." };
    }

    if (opcodeName == "prefix_sfz_path")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.samplePath",
                 "Sample-path prefixes are consumed during import so each referenced sample resolves into a native source path." };
    }

    if (opcodeName == "seq_length")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.roundRobin.slotCount",
                 "Sequential round-robin sequence length converts into native Round Robin pool metadata and deterministic slot advancement." };
    }

    if (opcodeName == "seq_position")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.roundRobin.slotIndex",
                 "Sequential round-robin sequence position converts into native Round Robin pool metadata and deterministic slot advancement." };
    }

    if (opcodeName == "lorand" || opcodeName == "hirand")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.roundRobin.randomPolicy",
                 "Randomized region-selection policies remain review-only because the native Round Robin contract is currently deterministic and sequential.",
                 "sfz.round_robin.random_policy.reported",
                 "Random round-robin policy will be reported",
                 "The importer recognizes SFZ random round-robin policy opcodes, but does not convert them into the native sequential Round Robin behavior." };
    }

    if (opcodeName == "sw_last"
        || opcodeName == "sw_lokey"
        || opcodeName == "sw_hikey"
        || opcodeName == "sw_default"
        || opcodeName == "sw_label")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.roundRobin.switchPolicy",
                 "Switch-driven region-selection policies remain review-only because they do not map directly onto the native sequential Round Robin contract.",
                 "sfz.round_robin.switch_policy.reported",
                 "Switch-driven round-robin policy will be reported",
                 "The importer recognizes SFZ switch-driven region-selection policy opcodes, but does not convert them into the native sequential Round Robin behavior." };
    }

    if (opcodeName == "volume")
    {
        return { SfzImportSupportDisposition::converted,
                 "instrument.gainDb",
                 "Document-level gain can map into native authored gain metadata." };
    }

    if (opcodeName == "ampeg_release")
    {
        return { SfzImportSupportDisposition::converted,
                 "ampEnvelope.releaseSeconds",
                 "Per-zone and inherited release times can map into native envelope release controls." };
    }

    if (opcodeName == "label_cc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.controls.cc1Label",
                 "CC labels are preserved for creator review before any final import.",
                 "sfz.cc.label.reported",
                 "CC label will be reported",
                 "The importer preserves the CC1 label for review, but does not yet apply it to a native modulation surface." };
    }

    if (opcodeName == "set_hdcc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.controls.cc1Default",
                 "Controller defaults are surfaced in the review report even when the native control path is not ready.",
                 "sfz.cc.default.reported",
                 "Controller default will be reported",
                 "The importer preserves the CC1 default value for review, but does not yet apply it to a native modulation surface." };
    }

    if (opcodeName == "width_oncc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.modulation.widthOnCc1",
                 "CC-driven width modulation is important to disclose before project mutation.",
                 "sfz.cc.width.reported",
                 "CC-driven width modulation will be reported",
                 "The importer preserves width-on-CC1 modulation for creator review, but does not yet recreate the stereo-width behavior." };
    }

    if (opcodeName == "width_curvecc1")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.modulation.widthCurveCc1",
                 "Curve-linked width modulation is surfaced for transparency until the native behavior lands.",
                 "sfz.cc.width_curve.reported",
                 "CC width curve will be reported",
                 "The importer preserves the width-control curve binding for review, but does not yet recreate the stereo-width behavior." };
    }

    if (opcodeName == "curve_index" || isCurveValueOpcode(opcodeName))
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.curves",
                 "Curve definitions are preserved for creator review until a native curve path exists.",
                 "sfz.curve.reported",
                 "Curve definition will be reported",
                 "The importer preserves SFZ curve definitions for transparency, but does not yet convert them into native modulation curves." };
    }

    return { SfzImportSupportDisposition::reportedOnly,
             {},
             "Recognized SFZ opcodes remain visible in the report even when native conversion is not implemented yet.",
             "sfz.opcode.unmapped",
             "Opcode will be reported instead of converted",
             "The importer recognizes opcode '" + opcode.name + "' but does not yet have a native conversion target for it." };
}
} // namespace

SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath)
{
    SfzImportAnalysisResult result;
    result.analyzed = true;
    result.parseResult = parseSfzDocument(sfzPath);

    result.report.available = true;
    result.report.rootDocumentPath = result.parseResult.document.rootDocumentPath;
    result.report.sourceFiles = result.parseResult.document.sourceFiles;
    result.report.summary.sourceFileCount = result.report.sourceFiles.size();
    result.report.summary.sectionCount = result.parseResult.document.sections.size();
    result.report.summary.opcodeCount = countParsedOpcodes(result.parseResult.document);
    result.report.findings = result.parseResult.findings;

    if (result.parseResult.parsed)
    {
        result.normalizeResult = normalizeSfzDocument(result.parseResult.document);
        result.report.findings.insert(result.report.findings.end(),
                                      result.normalizeResult.findings.begin(),
                                      result.normalizeResult.findings.end());

        if (result.normalizeResult.normalized)
        {
            result.report.rootDocumentPath = result.normalizeResult.document.rootDocumentPath;
            result.report.sourceFiles = result.normalizeResult.document.sourceFiles;
            result.report.summary.sourceFileCount = result.report.sourceFiles.size();
            result.report.summary.sectionCount = result.normalizeResult.document.sections.size();
            result.report.summary.opcodeCount = 0;
            const auto crossfadeClassifications =
                buildVelocityCrossfadeOpcodeClassifications(result.normalizeResult.document);
            const auto roundRobinClassifications =
                buildSequentialRoundRobinOpcodeClassifications(result.normalizeResult.document);

            std::map<SupportKey, SfzImportOpcodeSupportSummary> supportSummaries;

            for (const auto& section : result.normalizeResult.document.sections)
            {
                const auto sampleReference = findEffectiveSampleReference(section);
                result.report.summary.opcodeCount += section.localOpcodes.size();

                for (const auto& opcode : section.localOpcodes)
                {
                    auto classification = toLowerAscii(opcode.name) == "sample"
                        ? classifySampleOpcode(section, opcode)
                        : classifyOpcode(opcode);
                    if (isVelocityCrossfadeOpcode(toLowerAscii(opcode.name)))
                    {
                        const auto classificationIterator =
                            crossfadeClassifications.find(makeCrossfadeOpcodeKey(opcode));
                        classification = classificationIterator != crossfadeClassifications.end()
                            ? classificationIterator->second
                            : makeUnsupportedVelocityCrossfadeClassification(
                                "The importer could not confirm a supported adjacent-layer pairing for this opcode.");
                    }
                    else if (isSequentialRoundRobinOpcode(toLowerAscii(opcode.name)))
                    {
                        if (const auto classificationIterator =
                                roundRobinClassifications.find(makeCrossfadeOpcodeKey(opcode));
                            classificationIterator != roundRobinClassifications.end())
                        {
                            classification = classificationIterator->second;
                        }
                    }
                    incrementDispositionCount(result.report.summary, classification.disposition);
                    addClassificationFinding(result.report.findings,
                                             classification,
                                             opcode,
                                             sampleReference);
                    updateSupportSummary(supportSummaries, opcode, classification);

                    result.report.traceEntries.push_back(
                        { section.documentOrder,
                          section.scope,
                          section.headerName,
                          opcode.name,
                          opcode.value,
                          classification.nativeTarget,
                          sampleReference,
                          classification.disposition,
                          classification.findingCode,
                          opcode.location });
                }
            }

            result.report.opcodeSupport.reserve(supportSummaries.size());
            for (const auto& [_, summary] : supportSummaries)
                result.report.opcodeSupport.push_back(summary);
        }
    }

    result.report.reviewDisposition = sfzImportReviewDispositionFor(result.report.findings);
    result.report.blocking = result.report.reviewDisposition == SfzImportReviewDisposition::blocked;
    result.report.stage = result.report.blocking ? SfzImportStage::blocked
                                                 : SfzImportStage::reviewReady;
    result.report.state = result.report.blocking ? "Blocked" : "Review Ready";

    accumulateFindingSeverities(result.report.summary, result.report.findings);
    return result;
}
} // namespace drs::engine
