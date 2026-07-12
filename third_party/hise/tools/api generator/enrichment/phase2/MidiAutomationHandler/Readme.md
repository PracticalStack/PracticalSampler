# MidiAutomationHandler -- Project Context

## Project Context

### Real-World Use Cases
- **Custom CC automation system for a multi-channel instrument**: An instrument maps a contiguous range of CC numbers to per-channel parameters, using exclusive mode so each CC controls exactly one parameter. The update callback feeds a Broadcaster that drives UI indicators showing which CC is assigned to which knob, disabling knobs that are under automation, and repositioning visual indicators next to automated controls.
- **Programmatic automation data manipulation**: A plugin that needs to modify CC-to-parameter mappings from script code -- reading the current automation data, modifying the `Attribute` field on specific entries (e.g., when re-routing connections between layers), and writing the modified array back. This is the round-trip pattern for any UI that lets users re-assign or batch-edit automation connections.
- **Undoable CC assignment removal**: An instrument that wraps automation data changes in `Engine.performUndoAction()` so that removing a CC assignment from a context menu can be undone. The undo action stores the before/after automation data arrays and calls `setAutomationDataFromObject()` for both directions.

### Complexity Tiers
1. **Basic popup customization** (most common): `Engine.createMidiAutomationHandler()`, `setControllerNumbersInPopup()`, `setControllerNumberNames()`, `setExclusiveMode(true)`. Enough to restrict which CC numbers appear in the right-click automation popup and give them readable names. No callback needed.
2. **Monitoring automation changes**: Add `setUpdateCallback()` to receive notifications when CC assignments change (MIDI learn, preset load, programmatic changes). Forward the callback data to a Broadcaster so multiple UI systems can react.
3. **Full programmatic automation management**: Add `getAutomationDataObject()` and `setAutomationDataFromObject()` to read, modify, and write back automation entries from script code. Used for batch operations (clear all, re-route connections between layers), undo/redo integration, and custom automation UIs that go beyond the built-in right-click popup.

### Practical Defaults
- Enable exclusive mode (`setExclusiveMode(true)`) when each CC should control only one parameter. This prevents accidental multi-assignment and grays out already-assigned CC numbers in the popup.
- Use `setConsumeAutomatedControllers(true)` (the default) unless you need the CC messages to also reach `onController` callbacks for visual feedback or additional processing.
- Forward update callback data to a Broadcaster rather than handling it in a single function. This decouples the automation system from UI code and allows multiple listeners (indicator panels, enable-state managers, sequencer refresh logic) to react independently.
- When restricting the CC popup, always pair `setControllerNumbersInPopup()` with `setControllerNumberNames()` so users see readable labels instead of raw CC numbers.

### Integration Patterns
- `MidiAutomationHandler.setUpdateCallback()` -> `Engine.createBroadcaster()` -- forward the automation data array to a broadcaster property inside the callback. Multiple UI systems then listen to the broadcaster for indicator positioning, knob enable-state, and sequencer refresh.
- `MidiAutomationHandler.getAutomationDataObject()` -> modify array -> `MidiAutomationHandler.setAutomationDataFromObject()` -- standard read-modify-write cycle for programmatic assignment changes (removing entries, re-routing `Attribute` fields between layers).
- `MidiAutomationHandler.setAutomationDataFromObject()` inside `Engine.performUndoAction()` -- wrap data modifications in an undo action by storing the before/after automation arrays, calling `setAutomationDataFromObject()` with the appropriate array on undo/redo.
- `Broadcaster.addComponentPropertyListener()` with automation data -- use a broadcaster driven by the update callback to disable UI controls that are currently under CC automation, preventing manual adjustment of automated parameters.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Handling all automation-change logic inside the `setUpdateCallback` function directly | Forward the data to a Broadcaster and add multiple independent listeners | A single callback creates tight coupling between the automation system and every UI element that needs to react. A broadcaster allows indicator panels, enable-state managers, sequencer logic, and other systems to subscribe independently. |
| Calling `setControllerNumbersInPopup()` without `setControllerNumberNames()` | Always pair both calls so users see readable names | When only the CC filter is set, the popup shows "CC#20", "CC#21", etc. -- meaningless to end users. Pair with custom names for a polished UX. |
| Removing an automation entry by writing a filtered array without undo support | Wrap `setAutomationDataFromObject()` in `Engine.performUndoAction()` with before/after snapshots | Users expect automation assignment changes to be undoable. Store the old and new arrays in the undo object and call `setAutomationDataFromObject()` in both directions. |
