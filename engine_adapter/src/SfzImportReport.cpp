#include "drs/engine/SfzImportReport.h"
#include "drs/engine/SfzRegionContract.h"
#include "drs/engine/VelocityCrossfade.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <filesystem>
#include <limits>
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

std::optional<int> parseControllerNumber(const std::string& opcodeName,
                                         const std::string& prefix)
{
    if (opcodeName.size() <= prefix.size()
        || opcodeName.compare(0, prefix.size(), prefix) != 0)
    {
        return std::nullopt;
    }

    const auto suffix = opcodeName.substr(prefix.size());
    if (!std::all_of(suffix.begin(), suffix.end(), [](const unsigned char character)
        {
            return std::isdigit(character) != 0;
        }))
    {
        return std::nullopt;
    }

    auto controllerNumber = -1;
    try
    {
        controllerNumber = std::stoi(suffix);
    }
    catch (...)
    {
        return std::nullopt;
    }
    if (controllerNumber < 0 || controllerNumber > 127)
        return std::nullopt;
    return controllerNumber;
}

std::optional<int> parseEmbeddedControllerNumber(const std::string& opcodeName,
                                                 const std::string& marker)
{
    const auto markerPosition = opcodeName.rfind(marker);
    if (markerPosition == std::string::npos)
        return std::nullopt;

    return parseControllerNumber(opcodeName.substr(markerPosition), marker);
}

std::optional<SfzImportSemanticDependency> classifySemanticDependency(
    const SfzResolvedOpcode& opcode)
{
    const auto opcodeName = toLowerAscii(opcode.name);
    auto dependency = SfzImportSemanticDependency {};
    dependency.opcodeName = opcodeName;
    dependency.opcodeValue = opcode.value;
    dependency.inherited = opcode.inherited;
    dependency.location = opcode.location;

    if (const auto controllerNumber = parseControllerNumber(opcodeName, "label_cc"))
    {
        dependency.kind = SfzImportSemanticDependencyKind::presentationMetadata;
        dependency.impact = SfzImportSemanticImpact::presentationOnly;
        dependency.support = SfzImportSemanticSupport::unsupported;
        dependency.controllerNumber = *controllerNumber;
        return dependency;
    }

    if (const auto controllerNumber = parseControllerNumber(opcodeName, "set_cc"))
    {
        dependency.kind = SfzImportSemanticDependencyKind::controllerDefault;
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.support = SfzImportSemanticSupport::native;
        dependency.controllerNumber = *controllerNumber;
        return dependency;
    }

    const auto makeControllerCondition = [&](const int controllerNumber,
                                             const bool triggerRange)
    {
        dependency.kind = controllerNumber == 64
            ? SfzImportSemanticDependencyKind::sustainPedalState
            : (triggerRange
                   ? SfzImportSemanticDependencyKind::controllerTriggerRange
                   : SfzImportSemanticDependencyKind::controllerRange);
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.support = SfzImportSemanticSupport::native;
        dependency.affectsRegionEligibility = true;
        dependency.controllerNumber = controllerNumber;
        return dependency;
    };

    if (const auto controllerNumber = parseControllerNumber(opcodeName, "on_locc"))
        return makeControllerCondition(*controllerNumber, true);
    if (const auto controllerNumber = parseControllerNumber(opcodeName, "on_hicc"))
        return makeControllerCondition(*controllerNumber, true);
    if (const auto controllerNumber = parseControllerNumber(opcodeName, "locc"))
        return makeControllerCondition(*controllerNumber, false);
    if (const auto controllerNumber = parseControllerNumber(opcodeName, "hicc"))
        return makeControllerCondition(*controllerNumber, false);

    if (opcodeName == "trigger")
    {
        dependency.kind = SfzImportSemanticDependencyKind::triggerEvent;
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.affectsRegionEligibility = true;
        const auto triggerValue = toLowerAscii(opcode.value);
        dependency.support = triggerValue == "attack" || triggerValue == "release"
            ? SfzImportSemanticSupport::native
            : (triggerValue == "legato"
                   ? SfzImportSemanticSupport::partial
                   : SfzImportSemanticSupport::unsupported);
        return dependency;
    }

    if (opcodeName == "lorand" || opcodeName == "hirand")
    {
        dependency.kind = SfzImportSemanticDependencyKind::randomPolicy;
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.support = SfzImportSemanticSupport::unsupported;
        dependency.affectsRegionEligibility = true;
        return dependency;
    }

    if (opcodeName.rfind("sw_", 0) == 0)
    {
        dependency.kind = SfzImportSemanticDependencyKind::switchCondition;
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.support = SfzImportSemanticSupport::unsupported;
        dependency.affectsRegionEligibility = true;
        return dependency;
    }

    auto modulationController = parseEmbeddedControllerNumber(opcodeName, "oncc");
    if (!modulationController.has_value())
        modulationController = parseEmbeddedControllerNumber(opcodeName, "curvecc");
    if (modulationController.has_value())
    {
        dependency.kind = SfzImportSemanticDependencyKind::controllerModulation;
        dependency.impact = SfzImportSemanticImpact::soundCritical;
        dependency.support = SfzImportSemanticSupport::unsupported;
        dependency.controllerNumber = *modulationController;
        return dependency;
    }

    return std::nullopt;
}

