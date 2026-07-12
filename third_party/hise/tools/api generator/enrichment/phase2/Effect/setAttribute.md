## setAttribute

**Examples:**

```javascript:named-constant-access
// Title: Named constants for self-documenting parameter access
// Context: Every Effect instance exposes its parameters as named constants.
// Use these instead of raw integer indices.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.PolyphonicFilter, "PolyphonicFilter", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var filter = Synth.getEffect("PolyphonicFilter");

// Named constants map to parameter indices automatically
filter.setAttribute(filter.Frequency, 1000.0);
filter.setAttribute(filter.Q, 0.7);
filter.setAttribute(filter.Mode, 2); // filter mode enum value
```
```json:testMetadata:named-constant-access
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "filter.getAttribute(filter.Frequency)", "value": 1000.0},
    {"type": "REPL", "expression": "filter.getAttribute(filter.Q)", "value": 0.7}
  ]
}
```

```javascript:band-offset-arithmetic
// Title: Parametric EQ band offset arithmetic
// Context: Multi-band effects use a BandOffset constant to calculate
// the parameter index for a specific band.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.ParametriqEQ, "Parametriq EQ", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var eq = Synth.getEffect("Parametriq EQ");

// BandOffset is the stride between bands in the parameter list.
// Band 0 gain = BandOffset * 0 + Gain, Band 1 gain = BandOffset * 1 + Gain, etc.
eq.setAttribute(eq.BandOffset * 0 + eq.Gain, -3.0);  // Band 1: -3 dB
eq.setAttribute(eq.BandOffset * 1 + eq.Gain, 2.0);   // Band 2: +2 dB
eq.setAttribute(eq.BandOffset * 2 + eq.Gain, -1.5);  // Band 3: -1.5 dB
```
```json:testMetadata:band-offset-arithmetic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "eq.getAttribute(eq.BandOffset * 0 + eq.Gain)", "value": -3.0},
    {"type": "REPL", "expression": "eq.getAttribute(eq.BandOffset * 1 + eq.Gain)", "value": 2.0},
    {"type": "REPL", "expression": "eq.getAttribute(eq.BandOffset * 2 + eq.Gain)", "value": -1.5}
  ]
}
```

```javascript:mirror-eq-parameters
// Title: Mirroring parameters across multiple EQ instances
// Context: A mid/side EQ setup where mid and side EQs share
// the same parameter values as the master EQ.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.ParametriqEQ, "MasterEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.ParametriqEQ, "MidEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.ParametriqEQ, "SideEQ", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var eqs = [Synth.getEffect("MasterEQ"),
                 Synth.getEffect("MidEQ"),
                 Synth.getEffect("SideEQ")];

// Copy all parameters from master EQ to mid and side EQs
const var NUM_PARAMS = 4 * 5; // 4 bands x 5 params per band

for (i = 0; i < NUM_PARAMS; i++)
{
    eqs[1].setAttribute(i, eqs[0].getAttribute(i));
    eqs[2].setAttribute(i, eqs[0].getAttribute(i));
}

// --- test-only ---
// Set a non-default value on MasterEQ and re-copy to verify the pattern
eqs[0].setAttribute(0, -5.0);
eqs[1].setAttribute(0, eqs[0].getAttribute(0));
// --- end test-only ---
```
```json:testMetadata:mirror-eq-parameters
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "eqs[1].getAttribute(0)", "value": -5.0}
  ]
}
```
