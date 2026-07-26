#include "drs/engine/RuntimeLoader.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

drs::engine::RuntimeProjectZoneDefinition makeLegacyRoundRobinZone(const std::string& id,
                                                                   const std::string& sampleSourceId,
                                                                   int velocityLow,
                                                                   int velocityHigh,
                                                                   int slotIndex)
{
    drs::engine::RuntimeProjectZoneDefinition zone;
    zone.id = id;
    zone.sampleSourceId = sampleSourceId;
    zone.displayName = id;
    zone.groupId = "main";
    zone.articulationId = "sustain";
    zone.rootKey = 60;
    zone.keyLow = 60;
    zone.keyHigh = 60;
    zone.velocityLow = velocityLow;
    zone.velocityHigh = velocityHigh;
    zone.roundRobinLength = 2;
    zone.roundRobinPosition = slotIndex;
    return zone;
}

drs::engine::RuntimeProjectModel makeLegacyProject(const fs::path& root)
{
    using namespace drs::engine;

    const auto contentRoot = root / "content";
    const auto samplesRoot = contentRoot / "Samples";
    const auto defaultInstrument = root / "legacy-project.drinst";

    fs::create_directories(samplesRoot);
    writeTextFile(defaultInstrument, "legacy project instrument placeholder");
    writeTextFile(samplesRoot / "rr-slot-1-low.flac", "sample");
    writeTextFile(samplesRoot / "rr-slot-1-high.flac", "sample");
    writeTextFile(samplesRoot / "rr-slot-2-low.flac", "sample");
    writeTextFile(samplesRoot / "rr-slot-2-high.flac", "sample");

    RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 2;
    project.projectId = "phase3.round-robin.legacy-project";
    project.displayName = "Phase 3 Round Robin Legacy Project";
    project.contentRootPath = contentRoot.generic_string();
    project.defaultInstrumentManifestPath = defaultInstrument.generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 1;
    project.authoring.notes = { "Legacy Phase 2 Round Robin persistence fixture." };
    project.notes = { "Legacy project fixture for Phase 3 Round Robin schema migration." };

    project.sampleSources = {
        { "slot-1-low", (samplesRoot / "rr-slot-1-low.flac").generic_string(), "sustain" },
        { "slot-1-high", (samplesRoot / "rr-slot-1-high.flac").generic_string(), "sustain" },
        { "slot-2-low", (samplesRoot / "rr-slot-2-low.flac").generic_string(), "sustain" },
        { "slot-2-high", (samplesRoot / "rr-slot-2-high.flac").generic_string(), "sustain" }
    };

    project.authoring.zones = {
        makeLegacyRoundRobinZone("rr-slot-1-low", "slot-1-low", 1, 63, 1),
        makeLegacyRoundRobinZone("rr-slot-1-high", "slot-1-high", 64, 127, 1),
        makeLegacyRoundRobinZone("rr-slot-2-low", "slot-2-low", 1, 63, 2),
        makeLegacyRoundRobinZone("rr-slot-2-high", "slot-2-high", 64, 127, 2)
    };

    return project;
}

