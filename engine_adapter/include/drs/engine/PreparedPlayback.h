#pragma once

#include "drs/engine/PlaybackSnapshot.h"
#include "drs/engine/RuntimeStream.h"

#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <memory>
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

struct PreparedPlaybackOwnershipRecord
{
    std::string ownershipToken;
    std::string retirementToken;
    std::string cacheKey;
    std::string sampleSourceId;
    std::string streamSampleId;
    std::string lifetimeState;
    std::uint64_t retainedBytes = 0;
    std::uint64_t preparedBuildId = 0;
    std::uint64_t retiredByBuildId = 0;
};

struct PreparedPlaybackDecodedSampleData
{
    std::vector<std::vector<float>> normalizedChannels;
};

struct PreparedPlaybackSampleHandle
{
    std::string sampleSourceId;
    std::string streamSampleId;
    std::string sourcePath;
    std::string canonicalSourcePath;
    std::string canonicalSourceIdentity;
    std::string sourceFingerprintHex;
    std::string formatName;
    std::string role;
    std::string channelLayout;
    double sampleRate = 0.0;
    std::uint64_t frameCount = 0;
    std::uint32_t channelCount = 0;
    bool rootMidiNotePresent = false;
    int rootMidiNote = 60;
    bool loopRangePresent = false;
    std::uint64_t loopStartFrame = 0;
    std::uint64_t loopEndFrame = 0;
    std::shared_ptr<const PreparedPlaybackDecodedSampleData> decodedSampleData;
    std::string ownershipToken;
    std::string cacheKey;
    std::size_t ownershipRecordIndex = 0;
};

struct PreparedPlaybackStreamHandle
{
    std::string sampleSourceId;
    std::string streamSampleId;
    std::string containerId;
    std::string containerPath;
    std::string payloadEncoding;
    std::string topologyKind;
    bool compiledStreamTopologyAvailable = false;
    std::uint64_t pageSizeBytes = 0;
    std::uint64_t payloadOffsetBytes = 0;
    std::uint64_t payloadSizeBytes = 0;
    std::uint64_t prefetchBytes = 0;
    std::uint64_t streamedPayloadOffsetBytes = 0;
    std::uint64_t streamedPayloadBytes = 0;
    std::size_t pageCount = 0;
    bool pageRangePresent = false;
    std::uint32_t firstPageIndex = 0;
    std::uint32_t lastPageIndex = 0;
    std::uint64_t firstPageOffsetBytes = 0;
    std::uint64_t lastPageOffsetBytes = 0;
    std::uint64_t lastPageSizeBytes = 0;
    std::string ownershipToken;
    std::string cacheKey;
    std::size_t ownershipRecordIndex = 0;
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

// S3.7-T5 deferral note: this remains a public aggregate for current prepared-cache plumbing,
// facade wiring, and regression tests. Treat it as a write-once build product after preparation
// completes; a dedicated encapsulation pass should hide mutable storage behind const views once
// Sprint 4's shared-renderer consumption API is settled.
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
    std::vector<PreparedPlaybackOwnershipRecord> ownershipRecords;
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
    std::size_t preparedOwnershipRecordCount = 0;
    // Total retained residency exposed by the prepared result.
    std::uint64_t preparedBytes = 0;
    // Cache-ownership accounting for the retained prepared residency.
    std::uint64_t preparedOwnershipBytes = 0;
    // Decoded PCM retained by prepared sample handles.
    std::uint64_t preparedSampleDataBytes = 0;
    std::uint64_t decodedBytes = 0;
    std::size_t cacheHitCount = 0;
    std::size_t cacheMissCount = 0;
    std::size_t failureCount = 0;
    std::size_t cancellationCount = 0;
    std::size_t pendingWorkCount = 0;
    std::size_t activeCachedOwnershipRecordCount = 0;
    std::size_t retiredOwnershipRecordCount = 0;
    std::uint64_t retiredBytesAwaitingCleanup = 0;
};

struct PreparedPlaybackSampleResolution
{
    std::size_t snapshotSampleIndex = 0;
    std::string sampleSourceId;
    std::string normalizedSourcePath;
    std::string selectedStreamSampleId;
    std::string selectedFormatName;
    std::string resolutionKind = "authored-source";
    bool compiledStreamTopologyAvailable = false;
    bool matchedBySourcePath = false;
    bool matchedBySampleSourceId = false;
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
    bool sampleResolutionReady = false;
    std::vector<PreparedPlaybackSampleResolution> sampleResolutions;
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
    std::size_t configuredMaxPendingWorkCount = 0;
    std::size_t configuredMaxInFlightWorkCount = 0;
    std::size_t completedWorkCount = 0;
    std::size_t cancellationCount = 0;
    std::size_t supersededCount = 0;
    std::size_t failureCount = 0;
    std::size_t maxPendingWorkCount = 0;
    std::size_t activeOwnershipRecordCount = 0;
    std::uint64_t activeOwnershipBytes = 0;
    std::size_t retiredOwnershipRecordCount = 0;
    std::uint64_t retiredBytesAwaitingCleanup = 0;
    std::string lastEvent;
    std::string lastCancellationLane;
    std::string lastCancellationReason;
    std::string lastSupersededLane;
    std::string lastSupersededReason;
};

