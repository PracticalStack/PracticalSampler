#pragma once

#include "drs/engine/RuntimeModel.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace drs::engine
{
struct RuntimeStreamPageRequest
{
    std::string sampleId;
    std::uint32_t pageIndex = 0;
};

struct RuntimeStreamEnqueueResult
{
    bool accepted = false;
    bool readyFromCache = false;
    bool alreadyPending = false;
    std::string state;
    RuntimeStreamPageRequest request;
};

struct RuntimeStreamPageLease
{
    bool valid = false;
    std::uint64_t leaseId = 0;
    std::string sampleId;
    std::uint32_t pageIndex = 0;
    std::uint64_t absoluteOffsetBytes = 0;
    std::vector<std::uint8_t> bytes;
};

struct RuntimeStreamingServiceOptions
{
    std::string loadProfileId = "custom";
    std::size_t maxCachedPages = 16;
    std::uint64_t simulatedReadLatencyMicros = 0;
};

struct RuntimeStreamingServiceMetrics
{
    std::size_t cacheHitCount = 0;
    std::size_t cacheMissCount = 0;
    std::size_t pageMissCount = 0;
    std::size_t queuedRequestCount = 0;
    std::size_t completedReadCount = 0;
    std::size_t failedReadCount = 0;
    std::size_t backgroundReadCount = 0;
    std::size_t residentPageCount = 0;
    std::size_t pendingPageCount = 0;
    std::size_t peakPendingPageCount = 0;
    std::size_t activeLeaseCount = 0;
    std::size_t activeVoiceCount = 0;
    std::size_t peakActiveVoiceCount = 0;
    std::size_t evictedPageCount = 0;
    std::size_t purgePassCount = 0;
    std::size_t dormantPurgeCount = 0;
    std::size_t lastPurgeEvictedPageCount = 0;
    std::size_t headUsageCount = 0;
    std::uint64_t headFramesRead = 0;
    std::uint64_t headBytesRead = 0;
    std::uint64_t totalReadLatencyMicros = 0;
    std::uint64_t averageReadLatencyMicros = 0;
    std::uint64_t maxReadLatencyMicros = 0;
    std::uint64_t lastReadLatencyMicros = 0;
    std::string activeLoadProfileId = "custom";
    std::size_t configuredMaxCachedPages = 0;
    std::uint64_t requesterThreadIdHash = 0;
    std::uint64_t workerThreadIdHash = 0;
    std::uint64_t completionThreadIdHash = 0;
};

class RuntimeStreamingService
{
public:
    RuntimeStreamingService(const RuntimeStreamContainerModel& container,
                            RuntimeStreamingServiceOptions options = {});
    ~RuntimeStreamingService();

    RuntimeStreamingService(const RuntimeStreamingService&) = delete;
    RuntimeStreamingService& operator=(const RuntimeStreamingService&) = delete;

    RuntimeStreamEnqueueResult enqueuePageRead(const RuntimeStreamPageRequest& request);
    bool isPageReady(const RuntimeStreamPageRequest& request) const;
    std::optional<RuntimeStreamPageLease> tryAcquirePageLease(const RuntimeStreamPageRequest& request);
    void releasePageLease(std::uint64_t leaseId);
    void applyLoadProfile(const RuntimeStreamingServiceOptions& options);
    void purgeDormantPages();
    void registerActiveVoice(std::uint64_t voiceId);
    void unregisterActiveVoice(std::uint64_t voiceId);
    void recordHeadUsage(std::uint64_t frameCount, std::uint64_t byteCount);
    void recordPageMiss(const RuntimeStreamPageRequest& request);
    RuntimeStreamingServiceMetrics getMetrics() const;

private:
    struct Impl;
    Impl* impl = nullptr;
};
} // namespace drs::engine
