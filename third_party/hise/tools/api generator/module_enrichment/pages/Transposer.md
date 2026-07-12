---
title: Transposer
moduleId: Transposer
type: MidiProcessor
subtype: MidiProcessor
tags: [note_processing]
builderPath: b.MidiProcessors.Transposer
screenshot: /images/v2/reference/audio-modules/transposer.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Held notes unaffected by transpose changes"
    wrong: "Changing TransposeAmount while notes are held and expecting them to shift immediately"
    right: "Set TransposeAmount before playing new notes"
    explanation: "The transpose offset is written to each note-on event at the moment it arrives. Already-sounding notes are not affected by subsequent parameter changes."
  - title: "No clamping at MIDI extremes"
    wrong: "Transposing notes near the top or bottom of the keyboard by large amounts"
    right: "Keep transposed note numbers within the 0-127 MIDI range"
    explanation: "No clamping is applied. Notes pushed outside 0-127 may cause unexpected behaviour in downstream modules such as samplers."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: trivial
  description: "A single-line script in onNoteOn that calls Message.setTransposeAmount(Message.getTransposeAmount() + offset)."
llmRef: |
  Transposer (MidiProcessor)

  Shifts incoming MIDI notes up or down by a fixed number of semitones. The offset is applied to note-on events and automatically propagated to their matching note-off events.

  Signal flow:
    MIDI event in -> [if note-on: add TransposeAmount to transpose value] -> MIDI event out

  CPU: negligible (single integer addition per note-on event), monophonic.

  Parameters:
    TransposeAmount (-24 to 24 semitones, default 0) - semitone offset added to each note-on event

  Behaviour:
    - Uses the transpose mechanism rather than modifying note numbers directly, so the original note number is preserved for event matching.
    - Multiple Transposers in a chain stack additively.
    - Parameter changes only affect subsequent note-on events, not currently held notes.
    - No clamping is applied when transposition pushes notes outside the 0-127 range.

  When to use:
    Quick key changes, octave shifts, or interval layering. Place in a sound generator's MIDI chain.

  Common mistakes:
    Expecting held notes to shift when TransposeAmount changes - changes only apply to new notes.
    Transposing near keyboard extremes can push notes outside valid MIDI range.

  See also: (none)
---

::category-tags
---
tags:
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events." }
---
::

![Transposer screenshot](/images/v2/reference/audio-modules/transposer.png)

The Transposer shifts incoming MIDI notes up or down by a fixed number of semitones. It operates on note-on events, and the matching note-off events receive the same offset automatically. The original note number is preserved internally - the offset is stored separately, so event matching and other MIDI processors continue to work correctly.

Multiple Transposer modules in the same MIDI chain stack additively: each one adds its offset to whatever transpose value is already on the event. Parameter changes only take effect on subsequent note-on events; notes that are already sounding are not affected.

## Signal Path

::signal-path
---
glossary:
  parameters:
    TransposeAmount:
      desc: "Semitone offset added to each note-on event"
      range: "-24 - 24 semitones"
      default: "0"
  functions:
    addTranspose:
      desc: "Adds the transpose offset to the event's existing transpose value (stacks with other Transposers)"
---

```
// Transposer - fixed semitone offset
// MIDI event in -> MIDI event out

onMidiEvent(message) {
    if message is note-on:
        addTranspose(message, TransposeAmount)

    // all events (including transposed note-ons) pass through
    output message
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Transpose
    params:
      - { name: TransposeAmount, desc: "Semitone offset applied to each incoming note-on event. Negative values shift down, positive values shift up. Stacks additively when multiple Transposers are chained.", range: "-24 - 24 semitones", default: "0" }
---
::

### Transpose Mechanism

The Transposer uses the transpose mechanism rather than changing note numbers directly. This means the original note number is preserved for event matching, and the effective pitch is calculated by adding the transpose offset at the point of playback. Downstream modules read the combined value when determining pitch.

### Out-of-Range Behaviour

When transposition pushes a note outside the valid MIDI range (0-127), no clamping or discarding occurs. The out-of-range value is passed through, which may cause unexpected behaviour in downstream modules. With the parameter range limited to -24 to +24 semitones, this only affects notes near the extremes of the keyboard.
