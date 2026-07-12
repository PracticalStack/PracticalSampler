# ChokeGroupProcessor - C++ Exploration

**Source:** `hi_core/hi_modules/midi_processor/mps/Transposer.h` (lines 38-138), `Transposer.cpp` (lines 66-177)
**Base class:** `MidiProcessor`, `EventIdHandler::ChokeListener`

## Signal Path

MIDI event in -> all-notes-off/sustain-pedal handling -> key range filter (ignore out-of-range note-ons) -> active/sustained event tracking -> choke broadcast via EventIdHandler -> MIDI event out (passthrough for in-range events)

On the receiving side: when another ChokeGroupProcessor in the same group broadcasts a choke, `chokeMessageSent()` kills or sends note-offs for all tracked active and sustained events on the owner synth.

The processor operates in two roles simultaneously:
1. **Sender:** On note-on within key range, broadcasts a choke message to all other listeners in the same group
2. **Receiver:** When another processor in the same group sends a choke, kills/releases its own tracked voices

## Gap Answers

### choke-matching-mechanism

**Question:** How does the processor find and choke other ChokeGroupProcessors in the same group?

**Answer:** The matching uses a central registry on `EventIdHandler` (owned by `MainController`). Each ChokeGroupProcessor registers itself as a `ChokeListener` in its constructor via `mc->getEventHandler().addChokeListener(this)` and unregisters in the destructor. The `EventIdHandler` maintains an `Array<WeakReference<ChokeListener>> chokeListeners` (HiseEventBuffer.h:703).

When a processor calls `getMainController()->getEventHandler().sendChokeMessage(this, m)` (Transposer.cpp:150), the EventIdHandler iterates all registered listeners (HiseEventBuffer.cpp:1108-1119), skipping the source itself and any listener with a different `getChokeGroup()` value. Matching listeners receive `chokeMessageSent()`.

This is a flat global registry - it works across the entire module tree, not just siblings. Any two ChokeGroupProcessors anywhere in the hierarchy with the same group number will choke each other. There is no container scoping.

### event-processing-logic

**Question:** What exactly happens in processHiseEvent()?

**Answer:** `processHiseEvent()` (Transposer.cpp:110-151) performs these steps in order:

1. **All-notes-off:** If the event is an all-notes-off message, clears both `activeEvents` and `sustainedEvents` stacks and returns immediately.

2. **Sustain pedal tracking:** If the event is CC#64, updates `sustainPedalPressed` state. When the pedal is released, clears `sustainedEvents`.

3. **Key range filtering:** If the event is a note-on, gets the note number including transpose amount via `getNoteNumberIncludingTransposeAmount()`. If the note is outside `midiRange`, calls `m.ignoreEvent(true)` which marks the event as ignored, effectively blocking it from reaching downstream processors and the synth.

4. **Event tracking:** If the event is not ignored and `getChokeGroup() != 0`:
   - Note-on: adds the event to `activeEvents`
   - Note-off: removes the matching event (by event ID) from `activeEvents` and adds it to `sustainedEvents`

5. **Choke broadcast:** If the event is a note-on and not ignored, calls `sendChokeMessage(this, m)` on the EventIdHandler, which triggers `chokeMessageSent()` on all other listeners in the same group.

The original event passes through unchanged (except for the ignore flag on out-of-range notes). The processor does not modify, consume, or generate MIDI events in the event stream.

### kill-voice-vs-note-off

**Question:** When KillVoice is off, how is the note-off delivered?

**Answer:** In `chokeMessageSent()` (Transposer.cpp:153-177), the behavior differs based on `killVoice`:

- **KillVoice = true:** Calls `getOwnerSynth()->killAllVoicesWithNoteNumber(e.getNoteNumber())` for each tracked event. This instantly kills all voices playing that note number on the parent synth with no release phase.

- **KillVoice = false:** Constructs a synthetic `HiseEvent` of type `NoteOff` with the original note number, velocity 0, and the original channel and event ID, then calls `getOwnerSynth()->noteOff(off)`. This triggers the normal note-off path on the synth, allowing envelope release stages to play out naturally.

Both paths process `activeEvents` and `sustainedEvents` separately. For sustained events with KillVoice off, the note-off event from the `sustainedEvents` stack is passed directly to `noteOff()` (it was already a note-off when stored).

After processing all events, both stacks are cleared.

### key-range-filtering

**Question:** Does LoKey/HiKey filter incoming note-ons only, or does it also affect which voices get killed?

**Answer:** The key range filter (Transposer.cpp:127-133) only affects incoming note-ons. It checks `midiRange.contains(n)` and marks out-of-range notes as ignored. This has two effects:

1. The ignored note-on never reaches the parent synth (no voice starts)
2. The ignored note-on is excluded from event tracking (the `if (!m.isIgnored() && getChokeGroup() != 0)` guard at line 135 prevents it from being added to `activeEvents`)
3. The ignored note-on does not trigger a choke broadcast (the `if (!m.isNoteOn() || m.isIgnored()) return;` guard at line 147 prevents `sendChokeMessage`)

