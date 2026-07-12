---
title: Pitch Wheel Modulator
moduleId: PitchWheel
type: Modulator
subtype: TimeVariantModulator
tags: [input]
builderPath: b.Modulators.PitchWheel
screenshot: /images/v2/reference/audio-modules/pitchwheel.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: MidiController, type: alternative, reason: "General-purpose CC modulator that can also read pitch bend as CC 128" }
  - { id: MPEModulator, type: companion, reason: "Per-voice pitch modulation via MPE Glide gesture on channels 2-16" }
commonMistakes:
  - title: "Centre position is 0.5, not 0.0"
    wrong: "Expecting 0.0 to mean no pitch change in a pitch chain"
    right: "The rest position is 0.5 (centre pitch bend). In a pitch chain, 0.5 means no pitch change."
    explanation: "The 14-bit MIDI pitch bend range (0-16383) is normalised to 0.0-1.0, with centre (8192) at 0.5. The modulator initialises at 0.5 before any MIDI is received, which is correct behaviour - not a bug."
  - title: "Table receives raw input before inversion"
    wrong: "Designing a table curve that accounts for the Inverted parameter"
    right: "The table always receives the normalised pitch wheel value (0.0 = full down, 1.0 = full up) regardless of the Inverted setting"
    explanation: "The processing order is fixed: normalise, then table lookup, then inversion. The table input is always the raw normalised value."
forumReferences:
  - id: 1
    title: "Pitch bend arrives as CC128 in onController"
    summary: "Hardware pitch wheel messages arrive in the scripting onController callback with controller number 128; the named constant Message.PITCH_BEND_CC can be used instead of the magic number."
    topic: 11256
  - id: 2
    title: "MIDI learn on pitch wheel sliders pollutes user presets"
    summary: "Connecting UI pitch wheel sliders via MIDI learn causes slider positions to be saved in user presets; handling CC128 manually in onController is the recommended alternative."
    topic: 13837
customEquivalent:
  approach: hisescript
  moduleType: ScriptTimeVariantModulator
  complexity: trivial
  description: "Read the pitch wheel value via Message.getControllerNumber() == 128 in onController and apply custom processing"
llmRef: |
  Pitch Wheel Modulator (Modulator/TimeVariantModulator)

  Converts MIDI pitch bend messages into a monophonic modulation signal (0-1). Centre pitch bend (8192) maps to 0.5. Includes optional table lookup, inversion, and configurable smoothing.

  Signal flow:
    MIDI pitch bend -> normalise (0-1) -> [table lookup] -> [invert] -> smooth -> modulation out

  CPU: negligible, monophonic (shared across all voices)

  Parameters:
    Inverted (Off/On, default Off) - flips the modulation output so pitch bend up produces low values
    UseTable (Off/On, default Off) - enables a lookup table for custom response curves
    SmoothTime (0-1000 ms, default 200 ms) - one-pole lowpass smoothing time; 0 disables smoothing

  When to use:
    Standard pitch bend modulation in any chain (pitch, gain, filter). Use the table for non-linear response curves. Place in the Pitch chain of any sound generator for pitch bend control.

  Common mistakes:
    Centre position is 0.5, not 0.0. In pitch chains, 0.5 = no pitch change.
    Table receives raw normalised input before inversion is applied.

  Custom equivalent:
    hisescript via ScriptTimeVariantModulator: handle CC 128 in onController callback.

  See also:
    alternative MidiController - general CC modulator that can read pitch bend as CC 128
    companion MPEModulator - per-voice pitch via MPE Glide gesture
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
---
::

![Pitch Wheel Modulator screenshot](/images/v2/reference/audio-modules/pitchwheel.png)

The Pitch Wheel Modulator converts incoming MIDI pitch bend messages into a monophonic modulation signal. The 14-bit pitch bend value (0-16383) is normalised to a 0.0-1.0 range, with centre position (8192) mapping to 0.5. The modulator initialises at 0.5 before any MIDI is received, representing the rest position.

