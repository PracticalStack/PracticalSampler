#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/NativeContent.h"

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

bool containsIssueFragment(const std::vector<std::string>& issues, const std::string& fragment)
{
    for (const auto& issue : issues)
    {
        if (issue.find(fragment) != std::string::npos)
            return true;
    }

    return false;
}

const drs::engine::PlaybackSnapshotZone& findSnapshotZone(
    const drs::engine::ImmutablePlaybackSnapshot& snapshot,
    const std::string& zoneId)
{
    for (const auto& zone : snapshot.zones)
    {
        if (zone.id == zoneId)
            return zone;
    }

    throw std::runtime_error("Could not find snapshot zone '" + zoneId + "'.");
}

const drs::engine::PreparedPlaybackZoneHandle& findPreparedZone(
    const drs::engine::ImmutablePreparedPlayback& prepared,
    const std::string& zoneId)
{
    for (const auto& zone : prepared.zones)
    {
        if (zone.zoneId == zoneId)
            return zone;
    }

    throw std::runtime_error("Could not find prepared zone '" + zoneId + "'.");
}

const drs::engine::RuntimeZoneDefinition& findInstrumentZone(
    const drs::engine::RuntimeInstrumentModel& instrument,
    const std::string& zoneId)
{
    for (const auto& zone : instrument.zones)
    {
        if (zone.id == zoneId)
            return zone;
    }

    throw std::runtime_error("Could not find instrument zone '" + zoneId + "'.");
}

void requireCrossfadeEquals(const drs::engine::VelocityCrossfadeDescriptor& crossfade,
                            int fadeInLowVelocity,
                            int fadeInHighVelocity,
                            int fadeOutLowVelocity,
                            int fadeOutHighVelocity,
                            const std::string& context)
{
    require(crossfade.fadeInLowVelocity == fadeInLowVelocity,
            context + " should preserve fadeInLowVelocity.");
    require(crossfade.fadeInHighVelocity == fadeInHighVelocity,
            context + " should preserve fadeInHighVelocity.");
    require(crossfade.fadeOutLowVelocity == fadeOutLowVelocity,
            context + " should preserve fadeOutLowVelocity.");
    require(crossfade.fadeOutHighVelocity == fadeOutHighVelocity,
            context + " should preserve fadeOutHighVelocity.");
}

void requireCrossfadeRuntimeEquals(const drs::engine::VelocityCrossfadeRuntimeDescriptor& runtime,
                                   int effectiveLowVelocity,
                                   int effectiveHighVelocity,
                                   const std::string& fadeInNeighborZoneId,
                                   const std::string& fadeOutNeighborZoneId,
                                   int fadeInOverlapLowVelocity,
                                   int fadeInOverlapHighVelocity,
                                   int fadeOutOverlapLowVelocity,
                                   int fadeOutOverlapHighVelocity,
                                   const std::string& context)
{
    require(runtime.effectiveLowVelocity == effectiveLowVelocity,
            context + " should preserve effectiveLowVelocity.");
    require(runtime.effectiveHighVelocity == effectiveHighVelocity,
            context + " should preserve effectiveHighVelocity.");
    require(runtime.fadeInNeighborZoneId == fadeInNeighborZoneId,
            context + " should preserve fadeInNeighborZoneId.");
    require(runtime.fadeOutNeighborZoneId == fadeOutNeighborZoneId,
            context + " should preserve fadeOutNeighborZoneId.");
    require(runtime.fadeInOverlapLowVelocity == fadeInOverlapLowVelocity,
            context + " should preserve fadeInOverlapLowVelocity.");
    require(runtime.fadeInOverlapHighVelocity == fadeInOverlapHighVelocity,
            context + " should preserve fadeInOverlapHighVelocity.");
    require(runtime.fadeOutOverlapLowVelocity == fadeOutOverlapLowVelocity,
            context + " should preserve fadeOutOverlapLowVelocity.");
    require(runtime.fadeOutOverlapHighVelocity == fadeOutOverlapHighVelocity,
            context + " should preserve fadeOutOverlapHighVelocity.");
}

