#include "drs/engine/RuntimeStreamingService.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace drs::engine
{
namespace
{
std::uint64_t hashThreadId(const std::thread::id& threadId)
{
    return static_cast<std::uint64_t>(std::hash<std::thread::id> {}(threadId));
}

std::string makeCacheKey(const RuntimeStreamPageRequest& request)
{
    return request.sampleId + "#" + std::to_string(request.pageIndex);
}

std::vector<std::uint8_t> buildSyntheticPageData(const RuntimeStreamSampleDefinition& sample,
                                                 const RuntimeStreamPageDefinition& page)
{
    std::vector<std::uint8_t> bytes(page.sizeBytes);
    const auto seed = sample.payloadOffsetBytes + page.offsetBytes + page.pageIndex;

    for (std::size_t index = 0; index < bytes.size(); ++index)
        bytes[index] = static_cast<std::uint8_t>((seed + index) % 251);

    return bytes;
}
} // namespace

struct RuntimeStreamingService::Impl
{
    enum class EntryState
    {
        queued,
        inFlight,
        ready,
        failed
    };

    struct CachedPageEntry
    {
        RuntimeStreamPageRequest request;
        const RuntimeStreamSampleDefinition* sample = nullptr;
        const RuntimeStreamPageDefinition* page = nullptr;
        EntryState state = EntryState::queued;
        std::vector<std::uint8_t> bytes;
        std::size_t retainCount = 0;
        std::uint64_t lastTouchCounter = 0;
        std::chrono::steady_clock::time_point queuedAt = std::chrono::steady_clock::time_point {};
    };

    RuntimeStreamContainerModel container;
    RuntimeStreamingServiceOptions options;
    mutable std::mutex mutex;
    std::condition_variable condition;
    bool stopRequested = false;
    std::deque<std::string> queuedKeys;
    std::unordered_map<std::string, CachedPageEntry> cache;
    std::unordered_map<std::uint64_t, std::string> leases;
    std::unordered_set<std::uint64_t> activeVoices;
    RuntimeStreamingServiceMetrics metrics;
    std::uint64_t nextLeaseId = 1;
    std::uint64_t touchCounter = 1;
    std::thread workerThread;

    explicit Impl(const RuntimeStreamContainerModel& containerIn,
                  RuntimeStreamingServiceOptions optionsIn)
        : container(containerIn),
          options(std::move(optionsIn))
    {
        metrics.activeLoadProfileId = options.loadProfileId;
        metrics.configuredMaxCachedPages = options.maxCachedPages;
        workerThread = std::thread([this] { runWorkerLoop(); });
    }

    ~Impl()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex);
            stopRequested = true;
        }

        condition.notify_all();
        if (workerThread.joinable())
            workerThread.join();
    }

    std::optional<std::pair<const RuntimeStreamSampleDefinition*, const RuntimeStreamPageDefinition*>>
    findSampleAndPage(const RuntimeStreamPageRequest& request) const
    {
        const auto sampleIterator = std::find_if(container.samples.begin(),
                                                 container.samples.end(),
                                                 [&](const RuntimeStreamSampleDefinition& sample)
                                                 {
                                                     return sample.sampleId == request.sampleId;
                                                 });

        if (sampleIterator == container.samples.end())
            return std::nullopt;

        if (request.pageIndex >= sampleIterator->pages.size())
            return std::nullopt;

        return std::make_pair(&(*sampleIterator), &(sampleIterator->pages[request.pageIndex]));
    }

    void runWorkerLoop()
    {
        while (true)
        {
            std::string cacheKey;
            RuntimeStreamPageRequest request;
            const RuntimeStreamSampleDefinition* sample = nullptr;
            const RuntimeStreamPageDefinition* page = nullptr;

            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [&]
                               { return stopRequested || !queuedKeys.empty(); });

                if (stopRequested && queuedKeys.empty())
                    return;

                cacheKey = queuedKeys.front();
                queuedKeys.pop_front();

                auto entryIterator = cache.find(cacheKey);
                if (entryIterator == cache.end())
                    continue;

                auto& entry = entryIterator->second;
                if (entry.state != EntryState::queued)
                    continue;

                entry.state = EntryState::inFlight;
                entry.lastTouchCounter = touchCounter++;
                request = entry.request;
                sample = entry.sample;
                page = entry.page;
                metrics.workerThreadIdHash = hashThreadId(std::this_thread::get_id());
            }

            if (options.simulatedReadLatencyMicros > 0)
            {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<long long>(options.simulatedReadLatencyMicros)));
            }

            auto bytes = buildSyntheticPageData(*sample, *page);

            {
                std::lock_guard<std::mutex> lock(mutex);
                auto entryIterator = cache.find(cacheKey);
                if (entryIterator == cache.end())
                    continue;

                auto& entry = entryIterator->second;
                entry.bytes = std::move(bytes);
                entry.state = EntryState::ready;
                entry.lastTouchCounter = touchCounter++;
                const auto readLatencyMicros = static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now() - entry.queuedAt)
                        .count());

                ++metrics.completedReadCount;
                ++metrics.backgroundReadCount;
                metrics.totalReadLatencyMicros += readLatencyMicros;
                metrics.lastReadLatencyMicros = readLatencyMicros;
                metrics.maxReadLatencyMicros = std::max(metrics.maxReadLatencyMicros, readLatencyMicros);
                if (metrics.pendingPageCount > 0)
                    --metrics.pendingPageCount;
                metrics.completionThreadIdHash = hashThreadId(std::this_thread::get_id());
                metrics.residentPageCount = countResidentPagesLocked();
            }
        }
    }

    std::size_t countResidentPagesLocked() const
    {
        return static_cast<std::size_t>(std::count_if(cache.begin(),
                                                      cache.end(),
                                                      [](const auto& pair)
                                                      {
                                                          return pair.second.state == EntryState::ready;
                                                      }));
    }

    void evictIfNeededLocked()
    {
        runPurgeLocked(false);
    }

    void runPurgeLocked(bool explicitDormantPurge)
    {
        ++metrics.purgePassCount;
        if (explicitDormantPurge)
            ++metrics.dormantPurgeCount;

        std::size_t evictedThisPass = 0;
        while (countResidentPagesLocked() > options.maxCachedPages)
        {
            auto candidateIterator = cache.end();

            for (auto iterator = cache.begin(); iterator != cache.end(); ++iterator)
            {
                if (iterator->second.state != EntryState::ready || iterator->second.retainCount != 0)
                    continue;

                if (candidateIterator == cache.end()
                    || iterator->second.lastTouchCounter < candidateIterator->second.lastTouchCounter)
                {
                    candidateIterator = iterator;
                }
            }

            if (candidateIterator == cache.end())
                break;

            cache.erase(candidateIterator);
            ++metrics.evictedPageCount;
            ++evictedThisPass;
        }

        metrics.lastPurgeEvictedPageCount = evictedThisPass;
        metrics.residentPageCount = countResidentPagesLocked();
    }
};

