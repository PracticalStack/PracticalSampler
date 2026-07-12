## setAttributes

**Examples:**

```javascript:type-specific-attributes
// Title: Type-specific attribute configuration in a modulator loop
// Context: When creating multiple modulators from a definition table, use
// conditional logic to apply different attribute sets based on the module
// type. This pattern is common when building a GlobalModulatorContainer
// with diverse modulator types that each need specific defaults.

const var b = Synth.createBuilder();

// Modulator definitions: [type, name]
const var MODULATORS = [
    ["FlexAHDSR", "AHDSR1"],
    ["FlexAHDSR", "AHDSR2"],
    ["LFO", "LFO"],
    ["Velocity", "Velocity"],
    ["MidiController", "MIDI CC"],
    ["Random", "Random"]
];

var gmcIdx = b.create(b.SoundGenerators.GlobalModulatorContainer,
    "GlobalMods", 0, b.ChainIndexes.Direct);

for (m in MODULATORS)
{
    var idx = b.create(m[0], m[1], gmcIdx, b.ChainIndexes.Gain);

    if (m[0] == "FlexAHDSR")
    {
        b.setAttributes(idx, {
            "Attack": 10,
            "Decay": 300,
            "Sustain": 0.5,
            "Release": 300,
            "AttackCurve": 0.5,
            "DecayCurve": 0.5
        });
    }
    if (m[0] == "LFO")
    {
        b.setAttributes(idx, {
            "FadeIn": 10.0
        });
    }
    if (m[0] == "Velocity")
    {
        b.setAttributes(idx, {
            "UseTable": 1.0
        });
    }
}

b.flush();

reg gmcName = Synth.getChildSynth("GlobalMods").getId();
reg randName = Synth.getModulator("Random").getId();
```
```json:testMetadata:type-specific-attributes
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "gmcName", "value": "GlobalMods"},
    {"type": "REPL", "expression": "randName", "value": "Random"}
  ]
}
```