drs::engine::RuntimeProjectModel buildCrossfadeReadyProject()
{
    const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
    require(phase2Project.loaded,
            "Phase 2 reference project must load for crossfade prepared-playback tests.");
    require(phase2Project.project.authoring.zones.size() >= 2,
            "Phase 2 reference project must expose at least two authored zones.");

    auto project = phase2Project.project;
    auto& lowerZone = project.authoring.zones.at(0);
    auto& upperZone = project.authoring.zones.at(1);

    project.authoring.selectedZoneId = lowerZone.id;

    lowerZone.rootKey = 57;
    lowerZone.keyLow = 36;
    lowerZone.keyHigh = 59;
    lowerZone.velocityLow = 1;
    lowerZone.velocityHigh = 60;
    lowerZone.roundRobinLength = 1;
    lowerZone.roundRobinPosition = 1;
    lowerZone.velocityCrossfade = {};
    lowerZone.velocityCrossfade.fadeOutLowVelocity = 25;
    lowerZone.velocityCrossfade.fadeOutHighVelocity = 60;
    lowerZone.tuningModulation.controllerNumber = 20;
    lowerZone.tuningModulation.amount = 4800.0;
    lowerZone.tuningModulation.curveIndex = 9;
    lowerZone.tuningModulation.curve[63] = 0.25f;
    lowerZone.tuningModulation.curve[127] = 1.0f;

    upperZone.rootKey = lowerZone.rootKey;
    upperZone.keyLow = lowerZone.keyLow;
    upperZone.keyHigh = lowerZone.keyHigh;
    upperZone.velocityLow = 25;
    upperZone.velocityHigh = 127;
    upperZone.roundRobinLength = 1;
    upperZone.roundRobinPosition = 1;
    upperZone.velocityCrossfade = {};
    upperZone.velocityCrossfade.fadeInLowVelocity = 25;
    upperZone.velocityCrossfade.fadeInHighVelocity = 60;

    return project;
}

