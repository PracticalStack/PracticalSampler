ScriptModulationMatrix::setMatrixModulationProperties(JSON newProperties) -> undefined

Thread safety: UNSAFE -- parses JSON and modifies container state.
Configures global properties for the modulation matrix. The JSON object can
contain three sections: SelectableSources (boolean), DefaultInitValues
(per-target defaults), and RangeProperties (per-target range/display config).
Throws a script error if a DefaultInitValues entry has non-zero Intensity but
no Mode property.

JSON schema:
  {
    "SelectableSources": false,
    "DefaultInitValues": {
      "targetId": {
        "Intensity": 0.5,
        "IsNormalized": true,
        "Mode": "Scale"            // "Scale", "Unipolar", or "Bipolar"
      }
    },
    "RangeProperties": {
      "targetId": "FilterFreq",    // preset name string, or full object:
      "other": {
        "InputRange": {"MinValue": 0.0, "MaxValue": 1.0},
        "OutputRange": {"MinValue": 0.0, "MaxValue": 1.0},
        "mode": "NormalizedPercentage",
        "UseMidPositionAsZero": false
      }
    }
  }

Range presets: NormalizedPercentage, Gain0dB, Gain6dB, Pitch1Octave,
  Pitch2Octaves, Pitch1Semitone, PitchOctaveStep, PitchSemitoneStep,
  FilterFreq, FilterFreqLog, Stereo

Required setup:
  const var mm = Engine.createModulationMatrix("Global Modulator Container0");

Pair with:
  getMatrixModulationProperties -- read back current configuration
  connect -- new connections use DefaultInitValues from this configuration

Anti-patterns:
  - Do NOT set DefaultInitValues with non-zero Intensity but no Mode -- throws
    a script error.

Source:
  ScriptModulationMatrix.cpp  setMatrixModulationProperties()
    -> parses JSON sections -> container->matrixProperties
    -> DefaultInitValues: validates Intensity+Mode pairing
    -> RangeProperties: resolves preset names or parses full range objects
