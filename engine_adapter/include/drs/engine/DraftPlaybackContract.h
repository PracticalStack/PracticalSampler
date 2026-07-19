#pragma once

#include "drs/engine/PreparedPlayback.h"
#include "drs/engine/PlaybackSnapshot.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace drs::engine
{
struct DraftPlaybackPreparedRevision
{
    bool available = false;
    std::size_t revision = 0;
    std::uint64_t buildId = 0;
    bool preparedAssetsAvailable = false;
    std::uint64_t preparedBuildId = 0;
    bool activationEligible = false;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::string contentDigest;
    std::string preparedContentDigest;
    std::string state;
    std::size_t preparedSampleCount = 0;
    std::size_t preparedStreamCount = 0;
    std::size_t preparedZoneCount = 0;
    std::size_t preparedOwnershipRecordCount = 0;
    std::uint64_t preparedBytes = 0;
    std::uint64_t preparedOwnershipBytes = 0;
    std::uint64_t preparedBuildDurationMicros = 0;
    std::uint64_t preparedDecodedBytes = 0;
    std::uint64_t preparedSampleDataBytes = 0;
    bool playableRangeAvailable = false;
    int lowestPlayableNote = 0;
    int highestPlayableNote = 127;
    std::size_t preparationCacheHitCount = 0;
    std::size_t preparationCacheMissCount = 0;
    std::vector<PlaybackSnapshotFinding> findings;
};

struct DraftPlaybackPendingRequest
{
    bool active = false;
    std::uint64_t requestId = 0;
    std::uint64_t cancellationId = 0;
    std::size_t requestedRevision = 0;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::string state;
};

struct DraftPlaybackBuildRequest
{
    bool accepted = false;
    std::uint64_t requestId = 0;
    std::uint64_t cancellationId = 0;
    std::size_t requestedRevision = 0;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::string state;
};

struct DraftPlaybackStatus
{
    bool projectOpen = true;
    bool deviceRestartInProgress = false;
    std::size_t draftRevision = 0;
    DraftPlaybackPreparedRevision preview;
    DraftPlaybackPreparedRevision performance;
    DraftPlaybackPendingRequest pendingPreview;
    DraftPlaybackPendingRequest pendingPerformance;
    std::string lastEvent;
};

class DraftPlaybackContract
{
public:
    explicit DraftPlaybackContract(std::size_t initialDraftRevision = 0);

    const DraftPlaybackStatus& getStatus() const { return status; }

    bool setDraftRevision(std::size_t revision);

    DraftPlaybackBuildRequest requestPreviewBuild();
    DraftPlaybackBuildRequest requestPerformanceBuild();

    bool completePreviewBuild(std::uint64_t requestId);
    bool completePerformanceBuild(std::uint64_t requestId);
    bool completePreviewBuild(std::uint64_t requestId, const PlaybackSnapshotBuildResult& buildResult);
    bool completePerformanceBuild(std::uint64_t requestId, const PlaybackSnapshotBuildResult& buildResult);
    bool completePreviewBuild(std::uint64_t requestId,
                              const PlaybackSnapshotBuildResult& snapshotResult,
                              const PreparedPlaybackBuildResult& preparedResult);
    bool completePerformanceBuild(std::uint64_t requestId,
                                  const PlaybackSnapshotBuildResult& snapshotResult,
                                  const PreparedPlaybackBuildResult& preparedResult);

    bool failPreviewBuild(std::uint64_t requestId,
                          const std::vector<std::string>& issues,
                          const std::string& state = "Preview preparation failed");
    bool failPerformanceBuild(std::uint64_t requestId,
                              const std::vector<std::string>& issues,
                              const std::string& state = "Publish preparation failed");

    bool cancelPreviewBuild(std::uint64_t requestId);
    bool cancelPerformanceBuild(std::uint64_t requestId);

    void closeProject();
    void reopenProject(std::size_t draftRevision);
    bool beginDeviceRestart();
    bool completeDeviceRestart(bool restored);

private:
    DraftPlaybackBuildRequest requestBuild(DraftPlaybackPendingRequest& pending,
                                           const std::string& kind,
                                           const std::string& state);
    bool completeBuild(DraftPlaybackPendingRequest& pending,
                       DraftPlaybackPreparedRevision& prepared,
                       const PlaybackSnapshotBuildResult* buildResult,
                       const PreparedPlaybackBuildResult* preparedBuildResult,
                       const std::string& completedState);
    bool failBuild(DraftPlaybackPendingRequest& pending,
                   DraftPlaybackPreparedRevision& prepared,
                   const std::vector<std::string>& issues,
                   const std::string& failedState);
    bool cancelBuild(DraftPlaybackPendingRequest& pending, const std::string& kind);
    void refreshPreparedStates();
    void resetPreparedRevision(DraftPlaybackPreparedRevision& prepared, const std::string& state);

    DraftPlaybackStatus status;
    std::uint64_t nextRequestId = 1;
};
} // namespace drs::engine
