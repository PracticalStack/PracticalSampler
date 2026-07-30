#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspAlgorithmicReverb.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace
{
void require(const bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

float energy(const std::vector<float>& values, const std::size_t begin)
{
    return std::accumulate(values.begin() + static_cast<std::ptrdiff_t>(begin), values.end(), 0.0f,
                           [](const float sum, const float value) { return sum + std::abs(value); });
}
}

int main()
{
    using namespace drs::engine;
    try
    {
        DspAlgorithmicReverbState state;
        require(state.prepare(48000.0), "Reverb must preallocate its bounded state during preparation.");
        const auto* descriptor = findCuratedDspEffect("drs.algorithmicReverb", 1);
        require(descriptor != nullptr && descriptor->cost.stateBytes == DspAlgorithmicReverbState::maximumStateBytes
                    && descriptor->cost.stateBytes <= 512u * 1024u,
                "The catalog must exactly declare the bounded 96 kHz reverb memory request.");
        require(descriptor->parameters.size() == 6, "Reverb v1 must expose its complete frozen parameter surface.");

        constexpr std::size_t firstLineFrame = 2064;
        std::vector<float> impulse(firstLineFrame + 2, 0.0f);
        impulse.front() = 1.0f;
        float* impulseChannel[] { impulse.data() };
        DspAlgorithmicReverbParameters immediate; immediate.preDelayMs = 0.0; immediate.size = 1.0;
        immediate.decaySeconds = 2.0; immediate.damping = 0.0; immediate.mix = 1.0;
        processDspAlgorithmicReverb({ impulseChannel, 1, static_cast<std::uint32_t>(impulse.size()) }, state, immediate);
        require(std::abs(impulse[firstLineFrame]) > .01f,
                "The fixed FDN topology must emit its earliest impulse response at the documented line position.");

        state.reset();
        std::vector<float> delayedImpulse(3026, 0.0f);
        delayedImpulse.front() = 1.0f;
        float* delayedChannel[] { delayedImpulse.data() };
        auto preDelayed = immediate; preDelayed.preDelayMs = 20.0;
        processDspAlgorithmicReverb({ delayedChannel, 1, static_cast<std::uint32_t>(delayedImpulse.size()) }, state, preDelayed);
        require(std::abs(delayedImpulse[3023]) < 1.0e-6f && std::abs(delayedImpulse[3024]) > .01f,
                "Pre-delay must shift the first FDN response by its exact prepared sample count.");

        std::vector<float> widthLeft(firstLineFrame + 2, 0.0f), widthRight(firstLineFrame + 2, 0.0f);
        widthLeft.front() = 1.0f;
        float* widthChannels[] { widthLeft.data(), widthRight.data() };
        state.reset();
        auto narrow = immediate; narrow.width = 0.0;
        processDspAlgorithmicReverb({ widthChannels, 2, static_cast<std::uint32_t>(widthLeft.size()) }, state, narrow);
        require(std::abs(widthLeft[firstLineFrame] - widthRight[firstLineFrame]) < 1.0e-6f,
                "Zero reverb width must collapse the wet output to mono.");
        widthLeft.assign(firstLineFrame + 2, 0.0f); widthRight.assign(firstLineFrame + 2, 0.0f); widthLeft.front() = 1.0f;
        state.reset();
        auto wide = immediate; wide.width = 1.0;
        processDspAlgorithmicReverb({ widthChannels, 2, static_cast<std::uint32_t>(widthLeft.size()) }, state, wide);
        require(std::abs(widthLeft[firstLineFrame] - widthRight[firstLineFrame]) > .01f,
                "Full reverb width must preserve the stereo FDN image.");

        std::vector<float> bright(8192, 0.0f), dark(8192, 0.0f);
        bright.front() = dark.front() = 1.0f;
        float* brightChannel[] { bright.data() };
        float* darkChannel[] { dark.data() };
        DspAlgorithmicReverbState brightState, darkState;
        brightState.prepare(48000.0); darkState.prepare(48000.0);
        processDspAlgorithmicReverb({ brightChannel, 1, static_cast<std::uint32_t>(bright.size()) }, brightState, immediate);
        auto darkParameters = immediate; darkParameters.damping = 1.0;
        processDspAlgorithmicReverb({ darkChannel, 1, static_cast<std::uint32_t>(dark.size()) }, darkState, darkParameters);
        require(energy(bright, 3000) > energy(dark, 3000),
                "Damping must attenuate the reverb's later feedback energy.");

        const auto* preDelayAllocation = state.preDelayLeft.data();
        const auto* firstLineAllocation = state.lines.front().left.data();
        state.prepare(96000.0);
        require(state.preDelayLeft.data() == preDelayAllocation && state.lines.front().left.data() == firstLineAllocation,
                "Sample-rate preparation must reuse the fixed reverb allocations.");
        std::array<float, 64> cleared {};
        float* clearedChannel[] { cleared.data() };
        processDspAlgorithmicReverb({ clearedChannel, 1, static_cast<std::uint32_t>(cleared.size()) }, state, immediate);
        require(std::all_of(cleared.begin(), cleared.end(), [](const auto value) { return value == 0.0f; }),
                "Sample-rate preparation must reset every reverb line deterministically.");

        std::array<float, 128> automated {};
        automated.front() = 1.0f;
        float* automatedChannel[] { automated.data() };
        state.prepare(48000.0);
        auto automationStart = immediate; automationStart.mix = 0.0;
        auto automationEnd = immediate; automationEnd.mix = 1.0; automationEnd.size = 0.0; automationEnd.damping = 1.0;
        processDspAlgorithmicReverbRamp({ automatedChannel, 1, static_cast<std::uint32_t>(automated.size()) }, state,
                                        automationStart, automationEnd);
        require(std::all_of(automated.begin(), automated.end(), [](const auto value) { return std::isfinite(value); }),
                "Reverb parameter automation must remain finite for the entire callback block.");

        std::array<float, 4> bypass { -.5f, .25f, .75f, -.125f };
        const auto bypassReference = bypass;
        float* bypassChannel[] { bypass.data() };
        state.reset();
        auto bypassParameters = immediate; bypassParameters.mix = 0.0;
        processDspAlgorithmicReverb({ bypassChannel, 1, static_cast<std::uint32_t>(bypass.size()) }, state,
                                    bypassParameters);
        require(bypass == bypassReference,
                "A zero-wet reverb bypass must preserve the dry callback samples exactly.");

        state.reset();
        std::vector<float> resetProbe(firstLineFrame + 2, 0.0f);
        float* resetChannel[] { resetProbe.data() };
        processDspAlgorithmicReverb({ resetChannel, 1, static_cast<std::uint32_t>(resetProbe.size()) }, state, immediate);
        require(std::all_of(resetProbe.begin(), resetProbe.end(), [](const auto value) { return value == 0.0f; }),
                "Reset must clear all pending reverb tail memory.");

        std::array<std::array<float, 512>, 6> benchmarkBlocks {};
        std::array<DspAlgorithmicReverbState, 6> benchmarkStates {};
        for (auto& benchmarkState : benchmarkStates) benchmarkState.prepare(48000.0);
        std::array<std::chrono::steady_clock::duration, 128> callbackDurations {};
        for (std::size_t callback = 0; callback < 128; ++callback)
        {
            for (auto& benchmarkBlock : benchmarkBlocks) std::fill(benchmarkBlock.begin(), benchmarkBlock.end(), .1f);
            const auto started = std::chrono::steady_clock::now();
            for (std::size_t instance = 0; instance < benchmarkStates.size(); ++instance)
            {
                float* benchmarkChannel[] { benchmarkBlocks[instance].data() };
                processDspAlgorithmicReverb({ benchmarkChannel, 1,
                                             static_cast<std::uint32_t>(benchmarkBlocks[instance].size()) },
                                            benchmarkStates[instance], immediate);
            }
            callbackDurations[callback] = std::chrono::steady_clock::now() - started;
        }
        std::sort(callbackDurations.begin(), callbackDurations.end());
        const auto p99Callback = callbackDurations[126];
        if (p99Callback >= std::chrono::microseconds(512000000 / 48000 / 2))
        {
            throw std::runtime_error(
                "The six-instance legal reverb graph p99 must remain below half of the 512-frame callback budget at 48 kHz; observed "
                + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(p99Callback).count()) + " us.");
        }
        std::cout << "Curated DSP reverb vectors passed (p99 legal six-instance 512-frame callback "
                  << std::chrono::duration_cast<std::chrono::microseconds>(p99Callback).count()
                  << " us; max "
                  << std::chrono::duration_cast<std::chrono::microseconds>(callbackDurations.back()).count()
                  << " us)." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP reverb vectors failed: " << exception.what() << std::endl;
        return 1;
    }
}
