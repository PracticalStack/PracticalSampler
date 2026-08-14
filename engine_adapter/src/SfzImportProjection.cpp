#include "drs/engine/SfzImportProjection.h"

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <tuple>

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

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

    return slug.empty() ? "sfz" : slug;
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
        try
        {
            const auto value = std::stoi(lowered);
            if (value >= 0 && value <= 127)
                return value;
        }
        catch (...)
        {
        }

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

    try
    {
        const auto octave = std::stoi(octaveText);
        const auto midiNote = ((octave + 1) * 12) + semitone;
        if (midiNote >= 0 && midiNote <= 127)
            return midiNote;
    }
    catch (...)
    {
    }

    return std::nullopt;
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

int normalizePlayableVelocityLowerBound(const int velocity) noexcept
{
    // MIDI note-on velocity zero is a note-off encoding. Some SFZ libraries nevertheless use
    // lovel=0 (and zero-valued crossfade lower bounds) to mean the bottom playable layer.
    return velocity == 0 ? 1 : velocity;
}

struct SequentialRoundRobinSlot
{
    int length = 0;
    int position = 0;
};

SequentialRoundRobinSlot parseSequentialRoundRobinSlot(const SfzNormalizedSection& section)
{
    const auto* lengthOpcode = findEffectiveOpcode(section, "seq_length");
    const auto* positionOpcode = findEffectiveOpcode(section, "seq_position");
    if (lengthOpcode == nullptr && positionOpcode == nullptr)
        return {};

    // SFZ v1 defines both sequence opcodes with a default value of 1. In
    // particular, libraries commonly specify seq_length on the first group
    // and omit seq_position to represent slot 1.
    return {
        parseIntValue(lengthOpcode != nullptr ? lengthOpcode->value : "1").value_or(0),
        parseIntValue(positionOpcode != nullptr ? positionOpcode->value : "1").value_or(0)
    };
}

int parseEffectiveKeyValue(const SfzNormalizedSection& section,
                           const char* specializedOpcode,
                           int fallback)
{
    const auto* opcode = findEffectiveOpcode(section, specializedOpcode);
    const auto* keyOpcode = findEffectiveOpcode(section, "key");
    const auto scopeRank = [](const SfzOpcodeScope scope)
    {
        switch (scope)
        {
            case SfzOpcodeScope::region: return 5;
            case SfzOpcodeScope::group: return 4;
            case SfzOpcodeScope::master: return 3;
            case SfzOpcodeScope::global: return 2;
            case SfzOpcodeScope::control: return 1;
            default: return 0;
        }
    };

    if (keyOpcode != nullptr
        && (opcode == nullptr
            || scopeRank(keyOpcode->location.scope) > scopeRank(opcode->location.scope)))
    {
        return parseMidiNoteValue(keyOpcode->value).value_or(fallback);
    }
    if (opcode != nullptr)
        return parseMidiNoteValue(opcode->value).value_or(fallback);
    return fallback;
}

std::optional<int> parseControllerOpcodeNumber(const std::string& opcodeName,
                                               const std::string& prefix)
{
    if (opcodeName.rfind(prefix, 0) != 0 || opcodeName.size() == prefix.size())
        return std::nullopt;
    const auto suffix = opcodeName.substr(prefix.size());
    if (!std::all_of(suffix.begin(), suffix.end(), [](const unsigned char character)
                     { return std::isdigit(character) != 0; }))
        return std::nullopt;
    const auto number = parseIntValue(suffix);
    if (!number.has_value() || *number < 0 || *number > 127)
        return std::nullopt;
    return number;
}

std::vector<RuntimeControllerCondition> buildControllerConditions(
    const SfzNormalizedSection& section,
    bool& hasControllerTrigger,
    int& triggerControllerNumber)
{
    struct Range { int low = 0; int high = 127; bool present = false; bool trigger = false; };
    std::map<int, Range> ranges;
    for (const auto& opcode : section.effectiveOpcodes)
    {
        const auto apply = [&](const std::string& prefix, const bool isLow, const bool trigger)
        {
            const auto controller = parseControllerOpcodeNumber(opcode.name, prefix);
            if (!controller.has_value()) return false;
            auto& range = ranges[*controller];
            range.present = true;
            range.trigger = range.trigger || trigger;
            const auto value = std::clamp(parseIntValue(opcode.value).value_or(isLow ? 0 : 127), 0, 127);
            if (isLow) range.low = value; else range.high = value;
            return true;
        };
        if (apply("on_locc", true, true) || apply("on_hicc", false, true)
            || apply("locc", true, false) || apply("hicc", false, false))
            continue;
    }

    std::vector<RuntimeControllerCondition> conditions;
    conditions.reserve(ranges.size());
    for (const auto& [controller, range] : ranges)
    {
        if (!range.present) continue;
        conditions.push_back({ controller, range.low, range.high });
        if (range.trigger && !hasControllerTrigger)
        {
            hasControllerTrigger = true;
            triggerControllerNumber = controller;
        }
    }
    return conditions;
}

std::optional<std::uint64_t> parseFrameValue(const std::string& text)
{
    try
    {
        const auto value = std::stoll(text);
        if (value >= 0)
            return static_cast<std::uint64_t>(value);
    }
    catch (...)
    {
    }

    return std::nullopt;
}

std::optional<double> parseDoubleValue(const std::string& text)
{
    try
    {
        return std::stod(text);
    }
    catch (...)
    {
        return std::nullopt;
    }
}

const SfzResolvedOpcode* findLocalOpcode(const SfzNormalizedSection& section,
                                         const std::string& opcodeName) noexcept
{
    const auto lowered = toLowerAscii(opcodeName);
    const auto iterator = std::find_if(section.localOpcodes.begin(),
                                       section.localOpcodes.end(),
                                       [&](const SfzResolvedOpcode& opcode)
                                       {
                                           return opcode.name == lowered;
                                       });
    return iterator == section.localOpcodes.end() ? nullptr : &(*iterator);
}

struct ScopedGainState
{
    double masterGainDb = 0.0;
    double groupGainDb = 0.0;
    bool hasMasterGain = false;
    bool hasGroupGain = false;
};

struct ScopedGainContribution
{
    double masterGainDb = 0.0;
    double groupGainDb = 0.0;
    double regionGainDb = 0.0;
    bool hasMasterGain = false;
    bool hasGroupGain = false;
    bool hasRegionGain = false;
};

