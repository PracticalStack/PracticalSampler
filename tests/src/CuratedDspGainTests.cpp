#include "drs/engine/DspGain.h"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void requireNear(float actual, float expected, const char* message)
{
    if (std::fabs(actual - expected) > 1.0e-5f) throw std::runtime_error(message);
}
} // namespace

int main()
{
    try
    {
        using namespace drs::engine;
        std::array<float, 3> mono { 0.25f, -0.5f, 1.0f };
        std::array<float, 3> stereoRight { -0.25f, 0.5f, -1.0f };
        float* channels[] { mono.data(), stereoRight.data() };
        const SamplerAudioBufferView stereo { channels, 2, 3 };

        processDspGain(stereo, {});
        requireNear(mono[0], 0.25f, "Unity Gain must be sample-identical on the left channel.");
        requireNear(stereoRight[1], 0.5f, "Unity Gain must be sample-identical on the right channel.");

        processDspGain(stereo, { 6.0, 0.0, 0.0 });
        requireNear(mono[0], 0.4988156f, "Positive dB Gain must use amplitude-domain decibels.");
        requireNear(stereoRight[0], -0.4988156f, "Positive dB Gain must preserve stereo channel polarity.");

        processDspGain(stereo, { -6.0, 1.0, 0.0 });
        requireNear(mono[0], -0.25f, "Negative dB plus polarity must be deterministic.");
        requireNear(stereoRight[0], 0.25f, "Polarity must apply to both channels.");

        processDspGain(stereo, { 0.0, 0.0, 1.0 });
        require(mono[0] == 0.0f && mono[1] == 0.0f && mono[2] == 0.0f
                    && stereoRight[0] == 0.0f && stereoRight[1] == 0.0f && stereoRight[2] == 0.0f,
                "Mute must clear every mono/stereo sample exactly.");
        requireNear(computeDspGainLinear({ 1000.0, 0.0, 0.0 }), 15.848932f,
                    "Gain must clamp to the v1 +24 dB numeric limit.");
        requireNear(computeDspGainLinear({ -1000.0, 0.0, 0.0 }), 0.0000158489f,
                    "Gain must clamp to the v1 -96 dB numeric limit.");

        std::array<float, 1> denormal { 1.0e-36f };
        float* monoChannel[] { denormal.data() };
        processDspGain({ monoChannel, 1, 1 }, { -96.0, 0.0, 0.0 });
        require(denormal[0] == 0.0f, "Subnormal Gain output must be flushed to zero.");

        std::vector<float> oneBlock(480, 1.0f);
        std::vector<float> splitFirst(240, 1.0f);
        std::vector<float> splitSecond(240, 1.0f);
        float* oneBlockChannel[] { oneBlock.data() };
        float* splitFirstChannel[] { splitFirst.data() };
        float* splitSecondChannel[] { splitSecond.data() };
        processDspGainRamp({ oneBlockChannel, 1, 480 }, { 0.0, 0.0, 0.0 }, { 6.0, 0.0, 0.0 });
        processDspGainRamp({ splitFirstChannel, 1, 240 }, { 0.0, 0.0, 0.0 }, { 3.0, 0.0, 0.0 });
        processDspGainRamp({ splitSecondChannel, 1, 240 }, { 3.0, 0.0, 0.0 }, { 6.0, 0.0, 0.0 });
        for (std::size_t frame = 0; frame < oneBlock.size(); ++frame)
        {
            const auto partitioned = frame < splitFirst.size()
                ? splitFirst[frame] : splitSecond[frame - splitFirst.size()];
            requireNear(oneBlock[frame], partitioned,
                        "Gain smoothing must produce the same ramp across block partitions.");
        }

        std::cout << "Curated DSP Gain v1 golden vectors passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP Gain tests failed: " << exception.what() << std::endl;
        return 1;
    }
}
