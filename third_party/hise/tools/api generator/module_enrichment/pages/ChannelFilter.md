---
title: Channel Filter
moduleId: ChannelFilter
type: MidiProcessor
subtype: MidiProcessor
tags: [routing, note_processing]
builderPath: b.MidiProcessors.ChannelFilter
screenshot: /images/v2/reference/audio-modules/channelfilter.png
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Inverted MPE range allows only channel 1"
    wrong: "Setting MPE Start higher than MPE End to invert the range"
    right: "Ensure MPE Start is less than or equal to MPE End"
    explanation: "When MPE Start exceeds MPE End, the range is empty and only channel 1 (the MPE master channel) passes through."
  - title: "Transport events bypass channel filtering"
    wrong: "Expecting the Channel Filter to block transport and internal events"
    right: "Use the Channel Filter only for note, CC, pitch bend, and aftertouch filtering"
    explanation: "Transport events (song position, MIDI start/stop) and internal events pass through unconditionally regardless of the channel setting."
customEquivalent:
  approach: hisescript
  moduleType: ScriptProcessor
  complexity: trivial
  description: "A ScriptProcessor checking Message.getChannel() in onNoteOn/onNoteOff/onController and calling Message.ignoreEvent() for non-matching channels."
llmRef: |
  ChannelFilter (MidiProcessor)

  Filters MIDI events by channel number. Has two mutually exclusive modes determined by the global MPE state:
  - Single-channel mode (MPE off): only events on the selected channel pass through.
  - MPE range mode (MPE on): events on channels within the start-end range pass through. Channel 1 (MPE master) is always allowed.

  Matching events pass unchanged - no remapping or transformation. Non-matching events are silently skipped by downstream processors.

  Filtered event types: noteOn, noteOff, CC, pitch bend, aftertouch.
  Unfiltered (always pass): transport events, internal events.

  CPU: negligible. Simple integer/bit comparisons per MIDI event.

  Parameters:
    channelNumber (1-16, default 1) - MIDI channel to allow when MPE is disabled
    mpeStart (2-16, default 2) - first channel in the MPE range when MPE is enabled
    mpeEnd (2-16, default 16) - last channel in the MPE range when MPE is enabled

  Common mistakes:
    Setting mpeStart > mpeEnd produces an empty range (only channel 1 passes).
    Transport events are not filtered.

  See also: (none)
---

::category-tags
---
tags:
  - { name: routing, desc: "Modules that forward, distribute, or proxy signals or events across the module tree" }
  - { name: note_processing, desc: "MIDI processors that transform, filter, or react to incoming note events" }
---
::

![Channel Filter screenshot](/images/v2/reference/audio-modules/channelfilter.png)

The Channel Filter passes or blocks incoming MIDI events based on their channel number. It operates in two mutually exclusive modes determined by the global MPE state: single-channel mode (when MPE is off) allows only one selected channel, while MPE range mode (when MPE is on) allows a configurable range of channels plus the MPE master channel (channel 1).

Events that pass the filter continue unchanged - no channel remapping or transformation occurs. Non-matching events are silently skipped by downstream processors.

## Signal Path

::signal-path
---
glossary:
  parameters:
    channelNumber:
      desc: "MIDI channel to allow (active when MPE is disabled)"
      range: "1 - 16"
      default: "1"
    mpeStart:
      desc: "First channel in the allowed MPE range (active when MPE is enabled)"
      range: "2 - 16"
      default: "2"
    mpeEnd:
      desc: "Last channel in the allowed MPE range (active when MPE is enabled)"
      range: "2 - 16"
      default: "16"
  functions:
    ignoreEvent:
      desc: "Marks the event so it is silently skipped by all downstream processors"
---

```
// Channel Filter - MIDI channel gate
// MIDI event in -> pass or ignore -> MIDI event out

onMidiEvent(message) {
    if MPE disabled:
        if message.channel != channelNumber:
            ignoreEvent(message)

    if MPE enabled:
        if message.channel not in range mpeStart..mpeEnd
        and message.channel != 1:
            ignoreEvent(message)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Single-Channel Mode
    params:
      - { name: channelNumber, desc: "MIDI channel to allow through the filter. Only active when MPE is disabled.", range: "1 - 16", default: "1" }
  - label: MPE Range Mode
    params:
      - { name: mpeStart, desc: "First channel in the allowed range. Only active when MPE is enabled.", range: "2 - 16", default: "2" }
      - { name: mpeEnd, desc: "Last channel in the allowed range (inclusive). Only active when MPE is enabled.", range: "2 - 16", default: "16" }
---
::

### Mode Selection

The active mode is determined by the global MPE enabled state, not by a parameter on the module itself. When MPE is enabled system-wide, the Channel Filter switches to range mode automatically.

### MPE Master Channel

In MPE range mode, channel 1 (the MPE master channel) is always allowed through the filter.

### Filtered Event Types

The filter applies to note-on, note-off, CC, pitch bend, and aftertouch events. Transport events (song position, MIDI start, MIDI stop) and internal events pass through unconditionally.

### Invalid MPE Range

If `mpeStart` is set higher than `mpeEnd`, the range is empty and only channel 1 passes through in MPE mode.
