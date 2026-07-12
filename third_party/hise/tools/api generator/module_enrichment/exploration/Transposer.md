# Transposer - C++ Exploration

**Source:** `hi_core/hi_modules/midi_processor/mps/Transposer.h` (lines 140-210), `Transposer.cpp` (lines 35-64)
**Base class:** `MidiProcessor`

## Signal Path

Transposer intercepts note-on events and adds a semitone offset via the HiseEvent transpose mechanism. It does not modify the note number directly - instead it accumulates into the event's `transposeValue` field. Note-off events are not processed by Transposer; HISE's EventIdHandler automatically copies the transpose amount from the matching note-on to the note-off when pairing them by event ID.

MIDI event in -> [if note-on: add transposeAmount to event's transposeValue] -> MIDI event out

## Gap Answers

### event-types-affected

**Question:** Does processHiseEvent() transpose only note-on events, or does it also transpose the corresponding note-off events to ensure matched pairs? Does it affect any other MIDI event types?

**Answer:** `processHiseEvent()` (Transposer.h:198-204) only processes note-on events (`m.isNoteOn()`). All other event types (note-off, CC, pitch bend, etc.) pass through unmodified. However, note-off events do receive the correct transpose amount automatically: HISE's `EventIdHandler::pushArtificialNoteOn()` (HiseEventBuffer.cpp:997-1019) matches note-off events to their corresponding note-on by note number and channel, then copies the transpose amount from the stored note-on to the note-off via `m->setTransposeAmount(on->getTransposeAmount())`. This means the Transposer does not need to handle note-offs - the infrastructure ensures matched pairs. The `toMidiMesage()` conversion (HiseEventBuffer.cpp:92-93) adds `transposeValue` to the note number for both note-on and note-off MIDI output.

### realtime-vs-noteon

**Question:** If TransposeAmount is changed while notes are held, do the currently sounding notes shift, or does the new value only apply to subsequent note-on events?

**Answer:** The new value only applies to subsequent note-on events. The transpose amount is read from the `transposeAmount` member and written to the event's `transposeValue` field at note-on time in `processHiseEvent()`. Once a note-on has been processed, its transpose value is baked into the HiseEvent. Changing the parameter later does not retroactively modify already-dispatched events. The `setInternalAttribute()` (Transposer.h:193-195) simply stores the new integer value - there is no mechanism to update in-flight voices. This makes it safe to automate, but the effect is quantised to note boundaries.

### boundary-clamping

**Question:** What happens when transposition pushes a note number outside the valid MIDI range (0-127)? Is the note clamped, wrapped, or discarded?

**Answer:** There is no clamping, wrapping, or discarding in the Transposer itself. `setTransposeAmount()` (HiseEventBuffer.h:197) stores the value as `int8`, and `getNoteNumberIncludingTransposeAmount()` (HiseEventBuffer.h:324) returns `(int)number + getTransposeAmount()` as a plain int addition with no range check. The result can exceed 0-127. Downstream consumers that use `getNoteNumberIncludingTransposeAmount()` (samplers, synths, global modulators) receive the unclamped value. Whether this causes issues depends on the downstream module - for example, sampler sound matching and array lookups may behave unexpectedly with out-of-range values. The Transposer's parameter range (-24 to +24) limits the worst case to note 127+24=151 or 0-24=-24.

### description-noteoff-omission

**Question:** The base description says 'Transposes incoming MIDI note-on events'. If note-off events are also transposed (as expected), the description should say 'MIDI note events' or 'note-on and note-off events'.

**Answer:** The description is technically accurate at the processor level - `processHiseEvent()` truly only modifies note-on events. The note-off transpose propagation happens in the EventIdHandler, not in the Transposer. However, from a user perspective the description is misleading because it implies note-offs might not be transposed. A more accurate user-facing description would be: "Transposes MIDI notes by a fixed number of semitones" since the end result (via EventIdHandler propagation) is that both note-on and note-off are effectively transposed. The current metadata description in `createMetadata()` (Transposer.cpp:42) reads "Transposes incoming MIDI note-on events by a fixed number of semitones for quick key changes or interval shifts".

## Processing Chain Detail

1. **Event type check** (negligible): Check if event is note-on
2. **Transpose accumulation** (negligible): Add `transposeAmount` to the event's existing transpose value via `setTransposeAmount(getTransposeAmount() + transposeAmount)`

The use of `getTransposeAmount() + transposeAmount` means multiple Transposer modules in a chain accumulate their offsets. Each adds to whatever transpose value is already on the event.

## Conditional Behavior

- **Non-note-on events**: All events that are not note-on pass through completely unmodified
- **TransposeAmount = 0**: Note-on events still have `setTransposeAmount` called, adding 0 to the existing value (effectively a no-op but not short-circuited)

## CPU Assessment

- **Overall baseline**: negligible
- No per-sample processing, no buffers, no allocations
- Single integer addition per note-on event
- No parameters that scale cost

## UI Components

- Backend editor: `TransposerEditor` (TransposerEditor.cpp) - provides a single `HiSlider` ("Transpose") for the TransposeAmount parameter

## Notes

- The transpose mechanism uses `setTransposeAmount` rather than `setNoteNumber`, which is the recommended HISE pattern. This preserves the original note number for event matching while allowing downstream DSP to read the effective note number via `getNoteNumberIncludingTransposeAmount()`.
- Multiple Transposer modules in the MIDI chain stack additively since each reads and adds to the event's existing transpose value.
- The `transposeValue` field is `int8` (-128 to 127), so extreme stacking of multiple Transposers could overflow, though the practical parameter range of -24 to +24 makes this unlikely.
