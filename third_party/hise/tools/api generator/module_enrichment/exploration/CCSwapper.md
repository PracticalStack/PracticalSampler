# CC Swapper - C++ Exploration

**Source:** `hi_scripting/scripting/HardcodedScriptProcessor.h` (lines 278-322), `HardcodedScriptProcessor.cpp` (lines 42-59)
**Base class:** `HardcodedScriptProcessor` (which extends `MidiProcessor`)

## Signal Path

CCSwapper is one of the simplest MIDI processors. It intercepts CC events and performs a bidirectional swap of two controller numbers. All other MIDI events pass through unchanged.

MIDI in -> [if CC: check number against FirstCC/SecondCC, swap if matched] -> MIDI out

## Gap Answers

### swap-logic

**Question:** How does processHiseEvent() implement the swap?

**Answer:** CCSwapper overrides `onController()` (HardcodedScriptProcessor.h:304-313), which is called by the base class `processHiseEvent()` (HardcodedScriptProcessor.cpp:261-264) for CC, PitchBend, and Aftertouch events.

The swap logic is a simple if/else-if chain:
1. If `Message.getControllerNumber()` equals `firstCC->getValue()`, set it to `secondCC->getValue()`
2. Else if `Message.getControllerNumber()` equals `secondCC->getValue()`, set it to `firstCC->getValue()`
3. Otherwise, the event passes through unmodified

The mutation is in-place: `Message.setHiseEvent(m)` stores a pointer to the original `HiseEvent&` (ScriptingApi.cpp:1123-1126), and `Message.setControllerNumber()` calls `messageHolder->setControllerNumber()` directly on that pointer (ScriptingApi.cpp:608), so the swap modifies the event that propagates downstream.

### non-cc-passthrough

**Question:** Are non-CC MIDI events passed through unchanged?

**Answer:** Yes. The base class `processHiseEvent()` (HardcodedScriptProcessor.cpp:253-290) dispatches by event type:
- NoteOn/NoteOff -> `onNoteOn()`/`onNoteOff()` (CCSwapper does not override these - base implementations are empty)
- Controller/PitchBend/Aftertouch -> `onController()` (CCSwapper overrides this)
- All other types (SongPosition, MidiStart, MidiStop, ProgramChange, etc.) -> no handler, pass through

For PitchBend and Aftertouch events, although they reach `onController()`, `Message.getControllerNumber()` returns 128 for PitchBend and 129 for Aftertouch (per the API documentation in ScriptingApi.h:80-84). Since FirstCC and SecondCC have a range of 0-127, these events can never match and pass through unmodified. Additionally, `setControllerNumber()` has a safe check (`isController()`) that would reject non-CC events in safe-check builds.

### same-value-behavior

**Question:** What happens when FirstCC and SecondCC are set to the same value?

**Answer:** It's a harmless no-op. The first `if` branch matches (the CC number equals FirstCC), and sets the controller number to SecondCC - which is the same value. The event is effectively unchanged. This includes the default state where both are 0: CC#0 (Bank Select MSB) would match the first branch and get "swapped" to CC#0.

### cc-value-preservation

**Question:** Does the swap preserve the CC value unchanged?

**Answer:** Yes. Only `setControllerNumber()` is called - the CC value (data byte 2) is never read or modified. The event's value, channel, timestamp, and all other properties are preserved exactly as received.

## Processing Chain Detail

1. **Event dispatch** (negligible): Base class `processHiseEvent()` checks event type and routes to `onController()`
2. **Controller number comparison** (negligible): Two integer comparisons against parameter values
3. **Controller number swap** (negligible): Single `setControllerNumber()` call on the matching branch, directly mutating the HiseEvent

## Conditional Behavior

- **Incoming CC matches FirstCC**: Controller number changed to SecondCC value
- **Incoming CC matches SecondCC**: Controller number changed to FirstCC value
- **Incoming CC matches neither**: Event passes through unchanged
- **PitchBend/Aftertouch events**: Reach `onController()` but never match (numbers 128/129 outside parameter range 0-127)
- **NoteOn/NoteOff/other events**: Never reach `onController()`, pass through unchanged
- **FirstCC == SecondCC**: No-op (swap to same value)

## CPU Assessment

- **Overall baseline**: negligible
- All processing is two integer comparisons plus one conditional integer write per CC event
- No per-sample processing, no buffers, no allocations
- No parameters that scale cost
- Cost is event-rate, not sample-rate

## Notes

CCSwapper is a `HardcodedScriptProcessor` subclass, meaning it uses the scripting API objects (`Message`, `Content`, `Synth`) internally but is implemented in C++ rather than HiseScript. The `onInit()` method creates two knob controls with `Content.addKnob()` and sets their ranges to 0-127 with step size 1. The member variables `firstCC` and `secondCC` are `ScriptSlider*` pointers, and their values are read via `getValue()` during processing.
