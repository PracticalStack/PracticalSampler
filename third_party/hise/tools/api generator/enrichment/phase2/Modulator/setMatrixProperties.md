## setMatrixProperties

**Examples:**

```javascript:configure-matrix-range-data
// Title: Configure a MatrixModulator's range data after Builder creation
// Context: When the Builder API creates MatrixModulators for the built-in
// ModulationMatrix, each modulator needs range data that defines its
// modulation target properties (chain index, min/max, skew, etc.).

const var builder = Synth.createBuilder();

const var TARGETS = {
    "Gain": { "Index": 1 },
    "Pitch": { "Index": 2 },
    "Fine": { "Index": 2 }
};

// Create a MatrixModulator and configure its range properties
const var matrixModule = builder.create(
    builder.Modulators.MatrixModulator, "GainMod", oscGroup, gainChainIndex
);

const var mod = builder.get(matrixModule, builder.InterfaceTypes.Modulator);

// setMatrixProperties passes range data to the GlobalModulatorContainer's
// matrix properties system for this modulator
mod.setMatrixProperties(TARGETS["Gain"]);
```
```json:testMetadata:configure-matrix-range-data
{
  "testable": false,
  "skipReason": "Requires Builder API context with an oscillator group and GlobalModulatorContainer"
}
```

**Pitfalls:**
- [BUG] Only functional on `MatrixModulator` instances. Calling on any other modulator type silently does nothing -- no error is reported, so the call appears to succeed.
