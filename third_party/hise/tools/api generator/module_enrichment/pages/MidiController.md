---
title: Midi Controller
moduleId: MidiController
type: Modulator
subtype: TimeVariantModulator
tags: [input]
builderPath: b.Modulators.MidiController
screenshot: /images/v2/reference/audio-modules/midicontroller.png
cpuProfile:
  baseline: low
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: PitchWheel, type: alternative, reason: "Dedicated pitch bend modulator with bipolar output and centre-zero default" }
  - { id: Velocity, type: alternative, reason: "Per-voice note-on velocity modulator rather than monophonic CC" }
commonMistakes:
  - title: "ControllerNumber 128 is pitch wheel, 129 is aftertouch"
    wrong: "Setting ControllerNumber to 128 expecting aftertouch"
    right: "Use 128 for pitch wheel and 129 for aftertouch"
    explanation: "The numbering follows the internal convention: 0-127 are standard CCs, 128 is pitch wheel (14-bit), and 129 is aftertouch."
  - title: "DefaultValue only applies at initial load"
    wrong: "Expecting DefaultValue to reset the modulator after all notes are released"
    right: "DefaultValue sets the initial output when the preset is loaded, before any MIDI is received"
    explanation: "The modulator starts at full output (1.0) by default. DefaultValue is applied once during preset loading and does not reset during playback."
forumReferences:
  - id: 1
    title: "Synth.sendController() drives MidiController modulators from script"
    summary: "To control a MidiController modulator from a UI element or script, use Synth.sendController(ccNumber, value) so the value passes through the full signal path including table lookup and smoothing."
    topic: 3179
customEquivalent:
  approach: hisescript
  moduleType: ScriptTimeVariantModulator
  complexity: simple
  description: "Listen for CC messages in the onController callback and write modulation values via ScriptTimeVariantModulator. Allows custom normalisation, filtering, and multi-CC logic."
llmRef: |
  Midi Controller (Modulator/TimeVariantModulator)

  Converts MIDI CC, pitch wheel, or aftertouch messages into a monophonic modulation signal (0-1). An optional lookup table shapes the response curve, and a one-pole smoother prevents zipper noise.

  Signal flow:
    MIDI event -> normalise to 0-1 -> [table lookup] -> [invert] -> smooth -> modulation out

  CPU: low, monophonic (one shared value, not per-voice)

  Parameters:
    Inverted (Off/On, default Off) - flips the output: 1 - value
    UseTable (Off/On, default Off) - enables a curve editor for custom response shaping
    ControllerNumber (0-129, default 1) - which MIDI source to listen to: 0-127 for CC, 128 for pitch wheel, 129 for aftertouch
    SmoothTime (0-2000 ms, default 200 ms) - one-pole low-pass smoothing time; 0 disables smoothing
    DefaultValue (0-100%, default 0%) - initial output value applied at preset load

  When to use:
    Map any MIDI CC to a modulation target (filter cutoff, volume, FX mix). Use when you need smooth, table-shaped CC response. For pitch bend specifically, the dedicated PitchWheel modulator may be more convenient.

  Common mistakes:
    ControllerNumber 128 is pitch wheel and 129 is aftertouch (not the reverse).
    DefaultValue only applies at initial load, not on note-off or all-notes-off.

  Custom equivalent:
    hisescript via ScriptTimeVariantModulator: handle CC in onController and write values manually.

  See also:
    alternative PitchWheel - dedicated pitch bend with bipolar output
    alternative Velocity - per-voice note-on velocity
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
---
::

![Midi Controller screenshot](/images/v2/reference/audio-modules/midicontroller.png)

The Midi Controller modulator converts incoming MIDI CC messages into a monophonic modulation signal between 0 and 1. It is the standard way to map hardware knobs, faders, and pedals to any modulatable parameter in the signal chain. The raw 7-bit CC value is normalised, optionally shaped through a lookup table, and smoothed to prevent zipper noise on rapid changes.

Beyond standard CCs (0-127), the modulator can also respond to pitch wheel (ControllerNumber 128, 14-bit resolution) and aftertouch (ControllerNumber 129, including both channel pressure and polyphonic aftertouch). Because it is monophonic, a single smoothed value is shared across all voices.

## Signal Path

