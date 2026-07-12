## setBypassed

**Examples:**

```javascript:global-fx-bypass-toggle
// Title: Global FX bypass with state preservation
// Context: A "bypass all FX" toggle that saves each effect's current
// bypass state before engaging, and restores it when disengaging.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "ChannelEq 1", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "ChannelEq 2", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "MasterComp", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "MasterLimiter", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var NUM_CHANNELS = 2;
const var fxModules = [];

for (i = 0; i < NUM_CHANNELS; i++)
    fxModules.push(Synth.getEffect("ChannelEq " + (i + 1)));

fxModules.push(Synth.getEffect("MasterComp"));
fxModules.push(Synth.getEffect("MasterLimiter"));

// Store the per-effect bypass state before the global bypass
const var savedBypassStates = {};
reg globalBypassActive = false;

inline function toggleGlobalBypass()
{
    globalBypassActive = !globalBypassActive;

    if (globalBypassActive)
    {
        // Save each effect's current state, then bypass all
        for (fx in fxModules)
        {
            savedBypassStates[fx.getId()] = fx.isBypassed();
            fx.setBypassed(true);
        }
    }
    else
    {
        // Restore each effect's individual bypass state
        for (fx in fxModules)
            fx.setBypassed(savedBypassStates[fx.getId()]);
    }
}

// --- test-only ---
toggleGlobalBypass();
// --- end test-only ---
```
```json:testMetadata:global-fx-bypass-toggle
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "globalBypassActive", "value": 1},
    {"type": "REPL", "expression": "fxModules[0].isBypassed()", "value": 1},
    {"type": "REPL", "expression": "fxModules[3].isBypassed()", "value": 1}
  ]
}
```

```javascript:mode-switching-mutual-exclusion
// Title: Mode switching with mutual exclusion
// Context: A mid/side EQ where selecting "Master", "Mid", or "Side" mode
// bypasses the unused EQ instances and the MS encoder/decoder.
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "LR2MS", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "MS2LR", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "MasterEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "MidEQ", 0, builder.ChainIndexes.FX);
builder.create(builder.Effects.SimpleGain, "SideEQ", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var msEncode = Synth.getEffect("LR2MS");
const var msDecode = Synth.getEffect("MS2LR");

const var eqs = [Synth.getEffect("MasterEQ"),
                 Synth.getEffect("MidEQ"),
                 Synth.getEffect("SideEQ")];

// index: 0 = Master (L/R), 1 = Mid, 2 = Side
inline function setEqMode(index)
{
    // MS encoding only needed for mid/side modes
    msEncode.setBypassed(index == 0);
    msDecode.setBypassed(index == 0);

    // Only the active EQ is unbypassed
    eqs[0].setBypassed(index != 0);
    eqs[1].setBypassed(index == 0);
    eqs[2].setBypassed(index == 0);
}

// --- test-only ---
setEqMode(1);
// --- end test-only ---
```
```json:testMetadata:mode-switching-mutual-exclusion
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "msEncode.isBypassed()", "value": 0},
    {"type": "REPL", "expression": "eqs[0].isBypassed()", "value": 1},
    {"type": "REPL", "expression": "eqs[1].isBypassed()", "value": 0}
  ]
}
```

**Pitfalls:**
- When switching between multi-output and stereo modes, the bypass strategy must change. In multi-output mode, use `setBypassed()` for on/off control. In stereo mode, use an internal enable parameter instead (`fx.setAttribute(fx.InternalEnable, value)`) so the enable state is stored in presets. Mixing the two approaches causes state confusion across preset loads.
