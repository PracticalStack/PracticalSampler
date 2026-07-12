# core.gain - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2086`
**Base class:** `HiseDspBase`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Multiplies all channels by a smoothed gain factor. Two processing paths:
1. **Smoothing active** (`gainer.get().isActive()`): frame-based processing via `FrameConverters::forwardToFrame16()`, applying `gainer.get().advance()` per sample.
2. **Smoothing settled**: block-based multiplication via `data.toChannelData(ch) *= gainFactor` for each channel.

The gain parameter is in decibels, converted to linear via `Decibels::decibelsToGain()`.

## Gap Answers

### processing-model: Block or frame-based?

Both, adaptively. When the smoothed value is still ramping (`isActive()`), it uses per-sample frame processing. When settled, it uses efficient per-channel block multiplication. This is an optimization -- block multiplication with SIMD is faster than per-sample.

### db-to-linear-conversion: Is -100 dB treated as silence?

`Decibels::decibelsToGain(-100.0)` returns approximately 1e-5 (0.00001), not exactly 0. However, this is effectively inaudible. The range is -100 to 0 dB, so the node can only attenuate.

### reset-value-usage: When is ResetValue applied?

In `reset()` (line 2147): the smoother is first set to `resetValue` and then immediately reset (jump to target), followed by setting the target to the current `gainValue`. This means on voice start, the gain instantly jumps to ResetValue and then smoothly ramps to the current Gain setting. Useful for fade-in effects (e.g., ResetValue=-100dB, Gain=0dB creates a fade-in on each voice start).

### smoothing-mechanism: What smoothing algorithm?

Uses `sfloat` (smoothed float), which is an exponential ramp. Default smoothing time is 20ms. The `sfloat::prepare(sampleRate, timeMs)` sets up the ramp rate, `set(target)` starts ramping, `advance()` returns current value and steps forward, `reset()` jumps to target instantly.

## Parameters

- **Gain** (-100 to 0 dB, default 0): Target gain in decibels. Converted to linear internally.
- **Smoothing** (0-1000 ms, default 20): Smoothing time for gain changes. Skew centered at 100ms.
- **ResetValue** (-100 to 0 dB, default 0): Gain value applied instantly on voice start before smoothing to target.

## Polyphonic Behaviour

`PolyData<sfloat, NumVoices> gainer` provides per-voice smoothed gain values. Each voice independently smooths to the target gain.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: [{"parameter": "Smoothing", "impact": "low", "note": "When smoothing is active, per-sample processing; when settled, efficient block multiply"}]
