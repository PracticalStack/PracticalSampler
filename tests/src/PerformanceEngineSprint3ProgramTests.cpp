#include "drs/engine/PerformanceProgram.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/SamplerRenderModel.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace
{
using namespace drs::engine;
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition) throw std::runtime_error(message);
}

RuntimeProjectModel makeProject()
{
    const auto loaded = loadRuntimeProjectManifest(DRS_PERFORMANCE_ENGINE_S3_LEGACY_FIXTURE_PATH);
    require(loaded.loaded, "Sprint 3 fixture must load.");
    auto legacy = loaded.project;
    for (std::size_t index = 0; index < legacy.authoring.zones.size(); ++index)
    {
        legacy.authoring.zones[index].articulationId = index == 1 ? "staccato" : "sustain";
        legacy.authoring.zones[index].keyLow = 36;
        legacy.authoring.zones[index].keyHigh = 100;
    }
    const auto migration = migrateRuntimeProjectToPerformanceArticulationSchema(legacy);
    require(migration.valid, "Sprint 3 fixture must migrate to explicit articulations.");
    auto project = migration.project;
    project.authoring.articulations[0].activation = { PerformanceEventKind::noteOn, 12, ArticulationActivationMode::latch, true };
    project.authoring.articulations[1].activation = { PerformanceEventKind::noteOn, 14, ArticulationActivationMode::latch, true };
    auto& main = project.authoring.zones[0];
    main.performance = { PerformanceEventKind::noteOn, PerformanceSustainCondition::any, PerformancePitchSource::eventNote };
    main.exclusiveGroupId = "main";
    main.exclusiveTargetGroupIds = { "release" };
    main.chokeReleaseSeconds = 0.02;
    auto& release = project.authoring.zones[1];
    release.triggerMode = ZoneTriggerMode::oneShot;
    release.performance = { PerformanceEventKind::release, PerformanceSustainCondition::pedalUp, PerformancePitchSource::eventNote };
    release.exclusiveGroupId = "release";
    auto& pedal = project.authoring.zones[2];
    pedal.triggerMode = ZoneTriggerMode::oneShot;
    pedal.rootKey = 36;
    pedal.performance = { PerformanceEventKind::pedalUp, PerformanceSustainCondition::pedalUp, PerformancePitchSource::fixedRoot };
    project.authoring.roundRobinResetRules = { { RoundRobinResetEvent::articulationChange, true, {} } };
    require(validateRuntimeProjectModel(project).valid, "Sprint 3 fixture must validate.");
    return project;
}

void verifyProgramAndSnapshot()
{
    static_assert(std::is_trivially_copyable<CompiledPerformanceTriggerRoute>::value, "Trigger records must be POD.");
    static_assert(std::is_trivially_copyable<CompiledPerformanceActivation>::value, "Activation records must be POD.");
    const auto project = makeProject();
    const auto first = compilePerformanceProgram(project.authoring);
    const auto second = compilePerformanceProgram(project.authoring);
    require(first.compiled && second.compiled, "Valid rules must compile.");
    require(serializeCompiledPerformanceProgram(first.program) == serializeCompiledPerformanceProgram(second.program),
            "Equivalent rule sets must compile byte deterministically.");
    require(first.program.activationByMidiNote[12].articulationIndex != kInvalidPerformanceProgramIndex,
            "Key switches must compile to numeric articulation indices.");
    require(first.program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::noteOn)].routeCount == 1
                && first.program.eventRanges[static_cast<std::size_t>(PerformanceEventKind::release)].routeCount == 1,
            "Event table ranges must isolate note and release routes.");
    require(first.program.triggerRoutes.front().chokeTargetMask != 0 && first.program.retainedBytes > 0,
            "Trigger routes must retain numeric choke masks and memory accounting.");

    PlaybackSnapshotBuilder builder;
    const auto snapshot = builder.buildSnapshot(builder.requestBuild(3, true), project);
    require(snapshot.built && snapshot.activationEligible, "Valid rules must enter an activation-eligible snapshot.");
    require(snapshot.snapshot.articulationDefinitions.size() == project.authoring.articulations.size()
                && snapshot.snapshot.roundRobinResetRules.size() == 1,
            "Snapshot must retain authored rule declarations.");
    require(serializeCompiledPerformanceProgram(snapshot.snapshot.performanceProgram)
                == serializeCompiledPerformanceProgram(first.program),
            "Snapshot must retain the compiled numeric program.");
    PreparedPlaybackService preparation;
    const RuntimeStreamLoadResult noCompiledStream;
    const auto prepared = preparation.prepare(preparation.requestBuild(snapshot), snapshot, noCompiledStream);
    require(prepared.built && prepared.metrics.preparedPerformanceProgramBytes == first.program.retainedBytes,
            "Prepared playback must retain and account for the compiled performance program.");
    auto displayOnlyProject = project;
    displayOnlyProject.authoring.articulations.front().displayName = "Renamed Sustain";
    const auto displayOnlySnapshot = builder.buildSnapshot(builder.requestBuild(4, true), displayOnlyProject);
    const auto displayOnlyPrepared = preparation.prepare(preparation.requestBuild(displayOnlySnapshot),
                                                          displayOnlySnapshot, noCompiledStream);
    require(displayOnlyPrepared.built
                && displayOnlyPrepared.metrics.cacheHitCount == displayOnlySnapshot.snapshot.sampleIdentities.size(),
            "Display-only articulation edits must reuse prepared sample decodes.");
    auto topologyProject = project;
    topologyProject.authoring.zones.front().performance.event = PerformanceEventKind::noteOff;
    const auto topologySnapshot = builder.buildSnapshot(builder.requestBuild(5, true), topologyProject);
    const auto topologyPrepared = preparation.prepare(preparation.requestBuild(topologySnapshot),
                                                       topologySnapshot, noCompiledStream);
    require(topologyPrepared.built
                && topologyPrepared.metrics.cacheHitCount == topologySnapshot.snapshot.sampleIdentities.size()
                && serializeCompiledPerformanceProgram(topologySnapshot.snapshot.performanceProgram)
                    != serializeCompiledPerformanceProgram(first.program),
            "Trigger-topology edits must rebuild numeric routes while reusing decoded samples.");
    DraftPlaybackContract contract(3);
    const auto request = contract.requestPreviewBuild();
    require(contract.completePreviewBuild(request.requestId, snapshot, prepared),
            "Prepared playback must produce an activation payload.");
    const auto render = buildSamplerRenderModel(contract.getStatus().preview.activationPayload);
    require(render.built && serializeCompiledPerformanceProgram(render.model->getPerformanceProgram())
                == serializeCompiledPerformanceProgram(first.program),
            "Render models must retain the string-free prepared program from activation payloads.");
    const auto text = serializeImmutablePlaybackSnapshot(snapshot.snapshot);
    require(text.find("\"performanceProgram\"") != std::string::npos
                && text.find("\"exclusiveTargetGroupIds\"") != std::string::npos,
            "Snapshot diagnostics must contain declarations and program data.");
}

