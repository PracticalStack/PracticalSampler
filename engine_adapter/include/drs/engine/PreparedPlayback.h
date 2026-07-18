#pragma once

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeStream.h"

#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace drs::engine
{
struct PreparedPlaybackPageHandle
{
    std::uint32_t pageIndex = 0;
    std::uint64_t offsetBytes = 0;
    std::uint64_t sizeBytes = 0;
};

struct PreparedPlaybackSampleHandle
{
    std::string sampleSourceId;
    std::string streamSampleId;
    std::string sourcePath;
    std::string sourceFingerprintHex;
    std::string formatName;
    std::string role;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    bool rootMidiNotePresent = false;
    int rootMidiNote = 60;
    bool loopRangePresent = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::string ownershipToken;
    std::string cacheKey;
};

struct PreparedPlaybackStreamHandle
{
    std::string sampleSourceId;
    std::string streamSampleId;
    std::string containerId;
    std::string containerPath;
    std::string payloadEncoding;
    std::uint64_t pageSizeBytes = 0;
    std::uint64_t payloadOffsetBytes = 0;
    std::uint64_t payloadSizeBytes = 0;
    std::uint64_t prefetchBytes = 0;
    std::string ownershipToken;
    std::string cacheKey;
    std::vector<PreparedPlaybackPageHandle> pages;
};

struct PreparedPlaybackZoneHandle
{
    std::string zoneId;
    std::string sampleSourceId;
    std::string streamSampleId;
    std::size_t preparedSampleIndex = 0;
    std::size_t preparedStreamIndex = 0;
    int rootKey = 60;
    int keyLow = 0;
    int keyHigh = 127;
    int velocityLow = 1;
    int velocityHigh = 127;
    double gainDb = 0.0;
    double pan = 0.0;
    std::uint64_t sampleStartFrame = 0;
    bool loopEnabled = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
};

struct ImmutablePreparedPlayback
{
    std::uint64_t snapshotBuildId = 0;
    std::string snapshotContentDigest;
    std::string compilerVersion;
    std::size_t draftRevision = 0;
    std::string containerId;
    std::string containerPath;
    std::string payloadEncoding;
    std::uint64_t pageSizeBytes = 0;
    std::string preparedContentDigest;
    std::vector<PreparedPlaybackSampleHandle> samples;
    std::vector<PreparedPlaybackStreamHandle> streams;
    std::vector<PreparedPlaybackZoneHandle> zones;
    std::vector<std::string> notes;
};

struct PreparedPlaybackMetrics
{
    std::size_t preparedSampleCount = 0;
    std::size_t preparedStreamCount = 0;
    std::size_t preparedZoneCount = 0;
    std::uint64_t preparedBytes = 0;
    std::uint64_t decodedBytes = 0;
    std::size_t cacheHitCount = 0;
    std::size_t cacheMissCount = 0;
    std::size_t failureCount = 0;
    std::size_t cancellationCount = 0;
    std::size_t pendingWorkCount = 0;
    std::uint64_t retiredBytesAwaitingCleanup = 0;
};

struct PreparedPlaybackBuildRequest
{
    bool accepted = false;
    std::uint64_t buildId = 0;
    std::uint64_t cancellationId = 0;
    std::uint64_t snapshotBuildId = 0;
    std::size_t requestedDraftRevision = 0;
    bool activationRequested = false;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::string state;
};

struct PreparedPlaybackBuildResult
{
    bool built = false;
    bool activationEligible = false;
    std::uint64_t buildId = 0;
    std::uint64_t cancellationId = 0;
    std::uint64_t snapshotBuildId = 0;
    std::size_t requestedDraftRevision = 0;
    bool activationRequested = false;
    PlaybackSnapshotLifecycleState lifecycleState = PlaybackSnapshotLifecycleState::idle;
    std::uint64_t buildDurationMicros = 0;
    std::string state;
    std::vector<PlaybackSnapshotFinding> findings;
    PreparedPlaybackMetrics metrics;
    ImmutablePreparedPlayback prepared;
};

enum class PreparedPlaybackWorkLane
{
    preview,
    performance
};

enum class PreparedPlaybackJobPriority
{
    preview = 0,
    performance = 1
};

struct PreparedPlaybackQueueSubmitResult
{
    bool accepted = false;
    PreparedPlaybackBuildRequest request;
    std::vector<PreparedPlaybackBuildResult> displacedResults;
    std::string state;
};

struct PreparedPlaybackWorkerStepResult
{
    bool processed = false;
    PreparedPlaybackWorkLane lane = PreparedPlaybackWorkLane::preview;
    PreparedPlaybackJobPriority priority = PreparedPlaybackJobPriority::preview;
    PreparedPlaybackBuildResult result;
};

struct PreparedPlaybackWorkerStatus
{
    std::size_t pendingWorkCount = 0;
    std::size_t inFlightWorkCount = 0;
    std::size_t completedWorkCount = 0;
    std::size_t cancellationCount = 0;
    std::size_t supersededCount = 0;
    std::size_t failureCount = 0;
    std::size_t maxPendingWorkCount = 0;
    std::uint64_t retiredBytesAwaitingCleanup = 0;
    std::string lastEvent;
};

class PreparedPlaybackService
{
public:
    explicit PreparedPlaybackService(std::string compilerVersion = "phase1-prepared-playback-v2",
                                     std::size_t maxPendingJobs = 2,
                                     bool enableBackgroundWorker = false);
    ~PreparedPlaybackService();

    PreparedPlaybackBuildRequest requestBuild(const PlaybackSnapshotBuildResult& snapshotResult);
    PreparedPlaybackBuildResult prepare(const PreparedPlaybackBuildRequest& request,
                                        const PlaybackSnapshotBuildResult& snapshotResult,
                                        const RuntimeStreamLoadResult& streamResult);
    PreparedPlaybackBuildResult cancelBuild(const PreparedPlaybackBuildRequest& request,
                                            const std::string& state = "Prepared playback build canceled") const;
    PreparedPlaybackBuildResult supersedeBuild(const PreparedPlaybackBuildRequest& request,
                                               std::uint64_t replacementBuildId,
                                               const std::string& state = "Prepared playback build superseded") const;
    void setBackgroundWorkerStream(const RuntimeStreamLoadResult& streamResult);
    PreparedPlaybackQueueSubmitResult enqueueBuild(const PlaybackSnapshotBuildResult& snapshotResult,
                                                   PreparedPlaybackWorkLane lane,
                                                   PreparedPlaybackJobPriority priority);
    PreparedPlaybackWorkerStepResult processNextQueuedBuild(const RuntimeStreamLoadResult& streamResult);
    std::vector<PreparedPlaybackWorkerStepResult> drainCompletedBuilds();
    std::vector<PreparedPlaybackBuildResult> cancelQueuedBuilds(
        PreparedPlaybackWorkLane lane,
        const std::string& state = "Prepared playback build canceled before worker execution");
    std::size_t retireStaleCacheEntries(std::size_t maxEntries = static_cast<std::size_t>(-1));
    const PreparedPlaybackWorkerStatus& getWorkerStatus() const { return workerStatus; }
    bool hasPendingQueuedBuilds() const { return !queuedJobs.empty(); }
    bool waitForWorkerIdle(std::uint64_t timeoutMillis);
    bool isBackgroundWorkerEnabled() const { return backgroundWorkerEnabled; }

private:
    struct CacheEntry
    {
        PreparedPlaybackSampleHandle sample;
        PreparedPlaybackStreamHandle stream;
        std::uint64_t retainedBytes = 0;
    };

    struct QueuedJob
    {
        PreparedPlaybackWorkLane lane = PreparedPlaybackWorkLane::preview;
        PreparedPlaybackJobPriority priority = PreparedPlaybackJobPriority::preview;
        PreparedPlaybackBuildRequest request;
        PlaybackSnapshotBuildResult snapshotResult;
        std::uint64_t enqueueOrdinal = 0;
    };

    PreparedPlaybackWorkerStepResult processQueuedJob(const QueuedJob& job,
                                                      const RuntimeStreamLoadResult& streamResult);
    void runBackgroundWorker();
    void refreshWorkerStatus();
    void retireSupersededCacheEntries(const std::string& sampleSourceId, const std::string& cacheKey);

    std::string compilerVersion;
    std::uint64_t nextBuildId = 1;
    std::uint64_t nextQueueOrdinal = 1;
    std::size_t maxPendingJobs = 2;
    bool backgroundWorkerEnabled = false;
    bool stopWorkerRequested = false;
    std::vector<std::pair<std::string, CacheEntry>> cacheEntries;
    std::vector<std::pair<std::string, CacheEntry>> retiredCacheEntries;
    std::vector<QueuedJob> queuedJobs;
    std::vector<PreparedPlaybackWorkerStepResult> completedResults;
    PreparedPlaybackWorkerStatus workerStatus;
    RuntimeStreamLoadResult workerStreamResult;
    std::thread workerThread;
    mutable std::mutex workerMutex;
    std::condition_variable workerCondition;
    std::condition_variable workerIdleCondition;
};

std::string serializeImmutablePreparedPlayback(const ImmutablePreparedPlayback& prepared);
std::string toString(PreparedPlaybackWorkLane lane);
std::string toString(PreparedPlaybackJobPriority priority);
} // namespace drs::engine
