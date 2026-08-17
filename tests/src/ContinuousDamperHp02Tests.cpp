#include "drs/engine/AuthoringSession.h"
#include "drs/engine/ContinuousDamper.h"
#include "drs/engine/PackageWriter.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/SfzImportProjection.h"
#include "shared/ProjectStorage.h"

#include <juce_core/juce_core.h>

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
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool contains(const std::vector<std::string>& values, const std::string& marker)
{
    return std::any_of(values.begin(), values.end(), [&](const auto& value)
                       { return value.find(marker) != std::string::npos; });
}

std::string join(const std::vector<std::string>& values)
{
    std::string result;
    for (const auto& value : values)
        result += (result.empty() ? "" : " | ") + value;
    return result;
}

RuntimeProjectModel blankSchemaSixProject(const fs::path& fixture)
{
    RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 6;
    project.projectId = "drs.hp02.salamander";
    project.displayName = "HP-02 Salamander Projection";
    project.contentRootPath = fixture.parent_path().generic_string();
    project.defaultInstrumentManifestPath = (fixture.parent_path() / "hp02.drinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 5;
    return project;
}

RuntimeProjectModel migrateReferenceProjectToDamperSchema()
{
    const auto loaded = loadPhase2ReferenceProjectManifest();
    require(loaded.loaded, "The Phase 2 reference project must load");
    auto project = loaded.project;
    if (project.schemaVersion == 2)
    {
        const auto phase3 = migrateRuntimeProjectToPhase3RoundRobinSchema(project);
        require(phase3.valid, "Reference project must migrate to schema 3");
        project = phase3.project;
    }
    if (project.schemaVersion == 3)
    {
        const auto groups = migrateRuntimeProjectToZoneGroupsSchema(project);
        require(groups.valid, "Reference project must migrate to schema 4");
        project = groups.project;
    }
    const auto dsp = migrateRuntimeProjectToCuratedDspSchema(project);
    require(dsp.valid, "Reference project must migrate to schema 5");
    const auto performance = migrateRuntimeProjectToPerformanceArticulationSchema(dsp.project);
    require(performance.valid, "Reference project must migrate to schema 6");
    const auto damper = migrateRuntimeProjectToContinuousDamperSchema(performance.project);
    require(damper.valid, "Reference project must migrate to schema 7");
    return damper.project;
}

void verifyCurveCompiler()
{
    const auto compiled = compileContinuousDamperCurve({
        { 32, 0.0 }, { 42, 0.1 }, { 64, 1.0 }, { 127, 1.0 }
    });
    require(compiled.compiled, "Valid sparse curves must compile");
    require(compiled.values[0] == 0.0 && compiled.values[127] == 1.0,
            "Curve endpoints must extend deterministically");
    require(std::abs(compiled.values[37] - 0.05) < 1.0e-12,
            "Sparse curve gaps must use linear interpolation");

    const auto duplicate = compileContinuousDamperCurve({ { 32, 0.0 }, { 32, 0.5 } });
    require(!duplicate.compiled && duplicate.findingCode == "damper.curve.point_duplicate",
            "Duplicate curve points must produce the stable duplicate finding");
    const auto outOfRange = compileContinuousDamperCurve({ { 64, 1.25 } });
    require(!outOfRange.compiled && outOfRange.findingCode == "damper.curve.point_out_of_range",
            "Out-of-range curve values must produce the stable range finding");
}

void verifyMalformedCurveReferenceIsBlocking(const fs::path& fixture,
                                             const fs::path& tempRoot)
{
    const auto malformed = tempRoot / "missing-curve-reference.sfz";
    fs::create_directories(tempRoot);
    std::ofstream stream(malformed, std::ios::binary | std::ios::trunc);
    stream << "<global> ampeg_dynamic=1 ampeg_releasecc64=100 "
              "ampeg_release_curvecc64=12\n"
              "<region> sample="
           << (fixture.parent_path() / ".." / ".." / ".." / ".."
               / "hise_project" / "Samples" / "DRS_Sine_A3.wav").lexically_normal().generic_string()
           << " key=69\n";
    stream.close();

    const auto projection = projectSfzImportDocument(blankSchemaSixProject(malformed),
                                                      malformed.generic_string());
    require(projection.blocking && !projection.projected && projection.zones.empty(),
            "A referenced curve that is not declared must block projection atomically");
    require(contains(projection.issues, "damper.curve_reference_missing"),
            "Missing curve references must retain their stable finding code: "
                + join(projection.issues));
}

void verifyProjectSnapshotAndPreparedPropagation(const ContinuousDamperDefinition& sourceDamper)
{
    auto project = migrateReferenceProjectToDamperSchema();
    require(!project.authoring.zones.empty(), "Reference project must expose zones");
    project.authoring.zones.front().damper = sourceDamper;
    require(validateRuntimeProjectModel(project).valid,
            "Schema-7 project with compiled damper metadata must validate");

    PlaybackSnapshotBuilder snapshotBuilder;
    const auto first = snapshotBuilder.buildSnapshot(snapshotBuilder.requestBuild(1, true), project);
    require(first.built && first.activationEligible && !first.snapshot.zones.empty(),
            "Damper project must build an activation-eligible snapshot");
    require(first.snapshot.zones.front().damper == sourceDamper,
            "Snapshot must retain exact immutable damper metadata");

    auto changed = project;
    changed.authoring.zones.front().damper.releaseAmountSeconds = 99.0;
    const auto second = snapshotBuilder.buildSnapshot(snapshotBuilder.requestBuild(2, true), changed);
    require(second.built && first.snapshot.contentDigest != second.snapshot.contentDigest,
            "Damper metadata changes must invalidate the snapshot content digest");

    const auto referenceManifest = loadPhase1ReferenceInstrumentManifest();
    require(referenceManifest.loaded, "Reference instrument must load for prepared playback");
    const auto referenceStream = loadRuntimeStreamContainerForInstrument(referenceManifest);
    require(referenceStream.loaded, "Reference stream must load for prepared playback");
    PreparedPlaybackService preparedService;
    const auto request = preparedService.requestBuild(first, referenceStream);
    require(request.accepted, "Prepared playback must accept the damper snapshot");
    const auto prepared = preparedService.prepare(request, first, referenceStream);
    require(prepared.built && prepared.activationEligible && !prepared.prepared.zones.empty(),
            "Prepared playback must build with damper metadata");
    require(prepared.prepared.zones.front().damper == sourceDamper,
            "Prepared playback must retain exact immutable damper metadata");
    require(serializeImmutablePreparedPlayback(prepared.prepared).find("\"damper\"")
                != std::string::npos,
            "Prepared playback serialization must include damper metadata in its digest input");
}

void verifyInstrumentAndPackageRoundTrip(const RuntimeProjectModel& project,
                                         const fs::path& tempRoot)
{
    const auto projectPath = tempRoot / "hp02.drsproj";
    const auto instrument = drs::app::buildInstrumentManifestForProject(
        project, juce::File(projectPath.generic_string()));
    require(instrument.schemaVersion == playbackRegionInstrumentSchemaVersion
                && !instrument.zones.empty() && instrument.zones.front().damper.dynamicRelease,
            "Current playback-region projects must emit runtime instrument schema 6 with damper metadata");

    const auto instrumentPath = tempRoot / "hp02.drinst";
    const auto serialized = serializeRuntimeInstrumentManifest(instrument,
                                                                instrumentPath.generic_string());
    const auto parsed = parseRuntimeInstrumentManifest(serialized,
                                                        instrumentPath.generic_string(), false);
    require(parsed.loaded && parsed.instrument.zones.front().damper == instrument.zones.front().damper,
            "Runtime instrument 6 must round-trip exact damper metadata: " + join(parsed.issues));
    require(serializeRuntimeInstrumentManifest(parsed.instrument, instrumentPath.generic_string())
                == serialized,
            "Runtime instrument 6 serialization must be byte deterministic");

    PerformancePackageWritePlan writePlan;
    writePlan.outputPackagePath = (tempRoot / "hp02.drpkg").generic_string();
    writePlan.manifest.packageId = "drs.hp02.package";
    writePlan.manifest.displayName = "HP-02 Package";
    writePlan.manifest.instrumentId = instrument.instrumentId;
    writePlan.payloads.push_back({
        "runtime-instrument", PerformancePackagePayloadKind::runtimeInstrument,
        "manifest/runtime-instrument.drinst", "application/json",
        std::vector<std::uint8_t>(serialized.begin(), serialized.end())
    });
    const auto written = writePerformancePackage(writePlan);
    require(written.written, "The unchanged package container must carry instrument schema 6");
    const auto inspected = inspectPerformancePackage(writePlan.outputPackagePath);
    require(inspected.valid, "The HP-02 package must reopen and authenticate");
    const auto payload = std::find_if(inspected.payloads.begin(), inspected.payloads.end(),
                                      [](const auto& value)
                                      { return value.payloadKind == "runtimeInstrument"; });
    require(payload != inspected.payloads.end(), "Reopened package must contain runtimeInstrument");
    const std::string reopenedJson(payload->plaintextBytes.begin(), payload->plaintextBytes.end());
    const auto reopened = parseRuntimeInstrumentManifest(reopenedJson,
                                                          "package://manifest/runtime-instrument.drinst",
                                                          false);
    require(reopened.loaded && reopened.instrument.zones.front().damper == instrument.zones.front().damper,
            "Package reopen must retain runtime-instrument damper metadata");
}
} // namespace

