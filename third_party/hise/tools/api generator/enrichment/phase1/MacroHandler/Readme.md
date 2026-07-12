# MacroHandler -- Class Analysis

## Brief
Programmatic read/write access to macro control connections with change notification callbacks.

## Purpose
MacroHandler provides scripting access to HISE's macro control infrastructure, allowing scripts to serialize, restore, and monitor macro-to-parameter connections. It wraps the C++ MacroControlBroadcaster system (which lives on the master ModulatorSynthChain) and the MacroManager singleton. The class supports a JSON-based round-trip workflow: read all macro connections as a JSON array, modify them, and write them back. A change callback notifies scripts when connections are modified through the UI or other code paths.

## Details

### JSON Schema

See `getMacroDataObject()` for the full JSON schema of macro connection objects. See `setMacroDataFromObject()` for the write-back workflow and additional input-only properties (`converter`).

### Range Handling

Two overlapping range sets are stored per connection:

- **Full range** (`FullStart`/`FullEnd`): The total parameter range -- determines the outer bounds of macro control.
- **Active range** (`Start`/`End`): The sub-range within the full range that the macro actually sweeps.

If only `Start`/`End` are provided (no `FullStart`/`FullEnd`), the active range is used as the full range. If both are provided and differ, the active range becomes a sub-range within the full range. The `Inverted` flag is taken from the active range.

### Exclusive Mode

When exclusive mode is enabled, each macro slot can only be connected to a single target parameter. Adding a new connection automatically removes the previous one. This setting is shared with the MacroManager singleton and persists for the session. See `setExclusiveMode()` for usage.

### Custom Automation Integration

Macro connections can target custom automation parameters defined through the UserPresetHandler. When `CustomAutomation` is `true` in the JSON object, the `Attribute` field is resolved via `UserPresetHandler.getCustomAutomationData()` instead of the processor's own parameter list.

### Macro Count Configuration

The number of active macro slots defaults to 8 (`HISE_NUM_MACROS`) but can be configured up to 64 via project preprocessor definitions. In the HISE IDE, this value is read from project extra definitions at runtime; in exported plugins, the compile-time value is used.

### Callback Behavior

See `setUpdateCallback()` for full callback registration and dispatch behavior. The callback receives the same data format as `getMacroDataObject()` output. Bulk operations via `setMacroDataFromObject()` coalesce notifications into a single callback.

## obtainedVia
`Engine.createMacroHandler()` -- creates and returns a MacroHandler instance.

## minimalObjectToken
mh

## Constants
(none)

## Dynamic Constants
(none)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Modifying the array from `getMacroDataObject` and expecting changes to apply | Call `setMacroDataFromObject(modifiedArray)` after modifying | `getMacroDataObject` returns a snapshot; modifications must be written back explicitly via `setMacroDataFromObject`. |

## codeExample
```javascript
// Create a MacroHandler and log connection changes
const var mh = Engine.createMacroHandler();

mh.setUpdateCallback(function(macroData)
{
    Console.print("Macro connections changed: " + macroData.length + " connections");
});
```

## Alternatives
- **UserPresetHandler** -- manages the full preset model including macro connections as part of preset save/load; MacroHandler provides direct programmatic access to macro connections independently of presets.
- **MidiAutomationHandler** -- maps MIDI CC messages to parameters with a similar get/set data pattern; MacroHandler maps macro knob indices to parameters.

## Related Preprocessors
`HISE_NUM_MACROS`, `HISE_NUM_MAX_MACROS`, `USE_BACKEND`

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: MacroHandler has only 4 methods with straightforward inputs. Invalid MacroIndex or Processor values produce runtime script errors, not silent failures. No parse-time validation opportunities exist.
