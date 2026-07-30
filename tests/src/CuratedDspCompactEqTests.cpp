#include "drs/engine/CuratedDspCatalog.h"
#include "drs/engine/DspCompactEq.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

float rms(const std::array<float, 2048>& samples)
{
    double total = 0.0;
    for (const auto sample : samples) total += sample * sample;
    return static_cast<float>(std::sqrt(total / samples.size()));
}
}

int main()
{
    using namespace drs::engine;
    try
    {
        const auto* descriptor = findCuratedDspEffect("drs.compactEq", 1);
        require(descriptor != nullptr && descriptor->parameters.size() == 5
                    && descriptor->cost.stateBytes == DspCompactEqState::maximumStateBytes,
                "Compact EQ v1 requires its frozen five-control catalog contract.");

        DspCompactEqState state;
        require(state.prepare(48000.0), "Compact EQ must prepare without allocating on the audio path.");
        std::array<float, 2048> low {}, high {}, bypass {};
        for (std::size_t index = 0; index < low.size(); ++index)
        {
            low[index] = high[index] = bypass[index] = static_cast<float>(std::sin(2.0 * 3.141592653589793 * 6000.0 * index / 48000.0));
        }
        float* lowChannel[] { low.data() };
        DspCompactEqParameters lowPass; lowPass.mode = 0.0; lowPass.frequencyHz = 800.0; lowPass.mix = 1.0;
        processDspCompactEqRamp({ lowChannel, 1, static_cast<std::uint32_t>(low.size()) }, state, lowPass, lowPass);
        state.reset();
        float* highChannel[] { high.data() };
        DspCompactEqParameters highPass; highPass.mode = 2.0; highPass.frequencyHz = 800.0; highPass.mix = 1.0;
        processDspCompactEqRamp({ highChannel, 1, static_cast<std::uint32_t>(high.size()) }, state, highPass, highPass);
        require(rms(low) < rms(high) * .25f, "Low-pass response must attenuate frequencies well above its frozen cutoff.");

        const auto bypassReference = bypass;
        float* bypassChannel[] { bypass.data() };
        DspCompactEqParameters dry; dry.mix = 0.0;
        processDspCompactEqRamp({ bypassChannel, 1, static_cast<std::uint32_t>(bypass.size()) }, state, dry, dry);
        require(bypass == bypassReference, "Zero wet mix must preserve the Compact EQ input exactly.");

        std::array<float, 4096> left {}, right {};
        left.front() = 1.0f; right.front() = 1.0f;
        float* stereo[] { left.data(), right.data() };
        DspCompactEqParameters sweepStart; sweepStart.mode = 1.0; sweepStart.frequencyHz = 40.0; sweepStart.q = .25; sweepStart.gainDb = -18.0;
        DspCompactEqParameters sweepEnd; sweepEnd.mode = 2.0; sweepEnd.frequencyHz = 18000.0; sweepEnd.q = 12.0; sweepEnd.gainDb = 18.0;
        processDspCompactEqRamp({ stereo, 2, static_cast<std::uint32_t>(left.size()) }, state, sweepStart, sweepEnd);
        require(std::all_of(left.begin(), left.end(), [](float sample) { return std::isfinite(sample) && std::abs(sample) <= 16.0f; })
                    && std::all_of(right.begin(), right.end(), [](float sample) { return std::isfinite(sample) && std::abs(sample) <= 16.0f; }),
                "Extreme Compact EQ sweeps must remain finite and bounded in stereo.");
        state.reset();
        require(state.channels[0].x1 == 0.0f && state.channels[1].y2 == 0.0f,
                "Compact EQ reset must clear both channel histories deterministically.");
        std::cout << "Curated DSP Compact EQ vectors passed." << std::endl;
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Curated DSP Compact EQ vectors failed: " << exception.what() << std::endl;
        return 1;
    }
}
