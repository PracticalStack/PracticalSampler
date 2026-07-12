# math.pack - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:512-597` (template `complex_data_lut<0>`)
**Base class:** `data::base` (for external data), no `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Slider pack lookup table. Identical template to math.table but with `DataSize=0`, which changes the input scaling:
1. Input is multiplied by the pack size: `v * (float)tableData.size()`.
2. Result is clamped to `[0, packSize - 1]` via `jlimit<float>(0.0f, (float)(tableData.size()-1), ...)`.
3. Linear interpolation between adjacent slider pack entries produces the output.
4. The display value is sent to the UI from the first channel's first sample.

Uses `DataReadLock` for thread-safe access. If the slider pack data is empty, the signal passes through unmodified.

## Gap Answers

### input-scaling-with-pack-size: How does the input scaling work with variable pack size?

The input value is multiplied by the pack size (`v * (float)tableData.size()`), then clamped to `[0, packSize - 1]`. So an input of 0.0 maps to index 0, and an input of 1.0 maps to index `packSize` which is then clamped to `packSize - 1`. The expected input range is [0, 1], same as math.table.

### pack-interpolation: What interpolation is used?

Linear interpolation, identical to math.table. Both use the same `InterpolatorType = index::lerp<index::normalised<float, TableClampType>>`. However, for math.pack the `TableClampType` is `index::clamped<0>` (DataSize=0), which uses dynamic sizing from the actual data block rather than a compile-time constant.

### functional-difference-from-table: How does math.pack differ from math.table?

Both share the exact same `complex_data_lut` template class. The differences are:
- **Data source:** math.table uses a Table (512-sample editable curve); math.pack uses a SliderPack (variable-size array of slider values).
- **Input scaling:** math.table clamps input directly to [0, 1]; math.pack multiplies input by pack size then clamps to [0, packSize-1].
- **Resolution:** math.table is always 512 samples; math.pack size is determined by the connected SliderPack (user-configurable).
- **UI:** Table provides a curve editor; SliderPack provides individual slider bars.

### pack-size-range: What pack sizes are supported?

The pack size is determined by the connected SliderPack external data. The `tableData` block is set via `setExternalData()` and its size comes from the SliderPack. The minimum is effectively 1 (a single slider). Maximum is determined by the SliderPack implementation. Resizing happens via `setExternalData()` which is called from the message thread; the `DataReadLock` ensures thread safety during resize.

### out-of-range-input: What happens with out-of-range input?

Input is clamped. After multiplication by pack size, the result is clamped to `[0, packSize - 1]` using `jlimit`. Negative inputs clamp to index 0. Inputs above 1.0 clamp to the last slider value. No wrapping or undefined behaviour occurs.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []
