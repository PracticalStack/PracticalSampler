#pragma once

#include "drs/engine/SamplerRenderModel.h"

namespace drs::engine
{
// Version-1 gain is intentionally stateless: gainDb is clamped to the catalog
// range, polarity and mute are boolean thresholds, and subnormal output is zeroed.
struct DspGainParameters
{
    double gainDb = 0.0;
    double polarity = 0.0;
    double mute = 0.0;
};

float computeDspGainLinear(const DspGainParameters& parameters) noexcept;
bool dspGainIsIdentity(const DspGainParameters& parameters) noexcept;
void processDspGain(SamplerAudioBufferView output, const DspGainParameters& parameters) noexcept;
void processDspGainRamp(SamplerAudioBufferView output,
                        const DspGainParameters& start,
                        const DspGainParameters& end) noexcept;
void processDspGainBypassRamp(SamplerAudioBufferView output,
                              const DspGainParameters& start,
                              const DspGainParameters& end,
                              float wetStart,
                              float wetEnd) noexcept;
} // namespace drs::engine
