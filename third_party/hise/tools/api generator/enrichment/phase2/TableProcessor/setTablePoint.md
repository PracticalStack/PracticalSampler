## setTablePoint

**Examples:**

```javascript:sampler-crossfade-table
// Title: Configuring a sampler crossfade table
// Context: When enabling dynamics crossfade on a sampler, the table needs a
// simple linear fade-out shape. Only the two default edge points are modified.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.SoundGenerators.StreamingSampler, "Sampler1", 0, builder.ChainIndexes.Direct);
builder.flush();
// --- end setup ---

const var tp = Synth.getTableProcessor("Sampler1");
const var sampler = Synth.getChildSynth("Sampler1");

// Enable crossfade on the sampler (attribute index for crossfade varies by module)
sampler.setAttribute(11, 1);

// Shape the crossfade: full volume at low velocity, silent at high
tp.reset(0);
tp.setTablePoint(0, 0, 0, 1.0, 0.5);   // edge point: y=1 (full)
tp.setTablePoint(0, 1, 1.0, 0.0, 0.5);  // edge point: y=0 (silent)

// Reset the second table to default
tp.reset(1);
```

```json:testMetadata:sampler-crossfade-table
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(0.0)", "value": 1.0},
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(1.0)", "value": 0.0}
  ]
}
```

Edge points (index 0 and the last index) ignore the `x` parameter -- only `y` and `curve` are applied. The x values 0 and 1.0 shown above are conventional but have no effect on edge points.

```javascript:velocity-threshold-knob
// Title: Adjusting a velocity threshold at runtime
// Context: A knob controls where a velocity modulator's table transitions from
// 0 to 1, creating a threshold effect. Only interior points are moved.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.Velocity, "AttackVelocity", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var velocityMod = Synth.getModulator("AttackVelocity");
const var tp = velocityMod.asTableProcessor();

// Build a threshold shape with 5 points so the two transition
// points (indices 2 and 3) stay interior and their x can be moved.
// Edge points (first and last index) ignore x in setTablePoint.
tp.reset(0);
tp.setTablePoint(0, 0, 0, 0, 0.5);        // edge start: silent
tp.addTablePoint(0, 0.3, 0.0);              // index 2: pre-threshold
tp.addTablePoint(0, 0.4, 1.0);              // index 3: post-threshold
tp.addTablePoint(0, 0.95, 1.0);             // index 4: new edge end
tp.setTablePoint(0, 4, 1.0, 1.0, 0.5);     // edge end: full volume

inline function onThresholdChanged(component, value)
{
    // Move the transition region based on the threshold knob (0-127)
    tp.setTablePoint(0, 2, (value - 1) / 127, 0.01, 0);
    tp.setTablePoint(0, 3, value / 127, 1, 0);
}

// --- test-only ---
onThresholdChanged(undefined, 64);
// --- end test-only ---
```

```json:testMetadata:velocity-threshold-knob
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(80 / 127) > 0.9", "value": true}
  ]
}
```

```javascript:staccato-envelope-reshape
// Title: Reshaping a table envelope for staccato articulation
// Context: When switching to staccato mode, the table envelope is rebuilt to
// create a sharp attack spike followed by silence.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
var ss = builder.create(builder.SoundGenerators.SineSynth, "TestSynth", 0, builder.ChainIndexes.Direct);
builder.create(builder.Modulators.TableEnvelope, "StaccatoEnvelope", ss, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var envelope = Synth.getModulator("StaccatoEnvelope");

// asTableProcessor() returns undefined if the modulator has no table
const var tp = envelope.asTableProcessor();

if (isDefined(tp))
{
    // Set attack time on the modulator itself
    envelope.setAttribute(envelope.Attack, 200);
    
    // Build a spike shape: silent start, sharp peak at 3%, silent end
    tp.reset(0);
    tp.setTablePoint(0, 0, 0, 0, 0.5);    // start silent
    tp.setTablePoint(0, 1, 1, 0, 0.4);    // end silent (curve 0.4 for slight concavity)
    tp.addTablePoint(0, 0.03, 1);          // sharp peak near the start
}
```

```json:testMetadata:staccato-envelope-reshape
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(0.0)", "value": 0.0},
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(0.03) > 0.8", "value": true},
    {"type": "REPL", "expression": "tp.getTable(0).getTableValueNormalised(1.0)", "value": 0.0}
  ]
}
```

**Pitfalls:**
- The `curve` parameter controls interpolation curvature between this point and the next: `0.5` = linear, values below 0.5 = concave (faster initial change), values above 0.5 = convex (slower initial change). This is not documented in the method signature and can only be understood through experimentation or by examining real-world usage patterns.
