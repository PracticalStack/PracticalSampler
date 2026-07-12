# GlobalTimeVariantModulator - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/modulators/mods/GlobalModulators.h:139-186` (class definition)
- `hi_core/hi_modules/modulators/mods/GlobalModulators.cpp:392-560` (implementation)
- `hi_core/hi_modules/modulators/editors/GlobalModulatorEditor.h` (shared editor)

**Base class:** `TimeVariantModulator` + `GlobalModulator`

## Signal Path

GlobalTimeVariantModulator continuously copies a source TimeVariantModulator's buffer from a GlobalModulatorContainer. It provides monophonic, per-block modulation.

1. User selects source via ComboBox dropdown (format: "ContainerId:ModulatorId")
2. On `calculateBlock()`: copies the source buffer from the container's pre-computed `TimeVariantData` buffer
3. Optional per-sample table lookup transforms each value
4. Inversion applied via `invertBuffer()` (non-table path only - BUG in table path)
5. Buffer becomes the modulation output

## Gap Answers

### signal-flow

**Question:** What is the complete signal flow in calculateBlock? How does this module retrieve the continuous modulation buffer from the source TimeVariantModulator in the GlobalModulatorContainer?

**Answer:** In `calculateBlock()`, the consumer calls `getConnectedContainer()->getModulationValuesForModulator(connectedMod, startSample)`. The container looks up the `TimeVariantData` entry matching the source modulator and returns a pointer to its pre-computed buffer (rendered during `preVoiceRendering()`). The consumer then either copies the buffer via `FloatVectorOperations::copy()` (non-table path) or performs per-sample table lookup. Finally, inversion is applied if enabled.

### connection-mechanism

**Question:** How does the user select which global time variant modulator to connect to?

**Answer:** Same mechanism as all GlobalModulator consumers. A `ComboBox` dropdown in `GlobalModulatorEditor` lists all available TimeVariantModulators from all GlobalModulatorContainers. The connection string format is `"ContainerId:ModulatorId"`, stored as the "Connection" property. Deferred connection via `pendingConnection` handles load-order issues.

### table-application

**Question:** Where is the table lookup applied? Is it per-sample on the buffer or per-block? Before or after inversion?

**Answer:** The table is applied per-sample in the `calculateBlock()` while loop when `useTable` is true. For each sample, the source value is passed through `getTableUnchecked(0)->getInterpolatedValue()`. The table path and the inversion path are separate branches. **BUG:** In the table path, the while loop decrements `numSamples` to -1 (using `--numSamples` as the loop condition), so when `invertBuffer()` is called after the loop, `numSamples` is <= 0 and the guard condition prevents execution. Inversion never works when the table is active.

### buffer-sharing

**Question:** Does this module copy the source buffer or reference it directly?

**Answer:** The consumer copies the buffer. In the non-table path, `FloatVectorOperations::copy()` copies from the container's shared buffer into the consumer's `internalBuffer`. In the table path, values are read per-sample from the source and written to the local buffer. Multiple consumers reading the same source is safe because each copies independently.

### disconnected-behavior

**Question:** What happens when no global time variant modulator is connected?

**Answer:** The buffer is filled with 1.0 via `FloatVectorOperations::fill(internalBuffer, 1.0f, numSamples)`. This provides unity/pass-through in gain mode.

## Processing Chain Detail

1. **calculateBlock** (low-medium) - buffer copy from pre-computed source data, optional per-sample table lookup

## Modulation Points

No modulation chains of its own. Receives a per-block buffer from the source TimeVariantModulator in the GlobalModulatorContainer.

## Vestigial / Notable

- **Inverted parameter (table path)**: BUG - inversion is never applied when UseTable is enabled. The while loop decrements `numSamples` to -1, so `invertBuffer()` is called with an invalid count. The non-table path works correctly because `numSamples` is not decremented before `invertBuffer()`.

## CPU Assessment

**Low baseline cost.** Per-block buffer copy via SIMD-optimized `FloatVectorOperations::copy()`. Optional per-sample table lookup adds moderate cost proportional to block size. No DSP computation of its own.

## UI Components

- `GlobalModulatorEditor` with ComboBox for source selection, UseTable toggle, TableEditor, and Invert toggle
- No dedicated FloatingTile

## Notes

The inversion bug in the table path is a real defect: the while loop uses `--numSamples` as both the loop condition and the counter, consuming it completely. After the loop, `numSamples` is 0 or -1, so `invertBuffer(internalBuffer, numSamples)` does nothing because of its `numSamples > 0` guard. A local copy of numSamples or restructuring the loop would fix this.
