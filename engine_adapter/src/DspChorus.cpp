#include "drs/engine/DspChorus.h"

#include <algorithm>
#include <cmath>

namespace drs::engine
{
namespace
{
constexpr double pi = 3.14159265358979323846;

float clean(const float value) noexcept { return std::isfinite(value) ? std::clamp(value, -16.0f, 16.0f) : 0.0f; }

float readLinear(const std::vector<float>& line, double position) noexcept
{
    const auto length = static_cast<std::int64_t>(line.size());
    auto base = static_cast<std::int64_t>(std::floor(position));
    while (base < 0) base += length;
    base %= length;
    const auto next = (base + 1) % length;
    const auto fraction = static_cast<float>(position - std::floor(position));
    return clean(line[static_cast<std::size_t>(base)]
                 + (line[static_cast<std::size_t>(next)] - line[static_cast<std::size_t>(base)]) * fraction);
}

double interpolate(const double start, const double end, const float amount) noexcept
{
    return start + (end - start) * amount;
}
} // namespace

bool DspChorusState::prepare(const double newSampleRate)
{
    sampleRate = std::clamp(std::isfinite(newSampleRate) ? newSampleRate : 48000.0, 8000.0,
                            static_cast<double>(maximumSampleRate));
    if (voices.front().left.empty())
    {
        for (auto& voice : voices)
        {
            voice.left.assign(maximumDelayFrames, 0.0f);
            voice.right.assign(maximumDelayFrames, 0.0f);
        }
    }
    reset();
    return true;
}

void DspChorusState::reset() noexcept
{
    phase = 0.0;
    for (auto& voice : voices)
    {
        std::fill(voice.left.begin(), voice.left.end(), 0.0f);
        std::fill(voice.right.begin(), voice.right.end(), 0.0f);
        voice.writeIndex = 0;
    }
}

void processDspChorusRamp(const SamplerAudioBufferView output, DspChorusState& state,
                          const DspChorusParameters& start, const DspChorusParameters& end) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0 || state.voices.front().left.empty()) return;
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto amount = static_cast<float>(frame + 1) / static_cast<float>(output.frameCount);
        const auto rate = std::clamp(interpolate(start.rateHz, end.rateHz, amount), .05, 5.0);
        const auto depthFrames = std::clamp(interpolate(start.depthMs, end.depthMs, amount), .1, 12.0) * state.sampleRate / 1000.0;
        const auto baseFrames = std::clamp(interpolate(start.baseDelayMs, end.baseDelayMs, amount), 5.0, 30.0) * state.sampleRate / 1000.0;
        const auto width = static_cast<float>(std::clamp(interpolate(start.width, end.width, amount), 0.0, 1.0));
        const auto mix = static_cast<float>(std::clamp(interpolate(start.mix, end.mix, amount), 0.0, 1.0));
        const auto inputLeft = clean(output.channels[0][frame]);
        const auto inputRight = output.channelCount > 1 ? clean(output.channels[1][frame]) : inputLeft;
        float wetLeft = 0.0f, wetRight = 0.0f;
        for (std::size_t index = 0; index < state.voices.size(); ++index)
        {
            auto& voice = state.voices[index];
            const auto voicePhase = state.phase + static_cast<double>(index) / state.voices.size();
            const auto lfoLeft = std::sin(2.0 * pi * voicePhase);
            const auto lfoRight = std::sin(2.0 * pi * (voicePhase + .25 * width));
            const auto delayLeft = std::clamp(baseFrames + depthFrames * (.5 + .5 * lfoLeft), 1.0,
                                              static_cast<double>(voice.left.size() - 2));
            const auto delayRight = std::clamp(baseFrames + depthFrames * (.5 + .5 * lfoRight), 1.0,
                                               static_cast<double>(voice.right.size() - 2));
            wetLeft += readLinear(voice.left, static_cast<double>(voice.writeIndex) - delayLeft);
            wetRight += readLinear(voice.right, static_cast<double>(voice.writeIndex) - delayRight);
            voice.left[voice.writeIndex] = inputLeft;
            voice.right[voice.writeIndex] = inputRight;
            voice.writeIndex = (voice.writeIndex + 1) % static_cast<std::uint32_t>(voice.left.size());
        }
        wetLeft /= static_cast<float>(state.voices.size()); wetRight /= static_cast<float>(state.voices.size());
        output.channels[0][frame] = clean(inputLeft * (1.0f - mix) + wetLeft * mix);
        if (output.channelCount > 1) output.channels[1][frame] = clean(inputRight * (1.0f - mix) + wetRight * mix);
        state.phase += rate / state.sampleRate;
        if (state.phase >= 1.0) state.phase -= std::floor(state.phase);
    }
}
} // namespace drs::engine
