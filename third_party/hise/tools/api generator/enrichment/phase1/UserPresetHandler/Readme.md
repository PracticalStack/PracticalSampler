# UserPresetHandler -- Class Analysis

## Brief
Manages user preset load/save lifecycle, custom automation slots, and host parameter integration.

## Purpose
UserPresetHandler provides comprehensive control over user preset management in HISE. It supports two data models: a default mode that serializes component values automatically, and a custom mode that delegates save/load to script callbacks. The class also manages custom automation slots that expose named parameters to DAW hosts and MIDI controllers, with polymorphic connections to processors, meta-parameters, and global cables. Pre/post callbacks enable preset migration, versioning, and UI synchronization during the load lifecycle.

## Details

### Data Models

UserPresetHandler supports two mutually exclusive data models:

| Model | Activation | Save/Load Mechanism |
|-------|-----------|---------------------|
| Default | Automatic (no setup needed) | Serializes all components with `saveInPreset` flag as ValueTree |
| Custom | `setUseCustomUserPresetModel(loadCb, saveCb, persistent)` | Script callbacks receive/return JSON data |

The custom data model must be enabled before calling `setCustomAutomation`.

### Preset Load Sequence

The preset load follows a multi-phase sequence:

1. **Pre-callback** (synchronous, main thread) -- can inspect/modify preset data before load
2. **Kill voices and enter loading thread**
3. **Restore macro connections** (routing first, values later)
4. **Restore content/custom data** (default model restores components; custom model calls load callback)
5. **Restore module states** (Modules state manager)
6. **Restore MIDI automation** (MidiAutomation state manager)
7. **Restore MPE data**
8. **Restore macro values** (after all connections are loaded)
9. **Restore additional states** (any non-standard state managers)
10. **Post-callback** (asynchronous, message thread) -- for UI updates after load completes

### Custom Automation Schema

`setCustomAutomation` accepts an array of automation slot definitions. Each slot is a JSON object:

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `ID` | String | (required) | Unique automation identifier |
| `min` | float | 0.0 | Range minimum |
| `max` | float | 1.0 | Range maximum |
| `middlePosition` | float | -- | If set, applies skew so this value is at range center |
| `stepSize` | float | 0.0 | Quantization step size |
| `defaultValue` | float | range start | Default value (clamped to range) |
| `allowMidiAutomation` | bool | true | Whether MIDI CC can control this slot |
| `allowHostAutomation` | bool | true | Whether DAW host can control this slot |
| `pluginParameterGroup` | String | "" | Plugin parameter group (must be registered first) |
| `connections` | Array | (required) | Array of connection target objects |
| `mode` | String | -- | Value-to-text converter mode |
| `options` | Array | -- | String array of discrete option labels |
| `suffix` | String | "" | Suffix for numeric text display |

### Connection Types

Each entry in the `connections` array uses exactly one identification pattern:

| Type | JSON Pattern | Behavior |
|------|-------------|----------|
| ProcessorConnection | `{"processorId": "...", "parameterId": "..."}` | Directly sets a module parameter |
| MetaConnection | `{"automationId": "..."}` | Routes to another automation slot (meta-parameter) |
| CableConnection | `{"cableId": "..."}` | Bidirectional link to a global routing cable |

MetaConnection targets must appear earlier in the automation array than the slot that references them.

### ValueToTextConverter Modes

The `mode` property in automation data controls DAW parameter display:

| Mode | Display Format | Example |
|------|---------------|---------|
| `"Frequency"` | Hz/kHz | "440 Hz", "1.2 kHz" |
| `"Time"` | ms/s | "100ms", "1.5s" |
| `"TempoSync"` | Tempo names | "1/4", "1/8T" |
| `"Pan"` | L/C/R | "50L", "C", "30R" |
| `"NormalizedPercentage"` | 0-100% | "50%" |
| `"Decibel"` | dB | "-6.0 dB" |
| `"Semitones"` | st | "+2 st", "-12 st" |

Alternatively, pass an `options` array for discrete choices. If neither `mode` nor `options` is set, the display uses `stepSize` and `suffix`.

### Preprocessing System

When `setEnableUserPresetPreprocessing(true, ...)` is enabled, the pre-callback receives a JSON representation of the preset instead of a File object. The JSON structure contains:

```
{
  "version": "1.0.0",
  "Content": [ { "id": "...", "value": ... }, ... ],
  "Modules": { ... },
  "MidiAutomation": { ... },
  "MPEData": { ... }
}
```

With `shouldUnpackComplexData=true`, JSON-encoded and Base64-encoded values within the preset are decoded back to their original form. After the pre-callback modifies the JSON, it is converted back to a ValueTree for loading.

