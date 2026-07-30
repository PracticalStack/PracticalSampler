#include "drs/engine/DspGain.h"
#include "drs/engine/DspSaturator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace
{
void require(const bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

void requireNear(const float left, const float right, const char* message)
{
    if (std::abs(left - right) > 1.0e-5f) throw std::runtime_error(message);
}
}

int main()
{
    using namespace drs::engine;
    try
    {
        std::array<float, 5> left { -2.0f, -0.5f, 0.0f, 0.5f, 2.0f };
        std::array<float, 5> right = left;
        float* stereoChannels[] { left.data(), right.data() };
        DspSaturatorState state;
        state.prepare(48000.0);
        processDspSaturator({ stereoChannels, 2, 5 }, state, { 1.0, 0.0, 1.0, 1.0, 0.0 });
        requireNear(left.front(), -1.0f, "Hard clip must cap negative input at -1.");
        requireNear(left.back(), 1.0f, "Hard clip must cap positive input at 1.");
        requireNear(left[2], 0.0f, "Zero input must remain zero through saturator v1.");
        require(left[1] < 0.0f && right[3] > 0.0f,
                "Stereo polarity must remain independent and deterministic.");

        std::array<float, 2> invalid { std::numeric_limits<float>::quiet_NaN(),
                                       std::numeric_limits<float>::infinity() };
        float* invalidChannel[] { invalid.data() };
        state.reset();
        processDspSaturator({ invalidChannel, 1, 2 }, state, {});
        require(invalid[0] == 0.0f && invalid[1] == 0.0f,
                "NaN and infinity must be contained before reaching output.");

        std::array<float, 8> impulse {};
        impulse.front() = 1.0f;
        const auto impulseReference = impulse;
        float* impulseChannel[] { impulse.data() };
        state.prepare(48000.0);
        processDspSaturator({ impulseChannel, 1, 8 }, state, { 0.0, 12.0, 1.0, 1.0, 0.0 });
        require(std::isfinite(impulse.front()) && impulse.front() > 0.0f
                    && std::all_of(impulse.begin() + 1, impulse.end(), [](const float value)
                    { return value == 0.0f; }),
                "Impulse and silence tails must remain deterministic for the zero-tail saturator.");
        std::array<float, 8> bypass = impulseReference;
        float* bypassChannel[] { bypass.data() };
        state.reset();
        processDspSaturatorBypassRamp({ bypassChannel, 1, 8 }, state, {}, {}, 0.0f, 0.0f);
        require(bypass == impulseReference, "A fully bypassed Saturator must preserve dry samples exactly.");

        std::array<float, 64> sine {};
        for (std::size_t frame = 0; frame < sine.size(); ++frame)
            sine[frame] = static_cast<float>(std::sin(2.0 * 3.141592653589793 * frame / sine.size()));
        float* sineChannel[] { sine.data() };
        state.prepare(48000.0);
        processDspSaturator({ sineChannel, 1, static_cast<std::uint32_t>(sine.size()) }, state,
                            { 2.0, 24.0, 1.0, 1.0, 0.0 });
        require(std::all_of(sine.begin(), sine.end(), [](const float value)
        { return std::isfinite(value) && std::abs(value) <= 1.0f; }),
                "Sine and stepped nonlinear levels must remain finite and bounded.");

        for (const auto sampleRate : { 44100.0, 48000.0, 88200.0, 96000.0 })
        {
            std::array<float, 3> rateVector { -0.75f, 0.25f, 0.75f };
            float* rateChannel[] { rateVector.data() };
            DspSaturatorState rateState;
            rateState.prepare(sampleRate);
            processDspSaturator({ rateChannel, 1, 3 }, rateState, { 0.0, 18.0, 0.25, 1.0, 0.0 });
            require(std::all_of(rateVector.begin(), rateVector.end(), [](const float value)
            { return std::isfinite(value) && std::abs(value) <= 1.0f; }),
                    "Every supported sample rate must produce finite bounded saturator output.");
        }

        std::vector<float> oneBlock(480, 0.75f);
        std::vector<float> splitFirst(240, 0.75f);
        std::vector<float> splitSecond(240, 0.75f);
        float* oneChannel[] { oneBlock.data() };
        float* firstChannel[] { splitFirst.data() };
        float* secondChannel[] { splitSecond.data() };
        DspSaturatorState oneState;
        DspSaturatorState splitState;
        oneState.prepare(48000.0);
        splitState.prepare(48000.0);
        const DspSaturatorParameters start { 0.0, 0.0, 0.5, 0.0, 0.0 };
        const DspSaturatorParameters middle { 0.0, 18.0, 0.75, 0.5, -3.0 };
        const DspSaturatorParameters end { 0.0, 36.0, 1.0, 1.0, -6.0 };
        processDspSaturatorRamp({ oneChannel, 1, 480 }, oneState, start, end);
        processDspSaturatorRamp({ firstChannel, 1, 240 }, splitState, start, middle);
        processDspSaturatorRamp({ secondChannel, 1, 240 }, splitState, middle, end);
        for (std::size_t frame = 0; frame < oneBlock.size(); ++frame)
        {
            const auto partitioned = frame < splitFirst.size() ? splitFirst[frame]
                                                                : splitSecond[frame - splitFirst.size()];
            requireNear(oneBlock[frame], partitioned,
                        "Saturator parameter ramps must remain stable across block partitions.");
        }

        std::array<float, 1> gainThenSaturator { 0.55f };
        std::array<float, 1> saturatorThenGain { 0.55f };
        float* firstOrder[] { gainThenSaturator.data() };
        float* secondOrder[] { saturatorThenGain.data() };
        DspSaturatorState firstOrderState;
        DspSaturatorState secondOrderState;
        firstOrderState.prepare(48000.0);
        secondOrderState.prepare(48000.0);
        processDspGain({ firstOrder, 1, 1 }, { 12.0, 0.0, 0.0 });
        processDspSaturator({ firstOrder, 1, 1 }, firstOrderState, { 1.0, 0.0, 1.0, 1.0, 0.0 });
        processDspSaturator({ secondOrder, 1, 1 }, secondOrderState, { 1.0, 0.0, 1.0, 1.0, 0.0 });
        processDspGain({ secondOrder, 1, 1 }, { 12.0, 0.0, 0.0 });
        require(std::abs(gainThenSaturator[0] - saturatorThenGain[0]) > 0.05f,
                "Gain then Saturator must remain intentionally distinct from Saturator then Gain.");

        std::cout << "Curated DSP saturator vectors passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP saturator vectors failed: " << exception.what() << std::endl;
        return 1;
    }
}
