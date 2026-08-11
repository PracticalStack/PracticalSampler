#pragma once

#include "drs/engine/PerformancePublishContract.h"
#include "drs/engine/PlaybackSnapshot.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace drs::engine
{
enum class PlaybackSnapshotWorkLane
{
    preview,
    performance
};

struct PlaybackSnapshotWorkerRequest
{
    PlaybackSnapshotWorkLane lane = PlaybackSnapshotWorkLane::preview;
    std::uint64_t contractRequestId = 0;
    PlaybackSnapshotBuildRequest snapshotRequest;
    PlaybackPreparationScopeRequest preparationScope;
    std::shared_ptr<const RuntimeProjectModel> project;
};

struct PlaybackSnapshotWorkerCompletion
{
    PlaybackSnapshotWorkLane lane = PlaybackSnapshotWorkLane::preview;
    std::uint64_t contractRequestId = 0;
    PlaybackSnapshotBuildResult snapshotResult;
    std::string macroSchemaDigest;
    std::size_t exposedMacroCount = 0;
    std::size_t hiddenMacroCount = 0;
    std::optional<PerformancePublishFinding> publishPreflightFinding;
};

class PlaybackSnapshotWorker final
{
public:
    PlaybackSnapshotWorker();
    ~PlaybackSnapshotWorker();

    bool submit(PlaybackSnapshotWorkerRequest request);
    std::vector<PlaybackSnapshotWorkerCompletion> drainCompleted();
    void cancelLane(PlaybackSnapshotWorkLane lane);

private:
    void run();

    std::mutex mutex;
    std::condition_variable condition;
    bool stopRequested = false;
    std::vector<PlaybackSnapshotWorkerRequest> queued;
    std::vector<PlaybackSnapshotWorkerCompletion> completed;
    std::thread worker;
};
} // namespace drs::engine
