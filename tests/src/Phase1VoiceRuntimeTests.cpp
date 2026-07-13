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

void drainVoiceToFinished(drs::engine::RuntimeVoice& voice,
                          drs::engine::RuntimeStreamingService& service)
{
    require(waitUntil(
                [&]
                {
                    const auto advance = voice.advanceFrames(8192, service);
                    const auto snapshot = voice.getSnapshot();
                    return advance.voiceFinished
                        || snapshot.state == drs::engine::RuntimeVoiceLifecycleState::finished;
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
        require(instrumentResult.loaded, "Reference instrument must load before voice runtime tests run.");

        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container must load before voice runtime tests run.");

        {
            drs::engine::RuntimeStreamingService service(
                streamResult.container,
                drs::engine::RuntimeStreamingServiceOptions { "custom", 4, 6000 });
            drs::engine::RuntimeVoice voice;
            std::string errorMessage;

            require(voice.allocate(instrumentResult.instrument,
                                   streamResult.container,
                                   {
                                       101,
                                       "pad-a3",
                                       57,
                                       96,
                                       {
                                           { "tone", 0.5 },
                                           { "motion", 0.25 }
                                       }
                                   },
                                   errorMessage),
                    "Voice allocation for pad-a3 should succeed.");

            const auto initialSnapshot = voice.getSnapshot();
            require(initialSnapshot.state == drs::engine::RuntimeVoiceLifecycleState::active,
                    "Allocated voice should start active.");
            require(initialSnapshot.zoneId == "pad-a3", "Allocated voice zone changed unexpectedly.");
            require(initialSnapshot.groupId == "pad-core", "Allocated voice group changed unexpectedly.");
            require(initialSnapshot.articulationId == "sustain", "Allocated voice articulation changed unexpectedly.");
            require(initialSnapshot.sampleId == "sine-a3", "Allocated voice sample changed unexpectedly.");
            require(initialSnapshot.cursor.prefetchBytes == 16384, "Allocated voice prefetch budget changed unexpectedly.");
            require(initialSnapshot.macroValues.size() == 2, "Allocated voice macro snapshot changed unexpectedly.");

            const auto headAdvance = voice.advanceFrames(4096, service);
            require(headAdvance.advanced, "Voice should advance through the entire prefetch head.");
            require(headAdvance.framesAdvanced == 4096, "Voice prefetch-head advance changed unexpectedly.");
            require(!headAdvance.waitingForPage, "Voice should not wait for stream pages while still in the head.");

            const auto firstStreamBoundary = voice.advanceFrames(64, service);
            require(firstStreamBoundary.waitingForPage, "Voice should wait when it first reaches stream pages.");
            require(firstStreamBoundary.queuedPageRead, "Voice should queue a background page read at the stream boundary.");
            require(voice.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::waitingForPage,
                    "Voice should enter waiting state at the first stream boundary.");

            require(waitUntil([&] { return service.isPageReady({ "sine-a3", 0 }); }, std::chrono::milliseconds(300)),
                    "First stream page for sine-a3 did not become ready in time.");

            const auto streamedAdvance = voice.advanceFrames(64, service);
            require(streamedAdvance.advanced, "Voice should resume once the first page is ready.");
            require(streamedAdvance.acquiredPageLease, "Voice should acquire a page lease when resuming from wait.");
            require(voice.getSnapshot().cursor.currentLeaseId != 0, "Voice should hold a page lease while reading a stream page.");

            voice.reset(service);
            const auto resetSnapshot = voice.getSnapshot();
            require(resetSnapshot.state == drs::engine::RuntimeVoiceLifecycleState::idle,
                    "Voice reset should return the voice to idle.");
            require(resetSnapshot.cursor.currentLeaseId == 0, "Voice reset should release any held page lease.");
            require(service.getMetrics().activeLeaseCount == 0, "Voice reset should not leave stale active leases.");
        }

        {
            drs::engine::RuntimeStreamingService service(
                streamResult.container,
                drs::engine::RuntimeStreamingServiceOptions { "custom", 6, 5000 });
            drs::engine::RuntimeVoice voiceA;
            drs::engine::RuntimeVoice voiceB;
            drs::engine::RuntimeVoice voiceC;
            std::string errorMessage;

            require(voiceA.allocate(instrumentResult.instrument,
                                    streamResult.container,
                                    { 201, "pad-a3", 57, 110, { { "tone", 0.35 } } },
                                    errorMessage),
                    "First polyphony voice should allocate.");
            require(voiceB.allocate(instrumentResult.instrument,
                                    streamResult.container,
                                    { 202, "lead-a4", 69, 105, { { "motion", 0.4 } } },
                                    errorMessage),
                    "Second polyphony voice should allocate.");
            require(voiceC.allocate(instrumentResult.instrument,
                                    streamResult.container,
                                    { 203, "pad-a3", 60, 100, { { "tone", 0.7 }, { "motion", 0.1 } } },
                                    errorMessage),
                    "Third polyphony voice should allocate.");

            require(voiceA.advanceFrames(4096, service).framesAdvanced == 4096,
                    "First polyphony voice head advance changed unexpectedly.");
            require(voiceB.advanceFrames(2048, service).framesAdvanced == 2048,
                    "Second polyphony voice head advance changed unexpectedly.");
            require(voiceC.advanceFrames(4096, service).framesAdvanced == 4096,
                    "Third polyphony voice head advance changed unexpectedly.");

            require(voiceA.advanceFrames(32, service).waitingForPage,
                    "First polyphony voice should wait when it reaches the streamed region.");
            require(voiceB.advanceFrames(32, service).waitingForPage,
                    "Second polyphony voice should wait when it reaches the streamed region.");
            require(voiceC.advanceFrames(32, service).waitingForPage,
                    "Third polyphony voice should wait when it reaches the streamed region.");

            require(waitUntil(
                        [&]
                        {
                            return service.isPageReady({ "sine-a3", 0 })
                                && service.isPageReady({ "triangle-a4", 0 });
                        },
                        std::chrono::milliseconds(400)),
                    "Polyphony test pages did not become ready in time.");

            const auto resumeA = voiceA.advanceFrames(64, service);
            const auto resumeB = voiceB.advanceFrames(64, service);
            const auto resumeC = voiceC.advanceFrames(64, service);

            require(resumeA.advanced && resumeB.advanced && resumeC.advanced,
                    "Polyphony voices should all resume once their pages are ready.");
            require(voiceA.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::active,
                    "Polyphony voice A should return to active state after resuming.");
            require(voiceB.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::active,
                    "Polyphony voice B should return to active state after resuming.");
            require(voiceC.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::active,
                    "Polyphony voice C should return to active state after resuming.");

            const auto midMetrics = service.getMetrics();
            require(midMetrics.activeLeaseCount >= 3,
                    "Polyphony resume should hold leases for all active streamed voices.");
            require(midMetrics.backgroundReadCount == 2,
                    "Duplicate polyphony requests should coalesce to one background read per unique page.");

            voiceA.beginRelease();
            voiceB.beginRelease();
            voiceC.beginRelease();

            drainVoiceToFinished(voiceA, service);
            drainVoiceToFinished(voiceB, service);
            drainVoiceToFinished(voiceC, service);

            require(voiceA.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished,
                    "Voice A should finish after release and drain.");
            require(voiceB.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished,
                    "Voice B should finish after release and drain.");
            require(voiceC.getSnapshot().state == drs::engine::RuntimeVoiceLifecycleState::finished,
                    "Voice C should finish after release and drain.");
            require(voiceA.getSnapshot().cursor.currentLeaseId == 0
                        && voiceB.getSnapshot().cursor.currentLeaseId == 0
                        && voiceC.getSnapshot().cursor.currentLeaseId == 0,
                    "Finished voices should not retain stale page leases.");
            require(service.getMetrics().activeLeaseCount == 0,
                    "Finished polyphony voices should not leave stale active leases behind.");

            require(voiceA.allocate(instrumentResult.instrument,
                                    streamResult.container,
                                    { 301, "lead-a4", 72, 90, { { "motion", 0.9 } } },
                                    errorMessage),
                    "Finished voice should be reusable for a later allocation.");
            const auto reusedSnapshot = voiceA.getSnapshot();
            require(reusedSnapshot.voiceId == 301 && reusedSnapshot.zoneId == "lead-a4",
                    "Reused voice allocation did not replace the previous routing state cleanly.");
            require(reusedSnapshot.macroValues.size() == 1
                        && reusedSnapshot.macroValues[0].id == "motion",
                    "Reused voice allocation did not replace the previous macro snapshot cleanly.");
        }

        {
            drs::engine::RuntimeStreamingService service(
                streamResult.container,
                drs::engine::RuntimeStreamingServiceOptions { "custom", 2, 2000 });
            drs::engine::RuntimeVoice invalidVoice;
            std::string errorMessage;
            require(!invalidVoice.allocate(instrumentResult.instrument,
                                           streamResult.container,
                                           { 401, "missing-zone", 60, 100, {} },
                                           errorMessage),
                    "Voice allocation should reject unknown zones.");
            require(!errorMessage.empty(), "Invalid voice allocation should report an actionable error.");
            invalidVoice.reset(service);
        }

        std::cout << "Phase 1 voice runtime tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 voice runtime tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
