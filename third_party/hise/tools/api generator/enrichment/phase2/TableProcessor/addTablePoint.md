## addTablePoint

**Examples:**

```javascript:key-range-gate
// Title: Building a key range gate on a KeyModulator table
// Context: Creates a bandpass-shaped curve so a sound only plays within a
// specific MIDI key range. The table acts as a gate: 0 outside the range, 1 inside.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.Velocity, "KeyMod", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var tp = Synth.getTableProcessor("KeyMod");
const var LOW_KEY = 36;
const var HIGH_KEY = 72;

// Always reset before building a new shape
tp.reset(0);

// Build a trapezoidal gate: silent -> full -> full -> silent
tp.addTablePoint(0, LOW_KEY / 127, 0.0);
tp.addTablePoint(0, LOW_KEY / 127, 1.0);
tp.addTablePoint(0, HIGH_KEY / 127, 1.0);
tp.addTablePoint(0, HIGH_KEY / 127, 0.0);

// Fix the last endpoint (index 5 after reset's 2 defaults + 4 added)
tp.setTablePoint(0, 5, 1.0, 0.0, 0.5);
```

```json:testMetadata:key-range-gate
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(0.4)", "value": 1.0},
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(0.1)", "value": 0.0}
  ]
}
```

The default table after `reset()` has two points (index 0 at x=0 and index 1 at x=1). Each `addTablePoint` appends after those, so the final point indices are 0-5. The last `setTablePoint` ensures the endpoint at x=1.0 has the correct y value.

```javascript:two-layer-crossfade
// Title: Building a 2-layer equal-power crossfade
// Context: Two velocity modulators are shaped so that one fades out while the
// other fades in, maintaining constant perceived loudness across the range.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.Velocity, "DynMod1", ss, builder.ChainIndexes.Gain);
builder.create(builder.Modulators.Velocity, "DynMod2", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var NUM_LAYERS = 2;
const var layers = [];

for (i = 0; i < NUM_LAYERS; i++)
    layers[i] = Synth.getTableProcessor("DynMod" + (i + 1));

// Layer 0: fade out (concave curve = 0.25)
layers[0].reset(0);
layers[0].setTablePoint(0, 0, 0, 1, 1);
layers[0].setTablePoint(0, 1, 1, 0, 0.25);

// Layer 1: fade in (convex curve = 0.75)
layers[1].reset(0);
layers[1].setTablePoint(0, 0, 0, 0, 1);
layers[1].setTablePoint(0, 1, 1, 1, 0.75);
```

```json:testMetadata:two-layer-crossfade
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "layers[0].getTable(0).getTableValueNormalised(0.0)", "value": 1.0},
    {"type": "REPL", "expression": "layers[0].getTable(0).getTableValueNormalised(1.0)", "value": 0.0},
    {"type": "REPL", "expression": "layers[1].getTable(0).getTableValueNormalised(1.0)", "value": 1.0}
  ]
}
```

For a simple 2-layer crossfade, no interior points are needed -- the curve parameter on the edge points creates the equal-power shape.

```javascript:three-layer-crossfade
// Title: 3-layer crossfade with interior points
// Context: When crossfading more than 2 layers, interior points define each
// layer's active region. Each layer rises and falls within its segment.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.Velocity, "DynMod2", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var mid = Synth.getTableProcessor("DynMod2");

mid.reset(0);

// Start at zero
mid.setTablePoint(0, 0, 0, 0, 1);

// Add an interior peak -- addTablePoint uses default curve 0.5
mid.addTablePoint(0, 1, 1);

// Shape the peak position and fade slopes
mid.setTablePoint(0, 1, 0.5, 1, 0.75);   // rise to peak at midpoint
mid.setTablePoint(0, 2, 1, 0, 0.25);       // fall to zero at end
```

```json:testMetadata:three-layer-crossfade
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "mid.getTable(0).getTableValueNormalised(0.0)", "value": 0.0},
    {"type": "REPL", "expression": "mid.getTable(0).getTableValueNormalised(0.5) > 0.99", "value": true},
    {"type": "REPL", "expression": "mid.getTable(0).getTableValueNormalised(1.0)", "value": 0.0}
  ]
}
```

**Pitfalls:**
- The `reset()` -> `addTablePoint()` -> `setTablePoint()` sequence is order-dependent. Point indices shift as points are added, so always add all interior points before adjusting them with `setTablePoint()`. Adding a point between two `setTablePoint()` calls changes the indices of subsequent points.
