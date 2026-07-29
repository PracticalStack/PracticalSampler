#include "drs/engine/AuthoringSession.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeLoader.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

bool containsFinding(const drs::engine::PlaybackSnapshotBuildResult& result,
                     drs::engine::PlaybackSnapshotFindingSeverity severity,
                     const std::string& code,
                     const std::string& pathFragment)
{
    for (const auto& finding : result.findings)
    {
        if (finding.severity == severity
            && finding.code == code
            && finding.path.find(pathFragment) != std::string::npos)
        {
            return true;
        }
    }

    return false;
}
} // namespace

int main()
{
    try
    {
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before playback snapshot tests run.");

        drs::engine::PlaybackSnapshotBuilder builder;
        const auto firstRequest = builder.requestBuild(0, true);
        require(firstRequest.accepted, "Initial playback snapshot build request should be accepted.");
        require(firstRequest.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::preparing,
                "Accepted playback snapshot requests should begin in the preparing state.");

        const auto firstResult = builder.buildSnapshot(firstRequest, phase2Project.project);
        require(firstResult.built, "Phase 2 authoring fixture should build a playback snapshot.");
        require(firstResult.activationEligible, "Valid playback snapshot result should be activation-eligible.");
        require(firstResult.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Successful playback snapshot build should finish in the ready state.");
        require(firstResult.findings.empty(), "Reference Phase 2 project should not produce playback snapshot findings.");
        require(firstResult.snapshot.schemaName == "drs.playbackSnapshot",
                "Playback snapshot schemaName changed unexpectedly.");
        require(firstResult.snapshot.schemaVersion == 1,
                "Playback snapshot schemaVersion changed unexpectedly.");
        require(firstResult.snapshot.sourceProjectSchemaName == phase2Project.project.schemaName,
                "Playback snapshot must record the source project schema name.");
        require(firstResult.snapshot.sourceProjectSchemaVersion == phase2Project.project.schemaVersion,
                "Playback snapshot must record the source project schema version.");
        require(firstResult.snapshot.sourceAuthoringSchemaName == phase2Project.project.authoring.schemaName,
                "Playback snapshot must record the source authoring schema name.");
        require(firstResult.snapshot.sourceAuthoringSchemaVersion == phase2Project.project.authoring.schemaVersion,
                "Playback snapshot must record the source authoring schema version.");
        require(firstResult.snapshot.draftRevision == 0,
                "Playback snapshot should record the requested draft revision.");
        require(firstResult.snapshot.selectedZoneId == phase2Project.project.authoring.selectedZoneId,
                "Playback snapshot should preserve the selected zone identity.");
        require(firstResult.snapshot.selectedGroupId == phase2Project.project.authoring.selectedGroupId,
                "Playback snapshot should preserve the selected group identity.");
        require(firstResult.snapshot.selectedPerformanceBankId == phase2Project.project.authoring.selectedPerformanceBankId,
                "Playback snapshot should preserve the selected performance-bank identity.");
        require(firstResult.snapshot.sampleIdentities.size() == phase2Project.project.sampleSources.size(),
                "Playback snapshot sample identity count changed unexpectedly.");
        require(firstResult.snapshot.macroDefaults.size() == phase2Project.project.authoring.macros.size(),
                "Playback snapshot macro-default count changed unexpectedly.");
        require(firstResult.snapshot.routingBuses.size() == phase2Project.project.authoring.routingBuses.size(),
                "Playback snapshot routing-bus count changed unexpectedly.");
        require(firstResult.snapshot.groupRoutes.size() == phase2Project.project.authoring.groups.size(),
                "Playback snapshot group-route count should stay aligned with authored groups.");
        require(firstResult.snapshot.zones.size() == phase2Project.project.authoring.zones.size(),
                "Playback snapshot zone count changed unexpectedly.");
        require(!firstResult.snapshot.contentDigest.empty(),
                "Successful playback snapshot builds must carry a stable content digest.");

        const auto firstSerialized = drs::engine::serializeImmutablePlaybackSnapshot(firstResult.snapshot);
        require(firstSerialized.find("\"selectedGroupId\"") != std::string::npos
                    && firstSerialized.find("\"routingSourceId\"") != std::string::npos
                    && firstSerialized.find("\"workspaceVisible\"") != std::string::npos,
                "Sprint 3 playback snapshots must serialize explicit group selection and route metadata.");
        require(firstSerialized.find("roundRobin") == std::string::npos,
                "Sprint 2 playback snapshots must not invent Round Robin entities early.");
        require(firstSerialized.find("micPosition") == std::string::npos,
                "Sprint 2 playback snapshots must not invent mic-position entities early.");
        require(firstSerialized.find("sfz") == std::string::npos,
                "Sprint 2 playback snapshots must not invent SFZ entities early.");

        const auto secondRequest = builder.requestBuild(0, true);
        const auto secondResult = builder.buildSnapshot(secondRequest, phase2Project.project);
        require(secondResult.built, "Repeated playback snapshot build should still succeed.");
        require(secondResult.snapshot.contentDigest == firstResult.snapshot.contentDigest,
                "Identical draft revisions should produce the same playback snapshot digest.");
        require(drs::engine::serializeImmutablePlaybackSnapshot(secondResult.snapshot) == firstSerialized,
                "Identical draft revisions should produce byte-equivalent playback snapshots.");

        auto visibilityOnlyProject = phase2Project.project;
        visibilityOnlyProject.authoring.groups[0].workspaceVisible = !visibilityOnlyProject.authoring.groups[0].workspaceVisible;
        const auto visibilityRequest = builder.requestBuild(0, true);
        const auto visibilityResult = builder.buildSnapshot(visibilityRequest, visibilityOnlyProject);
        require(visibilityResult.built, "Visibility-only group changes should still build a snapshot.");
        require(visibilityResult.snapshot.contentDigest == firstResult.snapshot.contentDigest,
                "Group workspace visibility must not affect the immutable snapshot digest.");
        require(drs::engine::serializeImmutablePlaybackSnapshot(visibilityResult.snapshot) != firstSerialized,
                "Group workspace visibility should remain visible in the serialized snapshot payload.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        auto editedProject = controller.getProject();
        editedProject.authoring.zones[0].gainDb += 1.0;
        const auto commitResult = controller.commitSnapshot(editedProject,
                                                            "Adjust the first zone gain for snapshot coverage",
                                                            {"authoring.zones[0].gainDb"});
        require(commitResult.applied, "Edited project revision should commit before snapshot rebuild.");

        const auto editedRequest = builder.requestBuild(commitResult.documentState.revision, true);
        const auto editedResult = builder.buildSnapshot(editedRequest, controller.getProject());
        require(editedResult.built, "Edited draft revision should still build a playback snapshot.");
        require(editedResult.snapshot.contentDigest != firstResult.snapshot.contentDigest,
                "Snapshot digest should change predictably when the authored draft changes.");
        require(editedResult.snapshot.zones[0].gainDb == controller.getProject().authoring.zones[0].gainDb,
                "Playback snapshot should carry the edited zone normalization values.");

        auto unknownGroupProject = phase2Project.project;
        unknownGroupProject.authoring.zones[0].groupId = "ghost-group";
        const auto unknownGroupResult = builder.buildSnapshot(builder.requestBuild(6, true), unknownGroupProject);
        require(!unknownGroupResult.built, "Zones that reference missing authored groups must fail snapshot validation.");
        require(containsFinding(unknownGroupResult,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "unknown-zone-group-reference",
                                "authoring.zones[0].groupId"),
                "Snapshot validation should report a structured finding when a zone references a missing authored group.");

        auto groupRoutingProject = phase2Project.project;
        groupRoutingProject.authoring.routingBuses[0].inputSourceId =
            "groups/" + groupRoutingProject.authoring.groups[0].id;
        groupRoutingProject.authoring.groups[0].routingBusId =
            groupRoutingProject.authoring.routingBuses[0].id;
        const auto groupRoutingResult = builder.buildSnapshot(builder.requestBuild(7, true), groupRoutingProject);
        require(groupRoutingResult.built && groupRoutingResult.activationEligible,
                "Sprint 4 group routing input sources should build when they resolve to an authored group.");
        require(groupRoutingResult.findings.empty(),
                "Valid Sprint 4 group routing sources should not produce snapshot findings.");

        auto unknownGroupRoutingProject = phase2Project.project;
        unknownGroupRoutingProject.authoring.routingBuses[0].inputSourceId = "groups/ghost-group";
        const auto unknownGroupRoutingResult = builder.buildSnapshot(
            builder.requestBuild(8, true), unknownGroupRoutingProject);
        require(!unknownGroupRoutingResult.built,
                "Unknown group routing input sources must fail snapshot validation.");
        require(containsFinding(unknownGroupRoutingResult,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "unknown-group-routing-input-source",
                                "authoring.routingBuses[0].inputSourceId"),
                "Snapshot validation should report a structured finding for unknown group routing inputs.");

        auto invalidProject = phase2Project.project;
        invalidProject.authoring.zones[0].sampleSourceId = "missing-source";
        const auto invalidRequest = builder.requestBuild(9, true);
        const auto invalidResult = builder.buildSnapshot(invalidRequest, invalidProject);
        require(!invalidResult.built, "Invalid sample references must fail playback snapshot activation.");
        require(!invalidResult.activationEligible,
                "Invalid playback snapshot result must never be activation-eligible.");
        require(invalidResult.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Invalid playback snapshot build should finish in the failed state.");
        require(containsFinding(invalidResult,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "unknown-zone-sample-source",
                                "authoring.zones[0].sampleSourceId"),
                "Invalid playback snapshot result should expose a structured sample-source finding.");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migration coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate into the Phase 2 authoring schema.");

        const auto migratedRequest = builder.requestBuild(0, true);
        const auto migratedResult = builder.buildSnapshot(migratedRequest, migratedProject.project);
        require(!migratedResult.built,
                "Migrated Phase 1 project should surface that it is not yet activation-eligible without imported zones.");
        require(!migratedResult.activationEligible,
                "Migrated Phase 1 project must not become activation-eligible before playable zones exist.");
        require(migratedResult.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Migrated Phase 1 project without imported zones should fail predictably.");
        require(migratedResult.snapshot.sourceProjectSchemaName == migratedProject.project.schemaName,
                "Migrated Phase 1 snapshot must report the migrated project schema name.");
        require(migratedResult.snapshot.sourceProjectSchemaVersion == migratedProject.project.schemaVersion,
                "Migrated Phase 1 snapshot must report the migrated project schema version.");
        require(migratedResult.snapshot.sourceAuthoringSchemaName == migratedProject.project.authoring.schemaName,
                "Migrated Phase 1 snapshot must report the migrated authoring schema name.");
        require(migratedResult.snapshot.sourceAuthoringSchemaVersion == migratedProject.project.authoring.schemaVersion,
                "Migrated Phase 1 snapshot must report the migrated authoring schema version.");
        require(migratedResult.snapshot.sampleIdentities.size() == migratedProject.project.sampleSources.size(),
                "Migrated Phase 1 snapshot must preserve the migrated sample-source inventory.");
        require(migratedResult.snapshot.macroDefaults.empty(),
                "Migrated Phase 1 snapshot must not invent macro defaults before authoring data exists.");
        require(migratedResult.snapshot.fxSlots.empty(),
                "Migrated Phase 1 snapshot must not invent FX slots before authoring data exists.");
        require(migratedResult.snapshot.routingBuses.empty(),
                "Migrated Phase 1 snapshot must not invent routing buses before authoring data exists.");
        require(migratedResult.snapshot.articulationRoutes.empty(),
                "Migrated Phase 1 snapshot must not invent articulation routes before zones exist.");
        require(migratedResult.snapshot.groupRoutes.empty(),
                "Migrated Phase 1 snapshot must not invent group routes before zones exist.");
        require(migratedResult.snapshot.zones.empty(),
                "Migrated Phase 1 snapshot must not invent zones before imported authoring content exists.");
        require(containsFinding(migratedResult,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Migrated Phase 1 project should report a structured no-playable-zones finding.");
        require(!containsFinding(migratedResult,
                                 drs::engine::PlaybackSnapshotFindingSeverity::error,
                                 "no-sample-identities",
                                 "sampleSources"),
                "Migrated Phase 1 project should preserve sample identities even while zones are still missing.");
        const auto migratedSerialized = drs::engine::serializeImmutablePlaybackSnapshot(migratedResult.snapshot);
        require(migratedSerialized.find("roundRobin") == std::string::npos,
                "Migrated Phase 1 snapshot coverage must not invent Round Robin entities early.");
        require(migratedSerialized.find("micPosition") == std::string::npos,
                "Migrated Phase 1 snapshot coverage must not invent mic-position entities early.");
        require(migratedSerialized.find("sfz") == std::string::npos,
                "Migrated Phase 1 snapshot coverage must not invent SFZ entities early.");

        const auto canceledResult = builder.cancelBuild(builder.requestBuild(10, false));
        require(canceledResult.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::canceled,
                "Canceled playback snapshot result should report the canceled lifecycle state.");
        require(!canceledResult.activationEligible,
                "Canceled playback snapshot result must not be activation-eligible.");

        const auto supersededSource = builder.requestBuild(11, true);
        const auto supersedingRequest = builder.requestBuild(12, true);
        const auto supersededResult = builder.supersedeBuild(supersededSource, supersedingRequest.buildId);
        require(supersededResult.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::superseded,
                "Superseded playback snapshot result should report the superseded lifecycle state.");
        require(supersededResult.cancellationId == supersedingRequest.buildId,
                "Superseded playback snapshot result should point at the replacement build identity.");

        std::cout << "Phase 1 playback snapshot tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 playback snapshot tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
