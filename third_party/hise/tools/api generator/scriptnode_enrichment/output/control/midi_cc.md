---
title: MIDI CC
description: "Sends a normalised modulation signal in response to a specific MIDI CC message, pitch bend, aftertouch, or note event."
factoryPath: control.midi_cc
factory: control
polyphonic: false
tags: [control, midi, cc, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.midi", type: companion, reason: "Converts note events (velocity, gate, note number) rather than CC messages" }
commonMistakes:
  - title: "CCNumber values above 127 are special"
    wrong: "Setting CCNumber to 128 and expecting standard MIDI CC behaviour"
    right: "Values 128-131 select pitch bend, aftertouch, note-on, and note-off respectively."
    explanation: "The CCNumber range extends beyond the standard 0-127 MIDI CC range. Values 128 (pitch bend), 129 (aftertouch), 130 (note-on velocity), and 131 (note-off velocity) listen for different MIDI event types."
llmRef: |
  control.midi_cc

  Sends a normalised 0..1 modulation signal when a specific MIDI CC, pitch bend, aftertouch, or note event is received.

  Signal flow:
    Control node -- no audio processing
    MIDI event -> filter by CCNumber -> normalise to 0..1 -> modulation output

  CPU: negligible, monophonic

  Parameters:
    CCNumber: 0 - 131 (default 1, step 1). Selects which MIDI message to listen for. 0-127 = standard CC, 128 = pitch bend, 129 = aftertouch, 130 = note-on, 131 = note-off.
    EnableMPE: Off / On (default Off). MPE mode toggle.
    DefaultValue: 0.0 - 1.0 (default 0.0). Initial/fallback value sent immediately when set.

  When to use:
    Mapping hardware MIDI controllers to scriptnode parameters. 2 instances in surveyed projects (rank 85). Use control.midi for note-based modulation (velocity, gate) instead.

  Common mistakes:
    CCNumber values above 127 select pitch bend, aftertouch, note-on, and note-off -- not standard CCs.

  See also:
    [companion] control.midi -- for note-based modulation
---

Listens for a specific MIDI message type and sends its value as a normalised modulation signal (0..1). The CCNumber parameter selects which message to respond to:

- **0-127** -- standard MIDI CC numbers (e.g. 1 = mod wheel, 7 = volume, 64 = sustain)
- **128** -- pitch bend (full 14-bit range normalised to 0..1)
- **129** -- aftertouch
- **130** -- note-on velocity
- **131** -- note-off velocity

The DefaultValue parameter provides an initial value that is sent immediately when set, serving as a fallback before any MIDI messages are received.

## Signal Path

::signal-path
---
glossary:
  parameters:
    CCNumber:
      desc: "Selects which MIDI message type to listen for"
      range: "0 - 131"
      default: "1"
    DefaultValue:
      desc: "Initial value sent immediately when set"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    filterAndNormalise:
      desc: "Checks if the incoming event matches the configured type and CC number, then normalises the value to 0..1"
---

```
// control.midi_cc - MIDI CC to modulation signal
// MIDI event -> normalised modulation out (0..1)

onMidiEvent(event) {
    if (event matches CCNumber type)
        value = filterAndNormalise(event)
        sendToOutput(value)             // normalised 0..1
}

onDefaultValueChange() {
    sendToOutput(DefaultValue)          // immediate
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: CCNumber, desc: "Selects which MIDI message to listen for. 0-127 for standard CCs, 128 for pitch bend, 129 for aftertouch, 130 for note-on velocity, 131 for note-off velocity.", range: "0 - 131", default: "1" }
      - { name: EnableMPE, desc: "Toggles MPE mode for per-channel MIDI handling.", range: "Off / On", default: "Off" }
  - label: Signal
    params:
      - { name: DefaultValue, desc: "Initial or fallback value. Sent immediately to the modulation output when set.", range: "0.0 - 1.0", default: "0.0" }
---
::

The output is normalised to 0..1 regardless of the source message type. For standard CCs and velocity, this is a simple division by 127. For pitch bend, the full 14-bit range (0-16383) is normalised.

**See also:** $SN.control.midi$ -- converts note events (gate, velocity, note number, frequency) to modulation signals
