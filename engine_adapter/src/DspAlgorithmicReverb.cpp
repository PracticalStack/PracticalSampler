#include "drs/engine/DspAlgorithmicReverb.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace drs::engine
{
namespace
{
constexpr std::array<double, 4> lineSeconds { .043, .053, .061, .071 };

float clean(const float value) noexcept
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

DspAlgorithmicReverbParameters interpolate(const DspAlgorithmicReverbParameters& start,
                                            const DspAlgorithmicReverbParameters& end,
                                            const float fraction) noexcept
{
    const auto lerp = [fraction](const double left, const double right)
    { return left + (right - left) * fraction; };
    return { lerp(start.preDelayMs, end.preDelayMs), lerp(start.size, end.size),
             lerp(start.decaySeconds, end.decaySeconds), lerp(start.damping, end.damping),
             lerp(start.width, end.width), lerp(start.mix, end.mix) };
}

struct ReverbFrameControls
{
    double preDelayFrames = 0.0;
    float dampingAlpha = 1.0f;
    float mix = 0.0f;
    float width = 1.0f;
    std::array<double, 4> lineFrames {};
    std::array<float, 4> feedback {};
};

ReverbFrameControls makeControls(const DspAlgorithmicReverbParameters& parameters,
                                 const DspAlgorithmicReverbState& state) noexcept
{
    ReverbFrameControls controls;
    controls.preDelayFrames = std::clamp(std::clamp(parameters.preDelayMs, 0.0, 250.0) * state.sampleRate / 1000.0,
                                         0.0, static_cast<double>(state.preDelayLeft.size() - 2));
    const auto size = std::clamp(parameters.size, 0.0, 1.0);
    const auto decay = std::clamp(parameters.decaySeconds, .1, 20.0);
    controls.dampingAlpha = 1.0f - .96f * static_cast<float>(std::clamp(parameters.damping, 0.0, 1.0));
    controls.mix = static_cast<float>(std::clamp(parameters.mix, 0.0, 1.0));
    controls.width = static_cast<float>(std::clamp(parameters.width, 0.0, 1.0));
    for (std::size_t line = 0; line < state.lines.size(); ++line)
    {
        controls.lineFrames[line] = std::clamp(lineSeconds[line] * (.35 + .65 * size) * state.sampleRate, 1.0,
                                               static_cast<double>(state.lines[line].left.size() - 2));
        controls.feedback[line] = static_cast<float>(std::clamp(
            std::pow(.001, controls.lineFrames[line] / state.sampleRate / decay), .01, .999));
    }
    return controls;
}

void process(SamplerAudioBufferView output, DspAlgorithmicReverbState& state,
             const DspAlgorithmicReverbParameters& start,
             const DspAlgorithmicReverbParameters& end) noexcept
{
    if (output.channels == nullptr || output.channelCount == 0 || output.frameCount == 0
        || state.preDelayLeft.empty() || state.preDelayRight.empty()) return;
    const auto parametersAreStatic = start.preDelayMs == end.preDelayMs && start.size == end.size
        && start.decaySeconds == end.decaySeconds && start.damping == end.damping && start.width == end.width
        && start.mix == end.mix;
    const auto staticControls = makeControls(start, state);
    state.inputPeak = 0.0f;
    for (std::uint32_t frame = 0; frame < output.frameCount; ++frame)
    {
        const auto controls = parametersAreStatic
            ? staticControls
            : makeControls(interpolate(start, end,
                                       static_cast<float>(frame + 1) / static_cast<float>(output.frameCount)), state);
        const auto inLeft = clean(output.channels[0][frame]);
        const auto inRight = output.channelCount > 1 ? clean(output.channels[1][frame]) : inLeft;
        state.inputPeak = std::max(state.inputPeak, std::max(std::abs(inLeft), std::abs(inRight)));
        const auto preLeft = controls.preDelayFrames < 1.0 ? inLeft
            : readLinear(state.preDelayLeft, static_cast<double>(state.preDelayWriteIndex) - controls.preDelayFrames);
        const auto preRight = controls.preDelayFrames < 1.0 ? inRight
            : readLinear(state.preDelayRight, static_cast<double>(state.preDelayWriteIndex) - controls.preDelayFrames);
        state.preDelayLeft[state.preDelayWriteIndex] = inLeft;
        state.preDelayRight[state.preDelayWriteIndex] = inRight;
        state.preDelayWriteIndex = (state.preDelayWriteIndex + 1) % static_cast<std::uint32_t>(state.preDelayLeft.size());

        std::array<float, 4> readLeft {}, readRight {}, feedbackLeft {}, feedbackRight {};
        for (std::size_t line = 0; line < state.lines.size(); ++line)
        {
            auto& delayLine = state.lines[line];
            readLeft[line] = readLinear(delayLine.left, static_cast<double>(delayLine.writeIndex) - controls.lineFrames[line]);
            readRight[line] = readLinear(delayLine.right, static_cast<double>(delayLine.writeIndex) - controls.lineFrames[line]);
            delayLine.dampingLeft = clean(delayLine.dampingLeft + controls.dampingAlpha * (readLeft[line] - delayLine.dampingLeft));
            delayLine.dampingRight = clean(delayLine.dampingRight + controls.dampingAlpha * (readRight[line] - delayLine.dampingRight));
            feedbackLeft[line] = delayLine.dampingLeft * controls.feedback[line];
            feedbackRight[line] = delayLine.dampingRight * controls.feedback[line];
        }
        const auto mixHadamard = [](const std::array<float, 4>& values)
        {
            return std::array<float, 4> { values[0] + values[1] + values[2] + values[3],
                                          values[0] - values[1] + values[2] - values[3],
                                          values[0] + values[1] - values[2] - values[3],
                                          values[0] - values[1] - values[2] + values[3] };
        };
        const auto mixedFeedbackLeft = mixHadamard(feedbackLeft);
        const auto mixedFeedbackRight = mixHadamard(feedbackRight);
        for (std::size_t line = 0; line < state.lines.size(); ++line)
        {
            auto& delayLine = state.lines[line];
            const auto injectedLeft = (preLeft * .75f + preRight * .25f) * .25f;
            const auto injectedRight = (preRight * .75f + preLeft * .25f) * .25f;
            delayLine.left[delayLine.writeIndex] = clean(injectedLeft + mixedFeedbackLeft[line] * .5f);
            delayLine.right[delayLine.writeIndex] = clean(injectedRight + mixedFeedbackRight[line] * .5f);
            delayLine.writeIndex = (delayLine.writeIndex + 1) % static_cast<std::uint32_t>(delayLine.left.size());
        }
        const auto wetLeft = clean((readLeft[0] + readLeft[1] + readLeft[2] + readLeft[3]) * .25f);
        const auto wetRight = clean((readRight[0] + readRight[1] + readRight[2] + readRight[3]) * .25f);
        const auto wetMid = (wetLeft + wetRight) * .5f;
        const auto wetSide = (wetLeft - wetRight) * .5f * controls.width;
        output.channels[0][frame] = clean(inLeft * (1.0f - controls.mix) + (wetMid + wetSide) * controls.mix);
        if (output.channelCount > 1) output.channels[1][frame] = clean(inRight * (1.0f - controls.mix) + (wetMid - wetSide) * controls.mix);
    }
}
} // namespace

bool DspAlgorithmicReverbState::prepare(const double newSampleRate)
{
    sampleRate = std::clamp(std::isfinite(newSampleRate) ? newSampleRate : 48000.0, 1.0,
                            static_cast<double>(maximumSampleRate));
    if (preDelayLeft.empty())
    {
        preDelayLeft.assign(maximumPreDelayFrames, 0.0f);
        preDelayRight.assign(maximumPreDelayFrames, 0.0f);
        for (std::size_t line = 0; line < lines.size(); ++line)
        {
            lines[line].left.assign(maximumLineFrames[line], 0.0f);
            lines[line].right.assign(maximumLineFrames[line], 0.0f);
        }
    }
    reset();
    return true;
}

void DspAlgorithmicReverbState::reset() noexcept
{
    std::fill(preDelayLeft.begin(), preDelayLeft.end(), 0.0f);
    std::fill(preDelayRight.begin(), preDelayRight.end(), 0.0f);
    preDelayWriteIndex = 0;
    inputPeak = 0.0f;
    for (auto& line : lines)
    {
        std::fill(line.left.begin(), line.left.end(), 0.0f);
        std::fill(line.right.begin(), line.right.end(), 0.0f);
        line.writeIndex = 0;
        line.dampingLeft = line.dampingRight = 0.0f;
    }
}

void processDspAlgorithmicReverb(const SamplerAudioBufferView output, DspAlgorithmicReverbState& state,
                                 const DspAlgorithmicReverbParameters& parameters) noexcept
{
    process(output, state, parameters, parameters);
}

void processDspAlgorithmicReverbRamp(const SamplerAudioBufferView output, DspAlgorithmicReverbState& state,
                                     const DspAlgorithmicReverbParameters& start,
                                     const DspAlgorithmicReverbParameters& end) noexcept
{
    process(output, state, start, end);
}
} // namespace drs::engine
