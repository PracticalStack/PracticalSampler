#include "drs/engine/DspStereoDelay.h"

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
        DspStereoDelayState state;
        require(state.prepare(48000.0), "Delay must preallocate bounded state during preparation.");
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
