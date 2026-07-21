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

namespace drs::engine
{
namespace
{
namespace fs = std::filesystem;

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

std::string buildGroupId(const SfzNormalizedSection& section,
                         const std::string& articulationId,
                         int keyLow,
                         int keyHigh,
                         int velocityLow,
                         int velocityHigh)
{
    std::ostringstream stream;
    stream << "sfz-" << slugify(articulationId)
           << "-k" << keyLow << "-" << keyHigh
           << "-v" << velocityLow << "-" << velocityHigh;

    if (const auto* roundRobinLength = findEffectiveOpcode(section, "seq_length"))
        stream << "-rr" << roundRobinLength->value;

    return slugify(stream.str());
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

    return notes;
}

RuntimeProjectModel buildProvisionalProject(const RuntimeProjectModel& baseProject,
                                            const SfzImportProjectionResult& projection)
{
    auto project = baseProject;
    project.sampleSources.insert(project.sampleSources.end(),
                                 projection.sampleSources.begin(),
                                 projection.sampleSources.end());
    project.authoring.zones.insert(project.authoring.zones.end(),
                                   projection.zones.begin(),
                                   projection.zones.end());
    project.notes.insert(project.notes.end(),
                         projection.projectNotes.begin(),
                         projection.projectNotes.end());
    project.authoring.notes.insert(project.authoring.notes.end(),
                                   projection.authoringNotes.begin(),
                                   projection.authoringNotes.end());
    if (!projection.zones.empty())
        project.authoring.selectedZoneId = projection.zones.front().id;
    return project;
}
} // namespace

SfzImportProjectionResult projectSfzImportAnalysis(const RuntimeProjectModel& baseProject,
                                                   const SfzImportAnalysisResult& analysis)
{
    SfzImportProjectionResult result;
    result.blocking = !analysis.analyzed
        || !analysis.report.available
        || analysis.report.blocking
        || analysis.report.reviewDisposition == SfzImportReviewDisposition::blocked;
    result.lossy = analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired;

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

    std::set<std::string> usedSampleSourceIds;
    for (const auto& sampleSource : baseProject.sampleSources)
        usedSampleSourceIds.insert(sampleSource.id);

    std::set<std::string> usedZoneIds;
    for (const auto& zone : baseProject.authoring.zones)
        usedZoneIds.insert(zone.id);

    std::map<std::string, std::string> sampleSourceIdsByPath;
    for (const auto& sampleSource : baseProject.sampleSources)
        sampleSourceIdsByPath.emplace(fs::path(sampleSource.path).lexically_normal().generic_string(),
                                      sampleSource.id);

    result.projectNotes = buildProjectNotes(analysis.report);
    result.authoringNotes = buildAuthoringNotes(analysis.report);

    for (const auto& section : analysis.normalizeResult.document.sections)
    {
        if (section.scope != SfzOpcodeScope::region)
            continue;

        const auto* sampleOpcode = findEffectiveOpcode(section, "sample");
        if (sampleOpcode == nullptr || sampleOpcode->value.empty())
        {
            result.issues.push_back("Region at document order " + std::to_string(section.documentOrder)
                                    + " did not resolve a sample opcode.");
            continue;
        }

        const auto samplePath = (fs::path(sampleOpcode->location.sourcePath).parent_path() / fs::path(sampleOpcode->value))
            .lexically_normal();
        const auto canonicalSamplePath = samplePath.generic_string();

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
        zone.rootKey = parseMidiNoteValue(findEffectiveOpcode(section, "pitch_keycenter") != nullptr
                                              ? findEffectiveOpcode(section, "pitch_keycenter")->value
                                              : "60")
                           .value_or(60);
        zone.keyLow = parseMidiNoteValue(findEffectiveOpcode(section, "lokey") != nullptr
                                             ? findEffectiveOpcode(section, "lokey")->value
                                             : std::to_string(zone.rootKey))
                          .value_or(zone.rootKey);
        zone.keyHigh = parseMidiNoteValue(findEffectiveOpcode(section, "hikey") != nullptr
                                              ? findEffectiveOpcode(section, "hikey")->value
                                              : std::to_string(zone.rootKey))
                           .value_or(zone.rootKey);
        zone.velocityLow = parseIntValue(findEffectiveOpcode(section, "lovel") != nullptr
                                             ? findEffectiveOpcode(section, "lovel")->value
                                             : "1")
                               .value_or(1);
        zone.velocityHigh = parseIntValue(findEffectiveOpcode(section, "hivel") != nullptr
                                              ? findEffectiveOpcode(section, "hivel")->value
                                              : "127")
                                .value_or(127);
        zone.gainDb = parseDoubleValue(findEffectiveOpcode(section, "volume") != nullptr
                                           ? findEffectiveOpcode(section, "volume")->value
                                           : "0")
                          .value_or(0.0);
        zone.releaseSeconds = parseDoubleValue(findEffectiveOpcode(section, "ampeg_release") != nullptr
                                                   ? findEffectiveOpcode(section, "ampeg_release")->value
                                                   : "0")
                                  .value_or(0.0);
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
        zone.roundRobinLength = parseIntValue(findEffectiveOpcode(section, "seq_length") != nullptr
                                                  ? findEffectiveOpcode(section, "seq_length")->value
                                                  : "0")
                                    .value_or(0);
        zone.roundRobinPosition = parseIntValue(findEffectiveOpcode(section, "seq_position") != nullptr
                                                    ? findEffectiveOpcode(section, "seq_position")->value
                                                    : "0")
                                      .value_or(0);
        zone.articulationId = buildArticulationId(section);
        zone.groupId = buildGroupId(section,
                                    zone.articulationId,
                                    zone.keyLow,
                                    zone.keyHigh,
                                    zone.velocityLow,
                                    zone.velocityHigh);

        if (const auto* trigger = findEffectiveOpcode(section, "trigger"))
        {
            const auto lowered = toLowerAscii(trigger->value);
            if (lowered == "release")
                zone.triggerMode = ZoneTriggerMode::oneShot;
        }

        result.zones.push_back(std::move(zone));
    }

    if (result.zones.empty())
    {
        result.state = "SFZ projection failed";
        if (result.zones.empty())
            result.issues.push_back("Projection did not create any zones.");
        return result;
    }

    const auto provisionalProject = buildProvisionalProject(baseProject, result);
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

    result.projected = true;
    result.state = result.lossy ? "SFZ projection ready for reviewed apply" : "SFZ projection ready";
    return result;
}

SfzImportProjectionResult projectSfzImportDocument(const RuntimeProjectModel& baseProject,
                                                   const std::string& sfzPath)
{
    return projectSfzImportAnalysis(baseProject, analyzeSfzImportDocument(sfzPath));
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
                                                  std::move(projection.projectNotes),
                                                  std::move(projection.authoringNotes),
                                                  label);
}
} // namespace drs::engine
