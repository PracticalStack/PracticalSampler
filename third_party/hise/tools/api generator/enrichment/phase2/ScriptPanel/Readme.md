# ScriptPanel -- Project Context

## Project Context

### Real-World Use Cases
- **Layout container**: The most common use by volume. Panels defined in the Interface Designer with no scripted behavior, used purely for `showControl(true/false)` visibility toggling to implement page navigation or effect routing UI. A plugin with multiple pages creates one panel per page and toggles visibility from tab button callbacks.
- **Timer-driven real-time meter**: The most common *scripted* use. A panel polls audio levels or module parameters at 30-50ms intervals via `setTimerCallback`, stores the value in `data`, calls `repaint()`, and the paint routine draws a gradient bar or indicator. Plugins use this for peak meters, gain reduction displays, and modulation level LEDs.
- **Custom interactive control**: Panels with full mouse interaction (`"All Callbacks"` or `"Clicks, Hover & Dragging"`) implementing controls that cannot be built with standard components: step sequencer grids, XY pads, interactive EQ displays, envelope editors, range sliders, and compressor visualizations. These use all three callback types (paint, mouse, timer) working together.
- **Factory-created dynamic UI**: At scale, plugins create panels programmatically in loops using factory functions that accept configuration parameters (ID, position, color, callback behavior) and return a configured panel. Essential for scrollable lists, browser items, mixer strip controls, and modulation source panels.
- **Data storage for preset persistence**: Hidden panels with `saveInPreset: true` and no visual presence exploit the HISE preset serialization system to persist complex state (articulation assignments, modulation connection data) that doesn't map to simple parameter values.
- **Modal dialog overlay**: A full-interface panel with semi-transparent background, custom-drawn dialog box with OK/Cancel buttons, and an optional text input field. The mouse callback dismisses the dialog when clicking outside the box area.

### Complexity Tiers
1. **Static container** (most common): No scripted behavior. Uses `set("visible", ...)` or `showControl()` for page navigation. Methods needed: `set`, `showControl`, `get`.
2. **Background image display**: `loadImage` + `setImage` to show a static image. No paint routine, no mouse callback.
3. **Simple painted panel**: `setPaintRoutine` with basic shape/text drawing. `repaint()` called from external events. No mouse or timer.
4. **Timer-driven visualizer**: `setPaintRoutine` + `setTimerCallback` + `startTimer`. The `data` object stores polled values between timer and paint callbacks. Common for meters and level indicators.
5. **Interactive control**: All of tier 4 plus `setMouseCallback` with `allowCallbacks` set to "Clicks, Hover & Dragging" or "All Callbacks". The mouse callback updates `data` state, calls `repaint()`, and the paint routine renders based on that state. Used for sequencers, XY pads, and custom sliders.
6. **Full-featured panel**: Interactive control plus `setFileDropCallback`, `startInternalDrag`, `setLoadingCallback`, broadcaster integration, and/or undo support via `setPanelValueWithUndo`.

### Practical Defaults
- Use `"Clicks Only"` for simple click handlers (buttons, links). Escalate to `"Clicks & Hover"` when hover feedback is needed, `"Clicks, Hover & Dragging"` for drag controls, and `"All Callbacks"` only for controls that need continuous mouse move tracking (sequencer grids, envelope editors).
- Set `"opaque"` to `true` on panels that fill their entire area with a solid background. This avoids unnecessary alpha compositing and improves rendering performance.
- A timer interval of 30ms is a good default for real-time meters and playhead tracking. Use 50ms for less time-critical polling. Never go below 15ms.
- Store all per-panel runtime state in the `data` object rather than in external variables. This keeps state co-located with the panel and accessible via `this.data` inside all callbacks.
- Use `const var` to cache `Content.getComponent()` references at init time. Never call `Content.getComponent()` inside paint routines or timer callbacks.

### Integration Patterns
- `ScriptPanel.setTimerCallback()` + `Synth.getEffect().getCurrentLevel()` -> `ScriptPanel.repaint()` -- Timer polls a module's level, stores in `data`, triggers repaint for meter visualization.
- `ScriptPanel.setMouseCallback()` + `ScriptPanel.startInternalDrag()` -- Mouse click in a panel header initiates a modulation drag operation, passing a typed drag data object.
- `ScriptPanel.setLoadingCallback()` + `ScriptPanel.startTimer()` / `ScriptPanel.stopTimer()` -- Loading callback starts a spinner animation timer when preloading begins and stops it when preloading ends.
- `ScriptPanel.setFileDropCallback()` + `Broadcaster.sendAsyncMessage()` -- File drop callback notifies a broadcaster which coordinates state updates across multiple panels (hover overlay, waveform display, channel assignment).
- `ScriptPanel.setMouseCallback()` + `ScriptPanel.setValue()` + `ScriptPanel.changed()` -- Mouse drag computes a normalized value, sets it on the panel, and fires the control callback for downstream processing.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `Content.getComponent()` inside `setPaintRoutine` or `setTimerCallback` | Cache component references in `const var` at init time | `Content.getComponent()` performs a lookup on every call. In a 30ms timer, this adds up to thousands of unnecessary lookups per minute. |
| Using external `var` for panel state shared between callbacks | Use `this.data.propertyName` inside callbacks | The `data` object is per-panel and accessible via `this` in all callbacks. External variables create coupling and break when panels are created in loops. |
| Setting `allowCallbacks` to `"All Callbacks"` for every panel | Use the minimum callback level needed | `"All Callbacks"` fires on every mouse move, which triggers unnecessary repaints. Most panels only need `"Clicks Only"` or `"Clicks & Hover"`. |
| Calling `repaint()` unconditionally in every timer tick | Only call `repaint()` when the displayed value actually changed | Comparing the new value to the stored value before repainting avoids redundant paint cycles. |
