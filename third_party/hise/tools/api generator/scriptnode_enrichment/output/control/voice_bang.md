---
title: Voice Bang
description: "Sends the current parameter value as a modulation signal whenever a note-on message is received."
factoryPath: control.voice_bang
factory: control
polyphonic: false
tags: [control, voice, trigger, midi]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.bang", type: disambiguation, reason: "Triggers on parameter change rather than note-on" }
commonMistakes:
  - title: "Using outside a polyphonic container"
    wrong: "Placing voice_bang in a monophonic container"
    right: "Always place voice_bang inside a polyphonic container (e.g. container.chain with polyphony enabled)"
    explanation: "The node requires a polyphonic context and will produce an error during initialisation if placed in a monophonic container."
llmRef: |
  control.voice_bang

  Sends the current Value parameter as a modulation signal each time a note-on message is received. Acts as a per-voice trigger that re-sends its stored value on every new note.

  Signal flow:
    Control node - no audio processing
    Note-on event -> send stored Value -> normalised modulation output

  CPU: negligible, monophonic (shared value, but requires polyphonic context)

  Parameters:
    Value (0.0 - 1.0, default 0.0): The value sent to the output on each note-on

  When to use:
    Use to create a static per-voice control signal that is set once at note start. Useful for randomised parameters (when driven by a random source) or for latching a value at voice onset.

  Common mistakes:
    Using outside a polyphonic container -- requires polyphonic context

  See also:
    [disambiguation] control.bang -- triggers on parameter change rather than note-on
---

Voice Bang sends its stored Value to the modulation output each time a note-on message is received. The Value parameter acts as the payload: whatever value is set when a note arrives is what gets sent. This makes it useful for latching a control value at voice onset, such as capturing a randomised parameter or a UI control state at the moment a note begins.

The node must be placed inside a polyphonic container. All voices share the same stored Value, but each voice's note-on independently triggers the output. Changing the Value between notes affects what subsequent voices receive.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "The value sent to the output on each note-on event"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.voice_bang - send value on note-on
// MIDI note-on -> normalised control out

onNoteOn() {
    output = Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: ""
    params:
      - { name: Value, desc: "The value sent to the modulation output whenever a note-on is received. All voices share this value, but each voice triggers independently.", range: "0.0 - 1.0", default: "0.0" }
---
::

Unlike [control.bang]($SN.control.bang$), which triggers when its input parameter changes, voice_bang triggers on MIDI note-on events. A common pattern is to connect a [control.random]($SN.control.random$) node to the Value input, producing a different random value for each new voice.

**See also:** $SN.control.bang$ -- triggers on parameter change rather than note-on
