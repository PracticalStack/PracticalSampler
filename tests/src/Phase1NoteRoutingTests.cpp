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
        const auto instrumentResult = drs::engine::loadPhase1ReferenceInstrumentManifest();
        require(instrumentResult.loaded, "Reference instrument must load before note-routing tests run.");

        const auto streamResult = drs::engine::loadPhase1ReferenceStreamContainer();
        require(streamResult.loaded, "Reference stream-container must load before note-routing tests run.");

        const auto defaultSoftRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 901, "", 57, 64, {}, "" });
        require(defaultSoftRoute.resolved, "Default-articulation soft route should resolve.");
        require(defaultSoftRoute.usedDefaultArticulation,
                "Default-articulation soft route should use the manifest default articulation.");
        require(defaultSoftRoute.zoneId == "pad-a3",
                "Default-articulation soft route should resolve to the base sustain zone.");
        require(defaultSoftRoute.articulationId == "sustain"
                    && defaultSoftRoute.sampleId == "sine-a3",
                "Default-articulation soft route changed unexpectedly.");

        const auto defaultAccentRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 902, "", 57, 120, {}, "" });
        require(defaultAccentRoute.resolved, "Default-articulation accent route should resolve.");
        require(defaultAccentRoute.zoneId == "pad-a3-accent",
                "High-velocity sustain route should resolve to the accent sustain zone.");

        const auto leadSoftRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 903, "", 69, 72, {}, "lead" });
        require(leadSoftRoute.resolved, "Lead soft route should resolve.");
        require(!leadSoftRoute.usedDefaultArticulation,
                "Explicit articulation routing should not report default-articulation use.");
        require(leadSoftRoute.zoneId == "lead-a4",
                "Lead soft route should resolve to the base lead zone.");
        require(leadSoftRoute.articulationId == "lead"
                    && leadSoftRoute.sampleId == "triangle-a4",
                "Lead soft route changed unexpectedly.");

        const auto leadAccentRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 904, "", 69, 120, {}, "lead" });
        require(leadAccentRoute.resolved, "Lead accent route should resolve.");
        require(leadAccentRoute.zoneId == "lead-a4-accent",
                "High-velocity lead route should resolve to the accent lead zone.");

        const auto unknownArticulationRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 905, "", 69, 100, {}, "unknown-articulation" });
        require(!unknownArticulationRoute.resolved,
                "Unknown articulation should be rejected during route resolution.");
        require(unknownArticulationRoute.state.find("unknown articulation") != std::string::npos,
                "Unknown articulation rejection should stay actionable.");

        const auto outOfRangeRoute = drs::engine::resolveRuntimeVoiceRoute(
            instrumentResult.instrument,
            streamResult.container,
            { 906, "", 24, 100, {}, "lead" });
        require(!outOfRangeRoute.resolved,
                "Out-of-range note should be rejected during route resolution.");
        require(outOfRangeRoute.state.find("could not find a matching zone") != std::string::npos,
                "Out-of-range route rejection should explain the missing mapping.");

        drs::engine::RuntimeStreamingService service(
            streamResult.container,
            drs::engine::RuntimeStreamingServiceOptions { "custom", 4, 5000 });
        drs::engine::RuntimeVoice sustainVoice;
        drs::engine::RuntimeVoice leadVoice;
        std::string errorMessage;

        require(sustainVoice.allocate(instrumentResult.instrument,
                                      streamResult.container,
                                      { 1001, "", 57, 120, { { "tone", 0.6 } }, "" },
                                      errorMessage),
                "Accent sustain allocation through note routing should succeed.");
        const auto sustainSnapshot = sustainVoice.getSnapshot();
        require(sustainSnapshot.zoneId == "pad-a3-accent"
                    && sustainSnapshot.sampleId == "sine-a3",
                "Accent sustain allocation did not keep the resolved route.");

        require(leadVoice.allocate(instrumentResult.instrument,
                                   streamResult.container,
                                   { 1002, "", 69, 120, { { "motion", 0.45 } }, "lead" },
                                   errorMessage),
                "Accent lead allocation through note routing should succeed.");
        const auto leadSnapshot = leadVoice.getSnapshot();
        require(leadSnapshot.zoneId == "lead-a4-accent"
                    && leadSnapshot.sampleId == "triangle-a4",
                "Accent lead allocation did not keep the resolved route.");

        require(sustainVoice.advanceFrames(4096, service).framesAdvanced == 4096,
                "Routed sustain voice should advance through the prefetch head.");
        require(sustainVoice.advanceFrames(64, service).waitingForPage,
                "Routed sustain voice should wait at the first streamed page boundary.");
        require(waitUntil([&] { return service.isPageReady({ "sine-a3", 0 }); }, std::chrono::milliseconds(300)),
                "Routed sustain page did not become ready in time.");
        require(sustainVoice.advanceFrames(64, service).acquiredPageLease,
                "Routed sustain voice should resume once its page is ready.");

        std::cout << "Phase 1 note routing tests passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Phase 1 note routing tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
