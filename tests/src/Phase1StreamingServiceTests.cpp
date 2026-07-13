#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeStreamingService.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace
{
using Clock = std::chrono::steady_clock;

void require(bool condition, const std::string& message)
{
    if (!condition)
        throw std::runtime_error(message);
}

template <typename TPredicate>
bool waitUntil(TPredicate predicate, std::chrono::milliseconds timeout)
{
    const auto deadline = Clock::now() + timeout;

    while (Clock::now() < deadline)
    {
        if (predicate())
            return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return predicate();
}
} // namespace

int main()
{
    try
    {
        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container must load before streaming-service tests run.");

        drs::engine::RuntimeStreamingService service(
            streamResult.container,
            drs::engine::RuntimeStreamingServiceOptions { "custom", 2, 10000 });

        const drs::engine::RuntimeStreamPageRequest firstPage { "sine-a3", 0 };
        const auto firstEnqueue = service.enqueuePageRead(firstPage);
        require(firstEnqueue.accepted, "First streaming-page request should be accepted.");
        require(!firstEnqueue.readyFromCache, "First streaming-page request should not be a cache hit.");

        const auto duplicateEnqueue = service.enqueuePageRead(firstPage);
        require(duplicateEnqueue.accepted && duplicateEnqueue.alreadyPending,
                "Duplicate in-flight request should be coalesced instead of queued twice.");

        require(waitUntil([&] { return service.isPageReady(firstPage); }, std::chrono::milliseconds(250)),
                "Streaming service did not resolve the first page in time.");

        const auto firstLease = service.tryAcquirePageLease(firstPage);
        require(firstLease.has_value() && firstLease->valid, "Resolved first page should produce a valid lease.");
        require(firstLease->bytes.size() == 65536, "Resolved first page size changed unexpectedly.");

        const auto cacheHitEnqueue = service.enqueuePageRead(firstPage);
        require(cacheHitEnqueue.accepted && cacheHitEnqueue.readyFromCache,
                "Ready page should serve subsequent requests from cache.");

        auto metrics = service.getMetrics();
        require(metrics.backgroundReadCount >= 1, "Streaming service should record at least one background read.");
        require(metrics.cacheHitCount >= 1, "Streaming service should record at least one cache hit.");
        require(metrics.requesterThreadIdHash != 0, "Streaming service should capture the requester thread id.");
        require(metrics.workerThreadIdHash != 0, "Streaming service should capture the worker thread id.");
        require(metrics.completionThreadIdHash != 0, "Streaming service should capture the completion thread id.");
        require(metrics.workerThreadIdHash != metrics.requesterThreadIdHash,
                "Streaming reads should complete on a worker thread, not the requester thread.");
        require(metrics.activeLeaseCount == 1, "Streaming service lease count changed unexpectedly after first acquire.");

        service.releasePageLease(firstLease->leaseId);
        metrics = service.getMetrics();
        require(metrics.activeLeaseCount == 0, "Streaming service lease release should decrement active leases.");

        drs::engine::RuntimeStreamingService burstService(
            streamResult.container,
            drs::engine::RuntimeStreamingServiceOptions { "custom", 2, 12000 });

        std::vector<drs::engine::RuntimeStreamPageRequest> burstRequests;
        for (const auto& sample : streamResult.container.samples)
        {
            for (const auto& page : sample.pages)
                burstRequests.push_back({ sample.sampleId, page.pageIndex });
        }

        const auto enqueueStart = Clock::now();
        for (const auto& request : burstRequests)
        {
            const auto enqueueResult = burstService.enqueuePageRead(request);
            require(enqueueResult.accepted, "Burst streaming-page request should be accepted.");
        }
        const auto enqueueDuration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - enqueueStart);

        require(enqueueDuration.count() < 60,
                "Burst request submission took too long; streaming requests should not block like synchronous reads.");

        auto burstMetrics = burstService.getMetrics();
        require(burstMetrics.pendingPageCount > 0,
                "Burst request submission should leave work pending for the background service.");
        require(burstMetrics.peakPendingPageCount >= burstRequests.size(),
                "Burst request submission should record the full pending burst depth.");

        require(waitUntil([&]
                          {
                              return burstService.getMetrics().backgroundReadCount >= burstRequests.size();
                          },
                          std::chrono::milliseconds(1000)),
                "Burst streaming requests did not complete in time.");

        burstMetrics = burstService.getMetrics();
        require(burstMetrics.backgroundReadCount == burstRequests.size(),
                "Burst streaming-service background read count changed unexpectedly.");
        require(burstMetrics.pendingPageCount == 0,
                "Burst streaming-service pending count should drain back to zero.");

        drs::engine::RuntimeStreamingService evictionService(
            streamResult.container,
            drs::engine::RuntimeStreamingServiceOptions { "custom", 1, 2000 });

        const drs::engine::RuntimeStreamPageRequest pageA { "sine-a3", 0 };
        const drs::engine::RuntimeStreamPageRequest pageB { "sine-a3", 1 };
        const drs::engine::RuntimeStreamPageRequest pageC { "sine-a3", 2 };

        require(evictionService.enqueuePageRead(pageA).accepted, "Eviction test page A should queue.");
        require(evictionService.enqueuePageRead(pageB).accepted, "Eviction test page B should queue.");
        require(evictionService.enqueuePageRead(pageC).accepted, "Eviction test page C should queue.");

        require(waitUntil([&]
                          {
                              return evictionService.isPageReady(pageA)
                                  && evictionService.isPageReady(pageB)
                                  && evictionService.isPageReady(pageC);
                          },
                          std::chrono::milliseconds(600)),
                "Eviction test pages did not resolve in time.");

        const auto leaseB = evictionService.tryAcquirePageLease(pageB);
        require(leaseB.has_value(), "Eviction test should acquire a lease for page B.");

        evictionService.releasePageLease(leaseB->leaseId);
        const auto evictionMetrics = evictionService.getMetrics();
        require(evictionMetrics.residentPageCount <= 2,
                "Eviction service should trim resident pages back to its configured cache budget.");
        require(evictionMetrics.evictedPageCount >= 1,
                "Eviction service should record at least one page eviction when over budget.");

        std::cout << "Phase 1 streaming service tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 streaming service tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
