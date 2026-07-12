---
title: CC2Note
moduleId: CC2Note
type: MidiProcessor
subtype: MidiProcessor
tags: [note_processing, routing]
builderPath: b.MidiProcessors.CC2Note
screenshot: /images/v2/reference/audio-modules/cc2note.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "CC messages require a held note"
    wrong: "Expecting notes to play without holding a key on the controller"
    right: "Hold a note on the keyboard/controller before sending CC messages"
    explanation: "The module captures the last held note number. Without a held note, all CC messages are ignored and no sound is triggered."
  - title: "Bypass does not stop note generation"
    wrong: "Assuming the Bypass parameter stops note generation"
    right: "Bypass only switches between the module's custom round-robin group cycling and the Sampler's built-in round-robin"
    explanation: "Notes are always generated from matching CC messages regardless of Bypass state. To stop note generation, remove or deactivate the module."
  - title: "CC2Note requires a Sampler module"
    wrong: "Using CC2Note with a non-Sampler sound generator"
    right: "Place CC2Note in the MIDI chain of a Sampler module"
    explanation: "The module calls Sampler-specific functions for round-robin group selection. It will produce errors when used with other sound generator types."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: simple
  description: "A ScriptProcessor with onNoteOn/onNoteOff/onController callbacks that captures the last note, suppresses note events, and calls Synth.playNote() on matching CC messages."
llmRef: |
  CC2Note (MidiProcessor)

  Converts incoming MIDI CC messages into note-on events for Sampler-based instruments. Incoming notes are captured (storing the note number) and suppressed. When a CC matching the selected number arrives while a note is held, the module triggers a new note using the captured note number and the CC value as velocity. Includes built-in round-robin group cycling with up/down-stroke pairing for percussion instruments.

  Signal flow:
    noteOn/noteOff -> captured & suppressed (stores last note number)
    CC matching selector + note held -> round-robin group select -> Synth.playNote(lastNote, ccValue)
    non-matching CC -> passthrough

  CPU: negligible (event-driven, no per-sample processing).

  Parameters:
    Bypass (Off/On, default Off) - toggles between custom round-robin group cycling (Off) and the Sampler's built-in round-robin (On). Does NOT bypass note generation.
    ccSelector (0-127, default 0) - which CC number triggers note generation.

  Requires: parent Sampler module. Groups should be organised in pairs for up/down-stroke alternation.

  When to use:
    Triggering drum or percussion samples from a CC source (e.g. e-drum pad, foot controller) where the CC value controls velocity. Particularly suited to hi-hat or instruments needing alternating articulation groups.

  Common mistakes:
    Must hold a note to set the target pitch - CC messages are ignored without one.
    Bypass does not stop note generation, only switches round-robin mode.
    Only works with Sampler sound generators.

  See also: (none)
---

::category-tags
---
tags:
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events. Contains MIDI processors and modulators." }
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree." }
---
::

![CC2Note screenshot](/images/v2/reference/audio-modules/cc2note.png)

CC2Note converts incoming MIDI CC messages into note-on events for Sampler-based instruments. It intercepts and suppresses all incoming note events, storing the most recent note number. When a CC message matching the selected CC number arrives while a note is held, the module generates a new note using the stored note number and the CC value as velocity (0-127). This makes it suited for triggering drum or percussion samples from a CC source such as an e-drum pad or foot controller, where the CC value directly controls hit intensity.

The module includes built-in round-robin group cycling that organises sample groups in up/down-stroke pairs. On each trigger it alternates between up and down strokes, randomly selecting a non-repeating group pair to avoid consecutive identical samples. This behaviour can be switched off in favour of the Sampler's built-in round-robin via the Bypass parameter. Generated notes are one-shot (no note-off is sent), so the module is designed for use with short, self-terminating samples.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Bypass:
      desc: "Toggles between custom round-robin group cycling (Off) and the Sampler's built-in round-robin (On)"
      range: "Off / On"
      default: "Off"
    ccSelector:
      desc: "Which CC number triggers note generation"
      range: "0 - 127"
      default: "0"
  functions:
    captureNote:
      desc: "Stores the note number from incoming noteOn events and suppresses the original event"
    selectGroup:
      desc: "Alternates up/down stroke and randomly selects a non-repeating group pair, then sets the active Sampler group"
    playNote:
      desc: "Generates an artificial note-on using the captured note number and the CC value as velocity"
---

```
// CC2Note - converts CC messages into note triggers
// MIDI in -> note capture + CC-to-note -> MIDI out

onNoteOn(message):
    lastNote = message.noteNumber
    captureNote(message)          // suppress original note

onNoteOff(message):
    if message.noteNumber == lastNote:
        lastNote = none
    captureNote(message)          // suppress original note

onCC(message):
    if message.ccNumber != ccSelector:
        pass through              // non-matching CC unchanged

    if lastNote == none:
        return                    // no held note, ignore

    if not Bypass:
        selectGroup()             // custom round-robin pair

    playNote(lastNote, message.ccValue)
```

::

## Parameters

::parameter-table
---
groups:
  - label: Round-Robin
    params:
      - { name: Bypass, desc: "Switches between the module's custom round-robin group cycling (Off) and the Sampler's built-in round-robin (On). Does not affect note generation - notes are always triggered from matching CC messages regardless of this setting.", range: "Off / On", default: "Off" }
  - label: CC Selection
    params:
      - { name: ccSelector, desc: "The MIDI CC number that triggers note generation. When a CC message with this number arrives while a note is held, a new note is generated using the CC value as velocity.", range: "0 - 127", default: "0" }
---
::

### Sampler Requirement

CC2Note is designed specifically for Sampler-based instruments. It calls Sampler-specific functions for round-robin group control and will produce errors if placed in the MIDI chain of a non-Sampler sound generator.

### Round-Robin Group Layout

The round-robin system expects sample groups to be organised in consecutive pairs (1-2, 3-4, 5-6, etc.), where each pair represents alternating articulations such as up-stroke and down-stroke. The total group count is read from the parent Sampler. If the group count changes after the module is initialised, the module may use a stale value until a parameter is adjusted.

### CC Passthrough

The matching CC message is not consumed - it passes through to downstream processors alongside the generated note. Non-matching CC messages and all other MIDI event types also pass through unchanged.

### Default CC Number

The default ccSelector value of 0 corresponds to CC#0 (Bank Select MSB), which is unlikely to be the intended trigger source. This should be changed to the appropriate CC number for the connected controller.
