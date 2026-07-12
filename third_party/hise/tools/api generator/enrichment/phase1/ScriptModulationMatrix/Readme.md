# ScriptModulationMatrix -- Class Analysis

## Brief
Dynamic many-to-many modulation routing between global modulator sources and UI/processor targets.

## Purpose
ScriptModulationMatrix provides a scriptable interface to HISE's modulation matrix system, enabling dynamic many-to-many connections between global modulator sources (hosted in a GlobalModulatorContainer) and modulation targets (MatrixModulator processors or ScriptSliders with a matrixTargetId property). It manages connection state as a ValueTree that integrates with the user preset system, supports per-connection properties (intensity, mode, inversion, aux source), and provides callbacks for connection changes, drag interaction, source selection, and custom edit menus.

## Details

### Architecture

The matrix operates on a `GlobalModulatorContainer` processor specified at creation time. Sources are the modulators in the container's gain modulation chain, identified by processor ID. Targets are discovered from two paths:

1. **MatrixModulator processors** in the module tree -- envelope modulators with a special Value parameter, identified by their processor ID or custom target ID.
2. **ScriptSlider components** with a non-empty `matrixTargetId` property -- these become parameter-level targets where modulation is applied via global routing cables.

### Connection Data Model

All connections are stored as children of a `MatrixData` ValueTree. Each connection has these properties (see `getConnectionProperty`/`setConnectionProperty` for the full access API):

| Property | Type | Description |
|----------|------|-------------|
| SourceIndex | int | Index into source list (-1 = disconnected) |
| TargetId | String | Target identifier |
| Intensity | double | Modulation depth [-1.0, 1.0] |
| Mode | int | 0 = Scale (gain), 1 = Unipolar (add), 2 = Bipolar (add/subtract) |
| Inverted | bool | Invert modulation signal |
| AuxIndex | int | Secondary source index (-1 = none) |
| AuxIntensity | double | Secondary source modulation depth |

### Modulation Modes

The `Mode` connection property (set via `setConnectionProperty`) controls how modulation is applied:

- **Scale (0):** Multiplies the target value by `(1 - intensity) + intensity * modValue`. Classic HISE intensity-scale behavior.
- **Unipolar (1):** Adds `intensity * modValue` to the target value.
- **Bipolar (2):** Adds `intensity * (modValue * 2 - 1)` to the target value, centering modulation around zero.

### Global Properties

The properties JSON passed to `setMatrixModulationProperties` configures three aspects of the matrix (see that method for full parameter documentation):

```javascript
{
  "SelectableSources": false,       // enable exclusive source selection
  "DefaultInitValues": {            // per-target default connection values
    "targetId": {
      "Intensity": 0.5,
      "IsNormalized": true,
      "Mode": "Scale"               // "Scale", "Unipolar", or "Bipolar"
    }
  },
  "RangeProperties": {              // per-target range/display config
    "targetId": "FilterFreq"        // preset name string, or full object
  }
}
```

Available range presets: `NormalizedPercentage`, `Gain0dB`, `Gain6dB`, `Pitch1Octave`, `Pitch2Octaves`, `Pitch1Semitone`, `PitchOctaveStep`, `PitchSemitoneStep`, `FilterFreq`, `FilterFreqLog`, `Stereo`.

### Drag Interaction

The drag callback (registered via `setDragCallback`) receives three arguments: `sourceId` (String), `targetId` (String), and `action` (String). See `setDragCallback` for the full list of action values and their meanings.

### User Preset Integration

The matrix automatically participates in user preset save/load via the `UserPresetStateManager` interface. Connection state is stored under the `MatrixData` key in the preset ValueTree. When a preset lacks matrix data, all connections are cleared.

### Threading

Mutating operations (`connect`, `fromBase64`, `clearAllConnections`) use `killVoicesAndCall` to safely suspend audio before modifying the connection tree. Property changes via `setConnectionProperty` use the undo manager directly. The loading thread is an exception -- mutations execute synchronously when called from the sample loading thread.

## obtainedVia
`Engine.createModulationMatrix(containerId)` -- containerId must be the processor ID of a GlobalModulatorContainer.

## minimalObjectToken
mm

## Constants
(none)

## Dynamic Constants
(none)

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Engine.createModulationMatrix("MySynth")` | `Engine.createModulationMatrix("Global Modulator Container0")` | The containerId must reference a GlobalModulatorContainer processor, not any other synth type. Passing a wrong ID causes a script error. |
| `mm.setCurrentlySelectedSource("LFO1")` without configuring selectable sources | Call `mm.setMatrixModulationProperties({"SelectableSources": true})` first | Calling setCurrentlySelectedSource when SelectableSources is not enabled triggers a script error. |
| `mm.setMatrixModulationProperties({"DefaultInitValues": {"target1": {"Intensity": 0.5}}})` | Include `"Mode": "Unipolar"` in the init value | Setting a non-zero Intensity without specifying a Mode triggers a script error because the system cannot determine how to apply the modulation. |

## codeExample
```javascript
// Create a modulation matrix connected to the global modulator container
const mm = Engine.createModulationMatrix("Global Modulator Container0");

// Set up a connection callback
mm.setConnectionCallback(function(sourceId, targetId, wasAdded)
{
    Console.print(sourceId + " -> " + targetId + (wasAdded ? " added" : " removed"));
});
```

## Alternatives
- **Modulator** -- Direct handle to a single fixed modulator for parameter control, without dynamic many-to-many routing.
- **GlobalRoutingManager** -- General-purpose signal/data routing via named cables, without modulation-specific features like intensity, modes, or display data.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- setCurrentlySelectedSource -- precondition (logged)
