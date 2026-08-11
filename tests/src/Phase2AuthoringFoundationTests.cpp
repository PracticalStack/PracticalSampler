#include "drs/engine/AuthoringSession.h"
#include "drs/engine/ProjectDocument.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/SampleImport.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

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

        drs::engine::RuntimeProjectGroupDefinition reassignedGroup;
        reassignedGroup.id = "reassigned-group";
        reassignedGroup.displayName = "Reassigned Group";
        reassignedGroup.workspaceVisible = true;
        const auto createGroupResult = blankSession.createGroup(reassignedGroup, "Create reassignment group");
        require(createGroupResult.applied,
                "Phase 2 authoring sessions should allow empty explicit groups to be created.");

        const auto reassignResult = blankSession.reassignZonesToGroup({ "imported-pad-a3" },
                                                                      "reassigned-group",
                                                                      "Move zone into reassigned group");
        require(reassignResult.applied,
                "Phase 2 authoring sessions should allow zones to move into another explicit group.");
        require(blankSession.getProject().authoring.zones[0].groupId == "reassigned-group",
                "Zone reassignment should persist the new group id onto the zone.");
        require(blankSession.getProject().authoring.selectedGroupId == "reassigned-group",
                "Reassigning the selected zone should keep group selection aligned with the moved zone.");

        drs::engine::RuntimeProjectSampleSource secondImportedSampleSource;
        secondImportedSampleSource.id = "imported-sine-a3-rr2";
        secondImportedSampleSource.path = phase2Project.project.sampleSources[0].path;
        secondImportedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition secondImportedZone;
        secondImportedZone.id = "imported-pad-a3-rr2";
        secondImportedZone.sampleSourceId = secondImportedSampleSource.id;
        secondImportedZone.displayName = "Imported Pad A3 RR2";
        secondImportedZone.groupId = "imported-group";
        secondImportedZone.articulationId = "sustain";
        secondImportedZone.rootKey = 60;
        secondImportedZone.keyLow = 60;
        secondImportedZone.keyHigh = 60;
        secondImportedZone.velocityLow = 1;
        secondImportedZone.velocityHigh = 127;
        secondImportedZone.roundRobin = drs::engine::RoundRobinDescriptor {
            "rr-import-test",
            2,
            2,
            drs::engine::RoundRobinMode::sequential
        };
        secondImportedZone.roundRobinLength = 2;
        secondImportedZone.roundRobinPosition = 2;

        const auto secondAppendResult = blankSession.appendImportedContent({ secondImportedSampleSource },
                                                                           { secondImportedZone },
                                                                           "Import second draft sample");
        require(secondAppendResult.applied,
                "Session import should heal incomplete single-zone round-robin descriptors instead of rejecting them.");
        require(blankSession.getProject().authoring.zones.size() == 2,
                "Second imported draft content should append its authoring zone.");
        require(!blankSession.getProject().authoring.zones[1].roundRobin.has_value()
                    && blankSession.getProject().authoring.zones[1].roundRobinLength == 0
                    && blankSession.getProject().authoring.zones[1].roundRobinPosition == 0,
                "Session import should clear incomplete single-zone round-robin metadata before validation.");

        const auto serializedPhase2Project = drs::engine::serializeRuntimeProjectManifest(phase2Project.project,
                                                                                           phase2ProjectPath.generic_string());
        require(serializedPhase2Project.find("\"masterGainDb\": 0.0") != std::string::npos,
                "Legacy Phase 2 authoring fixtures must normalize to an explicit master gain on save.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        require(controller.getDocumentState().revision == 0, "Fresh project document controller should start at revision 0.");
        require(!controller.getDocumentState().dirty, "Fresh project document controller should start clean.");

        auto firstEdit = controller.getProject();
        firstEdit.authoring.selectedZoneId = "pad-a3-high";
        firstEdit.authoring.selectedGroupId = "pad-core";
        firstEdit.authoring.masterGainDb = -2.5;
        firstEdit.authoring.zones[2].gainDb = 2.0;
        firstEdit.authoring.zones[2].triggerMode = drs::engine::ZoneTriggerMode::oneShot;
        firstEdit.authoring.notes.push_back("First transaction for Sprint 1 history coverage.");

        const auto firstCommit = controller.commitSnapshot(
            firstEdit,
            "Select alternate zone and trim lead gain",
            {"authoring.selectedZoneId", "authoring.selectedGroupId", "authoring.masterGainDb", "authoring.zones[2].gainDb",
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

        const auto exportedCheckpoint = controller.exportCheckpoint();
        require(exportedCheckpoint.project.projectId == phase2Project.project.projectId,
                "Exported checkpoint must contain the current project model.");
        require(exportedCheckpoint.revision == 2
                    && exportedCheckpoint.savedRevision == 1
                    && exportedCheckpoint.dirty,
                "Exported checkpoint must preserve document revision metadata exactly.");

        drs::engine::RuntimeProjectDocumentController restoredController(blankProject);
        auto temporaryEdit = restoredController.getProject();
        temporaryEdit.displayName = "Temporary destination edit";
        require(restoredController.commitSnapshot(temporaryEdit, "Temporary destination history").applied,
                "Checkpoint destination must establish undo history before replacement.");
        require(restoredController.undo().applied
                    && restoredController.getDocumentState().redoDepth == 1,
                "Checkpoint destination must establish redo history before replacement.");

        const auto restoreCheckpointResult = restoredController.restoreCheckpoint(exportedCheckpoint);
        require(restoreCheckpointResult.applied,
                "A valid project document checkpoint must restore atomically.");
        require(restoredController.getProject().projectId == phase2Project.project.projectId
                    && !restoredController.getProject().authoring.fxSlots[1].bypassed,
                "Checkpoint restore must replace the complete current project model.");
        require(restoredController.getDocumentState().revision == 2
                    && restoredController.getDocumentState().savedRevision == 1
                    && restoredController.getDocumentState().dirty,
                "Checkpoint restore must preserve revision, saved revision, and dirty state exactly.");
        require(restoredController.getDocumentState().lastChangeLabel
                    == exportedCheckpoint.lastChangeLabel,
                "Checkpoint restore must preserve the last change label.");
        require(restoredController.getDocumentState().undoDepth == 0
                    && restoredController.getDocumentState().redoDepth == 0,
                "Checkpoint restore must intentionally reset undo and redo history.");
        require(!restoredController.undo().applied && !restoredController.redo().applied,
                "Restored checkpoints must not manufacture unavailable undo or redo entries.");

        const auto stableCheckpoint = restoredController.exportCheckpoint();
        auto invalidMetadataCheckpoint = stableCheckpoint;
        invalidMetadataCheckpoint.savedRevision = invalidMetadataCheckpoint.revision + 1;
        invalidMetadataCheckpoint.dirty = true;
        require(!restoredController.restoreCheckpoint(invalidMetadataCheckpoint).applied,
                "A checkpoint with savedRevision above revision must be rejected.");
        require(restoredController.getDocumentState().revision == stableCheckpoint.revision
                    && restoredController.getProject().projectId == stableCheckpoint.project.projectId,
                "Rejected checkpoint metadata must preserve the prior document atomically.");

        auto invalidProjectCheckpoint = stableCheckpoint;
        invalidProjectCheckpoint.project.projectId.clear();
        require(!restoredController.restoreCheckpoint(invalidProjectCheckpoint).applied,
                "A checkpoint with an invalid project model must be rejected.");
        require(restoredController.getProject().projectId == stableCheckpoint.project.projectId,
                "Rejected checkpoint project content must preserve the prior valid project.");

        const auto requireMalformedCheckpointRejected =
            [&](drs::engine::RuntimeProjectDocumentCheckpoint checkpoint,
                drs::engine::RuntimeProjectDocumentCheckpointConstraints constraints,
                const std::string& expectedIssue,
                const std::string& context)
            {
                const auto result = restoredController.restoreCheckpoint(
                    std::move(checkpoint),
                    std::move(constraints));
                require(!result.applied, context + " must be rejected.");
                require(std::any_of(result.issues.begin(),
                                    result.issues.end(),
                                    [&](const std::string& issue)
                                    {
                                        return issue.find(expectedIssue) != std::string::npos;
                                    }),
                        context + " must report its typed validation category.");
                require(restoredController.getProject().projectId
                            == stableCheckpoint.project.projectId
                            && restoredController.getProject().authoring.selectedZoneId
                                == stableCheckpoint.project.authoring.selectedZoneId
                            && restoredController.getDocumentState().revision
                                == stableCheckpoint.revision,
                        context + " must preserve the prior session atomically.");
            };

        auto unknownSelectedZoneCheckpoint = stableCheckpoint;
        unknownSelectedZoneCheckpoint.project.authoring.selectedZoneId = "missing-zone";
        requireMalformedCheckpointRejected(
            std::move(unknownSelectedZoneCheckpoint),
            {},
            "selectedZoneId references unknown zone",
            "A checkpoint with an unknown selected zone");

        auto unknownSelectedGroupCheckpoint = stableCheckpoint;
        unknownSelectedGroupCheckpoint.project.authoring.selectedGroupId = "missing-group";
        requireMalformedCheckpointRejected(
            std::move(unknownSelectedGroupCheckpoint),
            {},
            "selectedGroupId references unknown group",
            "A checkpoint with an unknown selected group");

        auto unknownSelectedBankCheckpoint = stableCheckpoint;
        unknownSelectedBankCheckpoint.project.authoring.selectedPerformanceBankId = "missing-bank";
        requireMalformedCheckpointRejected(
            std::move(unknownSelectedBankCheckpoint),
            {},
            "selectedPerformanceBankId references unknown bank",
            "A checkpoint with an unknown selected performance bank");

        auto incompatibleSchemaCheckpoint = stableCheckpoint;
        incompatibleSchemaCheckpoint.project.authoring.schemaVersion = 2;
        requireMalformedCheckpointRejected(
            std::move(incompatibleSchemaCheckpoint),
            {},
            "authoring schemaVersion must be 3",
            "A checkpoint with incompatible nested schema versions");

        auto malformedPathCheckpoint = stableCheckpoint;
        malformedPathCheckpoint.project.contentRootPath
            = std::string("invalid\0content-root", 20);
        requireMalformedCheckpointRejected(
            std::move(malformedPathCheckpoint),
            {},
            "contentRootPath is not a valid bounded filesystem path",
            "A checkpoint with a malformed project path");

        auto dirtyMismatchCheckpoint = stableCheckpoint;
        dirtyMismatchCheckpoint.dirty = false;
        requireMalformedCheckpointRejected(
            std::move(dirtyMismatchCheckpoint),
            {},
            "dirty must equal revision != savedRevision",
            "A checkpoint with inconsistent dirty metadata");

        drs::engine::RuntimeProjectDocumentCheckpointConstraints wrongIdentityConstraints;
        wrongIdentityConstraints.expectedProjectId = "drs.unrelated-project";
        wrongIdentityConstraints.manifestPath = phase2ProjectPath.generic_string();
        requireMalformedCheckpointRejected(
            stableCheckpoint,
            std::move(wrongIdentityConstraints),
            "does not match the expected host-state project identity",
            "A checkpoint with a mismatched expected project identity");

        drs::engine::RuntimeProjectDocumentCheckpointConstraints malformedManifestConstraints;
        malformedManifestConstraints.expectedProjectId = stableCheckpoint.project.projectId;
        malformedManifestConstraints.manifestPath = "C:/Projects/not-a-project.txt";
        requireMalformedCheckpointRejected(
            stableCheckpoint,
            std::move(malformedManifestConstraints),
            "manifestPath must use the .drsproj extension",
            "A checkpoint with a malformed manifest locator");

        drs::engine::AuthoringSession checkpointSourceSession(phase2Project.project);
        require(checkpointSourceSession.selectZone("pad-a3-high").applied,
                "AuthoringSession checkpoint source must accept a distinct workspace selection.");
        checkpointSourceSession.markSaved();
        require(checkpointSourceSession.selectZone("lead-a4-sustain").applied,
                "AuthoringSession checkpoint source must accept a post-save workspace selection.");
        const auto sessionCheckpoint = checkpointSourceSession.exportCheckpoint();
        require(sessionCheckpoint.revision == 0
                    && sessionCheckpoint.savedRevision == 0
                    && !sessionCheckpoint.dirty
                    && sessionCheckpoint.project.authoring.selectedZoneId
                        == phase2Project.project.authoring.selectedZoneId
                    && checkpointSourceSession.getSelectedZone().has_value()
                    && checkpointSourceSession.getSelectedZone()->id == "lead-a4-sustain",
                "Workspace selection must remain transient and absent from document checkpoint metadata.");

        drs::engine::AuthoringSession checkpointDestinationSession(blankProject);
        const auto sessionRestore = checkpointDestinationSession.restoreCheckpoint(sessionCheckpoint);
        require(sessionRestore.applied,
                "AuthoringSession must atomically restore a valid document checkpoint.");
        require(checkpointDestinationSession.getProject().projectId
                    == checkpointSourceSession.getProject().projectId
                    && checkpointDestinationSession.getProject().authoring.selectedZoneId
                        == phase2Project.project.authoring.selectedZoneId,
                "AuthoringSession checkpoint restore must replace the authored project without persisting workspace selection.");
        require(checkpointDestinationSession.getDocumentState().revision == 0
                    && checkpointDestinationSession.getDocumentState().savedRevision == 0
                    && !checkpointDestinationSession.getDocumentState().dirty
                    && checkpointDestinationSession.getDocumentState().undoDepth == 0
                    && checkpointDestinationSession.getDocumentState().redoDepth == 0,
                "AuthoringSession checkpoint restore must preserve metadata and reset history.");

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
        require(std::abs(roundTripLoad.project.authoring.masterGainDb - (-2.5)) < 1.0e-9,
                "Saved Phase 2 authoring project must preserve the edited master gain.");
        require(roundTripLoad.project.authoring.zones[2].triggerMode == drs::engine::ZoneTriggerMode::oneShot,
                "Saved Phase 2 authoring project must preserve one-shot trigger mode.");
        require(roundTripJson.find("\"masterGainDb\": -2.5") != std::string::npos,
                "Saved Phase 2 authoring project must serialize master gain explicitly.");
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
        require(std::abs(blankRoundTrip.project.authoring.masterGainDb) < 1.0e-9,
                "Blank Phase 2 authoring project should default master gain to unity.");

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
