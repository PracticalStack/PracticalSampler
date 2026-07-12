# Buffer -- Project Context

## Project Context

### Real-World Use Cases
- **Sparse step-lane persistence**: A sequencer stores per-lane step values as Buffer payloads, then writes only non-empty lanes to preset JSON. This keeps preset data compact while preserving exact float lane state.
- **Offline render post-processing**: A render/export pipeline scans Buffer windows for activity, calculates tail length, then writes trimmed channel copies. This avoids exporting long silence regions.
- **Shader data transport**: A visualization layer keeps fixed-size state buffers (notes, envelopes, activity) and pushes them to shader uniforms on a timer. Buffer is used as the lightweight bridge between script state and GPU parameters.

### Complexity Tiers
1. **Level checks and gates**: Use `Buffer.create()`, `Buffer.getMagnitude()`, and `Buffer.getPeakRange()` to drive conditional logic such as activity detection.
2. **State persistence**: Add `Buffer.toBase64()` / `Buffer.fromBase64()` with a sentinel value for empty lanes so state can round-trip through JSON.
3. **Export workflow integration**: Combine window scans with `Buffer.trim()` and `Buffer.referTo()` to produce efficient file-write buffers with minimal copying.

### Practical Defaults
- Use a named block size constant (for example `ANALYSIS_BLOCK_SIZE = 256`) and scan in fixed windows when detecting the last active sample.
- Use an explicit sentinel like `"EMPTY"` for absent serialized buffers, and only call `fromBase64()` when the payload is not the sentinel.
- Reuse one scratch Buffer for repeated decode/transform loops instead of allocating a new Buffer per entry.

### Integration Patterns
- `Buffer.getPeakRange()` -> `Buffer.toBase64()` - Gate serialization on actual lane activity to avoid writing empty payloads.
- `Buffer.fromBase64()` -> `ScriptSliderPack.setAllValuesWithUndo()` - Decode lane data into a temporary Buffer, then apply values through the UI undo path.
- `Buffer.getMagnitude()` -> `Buffer.trim()` - Detect the active tail first, then create trimmed copies for export.
- `Buffer.create()` -> `ScriptShader.setUniformData()` - Maintain fixed-size runtime state arrays and push them to GLSL uniforms.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Call `fromBase64()` on every stored value, including placeholders | Check for a sentinel string first, then decode | Placeholder entries are part of sparse storage patterns; decoding them adds error handling noise and unnecessary work. |
| Decide lane activity with only one side of the range (eg. `range[1] > 0`) | Check both values from `getPeakRange()` (`range[0]` and `range[1]`) | Negative-only lanes can be valid data; checking only the positive peak can misclassify non-empty content as empty. |
| Allocate a fresh temp Buffer inside large import loops | Create one scratch Buffer and reuse it | Reusing temporary buffers keeps import/restore paths predictable and avoids avoidable allocations. |
