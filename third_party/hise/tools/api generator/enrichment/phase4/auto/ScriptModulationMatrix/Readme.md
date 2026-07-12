<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# ScriptModulationMatrix

ScriptModulationMatrix provides scriptable access to HISE's modulation matrix system, enabling dynamic many-to-many routing between global modulator sources and modulation targets. Sources are modulators hosted in a GlobalModulatorContainer; targets are either MatrixModulator processors in the module tree or ScriptSlider components with a `matrixTargetId` property.

The matrix handles connection state, user preset integration, and modulation display data automatically. At its simplest, you only need the object reference and the built-in FloatingTile panels for a complete drag-and-drop modulation UI. For advanced use cases, register callbacks to build fully custom interaction layers with context menus, drag visualisation, and source selection tracking.

```javascript
const mm = Engine.createModulationMatrix("Global Modulator Container0");
```

To set up the modulation matrix system:

1. Create a global modulator container and add modulation sources to its gain modulation chain.
2. Add MatrixModulator processors to every target chain you want to modulate.
3. Add ModulationMatrix and ModulationMatrixController FloatingTile panels for the UI.
4. Create the script object in `onInit` with `Engine.createModulationMatrix()` and register any callbacks.

Each connection carries these properties, accessible via `getConnectionProperty` and `setConnectionProperty`:

| Property | Type | Description |
|----------|------|-------------|
| `Intensity` | double | Modulation depth in the range [-1.0, 1.0] |
| `Mode` | int | 0 = Scale, 1 = Unipolar, 2 = Bipolar |
| `Inverted` | bool | Invert the modulation signal |
| `AuxIndex` | int | Secondary source index (-1 = none) |
| `AuxIntensity` | double | Secondary source modulation depth |

> This class was completely redesigned in HISE 5.0 with no backwards compatibility to the previous API. If you used an earlier version, you will need to rewrite your modulation logic.

> Mutating operations (`connect`, `clearAllConnections`, `fromBase64`) kill all voices and execute on the scripting thread to avoid audio glitches. These operations, along with property changes via `setConnectionProperty`, are fully undoable with `Engine.undo()`.

> The matrix automatically participates in user preset save/load. Connection state is stored under the `MatrixData` key in the preset. When a preset lacks matrix data, all connections are cleared.

## Common Mistakes

- **Wrong:** `Engine.createModulationMatrix("MySynth")`
  **Right:** `Engine.createModulationMatrix("Global Modulator Container0")`
  *The argument must be the ID of a GlobalModulatorContainer processor, not any other module type.*

- **Wrong:** `mm.setMatrixModulationProperties({"DefaultInitValues": {"target1": {"Intensity": 0.5}}})`
  **Right:** `mm.setMatrixModulationProperties({"DefaultInitValues": {"target1": {"Intensity": 0.5, "Mode": "Unipolar"}}})`
  *A non-zero Intensity without a Mode triggers a script error because the system cannot determine how to apply the modulation.*

- **Wrong:** Calling `mm.setCurrentlySelectedSource("LFO1")` without enabling selectable sources.
  **Right:** Call `mm.setMatrixModulationProperties({"SelectableSources": true})` or register a source selection callback first.
  *Throws a script error if selectable sources mode is not active.*

- **Wrong:** Creating the matrix object but never placing FloatingTile panels or registering callbacks.
  **Right:** Use ModulationMatrix / ModulationMatrixController FloatingTile panels, or register drag and connection callbacks for a custom UI.
  *The matrix object alone provides no user-facing interaction. Without panels or callbacks, users have no way to create or manage connections.*
