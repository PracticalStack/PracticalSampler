# MidiAutomationHandler -- Method Reference

## getAutomationDataObject

**Signature:** `Array getAutomationDataObject()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates DynamicObjects and Array elements when converting the internal ValueTree to JSON.
**Minimal Example:** `var data = {obj}.getAutomationDataObject();`

**Description:**
Returns the complete MIDI automation configuration as an array of JSON objects. Each object represents one CC-to-parameter mapping. The array is a snapshot of the current state -- modifying the returned objects does not affect the live automation data. Use `setAutomationDataFromObject()` to write changes back.

**Parameters:**
None.

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| Controller | int | CC number (0-127) |
| Channel | int | MIDI channel (1-based, 1-16), or -1 for omni |
| Processor | String | Target processor ID |
| MacroIndex | int | Macro slot index, or -1 for direct mapping |
| Attribute | String | Parameter ID or custom automation slot ID |
| Start | double | Active sweep range start |
| End | double | Active sweep range end |
| FullStart | double | Full parameter range start |
| FullEnd | double | Full parameter range end |
| Skew | double | Range skew factor |
| Interval | double | Range step size |
| Inverted | bool | Whether the CC-to-parameter mapping is inverted |
| Converter | String | Value-to-text converter serialization |

**Cross References:**
- `$API.MidiAutomationHandler.setAutomationDataFromObject$`
- `$API.MidiAutomationHandler.setUpdateCallback$`
- `$API.MacroHandler.getMacroDataObject$`

**Example:**
```javascript:inspect-automation-data
// Title: Inspect current MIDI automation mappings
const var mah = Engine.createMidiAutomationHandler();
var data = mah.getAutomationDataObject();

for (entry in data)
{
    Console.print("CC" + entry.Controller + " -> " + entry.Processor + "." + entry.Attribute);
}
```
```json:testMetadata:inspect-automation-data
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(data)", "value": true},
    {"type": "REPL", "expression": "data.length", "value": 0}
  ]
}
```

## setAutomationDataFromObject

**Signature:** `undefined setAutomationDataFromObject(Array automationData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Rebuilds all automation entries from ValueTree. Involves heap allocation, String construction, and change notification dispatch.
**Minimal Example:** `{obj}.setAutomationDataFromObject(data);`

**Description:**
Replaces all MIDI automation mappings with the entries from the given array. The array format matches the output of `getAutomationDataObject()` -- each element is a JSON object with properties like `Controller`, `Processor`, `Attribute`, `Start`, `End`, etc. This is the round-trip counterpart to `getAutomationDataObject()` and is the primary mechanism for programmatic automation configuration or restoring saved automation state.

Internally, the method converts the array to a ValueTree and calls `restoreFromValueTree()`, which clears all existing entries before adding the new ones. After restoring, a change notification fires (synchronous during preset loads, asynchronous otherwise), which triggers any registered update callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| automationData | Array | no | Array of automation entry objects matching the schema from `getAutomationDataObject()` | Must be an Array |

**Pitfalls:**
- [BUG] Passing a non-Array value (e.g., a single object, a number, or a string) silently clears all automation data instead of reporting an error. The internal converter produces an empty ValueTree, and `restoreFromValueTree` wipes all entries.

**Cross References:**
- `$API.MidiAutomationHandler.getAutomationDataObject$`
- `$API.MidiAutomationHandler.setUpdateCallback$`
- `$API.MacroHandler.setMacroDataFromObject$`

**Example:**
```javascript:roundtrip-automation-data
// Title: Save and restore automation data
const var mah = Engine.createMidiAutomationHandler();

// Snapshot current state
var saved = mah.getAutomationDataObject();
Console.print("Saved " + saved.length + " entries");

// Restore the saved state (clears existing, then adds saved entries)
mah.setAutomationDataFromObject(saved);
```
```json:testMetadata:roundtrip-automation-data
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Saved 0 entries"]},
    {"type": "REPL", "expression": "mah.getAutomationDataObject().length", "value": 0}
  ]
}
```

## setConsumeAutomatedControllers

**Signature:** `undefined setConsumeAutomatedControllers(Integer shouldBeConsumed)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setConsumeAutomatedControllers(false);`

**Description:**
Controls whether MIDI CC messages that match an automation entry are removed from the MIDI buffer before reaching script callbacks. When enabled (the default), automated CC messages are consumed and never arrive in `onController`. When disabled, CC messages pass through to script MIDI callbacks even after setting the automated parameter.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeConsumed | Integer | no | `true` (default) to consume automated CC messages, `false` to let them pass through to script callbacks | -- |

**Cross References:**
- `$API.MidiAutomationHandler.setAutomationDataFromObject$`

## setControllerNumberNames

**Signature:** `undefined setControllerNumberNames(String ccName, Array nameArray)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a StringArray from the input, involving String allocation.
**Minimal Example:** `{obj}.setControllerNumberNames("Controls", ["Mod Wheel", "Expression"]);`

**Description:**
Customizes the display names used in the right-click MIDI automation popup on UI components. The `ccName` parameter replaces the default "MIDI CC" label used as the popup section header (e.g., "Assign MIDI CC" becomes "Assign Controls"). The `nameArray` parameter provides custom names for individual CC numbers, replacing the default "CC#N" format. The name at index 0 in the array maps to CC#0, index 1 to CC#1, and so on. CC numbers beyond the array length fall back to the default "CC#N" format.

This method is typically used together with `setControllerNumbersInPopup()` to show a curated list of CC numbers with meaningful names.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| ccName | String | no | Category label for the popup section header. Replaces the default "MIDI CC" text. | -- |
| nameArray | Array | no | Array of display names for CC numbers. Index maps to CC number (index 0 = CC#0). | Elements are converted to strings via `toString()` |

**Cross References:**
- `$API.MidiAutomationHandler.setControllerNumbersInPopup$`

**Example:**
```javascript:custom-cc-names
// Title: Customize CC popup with named controllers
const var mah = Engine.createMidiAutomationHandler();

// Only show CC1 and CC11 in the popup
mah.setControllerNumbersInPopup([1, 11]);

// Set the section header and give them readable names
// Note: nameArray is indexed by CC number, so we need
// entries at index 1 and 11
var names = [];
names[1] = "Mod Wheel";
names[11] = "Expression";
mah.setControllerNumberNames("Performance", names);
```
```json:testMetadata:custom-cc-names
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "names.length", "value": 12},
    {"type": "REPL", "expression": "isDefined(names[1]) && isDefined(names[11])", "value": true},
    {"type": "REPL", "expression": "isDefined(names[0])", "value": false}
  ]
}
```

## setControllerNumbersInPopup

**Signature:** `undefined setControllerNumbersInPopup(Array numberArray)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a BigInteger bitmask from the array, which may involve heap allocation.
**Minimal Example:** `{obj}.setControllerNumbersInPopup([1, 7, 10, 11]);`

