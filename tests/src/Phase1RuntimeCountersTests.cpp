#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeVoice.h"

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

void drainVoice(drs::engine::RuntimeVoice& voice, drs::engine::RuntimeStreamingService& service)
{
    require(waitUntil(
                [&]
                {
                    const auto advance = voice.advanceFrames(8192, service);
                    return advance.voiceFinished
                        || voice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                },
                std::chrono::milliseconds(1500)),
            "Voice did not finish draining in time.");
}
} // namespace

int main()
{
    try
    {
        const auto instrumentResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(instrumentResult.loaded, "Reference instrument must load before runtime-counter tests run.");

        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container must load before runtime-counter tests run.");

        const auto performanceProfile = drs::engine::findPhase1RuntimeLoadProfile("performance");
        const auto ecoProfile = drs::engine::findPhase1RuntimeLoadProfile("eco");
        require(performanceProfile.has_value() && ecoProfile.has_value(),
                "Runtime-counter tests require the Phase 1 Performance and Eco profiles.");

        drs::engine::RuntimeStreamingService service(
            streamResult.container,
            drs::engine::buildRuntimeStreamingServiceOptions(*performanceProfile, 5000));

        drs::engine::RuntimeVoice sustainVoiceA;
        drs::engine::RuntimeVoice sustainVoiceB;
        drs::engine::RuntimeVoice leadVoice;
        std::string errorMessage;

        require(sustainVoiceA.allocate(instrumentResult.instrument,
                                       streamResult.container,
                                       { 1101, "", 57, 64, { { "tone", 0.3 } }, "" },
                                       errorMessage),
                "First routed sustain voice should allocate.");
        require(sustainVoiceB.allocate(instrumentResult.instrument,
                                       streamResult.container,
                                       { 1102, "", 57, 120, { { "tone", 0.75 } }, "" },
                                       errorMessage),
                "Second routed sustain voice should allocate.");
        require(leadVoice.allocate(instrumentResult.instrument,
                                   streamResult.container,
                                   { 1103, "", 69, 120, { { "motion", 0.45 } }, "lead" },
                                   errorMessage),
                "Routed lead voice should allocate.");

        require(sustainVoiceA.advanceFrames(4096, service).framesAdvanced == 4096,
                "First sustain voice should advance through its entire prefetch head.");
        require(sustainVoiceB.advanceFrames(4096, service).framesAdvanced == 4096,
                "Second sustain voice should advance through its entire prefetch head.");
        require(leadVoice.advanceFrames(2048, service).framesAdvanced == 2048,
                "Lead voice should advance through its entire prefetch head.");

        require(sustainVoiceA.advanceFrames(64, service).waitingForPage,
                "First sustain voice should wait at the first streamed page boundary.");
        require(sustainVoiceB.advanceFrames(64, service).waitingForPage,
                "Second sustain voice should wait at the first streamed page boundary.");
        require(leadVoice.advanceFrames(64, service).waitingForPage,
                "Lead voice should wait at the first streamed page boundary.");

        require(waitUntil(
                    [&]
                    {
                        return service.isPageReady({ "sine-a3", 0 })
                            && service.isPageReady({ "triangle-a4", 0 });
                    },
                    std::chrono::milliseconds(500)),
                "Initial streamed pages did not become ready in time for runtime-counter tests.");

        require(sustainVoiceA.advanceFrames(64, service).acquiredPageLease,
                "First sustain voice should resume once its page is ready.");
        require(sustainVoiceB.advanceFrames(64, service).acquiredPageLease,
                "Second sustain voice should resume once its page is ready.");
        require(leadVoice.advanceFrames(64, service).acquiredPageLease,
                "Lead voice should resume once its page is ready.");

        for (const auto& request : std::vector<drs::engine::RuntimeStreamPageRequest> {
                 { "sine-a3", 1 },
                 { "sine-a3", 2 },
                 { "triangle-a4", 1 },
                 { "triangle-a4", 2 }
             })
        {
            require(service.enqueuePageRead(request).accepted,
                    "Warm-cache stress request should queue successfully.");
        }

        require(waitUntil(
                    [&]
                    {
                        const auto metrics = service.getMetrics();
                        return metrics.backgroundReadCount >= 6;
                    },
                    std::chrono::milliseconds(1000)),
                "Warm-cache stress reads did not complete in time.");

        const auto stressMetrics = service.getMetrics();
        require(stressMetrics.activeVoiceCount == 3,
                "Stress metrics should report three active routed voices.");
        require(stressMetrics.peakActiveVoiceCount >= 3,
                "Stress metrics should record the peak routed voice count.");
        require(stressMetrics.headUsageCount >= 3,
                "Stress metrics should record head usage for each routed voice.");
        require(stressMetrics.headFramesRead >= 10240,
                "Stress metrics should record the routed head frame count.");
        require(stressMetrics.headBytesRead >= 49152,
                "Stress metrics should record the routed head byte count.");
        require(stressMetrics.pageMissCount >= 3,
                "Stress metrics should record a page miss for each routed voice wait.");
        require(stressMetrics.averageReadLatencyMicros > 0,
                "Stress metrics should expose a positive average read latency.");
        require(stressMetrics.maxReadLatencyMicros >= stressMetrics.averageReadLatencyMicros,
                "Stress metrics should expose a max read latency at least as large as the average.");

        sustainVoiceA.beginRelease();
        sustainVoiceB.beginRelease();
        leadVoice.beginRelease();
        drainVoice(sustainVoiceA, service);
        drainVoice(sustainVoiceB, service);
        drainVoice(leadVoice, service);

        service.applyLoadProfile(drs::engine::buildRuntimeStreamingServiceOptions(*ecoProfile, 5000));
        service.purgeDormantPages();

        const auto idleRecoveryMetrics = service.getMetrics();
        require(idleRecoveryMetrics.activeVoiceCount == 0,
                "Idle-recovery metrics should report zero active voices after draining.");
        require(idleRecoveryMetrics.dormantPurgeCount >= 1,
                "Idle-recovery metrics should record an explicit dormant purge.");
        require(idleRecoveryMetrics.purgePassCount >= 1,
                "Idle-recovery metrics should record at least one purge pass.");
        require(idleRecoveryMetrics.evictedPageCount >= 1,
                "Idle-recovery metrics should preserve the cumulative eviction count.");
        require(idleRecoveryMetrics.residentPageCount <= ecoProfile->maxCachedPages,
                "Idle-recovery metrics should show resident pages trimmed to the downgraded budget.");

        std::cout << "Phase 1 runtime counter tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 runtime counter tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
