#include "drs/engine/DspGain.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr double minimumGainDb = -96.0;
constexpr double maximumGainDb = 24.0;

double finiteClampedGainDb(const double value) noexcept
{
    if (!std::isfinite(value)) return 0.0;
    return value < minimumGainDb ? minimumGainDb : (value > maximumGainDb ? maximumGainDb : value);
}
} // namespace

float computeDspGainLinear(const DspGainParameters& parameters) noexcept
{
    if (parameters.mute >= 0.5) return 0.0f;
    const auto amplitude = static_cast<float>(std::pow(10.0, finiteClampedGainDb(parameters.gainDb) / 20.0));
    return parameters.polarity >= 0.5 ? -amplitude : amplitude;
}

bool dspGainIsIdentity(const DspGainParameters& parameters) noexcept
{
    return parameters.mute < 0.5 && parameters.polarity < 0.5
        && std::isfinite(parameters.gainDb) && parameters.gainDb == 0.0;
}

void processDspGain(const SamplerAudioBufferView output, const DspGainParameters& parameters) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0
        || dspGainIsIdentity(parameters)) return;
    const auto gain = computeDspGainLinear(parameters);
    constexpr auto denormalFloor = std::numeric_limits<float>::min();
    for (std::uint32_t channel = 0; channel < output.channelCount; ++channel)
    {
        auto* samples = output.channels[channel];
        if (samples == nullptr) return;
        for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
        {
            const auto value = samples[frame] * gain;
            samples[frame] = std::fabs(value) < denormalFloor ? 0.0f : value;
        }
    }
}

void processDspGainRamp(const SamplerAudioBufferView output,
                        const DspGainParameters& start,
                        const DspGainParameters& end) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0) return;
    if (start.gainDb == end.gainDb && start.polarity == end.polarity && start.mute == end.mute)
    {
        processDspGain(output, end);
        return;
    }
    constexpr auto denormalFloor = std::numeric_limits<float>::min();
    const auto denominator = static_cast<double>(output.frameCount);
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        // The generation has already advanced by this block's frame count. Sampling at
        // (frame + 1) makes adjacent blocks meet without repeating an endpoint.
        const auto fraction = static_cast<double>(frame + 1) / denominator;
        DspGainParameters parameters;
        parameters.gainDb = start.gainDb + (end.gainDb - start.gainDb) * fraction;
        // Polarity is declared non-smoothed in the catalog; changes take effect at the block edge.
        parameters.polarity = end.polarity;
        parameters.mute = start.mute + (end.mute - start.mute) * fraction;
        auto gain = computeDspGainLinear({ parameters.gainDb, parameters.polarity, 0.0 });
        gain *= static_cast<float>(1.0 - std::clamp(parameters.mute, 0.0, 1.0));
        for (std::uint32_t channel = 0; channel < output.channelCount; ++channel)
        {
            auto* samples = output.channels[channel];
            if (samples == nullptr) return;
            const auto value = samples[frame] * gain;
            samples[frame] = std::fabs(value) < denormalFloor ? 0.0f : value;
        }
    }
}

void processDspGainBypassRamp(const SamplerAudioBufferView output,
                              const DspGainParameters& start,
                              const DspGainParameters& end,
                              const float wetStart,
                              const float wetEnd) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0) return;
    const auto denominator = static_cast<double>(output.frameCount);
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto fraction = static_cast<double>(frame + 1) / denominator;
        const auto gainDb = start.gainDb + (end.gainDb - start.gainDb) * fraction;
        const auto mute = std::clamp(start.mute + (end.mute - start.mute) * fraction, 0.0, 1.0);
        const auto wet = wetStart + (wetEnd - wetStart) * static_cast<float>(fraction);
        auto gain = computeDspGainLinear({ gainDb, end.polarity, 0.0 });
        gain *= static_cast<float>(1.0 - mute);
        constexpr auto denormalFloor = std::numeric_limits<float>::min();
        for (std::uint32_t channel = 0; channel < output.channelCount; ++channel)
        {
            auto* samples = output.channels[channel];
            if (samples == nullptr) return;
            const auto dry = samples[frame];
            const auto value = dry + (dry * gain - dry) * wet;
            samples[frame] = std::fabs(value) < denormalFloor ? 0.0f : value;
        }
    }
}
} // namespace drs::engine
