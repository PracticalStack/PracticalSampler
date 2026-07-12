---
title: Event Data Writer
description: "Writes a value to per-event data storage for retrieval by event_data_reader nodes."
factoryPath: routing.event_data_writer
factory: routing
polyphonic: true
tags: [routing, control, event, polyphonic]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "routing.event_data_reader", type: companion, reason: "Reads the data that this node writes" }
commonMistakes:
  - title: "Requires MIDI processing context"
    wrong: "Placing the event_data_writer outside a MIDI-processing container"
    right: "Place the node inside a container that processes MIDI events (e.g. a midichain). The global routing manager must be initialised externally."
    explanation: "The node needs access to HiseEvents to capture event IDs. Without a MIDI processing context, no event IDs are received and no values are written."
  - title: "Value updates apply to all active voices"
    wrong: "Expecting a Value parameter change to write only for the current voice"
    right: "When the Value parameter changes, the new value is written for all active voices' event IDs. The per-voice write only happens automatically on note-on."
    explanation: "The parameter callback iterates all voices and writes the new value for each stored event ID. If you need voice-specific values, connect a polyphonic modulation source to the Value parameter."
llmRef: |
  routing.event_data_writer

  Writes a value to one of 16 per-event data storage slots. The value is written on note-on for the new voice, and on every Value parameter change for all active voices. Companion to event_data_reader.

  Signal flow:
    Control node -- no audio processing
    note-on event -> capture event ID -> write Value to storage[eventId][SlotIndex]
    Value param change -> write to storage for all active voices

  CPU: negligible, polyphonic

  Parameters:
    SlotIndex: 0 - 16, step 1 (default 0). Which data slot to write.
    Value: 0.0 - 1.0 (default 0.0). The value to store.

  When to use:
    Per-voice data storage for polyphonic event-based parameter communication. Connect a modulation source to Value for dynamic updates. Specialised feature -- not commonly used in surveyed projects.

  Common mistakes:
    Requires MIDI processing context. Value updates write for all active voices.

  See also:
    [companion] routing.event_data_reader -- reads the data that this node writes
---

Writes a value to one of 16 per-event data storage slots for retrieval by [event_data_reader]($SN.routing.event_data_reader$) nodes. This is the scriptnode equivalent of the scripting API's GlobalRoutingManager.setEventData() method. Connect a modulation source or macro parameter to the Value knob to store dynamic per-voice data associated with each note event.

The node writes at two points: on note-on (storing the current value for the new voice's event ID) and whenever the Value parameter changes (updating the stored value for all active voices). In a polyphonic context, each voice tracks its own event ID, ensuring that written data is associated with the correct note event.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SlotIndex:
      desc: "Which of the 16 data slots to write"
      range: "0 - 16"
      default: "0"
    Value:
      desc: "The value to store in event data"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    writeToStorage:
      desc: "Writes the value to event storage at the selected slot for the given event ID"
---

```
// routing.event_data_writer - store per-event data
// note-on event + Value -> event storage

onNoteOn(event) {
    eventId = event.id  // captured per voice
    writeToStorage(eventId, SlotIndex, Value)
}

onValueChange(Value) {
    // writes to storage for all active voices
    for each activeVoice:
        writeToStorage(activeVoice.eventId, SlotIndex, Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: SlotIndex, desc: "Selects which of the 16 per-event data slots to write.", range: "0 - 16", default: "0" }
      - { name: Value, desc: "The value to store. Written on note-on for the new voice, and on every parameter change for all active voices.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Update Resolution

The value is written once per buffer when driven by a modulation source. For higher-resolution updates across different sound generators, reduce the maximum block size with Engine.setMaximumBlockSize().

### Compilation

This node can be compiled within a DLL network as it does not depend on dynamic connections.

**See also:** $SN.routing.event_data_reader$ -- reads the data that this node writes
