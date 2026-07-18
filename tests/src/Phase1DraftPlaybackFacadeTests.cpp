#include "drs/engine/AuthoringSession.h"
#include "drs/engine/EngineFacade.h"
#include "drs/engine/RuntimeLoader.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

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

bool waitForWorkerToSettle(drs::engine::EngineFacade& engineFacade,
                           std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() <= deadline)
    {
        const auto& workerStatus = engineFacade.getPreparedPlaybackWorkerStatus();
        if (workerStatus.pendingWorkCount == 0 && workerStatus.inFlightWorkCount == 0)
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    const auto& workerStatus = engineFacade.getPreparedPlaybackWorkerStatus();
    return workerStatus.pendingWorkCount == 0 && workerStatus.inFlightWorkCount == 0;
}

bool waitForCondition(const std::function<bool()>& condition,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(250))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() <= deadline)
    {
        if (condition())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return condition();
}

void requireFacadeSnapshotConsistency(drs::engine::EngineFacade& engineFacade, const std::string& context)
{
    const auto performanceSnapshot = engineFacade.getPerformanceSnapshot();
    const auto diagnosticsSnapshot = engineFacade.getDiagnosticsSnapshot();
    const auto statusSnapshot = engineFacade.getStatusSnapshot();
    const auto& draftStatus = engineFacade.getDraftPlaybackStatus();

    require(performanceSnapshot.draftRevision == draftStatus.draftRevision,
            context + " should mirror the draft revision through the performance snapshot.");
    require(performanceSnapshot.previewRevision == draftStatus.preview.revision,
            context + " should mirror the preview revision through the performance snapshot.");
    require(performanceSnapshot.publishedRevision == draftStatus.performance.revision,
            context + " should mirror the published revision through the performance snapshot.");
    require(performanceSnapshot.previewBuildId == draftStatus.preview.buildId,
            context + " should mirror the preview snapshot build identity.");
    require(performanceSnapshot.publishedBuildId == draftStatus.performance.buildId,
            context + " should mirror the published snapshot build identity.");
    require(performanceSnapshot.previewPreparedBuildId == draftStatus.preview.preparedBuildId,
            context + " should mirror the preview prepared-playback build identity.");
    require(performanceSnapshot.publishedPreparedBuildId == draftStatus.performance.preparedBuildId,
            context + " should mirror the published prepared-playback build identity.");
    require(performanceSnapshot.previewActivationEligible == draftStatus.preview.activationEligible,
            context + " should mirror preview activation eligibility.");
    require(performanceSnapshot.publishedActivationEligible == draftStatus.performance.activationEligible,
            context + " should mirror published activation eligibility.");
    require(performanceSnapshot.previewContentDigest == draftStatus.preview.contentDigest,
            context + " should mirror the preview immutable snapshot digest.");
    require(performanceSnapshot.publishedContentDigest == draftStatus.performance.contentDigest,
            context + " should mirror the published immutable snapshot digest.");
    require(performanceSnapshot.previewPreparedContentDigest == draftStatus.preview.preparedContentDigest,
            context + " should mirror the preview prepared-playback digest.");
    require(performanceSnapshot.publishedPreparedContentDigest == draftStatus.performance.preparedContentDigest,
            context + " should mirror the published prepared-playback digest.");
    require(performanceSnapshot.previewPreparedOwnershipRecordCount == draftStatus.preview.preparedOwnershipRecordCount,
            context + " should mirror preview prepared ownership-record counts.");
    require(performanceSnapshot.publishedPreparedOwnershipRecordCount == draftStatus.performance.preparedOwnershipRecordCount,
            context + " should mirror published prepared ownership-record counts.");
    require(performanceSnapshot.previewPreparedOwnershipBytes == draftStatus.preview.preparedOwnershipBytes,
            context + " should mirror preview prepared ownership bytes.");
    require(performanceSnapshot.publishedPreparedOwnershipBytes == draftStatus.performance.preparedOwnershipBytes,
            context + " should mirror published prepared ownership bytes.");
    const auto expectedPlayableRangeAvailable = draftStatus.performance.playableRangeAvailable || draftStatus.preview.playableRangeAvailable;
    require(performanceSnapshot.playableRangeAvailable == expectedPlayableRangeAvailable,
            context + " should mirror whether a prepared draft-playback range is available.");
    if (draftStatus.performance.playableRangeAvailable && draftStatus.performance.available)
    {
        require(performanceSnapshot.playableRangeSource == "published",
                context + " should prefer the published playable range when it is available.");
        require(performanceSnapshot.lowestPlayableNote == draftStatus.performance.lowestPlayableNote,
                context + " should mirror the published lowest playable note.");
        require(performanceSnapshot.highestPlayableNote == draftStatus.performance.highestPlayableNote,
                context + " should mirror the published highest playable note.");
    }
    else if (draftStatus.preview.playableRangeAvailable && draftStatus.preview.available)
    {
        require(performanceSnapshot.playableRangeSource == "preview",
                context + " should fall back to the preview playable range when publish is unavailable.");
        require(performanceSnapshot.lowestPlayableNote == draftStatus.preview.lowestPlayableNote,
                context + " should mirror the preview lowest playable note.");
        require(performanceSnapshot.highestPlayableNote == draftStatus.preview.highestPlayableNote,
                context + " should mirror the preview highest playable note.");
    }
    else
    {
        require(performanceSnapshot.playableRangeSource == "default",
                context + " should report the default range source when no prepared draft-playback range exists.");
    }
    require(performanceSnapshot.surfaceStateSource == diagnosticsSnapshot.surfaceStateSource,
            context + " should keep diagnostics and performance snapshots aligned on surface provenance.");
    require(performanceSnapshot.rendererMode == diagnosticsSnapshot.rendererMode,
            context + " should keep diagnostics and performance snapshots aligned on renderer provenance.");
    require(diagnosticsSnapshot.playableRangeAvailable == performanceSnapshot.playableRangeAvailable,
            context + " should keep diagnostics and performance snapshots aligned on playable-range availability.");
    require(diagnosticsSnapshot.lowestPlayableNote == performanceSnapshot.lowestPlayableNote,
            context + " should keep diagnostics and performance snapshots aligned on the lowest playable note.");
    require(diagnosticsSnapshot.highestPlayableNote == performanceSnapshot.highestPlayableNote,
            context + " should keep diagnostics and performance snapshots aligned on the highest playable note.");
    require(diagnosticsSnapshot.playableRangeSource == performanceSnapshot.playableRangeSource,
            context + " should keep diagnostics and performance snapshots aligned on playable-range provenance.");
    require(performanceSnapshot.draftPlaybackEvent == draftStatus.lastEvent,
            context + " should mirror the latest draft-playback event.");
    require(performanceSnapshot.previewFindings.size() == draftStatus.preview.findings.size(),
            context + " should mirror preview findings.");
    require(performanceSnapshot.publishedFindings.size() == draftStatus.performance.findings.size(),
            context + " should mirror published findings.");

    require(diagnosticsSnapshot.draftRevision == performanceSnapshot.draftRevision,
            context + " should keep diagnostics and performance snapshots on the same draft revision.");
    require(diagnosticsSnapshot.previewRevision == performanceSnapshot.previewRevision,
            context + " should keep diagnostics and performance snapshots on the same preview revision.");
    require(diagnosticsSnapshot.publishedRevision == performanceSnapshot.publishedRevision,
            context + " should keep diagnostics and performance snapshots on the same published revision.");
    require(diagnosticsSnapshot.previewBuildId == performanceSnapshot.previewBuildId,
            context + " should keep diagnostics and performance snapshots on the same preview build identity.");
    require(diagnosticsSnapshot.publishedBuildId == performanceSnapshot.publishedBuildId,
            context + " should keep diagnostics and performance snapshots on the same published build identity.");
    require(diagnosticsSnapshot.previewPreparedBuildId == performanceSnapshot.previewPreparedBuildId,
            context + " should keep diagnostics and performance snapshots on the same preview prepared build identity.");
    require(diagnosticsSnapshot.publishedPreparedBuildId == performanceSnapshot.publishedPreparedBuildId,
            context + " should keep diagnostics and performance snapshots on the same published prepared build identity.");
    require(diagnosticsSnapshot.previewActivationEligible == performanceSnapshot.previewActivationEligible,
            context + " should keep diagnostics and performance snapshots aligned on preview activation eligibility.");
    require(diagnosticsSnapshot.publishedActivationEligible == performanceSnapshot.publishedActivationEligible,
            context + " should keep diagnostics and performance snapshots aligned on published activation eligibility.");
    require(diagnosticsSnapshot.previewContentDigest == performanceSnapshot.previewContentDigest,
            context + " should keep diagnostics and performance snapshots aligned on preview digest.");
    require(diagnosticsSnapshot.publishedContentDigest == performanceSnapshot.publishedContentDigest,
            context + " should keep diagnostics and performance snapshots aligned on published digest.");
    require(diagnosticsSnapshot.previewPreparedContentDigest == performanceSnapshot.previewPreparedContentDigest,
            context + " should keep diagnostics and performance snapshots aligned on preview prepared digest.");
    require(diagnosticsSnapshot.publishedPreparedContentDigest == performanceSnapshot.publishedPreparedContentDigest,
            context + " should keep diagnostics and performance snapshots aligned on published prepared digest.");
    require(diagnosticsSnapshot.preparedWorkerPendingCount == performanceSnapshot.preparedWorkerPendingCount,
            context + " should keep diagnostics and performance snapshots aligned on pending worker count.");
    require(diagnosticsSnapshot.preparedWorkerCancellationCount == performanceSnapshot.preparedWorkerCancellationCount,
            context + " should keep diagnostics and performance snapshots aligned on worker cancellation count.");
    require(diagnosticsSnapshot.preparedWorkerSupersededCount == performanceSnapshot.preparedWorkerSupersededCount,
            context + " should keep diagnostics and performance snapshots aligned on worker supersede count.");
    require(diagnosticsSnapshot.preparedWorkerFailureCount == performanceSnapshot.preparedWorkerFailureCount,
            context + " should keep diagnostics and performance snapshots aligned on worker failure count.");
    require(diagnosticsSnapshot.preparedWorkerActiveOwnershipRecordCount
                == performanceSnapshot.preparedWorkerActiveOwnershipRecordCount,
            context + " should keep diagnostics and performance snapshots aligned on active ownership-record count.");
    require(diagnosticsSnapshot.preparedWorkerRetiredOwnershipRecordCount
                == performanceSnapshot.preparedWorkerRetiredOwnershipRecordCount,
            context + " should keep diagnostics and performance snapshots aligned on retired ownership-record count.");
    require(diagnosticsSnapshot.preparedWorkerRetiredBytes == performanceSnapshot.preparedWorkerRetiredBytes,
            context + " should keep diagnostics and performance snapshots aligned on retired prepared bytes.");
    require(diagnosticsSnapshot.preparedWorkerEvent == performanceSnapshot.preparedWorkerEvent,
            context + " should keep diagnostics and performance snapshots aligned on the latest worker event.");
    require(diagnosticsSnapshot.previewPreparedOwnershipRecordCount
                == performanceSnapshot.previewPreparedOwnershipRecordCount,
            context + " should keep diagnostics and performance snapshots aligned on preview ownership-record counts.");
    require(diagnosticsSnapshot.publishedPreparedOwnershipRecordCount
                == performanceSnapshot.publishedPreparedOwnershipRecordCount,
            context + " should keep diagnostics and performance snapshots aligned on published ownership-record counts.");
    require(diagnosticsSnapshot.previewPreparedOwnershipBytes == performanceSnapshot.previewPreparedOwnershipBytes,
            context + " should keep diagnostics and performance snapshots aligned on preview ownership bytes.");
    require(diagnosticsSnapshot.publishedPreparedOwnershipBytes == performanceSnapshot.publishedPreparedOwnershipBytes,
            context + " should keep diagnostics and performance snapshots aligned on published ownership bytes.");
    require(statusSnapshot.detail.find("Snapshot ids:") != std::string::npos,
            context + " should keep the shell-facing contract line for snapshot ids.");
    require(statusSnapshot.detail.find("Snapshot digests:") != std::string::npos,
            context + " should keep the shell-facing contract line for snapshot digests.");
    require(statusSnapshot.detail.find("Prepared playback assets:") != std::string::npos,
            context + " should keep the shell-facing contract line for prepared asset counts.");
    require(statusSnapshot.detail.find("preview ownership=") != std::string::npos,
            context + " should keep the shell-facing contract line for prepared ownership counts.");
    require(statusSnapshot.detail.find("activeOwnership=") != std::string::npos,
            context + " should keep the shell-facing contract line for worker ownership backlog.");
    require(statusSnapshot.detail.find("Playable range:") != std::string::npos,
            context + " should keep the shell-facing contract line for playable-range summaries.");
    require(statusSnapshot.detail.find("Surface provenance:") != std::string::npos,
            context + " should keep the shell-facing contract line for surface provenance.");
    require(statusSnapshot.detail.find("source=" + performanceSnapshot.surfaceStateSource) != std::string::npos,
            context + " should report the current surface provenance in the shell-facing status detail.");
    require(statusSnapshot.detail.find("renderer=" + performanceSnapshot.rendererMode) != std::string::npos,
            context + " should report the current renderer provenance in the shell-facing status detail.");
}
} // namespace

