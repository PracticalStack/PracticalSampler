#include "drs/engine/EngineFacade.h"

#include <chrono>
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

        require(engineFacade.stageDraftRevision(2),
                "A second draft revision should stage successfully.");
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
        require(snapshot.previewRevision == 1 && snapshot.previewRevisionState == "Stale",
                "Successful device restart should preserve preview identity while acknowledging the newer draft.");

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

        std::cout << "Phase 1 draft playback facade tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 draft playback facade tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
