## setIntensity

**Examples:**

```javascript:pitch-bend-range
// Title: Control pitch bend range via a UI slider
// Context: Pitch-mode modulators use semitones (-12 to 12) for intensity.
// A common pattern is letting users configure pitch wheel range.

const var pitchMod1 = Synth.getModulator("PitchWheel1");
const var pitchMod2 = Synth.getModulator("PitchWheel2");

inline function onPitchRangeControl(component, value)
{
    // value is in semitones (e.g., 2.0 for whole-tone bend range)
    pitchMod1.setIntensity(value);
    pitchMod2.setIntensity(value);
}

pitchRangeKnob.setControlCallback(onPitchRangeControl);
```
```json:testMetadata:pitch-bend-range
{
  "testable": false,
  "skipReason": "Requires pitch wheel modulators and a UI knob in the module tree"
}
```

```javascript:gain-vs-pitch-intensity
// Title: Set full-range intensity for gain vs pitch modulators
// Context: The intensity range depends on the parent chain's modulation mode.
// GainMode uses 0.0-1.0, PitchMode uses -12.0 to 12.0 (semitones).

const var gainMod = Synth.getModulator("VelocityMod1");
const var pitchMod = Synth.getModulator("PitchEnvelope1");

// GainMode: 1.0 = full modulation depth
gainMod.setIntensity(1.0);

// PitchMode: 12.0 = full octave range, 1.0 = only 1 semitone
pitchMod.setIntensity(12.0);
```
```json:testMetadata:gain-vs-pitch-intensity
{
  "testable": false,
  "skipReason": "Requires gain and pitch modulators in the module tree"
}
```

**Pitfalls:**
- When dynamically creating a modulation connection targeting a pitch chain, remember to set `setIntensity(12.0)` (or the desired semitone range) immediately after creation. The default intensity of 1.0 gives only 1 semitone of pitch range, which is almost certainly not what you want.
