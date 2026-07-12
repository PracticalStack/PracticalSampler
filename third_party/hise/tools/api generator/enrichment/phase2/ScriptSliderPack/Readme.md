# ScriptSliderPack -- Project Context

## Project Context

### Real-World Use Cases
- **Step-lane editors for sequencer workflows**: A sequencer UI uses one slider pack per lane to edit per-step velocity, chance, pitch, or modulation values. The same lane is rebound to different `SliderPackData` sources as the edit mode changes, so one UI surface can edit multiple datasets.
- **Shared multi-view editing**: A main editor and a compact overview bind to the same `SliderPackData` handle with `referToData()`. This keeps multiple views in sync without manual copy logic.
- **Batch visualization tools**: Utility scripts collect measured values, sort them, then write the result into a slider pack with `setSliderAtIndex()` for quick visual inspection and manual correction.
- **Context-aware rhythmic grids**: Sequencer pages switch width maps at runtime with `setWidthArray()` so hit-testing and drawing reflect the active rhythmic subdivision.

### Complexity Tiers
1. **Single-lane editor** (most common): `set("sliderAmount", ...)`, `setControlCallback()`, and `getSliderValueAt()` are enough for one editable lane.
2. **Shared-data lane system**: Add `referToData()` so the same UI component can edit different external slider-pack data objects.
3. **Sequencer-scale lane management**: Add `setAllValueChangeCausesCallback()`, `setWidthArray()`, and lane-group styling via `setLocalLookAndFeel()` for high-density editors.

### Practical Defaults
- Use `set("mouseUpCallback", true)` for step-lane editing so drag gestures commit once instead of firing dense callback bursts.
- Disable bulk callbacks with `setAllValueChangeCausesCallback(false)` during pattern loads, then trigger one explicit downstream refresh.
- Keep `sliderAmount` and `setWidthArray()` synchronized in the same update path.
- Use one shared local LAF object for all lane packs in a section, and override slider-pack-specific draw functions together.

### Integration Patterns
- `ScriptSliderPack.referToData()` -> `Engine.createAndRegisterSliderPackData()` -- Bind UI lanes to external data handles so playback logic and UI editing share one source of truth.
- `ScriptSliderPack.setControlCallback()` -> `ScriptSliderPack.getSliderValueAt()` -- Treat the callback payload as an index and fetch the edited value explicitly.
- `ScriptSliderPack.setAllValues()` -> `ScriptSliderPack.setAllValueChangeCausesCallback()` -- Suppress callback storms during data import and apply one explicit rebuild pass.
- `ScriptSliderPack.setLocalLookAndFeel()` -> `ScriptLookAndFeel.registerFunction()` (`drawSliderPackBackground`, `drawSliderPackTextPopup`) -- Apply lane-specific rendering behavior across a grouped editor.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Importing many lanes with callbacks enabled, then wondering why rebuild logic runs repeatedly | Disable callbacks with `setAllValueChangeCausesCallback(false)` during import, then trigger one explicit rebuild | Bulk writes are often setup operations. Realtime-style callback fanout during imports causes avoidable work and UI churn. |
| Treating the `setControlCallback()` `value` argument as the lane value | Interpret `value` as the edited index and fetch data with `getSliderValueAt(index)` | Slider-pack callbacks are index-centric in production lane editors. Reading by index avoids wrong mappings when range or mode changes. |
| Updating `setWidthArray()` without updating `sliderAmount` in the same code path | Update both together from one subdivision switch function | Width-map size and lane count must stay aligned for predictable drawing and hit-testing. |