void verifyInstrumentV3AndCompatibility()
{
    const auto legacy = loadPhase1ReferenceInstrumentManifest();
    require(legacy.loaded && legacy.instrument.schemaVersion == 1, "Reference v1 manifest must continue to load.");
    auto instrument = legacy.instrument;
    instrument.schemaVersion = 3;
    instrument.articulations.front().activation = { PerformanceEventKind::noteOn, 12, ArticulationActivationMode::latch, true };
    instrument.zones.front().exclusiveGroupId = "voices";
    instrument.zones.front().exclusiveTargetGroupIds = { "release" };
    instrument.zones.front().chokeReleaseSeconds = 0.02;
    instrument.roundRobinResetRules = { { RoundRobinResetEvent::pedalUp, true, {} } };
    const auto output = fs::temp_directory_path() / "drs-performance-engine-s3-v3.drinst";
    std::ofstream(output, std::ios::binary) << serializeRuntimeInstrumentManifest(instrument, output.generic_string());
    const auto reloaded = loadRuntimeInstrumentManifest(output.generic_string());
    require(reloaded.loaded && reloaded.instrument.schemaVersion == 3, "Schema-3 instruments must load.");
    require(reloaded.instrument.articulations.front().activation.has_value()
                && reloaded.instrument.zones.front().exclusiveTargetGroupIds.size() == 1
                && reloaded.instrument.roundRobinResetRules.size() == 1,
            "Schema-3 round-trip must preserve performance declarations.");
}

void verifyCompilerSchemaThree()
{
    const auto contentRoot = fs::path(getPhase1ReferenceProjectManifestPath()).parent_path()
        / ".." / ".." / ".." / ".." / "hise_project";
    const auto samplePath = (contentRoot / "Samples" / "DRS_Sine_A3.wav").lexically_normal();
    const auto inspected = inspectSampleFile(samplePath.generic_string());
    require(inspected.accepted, "Compiler v3 coverage needs the checked-in reference sample.");
    RuntimeCompilePlan plan;
    plan.outputProjectPath = (fs::temp_directory_path() / "drs-s3.drsproj").generic_string();
    plan.outputInstrumentPath = (fs::temp_directory_path() / "drs-s3.drinst").generic_string();
    plan.outputStreamPath = (fs::temp_directory_path() / "drs-s3.drstrm").generic_string();
    plan.projectId = "s3-project";
    plan.projectDisplayName = "Sprint 3";
    plan.contentRootPath = contentRoot.lexically_normal().generic_string();
    plan.instrumentId = "s3-instrument";
    plan.instrumentDisplayName = "Sprint 3";
    plan.defaultLoadProfile = "balanced";
    plan.sampleSources.push_back({ "source", samplePath.generic_string(), "core", inspected.metadata });
    RuntimeArticulationDefinition articulation { "default", "Default", true };
    articulation.activation = { PerformanceEventKind::noteOn, 12, ArticulationActivationMode::latch, true };
    plan.articulations.push_back(articulation);
    plan.groups.push_back({ "group", "Group", { "default" } });
    RuntimeCompileZoneDefinition zone;
    zone.id = "zone";
    zone.sourceId = "source";
    zone.groupId = "group";
    zone.articulationId = "default";
    zone.keyLow = 36;
    zone.keyHigh = 100;
    zone.exclusiveGroupId = "voices";
    plan.zones.push_back(zone);
    const auto result = compileRuntimeInstrument(plan);
    require(result.compiled && result.instrument.schemaVersion == 3,
            "Compiler must emit schemaVersion 3 when a performance declaration is present.");
    require(serializeRuntimeInstrumentManifest(result.instrument, plan.outputInstrumentPath).find("\"performance\"")
                != std::string::npos,
            "Compiled schema-3 manifests must persist trigger declarations.");
}
} // namespace

int main()
{
    try
    {
        verifyProgramAndSnapshot();
        verifyInstrumentV3AndCompatibility();
        verifyCompilerSchemaThree();
        std::cout << "Performance-engine Sprint 3 program tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 3 program tests failed: " << exception.what() << '\n';
        return 1;
    }
}