::signal-path
---
glossary:
  parameters:
    ControllerNumber:
      desc: "Selects which MIDI source to listen to (0-127: CC, 128: pitch wheel, 129: aftertouch)"
      range: "0 - 129"
      default: "1"
    UseTable:
      desc: "Enables the lookup table for custom response curves"
      range: "Off / On"
      default: "Off"
    Inverted:
      desc: "Flips the output value: 1 - value"
      range: "Off / On"
      default: "Off"
    SmoothTime:
      desc: "One-pole low-pass smoothing time to prevent zipper noise"
      range: "0 - 2000 ms"
      default: "200 ms"
  functions:
    normalise:
      desc: "Converts the raw MIDI value to 0-1 range (CC: /127, pitch wheel: /16383, aftertouch: /127)"
    tableLookup:
      desc: "Maps the normalised value through a user-defined curve (0-1 input, 0-1 output)"
    smooth:
      desc: "One-pole IIR low-pass filter that interpolates toward the target value per sample"
  modulations: {}
---

```
// Midi Controller - MIDI CC to modulation signal
// MIDI event in -> modulation out (monophonic)

onControlChange() {
    if event does not match ControllerNumber
        return

    value = normalise(event)    // 0.0 - 1.0

    if (UseTable)
        value = tableLookup(value)

    if (Inverted)
        value = 1.0 - value

    output = smooth(value, SmoothTime)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: MIDI Source
    params:
      - name: ControllerNumber
        desc: "Selects which MIDI source to listen to. Values 0-127 correspond to standard CC numbers. Value 128 selects pitch wheel (14-bit, normalised by 16383 so that 0.5 is centre). Value 129 selects aftertouch (both channel pressure and polyphonic)."
        range: "0 - 129"
        default: "1"
        hints:
          - type: warning
            text: "128 is pitch wheel and 129 is aftertouch. The metadata description has these swapped - the values listed here reflect the actual behaviour."
  - label: Response Shaping
    params:
      - name: UseTable
        desc: "Enables a lookup table with a curve editor for custom response shaping. The table input is the normalised CC value (0-1) and the output replaces it before inversion."
        range: "Off / On"
        default: "Off"
        hints:
          - type: info
            text: "The table X axis displays 0-127 but operates on the normalised 0-1 value internally. Inversion is applied after the table, so design your curve on the pre-inversion value."
      - { name: Inverted, desc: "Flips the output value so that high CC values produce low modulation values and vice versa. Applied after the table lookup.", range: "Off / On", default: "Off" }
  - label: Smoothing
    params:
      - name: SmoothTime
        desc: "Time constant for the one-pole low-pass smoother. Higher values produce slower, more gradual transitions. Set to 0 to disable smoothing entirely."
        range: "0 - 2000 ms"
        default: "200 ms"
      - name: DefaultValue
        desc: "The initial modulation output applied when the preset is loaded, before any MIDI message is received."
        range: "0 - 100%"
        default: "0%"
---
::

### Aftertouch Aggregation

When ControllerNumber is set to 129 (aftertouch), the modulator accepts both channel pressure and polyphonic aftertouch messages. For polyphonic aftertouch, each note's pressure value is tracked independently, and the modulator outputs the **maximum** pressure across all currently held notes. When a note is released, its pressure value is cleared. This means the output always reflects the highest finger pressure rather than any single voice's aftertouch.

### Scripting Integration

To drive a Midi Controller modulator from script, use [Synth.sendController]($API.Synth.sendController$) with the matching CC number. This sends an internal controller message that the modulator responds to as if it came from external MIDI hardware. This is useful for connecting UI sliders to modulation targets without bypassing the smoothing and table processing.

> [!Tip:Use Synth.sendController for scripted CC values] Rather than setting the modulation value directly, send a controller message via `Synth.sendController(ccNumber, value)` so the value passes through the full signal path including table lookup and smoothing. [1]($FORUM_REF.3179$)

### MPE Behaviour

When MPE mode is enabled globally, the Midi Controller modulator only responds to events on MIDI channel 1 (the MPE master channel). Per-note expression channels are filtered out to avoid interference with MPE voice-level modulation.

**See also:** $MODULES.PitchWheel$ -- dedicated pitch bend modulator with bipolar output and centre-zero default, $MODULES.Velocity$ -- per-voice note-on velocity modulator for dynamics that vary with each key press
