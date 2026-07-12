# core.peak_unscaled - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:257`
**Base class:** `peak_base<true>`, `data::display_buffer_base<true>`
**Classification:** analysis

## Signal Path

Same as core.peak but with different peak detection: preserves sign information. The audio signal passes through unmodified.

In the `Unscaled=true` path of `process()` (line 184): finds min and max separately across channels. Returns the one with larger absolute value, preserving the sign. So a signal peaking at -0.8 returns -0.8 (not 0.8).

`isNormalisedModulation()` returns `!Unscaled` = false, so the modulation output is unnormalized.

## Gap Answers

### raw-vs-scaled: How does it differ from core.peak?

Two differences:
1. **Sign preservation:** peak_unscaled returns the signed peak (the sample with the largest absolute value, keeping its sign). peak returns the absolute maximum.
2. **Normalisation:** peak_unscaled has `isNormalisedModulation() = false` (unnormalized output). peak has `isNormalisedModulation() = true`.

### unnormalised-output: Is the modulation truly unnormalized?

Yes. `isNormalisedModulation()` returns false (via `!Unscaled` where Unscaled=true). The output range matches the input signal range, which can be negative and can exceed [-1, 1] for hot signals.

### audio-passthrough: Does it modify audio?

No. Same as core.peak -- audio passes through unmodified.

### detection-algorithm: Same algorithm as peak?

Same `FloatVectorOperations::findMinAndMax()` but the result processing differs. Instead of taking `abs(max)` and `abs(min)` and returning the larger, peak_unscaled compares `abs(thisMin)` vs `abs(thisMax)` and returns the original (signed) value of whichever has larger absolute value.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