std::string findEffectiveSampleReference(const SfzNormalizedSection& section);
std::optional<int> parseIntValue(const std::string& text);

SfzImportRegionSemanticAnalysis analyzeRegionSemantics(const SfzNormalizedSection& section)
{
    SfzImportRegionSemanticAnalysis analysis;
    analysis.documentOrder = section.documentOrder;
    analysis.sampleReference = findEffectiveSampleReference(section);

    std::vector<SfzImportSemanticDependency> candidates;
    std::set<int> referencedControllers;
    const auto* dynamicDamper = findEffectiveOpcode(section, "ampeg_dynamic");
    const auto hasNativeDamperReleaseBlock = dynamicDamper != nullptr
        && parseIntValue(dynamicDamper->value).value_or(0) == 1
        && findEffectiveOpcode(section, "ampeg_releasecc64") != nullptr
        && findEffectiveOpcode(section, "ampeg_release_curvecc64") != nullptr;
    for (const auto& opcode : section.effectiveOpcodes)
    {
        auto dependency = classifySemanticDependency(opcode);
        if (!dependency.has_value()
            || dependency->impact == SfzImportSemanticImpact::presentationOnly)
        {
            continue;
        }

        const auto dependencyOpcode = toLowerAscii(dependency->opcodeName);
        if (hasNativeDamperReleaseBlock
            && (dependencyOpcode == "ampeg_releasecc64"
                || dependencyOpcode == "ampeg_release_curvecc64"))
        {
            dependency->support = SfzImportSemanticSupport::native;
        }

        if (dependency->kind != SfzImportSemanticDependencyKind::controllerDefault
            && dependency->controllerNumber >= 0)
        {
            referencedControllers.insert(dependency->controllerNumber);
        }
        candidates.push_back(std::move(*dependency));
    }

    for (auto& dependency : candidates)
    {
        if (dependency.kind == SfzImportSemanticDependencyKind::controllerDefault
            && referencedControllers.count(dependency.controllerNumber) == 0)
        {
            continue;
        }

        analysis.hasSoundCriticalDependencies = true;
        const auto incomplete = dependency.support != SfzImportSemanticSupport::native;
        analysis.hasIncompleteSoundCriticalDependencies
            = analysis.hasIncompleteSoundCriticalDependencies || incomplete;
        if (dependency.affectsRegionEligibility && incomplete)
            analysis.safeToProjectUnconditionally = false;
        analysis.dependencies.push_back(std::move(dependency));
    }

    return analysis;
}

void publishRegionSemanticAnalysis(SfzImportReport& report,
                                   const SfzNormalizedDocument& document)
{
    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;

        auto analysis = analyzeRegionSemantics(section);
        ++report.summary.semanticAnalyzedRegionCount;
        if (!analysis.safeToProjectUnconditionally)
            ++report.summary.unsafeUnconditionalRegionCount;

        for (const auto& dependency : analysis.dependencies)
        {
            ++report.summary.semanticDependencyCount;
            if (dependency.impact == SfzImportSemanticImpact::soundCritical)
                ++report.summary.soundCriticalDependencyCount;
            if (dependency.impact == SfzImportSemanticImpact::soundCritical
                && dependency.support != SfzImportSemanticSupport::native)
            {
                ++report.summary.incompleteSoundCriticalDependencyCount;
            }
        }

        report.regionSemanticAnalysis.push_back(std::move(analysis));
    }
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

std::optional<std::uint64_t> parseUnsignedFrameValue(const std::string& text) noexcept
{
    if (text.empty())
        return std::nullopt;

    std::uint64_t value = 0;
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    if (result.ec != std::errc {} || result.ptr != end)
        return std::nullopt;
    return value;
}

