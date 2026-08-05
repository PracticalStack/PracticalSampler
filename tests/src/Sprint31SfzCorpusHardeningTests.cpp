#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/VelocityCrossfade.h"
#include "shared/ProjectStorage.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

fs::path resolveFixturePath(const fs::path& relativeFixturePath)
{
    const auto sourceRoot = fs::path(DRS_SOURCE_ROOT);

    const auto localFixturePath = sourceRoot / relativeFixturePath;
    if (fs::exists(localFixturePath))
        return localFixturePath;

    const auto workspaceFixturePath = sourceRoot.parent_path() / relativeFixturePath;
    if (fs::exists(workspaceFixturePath))
        return workspaceFixturePath;

    throw std::runtime_error("Could not locate " + relativeFixturePath.generic_string());
}

drs::engine::RuntimeProjectModel makeBlankPhase2Project(const fs::path& fixturePath,
                                                        const std::string& projectId)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = projectId;
    project.displayName = "Sprint 3.1.6 Corpus Hardening";
    project.contentRootPath = fixturePath.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixturePath.parent_path() / (projectId + ".drstrm")).generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    return project;
}

std::size_t countFindingsWithCode(const drs::engine::SfzImportAnalysisResult& analysis,
                                  const std::string& code)
{
    return static_cast<std::size_t>(
        std::count_if(analysis.report.findings.begin(),
                      analysis.report.findings.end(),
                      [&](const drs::engine::SfzImportFinding& finding)
                      {
                          return finding.code == code;
                      }));
}

template <typename TZone>
std::size_t countCrossfadeZones(const std::vector<TZone>& zones)
{
    return static_cast<std::size_t>(
        std::count_if(zones.begin(),
                      zones.end(),
                      [](const TZone& zone)
                      {
                          return drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade);
                      }));
}

template <typename TZone>
double computeZoneGain(const TZone& zone, int velocity)
{
    if (!drs::engine::hasAnyVelocityCrossfadeValue(zone.velocityCrossfade))
        return velocity >= zone.velocityLow && velocity <= zone.velocityHigh ? 1.0 : 0.0;

    return drs::engine::computeFirstPassVelocityCrossfadeGain(
        { zone.velocityLow, zone.velocityHigh, zone.velocityCrossfade },
        velocity);
}

struct OverlapSummary
{
    std::size_t positiveParticipantCount = 0;
    double totalGain = 0.0;
};

template <typename TZone>
OverlapSummary summarizeOverlap(const std::vector<TZone>& zones,
                                int midiNote,
                                int velocity,
                                int roundRobinPosition)
{
    OverlapSummary summary;
    for (const auto& zone : zones)
    {
        if (midiNote < zone.keyLow || midiNote > zone.keyHigh)
            continue;

        if (zone.roundRobinLength > 0
            && zone.roundRobinPosition > 0
            && zone.roundRobinPosition != roundRobinPosition)
        {
            continue;
        }

        const auto gain = computeZoneGain(zone, velocity);
        if (gain <= 0.0)
            continue;

        ++summary.positiveParticipantCount;
        summary.totalGain += gain;
    }

    return summary;
}

