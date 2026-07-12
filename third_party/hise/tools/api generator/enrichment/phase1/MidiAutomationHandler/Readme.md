# MidiAutomationHandler -- Class Analysis

## Brief
Manages MIDI CC-to-parameter automation mappings with popup customization and change notification callbacks.

## Purpose
MidiAutomationHandler is a scripting wrapper around the singleton `MidiControllerAutomationHandler` that manages MIDI CC-to-parameter automation connections. It provides methods to read and write automation data as JSON, configure the right-click CC assignment popup (which CC numbers appear, their display names), control exclusive mode (one CC per parameter), and toggle whether automated CC messages are consumed from the MIDI stream. A callback mechanism notifies scripts whenever the automation configuration changes (e.g., after MIDI learn, preset load, or programmatic data updates).

## Details

### Architecture

MidiAutomationHandler is a thin scripting facade. Each call to `Engine.createMidiAutomationHandler()` creates a new wrapper object, but all wrappers share the same underlying C++ singleton accessed via `MainController -> MacroManager -> getMidiControlAutomationHandler()`. The wrapper subscribes as a `SafeChangeListener` on construction and unsubscribes on destruction.

### Automation Data JSON Schema

See `getAutomationDataObject()` for the full property table. The `Channel` property uses 1-based indexing (1-16, or -1 for omni). The `Attribute` property can be a string identifier (for custom automation or named parameters) or a numeric index (legacy format). Use `setAutomationDataFromObject()` to write the data back.

### Popup Configuration

Two methods customize the right-click CC assignment popup on UI components. See `setControllerNumbersInPopup()` for CC filtering (flat list vs. submenu layout) and `setControllerNumberNames()` for custom display names (section header and per-CC labels).

### Exclusive Mode

See `setExclusiveMode()`. When enabled, each CC number can only control one parameter at a time. Assigning a CC to a new parameter first removes all existing automations for that CC. The popup also grays out CC numbers that are already assigned.

### Event Consumption

See `setConsumeAutomatedControllers()`. By default, CC messages matching an automation entry are removed from the MIDI buffer before reaching script callbacks like `onController`. Disabling consumption allows automated CC messages to pass through to scripts.

### Macro Routing

When an automation entry has `MacroIndex != -1`, the incoming CC value is routed through the macro system (`MacroChain.setMacroControl()`) instead of directly setting the processor attribute. The raw CC value (0-127) is passed to the macro, which handles its own range mapping.

### Custom Automation Data Model

When `UserPresetHandler.setCustomAutomation()` is active, CC automation targets custom automation slot indices instead of raw processor attributes. The `Attribute` field is reinterpreted as a custom automation slot identifier.

### Change Notification

See `setUpdateCallback()` for the callback registration API. The callback receives the complete current automation data array (not a delta) whenever mappings change (MIDI learn, removal, preset restore, clear). During preset loads, the notification is synchronous; all other changes fire asynchronously.

### UserPreset Integration

Automation data is automatically saved and restored as part of user presets via the `UserPresetStateManager` interface (segment ID: `"MidiAutomation"`). No manual save/restore is needed unless you want to implement a custom automation UI.

### Per-Channel Automation

The dynamic preprocessor `HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION` (default: disabled) enables per-channel CC assignment, allowing the same CC number on different MIDI channels to control different parameters. This is set via project ExtraDefinitions without recompiling HISE.

## obtainedVia
`Engine.createMidiAutomationHandler()`

## minimalObjectToken
mah

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `mah.setUpdateCallback(function(data){ ... }); mah.setUpdateCallback(undefined);` | Only register the callback once; there is no unregister mechanism | Passing a non-function value to `setUpdateCallback` is a silent no-op -- the previous callback remains active. There is no way to remove a registered callback. |

## codeExample
```javascript
// Create a MidiAutomationHandler and configure its popup
const mah = Engine.createMidiAutomationHandler();

// Only show Mod Wheel and Expression in the CC popup
mah.setControllerNumbersInPopup([1, 11]);

// Give them readable names
mah.setControllerNumberNames("Controls", ["Mod Wheel", "Expression"]);

// Get notified when automation mappings change
mah.setUpdateCallback(function(data)
{
    Console.print("Automation entries: " + data.length);
});
```

## Alternatives
- **MacroHandler** -- Both manage parameter automation mappings; MidiAutomationHandler maps MIDI CC messages while MacroHandler maps macro knob indices.
- **UserPresetHandler** -- UserPresetHandler manages the full preset and host automation system; MidiAutomationHandler handles the MIDI CC mapping subset.

## Related Preprocessors
`HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION` -- dynamic preprocessor (default: 0). When enabled, the same CC number on different MIDI channels can control different parameters.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods are simple state-setters or data accessors with no silent-failure preconditions or timeline dependencies. Invalid inputs (non-array to setControllerNumbersInPopup, non-function to setUpdateCallback) are handled gracefully as no-ops.
