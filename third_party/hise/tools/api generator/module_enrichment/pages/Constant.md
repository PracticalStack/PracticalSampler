---
title: Constant
moduleId: Constant
type: Modulator
subtype: VoiceStartModulator
tags: [generator]
builderPath: b.Modulators.Constant
screenshot: /images/v2/reference/audio-modules/constant.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: Velocity, type: alternative, reason: "Dynamic per-note value from MIDI velocity instead of a fixed constant" }
  - { id: KeyNumber, type: alternative, reason: "Note-based mapping instead of a fixed constant" }
commonMistakes:
  - title: "The intensity knob IS the output value"
    wrong: "Looking for a dedicated parameter to set the constant level"
    right: "Use the inherited intensity knob (or setIntensity() from script) to control the output"
    explanation: "The Constant modulator has no parameters of its own. The intensity control inherited from the modulator base class is the sole mechanism for setting the output value."
forumReferences:
  - id: 1
    title: "No getIntensity() method on Constant modulator"
    summary: "There is no API call to read back the current intensity value; store it in a script variable when you call setIntensity() if you need to retrieve it later."
    topic: 47
  - id: 2
    title: "Use scriptnode time-variant modulator for smooth real-time gain control"
    summary: "A plain Constant modulator produces zipper noise when changed during playback; the recommended fix is a Script Time Variant Modulator with a scriptnode smoothing node."
    topic: 1217
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Return a fixed value from onVoiceStart"
llmRef: |
  Constant (Modulator/VoiceStartModulator)

  Outputs a fixed modulation value controlled entirely by the inherited intensity knob. Has no parameters of its own. In gain chains the raw output is 0.0 and intensity carries the value directly; in pitch chains the raw output is 1.0 and intensity scales in semitones.

  Signal flow:
    noteOn -> mode check -> return 0.0 (gain) or 1.0 (pitch) -> intensity applied -> modulation out

  CPU: negligible, polyphonic (runs once per voice on note-on)

  Parameters: none

  When to use:
    Script-controlled per-voice gain or pitch. Add to a modulation chain and call setIntensity() from script for real-time control. Stack two Constant modulators when you need independent control sources (e.g., one for a knob, one for a key switch).

  Common mistakes:
    The intensity knob is the output value - there is no separate parameter to set.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: return a fixed value from onVoiceStart.

  See also:
    alternative Velocity - dynamic per-note value from MIDI velocity
    alternative KeyNumber - note-based mapping instead of a constant
---

::category-tags
---
tags:
  - { name: generator, desc: "Modulators that create modulation signals internally, such as envelopes and LFOs" }
---
::

![Constant screenshot](/images/v2/reference/audio-modules/constant.png)

The Constant modulator outputs a fixed per-voice modulation value. Unlike other voice-start modulators that read from MIDI data, it always returns the same value, making it the primary tool for script-controlled modulation. Add it to any modulation chain and call `setIntensity()` from script to set the level at runtime - even while voices are already playing. Note that there is no corresponding `getIntensity()` call; if you need to read the value back, store it in a script variable when you set it. [1]($FORUM_REF.47$)

The module has no parameters of its own. The inherited intensity knob is the sole control and directly determines the output value. In gain modulation chains the raw output is 0.0 and the intensity value is used as-is; in pitch modulation chains the raw output is 1.0 and the intensity scales in semitones.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Intensity:
      desc: "Sets the constant modulation output value (inherited from modulator base)"
      range: "0.0 - 1.0"
      default: "1.0"
  functions: {}
  modulations: {}
---

```
// Constant - fixed modulation value
// noteOn in -> modulation out (per voice)

onNoteOn() {
    if (mode == Gain)
        value = 0.0     // intensity carries the actual value
    else
        value = 1.0     // neutral multiplier for pitch/pan

    return value * Intensity
}
```

::

### Stacking for Independent Controls

When two sources need to control the same modulation chain independently - for example a UI knob and a key switch - stack two Constant modulators rather than computing a relative offset from a single one. Each modulator's intensity can be set separately via script, and the modulation chain combines them. Voice-start modulators have virtually no CPU cost, so the overhead is negligible.

### Zipper Noise with Real-Time Changes

Changing the intensity of a Constant modulator during playback can produce zipper noise because the value jumps without interpolation. For smooth real-time gain or pitch control, use a [Script Time Variant Modulator]($MODULES.ScriptTimeVariantModulator$) built with scriptnode, which can include a smoothing node. The Constant modulator is best suited to values that change between notes or change infrequently. [2]($FORUM_REF.1217$)

**See also:** $MODULES.Velocity$ -- dynamic per-note value from MIDI velocity instead of a fixed constant, $MODULES.KeyNumber$ -- note-based mapping instead of a fixed constant
