# ScriptComboBox -- Project Context

## Project Context

### Real-World Use Cases
- **Audio file selector**: A combo box populated dynamically from audio files on disk (samples, impulse responses, noise layers). The item list is built at init by scanning a folder, and the control callback loads the selected file into an `AudioSampleProcessor` or `Sampler`. This is the most common combo box use case in production plugins.
- **Multi-output channel router**: A combo box for each instrument channel lets the user pick a stereo output pair (e.g., "MASTER", "3/4", "5/6"). The control callback reconfigures the `RoutingMatrix`. Typically paired with a minimal LAF that shows only a dropdown triangle.
- **Cascading parameter selector**: Two or three dependent combo boxes where changing the first rebuilds the item list of the second (and third). Used for hierarchical selection: category -> option -> variant. Requires `changed()` to propagate dependent updates after rebuilding.
- **Synthesizer mode/type selector**: Combo boxes for oscillator waveform, filter type, arpeggiator direction, or unisono voice count. These drive module attributes or load different configurations.

### Complexity Tiers
1. **Static selector** (most common): Set items via `set("items", ...)` at init, add a `setControlCallback`, map `value-1` to an action. Covers oscillator type, filter mode, and similar fixed-option selectors.
2. **Dynamic file selector**: Clear items, scan files with `FileSystem.findFiles()`, populate with `addItem()` in a loop, use `getItemText()` in the callback to resolve the selection. Set `saveInPreset` to `false` since file lists may change between sessions.
3. **Cascading dependent lists**: Multiple combo boxes where changing one rebuilds the items of another. Requires `set("items", ...)` to update the dependent list, `setValue()` to clamp the selection to the new range, and `changed()` to trigger the dependent callback chain.
4. **Fully styled selector**: Custom `drawComboBox` LAF with matching popup menu LAF (`drawPopupMenuBackground`, `drawPopupMenuItem`, `getIdealPopupMenuItemSize`). Often uses `Content.createLocalLookAndFeel()` for per-context styling (minimal dropdown arrow, LED-style text, image-based labels).

### Practical Defaults
- Set `saveInPreset` to `false` for combo boxes whose items are generated dynamically from file scanning. The item list may differ between machines, so persisting the index is meaningless. Use a processor connection or manual state management instead.
- Clear items with `set("items", "")` before populating with `addItem()` to avoid stale entries from the Interface Designer.
- Use `parseInt(value)` when using the combo box value as an array index. Combo box values arrive as floats in callbacks (e.g., `1.0` not `1`).
- Use `value - 1` (after `parseInt`) to convert from the 1-based combo box index to a 0-based array index.

### Integration Patterns
- `ScriptComboBox.getItemText()` -> `Sampler.loadSampleMap()` -- The selected item text maps directly to a sample map name for loading.
- `ScriptComboBox.addItem()` <- `FileSystem.findFiles()` -- Dynamically build item lists from audio files found on disk.
- `ScriptComboBox.setLocalLookAndFeel()` + `drawComboBox` + `drawPopupMenuItem` -- Custom combo box rendering requires styling both the closed combo box and its popup menu items.
- `ScriptComboBox.changed()` -> dependent `ScriptComboBox.set("items", ...)` -- In cascading selectors, call `changed()` on dependent combo boxes after rebuilding their item lists to trigger their callbacks.
- `Broadcaster.addListener()` -> `ScriptComboBox.setValue()` -- Broadcasters can drive combo box selection to keep UI in sync with state changes from other sources.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using `value` directly as array index | `array[parseInt(value) - 1]` | Combo box values are 1-based floats. Convert to integer and subtract 1 for 0-based array access. |
| Populating with `addItem()` without clearing first | `cb.set("items", ""); cb.addItem(...)` | The Interface Designer may have left stale items in the property. Clear before rebuilding. |
| Saving dynamic file lists in presets | `cb.set("saveInPreset", false)` | File-scanned item lists differ between machines. Persist the file reference or processor state instead. |
| Styling only `drawComboBox` | Also register `drawPopupMenuBackground` and `drawPopupMenuItem` on the same LAF | The popup menu uses separate draw functions. Without them, the popup renders with default styling while the closed box looks custom. |
| Rebuilding dependent combo items without `changed()` | Call `cb.changed()` after `set("items", ...)` and `setValue()` | Without `changed()`, the dependent callback chain does not fire, leaving the UI in a stale state. |
