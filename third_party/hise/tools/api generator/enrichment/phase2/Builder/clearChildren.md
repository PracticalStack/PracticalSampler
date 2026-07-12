## clearChildren

**Examples:**

```javascript:clear-modulation-chain
// Title: Clearing a specific modulation chain before rebuilding
// Context: When rebuilding only part of the module tree (e.g. the gain
// modulators of an oscillator group), use clearChildren() to remove
// existing children from a specific chain without affecting the rest.

const var b = Synth.createBuilder();

inline function buildOscillator(index)
{
    local group = b.create(b.SoundGenerators.SynthGroup,
        "OSC" + index, 0, b.ChainIndexes.Direct);

    // Clear existing gain modulators before adding new ones
    b.clearChildren(group, b.ChainIndexes.Gain);

    local synth = b.create(b.SoundGenerators.WavetableSynth,
        "WT" + index, group, b.ChainIndexes.Direct);

    // Add MatrixModulators to the now-empty gain chain
    b.create(b.Modulators.MatrixModulator,
        "OSC" + index + " Gain", group, b.ChainIndexes.Gain);
    b.create(b.Modulators.MatrixModulator,
        "OSC" + index + " Pan", group, b.ChainIndexes.Gain);
}

b.clear();
buildOscillator(1);
b.flush();

Console.print(Synth.getChildSynth("OSC1").getId());
Console.print(Synth.getModulator("OSC1 Gain").getId());
```
```json:testMetadata:clear-modulation-chain
{
  "testable": true,
  "verifyScript": {"type": "log-output", "values": ["OSC1", "OSC1 Gain"]}
}
```
