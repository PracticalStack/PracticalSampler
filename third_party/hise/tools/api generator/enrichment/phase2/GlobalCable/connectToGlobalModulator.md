## connectToGlobalModulator

**Examples:**

```javascript:connecting-lfo-to-cable
// Title: Connecting a global LFO modulator to a cable
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var gmcIndex = builder.create(builder.SoundGenerators.GlobalModulatorContainer,
                              "GMC1", 0, builder.ChainIndexes.Direct);
// Chain index 1 is the "Global Modulators" slot in a GlobalModulatorContainer
builder.create(builder.Modulators.LFO, "TestLFO", gmcIndex, 1);
builder.flush();
// --- end setup ---

// Context: A GlobalModulatorContainer hosts shared modulators
// (LFOs, envelopes, velocity, etc.) whose output can be
// distributed to any part of the project via cables.

const var rm = Engine.getGlobalRoutingManager();
const var cable = rm.getCable("LfoMod");

// Connect the LFO to this cable. The modulator must be
// a child of a GlobalModulatorContainer.
cable.connectToGlobalModulator("TestLFO", true);

// --- test-only ---
reg v1 = 0.0;
reg v2 = 0.0;
// --- end test-only ---
```
```json:testMetadata:connecting-lfo-to-cable
{
  "testable": true,
  "verifyScript": [
    {
      "delay": 200,
      "expression": "v1 = cable.getValueNormalised() || true",
      "value": true
    },
    {
      "delay": 300,
      "expression": "v2 = cable.getValueNormalised() || true",
      "value": true
    },
    {
      "expression": "Math.abs(v1 - v2) > 0.01",
      "value": true
    }
  ]
}
```


**Cross References:**
- `Builder.create` -- used to construct the `GlobalModulatorContainer` and its child modulators before connecting them to cables.
