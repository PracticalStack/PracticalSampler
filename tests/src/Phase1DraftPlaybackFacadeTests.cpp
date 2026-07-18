#include "drs/engine/EngineFacade.h"

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
    require(diagnosticsSnapshot.preparedWorkerRetiredBytes == performanceSnapshot.preparedWorkerRetiredBytes,
            context + " should keep diagnostics and performance snapshots aligned on retired prepared bytes.");
    require(diagnosticsSnapshot.preparedWorkerEvent == performanceSnapshot.preparedWorkerEvent,
            context + " should keep diagnostics and performance snapshots aligned on the latest worker event.");
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

        std::cout << "Phase 1 draft playback facade tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 draft playback facade tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
