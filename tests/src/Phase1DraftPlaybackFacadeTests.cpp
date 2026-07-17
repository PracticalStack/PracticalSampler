#include "drs/engine/EngineFacade.h"

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
        require(snapshot.loaded, "Resetting to default should seed an active published revision.");
        require(snapshot.draftRevision == 0, "Default draft revision should start at 0.");
        require(snapshot.previewRevision == 0, "Default preview revision should start at 0.");
        require(snapshot.publishedRevision == 0, "Default published revision should start at 0.");
        require(snapshot.previewRevisionState == "Ready",
                "Default preview revision should be ready immediately after seeding the reference runtime.");
        require(snapshot.publishedRevisionState == "Active",
                "Default published revision should be active immediately after seeding the reference runtime.");

        require(engineFacade.stageDraftRevision(1),
                "Engine facade should accept a staged draft revision.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.draftRevision == 1, "Staged draft revision should become visible through the performance snapshot.");
        require(snapshot.previewRevision == 0 && snapshot.previewRevisionState == "Stale",
                "Preview revision should become stale when the draft advances.");
        require(snapshot.publishedRevision == 0 && snapshot.publishedRevisionState == "Active",
                "Published revision should remain on the last applied version.");

        require(engineFacade.refreshPreviewToCurrentDraft(),
                "Engine facade should prepare preview for the current draft.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.previewRevision == 1 && snapshot.previewRevisionState == "Ready",
                "Preparing preview should advance the preview revision to the current draft.");
        require(snapshot.publishedRevision == 0,
                "Preparing preview must not change the published revision.");

        require(engineFacade.publishCurrentDraft(),
                "Engine facade should publish the prepared current draft.");
        snapshot = engineFacade.getPerformanceSnapshot();
        require(snapshot.publishedRevision == 1 && snapshot.publishedRevisionState == "Active",
                "Publishing should advance the published revision and mark it active.");
        require(snapshot.draftPlaybackEvent == "Build completed",
                "Publishing should expose the latest contract event.");

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
