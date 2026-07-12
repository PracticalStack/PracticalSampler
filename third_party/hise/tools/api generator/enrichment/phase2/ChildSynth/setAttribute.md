## setAttribute

**Examples:**

```javascript:per-channel-gain-control
// Title: Per-channel parameter control from a parent interface
// Context: A multi-channel instrument stores ChildSynth references in an array
// and drives parameters from UI knobs in a loop.

const var NUM_CHANNELS = 4;
const var channels = [];

for (i = 0; i < NUM_CHANNELS; i++)
    channels[i] = Synth.getChildSynth("Channel" + (i + 1));

// Control per-channel gain from an array of knobs
inline function onGainKnobChanged(component, value)
{
    local idx = component.get("id").getTrailingIntValue() - 1;

    // Use dynamic constants instead of raw index numbers
    channels[idx].setAttribute(channels[idx].Gain, value);
}
```
```json:testMetadata:per-channel-gain-control
{
  "testable": false,
  "skipReason": "Defines a control callback requiring UI interaction; requires named child synths (Channel1-4) in the module tree"
}
```

```javascript:subclass-parameter-control
// Title: Controlling subclass-specific parameters
// Context: ChildSynth dynamic constants extend beyond the base four parameters.
// A SynthGroup child exposes parameters like FM depth or detune that
// the parent script controls directly.

const var osc1 = Synth.getChildSynth("Oscillator1");
const var osc2 = Synth.getChildSynth("Oscillator2");

// Parameter indices are registered as dynamic constants on each instance.
// For a SynthGroup these might include Detune (4) and Spread (5)
// in addition to the base Gain (0), Balance (1), VoiceLimit (2), KillFadeTime (3).
inline function onDetuneChanged(component, value)
{
    local idx = parseInt(component.get("id").getTrailingIntValue()) - 1;
    local targets = [osc1, osc2];
    targets[idx].setAttribute(4, value);
}
```
```json:testMetadata:subclass-parameter-control
{
  "testable": false,
  "skipReason": "Defines a control callback requiring UI interaction; requires named child synths with SynthGroup-specific parameters"
}
```
