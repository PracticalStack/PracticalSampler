#include "drs/engine/RuntimeLoader.h"
#include "shared/ProjectStorage.h"

#include <filesystem>
#include <fstream>
#include <iostream>
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

bool contains(const std::string& text, const std::string& needle)
{
    return text.find(needle) != std::string::npos;
}

drs::engine::RuntimeProjectZoneDefinition makeZone(const std::string& id,
                                                   const std::string& sampleSourceId,
                                                   const std::string& groupId,
                                                   const std::string& articulationId,
                                                   int keyLow,
                                                   int keyHigh)
{
    drs::engine::RuntimeProjectZoneDefinition zone;
    zone.id = id;
    zone.sampleSourceId = sampleSourceId;
    zone.displayName = id;
    zone.groupId = groupId;
    zone.articulationId = articulationId;
    zone.rootKey = keyLow;
    zone.keyLow = keyLow;
    zone.keyHigh = keyHigh;
    zone.velocityLow = 1;
    zone.velocityHigh = 127;
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
    writeTextFile(samplesRoot / "pad-low.wav", "sample");
    writeTextFile(samplesRoot / "pad-high.wav", "sample");
    writeTextFile(samplesRoot / "lead.wav", "sample");

    RuntimeProjectModel project;
    project.schemaName = "drs.project";
    project.schemaVersion = 3;
    project.projectId = "phase1.zone-groups.legacy-project";
    project.displayName = "Phase 1 Zone Groups Legacy Project";
    project.contentRootPath = contentRoot.generic_string();
    project.defaultInstrumentManifestPath = defaultInstrument.generic_string();
    project.authoring.schemaName = "drs.authoring";
    project.authoring.schemaVersion = 2;
    project.authoring.selectedZoneId = "lead-zone";
    project.authoring.selectedPerformanceBankId = "";
    project.authoring.notes = { "Legacy pre-group persistence fixture." };
    project.notes = { "Legacy project fixture for Zone Groups schema migration." };

    project.sampleSources = {
        { "pad-low-source", (samplesRoot / "pad-low.wav").generic_string(), "pad" },
        { "pad-high-source", (samplesRoot / "pad-high.wav").generic_string(), "pad" },
        { "lead-source", (samplesRoot / "lead.wav").generic_string(), "lead" }
    };

    project.authoring.zones = {
        makeZone("pad-low-zone", "pad-low-source", "pad-core", "sustain", 36, 59),
        makeZone("pad-high-zone", "pad-high-source", "pad-core", "sustain", 60, 84),
        makeZone("lead-zone", "lead-source", "lead-core", "lead", 60, 96)
    };

    return project;
}
} // namespace

int main()
{
    using namespace drs::engine;

    try
    {
        const auto tempRoot = fs::temp_directory_path() / "drs-phase1-zone-groups-schema-persistence-tests";
        fs::create_directories(tempRoot);

        const auto legacyProject = makeLegacyProject(tempRoot);
        const auto migratedProject = migrateRuntimeProjectToZoneGroupsSchema(legacyProject);
        require(migratedProject.valid, "Legacy project should migrate into the explicit Zone Groups schema.");
        require(migratedProject.migrated, "Legacy project zone-group migration should report an applied migration.");
        require(migratedProject.project.schemaVersion == 4,
                "Migrated project should now use schemaVersion 4.");
        require(migratedProject.project.authoring.schemaVersion == 3,
                "Migrated project should now use authoring schemaVersion 3.");
        require(migratedProject.project.authoring.groups.size() == 2,
                "Migrated project should synthesize one authored group per distinct legacy groupId.");
        require(migratedProject.project.authoring.groups[0].id == "pad-core"
                    && migratedProject.project.authoring.groups[0].auditionAnchorZoneId == "pad-low-zone",
                "Migrated project should synthesize deterministic authored-group order and audition anchors.");
        require(migratedProject.project.authoring.groups[1].id == "lead-core"
                    && migratedProject.project.authoring.selectedGroupId == "lead-core",
                "Migrated project should preserve selectedGroupId from the selected zone's membership.");

        const auto projectPath = tempRoot / "zone-groups-migrated.drsproj";
        const auto serializedProject = serializeRuntimeProjectManifest(
            migratedProject.project, projectPath.generic_string());
        require(contains(serializedProject, "\"schemaVersion\": 4"),
                "Migrated project serialization should emit schemaVersion 4.");
        require(contains(serializedProject, "\"selectedGroupId\": \"lead-core\""),
                "Migrated project serialization should emit selectedGroupId.");
        require(contains(serializedProject, "\"groups\""),
                "Migrated project serialization should emit explicit groups.");
        writeTextFile(projectPath, serializedProject);

        const auto roundTripProject = loadRuntimeProjectManifest(projectPath.generic_string());
        require(roundTripProject.loaded, "SchemaVersion 4 Zone Groups project should load successfully.");
        require(roundTripProject.project.authoring.groups.size() == 2,
                "Round-tripped Zone Groups project should preserve explicit groups.");
        require(roundTripProject.project.authoring.groups[0].displayName == "pad-core",
                "Round-tripped Zone Groups project should preserve group display names.");
        require(roundTripProject.project.authoring.selectedGroupId == "lead-core",
                "Round-tripped Zone Groups project should preserve selectedGroupId.");

        const auto builtInstrument = drs::app::buildInstrumentManifestForProject(
            migratedProject.project, juce::File(projectPath.generic_string()));
        require(builtInstrument.groups.size() == migratedProject.project.authoring.groups.size(),
                "Project-to-instrument conversion should preserve the authored group inventory.");
        require(builtInstrument.groups[0].name == "pad-core"
                    && builtInstrument.groups[1].name == "lead-core",
                "Project-to-instrument conversion should use authored group display names.");

        std::cout << "Phase 1 Zone Groups schema persistence tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 Zone Groups schema persistence tests failed: "
                  << exception.what() << std::endl;
        return 1;
    }
}