### Parameter Gesture Types

The `sendParameterGesture` method's `automationType` parameter uses these values:

| Value | Type | Description |
|-------|------|-------------|
| 0 | Macro | Macro parameter |
| 1 | CustomAutomation | Custom automation slot |
| 2 | ScriptControl | Script UI control |
| 3 | NKSWrapper | NKS integration |

### Plugin Parameter Sort Callback

The `setPluginParameterSortFunction` callback receives two objects, each with:

| Property | Type | Description |
|----------|------|-------------|
| `type` | int | Automation type (see gesture types above) |
| `parameterIndex` | int | HISE parameter index |
| `typeIndex` | int | Slot index within type |
| `name` | String | Parameter name |
| `group` | String | Plugin parameter group name |

Return negative (first before second), zero (equal), or positive (second before first).

### Undo Integration

Both preset loads and automation value changes can optionally participate in the undo system:

- `setUseUndoForPresetLoading(true)` -- wraps preset loads in UndoableAction; consecutive loads coalesce
- `updateAutomationValues(data, msg, true)` -- wraps value changes in UndoableAction

Both integrate with `Engine.undo()`.

### State Manager Composition

Presets are composed of independently managed state segments:

| State ID | Manager | Content |
|----------|---------|---------|
| Content | (built-in) | Component values from saveInPreset components |
| CustomJSON | CustomStateManager | Custom data model JSON |
| Modules | ModuleStateManager | Module parameter states |
| MidiAutomation | MidiControllerAutomationHandler | MIDI CC mappings |
| MPEData | (built-in) | MPE configuration |
| AdditionalStates | (custom) | Any user-registered state managers |

## obtainedVia
`Engine.createUserPresetHandler()` -- creates and returns a new UserPresetHandler instance.

## minimalObjectToken
uph

## Constants
(none)

## Dynamic Constants
(none)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `uph.setCustomAutomation(data)` without custom model | `uph.setUseCustomUserPresetModel(load, save, false); uph.setCustomAutomation(data);` | Custom automation requires the custom data model to be enabled first; calling setCustomAutomation without it throws a script error. |
| `uph.isInternalPresetLoad()` outside callbacks | `uph.setPreCallback(function(f){ if(uph.isInternalPresetLoad()) ... })` | isInternalPresetLoad only returns meaningful results during pre/post callbacks; outside those callbacks the flag is undefined. |

## codeExample
```javascript
// Create a UserPresetHandler and set up basic callbacks
const var uph = Engine.createUserPresetHandler();

uph.setPreCallback(function(presetFile)
{
    // Called synchronously before preset load
    Console.print("Loading: " + presetFile.toString(""));
});

uph.setPostCallback(function(presetFile)
{
    // Called asynchronously after preset load completes
    Console.print("Loaded: " + presetFile.toString(""));
});
```

## Alternatives
- **MacroHandler** -- manages macro control connections; UserPresetHandler manages the full preset and host automation model that macros feed into.
- **MidiAutomationHandler** -- maps MIDI CC to parameters; UserPresetHandler manages the broader preset model including custom automation slots.

## Related Preprocessors
`USE_BACKEND`, `USE_FRONTEND`, `USE_RAW_FRONTEND`, `READ_ONLY_FACTORY_PRESETS`, `HISE_MACROS_ARE_PLUGIN_PARAMETERS`, `USE_NEW_AUTOMATION_DISPATCH`

## Diagrams

### preset-load-sequence
- **Brief:** Preset Load Lifecycle
- **Type:** topology
- **Description:** Shows the preset load sequence: Pre-callback (synchronous, main thread) -> Kill voices -> Loading thread entry -> Macro connections restored -> Content/Custom data restored -> Module states restored -> MIDI automation restored -> MPE data restored -> Macro values restored -> Additional states restored -> Post-callback (asynchronous, message thread). The pre-callback branch shows the two paths: default (receives File) vs preprocessing enabled (receives JSON object).

### automation-connection-types
- **Brief:** Custom Automation Connection Types
- **Type:** topology
- **Description:** Shows a CustomAutomationData slot at the center, with three outgoing connection types: ProcessorConnection (targets a module parameter via setAttribute), MetaConnection (targets another automation slot, enabling meta-parameter routing), and CableConnection (bidirectional link to a global routing cable with 0-1 normalization). Incoming sources include DAW host parameters, MIDI CC (via MidiControllerAutomationHandler), script calls (setAutomationValue), and AttachedCallback listeners.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: UserPresetHandler methods either throw explicit script errors on misconfiguration (e.g., setCustomAutomation without custom model) or have runtime-only dependencies (callback timing, thread context) that cannot be validated at parse time.
