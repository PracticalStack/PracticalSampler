# ScriptSlider -- Project Context

## Project Context

### Real-World Use Cases
- **Channel-strip and mixer controls**: A multi-channel instrument UI builds slider groups for pan, level, and send amounts, then applies shared modifier behavior and popup formatting so every channel strip feels consistent.
- **Matrix-target modulation controls**: A synth modulation page marks parameter sliders as matrix targets and applies a custom local look-and-feel so each slider can show modulation overlays and drag feedback.
- **Custom gesture surfaces driving parameters**: A custom panel handles mouse gestures, converts pointer movement to normalized values, and forwards those values into hidden or compact ScriptSlider controls to reuse parameter wiring.

### Complexity Tiers
1. **Parameter-bound control**: Use `set("mode", ...)`, range/default properties, and `setControlCallback()` or processor binding for standard parameter editing.
2. **Interaction tuning**: Add `createModifiers()`, `setModifiers()`, and `setValuePopupFunction()` for fine-tune gestures, reset gestures, and readable value text.
3. **Deep integration**: Combine local LAF (`setLocalLookAndFeel()`), matrix targeting (`matrixTargetId` property), and post-restore sync (`updateValueFromProcessorConnection()`) for advanced modulation and preset workflows.

### Practical Defaults
- Reuse one `createModifiers()` object across a slider collection, then apply `setModifiers()` in a loop for consistent interaction mapping.
- Use `setValuePopupFunction()` with a central formatter for non-linear or semantic values (for example, displaying an "Off" label at the floor value).
- When driving sliders from custom UI gestures, use `setValueNormalized()` and call `changed()` immediately after if downstream callbacks or parameter updates must run.
- After restoring processor state programmatically, call `updateValueFromProcessorConnection()` on connected sliders to keep UI state aligned.

### Integration Patterns
- `ScriptSlider.setValuePopupFunction()` -> `Engine.doubleToString()` - central value-format helper keeps popup text consistent across many controls.
- `ScriptSlider.createModifiers()` -> `ScriptSlider.setModifiers()` - one modifier schema is created once and broadcast to many sliders.
- `ScriptSlider.setLocalLookAndFeel()` -> `ScriptLookAndFeel.registerFunction("drawRotarySlider", ...)` - slider-specific rendering is attached locally without changing global styles.
- `Effect.restoreState()` -> `ScriptSlider.updateValueFromProcessorConnection()` - after state restoration, connected sliders pull fresh processor values.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new modifiers object for every slider in a large loop | Create one modifiers object from a representative slider and reuse its constants for all mappings | This keeps interaction behavior consistent and avoids repeated setup noise. |
| Updating a slider with `setValueNormalized()` from a custom panel and assuming parameter logic already ran | Call `setValueNormalized()` and then `changed()` when callback-driven logic must run | Normalized assignment updates value state, but callback-dependent workflows need an explicit change trigger. |
| Restoring processor/module state and leaving connected sliders untouched | Run `updateValueFromProcessorConnection()` after restore for each connected slider | This prevents stale UI values when processor state changes outside direct slider interaction. |
