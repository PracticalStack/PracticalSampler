---
title: MIDI
description: "Converts incoming MIDI events into a normalised modulation signal based on a selectable mode (gate, velocity, note number, frequency, or random)."
factoryPath: control.midi
factory: control
polyphonic: false
tags: [control, midi, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.midi_cc", type: companion, reason: "Listens for MIDI CC messages rather than note events" }
commonMistakes:
  - title: "Node sits inside the signal path"
    wrong: "Placing control.midi outside the audio signal path and expecting it to receive MIDI events"
    right: "Place control.midi inside a container that processes MIDI events (e.g. a midichain or a standard chain)."
    explanation: "Unlike most control nodes, control.midi must sit within the signal path to receive MIDI events via the event processing system."
llmRef: |
  control.midi

  Converts MIDI events into a normalised 0..1 modulation signal. The Mode property selects which MIDI data to extract.

  Signal flow:
    MIDI event -> convert to 0..1 based on Mode -> normalised modulation output

  CPU: negligible, monophonic

  Properties:
    Mode: Gate | Velocity | NoteNumber | Frequency | Random

  Modes:
    Gate: 1.0 on note-on, 0.0 on note-off
    Velocity: note-on velocity / 127
    NoteNumber: note number / 127
    Frequency: note frequency / 20000
    Random: random 0..1 on each note-on

  When to use:
    Driving scriptnode parameters from MIDI input. 9 instances across surveyed projects (rank 43). Common for velocity-sensitive effects, key-tracking filters, or gate-triggered envelopes.

  See also:
    [companion] control.midi_cc -- for MIDI CC messages
---

Converts incoming MIDI note events into a normalised modulation signal (0..1). The Mode property determines which aspect of the MIDI data is extracted and how it maps to the output value. All modes produce normalised output suitable for driving any parameter via modulation connections.

The available modes are:

- **Gate** -- outputs 1.0 on note-on and 0.0 on note-off
- **Velocity** -- outputs the note-on velocity divided by 127
- **NoteNumber** -- outputs the note number (including transposition) divided by 127
- **Frequency** -- outputs the note frequency divided by 20000
- **Random** -- outputs a random value between 0.0 and 1.0 on each note-on

## Signal Path

::signal-path
---
glossary:
  functions:
    convertToValue:
      desc: "Extracts and normalises the relevant MIDI data based on the current Mode"
---

```
// control.midi - MIDI event to modulation signal
// MIDI event -> normalised modulation out (0..1)

onMidiEvent(event) {
    value = convertToValue(event)   // mode-dependent: gate, velocity, etc.
    if (value has changed)
        sendToOutput(value)         // normalised 0..1
}
```

::

The node does not process audio -- it only responds to MIDI events. However, it must be placed inside the signal path so that the event processing system can deliver MIDI events to it. The output only updates when a relevant MIDI event is received.

**See also:** $SN.control.midi_cc$ -- listens for MIDI CC messages rather than note events
