#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <cstdint>
#include <vector>

namespace drs::engine
{
struct DspStereoDelayParameters
{
    double timeMs = 375.0;
    double sync = 0.0;
    double divisionBeats = 0.5;
    double feedback = 0.35;
    double pingPong = 0.0;
    double tone = 0.7;
    double width = 1.0;
    double mix = 0.25;
};

struct DspStereoDelayTransport
{
    double tempoBpm = 120.0;
    bool valid = false;
    bool hasTempo = false;
    bool isPlaying = false;
};

struct DspStereoDelayState
{
    static constexpr std::uint32_t maximumSampleRate = 96000;
    static constexpr std::uint32_t maximumDelayFrames = maximumSampleRate * 2 + 2;
    std::vector<float> left;
    std::vector<float> right;
    std::uint32_t writeIndex = 0;
    float feedbackLowpassLeft = 0.0f;
    float feedbackLowpassRight = 0.0f;
    float lastPeak = 0.0f;
    float inputPeak = 0.0f;
    double sampleRate = 48000.0;

    bool prepare(double newSampleRate);
    void reset() noexcept;
};

void processDspStereoDelay(SamplerAudioBufferView output, DspStereoDelayState& state,
                           const DspStereoDelayParameters& parameters,
                           const DspStereoDelayTransport& transport) noexcept;
void processDspStereoDelayRamp(SamplerAudioBufferView output, DspStereoDelayState& state,
                               const DspStereoDelayParameters& start,
                               const DspStereoDelayParameters& end,
                               const DspStereoDelayTransport& transport) noexcept;
} // namespace drs::engine