struct FixtureExpectation
{
    const char* label = "";
    const char* relativePath = "";
    std::size_t expectedConvertedCount = 0;
    std::size_t expectedApproximateCount = 0;
    std::size_t expectedWarningCount = 0;
    bool expectsCrossfadeOverlap = false;
    std::string expectedSampleFragment;
};
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const std::vector<FixtureExpectation> fixtures {
            { "mono-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono/_jRhodes3d-mono-flac.sfz",
              1599,
              0,
              9,
              true,
              "jRhodes3d-mono" },
            { "mono-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-mono-no-xfade.sfz",
              1592,
              0,
              9,
              false,
              "jRhodes3d-mono" },
            { "stereo-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st.sfz",
              1600,
              0,
              9,
              true,
              "jRhodes3d-st" },
            { "stereo-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-st-no-xfade.sfz",
              1592,
              0,
              9,
              false,
              "jRhodes3d-st" },
            { "stereo-vibrato-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv.sfz",
              1600,
              0,
              9,
              true,
              "jRhodes3d-sv" },
            { "stereo-vibrato-no-xfade",
              "DemoSFVInstruments/jlearman.jRhodes3d-master-rr/jRhodes3d-sv-no-xfade.sfz",
              1592,
              0,
              9,
              false,
              "jRhodes3d-sv" }
        };

        for (const auto& fixture : fixtures)
        {
            const auto fixturePath = resolveFixturePath(fixture.relativePath);
            const auto analysis = analyzeSfzImportDocument(fixturePath.generic_string());

            require(analysis.analyzed,
                    std::string("Fixture should analyze successfully: ") + fixture.label);
            require(analysis.parseResult.parsed && analysis.parseResult.complete,
                    std::string("Fixture should parse completely: ") + fixture.label);
            require(analysis.normalizeResult.normalized,
                    std::string("Fixture should normalize completely: ") + fixture.label);
            require(analysis.report.available,
                    std::string("Fixture should publish a report: ") + fixture.label);
            require(!analysis.report.blocking,
                    std::string("Fixture should not be blocked: ") + fixture.label);
            require(analysis.report.stage == SfzImportStage::reviewReady,
                    std::string("Fixture should stay review-ready: ") + fixture.label);
            require(analysis.report.reviewDisposition == SfzImportReviewDisposition::confirmationRequired,
                    std::string("Fixture should preserve the review confirmation gate: ") + fixture.label);

            require(analysis.report.summary.sourceFileCount == 1,
                    std::string("Fixture should still analyze as one source file: ") + fixture.label);
            require(analysis.report.summary.sectionCount == 233,
                    std::string("Fixture section count changed unexpectedly: ") + fixture.label);
            require(analysis.report.summary.opcodeCount
                        == analysis.report.summary.convertedOpcodeCount
                            + analysis.report.summary.approximatedOpcodeCount
                            + analysis.report.summary.reportedOnlyOpcodeCount
                            + analysis.report.summary.blockingOpcodeCount,
                    std::string("Fixture opcode summary should still balance exactly: ") + fixture.label);
            require(analysis.report.summary.convertedOpcodeCount == fixture.expectedConvertedCount,
                    std::string("Fixture converted-opcode count changed unexpectedly: ") + fixture.label
                        + " expected=" + std::to_string(fixture.expectedConvertedCount)
                        + " actual=" + std::to_string(analysis.report.summary.convertedOpcodeCount));
            require(analysis.report.summary.approximatedOpcodeCount == fixture.expectedApproximateCount,
                    std::string("Fixture approximated-opcode count changed unexpectedly: ") + fixture.label
                        + " expected=" + std::to_string(fixture.expectedApproximateCount)
                        + " actual=" + std::to_string(analysis.report.summary.approximatedOpcodeCount));
            require(analysis.report.summary.reportedOnlyOpcodeCount == 9,
                    std::string("Fixture reported-only opcode count changed unexpectedly: ") + fixture.label
                        + " actual=" + std::to_string(analysis.report.summary.reportedOnlyOpcodeCount));
            require(analysis.report.summary.blockingOpcodeCount == 0,
                    std::string("Fixture should not contribute blocking opcode counts: ") + fixture.label
                        + " actual=" + std::to_string(analysis.report.summary.blockingOpcodeCount));
            require(analysis.report.summary.warningFindingCount == fixture.expectedWarningCount
                        && analysis.report.summary.errorFindingCount == 0,
                    std::string("Fixture warning/error counts changed unexpectedly: ") + fixture.label
                        + " warnings=" + std::to_string(analysis.report.summary.warningFindingCount)
                        + " errors=" + std::to_string(analysis.report.summary.errorFindingCount));
            require(analysis.report.findings.size() == fixture.expectedWarningCount,
                    std::string("Fixture finding count changed unexpectedly: ") + fixture.label
                        + " actual=" + std::to_string(analysis.report.findings.size()));

            const auto crossfadeFindingCount =
                countFindingsWithCode(analysis, "sfz.velocity_crossfade.approximated");
            require(crossfadeFindingCount == fixture.expectedApproximateCount,
                    std::string("Fixture crossfade finding count changed unexpectedly: ") + fixture.label);

            const auto curveFindingCount = countFindingsWithCode(analysis, "sfz.curve.reported");
            require(curveFindingCount == 5,
                    std::string("Fixture should still surface every curve opcode: ") + fixture.label);
            require(countFindingsWithCode(analysis, "sfz.cc.label.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.default.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.width.reported") == 1
                        && countFindingsWithCode(analysis, "sfz.cc.width_curve.reported") == 1,
                    std::string("Fixture control-surface reporting changed unexpectedly: ") + fixture.label);

            const auto project = makeBlankPhase2Project(fixturePath, std::string("sprint31.") + fixture.label);
            const auto projection = projectSfzImportAnalysis(project, analysis);
            require(projection.projected,
                    std::string("Fixture should project into native content: ") + fixture.label);
            require(projection.playable,
                    std::string("Fixture projection should remain playable: ") + fixture.label);
            require(projection.lossy,
                    std::string("Fixture projection should preserve the review gate as lossy content: ") + fixture.label);
            require(!projection.blocking,
                    std::string("Fixture projection should not be blocked: ") + fixture.label);
            require(projection.sampleSources.size() == 195,
                    std::string("Fixture projected sample-source count changed unexpectedly: ") + fixture.label);
            require(projection.zones.size() == 225,
                    std::string("Fixture projected zone count changed unexpectedly: ") + fixture.label);
            require(!projection.projectNotes.empty() && !projection.authoringNotes.empty(),
                    std::string("Fixture projection should preserve import notes: ") + fixture.label);
            require(std::abs(projection.masterGainDb - 6.0) < 0.0001,
                    std::string("Fixture projection should preserve the shared Rhodes master gain at project scope: ")
                        + fixture.label);
            require(std::all_of(projection.groups.begin(),
                                projection.groups.end(),
                                [](const RuntimeProjectGroupDefinition& group)
                                {
                                    return std::abs(group.gainDb) < 0.0001;
                                }),
                    std::string("Fixture projection should keep zero-dB group gain when the source uses only master gain: ")
                        + fixture.label);
            require(std::all_of(projection.zones.begin(),
                                projection.zones.end(),
                                [](const RuntimeProjectZoneDefinition& zone)
                                {
                                    return std::abs(zone.gainDb) < 0.0001;
                                }),
                    std::string("Fixture projection should no longer flatten shared master gain into zone gain: ")
                        + fixture.label);
            require(std::all_of(projection.sampleSources.begin(),
                                projection.sampleSources.end(),
                                [&](const RuntimeProjectSampleSource& sampleSource)
                                {
                                    return sampleSource.path.find(fixture.expectedSampleFragment) != std::string::npos;
                                }),
                    std::string("Fixture projected sample paths should stay rooted in the expected corpus folder: ")
                        + fixture.label);

            const auto projectedCrossfadeZoneCount = countCrossfadeZones(projection.zones);
            if (fixture.expectsCrossfadeOverlap)
            {
                require(projectedCrossfadeZoneCount > 0,
                        std::string("Crossfade fixtures should still project native crossfade metadata: ")
                            + fixture.label);
            }
            else
            {
                require(projectedCrossfadeZoneCount == 0,
                        std::string("No-crossfade fixtures should stay free of projected crossfade metadata: ")
                            + fixture.label);
            }

            const auto projectedOverlap = summarizeOverlap(projection.zones, 29, 32, 1);
            require(projectedOverlap.positiveParticipantCount
                        == (fixture.expectsCrossfadeOverlap ? std::size_t { 2 } : std::size_t { 1 }),
                    std::string("Projected overlap participant count changed unexpectedly: ") + fixture.label);
            require(std::abs(projectedOverlap.totalGain - 1.0) < 0.0001,
                    std::string("Projected overlap gains should stay normalized: ") + fixture.label);

            AuthoringSession session(project);
            const auto applyResult = applySfzImportProjection(
                session,
                projection,
                std::string("Sprint 3.1.6 import ") + fixture.label);
            require(applyResult.applied,
                    std::string("Corpus fixtures should stay applyable after review: ") + fixture.label);
            require(std::abs(session.getProject().authoring.masterGainDb - projection.masterGainDb) < 0.0001,
                    std::string("Corpus fixture apply should preserve projected master gain: ") + fixture.label);

            const auto tempDirectory =
                fs::temp_directory_path() / "drs-sprint31-sfz-corpus-hardening" / fixture.label;
            const auto projectPath = tempDirectory / "roundtrip.drsproj";
            const auto instrumentPath = tempDirectory / "roundtrip.drinst";
            const auto streamPath = tempDirectory / "roundtrip.drstrm";

            auto savedProject = session.getProject();
            savedProject.defaultInstrumentManifestPath = instrumentPath.generic_string();
            const auto instrument = drs::app::buildInstrumentManifestForProject(
                savedProject,
                juce::File(projectPath.generic_string()));
            require(instrument.zones.size() == projection.zones.size(),
                    std::string("Publish conversion should preserve projected zone counts: ") + fixture.label);
            require(countCrossfadeZones(instrument.zones)
                        == (fixture.expectsCrossfadeOverlap ? projectedCrossfadeZoneCount : std::size_t { 0 }),
                    std::string("Publish conversion crossfade metadata changed unexpectedly: ") + fixture.label);

            const auto instrumentOverlap = summarizeOverlap(instrument.zones, 29, 32, 1);
            require(instrumentOverlap.positiveParticipantCount == projectedOverlap.positiveParticipantCount,
                    std::string("Preview/publish overlap participant parity changed unexpectedly: ")
                        + fixture.label);
            require(std::abs(instrumentOverlap.totalGain - projectedOverlap.totalGain) < 0.0001,
                    std::string("Preview/publish overlap gain parity changed unexpectedly: ")
                        + fixture.label);

            writeTextFile(streamPath, "sprint31 corpus hardening stream placeholder");
            writeTextFile(projectPath,
                          serializeRuntimeProjectManifest(savedProject, projectPath.generic_string()));
            writeTextFile(instrumentPath,
                          serializeRuntimeInstrumentManifest(instrument, instrumentPath.generic_string()));

            const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
            require(roundTripProject.loaded,
                    std::string("Corpus project round-trip should stay valid: ") + fixture.label);
            require(countCrossfadeZones(roundTripProject.project.authoring.zones)
                        == (fixture.expectsCrossfadeOverlap ? projectedCrossfadeZoneCount : std::size_t { 0 }),
                    std::string("Project round-trip crossfade metadata changed unexpectedly: ") + fixture.label);
            const auto roundTripProjectOverlap =
                summarizeOverlap(roundTripProject.project.authoring.zones, 29, 32, 1);
            require(roundTripProjectOverlap.positiveParticipantCount == projectedOverlap.positiveParticipantCount,
                    std::string("Project round-trip overlap participant parity changed unexpectedly: ")
                        + fixture.label);
            require(std::abs(roundTripProjectOverlap.totalGain - projectedOverlap.totalGain) < 0.0001,
                    std::string("Project round-trip overlap gain parity changed unexpectedly: ")
                        + fixture.label);

            const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
            require(roundTripInstrument.loaded,
                    std::string("Corpus instrument round-trip should stay valid: ") + fixture.label);
            require(countCrossfadeZones(roundTripInstrument.instrument.zones)
                        == (fixture.expectsCrossfadeOverlap ? projectedCrossfadeZoneCount : std::size_t { 0 }),
                    std::string("Instrument round-trip crossfade metadata changed unexpectedly: ")
                        + fixture.label);
            const auto roundTripInstrumentOverlap =
                summarizeOverlap(roundTripInstrument.instrument.zones, 29, 32, 1);
            require(roundTripInstrumentOverlap.positiveParticipantCount
                        == projectedOverlap.positiveParticipantCount,
                    std::string("Instrument round-trip overlap participant parity changed unexpectedly: ")
                        + fixture.label);
            require(std::abs(roundTripInstrumentOverlap.totalGain - projectedOverlap.totalGain) < 0.0001,
                    std::string("Instrument round-trip overlap gain parity changed unexpectedly: ")
                        + fixture.label);
        }

        std::cout << "Sprint 3.1.6 SFZ corpus hardening tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Sprint 3.1.6 SFZ corpus hardening tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