ScopedGainContribution buildScopedGainContribution(const SfzNormalizedSection& section,
                                                   const ScopedGainState& state)
{
    ScopedGainContribution contribution;
    contribution.masterGainDb = state.masterGainDb;
    contribution.groupGainDb = state.groupGainDb;
    contribution.hasMasterGain = state.hasMasterGain;
    contribution.hasGroupGain = state.hasGroupGain;
    if (const auto* volumeOpcode = findLocalOpcode(section, "volume"))
    {
        contribution.regionGainDb = parseDoubleValue(volumeOpcode->value).value_or(0.0);
        contribution.hasRegionGain = true;
    }
    return contribution;
}

std::string makeUniqueId(std::set<std::string>& usedIds, const std::string& base)
{
    auto candidate = base.empty() ? std::string("sfz") : base;
    if (usedIds.insert(candidate).second)
        return candidate;

    for (std::size_t suffix = 2; suffix < 1000000; ++suffix)
    {
        const auto suffixed = candidate + "-" + std::to_string(suffix);
        if (usedIds.insert(suffixed).second)
            return suffixed;
    }

    return candidate;
}

std::string sampleStem(const fs::path& samplePath)
{
    return samplePath.stem().generic_string();
}

std::string buildDisplayName(const fs::path& samplePath)
{
    auto stem = sampleStem(samplePath);
    std::replace(stem.begin(), stem.end(), '_', ' ');
    std::replace(stem.begin(), stem.end(), '-', ' ');
    return stem.empty() ? samplePath.filename().generic_string() : stem;
}

std::string buildArticulationDisplayName(const std::string& articulationId)
{
    auto displayName = articulationId;
    if (displayName.empty())
        return "Articulation";

    std::replace(displayName.begin(), displayName.end(), '-', ' ');
    std::replace(displayName.begin(), displayName.end(), '_', ' ');
    displayName.front() = static_cast<char>(
        std::toupper(static_cast<unsigned char>(displayName.front())));
    return displayName;
}

std::string buildArticulationId(const SfzNormalizedSection& section)
{
    static_cast<void>(section);
    // A single SFZ program is one selectable articulation. Trigger kinds and controller
    // conditions are route eligibility, not articulation-selection dimensions.
    return "sustain";
}

std::string buildGroupId(const SfzNormalizedSection& section,
                         const std::string& articulationId,
                         int keyLow,
                         int keyHigh)
{
    std::ostringstream stream;
    stream << "sfz-" << slugify(articulationId)
           << "-k" << keyLow << "-" << keyHigh;

    if (const auto* roundRobinLength = findEffectiveOpcode(section, "seq_length"))
        stream << "-rr" << roundRobinLength->value;

    return slugify(stream.str());
}

struct ProjectedGroupState
{
    std::size_t groupIndex = 0;
    double sharedGainDb = 0.0;
    bool bakedIntoZones = false;
    std::vector<std::size_t> zoneIndices;
};

std::string buildRoundRobinPoolSignature(const RuntimeProjectZoneDefinition& zone)
{
    std::ostringstream stream;
    stream << zone.articulationId
           << "|" << zone.rootKey
           << "|" << zone.keyLow
           << "|" << zone.keyHigh;
    return slugify(stream.str());
}

std::optional<RoundRobinDescriptor> buildSequentialRoundRobinDescriptor(
    const RuntimeProjectZoneDefinition& zone,
    std::set<std::string>& usedPoolIds,
    std::map<std::string, std::string>& poolIdsBySignature)
{
    if (zone.roundRobinLength <= 0
        || zone.roundRobinPosition <= 0
        || zone.roundRobinPosition > zone.roundRobinLength)
    {
        return std::nullopt;
    }

    const auto signature = buildRoundRobinPoolSignature(zone);
    auto existingPool = poolIdsBySignature.find(signature);
    if (existingPool == poolIdsBySignature.end())
    {
        const auto poolId = makeUniqueId(usedPoolIds, "sfz-rr-" + signature);
        existingPool = poolIdsBySignature.emplace(signature, poolId).first;
    }

    return RoundRobinDescriptor {
        existingPool->second,
        zone.roundRobinLength,
        zone.roundRobinPosition,
        RoundRobinMode::sequential
    };
}

bool shouldEnableLoop(const SfzNormalizedSection& section)
{
    const auto* loopStart = findEffectiveOpcode(section, "loop_start");
    const auto* loopEnd = findEffectiveOpcode(section, "loop_end");
    if (loopStart == nullptr || loopEnd == nullptr)
        return false;

    if (const auto* loopMode = findEffectiveOpcode(section, "loop_mode"))
    {
        const auto lowered = toLowerAscii(loopMode->value);
        if (lowered == "no_loop" || lowered == "one_shot")
            return false;
    }

    return true;
}

bool isVelocityCrossfadeOpcode(const std::string& opcodeName)
{
    return opcodeName == "xfin_lovel"
        || opcodeName == "xfin_hivel"
        || opcodeName == "xfout_lovel"
        || opcodeName == "xfout_hivel";
}

fs::path resolveSamplePath(const SfzNormalizedSection& section,
                           const SfzResolvedOpcode& sampleOpcode)
{
    const auto normalizeSeparators = [](std::string value)
    {
        std::replace(value.begin(), value.end(), '\\', '/');
        return value;
    };

    auto sampleReference = normalizeSeparators(sampleOpcode.value);
    auto samplePath = fs::path(sampleReference);
    if (!samplePath.is_absolute())
    {
        if (const auto* defaultPath = findEffectiveOpcode(section, "default_path");
            defaultPath != nullptr && !defaultPath->value.empty())
        {
            sampleReference = normalizeSeparators(defaultPath->value) + sampleReference;
            samplePath = fs::path(sampleReference);
        }
    }

    if (samplePath.is_absolute())
        return samplePath.lexically_normal();

    fs::path resolvedBase = sampleOpcode.resolutionBasePath.empty()
        ? fs::path(sampleOpcode.location.sourcePath).parent_path()
        : fs::path(sampleOpcode.resolutionBasePath);
    if (const auto* prefix = findEffectiveOpcode(section, "prefix_sfz_path");
        prefix != nullptr && !prefix->value.empty())
    {
        resolvedBase /= fs::path(normalizeSeparators(prefix->value));
    }

    return (resolvedBase / samplePath).lexically_normal();
}

CrossfadeOpcodeKey makeCrossfadeOpcodeKey(const SfzImportTraceEntry& trace)
{
    return {
        trace.location.sourcePath,
        trace.location.lineNumber,
        trace.location.columnNumber,
        trace.location.scope,
        toLowerAscii(trace.opcodeName)
    };
}

