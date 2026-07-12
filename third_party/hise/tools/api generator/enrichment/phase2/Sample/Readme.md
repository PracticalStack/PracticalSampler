# Sample -- Project Context

## Project Context

### Real-World Use Cases
- **Velocity zone mapping tool**: A sampler tool that loads all samples, analyzes their peak loudness via `loadIntoBufferArray()`, attaches the measured peaks as custom metadata via `getCustomProperties()`, sorts by loudness, then assigns velocity ranges with `set()`. This is the most common Sample workflow in production plugins - building or refining velocity layers programmatically rather than manually in the sample map editor.
- **Sample map cleaning/reorganization**: A percussion instrument that needs to delete unwanted samples from a loaded map and remap the remaining ones to new key positions. Uses `deleteSample()` to remove a sample, then `set()` on neighboring samples to fill the gap. This pattern appears in tools that let users curate sample content at runtime.
- **Audio analysis pipeline**: A plugin that detects the pitch of imported samples by loading audio data with `loadIntoBufferArray()`, running `Buffer.detectPitch()`, and writing the detected root note back with `set(Sample.Root, ...)`. This pattern supports user-import workflows where sample metadata is incomplete.
- **Loop point configuration**: A custom sample import tool that reads dynamic property bounds with `getRange()` to set loop start, loop end, and crossfade values within valid ranges. The dynamic range system (where LoopEnd's valid range depends on SampleEnd) makes `getRange()` essential for loop editing UIs.

### Complexity Tiers
1. **Property read/write** (most common): `get()`, `set()`, `setFromJSON()` for reading and modifying sample properties like velocity ranges, root notes, and key mappings. Sufficient for basic sample map editing tools and batch property changes.
2. **Dynamic bounds and metadata**: `getRange()` for querying valid property ranges before setting loop points or velocity zones. `getCustomProperties()` for attaching transient analysis data (peaks, categories) that persists during the session but is not saved to the sample map.
3. **Audio manipulation**: `loadIntoBufferArray()` for loading sample audio into buffers for analysis (peak detection, pitch detection, waveform display). `replaceAudioFile()` for writing modified audio back. `deleteSample()` and `duplicateSample()` for restructuring sample maps programmatically.

### Practical Defaults
- Use `Sampler.createSelection(".*")` to get all samples as an array of Sample objects. Pass `-1` to `createSelectionFromIndexes()` for the same result by index.
- When setting velocity ranges, call `set()` twice for both `LoVel` and `HiVel` in the same pass - the auto-clipping ensures consistency but the order matters when narrowing ranges.
- Use `getRange()` before setting loop-related properties. Loop bounds are interdependent (LoopEnd depends on SampleEnd, LoopStart depends on LoopXFade), so reading the valid range first prevents silent clamping surprises.
- Attach analysis results to `getCustomProperties()` rather than external arrays when you need to sort or filter samples by computed values. The metadata stays associated with each Sample object through sorting operations.

### Integration Patterns
- `Sampler.createSelection()` / `createSelectionWithFilter()` -> `Sample.get()` / `Sample.set()` -- the standard selection-then-modify workflow for all sample map editing.
- `Sample.loadIntoBufferArray()` -> `Buffer.detectPitch()` / `Buffer.getMagnitude()` -- audio analysis pipeline. Load audio, analyze, then write results back via `Sample.set()` or store in `getCustomProperties()`.
- `Sample.getCustomProperties()` -> `Engine.sortWithFunction()` -- sort samples by computed metadata (e.g., sort by peak loudness to assign velocity layers).
- `Sample.getRange()` -> `Sample.set()` -- query dynamic bounds then set properties within valid range, essential for loop point UIs where `getRange(Sample.LoopEnd)[1]` gives the maximum valid loop end.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Setting `LoVel`/`HiVel` once and assuming it sticks | Set both `LoVel` and `HiVel` in the same pass, potentially twice | Due to auto-clipping and interdependent ranges, setting `LoVel` higher than the current `HiVel` gets clamped. Set `HiVel` first when widening, `LoVel` first when narrowing. |
| Storing analysis data in a parallel array alongside a sample selection | Use `getCustomProperties()` to attach data directly to each Sample | Parallel arrays break when the selection is sorted or filtered. Custom properties travel with the Sample object. |
| Setting loop points without checking `getRange()` first | Query `getRange(Sample.LoopEnd)` to get the valid bounds | Loop property ranges are dynamic - LoopEnd's max depends on SampleEnd, LoopStart's min depends on SampleStart + LoopXFade. Setting values outside the valid range silently clamps them. |
