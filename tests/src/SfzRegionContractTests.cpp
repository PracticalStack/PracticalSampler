#include "drs/engine/SfzImportReport.h"
#include "drs/engine/SfzImportProjection.h"
#include "drs/engine/SfzRegionContract.h"
#include "drs/engine/PlaybackRegionContract.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
namespace fs = std::filesystem;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

fs::path fixturePath(const std::string& name)
{
    return fs::path(DRS_SOURCE_ROOT) / "tests" / "fixtures" / "sfz-region-contract" / name;
}

const drs::engine::SfzNormalizedSection& regionAt(
    const drs::engine::SfzNormalizedDocument& document,
    const std::size_t regionIndex)
{
    std::size_t index = 0;
    for (const auto& section : document.sections)
    {
        if (section.scope != drs::engine::SfzOpcodeScope::region)
            continue;
        if (index++ == regionIndex)
            return section;
    }
    throw std::runtime_error("The requested normalized region fixture does not exist.");
}

drs::engine::SfzNormalizedSection makeRegion(
    const std::vector<std::pair<std::string, std::string>>& values)
{
    drs::engine::SfzNormalizedSection section;
    section.scope = drs::engine::SfzOpcodeScope::region;
    section.headerName = "region";
    std::size_t column = 1;
    for (const auto& [name, value] : values)
    {
        drs::engine::SfzResolvedOpcode opcode;
        opcode.name = name;
        opcode.value = value;
        opcode.location = { "synthetic.sfz",
                            1,
                            column++,
                            drs::engine::SfzOpcodeScope::region,
                            name };
        section.localOpcodes.push_back(opcode);
        section.effectiveOpcodes.push_back(std::move(opcode));
    }
    section.localOpcodeCount = section.localOpcodes.size();
    return section;
}

const drs::engine::SfzImportOpcodeSupportSummary* findSupport(
    const drs::engine::SfzImportReport& report,
    const std::string& opcodeName)
{
    const auto iterator = std::find_if(report.opcodeSupport.begin(),
                                       report.opcodeSupport.end(),
                                       [&](const drs::engine::SfzImportOpcodeSupportSummary& value)
                                       {
                                           return value.opcodeName == opcodeName;
                                       });
    return iterator == report.opcodeSupport.end() ? nullptr : &(*iterator);
}

