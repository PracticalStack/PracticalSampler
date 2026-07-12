---
title: Event Data Reader
description: "Reads per-event data from a storage slot and outputs it as a modulation signal."
factoryPath: routing.event_data_reader
factory: routing
polyphonic: true
tags: [routing, control, event, polyphonic, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "routing.event_data_writer", type: companion, reason: "Writes the data that this node reads" }
commonMistakes:
  - title: "Requires MIDI processing context"
    wrong: "Placing the event_data_reader outside a MIDI-processing container"
    right: "Place the node inside a container that processes MIDI events (e.g. a midichain). The global routing manager must be initialised externally."
    explanation: "The node needs access to HiseEvents to capture event IDs. Without a MIDI processing context, no event IDs are received and no values are read."
  - title: "Static mode reads once at note-on"
    wrong: "Expecting Static mode to update continuously when the stored value changes"
    right: "Static mode captures the value at note-on time and does not update afterwards. Use dynamic mode (Static = Off) for continuous reading."
    explanation: "When Static is enabled, the value is read once when the note-on event arrives and cached. Subsequent changes to the storage slot are ignored for that voice."
llmRef: |
  routing.event_data_reader

  Reads per-event data from one of 16 storage slots and outputs it as a polyphonic modulation signal. Two modes: Static (read once at note-on) and dynamic (read continuously).

  Signal flow:
    Control node -- no audio processing
    note-on event -> capture event ID -> read storage[eventId][SlotIndex] -> modulation output

  CPU: negligible, polyphonic

  Parameters:
    SlotIndex: 0 - 16, step 1 (default 0). Which data slot to read.
    Static: Off / On (default Off). Off = read continuously; On = read once at note-on.

  When to use:
    Per-voice parameter variation based on data written by event_data_writer or the scripting API. Static mode replicates the EventData Voice Start Modulator; dynamic mode replicates the EventData Envelope. Specialised feature -- not commonly used in surveyed projects.

  Common mistakes:
    Requires MIDI processing context. Static mode reads once at note-on only.

  See also:
    [companion] routing.event_data_writer -- writes the data that this node reads
---

Reads per-event data from one of 16 storage slots and outputs it as a polyphonic modulation signal. Each voice reads the value associated with its own note-on event ID, enabling per-voice parameter variation. Data is written to these slots by [event_data_writer]($SN.routing.event_data_writer$) nodes or via the scripting API.

The node operates in two modes controlled by the Static parameter. When Static is off, the node continuously reads from storage on each audio callback and sends a modulation signal whenever the value changes. When Static is on, the value is captured once at note-on and cached -- subsequent changes to the storage are ignored for that voice. Static mode replicates the behaviour of the EventData Voice Start Modulator, while dynamic mode replicates the EventData Envelope.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SlotIndex:
      desc: "Which of the 16 data slots to read"
      range: "0 - 16"
      default: "0"
    Static:
      desc: "Off: read continuously. On: read once at note-on."
      range: "Off / On"
      default: "Off"
  functions:
    readFromStorage:
      desc: "Looks up the value for the current voice's event ID at the selected slot"
---

```
// routing.event_data_reader - per-event data to modulation signal
// note-on event -> modulation output

onNoteOn(event) {
    eventId = event.id  // captured per voice

    if Static == On:
        value = readFromStorage(eventId, SlotIndex)
        // cached -- no further updates
}

onModulationTick() {
    if Static == Off:
        value = readFromStorage(eventId, SlotIndex)
    output = value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: SlotIndex, desc: "Selects which of the 16 per-event data slots to read.", range: "0 - 16", default: "0" }
      - { name: Static, desc: "When On, reads the value once at note-on and caches it. When Off, reads continuously from storage and updates the modulation output whenever the value changes.", range: "Off / On", default: "Off" }
---
::

### Compilation

This node can be compiled within a DLL network as it does not depend on dynamic connections.

**See also:** $SN.routing.event_data_writer$ -- writes the data that this node reads