**Description:**
Restricts which CC numbers appear in the right-click MIDI automation popup on UI components. Pass an array of CC numbers (0-127) to show only those controllers. When a filter is set, the popup layout changes from a nested submenu with a "Learn" option to a flat list of the specified CC numbers directly.

Passing an empty array resets the filter, restoring the default behavior where all 128 CC numbers are available in a submenu layout.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numberArray | Array | no | Array of CC numbers to show in the popup. Each element is cast to int. | Values 0-127 |

**Cross References:**
- `$API.MidiAutomationHandler.setControllerNumberNames$`
- `$API.MidiAutomationHandler.setExclusiveMode$`

## setExclusiveMode

**Signature:** `undefined setExclusiveMode(Integer shouldBeExclusive)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setExclusiveMode(true);`

**Description:**
Enables or disables exclusive mode for MIDI CC automation. When exclusive mode is active, each CC number can only control one parameter at a time. Assigning a CC to a new parameter via MIDI learn first removes all existing automations for that CC number. The right-click popup also grays out CC numbers that already have an automation assigned.

When disabled (the default), multiple parameters can share the same CC number, and all receive the value simultaneously.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeExclusive | Integer | no | `true` to enable exclusive mode (one CC per parameter), `false` (default) to allow shared CC assignments | -- |

**Cross References:**
- `$API.MidiAutomationHandler.setControllerNumbersInPopup$`
- `$API.MacroHandler.setExclusiveMode$`

## setUpdateCallback

**Signature:** `undefined setUpdateCallback(Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a WeakCallbackHolder and invokes the callback synchronously once during registration.
**Minimal Example:** `{obj}.setUpdateCallback(onAutomationChanged);`

**Description:**
Registers a callback function that is invoked whenever the MIDI automation configuration changes. The callback receives one argument: the complete current automation data as an array of JSON objects (same format as `getAutomationDataObject()`). The callback fires when:

- A MIDI learn operation completes a new CC assignment
- An automation entry is removed
- Automation data is restored from a user preset
- All automation data is cleared

The callback is called immediately once during registration with the current automation state. During preset loads, the notification is synchronous; all other changes fire asynchronously on the message thread.

There is no mechanism to unregister the callback. Passing a non-function value is silently ignored, leaving the previous callback active.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Function receiving one argument (the automation data array) | Must be a valid function |

**Callback Signature:** callback(automationData: Array)

**Pitfalls:**
- [BUG] Passing a non-function value (e.g., `false`, a number) to clear the callback is silently ignored. The previously registered callback remains active. There is no way to unregister a callback once set.
- The callback fires immediately during registration. If the callback depends on state that is set up after the `setUpdateCallback` call, the initial invocation may produce unexpected results.
- Do not call `setAutomationDataFromObject()` from inside the update callback. Since `setAutomationDataFromObject` triggers the callback synchronously during preset loads, calling it from the callback creates infinite recursion.

**Cross References:**
- `$API.MidiAutomationHandler.getAutomationDataObject$`
- `$API.MidiAutomationHandler.setAutomationDataFromObject$`
- `$API.MacroHandler.setUpdateCallback$`

**Example:**
```javascript:automation-update-callback
// Title: Monitor automation changes with an update callback
const var mah = Engine.createMidiAutomationHandler();

var lastData = [];

inline function onAutomationChanged(data)
{
    lastData = data;
    Console.print("Automation changed: " + data.length + " entries");
};

// Registers and fires immediately with current state
mah.setUpdateCallback(onAutomationChanged);
```
```json:testMetadata:automation-update-callback
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Automation changed: 0 entries"]}
}
```