drs::engine::RuntimeProjectModel makeProjectionProject(const fs::path& root)
{
    drs::engine::RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "sfz-region-contract-projection";
    project.displayName = "SFZ Region Contract Projection";
    project.contentRootPath = root.generic_string();
    project.defaultInstrumentManifestPath = (root / "region-contract.drstrm").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto portablePath = fixturePath("portable-regions.sfz");
        const auto parsed = parseSfzDocument(portablePath.generic_string());
        require(parsed.parsed && parsed.complete,
                "The portable SFZ region fixture must parse completely.");
        const auto normalized = normalizeSfzDocument(parsed.document);
        require(normalized.normalized,
                "The portable SFZ region fixture must normalize completely.");

        const auto inherited = resolveSfzRegionContract(
            regionAt(normalized.document, 0),
            { std::uint64_t { 200 }, std::nullopt });
        require(inherited.valid,
                "The inherited portable region contract should be valid.");
        require(inherited.disposition == SfzRegionMappingDisposition::normalized,
                "Inclusive SFZ endpoints should make the mapping explicitly normalized.");
        require(inherited.region.playbackStart.frame == 12
                    && inherited.region.playbackStart.provenance.origin
                        == SfzRegionValueOrigin::inheritedOpcode,
                "The group offset should override global offset and retain inherited provenance.");
        require(inherited.region.playbackEndExclusive.frame == 100,
                "SFZ end=99 must convert once to native exclusive frame 100.");
        require(inherited.region.loopMode.mode == SfzRegionLoopMode::loopContinuous
                    && inherited.region.loopEnabledCompatibility(),
                "The inherited continuous loop mode should enable the compatibility loop.");
        require(inherited.region.loopStart.frame == 20
                    && inherited.region.loopEndExclusive.frame == 80,
                "SFZ loop_end=79 must convert once to native exclusive frame 80.");

        const auto oneFrame = resolveSfzRegionContract(
            regionAt(normalized.document, 1),
            { std::uint64_t { 1 }, std::nullopt });
        require(oneFrame.valid
                    && oneFrame.region.playbackStart.frame == 0
                    && oneFrame.region.playbackEndExclusive.frame == 1,
                "SFZ end=0 must remain a legal one-frame region and not collide with a sentinel.");
        require(oneFrame.region.loopMode.mode == SfzRegionLoopMode::oneShot
                    && !oneFrame.region.loopEnabledCompatibility(),
                "one_shot must retain typed identity while disabling the legacy loop boolean.");
        require(oneFrame.region.playbackEndExclusive.provenance.origin
                    == SfzRegionValueOrigin::localOpcode,
                "A region-local end override should retain local provenance.");

        const auto sustain = resolveSfzRegionContract(
            regionAt(normalized.document, 2),
            { std::uint64_t { 128 }, std::nullopt });
        require(sustain.valid
                    && sustain.region.loopMode.mode == SfzRegionLoopMode::loopSustain
                    && sustain.region.loopEnabledCompatibility(),
                "loop_sustain must be a first-class contract value even before typed runtime cutover.");

        require(parseSfzRegionLoopMode("NO_LOOP") == SfzRegionLoopMode::noLoop
                    && parseSfzRegionLoopMode("one_shot") == SfzRegionLoopMode::oneShot
                    && parseSfzRegionLoopMode("loop_continuous") == SfzRegionLoopMode::loopContinuous
                    && parseSfzRegionLoopMode("loop_sustain") == SfzRegionLoopMode::loopSustain
                    && !parseSfzRegionLoopMode("ping_pong").has_value(),
                "Only the four portable SFZ v1 loop modes belong to the baseline contract.");

        const auto metadataOnly = resolveSfzRegionContract(
            makeRegion({ { "sample", "metadata.wav" } }),
            { std::uint64_t { 1000 }, SfzWaveLoopMetadata { 100, 899 } });
        require(metadataOnly.valid
                    && metadataOnly.region.playbackEndExclusive.frame == 1000
                    && metadataOnly.region.playbackEndExclusive.provenance.origin
                        == SfzRegionValueOrigin::sourceAudioMetadata,
                "An omitted SFZ end should resolve to source length when metadata is available.");
        require(metadataOnly.region.loopMode.mode == SfzRegionLoopMode::loopContinuous
                    && metadataOnly.region.loopMode.provenance.origin
                        == SfzRegionValueOrigin::wavLoopMetadata
                    && metadataOnly.region.loopStart.frame == 100
                    && metadataOnly.region.loopEndExclusive.frame == 900,
                "WAV loop metadata should supply default loop behavior and convert its inclusive end.");

        const auto explicitOverride = resolveSfzRegionContract(
            makeRegion({ { "loop_mode", "loop_sustain" },
                         { "loop_start", "200" } }),
            { std::uint64_t { 1000 }, SfzWaveLoopMetadata { 100, 899 } });
        require(explicitOverride.valid
                    && explicitOverride.region.loopStart.frame == 200
                    && explicitOverride.region.loopStart.provenance.origin
                        == SfzRegionValueOrigin::localOpcode
                    && explicitOverride.region.loopEndExclusive.frame == 900
                    && explicitOverride.region.loopEndExclusive.provenance.origin
                        == SfzRegionValueOrigin::wavLoopMetadata,
                "Explicit SFZ loop values must override WAV fallback independently.");

        const auto wholeSourceLoop = resolveSfzRegionContract(
            makeRegion({ { "loop_mode", "loop_continuous" } }),
            { std::uint64_t { 64 }, std::nullopt });
        require(wholeSourceLoop.valid
                    && wholeSourceLoop.region.loopStart.frame == 0
                    && wholeSourceLoop.region.loopEndExclusive.frame == 64,
                "A loop mode with known source length should resolve to the complete playback region.");

        const auto deferredLoop = resolveSfzRegionContract(
            makeRegion({ { "loop_mode", "loop_continuous" } }));
        require(deferredLoop.valid
                    && deferredLoop.disposition == SfzRegionMappingDisposition::unsupported
                    && !deferredLoop.region.hasResolvedLoopRange()
                    && !deferredLoop.findings.empty(),
                "Unavailable source metadata must produce an honest finding instead of inventing a loop range.");

        const auto silentEnd = resolveSfzRegionContract(
            makeRegion({ { "end", "-1" } }));
        require(silentEnd.valid && silentEnd.region.playbackSuppressed,
                "The SFZ end=-1 silent-region sentinel must not be misclassified as malformed.");

        const auto invalidEnd = resolveSfzRegionContract(
            makeRegion({ { "end", "-2" } }));
        require(!invalidEnd.valid
                    && invalidEnd.disposition == SfzRegionMappingDisposition::invalid
                    && !invalidEnd.findings.empty()
                    && invalidEnd.findings.front().disposition
                        == SfzImportSupportDisposition::blocking,
                "Negative frame values must block region conversion.");

        const auto overflowingEnd = resolveSfzRegionContract(
            makeRegion({ { "loop_end", std::to_string(
                std::numeric_limits<std::uint64_t>::max()) } }));
        require(!overflowingEnd.valid,
                "An inclusive endpoint that cannot become exclusive must block conversion.");

        const auto unsupportedMode = resolveSfzRegionContract(
            makeRegion({ { "loop_mode", "ping_pong" },
                         { "loop_start", "1" },
                         { "loop_end", "2" } }));
        require(unsupportedMode.valid
                    && unsupportedMode.disposition == SfzRegionMappingDisposition::unsupported
                    && unsupportedMode.region.loopMode.mode == SfzRegionLoopMode::noLoop,
                "Unsupported loop modes should be reported without silently enabling a different loop.");

        const auto malformedPath = fixturePath("malformed-regions.sfz");
        const auto malformedAnalysis = analyzeSfzImportDocument(malformedPath.generic_string());
        require(malformedAnalysis.analyzed,
                "Malformed region values should still produce an analyze-first report.");
        const auto* endSupport = findSupport(malformedAnalysis.report, "end");
        const auto* offsetSupport = findSupport(malformedAnalysis.report, "offset");
        const auto* modeSupport = findSupport(malformedAnalysis.report, "loop_mode");
        require(endSupport != nullptr
                    && endSupport->disposition == SfzImportSupportDisposition::blocking
                    && offsetSupport != nullptr
                    && offsetSupport->disposition == SfzImportSupportDisposition::blocking,
                "Malformed frame opcodes must be visible as blocking import support findings.");
        require(modeSupport != nullptr
                    && modeSupport->disposition == SfzImportSupportDisposition::reportedOnly,
                "Unsupported loop modes must be visible as reported-only compatibility findings.");

        const auto portableAnalysis = analyzeSfzImportDocument(portablePath.generic_string());
        const auto* portableOffset = findSupport(portableAnalysis.report, "offset");
        const auto* portableEnd = findSupport(portableAnalysis.report, "end");
        const auto* portableLoopEnd = findSupport(portableAnalysis.report, "loop_end");
        const auto hasConvertedLoopMode = std::any_of(
            portableAnalysis.report.opcodeSupport.begin(),
            portableAnalysis.report.opcodeSupport.end(),
            [](const SfzImportOpcodeSupportSummary& value)
            {
                return value.opcodeName == "loop_mode"
                    && value.disposition == SfzImportSupportDisposition::converted;
            });
        require(portableOffset != nullptr
                    && portableOffset->disposition == SfzImportSupportDisposition::converted
                    && portableLoopEnd != nullptr
                    && portableLoopEnd->disposition == SfzImportSupportDisposition::converted,
                "Implemented offset and inclusive loop-end conversion should be reported as converted.");
        require(portableEnd != nullptr
                    && portableEnd->disposition == SfzImportSupportDisposition::converted,
                "SFZ end must report the typed playback-region conversion.");
        require(hasConvertedLoopMode,
                "Portable loop modes should report the typed loop-mode conversion.");

        const auto projectionRoot = fs::temp_directory_path()
            / "drs-sfz-region-contract-projection";
        fs::create_directories(projectionRoot);
        {
            std::ofstream sample(projectionRoot / "fixture.wav", std::ios::binary);
            sample.put('\0');
        }
        const auto projectionSfz = projectionRoot / "fixture.sfz";
        {
            std::ofstream sfz(projectionSfz, std::ios::binary);
            sfz << "<region> sample=fixture.wav offset=1 loop_mode=loop_continuous "
                   "loop_start=2 loop_end=5\n"
                   "<region> sample=fixture.wav end=-1\n"
                   "<region> sample=fixture.wav key=61 end=0\n"
                   "<region> sample=fixture.wav key=62 end=5\n";
        }
        const auto projection = projectSfzImportDocument(
            makeProjectionProject(projectionRoot),
            projectionSfz.generic_string());
        require(projection.projected && !projection.blocking
                    && projection.zones.size() == 3,
                "Production projection should consume the region contract and omit end=-1 silent regions.");
        require(projection.zones.front().sampleStartFrame == 1
                    && projection.zones.front().loopEnabled
                    && projection.zones.front().loopMode == RegionLoopMode::loopContinuous
                    && projection.zones.front().loopStartFrame == 2
                    && projection.zones.front().loopEndFrame == 6,
                "Production projection must retain the typed mode and convert inclusive SFZ loop_end exactly once.");
        require(projection.zones[0].sampleEndFrame == 0,
                "Omitted SFZ end must preserve the zero sentinel for physical source end.");
        require(projection.zones[1].sampleEndFrame == 1,
                "SFZ end=0 must convert from an inclusive endpoint to exclusive frame one.");
        require(projection.zones[2].sampleEndFrame == 6,
                "SFZ end must convert from inclusive to exclusive exactly once.");

        const auto preStartProjectionSfz = projectionRoot / "pre-start-projection.sfz";
        {
            std::ofstream sfz(preStartProjectionSfz, std::ios::binary | std::ios::trunc);
            sfz << "<region> sample=fixture.wav offset=10 end=29 loop_mode=loop_continuous "
                   "loop_start=4 loop_end=19\n";
        }
        const auto preStartProjection = projectSfzImportDocument(
            makeProjectionProject(projectionRoot), preStartProjectionSfz.generic_string());
        require(preStartProjection.projected && preStartProjection.playable
                    && !preStartProjection.blocking
                    && preStartProjection.zones.size() == 1,
                "SFZ projection must accept an earlier loop head in a playable zone.");
        require(preStartProjection.zones.front().sampleStartFrame == 10
                    && preStartProjection.zones.front().sampleEndFrame == 30
                    && preStartProjection.zones.front().loopEnabled
                    && preStartProjection.zones.front().loopMode == RegionLoopMode::loopContinuous
                    && preStartProjection.zones.front().loopStartFrame == 4
                    && preStartProjection.zones.front().loopEndFrame == 20,
                "SFZ projection must preserve playback start, earlier loop start, exclusive loop end, and mode.");
        AuthoringSession projectionSession(makeProjectionProject(projectionRoot));
        const auto applyResult = applySfzImportProjection(
            projectionSession, projection, "Import playback-region contract fixture");
        require(applyResult.applied
                    && projectionSession.getProject().schemaVersion == playbackRegionProjectSchemaVersion
                    && projectionSession.getProject().authoring.schemaVersion == playbackRegionAuthoringSchemaVersion
                    && sfzRegionPlaybackContractSchemaVersion == 2,
                "Applying a projection must promote old inputs to the named playback-region schema contract.");

        const auto savedProjectPath = projectionRoot / "region-contract.drsproj";
        {
            std::ofstream savedProject(savedProjectPath, std::ios::binary | std::ios::trunc);
            savedProject << serializeRuntimeProjectManifest(
                projectionSession.getProject(), savedProjectPath.generic_string());
        }
        std::ifstream savedProjectInput(savedProjectPath, std::ios::binary);
        const std::string savedProjectText((std::istreambuf_iterator<char>(savedProjectInput)),
                                           std::istreambuf_iterator<char>());
        const auto reopenedProject = parseRuntimeProjectManifest(
            savedProjectText, savedProjectPath.generic_string(), false);
        require(reopenedProject.loaded && reopenedProject.project.authoring.zones.size() == 3,
                "The imported SFZ region project must save and reopen"
                    + (reopenedProject.issues.empty()
                           ? std::string(".")
                           : std::string(": ") + reopenedProject.issues.front()));
        const auto& reopenedRegion = reopenedProject.project.authoring.zones.front();
        require(reopenedRegion.sampleStartFrame == 1
                    && reopenedRegion.sampleEndFrame == 0
                    && reopenedRegion.loopMode == RegionLoopMode::loopContinuous
                    && reopenedRegion.loopEnabled
                    && reopenedRegion.loopStartFrame == 2
                    && reopenedRegion.loopEndFrame == 6,
                "Project save/reopen must preserve the complete imported SFZ region contract.");

        const auto crossfadeMigration = migrateRuntimeProjectToLoopCrossfadeSchema(
            reopenedProject.project);
        require(crossfadeMigration.valid && crossfadeMigration.migrated,
                "The optional native loop crossfade must use its own project-schema increment.");
        auto crossfadeProject = crossfadeMigration.project;
        crossfadeProject.authoring.zones.front().loopCrossfadeFrames = 2;
        const auto crossfadeText = serializeRuntimeProjectManifest(
            crossfadeProject, savedProjectPath.generic_string());
        const auto crossfadeReopened = parseRuntimeProjectManifest(
            crossfadeText, savedProjectPath.generic_string(), false);
        require(crossfadeReopened.loaded
                    && crossfadeReopened.project.schemaVersion
                        == loopCrossfadeProjectSchemaVersion
                    && crossfadeReopened.project.authoring.schemaVersion
                        == loopCrossfadeAuthoringSchemaVersion
                    && crossfadeReopened.project.authoring.zones.front().loopCrossfadeFrames == 2,
                "Native loop crossfade metadata must survive project persistence independently of SFZ semantics.");

        const auto metadataSfz = projectionRoot / "metadata-fallback.sfz";
        {
            std::ofstream sfz(metadataSfz, std::ios::binary | std::ios::trunc);
            sfz << "<region> sample=fixture.wav key=60\n"
                   "<region> sample=fixture.wav key=61 loop_mode=loop_sustain "
                   "loop_start=80 loop_end=159\n"
                   "<region> sample=fixture.wav key=62 loop_mode=ping_pong\n";
        }
        SfzImportExecutionContext metadataContext;
        metadataContext.sourceRegionMetadataResolver = [](const std::string&)
            -> std::optional<SfzImportSourceRegionMetadata>
        {
            return SfzImportSourceRegionMetadata {
                std::uint64_t { 480 }, true, 64, 192
            };
        };
        const auto metadataProjection = projectSfzImportDocument(
            makeProjectionProject(projectionRoot), metadataSfz.generic_string(), metadataContext);
        require(metadataProjection.projected && metadataProjection.zones.size() == 3,
                "Production projection must accept source-metadata region fallback.");
        require(metadataProjection.zones[0].sampleEndFrame == 0
                    && metadataProjection.zones[0].loopMode == RegionLoopMode::loopContinuous
                    && metadataProjection.zones[0].loopStartFrame == 64
                    && metadataProjection.zones[0].loopEndFrame == 193,
                "Omitted SFZ region values must use WAV frame and inclusive-loop metadata fallback.");
        require(metadataProjection.zones[1].loopMode == RegionLoopMode::loopSustain
                    && metadataProjection.zones[1].loopStartFrame == 80
                    && metadataProjection.zones[1].loopEndFrame == 160,
                "Explicit SFZ loop values must take precedence over embedded WAV loop metadata.");
        require(std::any_of(metadataProjection.authoringNotes.begin(),
                            metadataProjection.authoringNotes.end(), [](const auto& note)
                            {
                                return note.find("sfz.region.loop_mode.unsupported")
                                    != std::string::npos;
                            }),
                "Unsupported region semantics must survive projection as an explicit conversion finding.");

        std::ifstream fixtureManifest(fixturePath("baseline-fixtures.csv"));
        require(fixtureManifest.good(),
                "The Sprint 1 deterministic fixture manifest must be checked in.");
        std::string manifestText((std::istreambuf_iterator<char>(fixtureManifest)),
                                 std::istreambuf_iterator<char>());
        for (const auto* requiredFixture : { "zero-length", "one-frame", "short-mono",
                                             "short-stereo", "multichannel", "long-mono" })
        {
            require(manifestText.find(requiredFixture) != std::string::npos,
                    std::string("The baseline fixture manifest is missing ") + requiredFixture + ".");
        }

        std::cout << "SFZ region contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "SFZ region contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
