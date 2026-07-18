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
        require(contract.getStatus().performance.available && contract.getStatus().performance.revision == 0,
                "Initial publish build should track revision 0.");
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
        require(contract.getStatus().performance.revision == 0,
                "Published revision must not change before Apply finishes.");
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
        require(contract.getStatus().preview.available && contract.getStatus().preview.revision == 5
                    && contract.getStatus().preview.state == "Stale",
                "Successful device restart should preserve preview identity while acknowledging the newer draft revision.");

        std::cout << "Phase 1 draft-to-playback contract tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 draft-to-playback contract tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
