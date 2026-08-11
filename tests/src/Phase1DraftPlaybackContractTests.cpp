#include "drs/engine/AuthoringSession.h"
#include "drs/engine/DraftPlaybackContract.h"
#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/ProjectDocument.h"
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

bool containsFinding(const std::vector<drs::engine::PlaybackSnapshotFinding>& findings,
                     drs::engine::PlaybackSnapshotFindingSeverity severity,
                     const std::string& code,
                     const std::string& pathFragment)
{
    for (const auto& finding : findings)
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

drs::engine::PlaybackSnapshotBuildResult buildSnapshot(drs::engine::PlaybackSnapshotBuilder& builder,
                                                       const drs::engine::RuntimeProjectModel& project,
                                                       std::size_t revision,
                                                       bool activationRequested)
{
    const auto request = builder.requestBuild(revision, activationRequested);
    require(request.accepted, "Playback snapshot build request should be accepted during contract tests.");
    return builder.buildSnapshot(request, project);
}

struct DraftBuildArtifacts
{
    drs::engine::PlaybackSnapshotBuildResult snapshot;
    drs::engine::PreparedPlaybackBuildResult prepared;
};

DraftBuildArtifacts buildPrepared(drs::engine::PlaybackSnapshotBuilder& snapshotBuilder,
                                  drs::engine::PreparedPlaybackService& preparedService,
                                  const drs::engine::RuntimeStreamLoadResult& streamResult,
                                  const drs::engine::RuntimeProjectModel& project,
                                  std::size_t revision,
                                  bool activationRequested)
{
    DraftBuildArtifacts artifacts;
    artifacts.snapshot = buildSnapshot(snapshotBuilder, project, revision, activationRequested);
    const auto preparedRequest = preparedService.requestBuild(artifacts.snapshot);
    artifacts.prepared = preparedService.prepare(preparedRequest, artifacts.snapshot, streamResult);
    return artifacts;
}
} // namespace

