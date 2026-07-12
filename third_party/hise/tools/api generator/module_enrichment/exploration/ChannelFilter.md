# MIDI Channel Filter - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 598-739)
**Base class:** `HardcodedScriptProcessor` (which extends `ScriptBaseMidiProcessor`)
**Listener:** `MidiControllerAutomationHandler::MPEData::Listener`

## Signal Path

ChannelFilter is a HardcodedScriptProcessor that filters MIDI events by channel. It has two mutually exclusive operating modes determined by the parent chain's MPE enabled state:

- **Single-channel mode** (MPE off): Only events matching the selected `channel` number pass through. All others are ignored via `Message.ignoreEvent(true)`.
- **MPE mode** (MPE on): Events on channels within the `mpeRange` bitmask pass through. Channel 1 (the MPE master channel) is always allowed after any parameter change.

MIDI event in -> [MPE mode check] -> [channel match check] -> pass or ignore -> MIDI event out

Events that fail the channel check are marked with `ignoreEvent(true)`. Events that pass are forwarded **unchanged** - no channel remapping or transformation occurs.

## Gap Answers

### event-filtering-logic

**Question:** How does processHiseEvent() filter MIDI events?

**Answer:** The base class `HardcodedScriptProcessor::processHiseEvent()` (HardcodedScriptProcessor.cpp:243-303) dispatches events by type to virtual callbacks. For each event, it first resets `Message.ignoreEvent(false)`, then calls `onNoteOn()`, `onNoteOff()`, or `onController()` depending on the event type.

ChannelFilter's three callbacks (`onNoteOn`, `onNoteOff`, `onController`) all share identical logic:
1. If `mpeEnabled` is true: check `mpeRange[Message.getChannel()-1]`. If the bit is not set, call `Message.ignoreEvent(true)`.
2. If `mpeEnabled` is false: check `Message.getChannel() != channel`. If they don't match, call `Message.ignoreEvent(true)`.

Events that pass the filter are forwarded **unchanged** - no transformation or remapping. The filter is purely a gate: pass or ignore.

### mpe-mode-toggle

**Question:** How is MPE mode activated?

**Answer:** ChannelFilter implements `MidiControllerAutomationHandler::MPEData::Listener` and registers itself in the constructor (line 612). The `mpeModeChanged(bool isEnabled)` callback (line 647-650) sets the `mpeEnabled` member variable. This means MPE mode is determined by the **global MPE state** of the MainController's MIDI automation handler, not by any parameter on the module itself. There is no explicit MPE toggle parameter - it follows the system-wide MPE enabled state.

When `mpeEnabled` is false, the `channelNumber` parameter is active. When `mpeEnabled` is true, the `mpeStart`/`mpeEnd` parameters control which channels pass.

### event-type-scope

**Question:** Does the filter apply to all MIDI event types?

**Answer:** The filter applies to:
- **NoteOn** events (via `onNoteOn()`)
- **NoteOff** events (via `onNoteOff()`)
- **Controller, PitchBend, and Aftertouch** events (all three types are dispatched to `onController()` by the base class switch at lines 261-264)

The following event types are **NOT filtered** (they pass through unconditionally):
- TimerEvent (handled separately by base class)
- AllNotesOff (dispatched to `onAllNotesOff()` which is not overridden - default is no-op)
- SongPosition, MidiStart, MidiStop, VolumeFade, PitchFade, ProgramChange (all fall through to `break` with no callback)

So all musically relevant MIDI event types (notes, CCs, pitchbend, aftertouch) are filtered. Transport and internal events pass through.

### mpe-range-edge-cases

**Question:** What happens when mpeStart > mpeEnd? Does channel 1 receive special treatment?