bool operator==(const PreparedPlaybackPageHandle& left, const PreparedPlaybackPageHandle& right);
bool operator==(const PreparedPlaybackOwnershipRecord& left, const PreparedPlaybackOwnershipRecord& right);
bool operator==(const PreparedPlaybackSampleHandle& left, const PreparedPlaybackSampleHandle& right);
bool operator==(const PreparedPlaybackStreamHandle& left, const PreparedPlaybackStreamHandle& right);
bool operator==(const PreparedPlaybackZoneHandle& left, const PreparedPlaybackZoneHandle& right);
bool operator==(const ImmutablePreparedPlayback& left, const ImmutablePreparedPlayback& right);

class PreparedPlaybackService
{
public:
    explicit PreparedPlaybackService(std::string compilerVersion = "phase1-prepared-playback-v2",
                                     std::size_t maxPendingJobs = 2,
                                     bool enableBackgroundWorker = false);
    ~PreparedPlaybackService();

    PreparedPlaybackBuildRequest requestBuild(const PlaybackSnapshotBuildResult& snapshotResult);
    PreparedPlaybackBuildRequest requestBuild(const PlaybackSnapshotBuildResult& snapshotResult,
                                             const RuntimeStreamLoadResult& streamResult);
    // Sprint 3 preparation boundary: Preview and Publish are only allowed to enter playback preparation
    // through these named service calls, not through ad hoc shell-side decode or generic lane plumbing.
    PreparedPlaybackQueueSubmitResult enqueuePreviewBuild(const PlaybackSnapshotBuildResult& snapshotResult);
    PreparedPlaybackQueueSubmitResult enqueuePublishBuild(const PlaybackSnapshotBuildResult& snapshotResult);
    PreparedPlaybackBuildResult prepare(const PreparedPlaybackBuildRequest& request,
                                        const PlaybackSnapshotBuildResult& snapshotResult,
                                        const RuntimeStreamLoadResult& streamResult);
    PreparedPlaybackBuildResult cancelBuild(const PreparedPlaybackBuildRequest& request,
                                            const std::string& state = "Prepared playback build canceled") const;
    PreparedPlaybackBuildResult supersedeBuild(const PreparedPlaybackBuildRequest& request,
                                               std::uint64_t replacementBuildId,
                                               const std::string& state = "Prepared playback build superseded") const;
    void setBackgroundWorkerStream(const RuntimeStreamLoadResult& streamResult);
    PreparedPlaybackWorkerStepResult processNextQueuedBuild(const RuntimeStreamLoadResult& streamResult);
    std::vector<PreparedPlaybackWorkerStepResult> drainCompletedBuilds();
    // Preview/Publish cancellation follows the same named entry-point rule as queue submission.
    std::vector<PreparedPlaybackBuildResult> cancelQueuedPreviewBuilds(
        const std::string& state = "Prepared playback build canceled before worker execution");
    std::vector<PreparedPlaybackBuildResult> cancelQueuedPublishBuilds(
        const std::string& state = "Prepared playback build canceled before worker execution");
    std::size_t serviceRetiredCacheCleanup(std::size_t maxEntries = static_cast<std::size_t>(-1));
    std::size_t retireStaleCacheEntries(std::size_t maxEntries = static_cast<std::size_t>(-1));
    std::vector<PreparedPlaybackOwnershipRecord> snapshotRetiredOwnershipRecords() const;
    PreparedPlaybackWorkerStatus getWorkerStatus() const;
    bool hasPendingQueuedBuilds() const;
    bool waitForWorkerIdle(std::uint64_t timeoutMillis);
    bool isBackgroundWorkerEnabled() const { return backgroundWorkerEnabled; }

private:
    struct CacheEntry
    {
        PreparedPlaybackOwnershipRecord ownership;
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
    PreparedPlaybackBuildRequest resolveBuildRequest(const PreparedPlaybackBuildRequest& request,
                                                     const PlaybackSnapshotBuildResult& snapshotResult,
                                                     const RuntimeStreamLoadResult& streamResult) const;
    std::vector<QueuedJob>::iterator selectQueuedJobToDisplaceForPriority(
        PreparedPlaybackJobPriority incomingPriority);
    PreparedPlaybackQueueSubmitResult enqueueBuildForLane(const PlaybackSnapshotBuildResult& snapshotResult,
                                                          PreparedPlaybackWorkLane lane,
                                                          PreparedPlaybackJobPriority priority);
    std::vector<PreparedPlaybackBuildResult> cancelQueuedBuildsForLane(
        PreparedPlaybackWorkLane lane,
        const std::string& state);
    void runBackgroundWorker();
    void refreshWorkerStatus();
    void retireSupersededCacheEntries(const std::string& sampleSourceId,
                                      const std::string& cacheKey,
                                      std::uint64_t retiredByBuildId);

    std::string compilerVersion;
    std::uint64_t nextBuildId = 1;
    std::uint64_t nextQueueOrdinal = 1;
    std::uint64_t nextRetirementToken = 1;
    std::size_t maxPendingJobs = 2;
    bool backgroundWorkerEnabled = false;
    bool workerStreamConfigured = false;
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

std::string computePreparedPlaybackContentDigest(const ImmutablePreparedPlayback& prepared);
std::string serializePreparedPlaybackContent(const ImmutablePreparedPlayback& prepared);
std::string serializeImmutablePreparedPlayback(const ImmutablePreparedPlayback& prepared);
std::string toString(PreparedPlaybackWorkLane lane);
std::string toString(PreparedPlaybackJobPriority priority);
} // namespace drs::engine
