---
title: Velocity Modulator
moduleId: Velocity
type: Modulator
subtype: VoiceStartModulator
tags: [input, note_processing]
builderPath: b.Modulators.Velocity
screenshot: /images/v2/reference/audio-modules/velocity.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: KeyNumber, type: alternative, reason: "Similar voice-start modulator that reads MIDI note number instead of velocity" }
  - { id: ArrayModulator, type: alternative, reason: "Per-note modulation via a 128-entry slider pack instead of the velocity value" }
  - { id: Constant, type: disambiguation, reason: "Also a VoiceStartModulator but outputs a fixed value rather than reading from MIDI" }
commonMistakes:
  - title: "DecibelMode applies after table lookup"
    wrong: "Enabling DecibelMode and expecting the table to operate in dB domain"
    right: "The table always operates in linear 0-1 domain; DecibelMode converts after the table lookup"
    explanation: "The processing order is fixed: invert, then table, then dB conversion. The table input and output are always linear regardless of DecibelMode."
  - title: "UseTable must be enabled to work"
    wrong: "Leaving UseTable off but expecting a custom velocity curve"
    right: "Enable UseTable to activate the curve editor"
    explanation: "The table is bypassed by default. Without UseTable enabled, the velocity mapping is linear (or inverted linear)."
forumReferences:
  - id: 1
    title: "Velocity table cannot be shared between modules"
    summary: "There is no way to share one velocity modulator's table between two modules; use a Global Voice Start Modulator with receivers instead."
    topic: 687
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Read Message.getVelocity() in the onVoiceStart callback and apply inversion, table lookup, or dB conversion in script"
llmRef: |
  Velocity Modulator (Modulator/VoiceStartModulator)

  Converts MIDI note-on velocity into a per-voice modulation value (0-1). Three optional processing stages - inversion, table lookup, and decibel conversion - shape the velocity response curve.

  Signal flow:
    noteOn velocity -> [invert] -> [table lookup] -> [dB conversion] -> modulation out

  CPU: negligible, polyphonic (runs once per voice on note-on)

  Parameters:
    Inverted (Off/On, default Off) - flips the velocity curve so loud notes produce low modulation values
    UseTable (Off/On, default Off) - enables a user-editable curve for custom velocity response
    DecibelMode (Off/On, default Off) - maps the value through a -100..0 dB range then converts to linear gain, providing a more perceptually natural response

  When to use:
    Standard velocity-to-volume or velocity-to-filter mapping. Use the table for custom response curves. Enable DecibelMode when mapping to gain parameters for a more natural dynamic feel.

  Common mistakes:
    DecibelMode applies after the table, not before - the table always works in linear domain.
    UseTable must be enabled for the curve editor to have any effect.

  Tips:
    To share the same velocity curve across multiple sound generators, use a Global Voice Start Modulator instead of duplicating the table in each module.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: read Message.getVelocity() and apply transforms in onVoiceStart.

  See also:
    alternative KeyNumber - reads note number instead of velocity
    alternative ArrayModulator - per-note values from a slider pack
    disambiguation Constant - fixed modulation value, no MIDI input
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events" }
---
::

![Velocity Modulator screenshot](/images/v2/reference/audio-modules/velocity.png)

The Velocity Modulator reads the MIDI velocity of each incoming note and converts it to a per-voice modulation value between 0 and 1. It is one of the most commonly used modulators, typically driving gain or filter cutoff to create velocity-sensitive dynamics.

Three optional stages transform the raw velocity value. **Inverted** flips the response so that soft notes produce high values. **UseTable** enables a curve editor for drawing custom velocity response shapes. **DecibelMode** applies a logarithmic gain curve that maps the value through a -100 to 0 dB range, providing a more perceptually natural dynamic response than the default linear mapping. These stages are applied in fixed order and can be combined freely.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Inverted:
      desc: "Flips the velocity response so high velocities produce low modulation values"
      range: "Off / On"
      default: "Off"
    UseTable:
      desc: "Enables the lookup table for custom velocity response curves"
      range: "Off / On"
      default: "Off"
    DecibelMode:
      desc: "Converts output through a -100..0 dB range to linear gain for a logarithmic response"
      range: "Off / On"
      default: "Off"
  functions:
    tableLookup:
      desc: "Maps the value through a user-defined curve (0-1 input, 0-1 output)"
    decibelToGain:
      desc: "Converts the linear value to a -100..0 dB range, then to linear gain"
  modulations: {}
---

```
// Velocity Modulator - MIDI velocity to modulation value
// noteOn in -> modulation out (per voice)

onNoteOn() {
    value = velocity    // 0.0 - 1.0 from MIDI

    if (Inverted)
        value = 1.0 - value

    if (UseTable)
        value = tableLookup(value)

    if (DecibelMode)
        value = decibelToGain(value)

    return value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Velocity Response
    params:
      - { name: Inverted, desc: "Flips the velocity curve so loud notes produce low modulation values and soft notes produce high values", range: "Off / On", default: "Off" }
      - name: UseTable
        desc: "Enables a lookup table with a curve editor for custom velocity response shapes"
        range: "Off / On"
        default: "Off"
        hints:
          - type: info
            text: "The table always operates in the linear 0-1 domain. When both Inverted and UseTable are active, the table receives the already-inverted value - design your curve with this in mind."
          - type: tip
            text: "To share the same velocity curve across multiple sound generators, use a $MODULES.GlobalVoiceStartModulator$ instead of duplicating the table in each module. [1]($FORUM_REF.687$)"
      - name: DecibelMode
        desc: "Maps the value through a -100 to 0 dB range then converts to linear gain, providing a logarithmic response curve"
        range: "Off / On"
        default: "Off"
        hints:
          - type: info
            text: "Applied after the table lookup. The processing order is fixed: inversion, then table, then dB conversion."
---
::

**See also:** $MODULES.KeyNumber$ -- similar voice-start modulator that reads MIDI note number instead of velocity, $MODULES.ArrayModulator$ -- per-note modulation values from a 128-entry slider pack, offering individual control per key, $MODULES.Constant$ -- also a VoiceStartModulator but outputs a fixed value (1.0) with no MIDI input