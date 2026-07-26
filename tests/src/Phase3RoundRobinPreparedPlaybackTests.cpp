#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/RuntimeCompiler.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"
#include "drs/engine/SamplerRenderModel.h"

#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
namespace fs = std::filesystem;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
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

bool hasFinding(const drs::engine::SamplerRenderModelBuildResult& result,
                const std::string& code)
{
    for (const auto& finding : result.findings)
    {
        if (finding.code == code)
            return true;
    }

    return false;
}

void requireRoundRobinEquals(const std::optional<drs::engine::RoundRobinDescriptor>& roundRobin,
                             const std::string& expectedPoolId,
                             int expectedSlotCount,
                             int expectedSlotIndex,
                             const std::string& context)
{
    require(roundRobin.has_value(), context + " should expose Round Robin metadata.");
    require(roundRobin->poolId == expectedPoolId, context + " poolId changed unexpectedly.");
    require(roundRobin->slotCount == expectedSlotCount, context + " slotCount changed unexpectedly.");
    require(roundRobin->slotIndex == expectedSlotIndex, context + " slotIndex changed unexpectedly.");
    require(roundRobin->mode == drs::engine::RoundRobinMode::sequential,
            context + " mode changed unexpectedly.");
}

fs::path getReferenceSamplePath()
{
    const auto referenceProjectPath = fs::path(drs::engine::getPhase1ReferenceProjectManifestPath());
    return (referenceProjectPath.parent_path()
            / ".." / ".." / ".." / ".." / "hise_project" / "Samples" / "DRS_Sine_A3.wav")
        .lexically_normal();
}

drs::engine::RuntimeProjectModel buildRoundRobinProject(const std::string& samplePath)
{
    using namespace drs::engine;

    RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 3;
    project.projectId = "phase3.round-robin.prepared-playback";
    project.displayName = "Phase 3 Round Robin Prepared Playback";
    project.contentRootPath = fs::path(samplePath).parent_path().generic_string();
    project.defaultInstrumentManifestPath =
        (fs::temp_directory_path() / "drs-phase3-round-robin-prepared-playback.drsinst").generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 2;
    project.authoring.selectedZoneId = "rr-zone-1";
    project.authoring.notes = { "Phase 3 Round Robin prepared playback fixture." };
    project.notes = { "Phase 3 Round Robin snapshot/prepared propagation fixture." };

    RuntimeProjectSampleSource sampleSource;
    sampleSource.id = "rr-sample";
    sampleSource.path = samplePath;
    sampleSource.role = "sustain";
    project.sampleSources.push_back(std::move(sampleSource));

    for (int slot = 1; slot <= 2; ++slot)
    {
        RuntimeProjectZoneDefinition zone;
        zone.id = "rr-zone-" + std::to_string(slot);
        zone.sampleSourceId = "rr-sample";
        zone.displayName = "RR Zone " + std::to_string(slot);
        zone.groupId = "rr-group";
        zone.articulationId = "sustain";
        zone.rootKey = 60;
        zone.keyLow = 60;
        zone.keyHigh = 60;
        zone.velocityLow = 1;
        zone.velocityHigh = 127;
        zone.roundRobinLength = 2;
        zone.roundRobinPosition = slot;

        RoundRobinDescriptor roundRobin;
        roundRobin.poolId = "rr-main";
        roundRobin.slotCount = 2;
        roundRobin.slotIndex = slot;
        roundRobin.mode = RoundRobinMode::sequential;
        zone.roundRobin = roundRobin;

        project.authoring.zones.push_back(std::move(zone));
    }

    return project;
}

