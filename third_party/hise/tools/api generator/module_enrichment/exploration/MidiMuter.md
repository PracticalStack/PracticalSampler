# MidiMuter - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 803-885), `HardcodedScriptProcessor.cpp` (lines 147-164, 243-303)
**Base class:** `HardcodedScriptProcessor` (-> `ScriptBaseMidiProcessor` -> `MidiProcessor`)

## Signal Path

MidiMuter is a hardcoded script processor that selectively blocks note events based on a mute toggle. When `ignore` is active, note-on events are always blocked. Note-off events are blocked unless `fix` (fixStuckNotes) is enabled AND the note was previously heard before mute was engaged. All non-note MIDI messages (CC, pitch bend, aftertouch, program change, etc.) always pass through unmodified.

MIDI event -> [dispatch by type] -> noteOn: block if muted -> noteOff: block if muted (unless fix + was heard) -> all else: passthrough -> MIDI out

## Gap Answers

### event-filtering-logic

**Question:** What exactly does processHiseEvent() do? Does ignoreButton=1 block only noteOn events, or also noteOff? What about other MIDI messages?

**Answer:** The base class `HardcodedScriptProcessor::processHiseEvent()` (HardcodedScriptProcessor.cpp:243-303) dispatches by `HiseEvent::Type`:

- **NoteOn** -> `onNoteOn()`: If `ignore==true`, calls `Message.ignoreEvent(true)` to block the event. Regardless of mute state, sets `noteOns.setBit(noteNumber, true)` to track the note as heard.
- **NoteOff** -> `onNoteOff()`: If `ignore==true`, blocks the event UNLESS both `fix==true` AND `noteOns[noteNumber]==true` (the note was previously heard). Always clears `noteOns.setBit(noteNumber, false)`.
- **Controller, PitchBend, Aftertouch** -> `onController()`: MuteAllScriptProcessor does NOT override `onController()`. The base class provides an empty no-op, so these events pass through completely unmodified.
- **ProgramChange, SongPosition, MidiStart, MidiStop, VolumeFade, PitchFade, Empty** -> no callback invoked, events pass through unmodified.
- **AllNotesOff** -> `onAllNotesOff()`: Not overridden, base no-op, passes through.
- **TimerEvent** -> only consumed if it matches this processor's chain index.

So with `ignoreButton=1`: note-on events are always blocked; note-off events are also blocked by default (unless fixStuckNotes allows them through). CC, pitch bend, aftertouch, and all other non-note messages always pass through.

### stuck-note-tracking

**Question:** How does fixStuckNotes track which notes were 'previously heard'? When fixStuckNotes=0 and ignoreButton=1, are note-offs also blocked?

**Answer:** The module maintains a `BigInteger noteOns` bitmask (128 bits, one per MIDI note number). In `onNoteOn()`, the bit for the incoming note number is always set to true, regardless of mute state. In `onNoteOff()`, the bit is always cleared to false.

The tracking logic is straightforward:
1. Before mute engages, notes that are sounding have their bits set in `noteOns`
2. When mute is active and a note-off arrives, the code checks `noteOns[noteNumber]`
3. If the bit is set, this note was heard before (or during a brief unmuted window), so the note-off is allowed through when `fix==true`
4. If the bit is not set, the note-on was already blocked while muted, so the note-off is also blocked

When `fixStuckNotes=0` and `ignoreButton=1`: Yes, ALL note-offs are blocked (the `!fix` condition in `onNoteOff` short-circuits the check). This is the scenario that causes stuck notes - notes that were sounding when mute was engaged will never receive their note-off.

### mute-toggle-midstream

**Question:** What happens when ignoreButton is toggled while notes are sounding?

**Answer:** The module does NOT send artificial note-offs or note-ons on transitions. The `onControl` callback (line 862-872) simply updates the `ignore` boolean. There is no transition handling.

- **Mute engages (ignore: false->true)**: Currently sounding notes continue to sound. Their note-offs will be blocked unless `fixStuckNotes` is enabled. If fixStuckNotes is on, the note-offs pass through because those notes have their `noteOns` bits set (they were heard).
- **Mute disengages (ignore: true->false)**: No artificial note-ons are sent for held keys. Notes that were being held on the input during the muted period had their note-ons blocked, so when mute disengages, those keys are "invisible" - the module has no mechanism to retroactively trigger them.

The `noteOns` bitmask provides passive tracking only. It does not drive any active note management on state transitions.

### fix-stuck-notes-interaction

**Question:** Does fixStuckNotes have any effect when ignoreButton=0?

**Answer:** No. When `ignore==false`, neither `onNoteOn()` nor `onNoteOff()` call `Message.ignoreEvent()`. The `fix` variable is only checked inside the `if (ignore)` block in `onNoteOff()`. So fixStuckNotes is purely a "while muted" behavior - it determines whether note-offs for previously-heard notes pass through during the muted state. It has no effect on unmute transitions or normal (unmuted) operation.

### description-verify-other-midi

**Question:** Verify that CC, pitch bend, aftertouch, and program change messages pass through unmodified when muted.

**Answer:** Confirmed. The base class `processHiseEvent()` dispatches Controller, PitchBend, and Aftertouch to `onController()`, which MuteAllScriptProcessor does not override (the base no-op runs). ProgramChange falls through the switch to the default no-op case. In all cases, `Message.ignoreEvent(false)` is called at the top of `processHiseEvent()` (line 250), resetting the ignore flag before dispatch. Since no callback sets it back to true for non-note events, they always pass through. The description "allowing other MIDI messages" is accurate.

## Processing Chain Detail

1. **Event dispatch** (negligible): Base class sets up Message wrapper, resets ignore flag, dispatches by event type
2. **Note-on gate** (negligible): Single boolean check on `ignore`, optionally marks event as ignored. Always updates `noteOns` bitmask.
3. **Note-off gate** (negligible): Checks `ignore`, then checks `fix` and `noteOns` bitmask. Optionally marks event as ignored. Always clears `noteOns` bit.
4. **Non-note passthrough** (negligible): No processing, events pass through unchanged

## Conditional Behavior

- **ignoreButton=0 (mute off)**: All events pass through. `noteOns` bitmask is still updated (tracking is always active) but never consulted for gating.
- **ignoreButton=1, fixStuckNotes=0**: Note-ons blocked. ALL note-offs blocked. CC/pitchbend/aftertouch pass through. This WILL cause stuck notes if notes were sounding when mute engaged.
- **ignoreButton=1, fixStuckNotes=1**: Note-ons blocked. Note-offs for previously-heard notes pass through. Note-offs for notes whose note-on was already blocked are also blocked (correct behavior - no orphan note-offs). CC/pitchbend/aftertouch pass through.

## CPU Assessment

- **Overall baseline**: negligible
- All operations are simple boolean checks and single-bit operations on a BigInteger
- No per-sample processing, no buffers, no allocations
- Cost is constant regardless of parameter values

## Notes

- The class is named `MuteAllScriptProcessor` in C++ but registered as `"MidiMuter"` via `SET_PROCESSOR_NAME`. The Doxygen comment (line 803) describes it as muting "incoming note-on messages" which is accurate.
- The `noteOns` bitmask is always updated regardless of mute state. This means that when mute is first engaged, the bitmask already has an accurate picture of which notes are currently sounding - no state is lost at the transition point.
- The `BigInteger` type provides 128+ bits, covering all MIDI note numbers (0-127). Each bit represents one note number, not one voice - if the same note is triggered twice, there is only one bit. This is sufficient for the use case since MIDI note-off matching is by note number.
