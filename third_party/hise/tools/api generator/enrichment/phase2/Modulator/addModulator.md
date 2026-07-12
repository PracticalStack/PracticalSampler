## addModulator

**Examples:**

```javascript:create-envelope-for-modulation
// Title: Create a per-voice envelope for a dynamic modulation connection
// Context: When building a modulation matrix, envelope sources need per-voice
// modulators created at the target. This creates an AHDSR in the target's
// gain chain and initializes its parameters from the current UI state.

const var filterEffect = Synth.getEffect("PolyFilter1");
const var GAIN_CHAIN = 0;

// Create an AHDSR modulator in the filter's gain chain
const var envMod = filterEffect.addModulator(GAIN_CHAIN, "AHDSR", "FilterEnvMod");

// Initialize envelope parameters from UI knob values
envMod.setAttribute(envMod.Attack, attackKnob.getValue());
envMod.setAttribute(envMod.Decay, decayKnob.getValue());
envMod.setAttribute(envMod.Sustain, sustainKnob.getValue());
envMod.setAttribute(envMod.Release, releaseKnob.getValue());

// Reduce CPU overhead for modulators that don't need sample-accurate updates
envMod.setAttribute(envMod.EcoMode, 32);

envMod.setIntensity(1.0);
```
```json:testMetadata:create-envelope-for-modulation
{
  "testable": false,
  "skipReason": "Requires a PolyFilter effect and UI knobs in the module tree"
}
```

**Pitfalls:**
- The chain index depends on the target module type. Gain chain is typically 0, but other chain indices vary. Check the module's child processor layout.
- Created modulators persist in the module tree until explicitly removed with `Synth.removeModulator()`. Forgetting to remove them when deleting a modulation connection causes accumulating modulators that waste CPU.