**Answer:** In `onControl()` (line 708-724), when either MPE parameter changes:
1. `startValue = mpeStartChannel->getValue() - 1` (0-indexed)
2. `endValue = mpeEndChannel->getValue() - 1` (0-indexed)
3. `mpeRange.clear()` (all bits cleared)
4. `mpeRange.setRange(startValue, (endValue - startValue) + 1, true)`
5. `mpeRange.setBit(0, true)` - **always allows channel 1** (the MPE master channel)

When mpeStart > mpeEnd, `setRange` receives a negative count and is a no-op (JUCE's `BigInteger::setRange` uses `while (--numBits >= 0)` which exits immediately for negative values). Only channel 1 would pass through.

**Channel 1 special treatment:** Yes, after any MPE parameter change, channel 1 (bit 0) is always force-enabled. However, there is an initialization inconsistency: `onInit()` sets `mpeRange.setRange(1, 15, true)` which sets bits 1-15 (channels 2-16) but does NOT set bit 0 (channel 1). So on first load before any parameter change, channel 1 events are filtered in MPE mode. See Issues section.

### description-mpe-optional

**Question:** Is MPE filtering truly optional with single-channel as default?

**Answer:** Confirmed. The default state is:
- `mpeEnabled = false` (member initializer, line 733)
- `channel = 1` (set in `onInit()`, line 641)

So the default mode is single-channel filtering on channel 1. MPE mode only activates when the global MPE state is enabled. The description "optional MPE start and end channel ranges" is accurate - MPE filtering is optional and only active when the system enables MPE.

## Processing Chain Detail

1. **Event dispatch** (negligible): Base class `processHiseEvent()` dispatches by event type to the appropriate callback
2. **Mode check** (negligible): Branch on `mpeEnabled` boolean
3. **Channel match** (negligible): Either integer comparison (`channel != Message.getChannel()`) or BigInteger bit lookup (`mpeRange[channel-1]`)
4. **Ignore/pass** (negligible): Call `Message.ignoreEvent(true)` for non-matching events; matching events pass unchanged

## Conditional Behavior

- **mpeEnabled = false** (default): Single-channel mode. Only events on `channel` (1-16) pass. All other channels are ignored.
- **mpeEnabled = true**: MPE range mode. Events on channels within the `mpeRange` bitmask pass. Channel 1 is always allowed (after first parameter change). The range is set from `mpeStart` to `mpeEnd` inclusive.
- **mpeStart > mpeEnd**: The range calculation produces a negative count, `setRange` is a no-op, only channel 1 passes (after parameter change).

## Vestigial / Notable

**Initialization inconsistency for MPE channel 1:** In `onInit()`, `mpeRange` is set to bits 1-15 (channels 2-16) but bit 0 (channel 1) is not set. The `onControl()` handler always sets bit 0 after any MPE parameter change. This means on first load (before the user touches any MPE parameter), MPE mode would filter out channel 1 events. After any MPE parameter change, channel 1 is correctly allowed.

## CPU Assessment

- **Overall baseline:** negligible
- All operations are simple integer comparisons or bit lookups, executed once per MIDI event
- No per-sample processing, no DSP, no allocations
- CPU cost is independent of parameter values

## UI Components

No FloatingTile content types. The module uses the default HardcodedScriptProcessor editor which renders the three ScriptSlider knobs defined in `onInit()` (channelNumber, mpeStart, mpeEnd).

## Notes

- ChannelFilter is a HardcodedScriptProcessor (C++ class implementing the ScriptProcessor pattern) rather than a native MidiProcessor subclass. This means it uses the `Message` scripting API wrapper and `Content` UI system rather than direct C++ parameter handling.
- The three callbacks (`onNoteOn`, `onNoteOff`, `onController`) contain identical filtering logic. This is typical for HardcodedScriptProcessors which mirror the ScriptProcessor callback pattern.
- The `mpeRange` is a `BigInteger` used as a 16-bit channel bitmask, where bit index = channel - 1 (0-indexed).
- The MPE listener is registered/unregistered in constructor/destructor, ensuring the `mpeEnabled` state stays synchronized with the global MPE configuration.
