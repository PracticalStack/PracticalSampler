## addStaticGlobalModulator

**Examples:**

```javascript:static-vs-pervoice-choice
// Title: Choosing between per-voice and static global modulation
// Context: A modulation matrix decides at connection time whether to use
// per-voice (addGlobalModulator) or static (addStaticGlobalModulator)
// modulation based on the target type. Effect parameters and synth-level
// controls don't need per-voice resolution.

const var globalLFO = Synth.getModulator("GlobalLFO1");
const var globalCC = Synth.getModulator("GlobalCC");

const var oscGroup = Synth.getChildSynth("Oscillator1");

const var GAIN_CHAIN = 1;
const var PITCH_CHAIN = 2;

inline function addModConnection(source, target, chainIndex, name, useStatic)
{
    local newMod;

    if (useStatic)
    {
        // Static: one value per audio block, shared across all voices.
        // More CPU-efficient for targets like effect parameters, FM depth,
        // or any control that doesn't need per-voice independence.
        newMod = target.addStaticGlobalModulator(chainIndex, source, name);
    }
    else
    {
        // Per-voice: each voice gets its own modulation value.
        // Required for gain/pitch chains where voice independence matters
        // (e.g., per-voice tremolo or vibrato).
        newMod = target.addGlobalModulator(chainIndex, source, name);
    }

    if (isDefined(newMod))
        newMod.setIntensity(1.0);

    return newMod;
}

// CC modulation on gain chain: static is sufficient (no per-voice needed)
addModConnection(globalCC, oscGroup, GAIN_CHAIN, "CC_Vol", true);

// LFO on pitch chain: per-voice for independent vibrato per voice
addModConnection(globalLFO, oscGroup, PITCH_CHAIN, "LFO_Pitch", false);
```
```json:testMetadata:static-vs-pervoice-choice
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with active modulators and named child synths in the module tree"
}
```

**Pitfalls:**
- Some modulation targets only support static modulation (e.g., effect parameters that operate at block level). Using `addGlobalModulator` on these targets wastes CPU computing per-voice values that collapse to a single value anyway. Default to `addStaticGlobalModulator` unless per-voice independence is specifically needed.

**Cross References:**
- `ChildSynth.addGlobalModulator`
- `ChildSynth.addModulator`
- `Synth.getModulator`
