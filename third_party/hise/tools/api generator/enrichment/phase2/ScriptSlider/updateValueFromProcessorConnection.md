## updateValueFromProcessorConnection

**Examples:**

```javascript:refresh-slider-values-from-connected-processor
// Title: Refresh connected sliders after processor state restore
// Context: Preset or state restore updates module internals first, then UI controls pull the new values.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "FxChain", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var connectedControls = [
    Content.addKnob("Drive", 0, 0),
    Content.addKnob("Tone", 80, 0),
    Content.addKnob("Mix", 160, 0)
];

for (c in connectedControls)
{
    c.set("processorId", "FxChain");
    c.set("parameterId", "Gain");
}

const var fx = Synth.getEffect("FxChain");

inline function syncControlsFromProcessor()
{
    for (c in connectedControls)
        c.updateValueFromProcessorConnection();
}

// Call this right after restoreState() or any non-UI parameter mutation path.
syncControlsFromProcessor();
```
```json:testMetadata:refresh-slider-values-from-connected-processor
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "fx.setAttribute(fx.Gain, -9.0) || true", "value": true},
    {"type": "REPL", "expression": "syncControlsFromProcessor() || false", "value": false},
    {"type": "REPL", "expression": "fx.getAttribute(fx.Gain)", "value": -9.0},
    {"type": "REPL", "expression": "connectedControls[0].getValue()", "value": -9.0}
  ]
}
```

**Pitfalls:**
- If processor state changes outside slider interaction and you skip this refresh, the UI can show stale values until the next manual interaction.

**Cross References:**
- `ScriptSlider.connectToModulatedParameter`
