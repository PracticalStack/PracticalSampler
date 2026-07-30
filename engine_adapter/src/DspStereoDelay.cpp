#include "drs/engine/DspStereoDelay.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
float clean(float value) noexcept
{
    return std::isfinite(value) && std::abs(value) >= std::numeric_limits<float>::min() ? value : 0.0f;
}

float readLinear(const std::vector<float>& line, const double position) noexcept
{
    const auto size = static_cast<std::int64_t>(line.size());
    auto base = static_cast<std::int64_t>(std::floor(position));
    while (base < 0) base += size;
    base %= size;
    const auto next = (base + 1) % size;
    const auto fraction = static_cast<float>(position - std::floor(position));
    return clean(line[static_cast<std::size_t>(base)]
                 + (line[static_cast<std::size_t>(next)] - line[static_cast<std::size_t>(base)]) * fraction);
}
}

bool DspStereoDelayState::prepare(const double newSampleRate)
{
    sampleRate = std::clamp(std::isfinite(newSampleRate) ? newSampleRate : 48000.0, 1.0,
                            static_cast<double>(maximumSampleRate));
    if (left.empty())
    {
        left.assign(maximumDelayFrames, 0.0f);
        right.assign(maximumDelayFrames, 0.0f);
    }
    reset();
    return true;
}

void DspStereoDelayState::reset() noexcept
{
    std::fill(left.begin(), left.end(), 0.0f);
    std::fill(right.begin(), right.end(), 0.0f);
    writeIndex = 0; feedbackLowpassLeft = feedbackLowpassRight = lastPeak = inputPeak = 0.0f;
}

void processDspStereoDelay(const SamplerAudioBufferView output, DspStereoDelayState& state,
                           const DspStereoDelayParameters& p,
                           const DspStereoDelayTransport& transport) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0
        || state.left.empty() || state.right.empty()) return;
    const auto tempo = transport.valid && std::isfinite(transport.tempoBpm)
        ? std::clamp(transport.tempoBpm, 20.0, 300.0) : 120.0;
    const auto delayFrames = std::clamp((p.sync >= 0.5 ? 60.0 / tempo * std::clamp(p.divisionBeats, .0625, 4.0)
                                                       : std::clamp(p.timeMs, 1.0, 2000.0) / 1000.0)
                                         * state.sampleRate, 1.0,
                                         static_cast<double>(state.left.size() - 2));
    const auto feedback = static_cast<float>(std::clamp(p.feedback, 0.0, .95));
    const auto mix = static_cast<float>(std::clamp(p.mix, 0.0, 1.0));
    const auto tone = static_cast<float>(std::clamp(p.tone, 0.0, 1.0));
    const auto width = static_cast<float>(std::clamp(p.width, 0.0, 1.0));
    const auto alpha = tone >= 1.0f ? 1.0f : .02f + .96f * tone;
    state.inputPeak = 0.0f;
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto readAt = static_cast<double>(state.writeIndex) - delayFrames;
        const auto delayedL = readLinear(state.left, readAt), delayedR = readLinear(state.right, readAt);
        const auto inL = clean(output.channels[0][frame]);
        const auto inR = output.channelCount > 1 ? clean(output.channels[1][frame]) : inL;
        state.inputPeak = std::max(state.inputPeak, std::max(std::abs(inL), std::abs(inR)));
        state.lastPeak = std::max(state.lastPeak * 0.999f,
                                  std::max(std::abs(inL), std::max(std::abs(delayedL), std::abs(delayedR))));
        state.feedbackLowpassLeft = clean(state.feedbackLowpassLeft + alpha * (delayedL - state.feedbackLowpassLeft));
        state.feedbackLowpassRight = clean(state.feedbackLowpassRight + alpha * (delayedR - state.feedbackLowpassRight));
        const auto pingPong = p.pingPong >= .5;
        state.left[state.writeIndex] = clean(inL + feedback * (pingPong ? state.feedbackLowpassRight : state.feedbackLowpassLeft));
        state.right[state.writeIndex] = clean(inR + feedback * (pingPong ? state.feedbackLowpassLeft : state.feedbackLowpassRight));
        const auto mid = (delayedL + delayedR) * 0.5f;
        const auto side = (delayedL - delayedR) * 0.5f * width;
        output.channels[0][frame] = clean(inL * (1.0f - mix) + (mid + side) * mix);
        if (output.channelCount > 1) output.channels[1][frame] = clean(inR * (1.0f - mix) + (mid - side) * mix);
        state.writeIndex = (state.writeIndex + 1) % static_cast<std::uint32_t>(state.left.size());
    }
}

void processDspStereoDelayRamp(const SamplerAudioBufferView output, DspStereoDelayState& state,
                               const DspStereoDelayParameters&, const DspStereoDelayParameters& end,
                               const DspStereoDelayTransport& transport) noexcept
{
    processDspStereoDelay(output, state, end, transport);
}
} // namespace drs::engine
