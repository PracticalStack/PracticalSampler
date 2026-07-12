# ScriptFloatingTile -- Project Context

## Project Context

### Real-World Use Cases
- **Preset browser integration**: The most common use case. A plugin needs a preset browser with bank/category/preset navigation, search, and custom styling. A ScriptFloatingTile with `"Type": "PresetBrowser"` provides this entire system as a single embedded widget. The preset browser is typically configured once in `onInit` with a JSON data object, then styled via `setLocalLookAndFeel()` with custom draw functions for list items, column backgrounds, and the search bar.
- **Interactive EQ display**: A plugin with parametric EQ needs a draggable frequency response display. A `"DraggableFilterPanel"` floating tile connected to a `CurvedEQ` processor provides interactive filter handles, real-time FFT spectrum overlay, and automatic parameter binding. The floating tile's `ProcessorId` is switched dynamically via `setContentData()` to display different EQ instances (e.g., switching between master/mid/side EQ views via a radio group).
- **Multi-channel peak metering**: A multi-output instrument needs per-channel level meters. A `"MatrixPeakMeter"` floating tile factory function creates reusable meter instances, each configured with specific `ChannelIndexes` and a target `ProcessorId`. The meter configuration is cloned and updated dynamically when the active channel changes.
- **Virtual MIDI keyboard**: A synthesizer needs an on-screen keyboard with custom key range, MPE support, and visual feedback. A `"Keyboard"` floating tile provides this with extensive JSON configuration for key width, range, MPE channels, and visual style. Multiple keyboard instances can coexist with different ranges and configurations.
- **Markdown info panel**: A plugin needs an in-app documentation or about panel. A `"MarkdownPanel"` floating tile renders formatted text with custom fonts, embedded in a modal overlay framework.
- **Modulation matrix UI**: A modulation synth embeds `"ModulationMatrix"` and `"ModulationMatrixController"` floating tiles for the built-in modulation routing system, styled via CSS for table layout.
- **Filter and analyser displays**: A synth page embeds `"FilterDisplay"` tiles connected to individual oscillator filters, each pointed at a different processor via `ProcessorId`. These are typically styled with a shared `setLocalLookAndFeel()` for consistent appearance.

### Complexity Tiers
1. **Static embed** (most common): Create a floating tile, call `setContentData()` once with a JSON object containing `"Type"` and optional configuration. Set position/size. Covers `PresetBrowser`, `Keyboard`, `PerformanceLabel`, `CustomSettings`, `AHDSRGraph`, and `MarkdownPanel` use cases. Methods needed: `setContentData()`, `set()`, `setPosition()`.
2. **Styled embed**: Add `setLocalLookAndFeel()` with custom draw functions for the embedded panel type. Preset browsers and filter displays are the most commonly styled floating tiles. Additional methods: `setLocalLookAndFeel()`, colour properties via `set()`.
3. **Dynamic retargeting**: Change the floating tile's processor connection or configuration at runtime by calling `setContentData()` with a modified JSON clone. Used for EQ panels that switch between processor instances or peak meters that follow channel selection. Requires understanding the `.clone()` pattern for JSON data mutation.
4. **Factory function pattern**: Wrap floating tile creation in a namespace with a `make()` function that returns configured instances. Used when multiple floating tiles share the same base configuration (peak meters, filter displays). Combines all three tiers above into a reusable module.

### Practical Defaults
- Use `"NumColumns": 2` for a two-column preset browser layout (category + presets) with `"ColumnWidthRatio": [0.3, 0.7]` as a good starting ratio.
- Use `"DefaultAppearance": false` and `"CustomGraphics": true` for keyboard tiles that need non-standard key rendering via LAF.
- Use `"AllowDynamicSpectrumAnalyser": 1` with `"AllowFilterResizing": false` for EQ floating tiles that show an FFT overlay without allowing band count changes.
- Use `"GainRange": 12.0` as a practical default for EQ filter display gain range.
- Use `"DownDecayTime": 1400` and `"SkewFactor": 0.5` as good defaults for peak meters that need smooth visual decay.
- Always `.clone()` the data object before modifying and passing to `setContentData()` when reusing a shared configuration template. Direct mutation of the template affects all future calls.
- Set `"bgColour"` to `0` (transparent) when the floating tile sits on a custom-drawn panel background.

### Integration Patterns
- `Content.addFloatingTile()` -> `setContentData({"Type": "DraggableFilterPanel", "ProcessorId": id})` -> `setLocalLookAndFeel(laf)` -- Standard pattern for an interactive EQ display with custom styling.
- `Broadcaster.addListener(floatingTile, ...)` -> `setContentData(data.clone())` -- Broadcaster-driven retargeting: when a radio group or channel selector changes, a listener on the floating tile calls `setContentData()` with an updated `ProcessorId` to point at a different processor.
- `Synth.getDisplayBufferSource(id).getDisplayBuffer(0)` -> `setRingBufferProperties({...})` -> `Content.addFloatingTile()` -> `setContentData({"Type": "DraggableFilterPanel"})` -- FFT spectrum setup: configure the display buffer properties before creating the floating tile that visualizes them.
- `Content.createLocalLookAndFeel()` -> `registerFunction("drawPresetBrowserListItem", ...)` -> `floatingTile.setLocalLookAndFeel(laf)` -- Preset browser styling: create a local LAF, register draw functions for the specific preset browser callbacks, then attach to the floating tile.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `ft.setContentData(data); data.ProcessorId = "NewEQ"; ft.setContentData(data);` | `var newData = data.clone(); newData.ProcessorId = "NewEQ"; ft.setContentData(newData);` | Mutating a shared data object then passing it again can produce unexpected results because `setContentData` stores a reference. Always `.clone()` before modifying shared configuration templates. |
| Setting colours via `ft.set("bgColour", ...)` before `setContentData()` | Call `setContentData()` first, then set colours via `ft.set()` | `setContentData()` triggers a full content reload that may reset colour properties. Set visual properties after the content type is established. |
| Calling `setContentData()` on every frame or timer tick | Call `setContentData()` only when the configuration actually changes | Every `setContentData()` call destroys and recreates the entire embedded panel. This is expensive and causes visual flicker. Gate calls behind a state-change check. |