int main()
{
    try
    {
        verifyCurveCompiler();
        const auto fixture = fs::path(DRS_HP02_FIXTURE_ROOT)
            / "accurate-salamander-half-pedal.sfz";
        const auto analysis = analyzeSfzImportDocument(fixture.generic_string());
        require(analysis.analyzed && analysis.report.available,
                "Focused Salamander fixture must analyze");
        const auto supported = [&](const std::string& opcode)
        {
            return std::any_of(analysis.report.opcodeSupport.begin(),
                               analysis.report.opcodeSupport.end(), [&](const auto& summary)
                               {
                                   return summary.opcodeName == opcode
                                       && summary.disposition == SfzImportSupportDisposition::converted;
                               });
        };
        for (const auto* opcode : { "sustain_cc", "ampeg_dynamic", "ampeg_releasecc64",
                                    "ampeg_release_curvecc64", "curve_index", "v064" })
            require(supported(opcode), std::string("Supported damper opcode must classify converted: ") + opcode);
        require(std::any_of(analysis.report.opcodeSupport.begin(), analysis.report.opcodeSupport.end(),
                            [](const auto& summary)
                            {
                                return summary.opcodeName == "width_oncc1"
                                    && summary.disposition == SfzImportSupportDisposition::reportedOnly;
                            }),
                "Unrelated modulation must remain explicitly report-only");

        const auto baseProject = blankSchemaSixProject(fixture);
        const auto projection = projectSfzImportAnalysis(baseProject, analysis);
        require(projection.projected && projection.playable && !projection.blocking
                    && projection.zones.size() == 1,
                "Focused Salamander half-pedal region must project safely");
        const auto& damper = projection.zones.front().damper;
        require(damper.sustainControllerNumber == 90
                    && std::abs(damper.sustainThreshold - 0.5) < 1.0e-12
                    && damper.dynamicRelease && damper.releaseControllerNumber == 64
                    && damper.releaseAmountSeconds == 100.0 && damper.releaseCurveIndex == 11,
                "Projection must retain Salamander sustain/release metadata and ARIA default");
        require(damper.releaseCurve[32] == 0.0 && damper.releaseCurve[64] == 1.0
                    && damper.releaseCurve[127] == 1.0,
                "Projection must carry the compiled 128-value Salamander curve");
        require(projection.zones.front().releaseSeconds == 0.25,
                "An explicit SFZ ampeg_release must override the 30 ms native release default");

        AuthoringSession session(baseProject);
        const auto applied = applySfzImportProjection(session, projection, "Import HP-02 fixture");
        require(applied.applied
                    && session.getProject().schemaVersion == playbackRegionProjectSchemaVersion
                    && session.getProject().authoring.schemaVersion == playbackRegionAuthoringSchemaVersion,
                "Applying imported content must atomically advance through damper schema 7/6 to playback-region schema 8/7");
        const auto projectJson = serializeRuntimeProjectManifest(session.getProject(),
                                                                 (fs::path(DRS_HP02_FIXTURE_ROOT) / "hp02.drsproj").generic_string());
        const auto reloadedProject = parseRuntimeProjectManifest(projectJson,
                                                                  (fs::path(DRS_HP02_FIXTURE_ROOT) / "hp02.drsproj").generic_string(),
                                                                  false);
        require(reloadedProject.loaded
                    && reloadedProject.project.authoring.zones.front().damper == damper,
                "Current project schema must round-trip exact damper metadata");

        verifyProjectSnapshotAndPreparedPropagation(damper);
        const auto tempRoot = fs::temp_directory_path() / "drs-hp02-tests";
        fs::create_directories(tempRoot);
        verifyMalformedCurveReferenceIsBlocking(fixture, tempRoot);
        verifyInstrumentAndPackageRoundTrip(session.getProject(), tempRoot);

        std::cout << "Continuous damper HP-02 persistence and SFZ projection tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Continuous damper HP-02 tests failed: " << exception.what() << '\n';
        return 1;
    }
}
