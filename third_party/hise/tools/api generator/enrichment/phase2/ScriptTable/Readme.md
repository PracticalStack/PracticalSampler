# ScriptTable -- Project Context

## Project Context

### Real-World Use Cases
- **Velocity and response shaping in MIDI callbacks**: A scripted instrument uses a table as a transfer function for incoming performance data. Note velocity (or note-length-derived values) is normalized, looked up with `getTableValue()`, then mapped back into MIDI or gain space before calling `Message.setVelocity()` or fade methods.
- **Shared curve data between UI and processing logic**: A UI table is registered with `registerAtParent()` so non-UI callbacks can read the same curve through a data handle. This keeps the editor and runtime mapping in sync without duplicating curve data in script arrays.
- **Modulation editor screens with custom table visuals**: A UI layer attaches one local look-and-feel to a group of table components and overrides table draw functions. This gives consistent styling for background, path, points, and ruler across all modulation curves.
- **Dense editor layouts that suppress drag popup text**: In compact modulation views, popup text is intentionally hidden via `setTablePopupFunction()` to avoid covering neighboring controls while dragging points.

### Complexity Tiers
1. **Single-curve mapper** (most common): Create one table and read it with `getTableValue()` in note callbacks for velocity or modulation shaping.
2. **Shared data workflow**: Use `registerAtParent()` and read through the returned handle in processing callbacks to decouple UI editing from playback logic.
3. **Full editor system**: Combine grouped `setLocalLookAndFeel()` styling with `setTablePopupFunction()` behavior control for multi-table modulation pages.

### Practical Defaults
- Normalize MIDI-domain inputs before lookup: `value / 127.0` is a good default whenever table lookups are driven by note velocity or note number.
- Call `registerAtParent()` once during init and cache the returned handle for runtime use.
- Register `drawTableBackground`, `drawTablePath`, `drawTablePoint`, and `drawTableRuler` together on the same LAF object for consistent table rendering.
- Use an empty popup formatter (or restore default with `false`) when drag popups obscure dense control layouts.

### Integration Patterns
- `ScriptTable.registerAtParent()` -> `ScriptTableData.getTableValueNormalised()` -- Register the UI curve once, then use the returned data handle in note-processing code paths.
- `ScriptTable.getTableValue()` -> `Message.setVelocity()` -- Use table output as a non-linear transfer curve for expressive velocity remapping.
- `ScriptTable.getTableValue()` -> `Synth.addVolumeFade()` -- Convert normalized lookup output into attenuation or gain offsets for timed release behavior.
- `ScriptTable.setLocalLookAndFeel()` -> `ScriptLookAndFeel.registerFunction()` (`drawTableBackground`, `drawTablePath`, `drawTablePoint`, `drawTableRuler`) -- Attach one shared LAF object to multiple table editors for a coherent visual system.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Passing raw MIDI values to `getTableValue()` (eg. `table.getTableValue(Message.getVelocity())`) | Normalizing before lookup (eg. `table.getTableValue(Message.getVelocity() / 127.0)`) | Table lookups are normalized-domain operations. Unnormalized inputs collapse mapping behavior and make the curve feel ineffective. |
| Calling `registerAtParent()` inside note callbacks | Registering once in init and reusing the returned handle | Registration is setup work. Repeating it during playback adds unnecessary overhead and can create confusing state changes. |
| Overriding only one table draw callback and leaving the rest implicit | Defining a complete table draw set (`background`, `path`, `point`, `ruler`) on the same LAF | Partial overrides often produce mixed visuals where default drawing and custom drawing conflict. |
