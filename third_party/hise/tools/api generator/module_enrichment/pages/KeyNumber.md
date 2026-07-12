---
title: Notenumber Modulator
moduleId: KeyNumber
type: Modulator
subtype: VoiceStartModulator
tags: [input, note_processing]
builderPath: b.Modulators.KeyNumber
screenshot: /images/v2/reference/audio-modules/keynumber.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: Velocity, type: alternative, reason: "Similar voice-start modulator that reads MIDI velocity instead of note number" }
  - { id: ArrayModulator, type: alternative, reason: "Per-note modulation values from a 128-entry slider pack, recommended when you need discrete per-key control" }
  - { id: Constant, type: disambiguation, reason: "Also a VoiceStartModulator but outputs a fixed value rather than reading from MIDI" }
commonMistakes:
  - title: "Table is always active"
    wrong: "Looking for a UseTable toggle to enable the curve editor"
    right: "The table is always active - edit it directly in the module panel"
    explanation: "Unlike Velocity or Random, KeyNumber has no UseTable parameter. The table is the entire module; without it there would be nothing to configure."
  - title: "Default table passes input through unchanged"
    wrong: "Adding KeyNumber and expecting it to reshape the note response immediately"
    right: "Edit the table curve to create the desired mapping"
    explanation: "The default table is a linear identity ramp, so the output simply tracks the normalised note number until you draw a custom curve."
forumReferences:
  - id: 1
    title: "Array Modulator is the preferred alternative for per-note value mapping"
    summary: "Christoph Hart (the author) recommends the Array Modulator as the preferred tool for per-note value mapping; it provides the same capability with a more convenient interface."
    topic: 398
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Read Message.getNoteNumber() in the onVoiceStart callback and divide by 127"
llmRef: |
  Notenumber Modulator (Modulator/VoiceStartModulator)

  Maps the MIDI note number of each incoming note to a per-voice modulation value (0-1) through a lookup table. The table is always active - there is no UseTable toggle.

  Signal flow:
    noteOn note number (0-127) -> normalise (/ 127.0) -> table lookup -> modulation out

  CPU: negligible, polyphonic (runs once per voice on note-on)

  Parameters: none

  The table X axis displays MIDI note names. Note 0 maps to X=0.0, note 127 maps to X=1.0. The default table is a linear identity ramp, so the output equals the normalised note number until the curve is edited.

  When to use:
    Map note position to volume, filter cutoff, or any other parameter to create keyboard-tracking behaviour. Edit the table curve to shape the response - for example, boost the bass range or create a split point.

  Common mistakes:
    The table is always active; there is no UseTable parameter to enable it.
    The default linear ramp means the module has no audible effect on parameters that already track note number linearly - edit the curve.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: read Message.getNoteNumber() and divide by 127 in onVoiceStart.

  See also:
    alternative Velocity - reads velocity instead of note number
    alternative ArrayModulator - per-note values from a slider pack, more flexible for discrete mappings
    disambiguation Constant - fixed modulation value, no MIDI input
---

::category-tags
---
tags:
  - { name: input, desc: "Modulators that convert external events like MIDI or MPE into modulation signals" }
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events" }
---
::

![Notenumber Modulator screenshot](/images/v2/reference/audio-modules/keynumber.png)

The Notenumber Modulator reads the MIDI note number of each incoming note and maps it through a lookup table to produce a per-voice modulation value between 0 and 1. It is the standard way to create keyboard-tracking behaviour - for example, brightening the tone as you play higher on the keyboard or scaling volume across the key range.

Unlike [Velocity]($MODULES.Velocity$), which has optional processing stages, the Notenumber Modulator has no parameters at all. The table is always active and is the entire module. MIDI note 0 maps to the left edge of the table (X = 0.0) and note 127 maps to the right edge (X = 1.0). The table's X axis displays MIDI note names (C1, F#3, etc.) for easy orientation. The default curve is a linear identity ramp, so without editing, the output simply tracks the normalised note number.

## Signal Path

::signal-path
---
glossary:
  parameters: {}
  functions:
    tableLookup:
      desc: "Maps the normalised note number through a user-defined curve (0-1 input, 0-1 output)"
  modulations: {}
---

```
// Notenumber Modulator - MIDI note number to modulation value
// noteOn in -> modulation out (per voice)

onNoteOn() {
    value = noteNumber / 127.0    // normalise to 0.0 - 1.0

    value = tableLookup(value)    // always active

    return value
}
```

::

### Keyboard Tracking Patterns

The table curve determines how note position maps to modulation output. Common shapes include:

- **Linear ramp (default)** - output tracks note number directly; higher notes produce higher values
- **Inverted ramp** - lower notes produce higher values, useful for bass-heavy volume scaling
- **Step function** - creates a keyboard split point where notes below a threshold receive one value and notes above receive another
- **S-curve** - gentle roll-off at the extremes with stronger response in the mid range

For discrete per-key control rather than a continuous curve, the [Array Modulator]($MODULES.ArrayModulator$) provides a 128-entry slider pack with individual values per note. It is also the recommended alternative when you need per-note value mapping more generally — Christoph Hart considers it the preferred tool over KeyNumber for that use case. [1]($FORUM_REF.398$)

**See also:** $MODULES.Velocity$ -- similar voice-start modulator that reads MIDI velocity instead of note number, $MODULES.ArrayModulator$ -- per-note modulation values from a 128-entry slider pack for discrete per-key control, $MODULES.Constant$ -- also a VoiceStartModulator but outputs a fixed value with no MIDI input
