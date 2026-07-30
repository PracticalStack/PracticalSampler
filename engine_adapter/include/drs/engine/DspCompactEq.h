#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <array>

namespace drs::engine
{
// v1 has exactly one stereo band. mode: 0=low-pass, 1=bell, 2=high-pass.
struct DspCompactEqParameters
{
    double mode = 1.0;
    double frequencyHz = 1000.0;
    double q = 0.707;
    double gainDb = 0.0;
    double mix = 1.0;
};

struct DspCompactEqBiquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
    void reset() noexcept { x1 = x2 = y1 = y2 = 0.0f; }
};

struct DspCompactEqState
{
    static constexpr std::size_t maximumStateBytes = sizeof(DspCompactEqBiquad) * 2u;
    std::array<DspCompactEqBiquad, 2> channels {};
    double sampleRate = 48000.0;

    bool prepare(double newSampleRate) noexcept;
    void reset() noexcept;
};

void processDspCompactEqRamp(SamplerAudioBufferView output, DspCompactEqState& state,
                             const DspCompactEqParameters& start,
                             const DspCompactEqParameters& end) noexcept;
} // namespace drs::engine
