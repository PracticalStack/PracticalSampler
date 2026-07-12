# math.table - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:512-597` (template `complex_data_lut<SAMPLE_LOOKUP_TABLE_SIZE>`)
**Base class:** `data::base` (for external data), no `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Lookup table waveshaper. For each sample across all channels:
1. Input is clamped to [0, 1] via `jlimit(0.0f, 1.0f, v)`.
2. The clamped value is used as a normalised index into a 512-sample table.
3. Linear interpolation between adjacent table entries produces the output.
4. The first channel's first sample value is sent to the UI as display position.

Uses `DataReadLock` for thread-safe table access. If the table data is empty (no table connected), the signal passes through unmodified.

## Gap Answers

### input-to-index-mapping: How is the input signal value mapped to a table index?

Input is clamped to [0, 1] using `jlimit(0.0f, 1.0f, v)` (line 539). The clamped value is then used with `InterpolatorType`, which is `index::lerp<index::normalised<float, TableClampType>>`. The `index::normalised` wrapper maps the [0, 1] float to the table's [0, 511] index range. Values outside [0, 1] are clamped, not wrapped.

### table-interpolation: What interpolation method is used?

Linear interpolation via `index::lerp`. The `InterpolatorType` is defined as `index::lerp<index::normalised<float, TableClampType>>` (line 591). This provides linear interpolation between adjacent table entries.

### processing-granularity: Per-sample or per-block?

Per-sample. Both `process()` and `processFrame()` iterate every sample and call `processFloat(s)` which performs the table lookup. There is no SIMD acceleration for table lookups. The display value calculation happens once per block/frame using the first sample of the first channel.

### table-size-fixed: Is the table size always 512?

Yes. The template parameter `DataSize` is set to `SAMPLE_LOOKUP_TABLE_SIZE` which is 512. This is a compile-time constant. The `TableClampType` is `index::clamped<512>`, confirming the fixed size.

### external-data-usage: How does the node interact with the Table ComplexData?

The node inherits from `data::base`. In `setExternalData()`, it calls `base::setExternalData(d, 0)` and then `d.referBlockTo(tableData, 0)` to point the internal `block tableData` at the table's float data. On the audio thread, `DataReadLock l(this)` is acquired (non-blocking try-lock by default). If the lock fails or `tableData.isEmpty()`, processing is skipped (passthrough). Table edits during playback are safe -- the try-lock prevents blocking, and a missed lock simply means one block uses the previous table state.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []
