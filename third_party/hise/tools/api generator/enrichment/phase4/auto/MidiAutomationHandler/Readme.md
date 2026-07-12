<!-- Diagram triage:
  - (no diagrams in Phase 1 data)
-->

# MidiAutomationHandler

MidiAutomationHandler provides scripting access to the MIDI CC-to-parameter automation system. It lets you:

1. Customise which CC numbers appear in the right-click automation popup and give them readable names.
2. Read and modify the current automation mappings programmatically (read-modify-write cycle).
3. Register a callback that fires whenever mappings change (MIDI learn, preset load, removal).
4. Enable exclusive mode so each CC controls only one parameter at a time.

The built-in way to enable MIDI CC assignment is to set the `enableMidiLearn` property of a suitable component (slider, button, combobox) to `true`. This adds a right-click popup that lets users assign a CC to that control. If you are using the custom automation model, set the `allowMidiAutomation` property in the JSON object passed to `UserPresetHandler.setCustomAutomation()` instead. The [MidiLearnPanel](/ui-components/floating-tiles/plugin/midilearnpanel) floating tile provides a no-code UI for viewing and editing all current assignments, including range adjustment and inversion.

If you need more control than the built-in popup and MidiLearnPanel offer, use this class. The object is obtained with `Engine.createMidiAutomationHandler()`. Multiple calls return separate wrapper objects, but they all operate on the same shared automation state.

```js
const var mah = Engine.createMidiAutomationHandler();
```

For basic popup customisation, pair `setControllerNumbersInPopup()` with `setControllerNumberNames()` so the right-click menu shows a curated set of controllers with descriptive labels instead of the full list of 128 CC numbers. Enable exclusive mode when you want to prevent the same CC from driving multiple parameters.

For advanced use cases, `getAutomationDataObject()` and `setAutomationDataFromObject()` provide a round-trip interface: read the current mappings as an array of JSON objects, modify entries, and write them back. This supports batch operations, undo/redo integration, and fully custom automation UIs.

> [!Tip:Automation data saved with user presets] Automation data is saved and restored automatically as part of user presets. You only need this class if you want to customise the popup appearance, monitor changes, or manage mappings from script code.
>
> [!Tip:Automated CCs consumed before onController] By default, CC messages that match an automation entry are consumed before reaching `onController`. Call `setConsumeAutomatedControllers(false)` if you need automated CCs to pass through to script callbacks as well.

## Common Mistakes

- **Pair popup filter with readable names**
  **Wrong:** Calling `setControllerNumbersInPopup()` without `setControllerNumberNames()`
  **Right:** Always pair both calls so users see readable names instead of raw CC numbers.
  *When only the CC filter is set, the popup shows "CC#20", "CC#21", etc. - meaningless to end users.*

- **Use Broadcaster for multi-consumer updates**
  **Wrong:** Handling all automation-change logic directly inside the `setUpdateCallback` function.
  **Right:** Forward the data to a Broadcaster and add multiple independent listeners.
  *A single callback creates tight coupling between the automation system and every UI element that needs to react. A broadcaster allows indicator panels, enable-state managers, and other systems to subscribe independently.*

- **Wrap automation changes in undo action**
  **Wrong:** Removing an automation entry by writing a filtered array without undo support.
  **Right:** Wrap `setAutomationDataFromObject()` in `Engine.performUndoAction()` with before/after snapshots.
  *Users expect automation assignment changes to be undoable. Store the old and new arrays in the undo object and call `setAutomationDataFromObject()` in both directions.*
