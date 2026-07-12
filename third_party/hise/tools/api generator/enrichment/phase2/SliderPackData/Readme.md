# SliderPackData -- Project Context

## Project Context

### Real-World Use Cases
- **Step sequencer data model**: A drum machine or arpeggiator that stores per-step values (velocity, pitch offset, note probability, timing offset) uses multiple SliderPackData objects organized into a multi-dimensional structure -- one pack per channel per parameter mode per pattern bank. The data exists independently of the UI, enabling MIDI generation logic to read step values from buffers while the UI displays them through ScriptSliderPack components.
- **Preset serialization backbone**: Plugins with complex sequencer state use `toBase64()`/`fromBase64()` to serialize and restore step data as part of a custom preset format, rather than relying on the default user preset system. This enables clipboard copy/paste of individual channel patterns, pattern randomization with undo, and cross-pattern data transfer.

### Complexity Tiers
1. **Basic** (most common): `setNumSliders()`, `setValue()`, `getValue()`, `setRange()` -- a single slider pack for parameter editing or simple data display.
2. **Undo-aware editing**: `setValueWithUndo()`, `setAllValuesWithUndo()` -- interactive step editing with full undo/redo support for recording, randomization, and pattern manipulation.
3. **Bulk data management**: `setUsePreallocatedLength()`, `getDataAsBuffer()`, `toBase64()`/`fromBase64()`, `setAllValues()` with arrays -- managing many SliderPackData objects in batch, serializing state for custom presets, and efficient buffer operations for MIDI generation.

### Practical Defaults
- Use `setUsePreallocatedLength()` immediately after creation when the slider count will change at runtime (e.g., variable-length step sequencers). This avoids reallocation and preserves existing values on resize.
- For step sequencers, initialize values to `0.0` (silence/off) rather than the internal default of `1.0`. Call `setAllValues(0.0)` after `setNumSliders()`.
- Use `setValueWithUndo()` rather than `setValue()` for any user-initiated edit (recording, click-to-edit, randomize). Reserve `setValue()` for non-interactive programmatic updates.
- Use `setAllValuesWithUndo()` for bulk operations that the user should be able to reverse (clear, paste, randomize).

### Integration Patterns
- `Broadcaster.attachToComplexData("SliderPack.Content", ...)` -> listener callback -- watches data changes across multiple SliderPackData objects without per-object callbacks. Preferred over `setContentCallback()` when managing many packs.
- `SliderPackData.getDataAsBuffer()` -> Buffer iteration with `for...in` -- efficient read access for MIDI generation, pattern analysis, and data copying without per-index `getValue()` calls.
- `SliderPackData.toBase64()` / `fromBase64()` -> custom JSON preset format -- serializes step data as compact strings for clipboard, preset storage, or pattern transfer between banks.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating SliderPackData objects without preallocation in a variable-length sequencer | Call `setUsePreallocatedLength(maxSteps)` immediately after creation | Without preallocation, every `setNumSliders()` call allocates a new buffer and loses values beyond the new count. With preallocation, resize is a view adjustment with no allocation. |
| Using `setValue()` for user-initiated edits | Use `setValueWithUndo()` for interactive edits | `setValue()` cannot be undone. Users expect Ctrl+Z to work after editing steps, recording, or randomizing patterns. |
| Assuming new sliders initialize to `0.0` | New sliders initialize to `1.0` (the internal default) | Call `setAllValues(0.0)` explicitly after `setNumSliders()` if zero-initialization is needed (e.g., empty sequencer steps). |
