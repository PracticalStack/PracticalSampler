#include "drs/engine/RuntimeLoadProfile.h"
#include "drs/engine/RuntimeLoader.h"
#include "drs/engine/RuntimeStream.h"
#include "drs/engine/RuntimeStreamingService.h"
#include "drs/engine/RuntimeVoice.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

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
        const auto eco = drs::engine::findPhase1RuntimeLoadProfile("eco");
        const auto balanced = drs::engine::findPhase1RuntimeLoadProfile("balanced");
        const auto performance = drs::engine::findPhase1RuntimeLoadProfile("performance");

        require(eco.has_value() && balanced.has_value() && performance.has_value(),
                "Phase 1 load profiles Eco, Balanced, and Performance must all exist.");
        require(eco->maxPrefetchBytesPerVoice < balanced->maxPrefetchBytesPerVoice,
                "Eco prefetch budget should be smaller than Balanced.");
        require(balanced->maxPrefetchBytesPerVoice < performance->maxPrefetchBytesPerVoice,
                "Balanced prefetch budget should be smaller than Performance.");
        require(eco->maxCachedPages < balanced->maxCachedPages
                    && balanced->maxCachedPages < performance->maxCachedPages,
                "Phase 1 cache-page budgets should increase from Eco to Balanced to Performance.");

        const auto instrumentResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(instrumentResult.loaded, "Reference instrument must load before load-profile tests run.");

        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container must load before load-profile tests run.");

        auto ecoInstrument = instrumentResult.instrument;
        ecoInstrument.defaultLoadProfile = "eco";
        auto balancedInstrument = instrumentResult.instrument;
        balancedInstrument.defaultLoadProfile = "balanced";
        auto performanceInstrument = instrumentResult.instrument;
        performanceInstrument.defaultLoadProfile = "performance";

        drs::engine::RuntimeStreamingService dummyService(streamResult.container,
                                                          drs::engine::buildRuntimeStreamingServiceOptions(*balanced, 0));
        drs::engine::RuntimeVoice ecoVoice;
        drs::engine::RuntimeVoice balancedVoice;
        drs::engine::RuntimeVoice performanceVoice;
        std::string errorMessage;

        require(ecoVoice.allocate(ecoInstrument,
                                  streamResult.container,
                                  { 601, "pad-a3", 57, 100, {} },
                                  errorMessage),
                "Eco voice allocation should succeed.");
        require(balancedVoice.allocate(balancedInstrument,
                                       streamResult.container,
                                       { 602, "pad-a3", 57, 100, {} },
                                       errorMessage),
                "Balanced voice allocation should succeed.");
        require(performanceVoice.allocate(performanceInstrument,
                                          streamResult.container,
                                          { 603, "pad-a3", 57, 100, {} },
                                          errorMessage),
                "Performance voice allocation should succeed.");

        require(ecoVoice.getSnapshot().cursor.prefetchBytes == 8192,
                "Eco voice prefetch budget should clamp down to 8192 bytes.");
        require(balancedVoice.getSnapshot().cursor.prefetchBytes == 16384,
                "Balanced voice prefetch budget should match the compiled reference head.");
        require(performanceVoice.getSnapshot().cursor.prefetchBytes == 16384,
                "Performance voice prefetch budget should not exceed the compiled reference head.");

        drs::engine::RuntimeStreamingService service(
            streamResult.container,
            drs::engine::buildRuntimeStreamingServiceOptions(*performance, 5000));
        drs::engine::RuntimeVoice activeVoice;
        require(activeVoice.allocate(performanceInstrument,
                                     streamResult.container,
                                     { 701, "pad-a3", 57, 100, { { "tone", 0.55 } } },
                                     errorMessage),
                "Active-voice profile test allocation should succeed.");

        require(activeVoice.advanceFrames(4096, service).framesAdvanced == 4096,
                "Active-voice test should cross the full prefetch head first.");
        require(activeVoice.advanceFrames(64, service).waitingForPage,
                "Active-voice test should wait when it first reaches streamed data.");

        require(waitUntil([&] { return service.isPageReady({ "sine-a3", 0 }); }, std::chrono::milliseconds(300)),
                "Active-voice test page did not become ready in time.");

        const auto acquiredLeaseAdvance = activeVoice.advanceFrames(64, service);
        require(acquiredLeaseAdvance.acquiredPageLease,
                "Active-voice test should acquire a page lease before profile switching.");

        const auto preSwitchMetrics = service.getMetrics();
        require(preSwitchMetrics.activeLeaseCount == 1,
                "Active-voice test should hold one active lease before profile switching.");
        require(preSwitchMetrics.activeLoadProfileId == "performance",
                "Streaming service should start in the Performance profile.");
        require(preSwitchMetrics.configuredMaxCachedPages == performance->maxCachedPages,
                "Streaming service should expose the Performance cache budget.");

        for (const auto& request : std::vector<drs::engine::RuntimeStreamPageRequest> {
                 { "sine-a3", 1 },
                 { "sine-a3", 2 },
                 { "triangle-a4", 0 }
             })
        {
            require(service.enqueuePageRead(request).accepted, "Additional warm-cache requests should queue successfully.");
        }

        require(waitUntil(
                    [&]
                    {
                        const auto metrics = service.getMetrics();
                        return metrics.backgroundReadCount >= 4;
                    },
                    std::chrono::milliseconds(600)),
                "Warm-cache page requests did not resolve in time.");

        service.applyLoadProfile(drs::engine::buildRuntimeStreamingServiceOptions(*eco, 5000));
        const auto downgradedMetrics = service.getMetrics();
        require(downgradedMetrics.activeLoadProfileId == "eco",
                "Streaming service should switch to the Eco profile.");
        require(downgradedMetrics.configuredMaxCachedPages == eco->maxCachedPages,
                "Streaming service should expose the Eco cache budget after switching.");
        require(downgradedMetrics.activeLeaseCount == 1,
                "Downgrading load profiles must not invalidate an active voice lease.");

        const auto postSwitchAdvance = activeVoice.advanceFrames(64, service);
        require(postSwitchAdvance.advanced,
                "Active voice should keep advancing after a load-profile downgrade.");
        require(activeVoice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::active,
                "Active voice should remain active after a load-profile downgrade.");

        activeVoice.beginRelease();
        require(waitUntil(
                    [&]
                    {
                        const auto advance = activeVoice.advanceFrames(8192, service);
                        return advance.voiceFinished
                            || activeVoice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished;
                    },
                    std::chrono::milliseconds(1500)),
                "Active voice did not finish after release during load-profile downgrade test.");

        require(service.getMetrics().activeLeaseCount == 0,
                "Finished voice should not leave an active lease after release.");

        service.purgeDormantPages();
        const auto finalMetrics = service.getMetrics();
        require(finalMetrics.residentPageCount <= eco->maxCachedPages,
                "Dormant purge should trim resident pages back to the Eco cache budget.");
        require(finalMetrics.evictedPageCount >= 1,
                "Dormant purge should record at least one eviction after a profile downgrade.");

        auto invalidProfileInstrument = instrumentResult.instrument;
        invalidProfileInstrument.defaultLoadProfile = "unknown-profile";
        drs::engine::RuntimeVoice invalidProfileVoice;
        require(!invalidProfileVoice.allocate(invalidProfileInstrument,
                                              streamResult.container,
                                              { 801, "pad-a3", 57, 100, {} },
                                              errorMessage),
                "Voice allocation should reject unknown load profiles.");
        require(!errorMessage.empty(), "Unknown load profile rejection should report an actionable error.");

        std::cout << "Phase 1 load profile tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 load profile tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
