---
title: MidiMuter
moduleId: MidiMuter
type: MidiProcessor
subtype: MidiProcessor
tags: [routing, mixing]
builderPath: b.MidiProcessors.MidiMuter
screenshot: /images/v2/reference/audio-modules/midimuter.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "fixStuckNotes required while playing"
    wrong: "Enabling the mute with fixStuckNotes off while notes are sounding"
    right: "Enable fixStuckNotes before engaging the mute, or release all notes first"
    explanation: "With fixStuckNotes off, note-off events are also blocked during mute. Notes that were sounding when the mute engaged will never receive their note-off and remain stuck."
  - title: "Held keys don't retrigger on unmute"
    wrong: "Expecting held keys to start sounding immediately when the mute is disengaged"
    right: "Held keys whose note-on was blocked during mute will not sound after unmuting - the key must be released and pressed again"
    explanation: "The module does not send artificial note-on events when the mute is disengaged. Keys held during the muted period are invisible to downstream processors."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: trivial
  description: "A few lines in onNoteOn/onNoteOff using Message.ignoreEvent() and a note tracking array."
llmRef: |
  MidiMuter (MidiProcessor)

  Selectively blocks note-on and note-off events based on a mute toggle. Non-note MIDI messages (CC, pitch bend, aftertouch, program change) always pass through unmodified.

  Signal flow:
    MIDI in -> [dispatch by event type] -> note-on: block if muted -> note-off: block if muted (unless fixStuckNotes + note was previously heard) -> all else: passthrough -> MIDI out

  CPU: negligible (boolean checks and single-bit operations per event), monophonic.

  Parameters:
    ignoreButton (Off/On, default Off) - mute toggle; when on, blocks note-on events
    fixStuckNotes (Off/On, default Off) - when on, allows note-offs for previously-heard notes to pass through during mute

  Tracks which note numbers have been heard using a 128-bit tracker (one bit per note number), updated regardless of mute state. This ensures accurate tracking when mute is first engaged.

  When to use:
    Muting a sound generator's MIDI input without removing it from the signal chain. Useful for A/B switching, layered instrument muting, or live performance mute groups.

  Common mistakes:
    Enable fixStuckNotes before engaging the mute if notes may be sounding - otherwise note-offs are blocked and notes get stuck.
    Held keys do not retroactively sound when the mute is disengaged - the key must be re-pressed.

  See also: (none)
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree." }
  - { name: mixing, desc: "Effects that control volume, stereo width, or stereo balance." }
---
::

![MidiMuter screenshot](/images/v2/reference/audio-modules/midimuter.png)

MidiMuter selectively blocks note events based on a mute toggle. When the mute is engaged, note-on events are blocked, preventing new notes from sounding. Note-off events are also blocked by default, but the fixStuckNotes parameter can allow note-offs through for notes that were already sounding before the mute was engaged. All non-note MIDI messages (CC, pitch bend, aftertouch, program change) always pass through unmodified.

The module tracks which note numbers have been heard using a 128-bit tracker that is updated continuously, regardless of mute state. This means the tracker already has an accurate picture of which notes are sounding at the moment the mute is engaged, so fixStuckNotes can immediately allow the correct note-offs through.

## Signal Path

::signal-path
---
glossary:
  parameters:
    ignoreButton:
      desc: "Mute toggle - when on, blocks note-on events"
      range: "Off / On"
      default: "Off"
    fixStuckNotes:
      desc: "When on, allows note-offs for previously-heard notes to pass through during mute"
      range: "Off / On"
      default: "Off"
  functions:
    trackNote:
      desc: "Updates the 128-bit note tracker (set on note-on, clear on note-off) regardless of mute state"
---

```
// MidiMuter - selective note event gate
// MIDI in -> MIDI out

onMidiEvent(message) {
    if message is NoteOn:
        trackNote(noteNumber, on)
        if ignoreButton:
            block message

    else if message is NoteOff:
        wasHeard = noteTracker[noteNumber]
        trackNote(noteNumber, off)
        if ignoreButton:
            if not fixStuckNotes or not wasHeard:
                block message

    // CC, pitch bend, aftertouch, program change:
    // always pass through unmodified
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Mute Control
    params:
      - { name: ignoreButton, desc: "Mute toggle. When on, all note-on events are blocked. When off, all events pass through normally.", range: "Off / On", default: "Off" }
      - { name: fixStuckNotes, desc: "Stuck note prevention. When on during mute, note-off events for notes that were already sounding are allowed through. Has no effect when the mute is off.", range: "Off / On", default: "Off" }
---
::

### Stuck Note Prevention

With ignoreButton on and fixStuckNotes off, both note-on and note-off events are blocked. This will cause stuck notes if any notes were sounding when the mute was engaged. Enable fixStuckNotes to avoid this.

### Mute Toggle Behaviour

The module does not send artificial note-off or note-on events when the mute is toggled. When the mute is engaged, currently sounding notes continue until their note-offs arrive (handled by fixStuckNotes). When the mute is disengaged, keys that were held during the muted period do not retroactively trigger - the key must be released and pressed again.

### Note Tracking

The note tracker uses one bit per note number (0-127), not one bit per voice. If the same note number is triggered multiple times, only one bit is tracked. This is consistent with standard MIDI note-off matching by note number.
