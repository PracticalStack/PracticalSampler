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
        require(migratedPhase1.project.authoring.zones.empty(),
                "Phase 1 migration should not invent authoring zones before the importer exists.");

        const auto phase2ProjectPath = fs::path(drs::engine::getPhase2ReferenceProjectManifestPath());
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring foundation fixture must load successfully.");
        require(phase2Project.project.schemaVersion == 2, "Phase 2 authoring fixture schemaVersion changed unexpectedly.");
        require(phase2Project.project.authoring.zones.size() == 3, "Phase 2 authoring fixture zone count changed unexpectedly.");
        require(phase2Project.project.authoring.macros.size() == 2, "Phase 2 authoring fixture macro count changed unexpectedly.");
        require(phase2Project.project.authoring.fxSlots.size() == 2, "Phase 2 authoring fixture FX slot count changed unexpectedly.");
        require(phase2Project.project.authoring.routingBuses.size() == 2,
                "Phase 2 authoring fixture routing-bus count changed unexpectedly.");
        require(phase2Project.project.authoring.performanceBanks.size() == 1,
                "Phase 2 authoring fixture performance-bank count changed unexpectedly.");

        const auto serializedPhase2Project = drs::engine::serializeRuntimeProjectManifest(phase2Project.project,
                                                                                           phase2ProjectPath.generic_string());
        require(serializedPhase2Project == readTextFile(phase2ProjectPath),
                "Phase 2 authoring fixture must round-trip without text drift.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        require(controller.getDocumentState().revision == 0, "Fresh project document controller should start at revision 0.");
        require(!controller.getDocumentState().dirty, "Fresh project document controller should start clean.");

        auto firstEdit = controller.getProject();
        firstEdit.authoring.selectedZoneId = "pad-a3-high";
        firstEdit.authoring.zones[2].gainDb = 2.0;
        firstEdit.authoring.notes.push_back("First transaction for Sprint 1 history coverage.");

        const auto firstCommit = controller.commitSnapshot(
            firstEdit,
            "Select alternate zone and trim lead gain",
            {"authoring.selectedZoneId", "authoring.zones[2].gainDb", "authoring.notes"});
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
