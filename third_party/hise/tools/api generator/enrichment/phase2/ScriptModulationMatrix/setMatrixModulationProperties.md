## setMatrixModulationProperties

**Examples:**

```javascript:per-target-defaults-and-ranges
// Title: Configure per-target defaults and range presets for a synthesizer
// Context: A synth with multiple modulation target types (gain, pitch, filter,
// pan) needs different default intensities, modes, and display ranges for each.

const var mm = Engine.createModulationMatrix("Global Modulator Container0");

mm.setMatrixModulationProperties({
    "DefaultInitValues": {
        "Gain": {
            "Intensity": 1.0,
            "Mode": "Scale",
            "IsNormalized": true
        },
        "Pitch": {
            "Intensity": 0.0,
            "Mode": "Bipolar"
        },
        "Filter Frequency": {
            "Intensity": 0.5,
            "Mode": "Unipolar"
        },
        "Pan": {
            "Intensity": 0.0,
            "Mode": "Bipolar"
        }
    },
    "RangeProperties": {
        "Gain": "Gain0dB",
        "Pitch": "Pitch1Octave",
        "Pan": "Stereo",
        "Filter Frequency": {
            "InputRange": {"MinValue": 20.0, "MaxValue": 20000.0, "MiddlePosition": 2000.0},
            "OutputRange": {"MinValue": 0.0, "MaxValue": 1.0, "MiddlePosition": 0.1},
            "mode": "Frequency"
        }
    }
});

// Verify the stored properties
trace(mm.getMatrixModulationProperties());
```

```json:testMetadata:per-target-defaults-and-ranges
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with Gain, Pitch, Pan, and Filter Frequency targets in the module tree"
}
```

**Pitfalls:**
- The `RangeProperties` per-target value can be either a preset name string (e.g., `"FilterFreq"`) or a full JSON object with `InputRange`, `OutputRange`, and `mode` keys. Mixing these two formats in the same call is valid -- use presets for standard ranges and custom objects for non-standard parameters.
