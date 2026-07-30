#include "drs/engine/DspCompactEq.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
namespace
{
constexpr double pi = 3.14159265358979323846;

float clean(const float value) noexcept
{
    return std::isfinite(value) ? std::clamp(value, -16.0f, 16.0f) : 0.0f;
}

DspCompactEqBiquad makeCoefficients(const DspCompactEqParameters& parameters, const double sampleRate) noexcept
{
    const auto frequency = std::clamp(parameters.frequencyHz, 40.0, std::min(18000.0, sampleRate * .45));
    const auto q = std::clamp(parameters.q, .25, 12.0);
    const auto mode = static_cast<int>(std::lround(std::clamp(parameters.mode, 0.0, 2.0)));
    const auto omega = 2.0 * pi * frequency / sampleRate;
    const auto cosine = std::cos(omega);
    const auto alpha = std::sin(omega) / (2.0 * q);
    const auto amplitude = std::pow(10.0, std::clamp(parameters.gainDb, -18.0, 18.0) / 40.0);
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;
    if (mode == 0)
    {
        b0 = (1.0 - cosine) * .5; b1 = 1.0 - cosine; b2 = b0;
        a0 = 1.0 + alpha; a1 = -2.0 * cosine; a2 = 1.0 - alpha;
    }
    else if (mode == 2)
    {
        b0 = (1.0 + cosine) * .5; b1 = -(1.0 + cosine); b2 = b0;
        a0 = 1.0 + alpha; a1 = -2.0 * cosine; a2 = 1.0 - alpha;
    }
    else
    {
        b0 = 1.0 + alpha * amplitude; b1 = -2.0 * cosine; b2 = 1.0 - alpha * amplitude;
        a0 = 1.0 + alpha / amplitude; a1 = -2.0 * cosine; a2 = 1.0 - alpha / amplitude;
    }
    return { static_cast<float>(b0 / a0), static_cast<float>(b1 / a0), static_cast<float>(b2 / a0),
             static_cast<float>(a1 / a0), static_cast<float>(a2 / a0) };
}

DspCompactEqBiquad interpolate(const DspCompactEqBiquad& start, const DspCompactEqBiquad& end, const float amount) noexcept
{
    const auto lerp = [amount](const float a, const float b) { return a + (b - a) * amount; };
    return { lerp(start.b0, end.b0), lerp(start.b1, end.b1), lerp(start.b2, end.b2),
             lerp(start.a1, end.a1), lerp(start.a2, end.a2) };
}
} // namespace

bool DspCompactEqState::prepare(const double newSampleRate) noexcept
{
    sampleRate = std::clamp(std::isfinite(newSampleRate) ? newSampleRate : 48000.0, 8000.0, 192000.0);
    reset();
    return true;
}

void DspCompactEqState::reset() noexcept
{
    for (auto& channel : channels) channel.reset();
}

void processDspCompactEqRamp(const SamplerAudioBufferView output, DspCompactEqState& state,
                             const DspCompactEqParameters& start, const DspCompactEqParameters& end) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0) return;
    const auto startCoefficients = makeCoefficients(start, state.sampleRate);
    const auto endCoefficients = makeCoefficients(end, state.sampleRate);
    const auto startMode = static_cast<int>(std::lround(std::clamp(start.mode, 0.0, 2.0)));
    const auto endMode = static_cast<int>(std::lround(std::clamp(end.mode, 0.0, 2.0)));
    const auto startMix = static_cast<float>(std::clamp(start.mix, 0.0, 1.0));
    const auto endMix = static_cast<float>(std::clamp(end.mix, 0.0, 1.0));
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto amount = static_cast<float>(frame + 1) / static_cast<float>(output.frameCount);
        // Mode changes switch between independently stable RBJ designs; continuous parameters
        // interpolate their coefficients sample-by-sample to avoid zippering.
        const auto coefficients = startMode == endMode ? interpolate(startCoefficients, endCoefficients, amount)
                                                       : (amount < .5f ? startCoefficients : endCoefficients);
        const auto mix = startMix + (endMix - startMix) * amount;
        for (std::uint32_t channel = 0; channel < std::min<std::uint32_t>(output.channelCount, 2); ++channel)
        {
            const auto input = clean(output.channels[channel][frame]);
            auto& filter = state.channels[channel];
            const auto wet = clean(coefficients.b0 * input + coefficients.b1 * filter.x1 + coefficients.b2 * filter.x2
                                   - coefficients.a1 * filter.y1 - coefficients.a2 * filter.y2);
            filter.x2 = filter.x1; filter.x1 = input; filter.y2 = filter.y1; filter.y1 = wet;
            output.channels[channel][frame] = clean(input * (1.0f - mix) + wet * mix);
        }
    }
}
} // namespace drs::engine