int normalizePlayableVelocityLowerBound(const int velocity) noexcept
{
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

    return {
        parseIntValue(lengthOpcode != nullptr ? lengthOpcode->value : "1").value_or(0),
        parseIntValue(positionOpcode != nullptr ? positionOpcode->value : "1").value_or(0)
    };
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

fs::path resolveEffectiveSamplePath(const SfzNormalizedSection& section,
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

std::string findEffectiveSampleReference(const SfzNormalizedSection& section)
{
    const auto* sample = findEffectiveOpcode(section, "sample");
    if (sample == nullptr)
        return {};

    return resolveEffectiveSamplePath(section, *sample).generic_string();
}

std::string resolveSamplePathForSection(const SfzNormalizedSection& section,
                                        const SfzResolvedOpcode& opcode)
{
    return resolveEffectiveSamplePath(section, opcode).generic_string();
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

        const auto rootKey = parseEffectiveKeyValue(section, "pitch_keycenter", 60);
        const auto keyLow = parseEffectiveKeyValue(section, "lokey", rootKey);
        const auto keyHigh = parseEffectiveKeyValue(section, "hikey", rootKey);
        auto velocityLow = normalizePlayableVelocityLowerBound(
            parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                              ? findEffectiveOpcode(section, "lovel")->value
                              : "1")
                .value_or(1));
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
        const auto sequentialRoundRobin = parseSequentialRoundRobinSlot(section);
        region.roundRobinLength = sequentialRoundRobin.length;
        region.roundRobinPosition = sequentialRoundRobin.position;
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
        if (region.topologyZone.crossfade.fadeInLowVelocity == 0
            && region.topologyZone.crossfade.fadeInHighVelocity > 0)
        {
            region.topologyZone.crossfade.fadeInLowVelocity = 1;
        }
        if (region.topologyZone.crossfade.fadeOutLowVelocity == 0
            && region.topologyZone.crossfade.fadeOutHighVelocity > 0)
        {
            region.topologyZone.crossfade.fadeOutLowVelocity = 1;
        }

        const auto rootKey = parseEffectiveKeyValue(section, "pitch_keycenter", 60);
        const auto keyLow = parseEffectiveKeyValue(section, "lokey", rootKey);
        const auto keyHigh = parseEffectiveKeyValue(section, "hikey", rootKey);
        auto velocityLow = normalizePlayableVelocityLowerBound(
            parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                              ? findEffectiveOpcode(section, "lovel")->value
                              : "1")
                .value_or(1));
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
        const auto sequentialRoundRobin = parseSequentialRoundRobinSlot(section);
        region.topologyZone.roundRobinLength = sequentialRoundRobin.length;
        region.topologyZone.roundRobinPosition = sequentialRoundRobin.position;
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

std::map<CrossfadeOpcodeKey, OpcodeClassification> buildContinuousDamperCurveClassifications(
    const SfzNormalizedDocument& document)
{
    std::set<int> referencedIndices;
    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;
        const auto* dynamic = findEffectiveOpcode(section, "ampeg_dynamic");
        const auto* amount = findEffectiveOpcode(section, "ampeg_releasecc64");
        const auto* curve = findEffectiveOpcode(section, "ampeg_release_curvecc64");
        if (dynamic == nullptr || parseIntValue(dynamic->value).value_or(0) != 1
            || amount == nullptr || curve == nullptr)
            continue;
        if (const auto index = parseIntValue(curve->value); index.has_value())
            referencedIndices.insert(*index);
    }

    std::map<CrossfadeOpcodeKey, OpcodeClassification> classifications;
    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::curve)
            continue;
        const auto* indexOpcode = findEffectiveOpcode(section, "curve_index");
        const auto index = indexOpcode != nullptr ? parseIntValue(indexOpcode->value) : std::nullopt;
        if (!index.has_value() || referencedIndices.count(*index) == 0)
            continue;
        for (const auto& opcode : section.localOpcodes)
        {
            classifications.emplace(
                makeCrossfadeOpcodeKey(opcode),
                OpcodeClassification {
                    SfzImportSupportDisposition::converted,
                    "zone.damper.releaseCurve[" + std::to_string(*index) + "]",
                    "Curve points referenced by ampeg_release_curvecc64 compile into an immutable 128-value native damper table."
                });
        }
    }
    return classifications;
}

