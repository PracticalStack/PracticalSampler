# MacroHandler -- Project Context

## Project Context

### Real-World Use Cases
- **Custom automation routing with NKS integration**: A multi-channel instrument that exposes XY pad parameters as macro-controlled host automation slots, allowing NKS hosts and DAW automation to control per-channel parameters. The MacroHandler bridges the internal custom automation system (via UserPresetHandler) with DAW-visible macro slots.
- **Clearing macro state on init**: Instruments that define their automation parameters programmatically need to clear all macro connections at startup before the custom preset model populates them, ensuring no stale connections survive between sessions.

### Complexity Tiers
1. **Basic macro monitoring** (most common): `Engine.createMacroHandler()`, `setUpdateCallback()`. Enough to watch for macro connection changes and update UI accordingly.
2. **Read-modify-write macro connections**: Add `getMacroDataObject()` and `setMacroDataFromObject()` to programmatically toggle individual macro-to-parameter connections from UI code (e.g., context menu actions).
3. **Full macro management with custom automation**: Combine with `UserPresetHandler.setCustomAutomation()` to route macros to custom automation slots. Use `setExclusiveMode(true)` to enforce one-target-per-macro. Clear on init with `setMacroDataFromObject([])`.

### Practical Defaults
- Enable exclusive mode (`setExclusiveMode(true)`) when each macro slot should control exactly one parameter. This is the expected mode for NKS and DAW automation workflows.
- Pass a Broadcaster as the update callback rather than a plain function. This turns macro changes into a broadcaster event that multiple listeners can subscribe to, avoiding tight coupling between the macro system and UI code.
- Clear all connections at startup with `setMacroDataFromObject([])` before the preset model restores them. This prevents stale connections from persisting when the plugin initializes without loading a preset.

### Integration Patterns
- `MacroHandler.setUpdateCallback()` -> `Engine.createBroadcaster()` -- pass a broadcaster directly as the callback to fan out macro change notifications to multiple listeners through the event bus.
- `MacroHandler.getMacroDataObject()` -> modify array -> `MacroHandler.setMacroDataFromObject()` -- standard read-modify-write cycle for toggling individual connections from context menus or UI controls.
- `MacroHandler` + `UserPresetHandler.setCustomAutomation()` -- custom automation IDs defined through the UserPresetHandler become valid `Attribute` targets for macro connections when `CustomAutomation` is `true` in the JSON object.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new MacroHandler per UI interaction | Store one `const var mh = Engine.createMacroHandler()` at init and reuse it | Each `createMacroHandler()` call registers a new listener on the macro chain. Multiple instances waste resources and receive duplicate notifications. |
| Passing an inline function to `setUpdateCallback` and trying to reach multiple UI systems | Pass a Broadcaster as the callback, then add listeners to the broadcaster | A single callback function creates tight coupling. A broadcaster allows multiple independent listeners to react to macro changes without the callback needing to know about all consumers. |
