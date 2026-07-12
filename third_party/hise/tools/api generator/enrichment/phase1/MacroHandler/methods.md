# MacroHandler -- Method Analysis

## getMacroDataObject

**Signature:** `Array getMacroDataObject()`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates Array and DynamicObject per connection; constructs Strings for processor IDs and parameter names.
**Minimal Example:** `var data = {obj}.getMacroDataObject();`

**Description:**
Returns an array of JSON objects representing all active macro-to-parameter connections across all macro slots. Each object describes one parameter connection within a macro slot, including the macro index, target processor, parameter name, range properties, and custom automation status. The number of macro slots is determined by the `HISE_NUM_MACROS` preprocessor value (default 8, configurable up to 64).

**Parameters:**

(none)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| MacroIndex | int | Macro slot index (0 to HISE_NUM_MACROS-1) |
| Processor | String | Processor ID of the connected module |
| Attribute | String | Parameter name on the processor (or custom automation ID if CustomAutomation is true) |
| CustomAutomation | bool | True if this targets a custom automation slot (only present when true) |
| FullStart | double | Total parameter range start |
| FullEnd | double | Total parameter range end |
| Start | double | Active sub-range start |
| End | double | Active sub-range end |
| Interval | double | Step size |
| Skew | double | Skew factor for non-linear mapping |
| Inverted | bool | Whether the range mapping is inverted |

**Pitfalls:**
- [BUG] When a macro slot has multiple connected parameters, the range properties (`Start`, `End`, `FullStart`, `FullEnd`, `Interval`, `Skew`, `Inverted`) and potentially the `Attribute` and `CustomAutomation` fields on each returned object reflect the LAST parameter in the macro slot rather than the specific parameter the object represents. The internal `getCallbackArg` method iterates all parameters in the macro slot and overwrites range/attribute properties on a single object, so only the last iteration's values survive.
- The returned array is a snapshot. Modifying objects in the array does not change the actual macro connections -- call `setMacroDataFromObject` with the modified array to apply changes.

**Cross References:**
- `$API.MacroHandler.setMacroDataFromObject$`
- `$API.MacroHandler.setUpdateCallback$`

**Example:**
```javascript:macro-data-snapshot
// Title: Reading and inspecting macro connection data
const var mh = Engine.createMacroHandler();
var data = mh.getMacroDataObject();

for (item in data)
{
    Console.print("Macro " + item.MacroIndex + ": " + item.Processor + "." + item.Attribute);
}
```
```json:testMetadata:macro-data-snapshot
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(data)", "value": true},
    {"type": "REPL", "expression": "data.length", "value": 0}
  ]
}
```

## setExclusiveMode

**Signature:** `undefined setExclusiveMode(Integer shouldBeExclusive)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setExclusiveMode(true);`

**Description:**
Enables or disables exclusive mode for macro connections. When exclusive mode is enabled, each macro slot can only be connected to a single target parameter -- adding a new connection automatically removes the previous one. This setting is shared with the global MacroManager and persists for the session. By default, exclusive mode is disabled, allowing multiple parameters per macro slot.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeExclusive | Integer | no | Whether each macro slot should allow only one connection | Boolean: true to enable, false to disable |

**Cross References:**
- `$API.MacroHandler.getMacroDataObject$`
- `$API.MacroHandler.setMacroDataFromObject$`

## setMacroDataFromObject

**Signature:** `undefined setMacroDataFromObject(Array jsonData)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Clears and rebuilds all macro connections. Allocates Strings, modifies processor state, sends connection change notifications.
**Minimal Example:** `{obj}.setMacroDataFromObject(macroData);`

**Description:**
Clears all existing macro connections and rebuilds them from the provided JSON array. Each element in the array must be an object matching the schema returned by `getMacroDataObject`. The method validates that each object contains `MacroIndex`, `Processor`, and `Attribute` properties, reporting a script error if any are missing. Individual connection notifications are suppressed during the rebuild -- the update callback fires once after all connections are restored.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| jsonData | Array | no | Array of macro connection objects | Each element must have MacroIndex, Processor, and Attribute properties |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| MacroIndex | int | Macro slot index (0 to HISE_NUM_MACROS-1) |
| Processor | String | Processor ID of the target module |
| Attribute | String/int | Parameter name or index (resolved by processor or custom automation) |
| CustomAutomation | bool | True to resolve Attribute via UserPresetHandler custom automation |
| FullStart | double | Total parameter range start |
| FullEnd | double | Total parameter range end |
| Start | double | Active sub-range start |
| End | double | Active sub-range end |
| Interval | double | Step size |
| Skew | double | Skew factor for non-linear mapping |
| Inverted | bool | Whether the range mapping is inverted |
| converter | String | ValueToTextConverter mode string for display formatting (input-only, not returned by getMacroDataObject) |

**Pitfalls:**
- [BUG] Silently does nothing if `jsonData` is not an Array. No error is reported -- the method returns without modifying any connections or notifying callbacks.
- All existing macro connections are cleared before the new ones are applied. There is no additive merge -- passing a partial list removes connections not in the array.

**Cross References:**
- `$API.MacroHandler.getMacroDataObject$`
- `$API.MacroHandler.setExclusiveMode$`
- `$API.MacroHandler.setUpdateCallback$`

**Example:**
```javascript:macro-roundtrip
// Title: Save and restore macro connections
const var mh = Engine.createMacroHandler();

// Snapshot current connections (returns a copy, not a live reference)
var savedData = mh.getMacroDataObject();

// Clear all connections
mh.setMacroDataFromObject([]);

// Restore from the saved snapshot
mh.setMacroDataFromObject(savedData);

var restored = mh.getMacroDataObject();
```
```json:testMetadata:macro-roundtrip
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Array.isArray(savedData)", "value": true},
    {"type": "REPL", "expression": "restored.length", "value": 0}
  ]
}
```

## setUpdateCallback

**Signature:** `undefined setUpdateCallback(Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates WeakCallbackHolder, increments reference count, and immediately fires the callback synchronously with the full macro data object (which allocates).
**Minimal Example:** `{obj}.setUpdateCallback(onMacroUpdate);`

**Description:**
Registers a callback function that fires whenever a macro connection changes. The callback receives the full macro data array (same format as `getMacroDataObject` output) as its single argument. On registration, the callback fires immediately with the current state (synchronous dispatch). Subsequent updates from UI or C++ code fire asynchronously via JUCE's AsyncUpdater (coalesced to the message thread). Only one callback can be active at a time -- calling `setUpdateCallback` again replaces the previous callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | no | Callback invoked on macro connection changes | Must be a JavaScript function |

**Callback Signature:** callback(macroData: Array)

**Pitfalls:**
- [BUG] Silently ignored if the argument is not a valid JavaScript function. No error is reported -- the method returns without modifying the callback.
- The callback fires immediately on registration with the current macro state. Code that assumes the callback only fires on future changes will execute the callback body unexpectedly during `onInit`.
- [BUG] There is no way to clear the callback. Passing `false` or a non-function value is silently ignored but does not unregister the previous callback. The callback remains active until the MacroHandler object is garbage collected.

**Cross References:**
- `$API.MacroHandler.getMacroDataObject$`
- `$API.MacroHandler.setMacroDataFromObject$`

**Example:**
```javascript:macro-update-callback
// Title: Registering a macro connection change callback
const var mh = Engine.createMacroHandler();

inline function onMacroUpdate(macroData)
{
    Console.print("Connections: " + macroData.length);
};

mh.setUpdateCallback(onMacroUpdate);
```
```json:testMetadata:macro-update-callback
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["Connections: 0"]}
}
```