RuntimeStreamingService::RuntimeStreamingService(const RuntimeStreamContainerModel& container,
                                                 RuntimeStreamingServiceOptions options)
    : impl(new Impl(container, std::move(options)))
{
}

RuntimeStreamingService::~RuntimeStreamingService()
{
    delete impl;
    impl = nullptr;
}

RuntimeStreamEnqueueResult RuntimeStreamingService::enqueuePageRead(const RuntimeStreamPageRequest& request)
{
    RuntimeStreamEnqueueResult result;
    result.request = request;
    result.state = "Streaming read rejected";

    const auto sampleAndPage = impl->findSampleAndPage(request);
    if (!sampleAndPage.has_value())
    {
        ++impl->metrics.failedReadCount;
        result.state = "Streaming read invalid";
        return result;
    }

    const auto cacheKey = makeCacheKey(request);

    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        impl->metrics.requesterThreadIdHash = hashThreadId(std::this_thread::get_id());

        const auto entryIterator = impl->cache.find(cacheKey);
        if (entryIterator != impl->cache.end())
        {
            auto& entry = entryIterator->second;
            entry.lastTouchCounter = impl->touchCounter++;

            if (entry.state == Impl::EntryState::ready)
            {
                ++impl->metrics.cacheHitCount;
                result.accepted = true;
                result.readyFromCache = true;
                result.state = "Streaming read ready from cache";
                impl->metrics.residentPageCount = impl->countResidentPagesLocked();
                return result;
            }

            if (entry.state == Impl::EntryState::queued || entry.state == Impl::EntryState::inFlight)
            {
                result.accepted = true;
                result.alreadyPending = true;
                result.state = "Streaming read already pending";
                return result;
            }
        }

        Impl::CachedPageEntry entry;
        entry.request = request;
        entry.sample = sampleAndPage->first;
        entry.page = sampleAndPage->second;
        entry.state = Impl::EntryState::queued;
        entry.lastTouchCounter = impl->touchCounter++;
        entry.queuedAt = std::chrono::steady_clock::now();
        impl->cache[cacheKey] = std::move(entry);
        impl->queuedKeys.push_back(cacheKey);

        ++impl->metrics.cacheMissCount;
        ++impl->metrics.queuedRequestCount;
        ++impl->metrics.pendingPageCount;
        impl->metrics.peakPendingPageCount = std::max(impl->metrics.peakPendingPageCount,
                                                      impl->metrics.pendingPageCount);
    }

    impl->condition.notify_one();

    result.accepted = true;
    result.state = "Streaming read queued";
    return result;
}

