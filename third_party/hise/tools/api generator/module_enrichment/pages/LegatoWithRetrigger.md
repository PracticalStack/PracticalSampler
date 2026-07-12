---
title: Legato with Retrigger
moduleId: LegatoWithRetrigger
type: MidiProcessor
subtype: MidiProcessor
tags: [note_processing]
builderPath: b.MidiProcessors.LegatoWithRetrigger
screenshot: /images/v2/reference/audio-modules/legatowithretrigger.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Only most recent note retriggered"
    wrong: "Playing three or more notes in legato and expecting all previous notes to retrigger in sequence"
    right: "Only the single most recent previous note is stored for retrigger"
    explanation: "The retrigger stack is one note deep. In a legato sequence A-B-C, releasing C retriggers B, but releasing B then produces silence because A was never stored."
  - title: "Retriggered notes use last velocity"
    wrong: "Expecting a retriggered note to sound at its original velocity"
    right: "The retriggered note always uses the velocity of the most recently played note"
    explanation: "If you play A at velocity 100 then B at velocity 50, releasing B retriggers A at velocity 50, not 100."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: simple
  description: "Reproducible with a ScriptProcessor using onNoteOn/onNoteOff callbacks, Message.makeArtificial(), Synth.noteOffByEventId(), and Synth.addNoteOn() with a few tracking variables."
llmRef: |
  LegatoWithRetrigger (MidiProcessor)

  Enforces monophonic note behaviour with single-depth retrigger. When a new note arrives while one is held, the previous note is killed and stored as a retrigger candidate. When the active note is released and a candidate exists, the candidate is retriggered as a new note.

  Signal flow:
    noteOn: make artificial -> kill previous if held -> store retrigger candidate -> pass through
    noteOff (active note released): kill active -> retrigger candidate if available, else go idle

  CPU: negligible (pure event manipulation, no DSP).

  Parameters: none.

  Retrigger stack depth: 1. Only the single most recent previous note is remembered.

  Velocity behaviour: retriggered notes use the velocity of the most recently played note, not the original velocity of the retriggered note.

  When to use:
    Lead lines and expressive legato phrasing where releasing a note should return to the previous pitch, similar to classic monosynth behaviour.

  Common mistakes:
    Stack is only 1 deep - playing A, B, C in legato means A is forgotten when C arrives.
    Retriggered notes inherit the last-played velocity, not their original velocity.

  See also: (none)
---

::category-tags
---
tags:
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events" }
---
::

![LegatoWithRetrigger screenshot](/images/v2/reference/audio-modules/legatowithretrigger.png)

Enforces monophonic note behaviour with a single-depth retrigger stack. When a new noteOn arrives while a note is already held, the previous note is killed and stored as a retrigger candidate. When the active note is released, the stored candidate is automatically retriggered as a new note. If no candidate exists, the module returns to silence.

This produces classic monosynth legato behaviour where releasing a note returns to the previous pitch - suitable for lead lines and expressive phrasing. The retrigger stack is exactly one note deep: in a legato sequence of three or more notes, only the most recent previous note is remembered.

## Signal Path

::signal-path
---
glossary:
  functions:
    makeArtificial:
      desc: "Assigns a new artificial event ID to the incoming noteOn for correct noteOff pairing"
    killPreviousNote:
      desc: "Sends an artificial noteOff for the currently sounding note"
    storeRetriggerCandidate:
      desc: "Saves the previous note's number and channel as the retrigger candidate (1-deep stack)"
    retriggerCandidate:
      desc: "Sends a new artificial noteOn for the stored candidate using the most recent velocity"
---

```
// Legato with Retrigger - monophonic enforcer with 1-deep retrigger
// MIDI noteOn/noteOff in -> transformed MIDI out

onNoteOn(message) {
    makeArtificial(message)

    if note already held:
        killPreviousNote(activeEventId)
        storeRetriggerCandidate(activeNote, activeChannel)

    // Update tracking: note, eventId, velocity, channel
    active = message
}

onNoteOff(message) {
    if message matches active note:
        ignore original noteOff
        killPreviousNote(activeEventId)

        if retrigger candidate available:
            retriggerCandidate(candidate.note, candidate.channel, active.velocity)
            active = retriggered note
        else:
            go idle    // no notes sounding

    else if message matches retrigger candidate:
        clear retrigger candidate
        // active note continues unchanged
}
```

::

### Velocity Behaviour

The retrigger stack stores only the note number and channel of the candidate - not its original velocity. When a candidate is retriggered, it uses the velocity of the most recently played note. For example, playing A at velocity 100 then B at velocity 50 in legato means releasing B retriggers A at velocity 50.

### Event ID Pairing

All incoming noteOn events are assigned new artificial event IDs. NoteOff events for the active note are handled through these artificial IDs rather than note number matching, which ensures correct pairing even when the same pitch is replayed.

### Multi-Channel Support

The module processes all MIDI channels. Both the active note and the retrigger candidate track their original channel, so cross-channel legato works correctly for noteOn transitions.