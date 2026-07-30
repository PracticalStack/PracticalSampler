#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <array>
#include <cstdint>
#include <vector>

namespace drs::engine
{
struct DspAlgorithmicReverbParameters
{
    double preDelayMs = 20.0;
    double size = 0.5;
    double decaySeconds = 2.5;
    double damping = 0.5;
    double width = 1.0;
    double mix = 0.2;
};

struct DspAlgorithmicReverbLine
{
    std::vector<float> left;
    std::vector<float> right;
    std::uint32_t writeIndex = 0;
    float dampingLeft = 0.0f;
    float dampingRight = 0.0f;
};

struct DspAlgorithmicReverbState
{
    static constexpr std::uint32_t maximumSampleRate = 96000;
    static constexpr std::uint32_t maximumPreDelayFrames = maximumSampleRate / 4 + 2;
    static constexpr std::array<std::uint32_t, 4> maximumLineFrames {
        4130u, 5090u, 5858u, 6818u
    };
    static constexpr std::size_t maximumStateBytes = sizeof(float)
        * (2u * maximumPreDelayFrames + 2u * (maximumLineFrames[0] + maximumLineFrames[1]
           + maximumLineFrames[2] + maximumLineFrames[3]));

    std::vector<float> preDelayLeft;
    std::vector<float> preDelayRight;
    std::array<DspAlgorithmicReverbLine, 4> lines;
    std::uint32_t preDelayWriteIndex = 0;
    float inputPeak = 0.0f;
    double sampleRate = 48000.0;

    bool prepare(double newSampleRate);
    void reset() noexcept;
};

void processDspAlgorithmicReverb(SamplerAudioBufferView output, DspAlgorithmicReverbState& state,
                                 const DspAlgorithmicReverbParameters& parameters) noexcept;
void processDspAlgorithmicReverbRamp(SamplerAudioBufferView output, DspAlgorithmicReverbState& state,
                                     const DspAlgorithmicReverbParameters& start,
                                     const DspAlgorithmicReverbParameters& end) noexcept;
} // namespace drs::engine