int main()
{
    try
    {
        const auto phase2Project = drs::engine::loadPhase2ReferenceProjectManifest();
        require(phase2Project.loaded, "Phase 2 authoring fixture must load before the draft-to-playback contract tests run.");

        drs::engine::RuntimeProjectDocumentController controller(phase2Project.project);
        drs::engine::DraftPlaybackContract contract(controller.getDocumentState().revision);
        drs::engine::PlaybackSnapshotBuilder snapshotBuilder;
        drs::engine::PreparedPlaybackService preparedService;
        const auto referenceManifest = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(referenceManifest.loaded, "Phase 1 reference manifest must load before contract preparation coverage runs.");
        const auto referenceStream = drs::engine::loadRuntimeStreamContainerForInstrument(referenceManifest);
        require(referenceStream.loaded, "Phase 1 reference stream must load before contract preparation coverage runs.");

        const auto initialPreview = contract.requestPreviewBuild();
        require(initialPreview.accepted, "Initial preview build should be accepted.");
        require(initialPreview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::preparing,
                "Initial preview request should report the preparing lifecycle state.");
        const auto initialPreviewBuild = buildPrepared(snapshotBuilder,
                                                       preparedService,
                                                       referenceStream,
                                                       controller.getProject(),
                                                       0,
                                                       false);
        require(contract.completePreviewBuild(initialPreview.requestId,
                                              initialPreviewBuild.snapshot,
                                              initialPreviewBuild.prepared),
                "Initial preview build should complete successfully.");

        const auto initialPublish = contract.requestPerformanceBuild();
        require(initialPublish.accepted, "Initial publish build should be accepted.");
        const auto initialPublishBuild = buildPrepared(snapshotBuilder,
                                                       preparedService,
                                                       referenceStream,
                                                       controller.getProject(),
                                                       0,
                                                       true);
        require(contract.completePerformanceBuild(initialPublish.requestId,
                                                  initialPublishBuild.snapshot,
                                                  initialPublishBuild.prepared),
                "Initial publish build should complete successfully.");
        require(contract.getStatus().preview.available && contract.getStatus().preview.revision == 0,
                "Initial preview build should track draft revision 0.");
        require(contract.getStatus().preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Initial preview build should surface the ready lifecycle state.");
        require(contract.getStatus().performance.available && contract.getStatus().performance.revision == 0,
                "Initial publish build should track revision 0.");
        require(initialPublish.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::activating,
                "Initial publish request should report the activating lifecycle state.");
        require(contract.getStatus().performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
                "Initial publish build should surface the active lifecycle state.");
        require(!contract.getStatus().preview.contentDigest.empty(),
                "Initial preview build should carry a playback snapshot digest.");
        require(contract.getStatus().preview.preparedAssetsAvailable,
                "Initial preview build should carry prepared playback assets.");
        require(!contract.getStatus().preview.preparedContentDigest.empty(),
                "Initial preview build should carry a prepared playback digest.");
        require(contract.getStatus().preview.preparedSampleCount == phase2Project.project.sampleSources.size(),
                "Initial preview build should expose prepared sample counts.");
        require(contract.getStatus().preview.contentDigest == contract.getStatus().performance.contentDigest,
                "Initial preview and publish for the same draft should share a digest.");
        require(contract.getStatus().preview.preparedContentDigest == contract.getStatus().performance.preparedContentDigest,
                "Initial preview and publish for the same draft should share a prepared digest.");

        auto firstEdit = controller.getProject();
        firstEdit.authoring.zones[0].gainDb += 1.5;
        const auto firstCommit = controller.commitSnapshot(firstEdit,
                                                           "Raise first zone gain for draft revision 1",
                                                           {"authoring.zones[0].gainDb"});
        require(firstCommit.applied, "First draft edit should commit successfully.");
        require(contract.setDraftRevision(firstCommit.documentState.revision),
                "Contract should accept the first authoring revision.");
        require(contract.getStatus().draftRevision == 1, "Draft revision should advance to 1.");
        require(contract.getStatus().preview.revision == 0 && contract.getStatus().preview.state == "Stale",
                "Preview should become stale when the draft moves ahead.");
        require(contract.getStatus().performance.revision == 0 && contract.getStatus().performance.state == "Active",
                "Published performance revision must stay on the last applied revision.");

        const auto previewRevision1 = contract.requestPreviewBuild();
        require(previewRevision1.accepted, "Revision 1 preview build should be accepted.");
        const auto previewRevision1Build = buildPrepared(snapshotBuilder,
                                                         preparedService,
                                                         referenceStream,
                                                         controller.getProject(),
                                                         firstCommit.documentState.revision,
                                                         false);
        require(contract.completePreviewBuild(previewRevision1.requestId,
                                              previewRevision1Build.snapshot,
                                              previewRevision1Build.prepared),
                "Revision 1 preview build should complete successfully.");
        require(contract.getStatus().preview.revision == 1 && contract.getStatus().preview.state == "Ready",
                "Preview should advance to the latest successfully prepared draft.");
        require(contract.getStatus().preview.preparedAssetsAvailable,
                "Latest preview revision should retain prepared assets.");
        require(contract.getStatus().performance.revision == 0,
                "Preview success must not change the published performance revision.");

        const auto publishRevision1 = contract.requestPerformanceBuild();
        require(publishRevision1.accepted, "Revision 1 publish build should be accepted.");
        require(publishRevision1.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::activating,
                "Revision 1 publish request should report the activating lifecycle state.");
        require(contract.getStatus().performance.revision == 0,
                "Published revision must not change before Apply finishes.");
        require(contract.getStatus().pendingPerformance.lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::activating,
                "Pending publish work should surface the activating lifecycle state.");
        require(contract.getStatus().performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
                "While publish is pending, the last good published revision should remain active.");
        const auto publishRevision1Build = buildPrepared(snapshotBuilder,
                                                         preparedService,
                                                         referenceStream,
                                                         controller.getProject(),
                                                         firstCommit.documentState.revision,
                                                         true);
        require(contract.completePerformanceBuild(publishRevision1.requestId,
                                                  publishRevision1Build.snapshot,
                                                  publishRevision1Build.prepared),
                "Revision 1 publish build should complete successfully.");
        require(contract.getStatus().performance.revision == 1,
                "Published performance revision should change only after a successful Apply.");
        require(contract.getStatus().performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
                "Successful publish should leave the published revision active.");

        auto secondEdit = controller.getProject();
        secondEdit.authoring.zones[1].pan = -0.2;
        const auto secondCommit = controller.commitSnapshot(secondEdit,
                                                            "Pan the second zone for draft revision 2",
                                                            {"authoring.zones[1].pan"});
        require(secondCommit.applied, "Second draft edit should commit successfully.");
        require(contract.setDraftRevision(secondCommit.documentState.revision),
                "Contract should accept draft revision 2.");
        const auto canceledPublish = contract.requestPerformanceBuild();
        require(canceledPublish.accepted, "Revision 2 publish request should be accepted.");
        require(contract.cancelPerformanceBuild(canceledPublish.requestId),
                "Revision 2 publish request should be cancelable.");
        require(contract.getStatus().performance.revision == 1 && contract.getStatus().performance.state == "Active",
                "Canceling Apply must preserve the last known-good published revision.");

        auto thirdEdit = controller.getProject();
        thirdEdit.authoring.zones[2].rootKey += 1;
        const auto thirdCommit = controller.commitSnapshot(thirdEdit,
                                                           "Retune the lead zone for draft revision 3",
                                                           {"authoring.zones[2].rootKey"});
        require(thirdCommit.applied, "Third draft edit should commit successfully.");
        require(contract.setDraftRevision(thirdCommit.documentState.revision),
                "Contract should accept draft revision 3.");

        const auto supersededPublish = contract.requestPerformanceBuild();
        require(supersededPublish.accepted, "First revision 3 publish request should be accepted.");

        auto fourthEdit = controller.getProject();
        fourthEdit.authoring.zones[2].velocityHigh = 120;
        const auto fourthCommit = controller.commitSnapshot(fourthEdit,
                                                            "Tighten lead velocity ceiling for draft revision 4",
                                                            {"authoring.zones[2].velocityHigh"});
        require(fourthCommit.applied, "Fourth draft edit should commit successfully.");
        require(contract.setDraftRevision(fourthCommit.documentState.revision),
                "Contract should accept draft revision 4.");

        const auto replacementPublish = contract.requestPerformanceBuild();
        require(replacementPublish.accepted, "Superseding publish request should be accepted.");
        require(!contract.completePerformanceBuild(supersededPublish.requestId),
                "A superseded publish request must not activate an older revision.");
        const auto replacementPublishBuild = buildPrepared(snapshotBuilder,
                                                           preparedService,
                                                           referenceStream,
                                                           controller.getProject(),
                                                           fourthCommit.documentState.revision,
                                                           true);
        require(contract.completePerformanceBuild(replacementPublish.requestId,
                                                  replacementPublishBuild.snapshot,
                                                  replacementPublishBuild.prepared),
                "The newest publish request should activate successfully.");
        require(contract.getStatus().performance.revision == 4,
                "Published performance revision should advance to the newest completed Apply.");

        const auto failedPreview = contract.requestPreviewBuild();
        require(failedPreview.accepted, "Revision 4 preview request should be accepted.");
        auto invalidPreviewProject = controller.getProject();
        invalidPreviewProject.authoring.zones[0].sampleSourceId = "missing-source";
        const auto failedPreviewBuild = buildPrepared(snapshotBuilder,
                                                      preparedService,
                                                      referenceStream,
                                                      invalidPreviewProject,
                                                      fourthCommit.documentState.revision,
                                                      false);
        require(contract.completePreviewBuild(failedPreview.requestId,
                                              failedPreviewBuild.snapshot,
                                              failedPreviewBuild.prepared),
                "Invalid preview snapshot result should still be recorded against the contract.");
        require(contract.getStatus().preview.available && contract.getStatus().preview.revision == 1,
                "Preview failure should retain the last good preview revision.");
        require(contract.getStatus().preview.state == "Stale",
                "Preview failure against a newer draft should mark preview stale.");
        require(contract.getStatus().preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Preview failure against a newer draft should preserve the last good preview lifecycle state.");
        require(!contract.getStatus().preview.findings.empty(),
                "Preview failure should surface actionable findings.");

        const auto failedPublish = contract.requestPerformanceBuild();
        require(failedPublish.accepted, "Revision 4 publish request should be accepted.");
        auto invalidPublishProject = controller.getProject();
        invalidPublishProject.authoring.zones[1].sampleSourceId = "missing-source";
        const auto failedPublishBuild = buildPrepared(snapshotBuilder,
                                                      preparedService,
                                                      referenceStream,
                                                      invalidPublishProject,
                                                      fourthCommit.documentState.revision,
                                                      true);
        require(contract.completePerformanceBuild(failedPublish.requestId,
                                                  failedPublishBuild.snapshot,
                                                  failedPublishBuild.prepared),
                "Invalid publish snapshot result should still be recorded against the contract.");
        require(contract.getStatus().performance.available && contract.getStatus().performance.revision == 4,
                "Failed publish should preserve the last known-good performance revision.");
        require(contract.getStatus().performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
                "Failed publish should preserve the last known-good active lifecycle state.");
        require(!contract.getStatus().performance.findings.empty(),
                "Failed publish should surface actionable findings.");

        const auto recoveredPreview = contract.requestPreviewBuild();
        require(recoveredPreview.accepted, "Recovered preview request should be accepted.");
        const auto recoveredPreviewBuild = buildPrepared(snapshotBuilder,
                                                         preparedService,
                                                         referenceStream,
                                                         controller.getProject(),
                                                         fourthCommit.documentState.revision,
                                                         false);
        require(contract.completePreviewBuild(recoveredPreview.requestId,
                                              recoveredPreviewBuild.snapshot,
                                              recoveredPreviewBuild.prepared),
                "Recovered preview request should complete successfully.");
        require(contract.getStatus().preview.revision == 4 && contract.getStatus().preview.state == "Ready",
                "A successful rebuild should recover preview to the current draft revision.");

        auto fifthEdit = controller.getProject();
        fifthEdit.authoring.macros[0].defaultValue = 0.48;
        const auto fifthCommit = controller.commitSnapshot(fifthEdit,
                                                           "Adjust macro default for draft revision 5",
                                                           {"authoring.macros[0].defaultValue"});
        require(fifthCommit.applied, "Fifth draft edit should commit successfully.");
        require(contract.setDraftRevision(fifthCommit.documentState.revision),
                "Contract should accept draft revision 5.");
        require(contract.requestPreviewBuild().accepted, "Pending preview build before close should be accepted.");
        require(contract.requestPerformanceBuild().accepted, "Pending publish build before close should be accepted.");
        contract.closeProject();
        require(!contract.getStatus().projectOpen, "Closing the project should mark the contract closed.");
        require(!contract.getStatus().pendingPreview.active && !contract.getStatus().pendingPerformance.active,
                "Closing the project should cancel all pending preparation work.");
        require(!contract.getStatus().preview.available && contract.getStatus().preview.state == "Closed",
                "Closing the project should unload preview state.");
        require(!contract.getStatus().performance.available && contract.getStatus().performance.state == "Closed",
                "Closing the project should unload published performance state.");

        contract.reopenProject(controller.getDocumentState().revision);
        require(contract.getStatus().projectOpen && contract.getStatus().draftRevision == 5,
                "Reopening the project should restore authoring ownership at the current draft revision.");
        require(!contract.getStatus().preview.available && contract.getStatus().preview.state == "Idle",
                "Reopening the project should require new preview preparation.");
        require(!contract.getStatus().performance.available && contract.getStatus().performance.state == "Idle",
                "Reopening the project should require a new published activation.");

        const auto reopenedPublish = contract.requestPerformanceBuild();
        require(reopenedPublish.accepted, "Publish after reopen should be accepted.");
        const auto reopenedPublishBuild = buildPrepared(snapshotBuilder,
                                                        preparedService,
                                                        referenceStream,
                                                        controller.getProject(),
                                                        fifthCommit.documentState.revision,
                                                        true);
        require(contract.completePerformanceBuild(reopenedPublish.requestId,
                                                  reopenedPublishBuild.snapshot,
                                                  reopenedPublishBuild.prepared),
                "Publish after reopen should complete successfully.");

        const auto reopenedPreview = contract.requestPreviewBuild();
        require(reopenedPreview.accepted, "Preview after reopen should be accepted.");
        const auto reopenedPreviewBuild = buildPrepared(snapshotBuilder,
                                                        preparedService,
                                                        referenceStream,
                                                        controller.getProject(),
                                                        fifthCommit.documentState.revision,
                                                        false);
        require(contract.completePreviewBuild(reopenedPreview.requestId,
                                              reopenedPreviewBuild.snapshot,
                                              reopenedPreviewBuild.prepared),
                "Preview after reopen should complete successfully.");

        auto sixthEdit = controller.getProject();
        sixthEdit.authoring.routingBuses[0].displayName = "Main Bus Wide";
        const auto sixthCommit = controller.commitSnapshot(sixthEdit,
                                                           "Rename routing bus for draft revision 6",
                                                           {"authoring.routingBuses[0].displayName"});
        require(sixthCommit.applied, "Sixth draft edit should commit successfully.");
        require(contract.setDraftRevision(sixthCommit.documentState.revision),
                "Contract should accept draft revision 6.");
        require(contract.beginDeviceRestart(), "Device restart should begin while the project is open.");
        require(contract.getStatus().deviceRestartInProgress, "Device restart should be tracked explicitly.");
        require(contract.getStatus().performance.available && contract.getStatus().performance.revision == 5,
                "Device restart should preserve the published revision identity.");
        require(contract.getStatus().performance.state == "Restarting",
                "Published performance should enter a restarting state during device restart.");
        require(contract.completeDeviceRestart(true), "Successful device restart should complete.");
        require(!contract.getStatus().deviceRestartInProgress, "Completed device restart should clear the restart flag.");
        require(contract.getStatus().performance.available && contract.getStatus().performance.revision == 5
                    && contract.getStatus().performance.state == "Active",
                "Successful device restart should restore the published revision as the active context.");
        require(contract.getStatus().performance.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::active,
                "Successful device restart should restore the active lifecycle state.");
        require(contract.getStatus().preview.available && contract.getStatus().preview.revision == 5
                    && contract.getStatus().preview.state == "Stale",
                "Successful device restart should preserve preview identity while acknowledging the newer draft revision.");
        require(contract.getStatus().preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Successful device restart should preserve the last good preview lifecycle state.");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migrated contract coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before contract coverage runs.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        drs::engine::DraftPlaybackContract migratedContract(migratedSession.getDocumentState().revision);
        drs::engine::PlaybackSnapshotBuilder migratedSnapshotBuilder;
        drs::engine::PreparedPlaybackService migratedPreparedService;

        const auto migratedInitialPreview = migratedContract.requestPreviewBuild();
        require(migratedInitialPreview.accepted,
                "Migrated project preview request should be accepted before imported zones exist.");
        const auto migratedInitialPreviewBuild = buildPrepared(migratedSnapshotBuilder,
                                                               migratedPreparedService,
                                                               referenceStream,
                                                               migratedSession.getProject(),
                                                               migratedSession.getDocumentState().revision,
                                                               false);
        require(migratedContract.completePreviewBuild(migratedInitialPreview.requestId,
                                                     migratedInitialPreviewBuild.snapshot,
                                                     migratedInitialPreviewBuild.prepared),
                "Migrated project preview rejection should still complete against the contract.");
        require(!migratedContract.getStatus().preview.available,
                "Migrated project without imported zones should not expose an available preview revision.");
        require(migratedContract.getStatus().preview.state
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Migrated project should preserve the rejected preview state before imported zones exist.");
        require(migratedContract.getStatus().preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Migrated project without imported zones should surface a failed preview lifecycle state.");
        require(containsFinding(migratedContract.getStatus().preview.findings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Migrated project preview rejection should surface the structured no-playable-zones finding.");

        const auto migratedInitialPublish = migratedContract.requestPerformanceBuild();
        require(migratedInitialPublish.accepted,
                "Migrated project publish request should be accepted before imported zones exist.");
        const auto migratedInitialPublishBuild = buildPrepared(migratedSnapshotBuilder,
                                                               migratedPreparedService,
                                                               referenceStream,
                                                               migratedSession.getProject(),
                                                               migratedSession.getDocumentState().revision,
                                                               true);
        require(migratedContract.completePerformanceBuild(migratedInitialPublish.requestId,
                                                          migratedInitialPublishBuild.snapshot,
                                                          migratedInitialPublishBuild.prepared),
                "Migrated project publish rejection should still complete against the contract.");
        require(!migratedContract.getStatus().performance.available,
                "Migrated project without imported zones should not expose an active published revision.");
        require(migratedContract.getStatus().performance.state
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Migrated project should preserve the rejected publish state before imported zones exist.");
        require(migratedContract.getStatus().performance.lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::failed,
                "Migrated project without imported zones should surface a failed published lifecycle state.");
        require(containsFinding(migratedContract.getStatus().performance.findings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Migrated project publish rejection should surface the structured no-playable-zones finding.");

        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "migrated-contract-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "migrated-contract-zone-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Migrated Contract Zone A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto migratedImport = migratedSession.appendImportedContent({ importedSampleSource },
                                                                          { importedZone },
                                                                          "Import migrated contract zone");
        require(migratedImport.applied, "Migrated project should accept imported authoring content for contract coverage.");
        require(migratedSession.selectZone(importedZone.id).applied,
                "Migrated contract coverage should explicitly select the imported transient zone.");
        require(migratedContract.setDraftRevision(migratedImport.documentState.revision),
                "Migrated contract should accept the imported draft revision.");

        const auto migratedPreviewRevision1 = migratedContract.requestPreviewBuild();
        require(migratedPreviewRevision1.accepted,
                "Migrated project preview should be accepted once imported zones exist.");
        const auto migratedPreviewRevision1Build = buildPrepared(migratedSnapshotBuilder,
                                                                 migratedPreparedService,
                                                                 referenceStream,
                                                                 migratedSession.getProject(),
                                                                 migratedImport.documentState.revision,
                                                                 false);
        require(migratedContract.completePreviewBuild(migratedPreviewRevision1.requestId,
                                                     migratedPreviewRevision1Build.snapshot,
                                                     migratedPreviewRevision1Build.prepared),
                "Migrated project preview should complete successfully after import.");
        require(migratedContract.getStatus().preview.available
                    && migratedContract.getStatus().preview.revision == migratedImport.documentState.revision
                    && migratedContract.getStatus().preview.state == "Ready",
                "Imported migrated project should expose a ready preview revision.");
        require(migratedContract.getStatus().preview.lifecycleState == drs::engine::PlaybackSnapshotLifecycleState::ready,
                "Imported migrated project should surface a ready preview lifecycle state.");
        require(migratedContract.getStatus().preview.preparedAssetsAvailable,
                "Imported migrated project preview should expose prepared playback assets.");
        require(migratedContract.getStatus().preview.preparedZoneCount == 1,
                "Imported migrated project preview should expose the imported playable zone.");
        require(migratedContract.getStatus().preview.playableRangeAvailable,
                "Imported migrated project preview should expose a playable-range summary.");
        require(migratedContract.getStatus().preview.lowestPlayableNote == 57
                    && migratedContract.getStatus().preview.highestPlayableNote == 57,
                "Imported migrated project preview should preserve the imported one-note playable range.");
        require(migratedContract.getStatus().preview.preparedSampleCount == migratedSession.getProject().sampleSources.size(),
                "Imported migrated project preview should materialize every migrated sample identity.");
        require(migratedContract.getStatus().preview.preparationCacheMissCount
                    == migratedSession.getProject().sampleSources.size(),
                "First successful migrated preview should cold-miss every prepared sample handle.");
        require(migratedContract.getStatus().preview.preparationCacheHitCount == 0,
                "First successful migrated preview should not report cache hits.");

        const auto migratedPublishRevision1 = migratedContract.requestPerformanceBuild();
        require(migratedPublishRevision1.accepted,
                "Migrated project publish should be accepted once imported zones exist.");
        const auto migratedPublishRevision1Build = buildPrepared(migratedSnapshotBuilder,
                                                                 migratedPreparedService,
                                                                 referenceStream,
                                                                 migratedSession.getProject(),
                                                                 migratedImport.documentState.revision,
                                                                 true);
        require(migratedContract.completePerformanceBuild(migratedPublishRevision1.requestId,
                                                          migratedPublishRevision1Build.snapshot,
                                                          migratedPublishRevision1Build.prepared),
                "Migrated project publish should complete successfully after import.");
        require(migratedContract.getStatus().performance.available
                    && migratedContract.getStatus().performance.revision == migratedImport.documentState.revision
                    && migratedContract.getStatus().performance.state == "Active",
                "Imported migrated project should expose an active published revision.");
        require(migratedContract.getStatus().performance.lifecycleState
                    == drs::engine::PlaybackSnapshotLifecycleState::active,
                "Imported migrated project should surface an active published lifecycle state.");
        require(migratedContract.getStatus().performance.preparedAssetsAvailable,
                "Imported migrated project publish should expose prepared playback assets.");
        require(migratedContract.getStatus().performance.preparationCacheHitCount
                    == migratedSession.getProject().sampleSources.size(),
                "Publishing the same imported migrated draft should reuse every prepared sample handle.");
        require(migratedContract.getStatus().performance.preparationCacheMissCount == 0,
                "Publishing the same imported migrated draft should not cold-miss prepared sample handles.");
        require(migratedContract.getStatus().performance.playableRangeAvailable,
                "Imported migrated project publish should expose a playable-range summary.");
        require(migratedContract.getStatus().performance.lowestPlayableNote == 57
                    && migratedContract.getStatus().performance.highestPlayableNote == 57,
                "Imported migrated project publish should preserve the imported one-note playable range.");
        require(migratedContract.getStatus().preview.contentDigest
                    == migratedContract.getStatus().performance.contentDigest,
                "Imported migrated preview and publish should share a snapshot digest for the same draft.");
        require(migratedContract.getStatus().preview.preparedContentDigest
                    == migratedContract.getStatus().performance.preparedContentDigest,
                "Imported migrated preview and publish should share a prepared digest for the same draft.");

        auto editedMigratedZone = *migratedSession.getSelectedZone();
        editedMigratedZone.gainDb = 2.5;
        editedMigratedZone.pan = -0.2;
        const auto migratedEdit = migratedSession.updateSelectedZone(editedMigratedZone,
                                                                     "Shape migrated contract zone");
        require(migratedEdit.applied, "Editing imported migrated content should commit successfully.");
        require(migratedContract.setDraftRevision(migratedEdit.documentState.revision),
                "Migrated contract should accept the edited draft revision.");
        require(migratedContract.getStatus().preview.revision == migratedImport.documentState.revision
                    && migratedContract.getStatus().preview.state == "Stale",
                "Editing imported migrated content should leave preview stale on the last prepared revision.");
        require(migratedContract.getStatus().performance.revision == migratedImport.documentState.revision
                    && migratedContract.getStatus().performance.state == "Active",
                "Editing imported migrated content should preserve the last active published revision.");

        const auto migratedPreviewRevision2 = migratedContract.requestPreviewBuild();
        require(migratedPreviewRevision2.accepted,
                "Edited migrated draft should accept a new preview request.");
        const auto migratedPreviewRevision2Build = buildPrepared(migratedSnapshotBuilder,
                                                                 migratedPreparedService,
                                                                 referenceStream,
                                                                 migratedSession.getProject(),
                                                                 migratedEdit.documentState.revision,
                                                                 false);
        require(migratedContract.completePreviewBuild(migratedPreviewRevision2.requestId,
                                                     migratedPreviewRevision2Build.snapshot,
                                                     migratedPreviewRevision2Build.prepared),
                "Edited migrated draft preview should complete successfully.");
        require(migratedContract.getStatus().preview.revision == migratedEdit.documentState.revision
                    && migratedContract.getStatus().preview.state == "Ready",
                "Edited migrated draft should advance preview to the current revision.");
        require(migratedContract.getStatus().preview.preparationCacheHitCount
                    == migratedSession.getProject().sampleSources.size(),
                "Zone-only migrated edits should reuse every prepared sample handle during preview.");
        require(migratedContract.getStatus().preview.preparationCacheMissCount == 0,
                "Zone-only migrated edits should not invalidate prepared sample handles during preview.");
        require(migratedContract.getStatus().preview.contentDigest
                    != migratedContract.getStatus().performance.contentDigest,
                "Edited migrated preview should diverge from the older published snapshot digest.");
        require(migratedContract.getStatus().preview.preparedContentDigest
                    != migratedContract.getStatus().performance.preparedContentDigest,
                "Edited migrated preview should diverge from the older published prepared digest.");

        const auto migratedPublishRevision2 = migratedContract.requestPerformanceBuild();
        require(migratedPublishRevision2.accepted,
                "Edited migrated draft should accept a publish request.");
        const auto migratedPublishRevision2Build = buildPrepared(migratedSnapshotBuilder,
                                                                 migratedPreparedService,
                                                                 referenceStream,
                                                                 migratedSession.getProject(),
                                                                 migratedEdit.documentState.revision,
                                                                 true);
        require(migratedContract.completePerformanceBuild(migratedPublishRevision2.requestId,
                                                          migratedPublishRevision2Build.snapshot,
                                                          migratedPublishRevision2Build.prepared),
                "Edited migrated draft publish should complete successfully.");
        require(migratedContract.getStatus().performance.revision == migratedEdit.documentState.revision
                    && migratedContract.getStatus().performance.state == "Active",
                "Edited migrated draft should advance the active published revision.");
        require(migratedContract.getStatus().performance.preparationCacheHitCount
                    == migratedSession.getProject().sampleSources.size(),
                "Publishing the edited migrated draft should reuse every prepared sample handle.");
        require(migratedContract.getStatus().performance.preparationCacheMissCount == 0,
                "Publishing the edited migrated draft should not invalidate prepared sample handles.");
        require(migratedContract.getStatus().preview.contentDigest
                    == migratedContract.getStatus().performance.contentDigest,
                "Publishing the edited migrated draft should realign preview and publish snapshot digests.");
        require(migratedContract.getStatus().preview.preparedContentDigest
                    == migratedContract.getStatus().performance.preparedContentDigest,
                "Publishing the edited migrated draft should realign preview and publish prepared digests.");

        std::cout << "Phase 1 draft-to-playback contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 draft-to-playback contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
