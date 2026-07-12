---
title: Channel Setter
moduleId: ChannelSetter
type: MidiProcessor
subtype: MidiProcessor
tags: [routing, note_processing]
builderPath: b.MidiProcessors.ChannelSetter
screenshot: /images/v2/reference/audio-modules/channelsetter.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Channel changes create stuck notes"
    wrong: "Changing the channel number while notes are held"
    right: "Change the channel only when no notes are sounding, or handle stuck notes with an All Notes Off message"
    explanation: "If the channel changes between a note-on and its corresponding note-off, the note-off is sent on the new channel. The synth listening on the original channel never receives a note-off, causing a stuck note."
customEquivalent:
  approach: hisescript
  moduleType: "ScriptProcessor"
  complexity: trivial
  description: "A ScriptProcessor calling Message.setChannel() in onNoteOn, onNoteOff, and onController callbacks."
llmRef: |
  ChannelSetter (MidiProcessor)

  Rewrites the MIDI channel of incoming events to a fixed target channel. Affects note-on, note-off, controller, pitch bend, and aftertouch messages. Programme change and transport events pass through unchanged.

  Signal flow:
    MIDI event in -> set channel to channelNumber -> MIDI event out

  CPU: negligible (single integer write per event), monophonic.

  Parameters:
    channelNumber (1-16, default 1) - target MIDI channel for all rewritten events

  When to use:
    Forcing all events from a source onto a specific MIDI channel before they reach a channel-filtered instrument or processor.

  Common mistakes:
    Changing channelNumber while notes are held causes stuck notes - the note-off arrives on the new channel while the note-on was sent on the old one.

  See also: (none)
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events" }
---
::

![ChannelSetter screenshot](/images/v2/reference/audio-modules/channelsetter.png)

The Channel Setter rewrites the MIDI channel of incoming events to a single target channel. It affects note-on, note-off, controller, pitch bend, and aftertouch messages. Programme change and transport events pass through with their original channel unchanged.

## Signal Path

::signal-path
---
glossary:
  parameters:
    channelNumber:
      desc: "Target MIDI channel for all rewritten events"
      range: "1 - 16"
      default: "1"
  functions:
    setChannel:
      desc: "Overwrites the event's MIDI channel in-place with the target channel"
---

```
// Channel Setter - rewrites MIDI channel
// MIDI event in -> MIDI event out

onMidiEvent(message) {
    if message is NoteOn, NoteOff, Controller, PitchBend, or Aftertouch:
        setChannel(message, channelNumber)

    // ProgramChange and transport events pass through unchanged
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Channel
    params:
      - { name: channelNumber, desc: "Target MIDI channel. All supported event types are rewritten to this channel.", range: "1 - 16", default: "1" }
---
::

### Stuck Notes

Changing the channel number while notes are held can produce stuck notes. If a note-on was sent on the old channel and the corresponding note-off is sent on the new channel, the receiving instrument never sees the note-off. Send an All Notes Off message after changing the channel to clear any stuck notes.

### Channel Range

The standard MIDI channel range of 1-16 is enforced by the parameter. HISE internally supports an extended channel range for advanced routing, but this module only exposes channels 1-16.