drs::engine::PlaybackActivationPayloadPtr makePayload(
    const drs::engine::PlaybackSnapshotBuildResult& snapshotResult,
    const drs::engine::PreparedPlaybackBuildResult& preparedResult)
{
    auto payload = std::make_shared<drs::engine::PlaybackActivationPayload>();
    payload->lane = drs::engine::PlaybackActivationLane::preview;
    payload->revision = snapshotResult.snapshot.draftRevision;
    payload->snapshotBuildId = snapshotResult.buildId;
    payload->preparedBuildId = preparedResult.buildId;
    payload->lifecycleState = drs::engine::PlaybackSnapshotLifecycleState::ready;
    payload->activationEligible = true;
    payload->snapshotContentDigest = snapshotResult.snapshot.contentDigest;
    payload->preparedContentDigest = preparedResult.prepared.preparedContentDigest;
    payload->routeDigest = preparedResult.prepared.routeDigest;
    payload->sourceProvenanceDigest = preparedResult.prepared.sourceProvenanceDigest;
    payload->macroSchemaDigest = preparedResult.prepared.macroSchemaDigest;
    payload->retainedPreparedBytes = preparedResult.metrics.preparedBytes;
    payload->snapshot = std::make_shared<const drs::engine::ImmutablePlaybackSnapshot>(snapshotResult.snapshot);
    payload->prepared = std::make_shared<const drs::engine::ImmutablePreparedPlayback>(preparedResult.prepared);
    return payload;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto samplePath = getReferenceSamplePath();
        const auto sampleImport = importSampleFile(samplePath.generic_string());
        require(sampleImport.imported,
                "Reference sine sample must import before Round Robin prepared-playback tests run.");

        const auto project = buildRoundRobinProject(samplePath.generic_string());

        PlaybackSnapshotBuilder snapshotBuilder;
        const auto snapshotRequest = snapshotBuilder.requestBuild(4, true);
        require(snapshotRequest.accepted, "Round Robin snapshot request should be accepted.");
        const auto snapshotResult = snapshotBuilder.buildSnapshot(snapshotRequest, project);
        require(snapshotResult.built, "Round Robin project should build a snapshot.");
        require(snapshotResult.activationEligible, "Round Robin snapshot should remain activation-eligible.");
        require(snapshotResult.snapshot.zones.size() == 2,
                "Round Robin snapshot should preserve both RR zones.");

        const auto& snapshotZone1 = findSnapshotZone(snapshotResult.snapshot, "rr-zone-1");
        const auto& snapshotZone2 = findSnapshotZone(snapshotResult.snapshot, "rr-zone-2");
        requireRoundRobinEquals(snapshotZone1.roundRobin, "rr-main", 2, 1, "Snapshot zone 1");
        requireRoundRobinEquals(snapshotZone2.roundRobin, "rr-main", 2, 2, "Snapshot zone 2");

        const auto serializedSnapshot = serializeImmutablePlaybackSnapshot(snapshotResult.snapshot);
        require(serializedSnapshot.find("\"roundRobin\"") != std::string::npos,
                "Snapshot serialization should emit explicit Round Robin descriptors.");
        require(serializedSnapshot.find("\"roundRobinLength\"") == std::string::npos,
                "Snapshot serialization should stop emitting legacy RR scalar fields.");
        require(serializedSnapshot.find("\"roundRobinPosition\"") == std::string::npos,
                "Snapshot serialization should stop emitting legacy RR scalar fields.");

        PreparedPlaybackService preparedService;
        const RuntimeStreamLoadResult streamResult;
        const auto preparedRequest = preparedService.requestBuild(snapshotResult);
        require(preparedRequest.accepted,
                "Prepared playback request should accept a valid Round Robin snapshot.");
        const auto preparedResult = preparedService.prepare(preparedRequest, snapshotResult, streamResult);
        require(preparedResult.built, "Prepared playback should build from a Round Robin snapshot.");
        require(preparedResult.activationEligible,
                "Prepared playback should remain activation-eligible for Round Robin fixtures.");
        require(preparedResult.prepared.zones.size() == 2,
                "Prepared playback should preserve both RR routes.");

        const auto& preparedZone1 = findPreparedZone(preparedResult.prepared, "rr-zone-1");
        const auto& preparedZone2 = findPreparedZone(preparedResult.prepared, "rr-zone-2");
        requireRoundRobinEquals(preparedZone1.roundRobin, "rr-main", 2, 1, "Prepared zone 1");
        requireRoundRobinEquals(preparedZone2.roundRobin, "rr-main", 2, 2, "Prepared zone 2");

        const auto serializedPrepared = serializeImmutablePreparedPlayback(preparedResult.prepared);
        require(serializedPrepared.find("\"roundRobin\"") != std::string::npos,
                "Prepared playback serialization should emit explicit Round Robin descriptors.");
        require(serializedPrepared.find("\"roundRobinLength\"") == std::string::npos,
                "Prepared playback serialization should stop emitting legacy RR scalar fields.");
        require(serializedPrepared.find("\"roundRobinPosition\"") == std::string::npos,
                "Prepared playback serialization should stop emitting legacy RR scalar fields.");
        require(preparedResult.prepared.routeDigest
                    == computePreparedPlaybackRouteDigest(snapshotResult.snapshot, preparedResult.prepared),
                "Prepared playback route digests should remain deterministic after RR propagation.");

        const auto renderModel = buildSamplerRenderModel(makePayload(snapshotResult, preparedResult));
        require(renderModel.built, "Round Robin payload should build a render model.");
        require(renderModel.model->getRoutes().size() == 2,
                "Render model should preserve both RR routes.");
        requireRoundRobinEquals(renderModel.model->getRoutes()[0].roundRobin,
                                "rr-main",
                                2,
                                1,
                                "Render route 0");
        requireRoundRobinEquals(renderModel.model->getRoutes()[1].roundRobin,
                                "rr-main",
                                2,
                                2,
                                "Render route 1");

        auto invalidPrepared = preparedResult.prepared;
        invalidPrepared.zones[1].roundRobin->poolId = "rr-other";
        const auto invalidPayload = [&]()
        {
            auto mutablePayload =
                std::const_pointer_cast<PlaybackActivationPayload>(makePayload(snapshotResult, preparedResult));
            mutablePayload->prepared = std::make_shared<const ImmutablePreparedPlayback>(invalidPrepared);
            return std::static_pointer_cast<const PlaybackActivationPayload>(mutablePayload);
        }();
        const auto invalidRenderModel = buildSamplerRenderModel(invalidPayload);
        require(!invalidRenderModel.built && hasFinding(invalidRenderModel, "render-model-route-topology-mismatch"),
                "Render model validation should reject RR topology mismatches between snapshot and prepared routes.");

        RuntimeCompilePlan compilePlan;
        compilePlan.outputProjectPath =
            (fs::temp_directory_path() / "drs-phase3-round-robin-compile.drsproj").generic_string();
        compilePlan.outputInstrumentPath =
            (fs::temp_directory_path() / "drs-phase3-round-robin-compile.drinst").generic_string();
        compilePlan.outputStreamPath =
            (fs::temp_directory_path() / "drs-phase3-round-robin-compile.drstrm").generic_string();
        compilePlan.projectId = "phase3.round-robin.compile";
        compilePlan.projectDisplayName = "Phase 3 Round Robin Compile";
        compilePlan.contentRootPath = samplePath.parent_path().parent_path().generic_string();
        compilePlan.instrumentId = "phase3.round-robin.instrument";
        compilePlan.instrumentDisplayName = "Phase 3 Round Robin Instrument";
        compilePlan.defaultLoadProfile = "balanced";
        compilePlan.pageSizeBytes = 65536;

        RuntimeCompileSourceDefinition source;
        source.id = "rr-sample";
        source.sourcePath = samplePath.generic_string();
        source.role = "sustain";
        source.metadata = sampleImport.sample.metadata;
        compilePlan.sampleSources.push_back(std::move(source));

        RuntimeArticulationDefinition articulation;
        articulation.id = "sustain";
        articulation.name = "Sustain";
        articulation.isDefault = true;
        compilePlan.articulations.push_back(std::move(articulation));

        RuntimeGroupDefinition group;
        group.id = "rr-group";
        group.name = "RR Group";
        group.articulationIds = { "sustain" };
        compilePlan.groups.push_back(std::move(group));

        for (int slot = 1; slot <= 2; ++slot)
        {
            RuntimeCompileZoneDefinition zone;
            zone.id = "compile-rr-zone-" + std::to_string(slot);
            zone.sourceId = "rr-sample";
            zone.groupId = "rr-group";
            zone.articulationId = "sustain";
            zone.rootKey = 60;
            zone.keyLow = 60;
            zone.keyHigh = 60;
            zone.velocityLow = 1;
            zone.velocityHigh = 127;
            zone.roundRobinLength = 2;
            zone.roundRobinPosition = slot;

            RoundRobinDescriptor roundRobin;
            roundRobin.poolId = "compile-rr-main";
            roundRobin.slotCount = 2;
            roundRobin.slotIndex = slot;
            roundRobin.mode = RoundRobinMode::sequential;
            zone.roundRobin = roundRobin;

            compilePlan.zones.push_back(std::move(zone));
        }

        const auto compileResult = compileRuntimeInstrument(compilePlan);
        require(compileResult.compiled, "Round Robin compile plan should compile successfully.");
        require(compileResult.instrument.schemaVersion == 2,
                "Compiled Round Robin instruments should use schemaVersion 2.");
        requireRoundRobinEquals(findInstrumentZone(compileResult.instrument, "compile-rr-zone-1").roundRobin,
                                "compile-rr-main",
                                2,
                                1,
                                "Compiled instrument zone 1");
        requireRoundRobinEquals(findInstrumentZone(compileResult.instrument, "compile-rr-zone-2").roundRobin,
                                "compile-rr-main",
                                2,
                                2,
                                "Compiled instrument zone 2");

        const auto serializedInstrument = serializeRuntimeInstrumentManifest(
            compileResult.instrument, compilePlan.outputInstrumentPath);
        require(serializedInstrument.find("\"roundRobin\"") != std::string::npos,
                "Compiled instrument serialization should emit explicit Round Robin descriptors.");
        require(serializedInstrument.find("\"roundRobinLength\"") == std::string::npos,
                "Compiled instrument serialization should stop emitting legacy RR scalar fields.");
        require(serializedInstrument.find("\"roundRobinPosition\"") == std::string::npos,
                "Compiled instrument serialization should stop emitting legacy RR scalar fields.");

        std::cout << "Phase 3 Round Robin prepared playback tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 Round Robin prepared playback tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
