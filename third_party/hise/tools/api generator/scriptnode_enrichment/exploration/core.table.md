# core.table - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:44`
**Base class:** `scriptnode::data::base`
**Classification:** audio_processor (also modulation source)

## Signal Path

Despite the description "a (symmetrical) lookup table based waveshaper", the table node does NOT modify the audio signal. It reads the absolute value of the input signal, looks up the table value, and outputs the result as a modulation signal only.

In `process()` (line 82): iterates all channels and samples but the loop body uses a local variable `v` with `ignoreUnused(s)` -- the audio sample `s` is never read or written. The variable `v` is processed through `processFloat(v)` and sent to `externalData.setDisplayedValue(v)`.

In `processFrame()` (line 115): `v = hmath::abs(s)` reads the sample, then `processFloat(v)` applies the table lookup to `v`, but `v` is a local variable -- `s` is not modified. The audio passes through unchanged.

The `processFloat()` method (line 103): creates a normalised interpolating index from the float value, then multiplies by the table lookup: `s *= tableData[ip]`. But since this operates on the local `v`, the result is only used for display.

The modulation output uses `ModValue currentValue` but `currentValue` is only updated in `reset()` -- it is never updated during processing. The `handleModulation()` returns `currentValue.getChangedValue(value)` which will only fire once after reset.

## Gap Answers

### symmetrical-meaning: What does "symmetrical" mean?

The code takes `hmath::abs(s)` before table lookup in `processFrame()`, making the lookup symmetric around zero -- positive and negative input values map to the same table position. However, since the audio is not actually modified, this symmetry only affects the modulation/display output.

### index-mapping: How is input mapped to table index?

Uses `index::lerp<index::normalised<float, TableClampType>>` where `TableClampType = index::clamped<SAMPLE_LOOKUP_TABLE_SIZE, false>`. The normalised index maps 0..1 input to 0..512 table entries with linear interpolation. Since abs(s) is used, the input range is effectively 0..1 (absolute audio signal).

### modulation-output: Does it output modulation?

The node declares `handleModulation()` and `ModValue currentValue`, and `isNormalisedModulation()` returns true. However, `currentValue` is only set in `reset()` to 0.0 -- it is never updated during processing. The `smoothedValue` member is prepared but never used in processing. This means the modulation output is essentially non-functional in the current code.

### table-size: Table resolution?

Yes, always `SAMPLE_LOOKUP_TABLE_SIZE` (512) entries. Linear interpolation via `index::lerp`.

## Notes

The table node appears to have a broken implementation. The description says "waveshaper" but the audio signal passes through unmodified. The modulation output is declared but not updated during processing. The `processFloat()` method modifies a local variable. The `smoothedValue` member is prepared but unused. This looks like either an incomplete refactor or intentional change to make it analysis-only, but the description has not been updated.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []
