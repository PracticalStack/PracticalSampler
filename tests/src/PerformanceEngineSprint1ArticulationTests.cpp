#include "drs/engine/AuthoringSession.h"
#include "drs/engine/RuntimeLoader.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
using namespace drs::engine;

void require(const bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool hasIssue(const RuntimeProjectValidationResult& validation, const std::string& token)
{
    return std::any_of(validation.issues.begin(), validation.issues.end(), [&](const auto& issue)
    {
        return issue.find(token) != std::string::npos;
    });
}

const RuntimeProjectArticulationDefinition& findArticulation(const RuntimeProjectModel& project,
                                                             const std::string& id)
{
    const auto iterator = std::find_if(project.authoring.articulations.begin(),
                                       project.authoring.articulations.end(),
                                       [&](const auto& articulation) { return articulation.id == id; });
    require(iterator != project.authoring.articulations.end(), "Missing articulation '" + id + "'.");
    return *iterator;
}

RuntimeProjectModel loadLegacyFixture()
{
    const auto loaded = loadRuntimeProjectManifest(DRS_PERFORMANCE_ENGINE_S1_LEGACY_FIXTURE_PATH);
    require(loaded.loaded, "The Sprint 1 legacy fixture must load before migration.");
    auto project = loaded.project;
    require(project.schemaVersion == 5 && project.authoring.schemaVersion == 4,
            "The control-law fixture must remain the schema-5/authoring-4 migration source.");
    require(project.authoring.zones.size() >= 2, "The migration fixture needs at least two zones.");
    project.authoring.zones[0].articulationId = "sustain";
    project.authoring.zones[1].articulationId = "staccato";
    for (std::size_t index = 2; index < project.authoring.zones.size(); ++index)
        project.authoring.zones[index].articulationId = "sustain";
    for (auto& zone : project.authoring.zones)
        zone.keyLow = 36;
    project.authoring.selectedZoneId = project.authoring.zones[1].id;
    return project;
}

void verifyMigrationAndRoundTrip()
{
    const auto migrated = migrateRuntimeProjectToPerformanceArticulationSchema(loadLegacyFixture());
    require(migrated.valid && migrated.migrated,
            "A valid schema-5 project must migrate to the explicit articulation schema.");
    require(migrated.project.schemaVersion == 6 && migrated.project.authoring.schemaVersion == 5,
            "Migration must advance project/authoring schemas to 6/5.");
    require(migrated.project.authoring.articulations.size() == 2,
            "Migration must synthesize articulations in first-zone appearance order.");
    require(migrated.project.authoring.articulations[0].id == "sustain"
                && migrated.project.authoring.articulations[0].displayOrder == 0
                && migrated.project.authoring.articulations[1].id == "staccato"
                && migrated.project.authoring.articulations[1].displayOrder == 1,
            "Migration must retain stable IDs and deterministic authored order.");
    require(findArticulation(migrated.project, "staccato").isDefault,
            "The selected-zone articulation must become the deterministic migrated default.");

    const auto serialized = serializeRuntimeProjectManifest(
        migrated.project, "performance-engine-sprint1-roundtrip.drsproj");
    const auto parsed = parseRuntimeProjectManifest(
        serialized, "performance-engine-sprint1-roundtrip.drsproj", false);
    require(parsed.loaded, "Schema-6 project serialization must parse.");
    require(parsed.project.authoring.articulations.size() == 2
                && findArticulation(parsed.project, "staccato").isDefault,
            "Schema-6 parsing must retain explicit articulation identity and default state.");
    require(serializeRuntimeProjectManifest(parsed.project, "performance-engine-sprint1-roundtrip.drsproj")
                == serialized,
            "Migrated schema-6 projects must round-trip byte deterministically.");
}

void verifyValidation()
{
    const auto migrated = migrateRuntimeProjectToPerformanceArticulationSchema(loadLegacyFixture());
    require(migrated.valid, "Validation fixture migration must succeed.");

    auto duplicate = migrated.project;
    duplicate.authoring.articulations.push_back(duplicate.authoring.articulations.front());
    auto validation = validateRuntimeProjectModel(duplicate);
    require(!validation.valid && hasIssue(validation, "articulation ids must be unique"),
            "Duplicate articulation IDs must be rejected.");

    auto noDefault = migrated.project;
    for (auto& articulation : noDefault.authoring.articulations)
        articulation.isDefault = false;
    validation = validateRuntimeProjectModel(noDefault);
    require(!validation.valid && hasIssue(validation, "exactly one default articulation"),
            "A schema-6 project must have exactly one default articulation.");

    auto unknownZoneReference = migrated.project;
    unknownZoneReference.authoring.zones.front().articulationId = "missing";
    validation = validateRuntimeProjectModel(unknownZoneReference);
    require(!validation.valid && hasIssue(validation, "unknown articulationId 'missing'"),
            "Zones must not reference articulations absent from the authored list.");
}

void verifyAuthoringTransactions()
{
    const auto migrated = migrateRuntimeProjectToPerformanceArticulationSchema(loadLegacyFixture());
    require(migrated.valid, "Transaction fixture migration must succeed.");
    AuthoringSession session(migrated.project);

    RuntimeProjectArticulationDefinition pizzicato;
    pizzicato.id = "pizzicato";
    pizzicato.displayName = "Pizzicato";
    pizzicato.activation = RuntimeProjectArticulationActivationDefinition {
        PerformanceEventKind::noteOn, 12, ArticulationActivationMode::latch, true };
    require(session.createArticulation(pizzicato, "Create pizzicato").applied,
            "Creating a first-class articulation must be undoable.");
    auto edited = findArticulation(session.getProject(), "pizzicato");
    edited.displayName = "Pizz.";
    require(session.updateArticulation(2, edited, "Rename pizzicato").applied,
            "Renaming a first-class articulation must be undoable.");
    require(session.moveArticulation(2, -1, "Move pizzicato").applied,
            "Reordering a first-class articulation must be undoable.");
    require(session.reassignZonesToArticulation({ session.getProject().authoring.zones.front().id },
                                                "pizzicato", "Assign pizzicato").applied,
            "Bulk zone articulation reassignment must be transactional.");
    require(session.deleteArticulation("pizzicato", "sustain", "Delete pizzicato").applied,
            "Deletion with reassignment must preserve every zone reference.");
    require(session.getProject().authoring.zones.front().articulationId == "sustain"
                && !std::any_of(session.getProject().authoring.articulations.begin(),
                                session.getProject().authoring.articulations.end(),
                                [](const auto& articulation) { return articulation.id == "pizzicato"; }),
            "Deletion must reassign zones before removing the articulation.");
    require(session.undo().applied && findArticulation(session.getProject(), "pizzicato").id == "pizzicato",
            "Undo must restore deleted articulation identity and references.");
    require(session.redo().applied
                && session.getProject().authoring.zones.front().articulationId == "sustain",
            "Redo must restore the reassignment and deleted-articulation state.");
}
} // namespace

int main()
{
    try
    {
        verifyMigrationAndRoundTrip();
        verifyValidation();
        verifyAuthoringTransactions();
        std::cout << "Performance-engine Sprint 1 articulation schema tests passed.\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Performance-engine Sprint 1 articulation schema tests failed: "
                  << exception.what() << '\n';
        return 1;
    }
}
