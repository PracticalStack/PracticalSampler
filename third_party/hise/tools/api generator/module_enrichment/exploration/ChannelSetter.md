# ChannelSetter - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 741-801)
**Base class:** `HardcodedScriptProcessor` (extends `ScriptBaseMidiProcessor`)

## Signal Path

ChannelSetter is a HardcodedScriptProcessor that rewrites the MIDI channel of incoming events. Events arrive via the base class `processHiseEvent()` which dispatches to callback overrides. Each callback calls `Message.setChannel(channel)` to overwrite the event's channel in-place. The channel value comes from a single integer member set by the UI knob.

MIDI event in -> setChannel(channel) -> MIDI event out (same event, channel rewritten)

The rewrite covers NoteOn, NoteOff, Controller, PitchBend, and Aftertouch events. ProgramChange events pass through without channel modification (the base class switch statement has a no-op case for ProgramChange). AllNotesOff events also pass through unmodified (the default `onAllNotesOff()` is a no-op).

## Gap Answers

### message-types-affected

**Question:** Which MIDI message types does ChannelSetter rewrite?

**Answer:** ChannelSetter rewrites the channel for five event types: NoteOn, NoteOff, Controller, PitchBend, and Aftertouch. This is determined by the base class `HardcodedScriptProcessor::processHiseEvent()` (HardcodedScriptProcessor.cpp:253-289), which dispatches:

- `HiseEvent::Type::NoteOn` -> `onNoteOn()` -> `Message.setChannel(channel)`
- `HiseEvent::Type::NoteOff` -> `onNoteOff()` -> `Message.setChannel(channel)`
- `HiseEvent::Type::Controller`, `PitchBend`, `Aftertouch` -> `onController()` -> `Message.setChannel(channel)`

**Not rewritten:** ProgramChange falls through the switch with a no-op `break;`. AllNotesOff calls the default empty `onAllNotesOff()`. Transport events (SongPosition, MidiStart, MidiStop), VolumeFade, PitchFade, and Empty are also not rewritten.

### processing-method

**Question:** How is the channel rewrite implemented in processHiseEvent()?

**Answer:** ChannelSetter does not override `processHiseEvent()` directly. The base class `HardcodedScriptProcessor::processHiseEvent()` handles event dispatch. It sets `currentEvent` and populates the `Message` wrapper, then switches on the event type to call the appropriate virtual callback. ChannelSetter overrides `onNoteOn()`, `onNoteOff()`, and `onController()`, each containing a single call: `Message.setChannel(channel)`.

`Message.setChannel()` (ScriptingApi.cpp:829-846) validates the channel is 1-16, then calls `messageHolder->setChannel(newValue)` which directly sets the `channel` field on the underlying `HiseEvent` (a uint8 cast). Since `messageHolder` points to the live event being processed, the channel rewrite is immediate and in-place - no copy is made.

### channel-parameter-application

**Question:** Is the channelNumber parameter applied in real time? Does changing it mid-stream cause orphaned noteOff messages?

**Answer:** The `channel` member variable is updated immediately when the knob is changed, via `onControl()` (line 789-792): `channel = value`. Subsequent events use the new value.

**Orphaned notes are possible.** If channel is changed between a NoteOn and its corresponding NoteOff, the NoteOn was sent on the old channel while the NoteOff is sent on the new channel. The synth listening on the old channel never receives a NoteOff for that note, causing a stuck note. This is inherent to the design - there is no tracking of in-flight notes or deferred channel switching. The same behavior occurs in the equivalent HISEScript pattern (`Message.setChannel()` in onNoteOn/onNoteOff callbacks).

## Processing Chain Detail

1. **Event dispatch** (negligible): Base class `processHiseEvent()` sets up Message wrapper and switches on event type
2. **Channel rewrite** (negligible): Single `setChannel()` call writing a uint8 to the event struct
3. **Parameter update** (negligible): `onControl()` stores integer value to member variable

## Conditional Behavior

- **Event type filtering**: Only NoteOn, NoteOff, Controller, PitchBend, and Aftertouch events are rewritten. ProgramChange and transport events pass through unchanged. This filtering is in the base class dispatch, not in ChannelSetter itself.

## CPU Assessment

- **Overall baseline**: negligible
- **Per-event cost**: A single integer assignment (uint8 write) per event
- **No scaling factors**: CPU cost is constant regardless of parameter value or event rate

## UI Components

None. Uses the default `HardcodedScriptProcessor` editor, which auto-generates UI from the Content components defined in `onInit()` (a single knob for channelNumber).

## Notes

- ChannelSetter is a HardcodedScriptProcessor, not a standalone C++ MidiProcessor class. It uses the scripting API (`Message`, `Content`) in C++ rather than implementing `processHiseEvent()` directly. This is a pattern used for simple MIDI processors that were originally scripts.
- The class docstring (line 744) mentions "256 MIDI channels in HISE" for advanced routing, referring to the HiseEvent uint8 channel field (0-255). However, both the scripting API `Message.setChannel()` and the ChannelSetter knob restrict the range to 1-16 (standard MIDI). The 256-channel capability is only accessible via lower-level APIs, not through this module.
- The `channel` member is an `int` but is always set from a knob with range 1-16 with step 1. No clamping is applied in `onControl()` beyond the knob's own range enforcement.