int main()
{
    try
    {
        drs::engine::EngineFacade engineFacade;

        require(engineFacade.getArticulationDescriptors().size() == 2,
                "Engine facade should expose the default reference articulations once the reference runtime loads.");

        engineFacade.resetSessionStateToDefault();
        auto snapshot = engineFacade.getPerformanceSnapshot();
        auto draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.loaded, "Resetting to default should seed an active published revision.");
        require(snapshot.draftRevision == 0, "Default draft revision should start at 0.");
        require(snapshot.previewRevision == 0, "Default preview revision should start at 0.");
        require(snapshot.publishedRevision == 0, "Default published revision should start at 0.");
        require(snapshot.previewRevisionState == "Ready",
                "Default preview revision should be ready immediately after seeding the reference runtime.");
        require(snapshot.publishedRevisionState == "Active",
                "Default published revision should be active immediately after seeding the reference runtime.");
        require(snapshot.previewBuildId != 0 && snapshot.publishedBuildId != 0,
                "Default preview and publish revisions should expose non-zero snapshot build ids.");
        require(!snapshot.previewContentDigest.empty() && !snapshot.publishedContentDigest.empty(),
                "Default preview and publish revisions should expose snapshot digests through the performance snapshot.");
        require(snapshot.previewPreparedBuildId != 0 && snapshot.publishedPreparedBuildId != 0,
                "Default preview and publish revisions should expose non-zero prepared playback build ids.");
        require(!snapshot.previewPreparedContentDigest.empty() && !snapshot.publishedPreparedContentDigest.empty(),
                "Default preview and publish revisions should expose prepared playback digests.");
        require(snapshot.previewPreparedSampleCount > 0 && snapshot.publishedPreparedSampleCount > 0,
                "Default preview and publish revisions should expose prepared sample counts.");
        require(snapshot.preparedWorkerPendingCount == 0,
                "Default preview and publish revisions should not leave prepared worker jobs pending.");
        require(snapshot.previewFindings.empty() && snapshot.publishedFindings.empty(),
                "Default preview and publish revisions should not expose findings for the reference project.");
        require(!draftStatus.preview.contentDigest.empty(),
                "Default preview revision should carry a playback snapshot digest.");
        require(draftStatus.preview.contentDigest == draftStatus.performance.contentDigest,
                "Default preview and publish revisions should share a digest for the same draft.");
        require(snapshot.previewActivationEligible && snapshot.publishedActivationEligible,
                "Default preview and publish revisions should both remain activation-eligible.");
        requireFacadeSnapshotConsistency(engineFacade, "Default facade state");
        const auto initialPreviewBuildId = snapshot.previewBuildId;
        const auto initialPublishedBuildId = snapshot.publishedBuildId;
        const auto initialPreviewPreparedBuildId = snapshot.previewPreparedBuildId;
        const auto initialPublishedPreparedBuildId = snapshot.publishedPreparedBuildId;

        require(engineFacade.stageDraftRevision(1),
                "Engine facade should accept a staged draft revision.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.draftRevision == 1, "Staged draft revision should become visible through the performance snapshot.");
        require(snapshot.previewRevision == 0 && snapshot.previewRevisionState == "Stale",
                "Preview revision should become stale when the draft advances.");
        require(snapshot.publishedRevision == 0 && snapshot.publishedRevisionState == "Active",
                "Published revision should remain on the last applied version.");
        const auto revision0Digest = draftStatus.performance.contentDigest;

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Engine facade should prepare preview for the current draft.");
        require(waitForWorkerToSettle(engineFacade),
                "Prepared-playback worker should finish the queued preview request.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.previewPending,
                "Preview contract state should remain pending until message-thread background work is serviced.");
        require(engineFacade.serviceBackgroundWork(),
                "Explicit background-work servicing should apply the completed preview build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.previewRevision == 1 && snapshot.previewRevisionState == "Ready",
                "Preparing preview should advance the preview revision to the current draft.");
        require(snapshot.publishedRevision == 0,
                "Preparing preview must not change the published revision.");
        require(snapshot.previewBuildId != 0,
                "Preparing preview should refresh the preview snapshot build identity.");
        require(snapshot.previewPreparedBuildId != 0,
                "Preparing preview should refresh the preview prepared playback build identity.");
        require(draftStatus.preview.contentDigest != revision0Digest,
                "Preparing a newer draft revision should produce a different snapshot digest.");
        require(snapshot.previewBuildId != initialPreviewBuildId,
                "Preparing a newer preview revision should replace the preview snapshot build identity.");
        require(snapshot.previewPreparedBuildId != initialPreviewPreparedBuildId,
                "Preparing a newer preview revision should replace the preview prepared-playback build identity.");
        require(snapshot.previewActivationEligible,
                "Prepared preview revisions should remain activation-eligible through the facade snapshot.");
        requireFacadeSnapshotConsistency(engineFacade, "Preview facade state after revision 1");

        require(engineFacade.publishCurrentDraft(),
                "Engine facade should publish the prepared current draft.");
        require(waitForWorkerToSettle(engineFacade),
                "Prepared-playback worker should finish the queued publish request.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.publishedPending,
                "Publish contract state should remain pending until message-thread background work is serviced.");
        require(engineFacade.serviceBackgroundWork(),
                "Explicit background-work servicing should apply the completed publish build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Publishing should advance the published revision and mark it active.");
        require(snapshot.draftPlaybackEvent == "Build completed",
                "Publishing should expose the latest contract event.");
        require(snapshot.previewContentDigest == snapshot.publishedContentDigest,
                "Publishing the current draft should surface matching digests through the performance snapshot.");
        require(snapshot.previewPreparedContentDigest == snapshot.publishedPreparedContentDigest,
                "Publishing the current draft should surface matching prepared digests.");
        require(draftStatus.preview.contentDigest == draftStatus.performance.contentDigest,
                "Publishing the current preview should preserve digest identity across both paths.");
        require(snapshot.publishedBuildId != initialPublishedBuildId,
                "Publishing a newer draft should replace the published snapshot build identity.");
        require(snapshot.publishedPreparedBuildId != initialPublishedPreparedBuildId,
                "Publishing a newer draft should replace the published prepared-playback build identity.");
        require(snapshot.publishedActivationEligible,
                "Published revisions should remain activation-eligible through the facade snapshot.");
        requireFacadeSnapshotConsistency(engineFacade, "Published facade state after revision 1");

        const auto revision1PublishedBuildId = snapshot.publishedBuildId;
        const auto revision1PublishedPreparedBuildId = snapshot.publishedPreparedBuildId;

        require(engineFacade.stageDraftRevision(2),
                "A second draft revision should stage successfully.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Revision 2 preview refresh should be accepted.");
        require(engineFacade.stageDraftRevision(3),
                "A third draft revision should stage successfully.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Revision 3 preview refresh should be accepted.");
        require(engineFacade.stageDraftRevision(4),
                "A fourth draft revision should stage successfully.");
        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Revision 4 preview refresh should be accepted.");
        require(engineFacade.waitForPreparedPlaybackIdle(std::chrono::milliseconds(1500)),
                "Rapid preview refreshes should settle through the facade idle wait.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.draftRevision == 4,
                "Rapid preview refreshes should leave the newest draft revision visible.");
        require(snapshot.previewRevision == 4 && snapshot.previewRevisionState == "Ready",
                "Rapid preview refreshes should leave the newest preview revision ready.");
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Rapid preview refreshes should not change the last published revision.");
        require(snapshot.previewBuildId != initialPreviewBuildId,
                "Rapid preview refreshes should keep advancing the preview snapshot build identity.");
        require(snapshot.previewPreparedBuildId != initialPreviewPreparedBuildId,
                "Rapid preview refreshes should keep advancing the preview prepared-playback build identity.");
        require(snapshot.publishedBuildId == revision1PublishedBuildId,
                "Rapid preview refreshes should preserve the last published snapshot build identity.");
        require(snapshot.publishedPreparedBuildId == revision1PublishedPreparedBuildId,
                "Rapid preview refreshes should preserve the last published prepared-playback build identity.");
        require(snapshot.previewActivationEligible && snapshot.publishedActivationEligible,
                "Rapid preview refreshes should preserve activation eligibility for both facade paths.");
        requireFacadeSnapshotConsistency(engineFacade, "Facade state after rapid preview supersede coverage");

        require(engineFacade.beginDraftPlaybackDeviceRestart(),
                "Device restart should begin while the project is open.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.previewRevisionState == "Restarting",
                "Preview revision state should surface restarting during device restart.");
        require(snapshot.publishedRevisionState == "Restarting",
                "Published revision state should surface restarting during device restart.");
        require(engineFacade.completeDraftPlaybackDeviceRestart(true),
                "Successful device restart should complete.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Successful device restart should preserve the last published revision.");
        require(snapshot.previewRevision == 4 && snapshot.previewRevisionState == "Ready",
                "Successful device restart should preserve the latest prepared preview identity for the current draft.");
        requireFacadeSnapshotConsistency(engineFacade, "Facade state after device restart");

        engineFacade.closeDraftPlaybackProject();
        snapshot = engineFacade.getPerformanceSnapshot();
        require(!snapshot.loaded, "Closing the draft playback project should unload the published performance context.");
        require(snapshot.previewRevisionState == "Closed",
                "Closing the project should surface the closed preview state.");
        require(snapshot.publishedRevisionState == "Closed",
                "Closing the project should surface the closed published state.");

        require(engineFacade.reopenDraftPlaybackProject(2),
                "Reopening the draft playback project should reactivate the facade.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.draftRevision == 2, "Reopening should restore the provided draft revision.");
        require(snapshot.previewRevision == 0 && snapshot.previewRevisionState == "Idle",
                "Reopening should require preview preparation again.");
        require(snapshot.publishedRevision == 0 && snapshot.publishedRevisionState == "Idle",
                "Reopening should require publish activation again.");
        requireFacadeSnapshotConsistency(engineFacade, "Facade state after reopen");

        const auto phase1Project = drs::engine::loadPhase1ReferenceProjectManifest();
        require(phase1Project.loaded, "Phase 1 reference project must load before migrated facade coverage runs.");
        const auto migratedProject = drs::engine::migrateRuntimeProjectToPhase2Authoring(phase1Project.project);
        require(migratedProject.valid, "Phase 1 reference project should migrate before facade coverage runs.");

        drs::engine::AuthoringSession migratedSession(migratedProject.project);
        engineFacade.closeDraftPlaybackProject();
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept a validated migrated authoring project replacement.");
        require(engineFacade.reopenDraftPlaybackProject(0),
                "Engine facade should reopen against the migrated authoring project.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.draftRevision == 0,
                "Migrated facade coverage should reopen at draft revision 0.");
        require(snapshot.previewRevision == 0 && snapshot.previewRevisionState == "Idle",
                "Migrated facade coverage should begin with an idle preview state.");
        require(snapshot.publishedRevision == 0 && snapshot.publishedRevisionState == "Idle",
                "Migrated facade coverage should begin with an idle published state.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after reopen");

        require(!engineFacade.refreshPreviewToCurrentDraft(),
                "Migrated project without imported zones should fail preview preparation through the facade.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.previewRevision == 0,
                "Failed migrated preview should not advance the prepared preview revision.");
        require(snapshot.previewRevisionState
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Failed migrated preview should surface the rejected prepared-playback state.");
        require(!snapshot.previewActivationEligible,
                "Failed migrated preview should never become activation-eligible.");
        require(containsFinding(snapshot.previewFindings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Failed migrated preview should surface the structured no-playable-zones finding.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after rejected preview");

        require(!engineFacade.publishCurrentDraft(),
                "Migrated project without imported zones should fail publish preparation through the facade.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(!snapshot.loaded,
                "Failed migrated publish should not expose a loaded published performance context.");
        require(snapshot.publishedRevision == 0,
                "Failed migrated publish should not advance the published revision.");
        require(snapshot.publishedRevisionState
                    == "Prepared playback build rejected because the immutable snapshot is unavailable",
                "Failed migrated publish should surface the rejected prepared-playback state.");
        require(!snapshot.publishedActivationEligible,
                "Failed migrated publish should never become activation-eligible.");
        require(containsFinding(snapshot.publishedFindings,
                                drs::engine::PlaybackSnapshotFindingSeverity::error,
                                "no-playable-zones",
                                "authoring.zones"),
                "Failed migrated publish should surface the structured no-playable-zones finding.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after rejected publish");

        drs::engine::RuntimeProjectSampleSource importedSampleSource;
        importedSampleSource.id = "migrated-facade-sine-a3";
        importedSampleSource.path = phase1Project.project.sampleSources[0].path;
        importedSampleSource.role = "imported-sustain";

        drs::engine::RuntimeProjectZoneDefinition importedZone;
        importedZone.id = "migrated-facade-zone-a3";
        importedZone.sampleSourceId = importedSampleSource.id;
        importedZone.displayName = "Migrated Facade Zone A3";
        importedZone.groupId = "main";
        importedZone.articulationId = "sustain";
        importedZone.rootKey = 57;
        importedZone.keyLow = 57;
        importedZone.keyHigh = 57;
        importedZone.velocityLow = 1;
        importedZone.velocityHigh = 127;

        const auto migratedImport = migratedSession.appendImportedContent({ importedSampleSource },
                                                                          { importedZone },
                                                                          "Import migrated facade zone");
        require(migratedImport.applied, "Migrated facade coverage should accept imported authoring content.");
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept the imported migrated authoring project update.");
        require(engineFacade.stageDraftRevision(migratedImport.documentState.revision),
                "Engine facade should stage the imported migrated draft revision.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.draftRevision == 1,
                "Imported migrated facade coverage should stage draft revision 1.");

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Imported migrated project should prepare preview successfully through the facade.");
        require(waitForWorkerToSettle(engineFacade),
                "Imported migrated preview should settle through the prepared-playback worker.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.previewPending,
                "Imported migrated preview should remain pending until background work is serviced.");
        require(engineFacade.serviceBackgroundWork(),
                "Background work servicing should apply the imported migrated preview build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.previewRevision == 1 && snapshot.previewRevisionState == "Ready",
                "Imported migrated project should expose a ready preview revision.");
        require(snapshot.previewPreparedSampleCount == migratedSession.getProject().sampleSources.size(),
                "Imported migrated preview should materialize every migrated sample identity.");
        require(snapshot.previewPreparationCacheHits + snapshot.previewPreparationCacheMisses
                    == migratedSession.getProject().sampleSources.size(),
                "Imported migrated facade preview should account for every prepared sample handle as either a hit or miss.");
        require(snapshot.previewPreparationCacheMisses >= 1,
                "Imported migrated facade preview should cold-miss at least the newly imported sample handle.");
        require(snapshot.previewActivationEligible,
                "Imported migrated preview should become activation-eligible.");
        require(snapshot.playableRangeAvailable && snapshot.playableRangeSource == "preview",
                "Imported migrated preview should surface a preview-derived playable range before publish.");
        require(snapshot.lowestPlayableNote == 57 && snapshot.highestPlayableNote == 57,
                "Imported migrated preview should surface the imported one-note playable range.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after imported preview");

        require(engineFacade.publishCurrentDraft(),
                "Imported migrated project should publish successfully through the facade.");
        require(waitForWorkerToSettle(engineFacade),
                "Imported migrated publish should settle through the prepared-playback worker.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.publishedPending,
                "Imported migrated publish should remain pending until background work is serviced.");
        require(engineFacade.serviceBackgroundWork(),
                "Background work servicing should apply the imported migrated publish build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.loaded,
                "Imported migrated publish should expose a loaded performance context.");
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Imported migrated project should expose an active published revision.");
        require(snapshot.previewContentDigest == snapshot.publishedContentDigest,
                "Imported migrated preview and publish should share a snapshot digest.");
        require(snapshot.previewPreparedContentDigest == snapshot.publishedPreparedContentDigest,
                "Imported migrated preview and publish should share a prepared digest.");
        require(snapshot.publishedPreparationCacheHits == migratedSession.getProject().sampleSources.size(),
                "Publishing the imported migrated draft should reuse every prepared sample handle.");
        require(snapshot.publishedPreparationCacheMisses == 0,
                "Publishing the imported migrated draft should not cold-miss prepared sample handles.");
        require(snapshot.publishedActivationEligible,
                "Imported migrated publish should become activation-eligible.");
        require(snapshot.playableRangeAvailable && snapshot.playableRangeSource == "published",
                "Imported migrated publish should prefer the published playable range.");
        require(snapshot.lowestPlayableNote == 57 && snapshot.highestPlayableNote == 57,
                "Imported migrated publish should preserve the imported one-note playable range.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after imported publish");

        auto editedMigratedZone = *migratedSession.getSelectedZone();
        editedMigratedZone.gainDb = 2.5;
        editedMigratedZone.pan = -0.2;
        const auto migratedEdit = migratedSession.updateSelectedZone(editedMigratedZone,
                                                                     "Shape migrated facade zone");
        require(migratedEdit.applied, "Editing the imported migrated zone should commit successfully.");
        require(engineFacade.replaceDraftPlaybackAuthoringProject(migratedSession.getProject()),
                "Engine facade should accept the edited migrated authoring project update.");
        require(engineFacade.stageDraftRevision(migratedEdit.documentState.revision),
                "Engine facade should stage the edited migrated draft revision.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.previewRevision == 1 && snapshot.previewRevisionState == "Stale",
                "Editing imported migrated content should leave preview stale on the prior prepared revision.");
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Editing imported migrated content should preserve the prior active published revision.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after edited draft staging");

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Edited migrated draft should prepare preview successfully through the facade.");
        require(waitForWorkerToSettle(engineFacade),
                "Edited migrated preview should settle through the prepared-playback worker.");
        require(engineFacade.serviceBackgroundWork(),
                "Background work servicing should apply the edited migrated preview build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.previewRevision == 2 && snapshot.previewRevisionState == "Ready",
                "Edited migrated draft should advance preview to the current revision.");
        require(snapshot.previewPreparationCacheHits == migratedSession.getProject().sampleSources.size(),
                "Zone-only migrated facade edits should reuse every prepared sample handle during preview.");
        require(snapshot.previewPreparationCacheMisses == 0,
                "Zone-only migrated facade edits should not invalidate prepared sample handles during preview.");
        require(snapshot.previewContentDigest != snapshot.publishedContentDigest,
                "Edited migrated preview should diverge from the older published snapshot digest.");
        require(snapshot.previewPreparedContentDigest != snapshot.publishedPreparedContentDigest,
                "Edited migrated preview should diverge from the older published prepared digest.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after edited preview");

        require(engineFacade.publishCurrentDraft(),
                "Edited migrated draft should publish successfully through the facade.");
        require(waitForWorkerToSettle(engineFacade),
                "Edited migrated publish should settle through the prepared-playback worker.");
        require(engineFacade.serviceBackgroundWork(),
                "Background work servicing should apply the edited migrated publish build.");
        snapshot = engineFacade.getPerformanceSnapshot();
        draftStatus = engineFacade.getDraftPlaybackStatus();
        require(snapshot.publishedRevision == 2 && snapshot.publishedRevisionState == "Active",
                "Edited migrated draft should advance the active published revision.");
        require(snapshot.publishedPreparationCacheHits == migratedSession.getProject().sampleSources.size(),
                "Publishing the edited migrated draft should reuse every prepared sample handle.");
        require(snapshot.publishedPreparationCacheMisses == 0,
                "Publishing the edited migrated draft should not invalidate prepared sample handles.");
        require(snapshot.previewContentDigest == snapshot.publishedContentDigest,
                "Publishing the edited migrated draft should realign preview and publish snapshot digests.");
        require(snapshot.previewPreparedContentDigest == snapshot.publishedPreparedContentDigest,
                "Publishing the edited migrated draft should realign preview and publish prepared digests.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after edited publish");

        auto invalidMigratedProject = migratedSession.getProject();
        invalidMigratedProject.authoring.zones[0].sampleSourceId = "missing-source";
        const auto stableStateRevision = engineFacade.getStateRevision();
        const auto stableSnapshot = engineFacade.getPerformanceSnapshot();
        require(!engineFacade.replaceDraftPlaybackAuthoringProject(invalidMigratedProject),
                "Engine facade should reject invalid authoring project replacements.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(engineFacade.getStateRevision() == stableStateRevision,
                "Rejected authoring project replacements must not mutate the facade state revision.");
        require(snapshot.draftRevision == stableSnapshot.draftRevision,
                "Rejected authoring project replacements must preserve the staged draft revision.");
        require(snapshot.previewRevision == stableSnapshot.previewRevision
                    && snapshot.previewRevisionState == stableSnapshot.previewRevisionState,
                "Rejected authoring project replacements must preserve preview state.");
        require(snapshot.publishedRevision == stableSnapshot.publishedRevision
                    && snapshot.publishedRevisionState == stableSnapshot.publishedRevisionState,
                "Rejected authoring project replacements must preserve published state.");
        require(snapshot.previewContentDigest == stableSnapshot.previewContentDigest
                    && snapshot.publishedContentDigest == stableSnapshot.publishedContentDigest,
                "Rejected authoring project replacements must preserve snapshot digests.");
        require(snapshot.previewPreparedContentDigest == stableSnapshot.previewPreparedContentDigest
                    && snapshot.publishedPreparedContentDigest == stableSnapshot.publishedPreparedContentDigest,
                "Rejected authoring project replacements must preserve prepared-playback digests.");
        requireFacadeSnapshotConsistency(engineFacade, "Migrated facade state after rejected replacement");

        std::cout << "Phase 1 draft playback facade tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 draft playback facade tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