std::map<CrossfadeOpcodeKey, OpcodeClassification> buildContinuousDamperOpcodeClassifications(
    const SfzNormalizedDocument& document)
{
    std::map<CrossfadeOpcodeKey, OpcodeClassification> classifications;
    for (const auto& section : document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;
        const auto* dynamic = findEffectiveOpcode(section, "ampeg_dynamic");
        const auto* amount = findEffectiveOpcode(section, "ampeg_releasecc64");
        const auto* curve = findEffectiveOpcode(section, "ampeg_release_curvecc64");
        if (dynamic == nullptr || parseIntValue(dynamic->value).value_or(0) != 1
            || amount == nullptr || curve == nullptr)
            continue;

        classifications[makeCrossfadeOpcodeKey(*dynamic)] = {
            SfzImportSupportDisposition::converted,
            "zone.damper.dynamicRelease",
            "ampeg_dynamic=1 enables the bounded native amplitude-release controller path when the complete CC64 release block is present."
        };
        classifications[makeCrossfadeOpcodeKey(*amount)] = {
            SfzImportSupportDisposition::converted,
            "zone.damper.releaseAmountSeconds",
            "CC64 amplitude-release depth maps into the native bounded release-time amount."
        };
        classifications[makeCrossfadeOpcodeKey(*curve)] = {
            SfzImportSupportDisposition::converted,
            "zone.damper.releaseCurveIndex",
            "The CC64 release curve reference resolves to one immutable native 128-value table."
        };
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

void appendFindingsBounded(std::vector<SfzImportFinding>& destination,
                           const std::vector<SfzImportFinding>& source,
                           const std::size_t maximumFindingCount,
                           std::size_t& suppressedFindingCount)
{
    for (const auto& finding : source)
    {
        if (destination.size() >= maximumFindingCount)
        {
            ++suppressedFindingCount;
            continue;
        }

        destination.push_back(finding);
    }
}

void addClassificationFinding(std::vector<SfzImportFinding>& findings,
                               const OpcodeClassification& classification,
                               const SfzResolvedOpcode& opcode,
                               const std::string& sampleReference,
                               const std::size_t maximumFindingCount,
                               std::size_t& suppressedFindingCount)
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
    if (findings.size() >= maximumFindingCount)
    {
        ++suppressedFindingCount;
        return;
    }

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

    const auto invalidFrameClassification = [&]()
    {
        return OpcodeClassification {
            SfzImportSupportDisposition::blocking,
            "report.region.invalidFrame",
            "Region frame positions must be non-negative integers representable by the native 64-bit frame contract.",
            "sfz.region." + opcodeName + ".invalid",
            "Invalid SFZ region frame",
            "Opcode '" + opcode.name + "' has an invalid source-frame value and cannot be converted safely." };
    };

    if (opcodeName == "offset")
    {
        if (!parseUnsignedFrameValue(opcode.value).has_value())
            return invalidFrameClassification();
        return { SfzImportSupportDisposition::converted,
                 "zone.sampleStartFrame",
                 "SFZ offset maps directly to the native inclusive playback-start frame." };
    }

    if (opcodeName == "end")
    {
        if (opcode.value == "-1")
        {
            return { SfzImportSupportDisposition::converted,
                     "projection.omittedSilentRegion",
                     "The SFZ end=-1 silent-region sentinel is preserved by omitting the region from audible native projection." };
        }
        const auto value = parseUnsignedFrameValue(opcode.value);
        if (!value.has_value() || *value == std::numeric_limits<std::uint64_t>::max())
            return invalidFrameClassification();
        return { SfzImportSupportDisposition::reportedOnly,
                 "regionContract.playbackEndExclusive",
                 "SFZ end is normalized from inclusive N to native exclusive N+1, but project persistence and playback cutover land in the dedicated playback-end schema phase.",
                 "sfz.region.end.pending_schema",
                 "SFZ playback end is recognized but not yet persisted",
                 "The importer validates and normalizes the inclusive endpoint, but the current project schema cannot retain it yet." };
    }

    if (opcodeName == "loop_start")
    {
        if (!parseUnsignedFrameValue(opcode.value).has_value())
            return invalidFrameClassification();
        return { SfzImportSupportDisposition::converted,
                 "zone.loopStartFrame",
                 "SFZ loop_start maps directly to the native inclusive loop-start frame." };
    }

    if (opcodeName == "loop_end")
    {
        const auto value = parseUnsignedFrameValue(opcode.value);
        if (!value.has_value() || *value == std::numeric_limits<std::uint64_t>::max())
            return invalidFrameClassification();
        return { SfzImportSupportDisposition::converted,
                 "zone.loopEndFrameExclusive",
                 "SFZ loop_end is normalized once from inclusive N to native exclusive N+1 at import." };
    }

    if (opcodeName == "loop_mode")
    {
        const auto mode = parseSfzRegionLoopMode(opcode.value);
        if (!mode.has_value())
        {
            return { SfzImportSupportDisposition::reportedOnly,
                     "report.region.loopMode",
                     "The value is outside the portable SFZ v1 loop-mode contract.",
                     "sfz.region.loop_mode.unsupported",
                     "Unsupported SFZ loop mode",
                     "The importer recognizes loop_mode but cannot safely convert this value." };
        }

        if (*mode == SfzRegionLoopMode::noLoop
            || *mode == SfzRegionLoopMode::loopContinuous)
        {
            return { SfzImportSupportDisposition::converted,
                     "regionContract.loopMode + zone.loopEnabled",
                     "The typed SFZ loop mode has an exact current compatibility projection." };
        }

        return { SfzImportSupportDisposition::approximated,
                 "regionContract.loopMode + zone.loopEnabled",
                 "The typed SFZ loop mode is retained by the region contract, while the current boolean playback model cannot yet preserve its distinct note-off lifecycle.",
                 "sfz.region.loop_mode.compatibility_projection",
                 "SFZ loop mode requires a typed playback model",
                 "The current project model approximates one_shot as unlooped playback and loop_sustain as a continuous enabled loop until typed runtime support lands." };
    }

    if (opcodeName == "default_path")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.samplePath",
                 "The control-level default sample path is applied while resolving native sample sources." };
    }

    if (opcodeName == "key")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.keyRange + zone.rootKey",
                 "The SFZ key shorthand maps to identical native low, high, and root keys." };
    }

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

    if (opcodeName == "pitch_keytrack")
    {
        auto value = 100.0;
        try { value = std::stod(opcode.value); } catch (...) {}
        if (std::abs(value) < 0.000001)
        {
            return { SfzImportSupportDisposition::converted,
                     "zone.performance.pitchSource",
                     "Zero pitch tracking keeps event-note eligibility while rendering the sample at its fixed root pitch." };
        }
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.pitch.keyTracking",
                 "Nonzero partial pitch tracking remains review-only; native playback currently supports full tracking or fixed pitch.",
                 "sfz.pitch.keytrack.reported",
                 "Partial pitch tracking will be reported",
                 "The importer supports pitch_keytrack values of 0 or 100, but does not approximate intermediate tracking ratios." };
    }

    if (opcodeName == "rt_decay")
    {
        return { SfzImportSupportDisposition::approximated,
                 "zone.performance.release",
                 "Release eligibility and velocity are native, while duration-dependent rt_decay attenuation is not yet applied.",
                 "sfz.release.rt_decay.approximated",
                 "Release-trigger decay will be approximated",
                 "The release sample is triggered under the correct controller and pedal state, but its gain does not yet vary with held-note duration." };
    }

    if (opcodeName == "lovel")
    {
        if (parseIntValue(opcode.value).value_or(1) == 0)
        {
            return { SfzImportSupportDisposition::approximated,
                     "zone.velocityRange.lowVelocity",
                     "A zero lower bound is normalized to MIDI's lowest playable note-on velocity, 1.",
                     "sfz.velocity.lovel_zero.normalized",
                     "Zero lower velocity will be normalized",
                     "SFZ lovel=0 is treated as the bottom playable layer and converted to native velocityLow=1 because MIDI note-on velocity zero represents note-off." };
        }
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

    if (const auto controllerNumber = parseControllerNumber(opcodeName, "set_cc"))
    {
        return { SfzImportSupportDisposition::converted,
                 "authoring.controllerDefaults[" + std::to_string(*controllerNumber) + "]",
                 "Controller defaults map directly into native authoring and playback controller state." };
    }

    const auto controllerConditionTarget = [&](const std::string& prefix,
                                               const std::string& target)
        -> std::optional<OpcodeClassification>
    {
        if (const auto controllerNumber = parseControllerNumber(opcodeName, prefix))
        {
            return OpcodeClassification {
                SfzImportSupportDisposition::converted,
                target + "[" + std::to_string(*controllerNumber) + "]",
                "Controller eligibility ranges map into native per-zone controller conditions." };
        }
        return std::nullopt;
    };
    if (const auto classification = controllerConditionTarget("on_locc", "zone.performance.controllerTriggerRange"))
        return *classification;
    if (const auto classification = controllerConditionTarget("on_hicc", "zone.performance.controllerTriggerRange"))
        return *classification;
    if (const auto classification = controllerConditionTarget("locc", "zone.controllerConditions"))
        return *classification;
    if (const auto classification = controllerConditionTarget("hicc", "zone.controllerConditions"))
        return *classification;

    if (opcodeName == "trigger")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.performance.event",
                 "Attack and release triggers map into explicit native performance events." };
    }

    if (opcodeName == "group_volume")
    {
        return { SfzImportSupportDisposition::converted,
                 "authoring.groups.gainDb",
                 "Group volume inherits through SFZ scope and maps into native group gain." };
    }

    if (opcodeName == "tune")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.fineTuneCents",
                 "Fine tuning in cents maps directly into the native route pitch ratio." };
    }

    if (opcodeName == "amp_veltrack")
    {
        auto value = 100.0;
        try { value = std::stod(opcode.value); } catch (...) {}
        if (value >= 0.0 && value <= 100.0)
        {
            return { SfzImportSupportDisposition::converted,
                     "zone.amplitudeVelocityTracking",
                     "Velocity tracking uses the documented native power law gain=(velocity/127)^(amp_veltrack/100)." };
        }
        return { SfzImportSupportDisposition::approximated,
                 "zone.amplitudeVelocityTracking",
                 "Velocity tracking values outside 0..100 are clamped to the native power-law range.",
                 "sfz.amp_veltrack.clamped",
                 "Amplitude velocity tracking will be clamped",
                 "The native velocity law supports 0..100 percent and clamps values outside that range." };
    }

    if (opcodeName == "volume")
    {
        switch (opcode.location.scope)
        {
            case SfzOpcodeScope::master:
                return { SfzImportSupportDisposition::converted,
                         "authoring.masterGainDb",
                         "Master-scope gain now maps into the authored project master gain control." };
            case SfzOpcodeScope::group:
                return { SfzImportSupportDisposition::converted,
                         "authoring.groups.gainDb",
                         "Group-scope gain now maps into authored group gain without being flattened into zones." };
            case SfzOpcodeScope::region:
                return { SfzImportSupportDisposition::converted,
                         "authoring.zones.gainDb",
                         "Region-local gain now maps into authored zone gain without carrying inherited master or group gain." };
            case SfzOpcodeScope::global:
                return { SfzImportSupportDisposition::approximated,
                         "report.gain.globalScope",
                         "Global gain does not yet have a first-class authored scope, so it remains a review-time approximation.",
                         "sfz.gain.global_volume.approximated",
                         "Global SFZ volume will require review",
                         "The importer preserves master, group, and region-local volume scopes directly, but <global> volume does not yet map one-to-one into the authored gain model." };
            default:
                return { SfzImportSupportDisposition::reportedOnly,
                         "report.gain.unsupportedScope",
                         "This gain scope remains review-only until the importer can map it into authored gain metadata.",
                         "sfz.gain.unsupported_scope.reported",
                         "Unsupported SFZ volume scope will be reported",
                         "The importer recognizes this SFZ volume opcode, but its current scope does not yet map directly into the authored gain model." };
        }
    }

    if (opcodeName == "ampeg_release")
    {
        return { SfzImportSupportDisposition::converted,
                 "ampEnvelope.releaseSeconds",
                 "Per-zone and inherited release times can map into native envelope release controls." };
    }

    if (opcodeName == "sustain_cc")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.damper.sustainControllerNumber",
                 "The authored binary sustain controller maps directly into native damper metadata." };
    }

    if (opcodeName == "sustain_lo")
    {
        return { SfzImportSupportDisposition::converted,
                 "zone.damper.sustainThreshold",
                 "The authored sustain lower threshold maps directly into native damper metadata; an absent value uses the ARIA-compatible 0.5 import default." };
    }

    if (opcodeName == "ampeg_dynamic")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.damper.dynamicRelease",
                 "ampeg_dynamic remains review-only unless it participates in the complete supported CC64 half-pedal release block.",
                 "sfz.damper.dynamic_flag.reported",
                 "General ampeg_dynamic use will be reported",
                 "The focused importer converts ampeg_dynamic=1 only when matching release amount and curve-reference declarations are inherited by a playable region." };
    }

    if (opcodeName == "ampeg_releasecc64")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.damper.releaseAmountSeconds",
                 "The release amount remains review-only unless it participates in the complete supported CC64 half-pedal release block." };
    }

    if (opcodeName == "ampeg_release_curvecc64")
    {
        return { SfzImportSupportDisposition::reportedOnly,
                 "report.damper.releaseCurveIndex",
                 "The release curve reference remains review-only unless it participates in the complete supported CC64 half-pedal release block." };
    }

    if (opcodeName == "ampeg_release_shape")
    {
        return { SfzImportSupportDisposition::converted,
                 "ampEnvelope.releaseShape",
                 "Per-zone and inherited release shapes map directly into the native amplitude envelope." };
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
    return analyzeSfzImportDocument(sfzPath, defaultSfzImportExecutionContext());
}

