## addGlobalModulator

**Examples:**

```javascript:modulation-matrix-connect
// Title: Custom modulation matrix - connecting global sources to synth targets
// Context: A synthesizer with a user-configurable modulation matrix connects
// global LFOs and envelopes from a GlobalModulatorContainer to per-oscillator
// gain and pitch chains. Each connection is created dynamically when the user
// enables a routing in the modulation matrix UI.

// Global modulation sources from a GlobalModulatorContainer
const var globalLFO1 = Synth.getModulator("GlobalLFO1");
const var globalLFO2 = Synth.getModulator("GlobalLFO2");

// Modulation targets - child synth oscillator groups
const var osc1 = Synth.getChildSynth("Oscillator1");
const var osc2 = Synth.getChildSynth("Oscillator2");

// Chain indices: 1 = GainModulation (per-voice volume), 2 = PitchModulation
const var GAIN_CHAIN = 1;
const var PITCH_CHAIN = 2;

inline function connectModulation(source, target, chainIndex, name)
{
    if (!isDefined(target) || target.getId() == "")
    {
        Console.print("Invalid modulation target: " + name);
        return;
    }

    // addGlobalModulator provides per-voice modulation -
    // each voice gets its own modulation value from the global source
    local newMod = target.addGlobalModulator(chainIndex, source, name);

    // The returned Modulator handle controls the connection's intensity
    if (isDefined(newMod))
        newMod.setIntensity(1.0);
}

// Connect LFO1 to Oscillator1's gain chain (per-voice)
connectModulation(globalLFO1, osc1, GAIN_CHAIN, "LFO1_Osc1_Vol");
```
```json:testMetadata:modulation-matrix-connect
{
  "testable": false,
  "skipReason": "Requires a GlobalModulatorContainer with active LFO modulators and named child synths in the module tree"
}
```

**Cross References:**
- `ChildSynth.addStaticGlobalModulator`
- `ChildSynth.addModulator`
- `Synth.getModulator`
