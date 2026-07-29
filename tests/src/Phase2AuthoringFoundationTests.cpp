#include "drs/engine/AuthoringSession.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"

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

std::string readTextFile(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

void writeTextFile(const fs::path& path, const std::string& text)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << text;
}
} // namespace

int main()
{
    try
    {
        const auto phase2ProjectPath = fs::path(drs::engine::getPhase2ReferenceProjectManifestPath());
        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must continue to load before Sprint 1 migration tests run.");

        const auto migratedPhase1 = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedPhase1.valid, "Phase 1 reference project should migrate into the Phase 2 authoring schema.");
        require(migratedPhase1.migrated, "Phase 1 reference project migration should report an applied migration.");
        require(migratedPhase1.project.schemaVersion == 2, "Migrated Phase 1 project must now use schemaVersion 2.");
        require(migratedPhase1.project.authoring.schemaName == "drs.authoring",
                "Migrated Phase 1 project must gain the authoring schema.");
        require(migratedPhase1.project.authoring.schemaVersion == 1,
                "Migrated Phase 1 authoring schema version changed unexpectedly.");
        require(migratedPhase1.project.sampleSources.size() == phase1Project.project.sampleSources.size(),
                "Phase 1 migration must preserve the existing sample-source inventory.");
        require(migratedPhase1.project.authoring.selectedZoneId.empty(),
                "Phase 1 migration must not invent a selected zone before imported zones exist.");
        require(migratedPhase1.project.authoring.selectedPerformanceBankId.empty(),
                "Phase 1 migration must not invent a selected performance bank early.");
        require(migratedPhase1.project.authoring.zones.empty(),
                "Phase 1 migration should not invent authoring zones before the importer exists.");
        require(migratedPhase1.project.authoring.macros.empty(),
                "Phase 1 migration should not invent authoring macros before authoring data exists.");
        require(migratedPhase1.project.authoring.fxSlots.empty(),
                "Phase 1 migration should not invent authoring FX slots before authoring data exists.");
        require(migratedPhase1.project.authoring.routingBuses.empty(),
                "Phase 1 migration should not invent authoring routing buses before authoring data exists.");
        require(migratedPhase1.project.authoring.performanceBanks.empty(),
                "Phase 1 migration should not invent performance banks before authoring data exists.");
        const auto migratedPhase1Json = drs::engine::serializeRuntimeProjectManifest(
            migratedPhase1.project,
            phase2ProjectPath.generic_string());
        require(migratedPhase1Json.find("\"authoring\"") != std::string::npos,
                "Migrated Phase 1 project must serialize the Phase 2 authoring container.");
        require(migratedPhase1Json.find("roundRobin") == std::string::npos,
                "Migrated Phase 1 project serialization must not invent Round Robin entities early.");
        require(migratedPhase1Json.find("micPosition") == std::string::npos,
                "Migrated Phase 1 project serialization must not invent mic-position entities early.");
        require(migratedPhase1Json.find("sfz") == std::string::npos,
                "Migrated Phase 1 project serialization must not invent SFZ entities early.");

        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring foundation fixture must load successfully.");
        require(phase2Project.project.schemaVersion == 4, "Phase 2 authoring fixture schemaVersion changed unexpectedly.");
        require(phase2Project.project.authoring.schemaVersion == 3,
                "Phase 2 authoring fixture authoring schemaVersion changed unexpectedly.");
        require(phase2Project.project.authoring.zones.size() == 3, "Phase 2 authoring fixture zone count changed unexpectedly.");
        require(phase2Project.project.authoring.groups.size() == 2,
                "Phase 2 authoring fixture group count changed unexpectedly.");
        require(phase2Project.project.authoring.macros.size() == 2, "Phase 2 authoring fixture macro count changed unexpectedly.");
        require(phase2Project.project.authoring.fxSlots.size() == 2, "Phase 2 authoring fixture FX slot count changed unexpectedly.");
        require(phase2Project.project.authoring.routingBuses.size() == 2,
                "Phase 2 authoring fixture routing-bus count changed unexpectedly.");
        require(phase2Project.project.authoring.performanceBanks.size() == 1,
                "Phase 2 authoring fixture performance-bank count changed unexpectedly.");
        require(phase2Project.project.authoring.selectedGroupId == "lead-core",
                "Phase 2 authoring fixture selectedGroupId changed unexpectedly.");

        drs::engine::RuntimeProjectModel blankProject;
        blankProject.schemaName = "drs.project";
        blankProject.schemaVersion = 4;
        blankProject.projectId = "phase2.blank-project";
        blankProject.displayName = "Blank Project";
        blankProject.contentRootPath = phase2Project.project.contentRootPath;
        blankProject.defaultInstrumentManifestPath = phase2Project.project.defaultInstrumentManifestPath;
        blankProject.authoring.schemaName = "drs.authoring";
        blankProject.authoring.schemaVersion = 3;
        blankProject.notes.push_back("Blank project validation fixture.");

        const auto blankValidation = drs::engine::validateRuntimeProjectModel(blankProject);
        require(blankValidation.valid, "Blank Phase 2 authoring project should validate without sample sources.");

        drs::engine::AuthoringSession blankSession(blankProject);
        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "imported-sine-a3";
        importedSampleSource.path = phase2Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "imported-pad-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Imported Pad A3";
        importedZone.groupId = "imported-group";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 60;
        importedZone.keyLow = 60;
        importedZone.keyHigh = 60;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto appendResult = blankSession.appendImportedContent({ importedSampleSource },
                                                                    { importedZone },
                                                                    "Import draft sample");
        require(appendResult.applied, "Blank Phase 2 authoring project should accept imported draft content.");
        require(blankSession.getProject().sampleSources.size() == 1,
                "Imported draft content should append a sample source.");
        require(blankSession.getProject().authoring.zones.size() == 1,
                "Imported draft content should append an authoring zone.");
        require(blankSession.getProject().authoring.groups.size() == 1,
                "Imported draft content should synthesize one explicit authored group.");
        require(blankSession.getProject().authoring.selectedZoneId == "imported-pad-a3",
                "Imported draft content should select the first imported zone.");
        require(blankSession.getProject().authoring.selectedGroupId == "imported-group",
                "Imported draft content should select the imported group for schemaVersion 4 projects.");
        require(blankSession.getDocumentState().dirty,
                "Imported draft content should mark the authoring project dirty.");

        const auto serializedPhase2Project = drs::engine::serializeRuntimeProjectManifest(phase2Project.project,
                                                                                           phase2ProjectPath.generic_string());
        require(serializedPhase2Project == readTextFile(phase2ProjectPath),
                "Phase 2 authoring fixture must round-trip without text drift.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        require(controller.getDocumentState().revision == 0, "Fresh project document controller should start at revision 0.");
        require(!controller.getDocumentState().dirty, "Fresh project document controller should start clean.");

        auto firstEdit = controller.getProject();
        firstEdit.authoring.selectedZoneId = "pad-a3-high";
        firstEdit.authoring.selectedGroupId = "pad-core";
        firstEdit.authoring.zones[2].gainDb = 2.0;
        firstEdit.authoring.zones[2].triggerMode = drs::engine::ZoneTriggerMode::oneShot;
        firstEdit.authoring.notes.push_back("First transaction for Sprint 1 history coverage.");

        const auto firstCommit = controller.commitSnapshot(
            firstEdit,
            "Select alternate zone and trim lead gain",
            {"authoring.selectedZoneId", "authoring.selectedGroupId", "authoring.zones[2].gainDb",
             "authoring.zones[2].triggerMode", "authoring.notes"});
        require(firstCommit.applied, "First authoring project transaction should commit successfully.");
        require(firstCommit.documentState.revision == 1, "First authoring project transaction should increment revision.");
        require(firstCommit.documentState.undoDepth == 1, "First authoring project transaction should create one undo checkpoint.");
        require(firstCommit.documentState.redoDepth == 0, "First authoring project transaction should clear redo history.");
        require(firstCommit.documentState.dirty, "Committed authoring project transaction should mark the document dirty.");

        controller.markSaved();
        require(controller.getDocumentState().savedRevision == 1,
                "Saved project document checkpoint should track the current revision.");
        require(!controller.getDocumentState().dirty,
                "Project document should become clean once the save checkpoint is recorded.");

        auto secondEdit = controller.getProject();
        secondEdit.authoring.fxSlots[1].bypassed = false;
        secondEdit.authoring.performanceBanks[0].notes.push_back("Second transaction for Sprint 1 redo coverage.");

        const auto secondCommit = controller.commitSnapshot(
            secondEdit,
            "Enable shimmer delay and extend bank notes",
            {"authoring.fxSlots[1].bypassed", "authoring.performanceBanks[0].notes"});
        require(secondCommit.applied, "Second authoring project transaction should commit successfully.");
        require(secondCommit.documentState.revision == 2, "Second authoring project transaction should increment revision.");
        require(secondCommit.documentState.undoDepth == 2, "Second authoring project transaction should deepen undo history.");
        require(secondCommit.documentState.dirty, "Second authoring project transaction should move the document away from its saved checkpoint.");

        const auto undoResult = controller.undo();
        require(undoResult.applied, "Undo should restore the previous authoring snapshot.");
        require(controller.getProject().authoring.fxSlots[1].bypassed,
                "Undo should restore the pre-edit FX bypass state.");
        require(controller.getDocumentState().revision == 1, "Undo should restore the saved revision id.");
        require(!controller.getDocumentState().dirty,
                "Undo back to the saved checkpoint should clear the dirty flag.");

        const auto redoResult = controller.redo();
        require(redoResult.applied, "Redo should restore the later authoring snapshot.");
        require(!controller.getProject().authoring.fxSlots[1].bypassed,
                "Redo should reapply the FX bypass edit.");
        require(controller.getDocumentState().revision == 2, "Redo should restore the later revision id.");
        require(controller.getDocumentState().dirty,
                "Redo away from the saved checkpoint should re-mark the document dirty.");

        const auto tempDirectory = fs::temp_directory_path() / "drs-phase2-authoring-foundation-tests";
        const auto tempProjectPath = tempDirectory / "phase2-authoring-roundtrip.drsproj";
        const auto roundTripJson = drs::engine::serializeRuntimeProjectManifest(controller.getProject(),
                                                                                tempProjectPath.generic_string());
        writeTextFile(tempProjectPath, roundTripJson);

        const auto roundTripLoad = drs::engine::loadRuntimeProjectManifest(tempProjectPath.generic_string());
        require(roundTripLoad.loaded, "Saved Phase 2 authoring project should load successfully.");
        require(!roundTripLoad.project.authoring.fxSlots[1].bypassed,
                "Saved Phase 2 authoring project must preserve the edited FX bypass state.");
        require(roundTripLoad.project.authoring.selectedZoneId == "pad-a3-high",
                "Saved Phase 2 authoring project must preserve the edited selected zone.");
        require(roundTripLoad.project.authoring.selectedGroupId == "pad-core",
                "Saved Phase 2 authoring project must preserve the edited selected group.");
        require(roundTripLoad.project.authoring.zones[2].triggerMode == drs::engine::ZoneTriggerMode::oneShot,
                "Saved Phase 2 authoring project must preserve one-shot trigger mode.");
        require(roundTripJson.find("\"triggerMode\": \"one-shot\"") != std::string::npos,
                "One-shot zones must serialize their trigger mode explicitly.");

        const auto blankProjectPath = tempDirectory / "blank-project-roundtrip.drsproj";
        writeTextFile(blankProjectPath,
                      drs::engine::serializeRuntimeProjectManifest(blankProject, blankProjectPath.generic_string()));
        const auto blankRoundTrip = drs::engine::loadRuntimeProjectManifest(blankProjectPath.generic_string());
        require(blankRoundTrip.loaded, "Blank Phase 2 authoring project should survive save/load round-tripping.");
        require(blankRoundTrip.project.sampleSources.empty(),
                "Blank Phase 2 authoring project should preserve an empty sample-source list.");
        require(blankRoundTrip.project.authoring.zones.empty(),
                "Blank Phase 2 authoring project should preserve an empty authoring-zone list.");
        require(blankRoundTrip.project.authoring.groups.empty(),
                "Blank Phase 2 authoring project should preserve an empty authored-group list.");

        auto invalidEdit = controller.getProject();
        invalidEdit.authoring.zones[0].sampleSourceId = "missing-source";
        const auto rejectedCommit = controller.commitSnapshot(invalidEdit,
                                                              "Introduce invalid sample-source reference",
                                                              {"authoring.zones[0].sampleSourceId"});
        require(!rejectedCommit.applied, "Invalid authoring project transaction should be rejected.");
        require(controller.getDocumentState().revision == 2,
                "Rejected authoring project transaction must not change the current revision.");
        require(controller.getProject().authoring.zones[0].sampleSourceId == "sine-a3",
                "Rejected authoring project transaction must leave the current project unchanged.");

        std::cout << "Phase 2 authoring foundation tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 2 authoring foundation tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