bool RuntimeStreamingService::isPageReady(const RuntimeStreamPageRequest& request) const
{
    const auto cacheKey = makeCacheKey(request);
    const std::lock_guard<std::mutex> lock(impl->mutex);
    const auto iterator = impl->cache.find(cacheKey);
    return iterator != impl->cache.end() && iterator->second.state == Impl::EntryState::ready;
}

std::optional<RuntimeStreamPageLease> RuntimeStreamingService::tryAcquirePageLease(const RuntimeStreamPageRequest& request)
{
    const auto cacheKey = makeCacheKey(request);
    std::lock_guard<std::mutex> lock(impl->mutex);
    const auto iterator = impl->cache.find(cacheKey);

    if (iterator == impl->cache.end() || iterator->second.state != Impl::EntryState::ready)
        return std::nullopt;

    auto& entry = iterator->second;
    ++entry.retainCount;
    entry.lastTouchCounter = impl->touchCounter++;

    RuntimeStreamPageLease lease;
    lease.valid = true;
    lease.leaseId = impl->nextLeaseId++;
    lease.sampleId = request.sampleId;
    lease.pageIndex = request.pageIndex;
    lease.absoluteOffsetBytes = entry.page->offsetBytes;
    lease.bytes = entry.bytes;

    impl->leases.emplace(lease.leaseId, cacheKey);
    ++impl->metrics.activeLeaseCount;
    impl->metrics.residentPageCount = impl->countResidentPagesLocked();
    return lease;
}

void RuntimeStreamingService::releasePageLease(std::uint64_t leaseId)
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    const auto leaseIterator = impl->leases.find(leaseId);
    if (leaseIterator == impl->leases.end())
        return;

    const auto cacheIterator = impl->cache.find(leaseIterator->second);
    if (cacheIterator != impl->cache.end())
    {
        auto& entry = cacheIterator->second;
        if (entry.retainCount > 0)
            --entry.retainCount;
        entry.lastTouchCounter = impl->touchCounter++;
    }

    impl->leases.erase(leaseIterator);
    if (impl->metrics.activeLeaseCount > 0)
        --impl->metrics.activeLeaseCount;
    impl->evictIfNeededLocked();
}

RuntimeStreamingServiceMetrics RuntimeStreamingService::getMetrics() const
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    auto metrics = impl->metrics;
    metrics.residentPageCount = impl->countResidentPagesLocked();
    metrics.activeVoiceCount = impl->activeVoices.size();
    metrics.averageReadLatencyMicros = metrics.completedReadCount == 0
        ? 0
        : (metrics.totalReadLatencyMicros / metrics.completedReadCount);
    return metrics;
}

void RuntimeStreamingService::applyLoadProfile(const RuntimeStreamingServiceOptions& options)
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    impl->options.loadProfileId = options.loadProfileId;
    impl->options.maxCachedPages = options.maxCachedPages;
    impl->options.simulatedReadLatencyMicros = options.simulatedReadLatencyMicros;
    impl->metrics.activeLoadProfileId = impl->options.loadProfileId;
    impl->metrics.configuredMaxCachedPages = impl->options.maxCachedPages;
    impl->evictIfNeededLocked();
}

void RuntimeStreamingService::purgeDormantPages()
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    impl->runPurgeLocked(true);
}

void RuntimeStreamingService::registerActiveVoice(std::uint64_t voiceId)
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    if (voiceId == 0)
        return;

    if (impl->activeVoices.insert(voiceId).second)
    {
        impl->metrics.activeVoiceCount = impl->activeVoices.size();
        impl->metrics.peakActiveVoiceCount = std::max(impl->metrics.peakActiveVoiceCount,
                                                      impl->metrics.activeVoiceCount);
    }
}

void RuntimeStreamingService::unregisterActiveVoice(std::uint64_t voiceId)
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    if (voiceId == 0)
        return;

    impl->activeVoices.erase(voiceId);
    impl->metrics.activeVoiceCount = impl->activeVoices.size();
}

void RuntimeStreamingService::recordHeadUsage(std::uint64_t frameCount, std::uint64_t byteCount)
{
    std::lock_guard<std::mutex> lock(impl->mutex);
    if (frameCount == 0 && byteCount == 0)
        return;

    ++impl->metrics.headUsageCount;
    impl->metrics.headFramesRead += frameCount;
    impl->metrics.headBytesRead += byteCount;
}

void RuntimeStreamingService::recordPageMiss(const RuntimeStreamPageRequest& request)
{
    (void)request;
    std::lock_guard<std::mutex> lock(impl->mutex);
    ++impl->metrics.pageMissCount;
}
} // namespace drs::engine
