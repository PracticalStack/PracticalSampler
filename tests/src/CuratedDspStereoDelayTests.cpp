#include "drs/engine/DspStereoDelay.h"
#include "drs/engine/CuratedDspCatalog.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace { void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); } }

int main()
{
    using namespace drs::engine;
    try
    {
        const SamplerRenderControlValues::TransportView defaultTransport;
        require(defaultTransport.tempoBpm == 120.0 && defaultTransport.timeSignatureNumerator == 4
                    && defaultTransport.timeSignatureDenominator == 4 && !defaultTransport.valid
                    && !defaultTransport.hasTempo && !defaultTransport.hasSamplePosition
                    && !defaultTransport.hasTimeSignature,
                "The core transport view must use numeric frozen defaults and explicit availability flags.");
        DspStereoDelayState state;
        require(state.prepare(48000.0), "Delay must preallocate bounded state during preparation.");
        const auto* delayDescriptor = findCuratedDspEffect("drs.stereoDelay", 1);
        require(delayDescriptor != nullptr
                    && delayDescriptor->cost.stateBytes
                        == 2u * DspStereoDelayState::maximumDelayFrames * sizeof(float)
                    && state.left.size() == DspStereoDelayState::maximumDelayFrames
                    && state.right.size() == DspStereoDelayState::maximumDelayFrames,
                "The catalog memory request must exactly describe the bounded preallocated delay lines.");
        std::array<float, 481> signal {};
        signal[0] = 1.0f;
        float* mono[] { signal.data() };
        DspStereoDelayParameters free; free.timeMs = 10.0; free.feedback = 0.0; free.mix = 1.0;
        processDspStereoDelay({ mono, 1, 481 }, state, free, {});
        require(std::abs(signal[480] - 1.0f) < 1.0e-6f,
                "Free delay time must place a 10 ms echo at 480 samples.");
        for (const auto sampleRate : { 44100.0, 96000.0 })
        {
            DspStereoDelayState rateState;
            rateState.prepare(sampleRate);
            const auto frames = static_cast<std::size_t>(std::lround(sampleRate * .01)) + 1;
            std::vector<float> rateSignal(frames, 0.0f);
            rateSignal.front() = 1.0f;
            float* rateChannel[] { rateSignal.data() };
            processDspStereoDelay({ rateChannel, 1, static_cast<std::uint32_t>(frames) }, rateState, free, {});
            require(std::abs(rateSignal.back() - 1.0f) < 1.0e-6f,
                    "Free delay time must scale exactly with the prepared sample rate.");
        }
        state.reset(); signal.fill(0.0f); signal[0] = 1.0f;
        DspStereoDelayParameters synced; synced.sync = 1.0; synced.divisionBeats = 0.25; synced.feedback = 0.0; synced.mix = 1.0;
        processDspStereoDelay({ mono, 1, 481 }, state, synced, { 120.0, true, true });
        require(std::abs(signal[480]) < 1.0e-6f,
                "Tempo sync must not emit a quarter-beat echo before its 6000-sample position.");
        for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
        {
            for (const auto tempo : { 60.0, 120.0, 240.0 })
            {
                DspStereoDelayState syncState;
                syncState.prepare(sampleRate);
                const auto expectedDelayFrames = sampleRate * 60.0 / tempo * .25;
                const auto wholeFrames = static_cast<std::size_t>(std::floor(expectedDelayFrames));
                const auto fractionalFrames = static_cast<float>(expectedDelayFrames - wholeFrames);
                const auto frames = wholeFrames + 2;
                std::vector<float> syncSignal(frames, 0.0f);
                syncSignal.front() = 1.0f;
                float* syncChannel[] { syncSignal.data() };
                processDspStereoDelay({ syncChannel, 1, static_cast<std::uint32_t>(frames) }, syncState, synced,
                                      { tempo, true, true, true });
                const auto firstExpected = 1.0f - fractionalFrames;
                const auto secondExpected = fractionalFrames;
                require(std::abs(syncSignal[wholeFrames] - firstExpected) < 1.0e-5f
                            && std::abs(syncSignal[wholeFrames + 1] - secondExpected) < 1.0e-5f,
                        "Tempo divisions must resolve to exact fractional sample positions at supported rates and tempos.");
            }
        }
        DspStereoDelayState fallbackTempoState;
        fallbackTempoState.prepare(48000.0);
        std::vector<float> fallbackTempoSignal(6001, 0.0f);
        fallbackTempoSignal.front() = 1.0f;
        float* fallbackTempoChannel[] { fallbackTempoSignal.data() };
        processDspStereoDelay({ fallbackTempoChannel, 1, static_cast<std::uint32_t>(fallbackTempoSignal.size()) },
                              fallbackTempoState, synced, { 10.0, false, false, false });
        require(std::abs(fallbackTempoSignal.back() - 1.0f) < 1.0e-6f,
                "Missing or invalid transport tempo must use the frozen 120 BPM fallback.");
        state.reset();
        std::array<float, 97> left {}, right {};
        left[0] = 1.0f;
        float* stereo[] { left.data(), right.data() };
        DspStereoDelayParameters ping; ping.timeMs = 1.0; ping.feedback = 0.5; ping.pingPong = 1.0; ping.mix = 1.0;
        processDspStereoDelay({ stereo, 2, 97 }, state, ping, {});
        require(std::abs(right[96]) > 0.1f,
                "Ping-pong feedback must transfer a left impulse to the right delay channel.");
        std::array<float, 128> whole {}, splitA {}, splitB {};
        whole[0] = splitA[0] = 1.0f;
        float* wholeChannel[] { whole.data() };
        float* splitAChannel[] { splitA.data() };
        float* splitBChannel[] { splitB.data() };
        DspStereoDelayState wholeState, splitState;
        wholeState.prepare(48000.0); splitState.prepare(48000.0);
        DspStereoDelayParameters partitioned; partitioned.timeMs = 1.0; partitioned.feedback = .4; partitioned.mix = 1.0;
        processDspStereoDelay({ wholeChannel, 1, 128 }, wholeState, partitioned, {});
        processDspStereoDelay({ splitAChannel, 1, 64 }, splitState, partitioned, {});
        processDspStereoDelay({ splitBChannel, 1, 64 }, splitState, partitioned, {});
        for (std::size_t frame = 0; frame < whole.size(); ++frame)
        {
            const auto splitValue = frame < 64 ? splitA[frame] : splitB[frame - 64];
            require(std::abs(whole[frame] - splitValue) < 1.0e-6f,
                    "Delay feedback state must remain stable across callback partitions.");
        }
        DspStereoDelayState automatedState;
        automatedState.prepare(1000.0);
        std::array<float, 8> automated {};
        automated[0] = 1.0f;
        float* automatedChannel[] { automated.data() };
        DspStereoDelayParameters automationStart; automationStart.timeMs = 1.0; automationStart.feedback = 0.0; automationStart.mix = 0.0;
        DspStereoDelayParameters automationEnd = automationStart; automationEnd.mix = 1.0;
        processDspStereoDelayRamp({ automatedChannel, 1, static_cast<std::uint32_t>(automated.size()) },
                                  automatedState, automationStart, automationEnd, {});
        require(std::abs(automated[1] - 0.25f) < 1.0e-6f,
                "Delay automation must interpolate the wet mix per sample instead of jumping to the block endpoint.");
        DspStereoDelayState tailState;
        tailState.prepare(48000.0);
        std::array<float, 49> tailInput {};
        tailInput[0] = 1.0f;
        float* tailInputChannel[] { tailInput.data() };
        DspStereoDelayParameters tailParameters; tailParameters.timeMs = 1.0; tailParameters.feedback = .5; tailParameters.tone = 1.0; tailParameters.mix = 1.0;
        processDspStereoDelay({ tailInputChannel, 1, static_cast<std::uint32_t>(tailInput.size()) }, tailState, tailParameters, {});
        std::array<float, 48> releaseTail {};
        float* releaseTailChannel[] { releaseTail.data() };
        processDspStereoDelay({ releaseTailChannel, 1, static_cast<std::uint32_t>(releaseTail.size()) }, tailState, tailParameters, {});
        require(std::abs(releaseTail.back() - .5f) < 1.0e-6f,
                "A normal release must retain feedback delay memory when no new input arrives.");
        const auto* tailLeftAllocation = tailState.left.data();
        const auto* tailRightAllocation = tailState.right.data();
        tailState.prepare(96000.0);
        releaseTail.fill(0.0f);
        processDspStereoDelay({ releaseTailChannel, 1, static_cast<std::uint32_t>(releaseTail.size()) }, tailState, tailParameters, {});
        require(tailState.left.data() == tailLeftAllocation && tailState.right.data() == tailRightAllocation
                    && std::all_of(releaseTail.begin(), releaseTail.end(), [](const auto value) { return value == 0.0f; }),
                "A sample-rate change must reuse bounded storage and clear delay memory deterministically.");
        tailState.reset();
        releaseTail.fill(0.0f);
        processDspStereoDelay({ releaseTailChannel, 1, static_cast<std::uint32_t>(releaseTail.size()) }, tailState, tailParameters, {});
        require(std::abs(releaseTail.back()) < 1.0e-6f,
                "A panic reset must clear delay memory before the next callback.");
        const auto* leftAllocation = state.left.data();
        const auto* rightAllocation = state.right.data();
        processDspStereoDelay({ mono, 1, 481 }, state, synced, { 240.0, true, true, true });
        require(state.left.data() == leftAllocation && state.right.data() == rightAllocation,
                "Tempo and parameter changes must not reallocate preallocated delay memory.");
        state.reset(); signal.fill(0.0f); signal[0] = 1.0f;
        DspStereoDelayParameters feedback; feedback.timeMs = 1.0; feedback.feedback = 1.0; feedback.mix = 1.0;
        processDspStereoDelay({ mono, 1, 480 }, state, feedback, {});
        for (const auto value : signal) require(std::isfinite(value) && std::abs(value) <= 1.0f,
                                                 "Feedback must be finite and constrained by the 0.95 ceiling.");
        state.reset(); signal.fill(0.0f); signal[0] = 1.0f;
        processDspStereoDelay({ mono, 1, 480 }, state, free, {});
        require(std::abs(signal[0]) < 1.0e-6f, "Reset must clear pending delay memory deterministically.");
        std::cout << "Curated DSP stereo-delay vectors passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception) { std::cerr << exception.what() << std::endl; return 1; }
}
