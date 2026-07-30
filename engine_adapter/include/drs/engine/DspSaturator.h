#pragma once

#include "drs/engine/SamplerRenderModel.h"

#include <cstdint>

namespace drs::engine
{
// v1 is intentionally 1x only: no look-ahead, oversampling, latency, or tail.
// character: 0 = normalized tanh, 1 = hard clip, 2 = soft cubic.
struct DspSaturatorParameters
{
    double character = 0.0;
    double driveDb = 6.0;
    double tone = 0.5;
    double mix = 1.0;
    double outputDb = 0.0;
};

struct DspSaturatorState
{
    float toneLowpass[2] {};
    double sampleRate = 48000.0;

    void prepare(double newSampleRate) noexcept;
    void reset() noexcept;
};

void processDspSaturator(SamplerAudioBufferView output,
                         DspSaturatorState& state,
                         const DspSaturatorParameters& parameters) noexcept;
void processDspSaturatorRamp(SamplerAudioBufferView output,
                             DspSaturatorState& state,
                             const DspSaturatorParameters& start,
                             const DspSaturatorParameters& end) noexcept;
void processDspSaturatorBypassRamp(SamplerAudioBufferView output,
                                   DspSaturatorState& state,
                                   const DspSaturatorParameters& start,
                                   const DspSaturatorParameters& end,
                                   float wetStart,
                                   float wetEnd) noexcept;
} // namespace drs::engine