The kill side (`chokeMessageSent`) kills ALL tracked events regardless of note number - there is no key range check on the receiving end. So yes, two processors in the same group can have different key ranges to create partial choke behavior: processor A (range 36-36, kick) and processor B (range 42-42, closed hi-hat) in the same group would mean playing note 42 chokes any active note 36 voices on processor A's synth, and vice versa. Each processor only starts voices for notes in its own range but chokes affect all tracked voices.

### chokegroup-zero-behavior

**Question:** When ChokeGroup is 0, does the processor pass MIDI through unchanged?

**Answer:** When ChokeGroup is 0:

1. **Key range filtering still applies.** The key range check (line 127-133) runs before the group check. Notes outside the range are still ignored/blocked even with group 0.

2. **Event tracking is skipped.** The guard `if (!m.isIgnored() && getChokeGroup() != 0)` (line 135) prevents events from being added to `activeEvents` or `sustainedEvents`.

3. **Choke broadcast still executes but is a no-op.** `sendChokeMessage` is still called (line 150), but `EventIdHandler::sendChokeMessage()` (HiseEventBuffer.cpp:1110) checks `if (auto group = source->getChokeGroup())` which evaluates to 0 (falsy), so the loop body is skipped entirely.

4. **Cannot be choked.** Other processors' choke broadcasts skip listeners where `l->getChokeGroup() != group` (HiseEventBuffer.cpp:1114), and since group 0 never matches any non-zero group, this processor is never a choke target.

So group 0 is not fully transparent - it still acts as a key range filter. The processor is only truly neutral when group is 0 AND the key range is 0-127 (the defaults).

### note-passthrough

**Question:** Does the processor pass the original note-on through?

**Answer:** Yes, with one exception. The processor never modifies MIDI event data (note number, velocity, channel, etc.). It only calls `m.ignoreEvent(true)` on note-ons outside the key range, which marks them as ignored. All other events - including in-range note-ons, note-offs, CCs, and everything else - pass through the processor unchanged. The choke mechanism operates as a side-effect (calling methods on the owner synth and other listeners), not by modifying the event stream.

## Processing Chain Detail

1. **All-notes-off handler** (negligible): Clears both event stacks on all-notes-off message. Early return.
2. **Sustain pedal tracker** (negligible): Updates internal sustain state on CC#64, clears sustained events on pedal release.
3. **Key range filter** (negligible): Checks note-on note number against LoKey/HiKey range. Marks out-of-range notes as ignored. Parameters: LoKey, HiKey.
4. **Event tracker** (negligible): Maintains two `UnorderedStack<HiseEvent, NUM_POLYPHONIC_VOICES>` stacks for active and sustained events. Only active when ChokeGroup != 0.
5. **Choke broadcast** (negligible): Calls `EventIdHandler::sendChokeMessage()` which iterates the global listener list. Parameter: ChokeGroup.

On the receiving side (called externally via `chokeMessageSent()`):
6. **Voice killer** (negligible): Iterates tracked events and either kills voices instantly or sends synthetic note-offs. Parameter: KillVoice.

## Conditional Behavior

- **ChokeGroup = 0**: Event tracking and choke broadcast are effectively disabled. Key range filtering still applies.
- **ChokeGroup = 1-16**: Full choke behavior active. Processor participates in both sending and receiving choke messages within its group.
- **KillVoice = On (default)**: Choked voices are killed instantly via `killAllVoicesWithNoteNumber()` - no release tail.
- **KillVoice = Off**: Choked voices receive a synthetic note-off, triggering normal envelope release for a natural fade-out.
- **Note outside LoKey-HiKey**: Note-on is ignored (blocked). No voice starts, no event tracking, no choke broadcast for that note.
- **Sustain pedal held**: Note-offs move events from `activeEvents` to `sustainedEvents`. Both stacks are choked when a choke message arrives. Pedal release clears `sustainedEvents`.

## CPU Assessment

- **Overall baseline**: negligible
- All operations are simple comparisons, stack lookups, and integer arithmetic
- No per-sample processing, no DSP, no allocations
- The choke broadcast iterates the global listener list (typically very small - one per choke group instance)
- `UnorderedStack` operations are O(n) where n is the number of active voices, but with a fixed upper bound of `NUM_POLYPHONIC_VOICES`

## UI Components

- Backend editor: `ChokeGroupEditor` (TransposerEditor.cpp:145-184) - provides ChokeGroup slider, LoKey slider, HiKey slider, and KillVoice toggle button. No FloatingTile content types.

## Notes

- The processor uses `getNoteNumberIncludingTransposeAmount()` for the key range check, meaning upstream transposition (e.g., from a Transposer processor) is respected when filtering notes.
- The sustain pedal tracking means that voices held by the sustain pedal are still choked when a choke message arrives, which is correct behavior for drum instruments.
- The `UnorderedStack` has a fixed capacity of `NUM_POLYPHONIC_VOICES`. If this limit is exceeded, `insertWithoutSearch` behavior depends on the stack implementation (likely silently drops the event).
- The choke mechanism is global across the entire module tree via `EventIdHandler`. There is no way to scope choke groups to a specific container or synth chain. Two ChokeGroupProcessors in completely separate synth chains with the same group number will choke each other.
- `chokeMessageSent()` calls `getOwnerSynth()` to kill voices on the parent synth only. It does not affect voices on other synths, even though the choke message comes from a processor potentially in a different synth chain. This is the correct behavior - each processor manages its own synth's voices.
