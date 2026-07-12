# core.peak - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:248`
**Base class:** `peak_base<false>`, `data::display_buffer_base<true>`
**Classification:** analysis

## Signal Path

Measures the maximum absolute signal magnitude across all channels per block and outputs it as a normalized modulation signal. The audio signal passes through completely unmodified.

In `process()` (line 180, Unscaled=false path): uses `FloatVectorOperations::findMinAndMax()` per channel, takes the maximum absolute value across all channels, stores in `max`. Writes to display buffer via `updateBuffer()`.

In `handleModulation()` (line 167): unconditionally returns `max` and true -- every block produces a modulation update.

## Gap Answers

### peak-detection-algorithm: How is peak computed?

Per-block maximum absolute value. `FloatVectorOperations::findMinAndMax()` finds the min and max of the channel buffer, then `Math.abs(range.getStart())` and `Math.abs(range.getEnd())` give absolute extremes. The `jmax` across channels picks the largest. No decay, no RMS -- purely instantaneous peak per block.

### normalisation-range: How is magnitude mapped to 0..1?

No explicit mapping or clamping. The `max` value is the raw absolute peak. For typical audio signals in [-1, 1], the output will naturally be in [0, 1]. The `isNormalisedModulation()` returns `!Unscaled` = true for peak (false for peak_unscaled). If the input exceeds 1.0, the modulation output will also exceed 1.0.

### audio-passthrough: Does it modify audio?

No. The audio signal is only read, never written. Both `process()` and `processFrame()` iterate the input but do not modify any samples.

### channel-handling: Multi-channel behaviour?

Max across all channels. The `for (auto& ch : data)` loop iterates every channel and takes `jmax` of all absolute values. Single modulation output regardless of channel count.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
