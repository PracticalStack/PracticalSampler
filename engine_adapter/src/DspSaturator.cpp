#include "drs/engine/DspSaturator.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr float denormalFloor = std::numeric_limits<float>::min();

double safeDbGain(const double db) noexcept
{
    return std::pow(10.0, std::clamp(std::isfinite(db) ? db : 0.0, -24.0, 36.0) / 20.0);
}

float sanitize(const float value) noexcept
{
    return std::isfinite(value) && std::abs(value) >= denormalFloor ? value : 0.0f;
}

float saturate(const float input, const DspSaturatorParameters& parameters) noexcept
{
    const auto driven = static_cast<float>(sanitize(input) * safeDbGain(parameters.driveDb));
    const auto character = static_cast<int>(std::lround(std::clamp(
        std::isfinite(parameters.character) ? parameters.character : 0.0, 0.0, 2.0)));
    if (character == 1)
        return std::clamp(driven, -1.0f, 1.0f);
    if (character == 2)
    {
        const auto clipped = std::clamp(driven, -1.5f, 1.5f);
        return sanitize(clipped - (clipped * clipped * clipped) / 3.0f);
    }
    const auto drive = static_cast<float>(safeDbGain(parameters.driveDb));
    return sanitize(std::tanh(driven) / std::tanh(std::max(drive, 1.0f)));
}

float processSample(const float dry, DspSaturatorState& state, const std::uint32_t channel,
                    const DspSaturatorParameters& parameters) noexcept
{
    const auto tone = static_cast<float>(std::clamp(std::isfinite(parameters.tone) ? parameters.tone : 0.5,
                                                     0.0, 1.0));
    // Tone is a one-pole post-drive low-pass: 0 is dark (about 250 Hz), 1 is open (about 12 kHz).
    const auto cutoff = 250.0 + 11750.0 * tone;
    const auto alpha = tone >= 1.0f ? 1.0f : static_cast<float>(1.0 - std::exp(
        -2.0 * 3.141592653589793 * cutoff / std::max(state.sampleRate, 1.0)));
    const auto distorted = saturate(dry, parameters);
    state.toneLowpass[channel] = sanitize(state.toneLowpass[channel] + alpha
        * (distorted - state.toneLowpass[channel]));
    const auto mix = static_cast<float>(std::clamp(std::isfinite(parameters.mix) ? parameters.mix : 1.0,
                                                    0.0, 1.0));
    return sanitize((sanitize(dry) * (1.0f - mix) + state.toneLowpass[channel] * mix)
                    * static_cast<float>(safeDbGain(parameters.outputDb)));
}

DspSaturatorParameters interpolate(const DspSaturatorParameters& start,
                                   const DspSaturatorParameters& end,
                                   const float fraction) noexcept
{
    const auto lerp = [fraction](const double left, const double right)
    { return left + (right - left) * fraction; };
    return { lerp(start.character, end.character), lerp(start.driveDb, end.driveDb),
             lerp(start.tone, end.tone), lerp(start.mix, end.mix), lerp(start.outputDb, end.outputDb) };
}

void process(SamplerAudioBufferView output, DspSaturatorState& state,
             const DspSaturatorParameters& start, const DspSaturatorParameters& end,
             const float wetStart, const float wetEnd) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0) return;
    const auto channelCount = std::min<std::uint32_t>(output.channelCount, 2);
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto fraction = static_cast<float>(frame + 1) / static_cast<float>(output.frameCount);
        const auto parameters = interpolate(start, end, fraction);
        const auto wet = wetStart + (wetEnd - wetStart) * fraction;
        for (std::uint32_t channel = 0; channel < channelCount; ++channel)
        {
            const auto dry = sanitize(output.channels[channel][frame]);
            const auto saturated = processSample(dry, state, channel, parameters);
            output.channels[channel][frame] = sanitize(dry + (saturated - dry) * wet);
        }
    }
}
} // namespace

void DspSaturatorState::prepare(const double newSampleRate) noexcept
{
    sampleRate = std::isfinite(newSampleRate) && newSampleRate > 0.0 ? newSampleRate : 48000.0;
    reset();
}

void DspSaturatorState::reset() noexcept { toneLowpass[0] = toneLowpass[1] = 0.0f; }

void processDspSaturator(const SamplerAudioBufferView output, DspSaturatorState& state,
                         const DspSaturatorParameters& parameters) noexcept
{
    process(output, state, parameters, parameters, 1.0f, 1.0f);
}

void processDspSaturatorRamp(const SamplerAudioBufferView output, DspSaturatorState& state,
                             const DspSaturatorParameters& start,
                             const DspSaturatorParameters& end) noexcept
{
    process(output, state, start, end, 1.0f, 1.0f);
}

void processDspSaturatorBypassRamp(const SamplerAudioBufferView output, DspSaturatorState& state,
                                   const DspSaturatorParameters& start,
                                   const DspSaturatorParameters& end,
                                   const float wetStart, const float wetEnd) noexcept
{
    process(output, state, start, end, wetStart, wetEnd);
}
} // namespace drs::engine