Two optional stages shape the response curve. **UseTable** enables a lookup table for drawing custom pitch wheel curves. **Inverted** flips the output so that bending up produces lower values. A configurable **SmoothTime** parameter applies one-pole lowpass filtering to eliminate stepping artefacts from rapid pitch wheel movements. When MPE mode is active, the modulator only processes pitch bend on channel 1 (the master channel) - per-voice pitch is handled by the [MPE Modulator]($MODULES.MPEModulator$) instead.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Inverted:
      desc: "Flips the modulation output so pitch bend up produces low values"
      range: "Off / On"
      default: "Off"
    UseTable:
      desc: "Enables a lookup table for custom pitch wheel response curves"
      range: "Off / On"
      default: "Off"
    SmoothTime:
      desc: "One-pole lowpass smoothing time to reduce stepping artefacts"
      range: "0 - 1000 ms"
      default: "200 ms"
  functions:
    tableLookup:
      desc: "Maps the normalised value through a user-defined curve (0-1 input, 0-1 output)"
    smooth:
      desc: "One-pole IIR lowpass filter that interpolates toward the target value per sample"
  modulations: {}
---

```
// Pitch Wheel Modulator - MIDI pitch bend to modulation signal
// pitch bend in -> modulation out (monophonic)

onPitchBend() {
    value = pitchBendValue / 16383    // normalise to 0.0 - 1.0

    if (UseTable)
        value = tableLookup(value)

    if (Inverted)
        value = 1.0 - value

    targetValue = value
}

perSample() {
    if (SmoothTime > 0)
        output = smooth(targetValue)      // one-pole lowpass
    else
        output = targetValue
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Pitch Wheel Response
    params:
      - { name: Inverted, desc: "Flips the modulation output so pitch bend up produces low values and pitch bend down produces high values. Centre position (0.5) is unchanged.", range: "Off / On", default: "Off" }
      - name: UseTable
        desc: "Enables a lookup table with a curve editor for custom pitch wheel response shapes"
        range: "Off / On"
        default: "Off"
        hints:
          - type: info
            text: "The table receives the raw normalised pitch wheel value (0.0 = full down, 0.5 = centre, 1.0 = full up) before inversion is applied. The X-axis displays the value as -8192 to +8192."
      - name: SmoothTime
        desc: "Time constant for the one-pole lowpass filter that smooths pitch wheel transitions"
        range: "0 - 1000 ms"
        default: "200 ms"
        hints:
          - type: info
            text: "Setting this to 0 disables smoothing entirely, passing the pitch wheel value through with no interpolation."
---
::

### MPE Behaviour

When MPE mode is active, the Pitch Wheel Modulator only processes pitch bend messages on MIDI channel 1 (the master channel). Pitch bend on channels 2-16 is ignored, as those channels carry per-voice MPE data handled by the [MPE Modulator]($MODULES.MPEModulator$). When MPE is not active, pitch bend on all channels is accepted.

### Scripting Integration

In the HISEScript `onController` callback, pitch bend messages arrive with a controller number of 128. The named constant `Message.PITCH_BEND_CC` can be used instead of the raw number. The value range is 0-16383 (14-bit), not 0-127 like standard CCs. [1]($FORUM_REF.11256$)

> [!Tip:Use onController for custom pitch wheel UI] To build a custom pitch wheel display panel, handle CC 128 in the `onController` callback rather than using MIDI learn on a slider. MIDI learn causes slider positions to be saved in user presets, which is usually undesirable. [2]($FORUM_REF.13837$)

The modulator's intensity parameter accepts values from -1 to +1 when set via `setAttribute()`. In a pitch chain, this maps to the semitone range (typically +/-12 semitones at full intensity).

**See also:** $MODULES.MidiController$ -- general-purpose CC modulator that can also read pitch bend as CC 128, $MODULES.MPEModulator$ -- per-voice pitch modulation via MPE Glide gesture on channels 2-16