SfzImportAnalysisResult analyzeSfzImportDocument(const std::string& sfzPath,
                                                const SfzImportExecutionContext& context)
{
    SfzImportAnalysisResult result;
    context.resetProgress();
    context.reportProgress(SfzImportStage::discovering, 0.0f);
    result.analyzed = true;
    result.parseResult = parseSfzDocument(sfzPath, context);
    result.execution = result.parseResult.execution;

    if (result.execution.canceled())
    {
        result.analyzed = false;
        result.report.stage = SfzImportStage::canceled;
        result.report.state = "Canceled";
        result.report.available = false;
        result.report.execution = result.execution;
        result.report.rootDocumentPath = result.parseResult.document.rootDocumentPath;
        result.report.sourceFiles = result.parseResult.document.sourceFiles;
        context.reportProgress(SfzImportStage::canceled, 0.30f);
        return result;
    }

    result.report.available = true;
    result.report.rootDocumentPath = result.parseResult.document.rootDocumentPath;
    result.report.sourceFiles = result.parseResult.document.sourceFiles;
    result.report.summary.sourceFileCount = result.report.sourceFiles.size();
    result.report.summary.sectionCount = result.parseResult.document.sections.size();
    result.report.summary.opcodeCount = countParsedOpcodes(result.parseResult.document);
    result.report.findings = result.parseResult.findings;
    result.report.summary.suppressedFindingCount = result.parseResult.suppressedFindingCount;

    if (result.parseResult.parsed)
    {
        result.normalizeResult = normalizeSfzDocument(result.parseResult.document, context);
        result.execution = result.normalizeResult.execution;
        appendFindingsBounded(result.report.findings,
                              result.normalizeResult.findings,
                              context.budgets.maximumFindingCount,
                              result.report.summary.suppressedFindingCount);

        if (result.execution.canceled())
        {
            result.analyzed = false;
            result.report.stage = SfzImportStage::canceled;
            result.report.state = "Canceled";
            result.report.available = false;
            result.report.execution = result.execution;
            context.reportProgress(SfzImportStage::canceled, 0.50f);
            return result;
        }

        if (result.normalizeResult.normalized)
        {
            context.reportProgress(SfzImportStage::validating, 0.55f);
            result.report.rootDocumentPath = result.normalizeResult.document.rootDocumentPath;
            result.report.sourceFiles = result.normalizeResult.document.sourceFiles;
            result.report.summary.sourceFileCount = result.report.sourceFiles.size();
            result.report.summary.sectionCount = result.normalizeResult.document.sections.size();
            result.report.summary.opcodeCount = 0;
            context.reportProgress(SfzImportStage::classifying, 0.65f);
            const auto crossfadeClassifications =
                buildVelocityCrossfadeOpcodeClassifications(result.normalizeResult.document);
            const auto roundRobinClassifications =
                buildSequentialRoundRobinOpcodeClassifications(result.normalizeResult.document);
            const auto continuousDamperCurveClassifications =
                buildContinuousDamperCurveClassifications(result.normalizeResult.document);
            const auto continuousDamperOpcodeClassifications =
                buildContinuousDamperOpcodeClassifications(result.normalizeResult.document);

            std::map<SupportKey, SfzImportOpcodeSupportSummary> supportSummaries;

            for (const auto& section : result.normalizeResult.document.sections)
            {
                const auto cancellationReason = context.pollCancellation();
                if (cancellationReason != SfzImportCancellationReason::none)
                {
                    result.analyzed = false;
                    result.execution.disposition = SfzImportExecutionDisposition::canceled;
                    result.execution.cancellationReason = cancellationReason;
                    result.report.stage = SfzImportStage::canceled;
                    result.report.state = "Canceled";
                    result.report.available = false;
                    result.report.execution = result.execution;
                    context.reportProgress(SfzImportStage::canceled, 0.80f);
                    return result;
                }

                const auto sampleReference = findEffectiveSampleReference(section);
                result.report.summary.opcodeCount += section.localOpcodes.size();

                for (const auto& opcode : section.localOpcodes)
                {
                    const auto opcodeCancellationReason = context.pollCancellation();
                    if (opcodeCancellationReason != SfzImportCancellationReason::none)
                    {
                        result.analyzed = false;
                        result.execution.disposition = SfzImportExecutionDisposition::canceled;
                        result.execution.cancellationReason = opcodeCancellationReason;
                        result.report.stage = SfzImportStage::canceled;
                        result.report.state = "Canceled";
                        result.report.available = false;
                        result.report.execution = result.execution;
                        context.reportProgress(SfzImportStage::canceled, 0.80f);
                        return result;
                    }

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
                    if (const auto classificationIterator =
                            continuousDamperCurveClassifications.find(makeCrossfadeOpcodeKey(opcode));
                        classificationIterator != continuousDamperCurveClassifications.end())
                    {
                        classification = classificationIterator->second;
                    }
                    if (const auto classificationIterator =
                            continuousDamperOpcodeClassifications.find(makeCrossfadeOpcodeKey(opcode));
                        classificationIterator != continuousDamperOpcodeClassifications.end())
                    {
                        classification = classificationIterator->second;
                    }
                    incrementDispositionCount(result.report.summary, classification.disposition);
                    addClassificationFinding(result.report.findings,
                                             classification,
                                             opcode,
                                             sampleReference,
                                             context.budgets.maximumFindingCount,
                                             result.report.summary.suppressedFindingCount);
                    updateSupportSummary(supportSummaries, opcode, classification);

                    SfzImportTraceEntry trace {
                        section.documentOrder,
                        section.scope,
                        section.headerName,
                        opcode.name,
                        opcode.value,
                        classification.nativeTarget,
                        sampleReference,
                        classification.disposition,
                        classification.findingCode,
                        opcode.location
                    };
                    if (const auto semantic = classifySemanticDependency(opcode))
                    {
                        trace.semanticDependencyKind = semantic->kind;
                        trace.semanticImpact = semantic->impact;
                        trace.semanticSupport = ((toLowerAscii(opcode.name) == "ampeg_releasecc64"
                                                  || toLowerAscii(opcode.name) == "ampeg_release_curvecc64")
                                                 && classification.disposition
                                                     == SfzImportSupportDisposition::converted)
                            ? SfzImportSemanticSupport::native
                            : semantic->support;
                        trace.affectsRegionEligibility = semantic->affectsRegionEligibility;
                        if (semantic->impact == SfzImportSemanticImpact::presentationOnly)
                        {
                            ++result.report.summary.semanticDependencyCount;
                            ++result.report.summary.presentationOnlyDependencyCount;
                        }
                    }
                    result.report.traceEntries.push_back(std::move(trace));
                }
            }

            result.report.opcodeSupport.reserve(supportSummaries.size());
            for (const auto& [_, summary] : supportSummaries)
                result.report.opcodeSupport.push_back(summary);

            publishRegionSemanticAnalysis(result.report,
                                          result.normalizeResult.document);
        }
    }

    const auto finalCancellationReason = context.pollCancellation();
    if (finalCancellationReason != SfzImportCancellationReason::none)
    {
        result.analyzed = false;
        result.execution.disposition = SfzImportExecutionDisposition::canceled;
        result.execution.cancellationReason = finalCancellationReason;
        result.report.stage = SfzImportStage::canceled;
        result.report.state = "Canceled";
        result.report.available = false;
        result.report.execution = result.execution;
        context.reportProgress(SfzImportStage::canceled, 0.95f);
        return result;
    }

    result.report.reviewDisposition = sfzImportReviewDispositionFor(result.report.findings);
    if (!result.parseResult.parsed || result.report.summary.blockingOpcodeCount > 0)
    {
        result.report.reviewDisposition = SfzImportReviewDisposition::blocked;
    }
    else if (result.report.summary.approximatedOpcodeCount > 0
             || result.report.summary.reportedOnlyOpcodeCount > 0)
    {
        result.report.reviewDisposition = SfzImportReviewDisposition::confirmationRequired;
    }
    result.report.blocking = result.report.reviewDisposition == SfzImportReviewDisposition::blocked;
    result.report.stage = result.report.blocking ? SfzImportStage::blocked
                                                 : SfzImportStage::reviewReady;
    result.report.state = result.report.blocking ? "Blocked" : "Review Ready";
    if (!result.parseResult.parsed)
    {
        result.execution.disposition = SfzImportExecutionDisposition::failed;
        result.execution.failureReason = result.parseResult.execution.failureReason
            == SfzImportFailureReason::none
            ? SfzImportFailureReason::malformedInput
            : result.parseResult.execution.failureReason;
    }
    else
    {
        // A blocking compatibility finding is a completed analysis with an
        // unsupported input disposition; it is not an engine exception.
        result.execution.disposition = SfzImportExecutionDisposition::completed;
        result.execution.failureReason = result.report.blocking
            ? SfzImportFailureReason::unsupportedInput
            : SfzImportFailureReason::none;
    }

    accumulateFindingSeverities(result.report.summary, result.report.findings);
    result.report.execution = result.execution;
    context.reportProgress(SfzImportStage::classifying, 0.80f);
    context.reportProgress(result.report.stage, result.report.blocking ? 0.80f : 1.0f);
    return result;
}
} // namespace drs::engine
