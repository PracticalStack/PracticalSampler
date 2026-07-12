<!-- Diagram triage:
  - No class-level or method-level diagrams specified in the JSON.
-->

# MacroHandler

MacroHandler gives scripts programmatic control over macro-to-parameter connections. It exposes a JSON-based round-trip workflow: read the current connections as an array of objects, modify the array, and write it back to apply changes. A change callback notifies your script whenever connections are modified - whether by your code or through the HISE IDE.

```js
const var mh = Engine.createMacroHandler();
```

Typical workflows range from simple monitoring (registering a callback to update UI indicators when macros change) to full connection management (building macro-to-parameter mappings programmatically for DAW automation or NKS integration). For custom automation workflows, macro connections can target automation parameters defined through `UserPresetHandler.setCustomAutomation()` by setting `CustomAutomation` to `true` in the connection object.

Each connection object contains these properties:

| Property | Type | Description |
|----------|------|-------------|
| `MacroIndex` | int | Macro slot index (0 to max macros - 1) |
| `Processor` | String | Target processor ID |
| `Attribute` | String | Parameter name (or custom automation ID) |
| `CustomAutomation` | bool | Whether this targets a custom automation slot |
| `FullStart` / `FullEnd` | double | Total parameter range bounds |
| `Start` / `End` | double | Active sub-range the macro sweeps |
| `Interval` | double | Step size |
| `Skew` | double | Skew factor for non-linear mapping |
| `Inverted` | bool | Whether the range mapping is inverted |

The active range (`Start`/`End`) sits within the full range (`FullStart`/`FullEnd`). If you only provide `Start`/`End`, they are used as both the full and active range.

> [!Tip:Up to 64 macro slots, create once at init] The number of available macro slots defaults to 8 but can be configured up to 64 via the `HISE_NUM_MACROS` project preprocessor definition. Store one MacroHandler instance at init time and reuse it - each call to `Engine.createMacroHandler()` registers a new internal listener.

## Common Mistakes

- **Write back snapshot with setMacroDataFromObject**
  **Wrong:** Modifying the array returned by `getMacroDataObject()` and expecting the changes to take effect.
  **Right:** Call `setMacroDataFromObject(modifiedArray)` after modifying the array.
  *`getMacroDataObject()` returns a snapshot. Changes to the array have no effect until you write it back.*

- **Create one MacroHandler instance at init**
  **Wrong:** Creating a new MacroHandler for each UI interaction.
  **Right:** Store one instance with `const var mh = Engine.createMacroHandler()` at init and reuse it.
  *Each call registers a new listener on the macro system. Multiple instances waste resources and produce duplicate notifications.*

- **Use Broadcaster for multi-consumer updates**
  **Wrong:** Passing an inline function to `setUpdateCallback()` and trying to reach multiple UI systems from it.
  **Right:** Pass a Broadcaster as the callback, then add independent listeners to the broadcaster.
  *A single callback function creates tight coupling. A broadcaster fans out notifications to multiple listeners without the callback needing to know about all consumers.*