CrossfadeOpcodeKey makeCrossfadeOpcodeKey(const SfzResolvedOpcode& opcode)
{
    return {
        opcode.location.sourcePath,
        opcode.location.lineNumber,
        opcode.location.columnNumber,
        opcode.location.scope,
        toLowerAscii(opcode.name)
    };
}

std::set<CrossfadeOpcodeKey> collectUnsupportedVelocityCrossfadeOpcodes(
    const SfzImportAnalysisResult& analysis)
{
    std::set<CrossfadeOpcodeKey> unsupportedOpcodes;
    for (const auto& trace : analysis.report.traceEntries)
    {
        const auto opcodeName = toLowerAscii(trace.opcodeName);
        if (!isVelocityCrossfadeOpcode(opcodeName))
            continue;

        if (trace.disposition != SfzImportSupportDisposition::converted)
            unsupportedOpcodes.insert(makeCrossfadeOpcodeKey(trace));
    }

    return unsupportedOpcodes;
}

bool sectionUsesUnsupportedVelocityCrossfade(
    const SfzNormalizedSection& section,
    const std::set<CrossfadeOpcodeKey>& unsupportedOpcodes)
{
    for (const auto& opcode : section.effectiveOpcodes)
    {
        if (!isVelocityCrossfadeOpcode(toLowerAscii(opcode.name)))
            continue;

        if (unsupportedOpcodes.find(makeCrossfadeOpcodeKey(opcode)) != unsupportedOpcodes.end())
            return true;
    }

    return false;
}

std::string semanticDependencyFeature(const SfzImportSemanticDependency& dependency)
{
    const auto controllerSuffix = dependency.controllerNumber >= 0
        ? " (CC" + std::to_string(dependency.controllerNumber) + ")"
        : std::string {};
    switch (dependency.kind)
    {
        case SfzImportSemanticDependencyKind::controllerRange:
            return "MIDI controller range" + controllerSuffix;
        case SfzImportSemanticDependencyKind::controllerTriggerRange:
            return "MIDI controller trigger range" + controllerSuffix;
        case SfzImportSemanticDependencyKind::sustainPedalState:
            return "sustain-pedal state" + controllerSuffix;
        case SfzImportSemanticDependencyKind::triggerEvent:
            return "trigger event";
        case SfzImportSemanticDependencyKind::randomPolicy:
            return "random selection policy";
        case SfzImportSemanticDependencyKind::switchCondition:
            return "switch condition";
        case SfzImportSemanticDependencyKind::controllerDefault:
            return "controller default" + controllerSuffix;
        case SfzImportSemanticDependencyKind::controllerModulation:
            return "controller modulation" + controllerSuffix;
        case SfzImportSemanticDependencyKind::presentationMetadata:
            return "presentation metadata";
        case SfzImportSemanticDependencyKind::none:
            break;
    }
    return "unsupported semantic dependency";
}

std::string sourceSectionLabel(const SfzOpcodeScope scope)
{
    switch (scope)
    {
        case SfzOpcodeScope::control: return "<control>";
        case SfzOpcodeScope::global: return "<global>";
        case SfzOpcodeScope::master: return "<master>";
        case SfzOpcodeScope::group: return "<group>";
        case SfzOpcodeScope::region: return "<region>";
        case SfzOpcodeScope::curve: return "<curve>";
        case SfzOpcodeScope::effect: return "<effect>";
        case SfzOpcodeScope::midi: return "<midi>";
        case SfzOpcodeScope::sample: return "<sample>";
        case SfzOpcodeScope::unknown: break;
    }
    return "<unknown>";
}

std::vector<SfzImportOmittedRegionSummary> buildOmittedRegionSummaries(
    const SfzImportReport& report)
{
    using SummaryKey = std::tuple<int, int, int, std::string>;
    struct SummaryBuilder
    {
        SfzImportOmittedRegionSummary summary;
        std::set<std::size_t> regionDocumentOrders;
    };

    std::map<SummaryKey, SummaryBuilder> builders;
    for (const auto& region : report.regionSemanticAnalysis)
    {
        if (region.safeToProjectUnconditionally)
            continue;

        for (const auto& dependency : region.dependencies)
        {
            if (!dependency.affectsRegionEligibility
                || dependency.support == SfzImportSemanticSupport::native)
            {
                continue;
            }

            const auto key = SummaryKey {
                static_cast<int>(dependency.kind),
                dependency.controllerNumber,
                static_cast<int>(dependency.location.scope),
                dependency.location.sourcePath
            };
            auto& builder = builders[key];
            if (builder.summary.feature.empty())
            {
                builder.summary.dependencyKind = dependency.kind;
                builder.summary.controllerNumber = dependency.controllerNumber;
                builder.summary.sourceScope = dependency.location.scope;
                builder.summary.sourcePath = dependency.location.sourcePath;
                builder.summary.firstSourceLineNumber = dependency.location.lineNumber;
                builder.summary.feature = semanticDependencyFeature(dependency);
            }
            else if (dependency.location.lineNumber > 0
                     && (builder.summary.firstSourceLineNumber == 0
                         || dependency.location.lineNumber
                             < builder.summary.firstSourceLineNumber))
            {
                builder.summary.firstSourceLineNumber = dependency.location.lineNumber;
            }
            builder.regionDocumentOrders.insert(region.documentOrder);
        }
    }

    std::vector<SfzImportOmittedRegionSummary> summaries;
    summaries.reserve(builders.size());
    for (auto& [_, builder] : builders)
    {
        builder.summary.affectedRegionCount = builder.regionDocumentOrders.size();
        summaries.push_back(std::move(builder.summary));
    }
    return summaries;
}

std::vector<std::string> buildProjectNotes(const SfzImportReport& report)
{
    std::vector<std::string> notes;
    notes.push_back("Imported from SFZ source: " + report.rootDocumentPath);

    std::ostringstream summary;
    summary << "SFZ import summary: " << report.summary.sectionCount << " sections, "
            << report.summary.opcodeCount << " opcodes, "
            << report.summary.approximatedOpcodeCount << " approximated, "
            << report.summary.reportedOnlyOpcodeCount << " review-only, "
            << report.summary.blockingOpcodeCount << " blocking.";
    notes.push_back(summary.str());

    if (report.summary.unsafeUnconditionalRegionCount > 0)
    {
        notes.push_back(
            "SFZ semantic safety analysis: "
            + std::to_string(report.summary.unsafeUnconditionalRegionCount)
            + " of " + std::to_string(report.summary.semanticAnalyzedRegionCount)
            + " regions contained incomplete sound-critical eligibility dependencies and were omitted by the sound-safe import policy.");
    }

    const auto hasConvertedScopedGain = std::any_of(
        report.opcodeSupport.begin(),
        report.opcodeSupport.end(),
        [](const SfzImportOpcodeSupportSummary& summary)
        {
            return summary.opcodeName == "volume"
                && summary.disposition == SfzImportSupportDisposition::converted
                && (summary.scope == SfzOpcodeScope::master
                    || summary.scope == SfzOpcodeScope::group
                    || summary.scope == SfzOpcodeScope::region);
        });
    if (hasConvertedScopedGain)
    {
        notes.push_back(
            "SFZ gain mapping preserves supported master, group, and region-local volume scopes in authored gain metadata.");
    }
    return notes;
}

std::vector<std::string> buildAuthoringNotes(const SfzImportReport& report)
{
    std::vector<std::string> notes;
    std::map<std::string, std::pair<std::string, std::size_t>> findingsByCode;

    for (const auto& finding : report.findings)
    {
        auto& entry = findingsByCode[finding.code];
        if (entry.first.empty())
            entry.first = finding.summary;
        ++entry.second;
    }

    for (const auto& [code, entry] : findingsByCode)
    {
        std::ostringstream note;
        note << "SFZ import finding [" << code << "]: " << entry.first;
        if (entry.second > 1)
            note << " (" << entry.second << " occurrences)";
        notes.push_back(note.str());
    }

    if (report.summary.suppressedFindingCount > 0)
    {
        notes.push_back("SFZ import omitted "
                        + std::to_string(report.summary.suppressedFindingCount)
                        + " additional findings after reaching the diagnostic safety limit.");
    }

    const auto hasApproximatedGainScope = std::any_of(
        report.opcodeSupport.begin(),
        report.opcodeSupport.end(),
        [](const SfzImportOpcodeSupportSummary& summary)
        {
            return summary.opcodeName == "volume"
                && summary.disposition == SfzImportSupportDisposition::approximated;
        });
    if (hasApproximatedGainScope)
    {
        notes.push_back(
            "SFZ gain review: at least one source gain scope still requires approximation; review compatibility findings before trusting loudness parity.");
    }

    return notes;
}

RuntimeProjectModel buildProvisionalProject(const RuntimeProjectModel& baseProject,
                                            const SfzImportProjectionResult& projection)
{
    auto project = baseProject;
    project.sampleSources.insert(project.sampleSources.end(),
                                 projection.sampleSources.begin(),
                                 projection.sampleSources.end());
    project.authoring.masterGainDb += projection.masterGainDb;
    if (project.schemaVersion >= 4 && project.authoring.schemaVersion >= 3)
    {
        project.authoring.groups.insert(project.authoring.groups.end(),
                                        projection.groups.begin(),
                                        projection.groups.end());
    }
    project.authoring.zones.insert(project.authoring.zones.end(),
                                   projection.zones.begin(),
                                   projection.zones.end());
    for (const auto& importedDefault : projection.controllerDefaults)
    {
        const auto existing = std::find_if(
            project.authoring.controllerDefaults.begin(), project.authoring.controllerDefaults.end(),
            [&](const RuntimeControllerDefault& value)
            { return value.controllerNumber == importedDefault.controllerNumber; });
        if (existing == project.authoring.controllerDefaults.end())
            project.authoring.controllerDefaults.push_back(importedDefault);
        else
            *existing = importedDefault;
    }
    project.notes.insert(project.notes.end(),
                         projection.projectNotes.begin(),
                         projection.projectNotes.end());
    project.authoring.notes.insert(project.authoring.notes.end(),
                                   projection.authoringNotes.begin(),
                                   projection.authoringNotes.end());
    if (!projection.zones.empty())
        project.authoring.selectedZoneId = projection.zones.front().id;

    if (project.schemaVersion >= 6 && project.authoring.schemaVersion >= 5)
    {
        auto nextDisplayOrder = static_cast<int>(project.authoring.articulations.size());
        bool hasDefaultArticulation = std::any_of(
            project.authoring.articulations.begin(),
            project.authoring.articulations.end(),
            [](const RuntimeProjectArticulationDefinition& articulation)
            {
                return articulation.isDefault;
            });
        std::set<std::string> articulationIds;
        for (const auto& articulation : project.authoring.articulations)
        {
            if (articulation.id.empty())
                continue;
            articulationIds.insert(articulation.id);
            nextDisplayOrder = std::max(nextDisplayOrder, articulation.displayOrder + 1);
        }

        for (const auto& zone : project.authoring.zones)
        {
            if (zone.articulationId.empty()
                || !articulationIds.insert(zone.articulationId).second)
            {
                continue;
            }

            RuntimeProjectArticulationDefinition articulation;
            articulation.id = zone.articulationId;
            articulation.displayName = buildArticulationDisplayName(zone.articulationId);
            articulation.displayOrder = nextDisplayOrder++;
            articulation.isDefault = !hasDefaultArticulation;
            hasDefaultArticulation = hasDefaultArticulation || articulation.isDefault;
            project.authoring.articulations.push_back(std::move(articulation));
        }

        std::stable_sort(project.authoring.articulations.begin(),
                         project.authoring.articulations.end(),
                         [](const RuntimeProjectArticulationDefinition& left,
                            const RuntimeProjectArticulationDefinition& right)
                         {
                             if (left.displayOrder != right.displayOrder)
                                 return left.displayOrder < right.displayOrder;
                             return left.id < right.id;
                         });

        for (std::size_t index = 0; index < project.authoring.articulations.size(); ++index)
        {
            auto& articulation = project.authoring.articulations[index];
            articulation.displayOrder = static_cast<int>(index);
            if (articulation.displayName.empty())
                articulation.displayName = buildArticulationDisplayName(articulation.id);
        }
    }

    if (project.schemaVersion >= 4 && project.authoring.schemaVersion >= 3)
    {
        auto nextDisplayOrder = static_cast<int>(project.authoring.groups.size());
        for (const auto& group : project.authoring.groups)
            nextDisplayOrder = std::max(nextDisplayOrder, group.displayOrder + 1);

        for (const auto& zone : project.authoring.zones)
        {
            if (zone.groupId.empty())
                continue;

            const auto existingGroup = std::find_if(project.authoring.groups.begin(),
                                                    project.authoring.groups.end(),
                                                    [&](const RuntimeProjectGroupDefinition& group)
                                                    {
                                                        return group.id == zone.groupId;
                                                    });
            if (existingGroup != project.authoring.groups.end())
                continue;

            RuntimeProjectGroupDefinition group;
            group.id = zone.groupId;
            group.displayName = zone.groupId;
            group.displayOrder = nextDisplayOrder++;
            group.workspaceVisible = true;
            group.gainDb = 0.0;
            group.pan = 0.0;
            group.auditionAnchorZoneId = zone.id;
            project.authoring.groups.push_back(std::move(group));
        }

        std::stable_sort(project.authoring.groups.begin(),
                         project.authoring.groups.end(),
                         [](const RuntimeProjectGroupDefinition& left,
                            const RuntimeProjectGroupDefinition& right)
                         {
                             if (left.displayOrder != right.displayOrder)
                                 return left.displayOrder < right.displayOrder;
                             return left.id < right.id;
                         });

        for (std::size_t index = 0; index < project.authoring.groups.size(); ++index)
        {
            auto& group = project.authoring.groups[index];
            group.displayOrder = static_cast<int>(index);
            if (group.displayName.empty())
                group.displayName = group.id;

            const auto anchorZone = std::find_if(project.authoring.zones.begin(),
                                                 project.authoring.zones.end(),
                                                 [&](const RuntimeProjectZoneDefinition& zone)
                                                 {
                                                     return zone.id == group.auditionAnchorZoneId
                                                         && zone.groupId == group.id;
                                                 });
            if (anchorZone != project.authoring.zones.end())
                continue;

            const auto firstMember = std::find_if(project.authoring.zones.begin(),
                                                  project.authoring.zones.end(),
                                                  [&](const RuntimeProjectZoneDefinition& zone)
                                                  {
                                                      return zone.groupId == group.id;
                                                  });
            group.auditionAnchorZoneId = firstMember != project.authoring.zones.end()
                ? firstMember->id
                : std::string {};
        }

        const auto selectedZone = std::find_if(project.authoring.zones.begin(),
                                               project.authoring.zones.end(),
                                               [&](const RuntimeProjectZoneDefinition& zone)
                                               {
                                                   return zone.id == project.authoring.selectedZoneId;
                                               });
        if (selectedZone != project.authoring.zones.end())
            project.authoring.selectedGroupId = selectedZone->groupId;
        else if (!project.authoring.groups.empty())
            project.authoring.selectedGroupId = project.authoring.groups.front().id;
        else
            project.authoring.selectedGroupId.clear();
    }

    return project;
}

