# control.cable_pack - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:44` (cable_pack class)
**Base class:** `data::base`, `parameter_node_base<ParameterClass>`, `no_processing`
**Classification:** control_source

## Signal Path

The cable_pack node uses a slider pack to reshape a normalised control signal. The input value (0..1) is used as a normalised index into the slider pack data. No interpolation is used -- the lookup returns the nearest discrete slider value.

Value parameter (0..1) -> normalised index into slider pack -> nearest-neighbour lookup -> slider value -> output (normalised)

## Gap Answers

### pack-lookup-interpolation: Does cable_pack use interpolation?

No interpolation. Line 87 defines `using IndexType = index::normalised<double, index::clamped<0>>` -- this is a normalised float index with dynamic clamped bounds but NO `index::lerp` wrapper. The lookup `v = b[index]` (line 78) returns the value at the truncated integer position. This is nearest-neighbour (step) lookup, producing discrete jumps between slider values.

### pack-size-behaviour: How does slider pack size affect the lookup?

The normalised input is scaled to the pack size by the `index::normalised` wrapper. With 8 sliders and Value=0.5: the normalised index becomes `0.5 * 8 = 4.0`, clamped to [0, 7], truncated to integer 4 -- so slider index 4 is read. With Value=0.0, index 0 is read. With Value=1.0, the index is `1.0 * 8 = 8.0`, clamped to 7, so the last slider is read. The clamped<0> uses dynamic bounds (0 = determined at runtime from `b.size()`).

### pack-display-value: Does cable_pack call setDisplayedValue()?

Yes. Line 83: `externalData.setDisplayedValue((double)index.getIndex(b.size()))`. The raw integer index (after normalisation and clamping) is sent to the slider pack UI to highlight the current lookup position.

### pack-value-range: What is the output range?

The output is whatever value is stored in the slider pack at the looked-up index. Slider pack values are typically 0..1 (the default slider pack range), but slider packs can be configured with arbitrary ranges in the UI. The node does NOT clamp the output -- it sends the raw slider value. Since the node does not inherit from `no_mod_normalisation`, the output is treated as normalised (0..1), and target parameter ranges will be applied.

## Parameters

- **Value** (single parameter via SN_ADD_SET_VALUE): Normalised input 0..1, default 0.0. This is the slider pack lookup position. NOT marked as unscaled.

## Complex Data

- **SliderPack** (1 slider pack): Variable-size float array. Connected via `setExternalData()` which stores a block reference to channel 0 (line 61). When data is connected and has samples, the current value is re-evaluated (line 63: `setValue(lastValue)`).

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The node acquires a `DataReadLock` (line 72) before reading the slider pack data, ensuring thread-safe access. If the lock fails (try-lock on audio thread), the block is accessed without the lock -- but the `b.size() > 0` check (line 74) still guards against null data. The `lastValue` member (line 89) stores the most recent input for refreshing when the slider pack data changes.
