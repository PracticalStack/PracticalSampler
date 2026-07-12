---
title: Event Data Modulator
moduleId: EventDataModulator
type: Modulator
subtype: VoiceStartModulator
tags: [routing]
builderPath: b.Modulators.EventDataModulator
screenshot: /images/v2/reference/audio-modules/eventdatamodulator.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: EventDataEnvelope, type: companion, reason: "Continuous version that re-reads the event data slot every audio block with smoothing, allowing live updates during the voice lifetime" }
  - { id: Velocity, type: alternative, reason: "Simpler voice-start modulator that reads MIDI velocity instead of custom event data" }
commonMistakes:
  - title: "Write event data before the note-on"
    wrong: "Setting event data in the same callback after triggering the note"
    right: "Call GlobalRoutingManager.setEventData() in a MIDI processor before the note-on event reaches the modulator"
    explanation: "The modulator reads the slot once at note-on. Data written after the note has already triggered will not be seen by that voice."
  - title: "SlotIndex 16 silently aliases to slot 0"
    wrong: "Using slot index 16, expecting it to be a distinct slot"
    right: "Use slot indices 0-15 only (16 slots total)"
    explanation: "The parameter slider allows 16, but internally it wraps to slot 0. Always stay within the 0-15 range."
customEquivalent:
  approach: hisescript
  moduleType: ScriptVoiceStartModulator
  complexity: trivial
  description: "Read the event data slot via GlobalRoutingManager.getEventData(eventId, slotIndex) in the onVoiceStart callback and return the value"
llmRef: |
  Event Data Modulator (Modulator/VoiceStartModulator)

  Reads a value from one of 16 per-event data slots at note-on and uses it as a constant per-voice modulation value. Event data is written by script via GlobalRoutingManager.setEventData(eventId, slotIndex, value) before the note triggers.

  Signal flow:
    noteOn -> extract event ID -> lookup slot[SlotIndex] for this event -> if found: clamp to 0-1 -> if not found: use DefaultValue -> modulation out

  CPU: negligible, polyphonic (single hash-table lookup per voice start, no per-sample processing)

  Parameters:
    SlotIndex (0-15, discrete, default 0) - which event data slot to read
    DefaultValue (0.0-1.0, default 0%) - fallback value when the slot has not been written for this event

  When to use:
    Route custom per-note data from script into the modulation system. Typical uses include scripted velocity curves, round-robin selection, or any per-note value computed in a MIDI processor.

  Common mistakes:
    Data must be written BEFORE note-on reaches the modulator - set it in a MIDI processor upstream.
    SlotIndex 16 wraps silently to slot 0 - valid range is 0-15.
    Writing 0.0 is distinct from not writing - 0.0 returns 0.0, not DefaultValue.

  Custom equivalent:
    hisescript via ScriptVoiceStartModulator: read GlobalRoutingManager.getEventData() in onVoiceStart.

  See also:
    companion EventDataEnvelope - continuous version with smoothing, re-reads every audio block
    alternative Velocity - reads MIDI velocity instead of custom data
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
---
::

![Event Data Modulator screenshot](/images/v2/reference/audio-modules/eventdatamodulator.png)

The Event Data Modulator reads a value from one of 16 per-event data slots at note-on and outputs it as a constant per-voice modulation value. This provides a bridge between script logic and the modulation system: a MIDI processor writes custom data for each event via the [GlobalRoutingManager]($API.GlobalRoutingManager$), and this modulator picks it up when the corresponding note starts a voice.

Because it is a sample-and-hold modulator, the value is fixed for the entire voice lifetime. If the event data slot is updated after note-on, the voice does not see the change. For live-updating behaviour, use the [Event Data Envelope]($MODULES.EventDataEnvelope$) instead, which re-reads the slot every audio block with smoothing.

## Signal Path

::signal-path
---
glossary:
  parameters:
    SlotIndex:
      desc: "Which of the 16 event data slots (0-15) to read"
      range: "0 - 15"
      default: "0"
    DefaultValue:
      desc: "Fallback value when the slot has not been written for this event"
      range: "0 - 100%"
      default: "0%"
  functions:
    lookupEventData:
      desc: "Hash-table lookup of the event data storage using the event ID and slot index"
  modulations: {}
---

```
// Event Data Modulator - per-event data to modulation value
// noteOn in -> modulation out (per voice)

onNoteOn() {
    eventId = message.getEventId()
    result = lookupEventData(eventId, SlotIndex)

    if (result.found)
        value = clamp(result.value, 0.0, 1.0)
    else
        value = DefaultValue

    return value    // constant for voice lifetime
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Event Data
    params:
      - name: SlotIndex
        desc: "Which of the 16 event data slots to read for the incoming event"
        range: "0 - 15"
        default: "0"
        hints:
          - type: warning
            text: "The slider allows values up to 16, but index 16 silently wraps to slot 0. Use indices 0-15 only."
      - name: DefaultValue
        desc: "The modulation value to use when the slot has not been written for this event"
        range: "0 - 100%"
        default: "0%"
        hints:
          - type: info
            text: "Writing 0.0 to a slot is distinct from not writing it. A slot explicitly set to 0.0 returns 0.0, not the DefaultValue."
---
::

### Writing Event Data

Event data slots are populated from script before the note-on event reaches the modulator. The typical workflow is:

1. Obtain the [GlobalRoutingManager]($API.GlobalRoutingManager$) via `Engine.getGlobalRoutingManager()`.
2. In a MIDI processor's `onNoteOn` callback, call `GlobalRoutingManager.setEventData(Message.getEventId(), slotIndex, value)` to write the desired value.
3. The Event Data Modulator downstream reads the slot when the voice starts.

The data is keyed by event ID, so each note carries its own independent set of slot values. This makes event data fully polyphonic - simultaneous notes do not interfere with each other even when using the same slot index.

> [!Warning:Write data upstream of the modulator] The event data must be written in a MIDI processor that sits before the sound generator containing this modulator. If the data is written too late (after note-on has already been processed), the modulator will see the slot as unwritten and return the DefaultValue.

A [GlobalRoutingManager]($API.GlobalRoutingManager$) must exist in the module tree. If none is present, the modulator logs an error and returns the DefaultValue for all notes.

**See also:** $MODULES.EventDataEnvelope$ -- continuous version that re-reads the slot every audio block with smoothing, allowing modulation values to change during the voice lifetime, $MODULES.Velocity$ -- simpler voice-start modulator that reads MIDI velocity instead of custom event data