bool projectUsesExplicitRoundRobin(const RuntimeProjectModel& project)
{
    return std::any_of(project.authoring.zones.begin(),
                       project.authoring.zones.end(),
                       [](const RuntimeProjectZoneDefinition& zone)
                       {
                           return zone.roundRobin.has_value();
                       });
}
} // namespace

SfzImportProjectionResult projectSfzImportAnalysis(const RuntimeProjectModel& baseProject,
                                                   const SfzImportAnalysisResult& analysis)
{
    return projectSfzImportAnalysis(baseProject,
                                    analysis,
                                    defaultSfzImportExecutionContext());
}

SfzImportProjectionResult projectSfzImportAnalysis(const RuntimeProjectModel& baseProject,
                                                   const SfzImportAnalysisResult& analysis,
                                                   const SfzImportExecutionContext& context)
{
    SfzImportProjectionResult result;
    context.reportProgress(SfzImportStage::projected, 0.80f);
    result.execution = analysis.execution;

    const auto initialCancellationReason = context.pollCancellation();
    if (initialCancellationReason != SfzImportCancellationReason::none)
    {
        result.state = "SFZ projection canceled";
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = initialCancellationReason;
        return result;
    }

    if (analysis.execution.canceled())
    {
        result.state = "SFZ projection canceled";
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = analysis.execution.cancellationReason;
        return result;
    }

    result.blocking = !analysis.analyzed
        || !analysis.report.available
        || analysis.report.blocking
        || analysis.report.reviewDisposition == SfzImportReviewDisposition::blocked;
    result.lossy = analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired;
    result.semanticAnalyzedRegionCount = analysis.report.summary.semanticAnalyzedRegionCount;
    result.unsafeUnconditionalRegionCount = analysis.report.summary.unsafeUnconditionalRegionCount;
    result.unsafeUnconditionalRegionDocumentOrders.reserve(
        result.unsafeUnconditionalRegionCount);
    for (const auto& region : analysis.report.regionSemanticAnalysis)
    {
        if (!region.safeToProjectUnconditionally)
            result.unsafeUnconditionalRegionDocumentOrders.push_back(region.documentOrder);
    }

    if (!analysis.analyzed)
    {
        result.state = "SFZ projection unavailable";
        result.issues.push_back("The SFZ document must be analyzed before projection can begin.");
        return result;
    }

    if (!analysis.normalizeResult.normalized)
    {
        result.state = "SFZ projection unavailable";
        result.issues.push_back("The SFZ document must normalize successfully before projection can begin.");
        return result;
    }

    if (!analysis.report.available)
    {
        result.state = "SFZ projection unavailable";
        result.issues.push_back("The SFZ report must be available before projection can begin.");
        return result;
    }

    if (result.blocking)
    {
        result.state = "SFZ projection blocked";
        result.issues.push_back("The SFZ report contains blocking findings and cannot be applied yet.");
        return result;
    }

    const std::set<std::size_t> unsafeRegionDocumentOrders(
        result.unsafeUnconditionalRegionDocumentOrders.begin(),
        result.unsafeUnconditionalRegionDocumentOrders.end());
    result.omittedUnsafeRegionCount = unsafeRegionDocumentOrders.size();
    result.omittedRegionSummaries = buildOmittedRegionSummaries(analysis.report);
    result.lossy = result.lossy || result.omittedUnsafeRegionCount > 0;

    std::set<std::string> usedSampleSourceIds;
    for (const auto& sampleSource : baseProject.sampleSources)
        usedSampleSourceIds.insert(sampleSource.id);

    std::set<std::string> usedZoneIds;
    for (const auto& zone : baseProject.authoring.zones)
        usedZoneIds.insert(zone.id);

    std::set<std::string> usedGroupIds;
    for (const auto& group : baseProject.authoring.groups)
    {
        if (!group.id.empty())
            usedGroupIds.insert(group.id);
    }
    for (const auto& zone : baseProject.authoring.zones)
    {
        if (!zone.groupId.empty())
            usedGroupIds.insert(zone.groupId);
    }

    std::set<std::string> usedRoundRobinPoolIds;
    for (const auto& zone : baseProject.authoring.zones)
    {
        if (zone.roundRobin.has_value() && !zone.roundRobin->poolId.empty())
            usedRoundRobinPoolIds.insert(zone.roundRobin->poolId);
    }

    std::map<std::string, std::string> sampleSourceIdsByPath;
    for (const auto& sampleSource : baseProject.sampleSources)
        sampleSourceIdsByPath.emplace(fs::path(sampleSource.path).lexically_normal().generic_string(),
                                      sampleSource.id);

    const auto unsupportedCrossfadeOpcodes = collectUnsupportedVelocityCrossfadeOpcodes(analysis);
    std::map<std::string, std::string> roundRobinPoolIdsBySignature;
    std::map<std::string, std::string> projectedGroupIdsByCandidate;
    std::map<std::string, ProjectedGroupState> projectedGroupStates;
    ScopedGainState scopedGainState;

    std::map<int, int> importedControllerDefaults;
    for (const auto& section : analysis.normalizeResult.document.sections)
    {
        for (const auto& opcode : section.localOpcodes)
        {
            const auto controllerNumber = parseControllerOpcodeNumber(opcode.name, "set_cc");
            if (!controllerNumber.has_value()) continue;
            importedControllerDefaults[*controllerNumber]
                = std::clamp(parseIntValue(opcode.value).value_or(0), 0, 127);
        }
    }
    for (const auto& [controllerNumber, value] : importedControllerDefaults)
        result.controllerDefaults.push_back({ controllerNumber, value });

    result.projectNotes = buildProjectNotes(analysis.report);
    result.authoringNotes = buildAuthoringNotes(analysis.report);
    for (const auto& omission : result.omittedRegionSummaries)
    {
        std::ostringstream note;
        note << "SFZ sound-safe omission: " << omission.feature
             << " from " << sourceSectionLabel(omission.sourceScope)
             << " affected " << omission.affectedRegionCount << " omitted region";
        if (omission.affectedRegionCount != 1)
            note << "s";
        if (!omission.sourcePath.empty())
        {
            note << " in " << omission.sourcePath;
            if (omission.firstSourceLineNumber > 0)
                note << ":" << omission.firstSourceLineNumber;
        }
        note << ".";
        result.authoringNotes.push_back(note.str());
    }

    for (const auto& section : analysis.normalizeResult.document.sections)
    {
        const auto cancellationReason = context.pollCancellation();
        if (cancellationReason != SfzImportCancellationReason::none)
        {
            result.projected = false;
            result.playable = false;
            result.sampleSources.clear();
            result.groups.clear();
            result.zones.clear();
            result.controllerDefaults.clear();
            result.projectNotes.clear();
            result.authoringNotes.clear();
            result.state = "SFZ projection canceled";
            result.execution.disposition = SfzImportExecutionDisposition::canceled;
            result.execution.cancellationReason = cancellationReason;
            context.reportProgress(SfzImportStage::canceled, 0.85f);
            return result;
        }

        if (section.scope == SfzOpcodeScope::master)
        {
            if (const auto* volumeOpcode = findLocalOpcode(section, "volume"))
            {
                scopedGainState.masterGainDb = parseDoubleValue(volumeOpcode->value).value_or(0.0);
                scopedGainState.hasMasterGain = true;
            }
            else
            {
                scopedGainState.masterGainDb = 0.0;
                scopedGainState.hasMasterGain = false;
            }

            continue;
        }

        if (section.scope == SfzOpcodeScope::group)
        {
            const auto* volumeOpcode = findLocalOpcode(section, "volume");
            const auto* groupVolumeOpcode = findLocalOpcode(section, "group_volume");
            scopedGainState.groupGainDb =
                (volumeOpcode != nullptr ? parseDoubleValue(volumeOpcode->value).value_or(0.0) : 0.0)
                + (groupVolumeOpcode != nullptr ? parseDoubleValue(groupVolumeOpcode->value).value_or(0.0) : 0.0);
            scopedGainState.hasGroupGain = volumeOpcode != nullptr || groupVolumeOpcode != nullptr;

            continue;
        }

        if (section.scope != SfzOpcodeScope::region)
            continue;

        if (unsafeRegionDocumentOrders.count(section.documentOrder) > 0)
            continue;

        const auto scopedGainContribution = buildScopedGainContribution(section, scopedGainState);
        const auto* sampleOpcode = findEffectiveOpcode(section, "sample");
        if (sampleOpcode == nullptr || sampleOpcode->value.empty())
        {
            result.issues.push_back("Region at document order " + std::to_string(section.documentOrder)
                                    + " did not resolve a sample opcode.");
            continue;
        }

        const auto samplePath = resolveSamplePath(section, *sampleOpcode);
        const auto canonicalSamplePath = samplePath.generic_string();
        const auto preserveVelocityCrossfade =
            !sectionUsesUnsupportedVelocityCrossfade(section, unsupportedCrossfadeOpcodes);

        std::string sampleSourceId;
        const auto existingSampleSource = sampleSourceIdsByPath.find(canonicalSamplePath);
        if (existingSampleSource != sampleSourceIdsByPath.end())
        {
            sampleSourceId = existingSampleSource->second;
        }
        else
        {
            RuntimeProjectSampleSource sampleSource;
            sampleSource.id = makeUniqueId(usedSampleSourceIds, "sfz-sample-" + slugify(sampleStem(samplePath)));
            sampleSource.path = canonicalSamplePath;
            sampleSource.role = "sfz-region-sample";
            sampleSourceId = sampleSource.id;
            sampleSourceIdsByPath.emplace(canonicalSamplePath, sampleSource.id);
            result.sampleSources.push_back(std::move(sampleSource));
        }

        RuntimeProjectZoneDefinition zone;
        zone.id = makeUniqueId(usedZoneIds, "sfz-zone-" + std::to_string(section.documentOrder));
        zone.sampleSourceId = sampleSourceId;
        zone.displayName = buildDisplayName(samplePath);
        zone.rootKey = parseEffectiveKeyValue(section, "pitch_keycenter", 60);
        zone.keyLow = parseEffectiveKeyValue(section, "lokey", zone.rootKey);
        zone.keyHigh = parseEffectiveKeyValue(section, "hikey", zone.rootKey);
        zone.velocityLow = normalizePlayableVelocityLowerBound(
            parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                              ? findEffectiveOpcode(section, "lovel")->value
                              : "1")
                .value_or(1));
        zone.velocityHigh = parseIntValue(findEffectiveOpcode(section, "hivel") != nullptr
                                              ? findEffectiveOpcode(section, "hivel")->value
                                              : "127")
                                .value_or(127);
        VelocityCrossfadeDescriptor crossfade;
        crossfade.fadeInLowVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfin_lovel") != nullptr
                              ? findEffectiveOpcode(section, "xfin_lovel")->value
                              : "0")
                .value_or(0);
        crossfade.fadeInHighVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfin_hivel") != nullptr
                              ? findEffectiveOpcode(section, "xfin_hivel")->value
                              : "0")
                .value_or(0);
        crossfade.fadeOutLowVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfout_lovel") != nullptr
                              ? findEffectiveOpcode(section, "xfout_lovel")->value
                              : "0")
                .value_or(0);
        crossfade.fadeOutHighVelocity =
            parseIntValue(findEffectiveOpcode(section, "xfout_hivel") != nullptr
                              ? findEffectiveOpcode(section, "xfout_hivel")->value
                              : "0")
                .value_or(0);
        if (crossfade.fadeInLowVelocity == 0 && crossfade.fadeInHighVelocity > 0)
            crossfade.fadeInLowVelocity = 1;
        if (crossfade.fadeOutLowVelocity == 0 && crossfade.fadeOutHighVelocity > 0)
            crossfade.fadeOutLowVelocity = 1;
        if (drs::engine::hasCompleteFadeIn(crossfade))
            zone.velocityLow = crossfade.fadeInLowVelocity;
        if (drs::engine::hasCompleteFadeOut(crossfade))
            zone.velocityHigh = crossfade.fadeOutHighVelocity;
        if (preserveVelocityCrossfade)
            zone.velocityCrossfade = crossfade;
        zone.gainDb = scopedGainContribution.hasRegionGain ? scopedGainContribution.regionGainDb : 0.0;
        zone.fineTuneCents = parseDoubleValue(findEffectiveOpcode(section, "tune") != nullptr
                                                  ? findEffectiveOpcode(section, "tune")->value
                                                  : "0")
                                 .value_or(0.0);
        zone.amplitudeVelocityTracking = std::clamp(
            parseDoubleValue(findEffectiveOpcode(section, "amp_veltrack") != nullptr
                                 ? findEffectiveOpcode(section, "amp_veltrack")->value
                                 : "100")
                .value_or(100.0),
            0.0,
            100.0);
        if (const auto* pitchKeytrack = findEffectiveOpcode(section, "pitch_keytrack"))
        {
            const auto value = parseDoubleValue(pitchKeytrack->value);
            if (value.has_value() && std::abs(*value) < 0.000001)
                zone.performance.pitchSource = PerformancePitchSource::eventKeyFixedPitch;
        }
        zone.releaseSeconds = parseDoubleValue(findEffectiveOpcode(section, "ampeg_release") != nullptr
                                                   ? findEffectiveOpcode(section, "ampeg_release")->value
                                                   : "0")
                                  .value_or(0.0);
        zone.releaseShape = sfzDefaultReleaseShape;
        if (const auto* releaseShape = findEffectiveOpcode(section, "ampeg_release_shape"))
            zone.releaseShape = parseDoubleValue(releaseShape->value).value_or(sfzDefaultReleaseShape);
        zone.sampleStartFrame = parseFrameValue(findEffectiveOpcode(section, "offset") != nullptr
                                                    ? findEffectiveOpcode(section, "offset")->value
                                                    : "0")
                                    .value_or(0);
        zone.loopEnabled = shouldEnableLoop(section);
        zone.loopStartFrame = parseFrameValue(findEffectiveOpcode(section, "loop_start") != nullptr
                                                  ? findEffectiveOpcode(section, "loop_start")->value
                                                  : "0")
                                  .value_or(0);
        zone.loopEndFrame = parseFrameValue(findEffectiveOpcode(section, "loop_end") != nullptr
                                                ? findEffectiveOpcode(section, "loop_end")->value
                                                : "0")
                                .value_or(0);
        const auto sequentialRoundRobin = parseSequentialRoundRobinSlot(section);
        zone.roundRobinLength = sequentialRoundRobin.length;
        zone.roundRobinPosition = sequentialRoundRobin.position;
        zone.articulationId = buildArticulationId(section);
        const auto groupIdCandidate = buildGroupId(section,
                                                   zone.articulationId,
                                                   zone.keyLow,
                                                   zone.keyHigh);
        const auto existingProjectedGroupId = projectedGroupIdsByCandidate.find(groupIdCandidate);
        if (existingProjectedGroupId != projectedGroupIdsByCandidate.end())
        {
            zone.groupId = existingProjectedGroupId->second;
        }
        else
        {
            zone.groupId = makeUniqueId(usedGroupIds, groupIdCandidate);
            projectedGroupIdsByCandidate.emplace(groupIdCandidate, zone.groupId);
        }
        zone.roundRobin = buildSequentialRoundRobinDescriptor(zone,
                                                              usedRoundRobinPoolIds,
                                                              roundRobinPoolIdsBySignature);

        bool hasControllerTrigger = false;
        int triggerControllerNumber = -1;
        zone.controllerConditions = buildControllerConditions(section,
                                                              hasControllerTrigger,
                                                              triggerControllerNumber);

        if (const auto* trigger = findEffectiveOpcode(section, "trigger"))
        {
            const auto lowered = toLowerAscii(trigger->value);
            if (lowered == "release")
            {
                zone.triggerMode = ZoneTriggerMode::oneShot;
                zone.performance.event = PerformanceEventKind::release;
                zone.performance.sustain = PerformanceSustainCondition::pedalUp;
            }
        }
        if (hasControllerTrigger)
        {
            zone.triggerMode = ZoneTriggerMode::oneShot;
            zone.performance.pitchSource = PerformancePitchSource::fixedRoot;
            zone.performance.triggerControllerNumber = triggerControllerNumber;
            const auto triggerCondition = std::find_if(
                zone.controllerConditions.begin(), zone.controllerConditions.end(),
                [&](const RuntimeControllerCondition& condition)
                { return condition.controllerNumber == triggerControllerNumber; });
            if (triggerControllerNumber == 64 && triggerCondition != zone.controllerConditions.end())
            {
                zone.performance.event = triggerCondition->minimumValue >= 64
                    ? PerformanceEventKind::pedalDown : PerformanceEventKind::pedalUp;
            }
            else
            {
                zone.performance.event = PerformanceEventKind::controllerChange;
            }
        }

        if (scopedGainContribution.hasMasterGain)
            result.masterGainDb = scopedGainContribution.masterGainDb;

        if (!zone.groupId.empty())
        {
            const auto scopedGroupGainDb = scopedGainContribution.hasGroupGain
                ? scopedGainContribution.groupGainDb : 0.0;
            auto projectedGroup = projectedGroupStates.find(zone.groupId);
            if (projectedGroup == projectedGroupStates.end())
            {
                RuntimeProjectGroupDefinition group;
                group.id = zone.groupId;
                group.displayName = zone.groupId;
                group.displayOrder = static_cast<int>(result.groups.size());
                group.workspaceVisible = true;
                group.gainDb = scopedGroupGainDb;
                group.pan = 0.0;
                group.auditionAnchorZoneId = zone.id;
                result.groups.push_back(std::move(group));

                ProjectedGroupState state;
                state.groupIndex = result.groups.size() - 1;
                state.sharedGainDb = scopedGroupGainDb;
                projectedGroup = projectedGroupStates.emplace(zone.groupId, std::move(state)).first;
            }
            else
            {
                auto& state = projectedGroup->second;
                if (!state.bakedIntoZones
                    && std::abs(state.sharedGainDb - scopedGroupGainDb) > 1.0e-9)
                {
                    for (const auto zoneIndex : state.zoneIndices)
                        result.zones[zoneIndex].gainDb += state.sharedGainDb;

                    result.groups[state.groupIndex].gainDb = 0.0;
                    state.bakedIntoZones = true;
                }

                if (state.bakedIntoZones)
                    zone.gainDb += scopedGroupGainDb;
            }
        }

        result.zones.push_back(std::move(zone));
        if (!result.zones.back().groupId.empty())
            projectedGroupStates[result.zones.back().groupId].zoneIndices.push_back(result.zones.size() - 1);
    }

    if (result.zones.empty())
    {
        result.state = "SFZ projection failed";
        if (result.omittedUnsafeRegionCount > 0)
        {
            result.issues.push_back(
                "The sound-safe import policy omitted all "
                + std::to_string(result.omittedUnsafeRegionCount)
                + " conditional regions, leaving no safe zones to import.");
        }
        else
        {
            result.issues.push_back("Projection did not create any zones.");
        }
        return result;
    }

    const auto finalCancellationReason = context.pollCancellation();
    if (finalCancellationReason != SfzImportCancellationReason::none)
    {
        result.sampleSources.clear();
        result.groups.clear();
        result.zones.clear();
        result.controllerDefaults.clear();
        result.projectNotes.clear();
        result.authoringNotes.clear();
        result.state = "SFZ projection canceled";
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = finalCancellationReason;
        context.reportProgress(SfzImportStage::canceled, 0.95f);
        return result;
    }

    auto provisionalProject = buildProvisionalProject(baseProject, result);
    if (projectUsesExplicitRoundRobin(provisionalProject)
        && provisionalProject.schemaVersion < 3)
    {
        const auto migration = migrateRuntimeProjectToPhase3RoundRobinSchema(provisionalProject);
        if (!migration.valid)
        {
            result.state = "SFZ projection failed";
            result.issues.insert(result.issues.end(), migration.issues.begin(), migration.issues.end());
            return result;
        }

        provisionalProject = migration.project;
    }

    const auto validation = validateRuntimeProjectModel(provisionalProject);
    if (!validation.valid)
    {
        result.state = "SFZ projection failed";
        result.issues.insert(result.issues.end(), validation.issues.begin(), validation.issues.end());
        return result;
    }

    PlaybackSnapshotBuilder snapshotBuilder;
    const auto snapshotRequest = snapshotBuilder.requestBuild(1, false);
    const auto snapshotResult = snapshotBuilder.buildSnapshot(snapshotRequest, provisionalProject);
    result.playable = snapshotResult.activationEligible;
    if (!snapshotResult.activationEligible)
    {
        result.state = "SFZ projection failed";
        for (const auto& finding : snapshotResult.findings)
        {
            if (finding.severity == PlaybackSnapshotFindingSeverity::error)
                result.issues.push_back(finding.message);
        }
        return result;
    }

    const auto completionCancellationReason = context.pollCancellation();
    if (completionCancellationReason != SfzImportCancellationReason::none)
    {
        result.sampleSources.clear();
        result.zones.clear();
        result.controllerDefaults.clear();
        result.projectNotes.clear();
        result.authoringNotes.clear();
        result.state = "SFZ projection canceled";
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = completionCancellationReason;
        context.reportProgress(SfzImportStage::canceled, 0.98f);
        return result;
    }

    result.projected = true;
    result.execution.disposition = SfzImportExecutionDisposition::completed;
    result.execution.failureReason = SfzImportFailureReason::none;
    result.state = result.omittedUnsafeRegionCount > 0
        ? "SFZ sound-safe projection ready for reviewed apply"
        : (result.lossy ? "SFZ projection ready for reviewed apply" : "SFZ projection ready");
    context.reportProgress(SfzImportStage::reviewReady, 1.0f);
    return result;
}

SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                   const std::string& sfzPath)
{
    return projectSfzImportDocument(baseProject,
                                     sfzPath,
                                     defaultSfzImportExecutionContext());
}

SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                   const std::string& sfzPath,
                                                   const SfzImportExecutionContext& context)
{
    return projectSfzImportAnalysis(baseProject,
                                    analyzeSfzImportDocument(sfzPath, context),
                                    context);
}

RuntimeProjectDocumentActionResult applySfzImportProjection(AuthoringSession& authoringSession,
                                                            SfzImportProjectionResult projection,
                                                            const std::string& label)
{
    if (!projection.projected)
    {
        RuntimeProjectDocumentActionResult result;
        result.state = "SFZ import apply rejected";
        result.issues = projection.issues.empty()
            ? std::vector<std::string> { "The SFZ projection is not ready to apply." }
            : projection.issues;
        result.documentState = authoringSession.getDocumentState();
        return result;
    }

    return authoringSession.appendImportedContent(std::move(projection.sampleSources),
                                                  std::move(projection.zones),
                                                  projection.masterGainDb,
                                                  std::move(projection.groups),
                                                  std::move(projection.projectNotes),
                                                  std::move(projection.authoringNotes),
                                                  label,
                                                  false,
                                                  std::move(projection.controllerDefaults));
}
} // namespace drs::engine