drs::engine::RuntimeCompilePlan buildCrossfadeCompilePlan(const fs::path& outputDirectory)
{
    const auto samplePath = fs::path(drs::engine::getNativeContentRoots().samplesRoot)
        / "DRS_Sine_A3.wav";

    const auto sampleImport = drs::engine::importSampleFile(samplePath.generic_string());
    require(sampleImport.imported,
            "Reference sine sample must import successfully before compile crossfade tests run.");

    drs::engine::RuntimeCompilePlan plan;
    plan.outputProjectPath = (outputDirectory / "crossfade-compile.drsproj").generic_string();
    plan.outputInstrumentPath = (outputDirectory / "crossfade-compile.drinst").generic_string();
    plan.outputStreamPath = (outputDirectory / "crossfade-compile.drstrm").generic_string();
    plan.projectId = "drs.phase3.crossfade-prepared-playback";
    plan.projectDisplayName = "DRS Phase 3 Crossfade Prepared Playback";
    plan.contentRootPath = fs::path(drs::engine::getNativeContentRoots().samplesRoot)
        .lexically_normal().generic_string();
    plan.instrumentId = "drs.phase3.crossfade-compile";
    plan.instrumentDisplayName = "DRS Phase 3 Crossfade Compile";
    plan.defaultLoadProfile = "balanced";
    plan.pageSizeBytes = 65536;
    plan.controllerDefaults = { { 20, 63 }, { 93, 127 } };

    drs::engine::RuntimeCompileSourceDefinition source;
    source.id = "sine-a3";
    source.sourcePath = samplePath.generic_string();
    source.role = "crossfade-sustain";
    source.metadata = sampleImport.sample.metadata;
    plan.sampleSources.push_back(std::move(source));

    drs::engine::RuntimeArticulationDefinition articulation;
    articulation.id = "sustain";
    articulation.name = "Sustain";
    articulation.isDefault = true;
    plan.articulations.push_back(std::move(articulation));

    drs::engine::RuntimeGroupDefinition group;
    group.id = "pad-core";
    group.name = "Pad Core";
    group.articulationIds = { "sustain" };
    plan.groups.push_back(std::move(group));

    drs::engine::RuntimeCompileZoneDefinition lowerZone;
    lowerZone.id = "pad-a3-layer-1";
    lowerZone.sourceId = "sine-a3";
    lowerZone.groupId = "pad-core";
    lowerZone.articulationId = "sustain";
    lowerZone.rootKey = 57;
    lowerZone.keyLow = 36;
    lowerZone.keyHigh = 76;
    lowerZone.velocityLow = 1;
    lowerZone.velocityHigh = 60;
    lowerZone.roundRobinLength = 1;
    lowerZone.roundRobinPosition = 1;
    lowerZone.velocityCrossfade.fadeOutLowVelocity = 25;
    lowerZone.velocityCrossfade.fadeOutHighVelocity = 60;
    lowerZone.tuningModulation.controllerNumber = 20;
    lowerZone.tuningModulation.amount = 4800.0;
    lowerZone.tuningModulation.curveIndex = 9;
    lowerZone.tuningModulation.curve[63] = 0.25;
    lowerZone.tuningModulation.curve[127] = 1.0;
    lowerZone.amplitudeModulation.controllerNumber = 93;
    lowerZone.amplitudeModulation.amount = 100.0;
    lowerZone.amplitudeEnvelope.holdSeconds = 0.4;
    lowerZone.amplitudeEnvelope.decaySeconds = 2.0;
    lowerZone.amplitudeEnvelope.holdModulation.controllerNumber = 27;
    lowerZone.amplitudeEnvelope.holdModulation.amount = -0.4;
    lowerZone.amplitudeEnvelope.holdModulation.curveIndex = 8;
    lowerZone.amplitudeEnvelope.holdModulation.curve[127] = 1.0;
    lowerZone.amplitudeEnvelope.decayModulation.controllerNumber = 27;
    lowerZone.amplitudeEnvelope.decayModulation.amount = -1.9;
    lowerZone.amplitudeEnvelope.decayModulation.curveIndex = 8;
    lowerZone.amplitudeEnvelope.decayModulation.curve[127] = 1.0;
    lowerZone.amplitudeEnvelope.sustainModulation.controllerNumber = 27;
    lowerZone.amplitudeEnvelope.sustainModulation.amount = -100.0;
    lowerZone.amplitudeEnvelope.sustainModulation.curveIndex = 7;
    lowerZone.amplitudeEnvelope.sustainModulation.curve[127] = 1.0;
    plan.zones.push_back(std::move(lowerZone));

    drs::engine::RuntimeCompileZoneDefinition upperZone;
    upperZone.id = "pad-a3-layer-2";
    upperZone.sourceId = "sine-a3";
    upperZone.groupId = "pad-core";
    upperZone.articulationId = "sustain";
    upperZone.rootKey = 57;
    upperZone.keyLow = 36;
    upperZone.keyHigh = 76;
    upperZone.velocityLow = 25;
    upperZone.velocityHigh = 127;
    upperZone.roundRobinLength = 1;
    upperZone.roundRobinPosition = 1;
    upperZone.velocityCrossfade.fadeInLowVelocity = 25;
    upperZone.velocityCrossfade.fadeInHighVelocity = 60;
    plan.zones.push_back(std::move(upperZone));

    return plan;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto crossfadeProject = buildCrossfadeReadyProject();
        const auto referenceManifest = loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded,
                "Phase 1 reference instrument must load for prepared crossfade tests.");
        const auto referenceStream = loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded,
                "Phase 1 reference stream container must load for prepared crossfade tests.");

        PlaybackSnapshotBuilder snapshotBuilder;
        const auto snapshotRequest = snapshotBuilder.requestBuild(1, true);
        require(snapshotRequest.accepted, "Crossfade snapshot request should be accepted.");
        const auto snapshotResult = snapshotBuilder.buildSnapshot(snapshotRequest, crossfadeProject);
        require(snapshotResult.built, "Crossfade-authored project should build a playback snapshot.");
        require(snapshotResult.activationEligible,
                "Crossfade-authored project should remain activation eligible.");

        const auto& lowerSnapshotZone = findSnapshotZone(snapshotResult.snapshot, "pad-a3-low");
        const auto& upperSnapshotZone = findSnapshotZone(snapshotResult.snapshot, "pad-a3-high");
        requireCrossfadeEquals(lowerSnapshotZone.velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Snapshot lower-zone crossfade metadata");
        requireCrossfadeEquals(upperSnapshotZone.velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Snapshot upper-zone crossfade metadata");
        requireCrossfadeRuntimeEquals(lowerSnapshotZone.velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      "pad-a3-high",
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Snapshot lower-zone runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(upperSnapshotZone.velocityCrossfadeRuntime,
                                      25,
                                      127,
                                      "pad-a3-low",
                                      "",
                                      25,
                                      60,
                                      0,
                                      0,
                                      "Snapshot upper-zone runtime crossfade metadata");
        require(lowerSnapshotZone.tuningModulation.controllerNumber == 20
                    && lowerSnapshotZone.tuningModulation.amount == 4800.0
                    && lowerSnapshotZone.tuningModulation.curveIndex == 9
                    && lowerSnapshotZone.tuningModulation.curve[127] == 1.0f,
                "Snapshot should preserve tuning modulation metadata.");

        const auto serializedSnapshot = serializeImmutablePlaybackSnapshot(snapshotResult.snapshot);
        require(serializedSnapshot.find("\"velocityCrossfadeRuntime\"") != std::string::npos,
                "Snapshot serialization should emit runtime-ready crossfade metadata.");
        require(serializedSnapshot.find("\"tuningModulation\"") != std::string::npos,
                "Snapshot serialization should emit tuning modulation metadata.");

        PreparedPlaybackService preparedService;
        const auto preparedRequest = preparedService.requestBuild(snapshotResult, referenceStream);
        require(preparedRequest.accepted,
                "Prepared playback request should accept a snapshot with valid crossfade topology.");
        const auto preparedResult = preparedService.prepare(preparedRequest,
                                                            snapshotResult,
                                                            referenceStream);
        require(preparedResult.built,
                "Prepared playback should build successfully for valid crossfade topology.");
        require(preparedResult.activationEligible,
                "Prepared playback should remain activation eligible for valid crossfade topology.");

        const auto& lowerPreparedZone = findPreparedZone(preparedResult.prepared, "pad-a3-low");
        const auto& upperPreparedZone = findPreparedZone(preparedResult.prepared, "pad-a3-high");
        requireCrossfadeEquals(lowerPreparedZone.velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Prepared lower-zone crossfade metadata");
        requireCrossfadeEquals(upperPreparedZone.velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Prepared upper-zone crossfade metadata");
        requireCrossfadeRuntimeEquals(lowerPreparedZone.velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      "pad-a3-high",
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Prepared lower-zone runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(upperPreparedZone.velocityCrossfadeRuntime,
                                      25,
                                      127,
                                      "pad-a3-low",
                                      "",
                                      25,
                                      60,
                                      0,
                                      0,
                                      "Prepared upper-zone runtime crossfade metadata");
        require(lowerPreparedZone.tuningModulation.controllerNumber == 20
                    && lowerPreparedZone.tuningModulation.amount == 4800.0
                    && lowerPreparedZone.tuningModulation.curveIndex == 9
                    && lowerPreparedZone.tuningModulation.curve[127] == 1.0f,
                "Prepared playback should preserve tuning modulation metadata.");

        const auto serializedPrepared = serializeImmutablePreparedPlayback(preparedResult.prepared);
        require(serializedPrepared.find("\"velocityCrossfadeRuntime\"") != std::string::npos,
                "Prepared playback serialization should emit runtime-ready crossfade metadata.");
        require(serializedPrepared.find("\"tuningModulation\"") != std::string::npos,
                "Prepared playback serialization should emit tuning modulation metadata.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase3-crossfade-prepared-playback-tests";
        const auto compilePlan = buildCrossfadeCompilePlan(tempDirectory);
        auto compileResult = compileRuntimeInstrument(compilePlan);
        require(compileResult.compiled,
                "Compile plan with valid crossfade topology should compile successfully.");
        const auto streamWrite = writeCompiledStreamAssets(compileResult);
        require(streamWrite.written,
                "Compile plan with valid crossfade topology should write compiled stream assets successfully.");
        require(compileResult.instrument.zones.size() == 2,
                "Crossfade compile plan should preserve both compiled zones.");

        const auto& lowerInstrumentZone = findInstrumentZone(compileResult.instrument, "pad-a3-layer-1");
        const auto& upperInstrumentZone = findInstrumentZone(compileResult.instrument, "pad-a3-layer-2");
        requireCrossfadeEquals(lowerInstrumentZone.velocityCrossfade,
                               0,
                               0,
                               25,
                               60,
                               "Compiled lower-zone crossfade metadata");
        requireCrossfadeEquals(upperInstrumentZone.velocityCrossfade,
                               25,
                               60,
                               0,
                               0,
                               "Compiled upper-zone crossfade metadata");
        requireCrossfadeRuntimeEquals(lowerInstrumentZone.velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      "pad-a3-layer-2",
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Compiled lower-zone runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(upperInstrumentZone.velocityCrossfadeRuntime,
                                      25,
                                      127,
                                      "pad-a3-layer-1",
                                      "",
                                      25,
                                      60,
                                      0,
                                      0,
                                      "Compiled upper-zone runtime crossfade metadata");
        require(lowerInstrumentZone.tuningModulation.controllerNumber == 20
                    && lowerInstrumentZone.tuningModulation.amount == 4800.0
                    && lowerInstrumentZone.tuningModulation.curveIndex == 9
                    && lowerInstrumentZone.tuningModulation.curve[127] == 1.0,
                "Compiled instrument should preserve tuning modulation metadata.");
        require(lowerInstrumentZone.amplitudeModulation.controllerNumber == 93
                    && lowerInstrumentZone.amplitudeModulation.amount == 100.0
                    && lowerInstrumentZone.amplitudeEnvelope.holdModulation.controllerNumber == 27
                    && lowerInstrumentZone.amplitudeEnvelope.holdModulation.curveIndex == 8
                    && lowerInstrumentZone.amplitudeEnvelope.decayModulation.curveIndex == 8
                    && lowerInstrumentZone.amplitudeEnvelope.sustainModulation.curveIndex == 7,
                "Compiled instrument should preserve Crash 13-style volume and envelope modulation metadata.");
        require(compileResult.instrument.controllerDefaults.size() == 2
                    && compileResult.instrument.controllerDefaults[0] == RuntimeControllerDefault { 20, 63 }
                    && compileResult.instrument.controllerDefaults[1] == RuntimeControllerDefault { 93, 127 },
                "Compiled instrument should preserve controller defaults used by published playback.");

        const auto serializedInstrument = serializeRuntimeInstrumentManifest(compileResult.instrument,
                                                                             compilePlan.outputInstrumentPath);
        require(serializedInstrument.find("\"velocityCrossfadeRuntime\"") != std::string::npos,
                "Compiled instrument serialization should emit runtime-ready crossfade metadata.");
        require(serializedInstrument.find("\"tuningModulation\"") != std::string::npos,
                "Compiled instrument serialization should emit tuning modulation metadata.");

        writeTextFile(compilePlan.outputProjectPath,
                      serializeRuntimeProjectManifest(compileResult.project,
                                                     compilePlan.outputProjectPath));
        writeTextFile(compilePlan.outputStreamPath,
                      serializeCompiledStreamIndex(compileResult, compilePlan.outputStreamPath));
        writeTextFile(compilePlan.outputInstrumentPath, serializedInstrument);
        const auto roundTripInstrument = loadRuntimeInstrumentManifest(compilePlan.outputInstrumentPath);
        require(roundTripInstrument.loaded,
                "Compiled crossfade instrument should survive save/load round-tripping.");
        const auto& roundTripLowerZone = findInstrumentZone(roundTripInstrument.instrument, "pad-a3-layer-1");
        const auto& roundTripUpperZone = findInstrumentZone(roundTripInstrument.instrument, "pad-a3-layer-2");
        requireCrossfadeRuntimeEquals(roundTripLowerZone.velocityCrossfadeRuntime,
                                      1,
                                      60,
                                      "",
                                      "pad-a3-layer-2",
                                      0,
                                      0,
                                      25,
                                      60,
                                      "Round-tripped compiled lower-zone runtime crossfade metadata");
        requireCrossfadeRuntimeEquals(roundTripUpperZone.velocityCrossfadeRuntime,
                                      25,
                                      127,
                                      "pad-a3-layer-1",
                                      "",
                                      25,
                                      60,
                                      0,
                                      0,
                                      "Round-tripped compiled upper-zone runtime crossfade metadata");
        require(roundTripLowerZone.tuningModulation.controllerNumber == 20
                    && roundTripLowerZone.tuningModulation.amount == 4800.0
                    && roundTripLowerZone.tuningModulation.curveIndex == 9
                    && roundTripLowerZone.tuningModulation.curve[127] == 1.0,
                "Round-tripped compiled instrument should preserve tuning modulation metadata.");
        require(roundTripLowerZone.amplitudeModulation.controllerNumber == 93
                    && roundTripLowerZone.amplitudeModulation.amount == 100.0
                    && roundTripLowerZone.amplitudeEnvelope.holdModulation.controllerNumber == 27
                    && roundTripLowerZone.amplitudeEnvelope.holdModulation.curveIndex == 8
                    && roundTripLowerZone.amplitudeEnvelope.decayModulation.curveIndex == 8
                    && roundTripLowerZone.amplitudeEnvelope.sustainModulation.curveIndex == 7,
                "Round-tripped compiled instrument should preserve Crash 13-style modulation metadata.");
        require(roundTripInstrument.instrument.controllerDefaults.size() == 2
                    && roundTripInstrument.instrument.controllerDefaults[0] == RuntimeControllerDefault { 20, 63 }
                    && roundTripInstrument.instrument.controllerDefaults[1] == RuntimeControllerDefault { 93, 127 },
                "Round-tripped compiled instrument should preserve published controller defaults.");

        auto invalidCompilePlan = compilePlan;
        invalidCompilePlan.zones.pop_back();
        const auto invalidCompileResult = compileRuntimeInstrument(invalidCompilePlan);
        require(!invalidCompileResult.compiled,
                "Compile plan missing a crossfade partner should be rejected.");
        require(containsIssueFragment(invalidCompileResult.issues, "upper crossfade partner"),
                "Compile rejection should explain the missing upper crossfade partner.");

        std::cout << "Phase 3 crossfade prepared playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 crossfade prepared playback tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
