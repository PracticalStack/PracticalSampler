#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <array>
#include <cstdint>
#include <vector>

namespace drs::engine
{
struct DspChorusParameters
{
    double rateHz = .8;
    double depthMs = 5.0;
    double baseDelayMs = 15.0;
    double width = 1.0;
    double mix = .35;
};

struct DspChorusVoice
{
    std::vector<float> left;
    std::vector<float> right;
    std::uint32_t writeIndex = 0;
};

struct DspChorusState
{
    static constexpr std::uint32_t voiceCount = 3;
    static constexpr std::uint32_t maximumSampleRate = 192000;
    static constexpr std::uint32_t maximumDelayFrames = maximumSampleRate * 42u / 1000u + 2u;
    static constexpr std::size_t maximumStateBytes = voiceCount * 2u * maximumDelayFrames * sizeof(float);
    std::array<DspChorusVoice, voiceCount> voices;
    double phase = 0.0;
    double sampleRate = 48000.0;

    bool prepare(double newSampleRate);
    void reset() noexcept;
};

void processDspChorusRamp(SamplerAudioBufferView output, DspChorusState& state,
                          const DspChorusParameters& start, const DspChorusParameters& end) noexcept;
} // namespace drs::engine
