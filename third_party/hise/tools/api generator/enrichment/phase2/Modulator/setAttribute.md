## setAttribute

**Examples:**

```javascript:configure-envelope-params
// Title: Configure envelope parameters from UI knob values
// Context: When creating modulators at runtime (e.g., for a modulation matrix),
// their parameters need to be initialized from the current UI state.

const var envelope = Synth.getModulator("GainEnvelope1");

// Use the dynamic parameter constants -- names depend on modulator type
envelope.setAttribute(envelope.Attack, 20.0);
envelope.setAttribute(envelope.Hold, 0.0);
envelope.setAttribute(envelope.Decay, 500.0);
envelope.setAttribute(envelope.Sustain, 0.7);
envelope.setAttribute(envelope.Release, 200.0);

// EcoMode reduces CPU for modulators that don't need sample-accurate updates
envelope.setAttribute(envelope.EcoMode, 32);
```
```json:testMetadata:configure-envelope-params
{
  "testable": false,
  "skipReason": "Requires an AHDSR modulator named 'GainEnvelope1' in the module tree"
}
```

```javascript:bracket-operator-shorthand
// Title: Bracket-operator shorthand for attribute access
// Context: The bracket operator provides a concise alternative for setting
// attributes by name. Useful when the parameter name is in a variable.

const var lfo = Synth.getModulator("LFO1");

// These two lines are equivalent:
lfo.setAttribute(lfo.Frequency, 4.0);
lfo["Frequency"] = 4.0;

// Useful when iterating parameter names from a different modulator
const var envelope = Synth.getModulator("GainEnvelope1");
const var params = ["Attack", "Decay", "Sustain", "Release"];
const var values = [10.0, 300.0, 0.8, 150.0];

for (i = 0; i < params.length; i++)
    envelope[params[i]] = values[i];
```
```json:testMetadata:bracket-operator-shorthand
{
  "testable": false,
  "skipReason": "Requires LFO and AHDSR modulators in the module tree"
}
```