void requireRoundRobinEquals(const std::optional<drs::engine::RoundRobinDescriptor>& roundRobin,
                             const std::string& expectedPoolId,
                             int expectedSlotCount,
                             int expectedSlotIndex,
                             const std::string& context)
{
    require(roundRobin.has_value(), context + " should have an explicit Round Robin descriptor.");
    require(roundRobin->poolId == expectedPoolId, context + " poolId changed unexpectedly.");
    require(roundRobin->slotCount == expectedSlotCount, context + " slotCount changed unexpectedly.");
    require(roundRobin->slotIndex == expectedSlotIndex, context + " slotIndex changed unexpectedly.");
    require(roundRobin->mode == drs::engine::RoundRobinMode::sequential,
            context + " mode changed unexpectedly.");
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto tempRoot = fs::temp_directory_path() / "drs-phase3-round-robin-schema-persistence-tests";
        fs::create_directories(tempRoot);

        const auto legacyProject = makeLegacyProject(tempRoot);
        const auto migratedProject = migrateRuntimeProjectToPhase3RoundRobinSchema(legacyProject);
        require(migratedProject.valid, "Legacy Phase 2 project should migrate into the Phase 3 Round Robin schema.");
        require(migratedProject.migrated, "Legacy Phase 2 project migration should report an applied migration.");
        require(migratedProject.project.schemaVersion == 3,
                "Migrated project should now use schemaVersion 3.");
        require(migratedProject.project.authoring.schemaVersion == 2,
                "Migrated project should now use authoring schemaVersion 2.");

        const auto expectedPoolId =
            migratedProject.project.authoring.zones.front().roundRobin->poolId;
        require(!expectedPoolId.empty(), "Migrated project should synthesize a non-empty Round Robin poolId.");
        requireRoundRobinEquals(migratedProject.project.authoring.zones[0].roundRobin,
                                expectedPoolId,
                                2,
                                1,
                                "Migrated project zone 0");
        requireRoundRobinEquals(migratedProject.project.authoring.zones[1].roundRobin,
                                expectedPoolId,
                                2,
                                1,
                                "Migrated project zone 1");
        requireRoundRobinEquals(migratedProject.project.authoring.zones[2].roundRobin,
                                expectedPoolId,
                                2,
                                2,
                                "Migrated project zone 2");
        requireRoundRobinEquals(migratedProject.project.authoring.zones[3].roundRobin,
                                expectedPoolId,
                                2,
                                2,
                                "Migrated project zone 3");

        const auto projectPath = tempRoot / "round-robin-migrated.drsproj";
        const auto serializedProject = serializeRuntimeProjectManifest(
            migratedProject.project, projectPath.generic_string());
        require(contains(serializedProject, "\"schemaVersion\": 3"),
                "Migrated project serialization should emit schemaVersion 3.");
        require(contains(serializedProject, "\"roundRobin\""),
                "Migrated project serialization should emit explicit Round Robin objects.");
        require(!contains(serializedProject, "\"roundRobinLength\""),
                "Migrated project serialization should stop emitting legacy roundRobinLength scalars.");
        require(!contains(serializedProject, "\"roundRobinPosition\""),
                "Migrated project serialization should stop emitting legacy roundRobinPosition scalars.");
        writeTextFile(projectPath, serializedProject);

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded, "SchemaVersion 3 Round Robin project should load successfully.");
        require(roundTripProject.project.authoring.zones.size() == 4,
                "Round-tripped project should preserve all Round Robin zones.");
        requireRoundRobinEquals(roundTripProject.project.authoring.zones[3].roundRobin,
                                expectedPoolId,
                                2,
                                2,
                                "Round-tripped project zone 3");
        require(roundTripProject.project.authoring.zones[3].roundRobinLength == 2
                    && roundTripProject.project.authoring.zones[3].roundRobinPosition == 2,
                "Round-tripped project should preserve derived legacy RR scalars for runtime code.");

        const auto builtInstrument = drs::app::buildInstrumentManifestForProject(
            migratedProject.project, juce::File(projectPath.generic_string()));
        require(builtInstrument.schemaVersion == 2,
                "Project-to-instrument conversion should now emit instrument schemaVersion 2.");
        require(builtInstrument.zones.size() == migratedProject.project.authoring.zones.size(),
                "Project-to-instrument conversion should preserve every authored RR zone.");
        requireRoundRobinEquals(builtInstrument.zones[0].roundRobin,
                                expectedPoolId,
                                2,
                                1,
                                "Built instrument zone 0");
        requireRoundRobinEquals(builtInstrument.zones[3].roundRobin,
                                expectedPoolId,
                                2,
                                2,
                                "Built instrument zone 3");

        writeTextFile(fs::path(builtInstrument.compiledStreamAssetPath), "stream placeholder");
        const auto instrumentPath = tempRoot / "round-robin-migrated.drinst";
        const auto serializedInstrument = serializeRuntimeInstrumentManifest(
            builtInstrument, instrumentPath.generic_string());
        require(contains(serializedInstrument, "\"schemaVersion\": 2"),
                "Instrument serialization should emit schemaVersion 2.");
        require(contains(serializedInstrument, "\"roundRobin\""),
                "Instrument serialization should emit explicit Round Robin objects.");
        require(!contains(serializedInstrument, "\"roundRobinLength\""),
                "Instrument serialization should stop emitting legacy roundRobinLength scalars.");
        require(!contains(serializedInstrument, "\"roundRobinPosition\""),
                "Instrument serialization should stop emitting legacy roundRobinPosition scalars.");
        writeTextFile(instrumentPath, serializedInstrument);

        const auto roundTripInstrument = loadRuntimeInstrumentManifest(instrumentPath.generic_string());
        require(roundTripInstrument.loaded, "SchemaVersion 2 Round Robin instrument should load successfully.");
        requireRoundRobinEquals(roundTripInstrument.instrument.zones[1].roundRobin,
                                expectedPoolId,
                                2,
                                1,
                                "Round-tripped instrument zone 1");

        auto legacyInstrument = builtInstrument;
        legacyInstrument.schemaVersion = 1;
        const auto legacyInstrumentPath = tempRoot / "round-robin-legacy.drinst";
        const auto serializedLegacyInstrument = serializeRuntimeInstrumentManifest(
            legacyInstrument, legacyInstrumentPath.generic_string());
        require(contains(serializedLegacyInstrument, "\"roundRobinLength\""),
                "Legacy instrument serialization should continue to emit roundRobinLength.");
        require(!contains(serializedLegacyInstrument, "\"roundRobin\":"),
                "Legacy instrument serialization should not emit explicit Round Robin objects.");
        writeTextFile(legacyInstrumentPath, serializedLegacyInstrument);

        const auto legacyInstrumentLoad = loadRuntimeInstrumentManifest(legacyInstrumentPath.generic_string());
        require(legacyInstrumentLoad.loaded, "Legacy schemaVersion 1 instrument should still load successfully.");
        requireRoundRobinEquals(legacyInstrumentLoad.instrument.zones[2].roundRobin,
                                expectedPoolId,
                                2,
                                2,
                                "Legacy round-tripped instrument zone 2");

        const auto precedenceProjectPath = tempRoot / "precedence-source.drsproj";
        const auto precedenceStreamPath = tempRoot / "precedence-source.drstrm";
        const auto precedenceSamplePath = tempRoot / "precedence-source.flac";
        const auto precedenceManifestPath = tempRoot / "round-robin-precedence.drinst";
        writeTextFile(precedenceProjectPath, "precedence source project placeholder");
        writeTextFile(precedenceStreamPath, "precedence stream placeholder");
        writeTextFile(precedenceSamplePath, "precedence sample placeholder");
        writeTextFile(precedenceManifestPath,
                      "{\n"
                      "  \"schemaName\": \"drs.instrument\",\n"
                      "  \"schemaVersion\": 1,\n"
                      "  \"instrumentId\": \"phase3.round-robin.precedence\",\n"
                      "  \"displayName\": \"Phase 3 Round Robin Precedence\",\n"
                      "  \"sourceProject\": \"precedence-source.drsproj\",\n"
                      "  \"compiledStreamAsset\": \"precedence-source.drstrm\",\n"
                      "  \"defaultLoadProfile\": \"balanced\",\n"
                      "  \"macros\": [],\n"
                      "  \"articulations\": [\n"
                      "    { \"id\": \"sustain\", \"name\": \"Sustain\", \"isDefault\": true }\n"
                      "  ],\n"
                      "  \"groups\": [\n"
                      "    { \"id\": \"main\", \"name\": \"Main\", \"articulationIds\": [\"sustain\"] }\n"
                      "  ],\n"
                      "  \"zones\": [\n"
                      "    {\n"
                      "      \"id\": \"precedence-zone\",\n"
                      "      \"groupId\": \"main\",\n"
                      "      \"articulationId\": \"sustain\",\n"
                      "      \"samplePath\": \"precedence-source.flac\",\n"
                      "      \"streamAssetPath\": \"precedence-source.drstrm\",\n"
                      "      \"rootKey\": 60,\n"
                      "      \"keyLow\": 60,\n"
                      "      \"keyHigh\": 60,\n"
                      "      \"velocityLow\": 1,\n"
                      "      \"velocityHigh\": 127,\n"
                      "      \"streamOffsetBytes\": 0,\n"
                      "      \"prefetchBytes\": 16384,\n"
                      "      \"releaseSeconds\": 0.0,\n"
                      "      \"roundRobin\": {\n"
                      "        \"poolId\": \"precedence-pool\",\n"
                      "        \"slotCount\": 3,\n"
                      "        \"slotIndex\": 2,\n"
                      "        \"mode\": \"sequential\"\n"
                      "      },\n"
                      "      \"roundRobinLength\": 9,\n"
                      "      \"roundRobinPosition\": 9\n"
                      "    }\n"
                      "  ],\n"
                      "  \"validationNotes\": [\"Legacy precedence fixture.\"]\n"
                      "}\n");

        const auto precedenceLoad = loadRuntimeInstrumentManifest(precedenceManifestPath.generic_string());
        require(precedenceLoad.loaded,
                "Legacy manifest containing both RR representations should still load when the explicit object is valid.");
        requireRoundRobinEquals(precedenceLoad.instrument.zones.front().roundRobin,
                                "precedence-pool",
                                3,
                                2,
                                "Legacy precedence zone");
        require(precedenceLoad.instrument.zones.front().roundRobinLength == 3
                    && precedenceLoad.instrument.zones.front().roundRobinPosition == 2,
                "Explicit Round Robin objects should win over legacy RR scalars when both appear.");

        std::cout << "Phase 3 Round Robin schema persistence tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 3 Round Robin schema persistence tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
