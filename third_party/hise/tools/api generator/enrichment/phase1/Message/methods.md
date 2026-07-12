# Message -- Method Analysis

## getMonophonicAftertouchPressure

**Signature:** `int getMonophonicAftertouchPressure()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pressure = Message.getMonophonicAftertouchPressure();`

**Description:**
Returns the pressure value of a monophonic (channel pressure) aftertouch event. Only valid when called on a channel pressure event inside the `onController` callback. Reports an error if the current event is not a channel pressure event or if called outside a MIDI callback.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Channel pressure and polyphonic aftertouch both use the same underlying `Aftertouch` event type in HISE's event system. Use `Message.isMonophonicAfterTouch()` to distinguish them before calling this method.

**Cross References:**
- `$API.Message.isMonophonicAfterTouch$`
- `$API.Message.setMonophonicAfterTouchPressure$`
- `$API.Message.getControllerValue$`

## getNoteNumber

**Signature:** `int getNoteNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var note = Message.getNoteNumber();`

**Description:**
Returns the MIDI note number (0-127) of the current note-on or note-off event. Only valid inside `onNoteOn` or `onNoteOff` callbacks. Reports an error if the current event is not a note-on or note-off, or if called outside a MIDI callback. Returns the raw note number without transpose -- use `getNoteNumber() + getTransposeAmount()` if the transposed pitch is needed.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setNoteNumber$`
- `$API.Message.getTransposeAmount$`
- `$API.Message.getVelocity$`

## getPolyAfterTouchNoteNumber

**Signature:** `int getPolyAfterTouchNoteNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var note = Message.getPolyAfterTouchNoteNumber();`

**Description:**
Returns the note number associated with a polyphonic aftertouch event. Only valid when called on an aftertouch event inside the `onController` callback. Reports an error if the current event is not an aftertouch event or if called outside a MIDI callback.

**Parameters:**

(No parameters.)

**Pitfalls:**
- [BUG] This method accesses the mutable `messageHolder` pointer internally despite being a const getter. In read-only contexts (e.g., voice start modulators), the null check on `constMessageHolder` passes but the method dereferences the null `messageHolder`, causing undefined behavior. Use `getControllerNumber()` and `getControllerValue()` as safer alternatives for aftertouch data in read-only contexts.

**Cross References:**
- `$API.Message.getPolyAfterTouchPressureValue$`
- `$API.Message.isPolyAftertouch$`
- `$API.Message.setPolyAfterTouchNoteNumberAndPressureValue$`

## getPolyAfterTouchPressureValue

**Signature:** `int getPolyAfterTouchPressureValue()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pressure = Message.getPolyAfterTouchPressureValue();`

**Description:**
Returns the pressure value (0-127) of a polyphonic aftertouch event. Only valid when called on an aftertouch event inside the `onController` callback. Reports an error if the current event is not an aftertouch event or if called outside a MIDI callback.

**Parameters:**

(No parameters.)

**Pitfalls:**
- [BUG] This method accesses the mutable `messageHolder` pointer internally despite being a const getter. In read-only contexts (e.g., voice start modulators), the null check on `constMessageHolder` passes but the method dereferences the null `messageHolder`, causing undefined behavior. Same root cause as `getPolyAfterTouchNoteNumber`.

**Cross References:**
- `$API.Message.getPolyAfterTouchNoteNumber$`
- `$API.Message.isPolyAftertouch$`
- `$API.Message.setPolyAfterTouchNoteNumberAndPressureValue$`
- `$API.Message.getControllerValue$`

## getProgramChangeNumber

**Signature:** `int getProgramChangeNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pgm = Message.getProgramChangeNumber();`

**Description:**
Returns the program change number (0-127) of a MIDI program change event. Only valid inside the `onController` callback when the current event is a program change. Returns -1 if the current event is not a program change. Use `Message.isProgramChange()` to check the event type before calling this method.

**Parameters:**

(No parameters.)

**Pitfalls:**
- [BUG] The null-pointer guard checks `messageHolder` (mutable pointer) instead of `constMessageHolder`, and the error message incorrectly says `"setVelocity()"` instead of `"getProgramChangeNumber()"`. This means the method fails with a misleading error in read-only contexts, and the wrong method name appears in the error output.

**Cross References:**
- `$API.Message.isProgramChange$`

## getTimestamp

**Signature:** `int getTimestamp()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ts = Message.getTimestamp();`

**Description:**
Returns the sample-accurate timestamp of the current MIDI event. The timestamp is the sample offset from the start of the current audio buffer. Events with timestamps beyond the current buffer size are automatically deferred to a future buffer. The returned value has the artificial and ignored flag bits masked out -- it reflects only the sample position.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.delayEvent$`

## getTransposeAmount

**Signature:** `int getTransposeAmount()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var transpose = Message.getTransposeAmount();`

**Description:**
Returns the transpose amount in semitones applied to the current event. This is stored as a signed 8-bit integer in the HiseEvent, set via `setTransposeAmount()`. The transpose does not alter the raw note number returned by `getNoteNumber()` -- the transposed pitch is `getNoteNumber() + getTransposeAmount()`. When a note-off is automatically matched to a note-on by the EventIdHandler, the transpose amount from the note-on is copied to the note-off so voice release uses the correct pitch.

**Parameters:**

(No parameters.)

**Pitfalls:**
- In exported plugins (frontend builds), this method silently returns 0 instead of throwing an error when called outside a MIDI callback. Backend builds report an error via `reportIllegalCall()`. This behavioral difference means code that accidentally calls `getTransposeAmount()` outside a callback may work in the exported plugin but fail during development.

**Cross References:**
- `$API.Message.setTransposeAmount$`
- `$API.Message.getNoteNumber$`

## getVelocity

**Signature:** `int getVelocity()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var vel = Message.getVelocity();`

**Description:**
Returns the velocity (0-127) of the current note-on or note-off event. Only valid inside `onNoteOn` or `onNoteOff` callbacks. Reports an error if the current event is not a note event or if called outside a MIDI callback. The error message references `"onNoteOn"` but the method works in `onNoteOff` as well.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setVelocity$`
- `$API.Message.getNoteNumber$`
- `$API.Message.getGain$`

## delayEvent

**Signature:** `undefined delayEvent(Number samplesToDelay)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.delayEvent(512);`

**Description:**
Adds a sample offset to the current event's timestamp, delaying when it is processed by downstream modules. If the resulting timestamp exceeds the current audio buffer size, the event is automatically carried over to a future buffer via the buffer-splitting mechanism. The delay is additive relative to the event's existing timestamp. Negative delta values are clamped so the resulting timestamp never goes below zero. This method modifies the event in-place in the audio buffer and requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| samplesToDelay | Number | yes | Number of samples to add to the event's timestamp | Result clamped to >= 0 |

**Pitfalls:**
- Does not delay sample playback position -- only delays when the event is processed. Use `setStartOffset()` to skip ahead in the sample without delaying event processing.

**Cross References:**
- `$API.Message.getTimestamp$`
- `$API.Message.setStartOffset$`

## getChannel

**Signature:** `Integer getChannel()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ch = Message.getChannel();`

**Description:**
Returns the MIDI channel (1-16) of the current event. Unlike most Message getters, this method does not require a specific event type -- it works on any event type (NoteOn, NoteOff, Controller, PitchBend, Aftertouch, etc.). Only requires that the Message object has an active event pointer (i.e., called inside a MIDI callback).

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setChannel$`

## getCoarseDetune

**Signature:** `Integer getCoarseDetune()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var semitones = Message.getCoarseDetune();`

**Description:**
Returns the coarse detune amount in semitones stored on the current event. This is a per-event property stored as an `int8` (-128 to 127), independent of the note number. Sound generators use this value alongside fine detune and transpose amount to calculate the final playback pitch via `getPitchFactorForEvent()`. The default value is 0 for unmodified events. Works on any event type without type restriction -- only requires an active MIDI callback context.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setCoarseDetune$`
- `$API.Message.getFineDetune$`
- `$API.Message.getTransposeAmount$`

## getControllerNumber

**Signature:** `Integer getControllerNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var cc = Message.getControllerNumber();`

**Description:**
Returns the controller number of the current event. Works on Controller, PitchBend, and Aftertouch events -- all of which trigger the `onController` callback. For standard MIDI CC events, returns the actual CC number (0-127). For PitchBend events, returns the virtual CC number 128 (`Message.PITCH_BEND_CC`). For Aftertouch events, returns the virtual CC number 129 (`Message.AFTERTOUC_CC`). This virtual CC convention allows scripts in `onController` to handle all three event types uniformly with a single switch or if-chain.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns virtual CC numbers 128 and 129 for PitchBend and Aftertouch events respectively. These are not real MIDI CC numbers but HISE-internal conventions. Code that filters by CC number range (e.g., `if (cc < 128)`) implicitly excludes pitch wheel and aftertouch events.

**Cross References:**
- `$API.Message.setControllerNumber$`
- `$API.Message.getControllerValue$`

## getControllerValue

**Signature:** `Integer getControllerValue()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var val = Message.getControllerValue();`

**Description:**
Returns the value of the current controller-type event. The return value range depends on the event type: standard MIDI CC returns 0-127, Aftertouch returns 0-127, and PitchBend returns the full 14-bit range 0-16383. Internally dispatches to different HiseEvent accessors based on the event type (`getControllerValue()` for CC, `getAfterTouchValue()` for Aftertouch, `getPitchWheelValue()` for PitchBend). Only works on Controller, PitchBend, and Aftertouch events inside the `onController` callback.

**Parameters:**

(No parameters.)

**Pitfalls:**
- The return value range is not uniform: PitchBend returns 0-16383 while CC and Aftertouch return 0-127. Code that normalizes by dividing by 127.0 will produce values greater than 1.0 for pitch wheel messages.

**Cross References:**
- `$API.Message.setControllerValue$`
- `$API.Message.getControllerNumber$`

## getEventId

**Signature:** `Integer getEventId()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var id = Message.getEventId();`

**Description:**
Returns the unique event ID assigned to the current event by HISE's EventIdHandler. Event IDs are unsigned 16-bit integers (0-65535) that wrap around. Note-on and their matching note-off events share the same event ID, which enables per-voice operations like `Synth.addVolumeFade()` and `Synth.addPitchFade()` that target a specific sounding voice. Works on any event type without type restriction. In exported plugins (frontend builds), returns 0 silently instead of throwing an error when called outside a MIDI callback.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Event IDs wrap around at 65536 (uint16 overflow). Do not assume older notes have lower IDs than newer notes.

**Cross References:**
- `$API.Message.makeArtificial$`
- `$API.Message.makeArtificialOrLocal$`
- `$API.Message.isArtificial$`

## setNoteNumber

**Signature:** `undefined setNoteNumber(Number newNoteNumber)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setNoteNumber(60);`

**Description:**
Sets the MIDI note number of the current note-on or note-off event. The value is clamped to 0-127 at the HiseEvent level via `jmin<uint8>`. Only valid when the current event is a note-on or note-off -- reports an error for other event types. Requires a mutable callback context (onNoteOn or onNoteOff on a JavascriptMidiProcessor). The event type check runs unconditionally (outside the `ENABLE_SCRIPTING_SAFE_CHECKS` guard), so even in builds with safe checks disabled, calling this on a non-note event triggers an error. The note number change is applied in-place to the event in the audio buffer, affecting all downstream processors.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newNoteNumber | Number | yes | New MIDI note number | 0-127 (clamped) |

**Pitfalls:**
- Values above 127 are silently clamped to 127 rather than producing an error. Negative values are cast to `uint8`, which wraps (e.g., -1 becomes 255, then clamped to 127). Always pass values in the 0-127 range.

**Cross References:**
- `$API.Message.getNoteNumber$`
- `$API.Message.setTransposeAmount$`

## setPolyAfterTouchNoteNumberAndPressureValue

**Signature:** `undefined setPolyAfterTouchNoteNumberAndPressureValue(Number noteNumber, Number aftertouchAmount)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setPolyAfterTouchNoteNumberAndPressureValue(60, 100);`

**Description:**
Sets both the note number and pressure value of a polyphonic aftertouch event in a single call. Only valid when the current event is an aftertouch event inside a mutable callback context. Reports an error if the event is not an aftertouch event or if called outside a mutable callback. Both values are cast to `uint8` at the HiseEvent level, so the effective range for each is 0-127 (higher values wrap). The check runs unconditionally (not guarded by `ENABLE_SCRIPTING_SAFE_CHECKS`). This method uses `ADD_API_METHOD_2` (not typed), so the parameter types are not enforced at registration -- any value types are accepted and converted to int.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Number | no | Target MIDI note number for the aftertouch | 0-127 |
| aftertouchAmount | Number | no | Pressure value | 0-127 |

**Pitfalls:**
- Both monophonic (channel pressure) and polyphonic aftertouch use the same `Aftertouch` event type in HISE. The type check uses `isAftertouch()`, which returns true for both subtypes. Calling this on a channel pressure event will silently convert it to a polyphonic aftertouch format by writing the note number field.

**Cross References:**
- `$API.Message.getPolyAfterTouchNoteNumber$`
- `$API.Message.getPolyAfterTouchPressureValue$`
- `$API.Message.isPolyAftertouch$`
- `$API.Message.setMonophonicAfterTouchPressure$`

## setStartOffset

**Signature:** `undefined setStartOffset(Number newStartOffset)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setStartOffset(4410);`

**Description:**
Sets the sample start offset on the current event. The start offset tells the sound generator to skip ahead by the specified number of samples when the voice starts -- it does NOT delay event processing (use `delayEvent()` for that). The value is stored as a `uint16` in the HiseEvent, giving a maximum of 65535 samples (approximately 1.36 seconds at 48kHz). Values exceeding 65535 produce a script error. Requires a mutable callback context. The start offset is independent of the event's timestamp: timestamp controls when the event is processed within the buffer, while start offset controls where in the sample playback begins.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newStartOffset | Number | yes | Number of samples to skip at voice start | 0-65535 (uint16 max) |

**Pitfalls:**
- [BUG] The null-pointer guard checks `constMessageHolder` (const pointer) instead of `messageHolder` (mutable pointer), but the actual write operation uses `messageHolder`. In a read-only context (e.g., voice start modulators), `constMessageHolder` is set but `messageHolder` is null -- the null check passes, then the write dereferences null `messageHolder`, causing undefined behavior.
- [BUG] The error message says "Max start offset is 65536 (2^16)" but the actual maximum accepted value is 65535 (UINT16_MAX). The check `newStartOffset > UINT16_MAX` correctly accepts 65535 and rejects 65536, but the error text is off by one.

**Cross References:**
- `$API.Message.delayEvent$`

## setTransposeAmount

**Signature:** `undefined setTransposeAmount(Number transposeValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setTransposeAmount(12);`

**Description:**
Sets the transpose amount in semitones on the current event. The value is stored as an `int8` in the HiseEvent, so it is truncated to the range -128 to 127 by the cast. The transpose amount does not change the raw note number returned by `getNoteNumber()` -- the transposed pitch is `getNoteNumber() + getTransposeAmount()`. Requires a mutable callback context. Works on any event type without type restriction. When the EventIdHandler matches a note-off to its note-on, it copies the note-on's transpose amount to the note-off, ensuring the voice release uses the correct pitch. This makes transpose safe to apply in `onNoteOn` without needing to repeat it in `onNoteOff`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| transposeValue | Number | yes | Transpose amount in semitones | Stored as int8 (-128 to 127) |

**Pitfalls:**
- In exported plugins (frontend builds), this method silently returns instead of throwing an error when called outside a mutable callback context. Backend builds report an error via `reportIllegalCall()`. Code that accidentally calls `setTransposeAmount()` outside a callback may appear to work in the exported plugin but fail during development.

**Cross References:**
- `$API.Message.getTransposeAmount$`
- `$API.Message.getNoteNumber$`
- `$API.Message.setCoarseDetune$`
- `$API.Message.setNoteNumber$`

## setVelocity

**Signature:** `undefined setVelocity(Number newVelocity)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setVelocity(100);`

**Description:**
Sets the velocity of the current note-on event. Only valid when the current event is a note-on -- reports an error for note-off and all other event types. This is stricter than `getVelocity()` which works on both note-on and note-off events. Requires a mutable callback context (onNoteOn on a JavascriptMidiProcessor). The value is cast to `uint8` at the HiseEvent level with no clamping, so the effective range is 0-255 though only 1-127 is meaningful for MIDI note-on velocity. Setting velocity to 0 does not convert the event to a note-off (HiseEvent uses explicit type fields, not velocity-based note-off detection), but downstream MIDI output or DAW processing may interpret velocity 0 as note-off per MIDI convention. The velocity change is applied in-place to the event in the audio buffer, affecting all downstream processors.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newVelocity | Number | yes | New velocity value | 1-127 (standard MIDI range for note-on) |

**Pitfalls:**
- Only works on note-on events, not note-off. The error message says `"onNoteOn"` explicitly. To modify velocity on both note types, use `setGain()` instead, which works on any event type.
- No range validation is performed. Values outside 0-127 are truncated by the `uint8` cast (e.g., 200 becomes 200, 256 wraps to 0). Always pass values in the 1-127 range.

**Cross References:**
- `$API.Message.getVelocity$`
- `$API.Message.setGain$`
- `$API.Message.getNoteNumber$`

## store

**Signature:** `undefined store(ScriptObject messageEventHolder)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.store(holder);`

**Description:**
Copies the current MIDI event into a `MessageHolder` object, allowing event data to persist beyond the callback scope. The Message object's internal pointer is only valid during callback execution -- accessing it outside a callback produces errors. By storing the event into a MessageHolder (created via `Engine.createMessageHolder()`), the event data becomes an independent copy that can be read, modified, or replayed at any time. The copy is a full 16-byte HiseEvent including all fields (note number, velocity, channel, transpose, gain, detune, start offset, event ID, timestamp, and flags). Uses the const pointer internally, so it works in both mutable and read-only callback contexts. Requires an active MIDI callback (reports an error if called outside one).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| messageEventHolder | ScriptObject | yes | A MessageHolder object to receive the event copy | Must be a MessageHolder from `Engine.createMessageHolder()` |

**Pitfalls:**
- [BUG] If the argument is not a valid MessageHolder object, the method silently does nothing -- no error is reported. The `dynamic_cast` fails and execution falls through. Pass only objects created by `Engine.createMessageHolder()`.

**Cross References:**
- `$API.Engine.createMessageHolder$`
- `$API.Message.getEventId$`
- `$API.Message.getNoteNumber$`

**Example:**
```javascript:store-event-for-later
// Title: Storing a MIDI event for deferred processing
const var holder = Engine.createMessageHolder();

function onNoteOn()
{
    Message.store(holder);
    Console.print("Stored note: " + holder.getNoteNumber());
}
```
```json:testMetadata:store-event-for-later
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context; Message object has no event outside callbacks"
}
```

## isArtificial

**Signature:** `Integer isArtificial()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var artificial = Message.isArtificial();`

**Description:**
Returns whether the current event was created by a script or internal HISE mechanism rather than from external MIDI input. The artificial flag is stored in bit 31 of the HiseEvent's timestamp field. Events become artificial through `makeArtificial()`, `makeArtificialOrLocal()`, `Synth.addNoteOn()`, `Synth.addNoteOff()`, `Synth.addController()`, or internal engine mechanisms (VolumeFade, PitchFade, TimerEvent factories). Returns `false` silently if called outside a MIDI callback (when `constMessageHolder` is null) -- no error is reported.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.makeArtificial$`
- `$API.Message.makeArtificialOrLocal$`
- `$API.Message.ignoreEvent$`

## isMonophonicAfterTouch

**Signature:** `Integer isMonophonicAfterTouch()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isMono = Message.isMonophonicAfterTouch();`

**Description:**
Returns whether the current event is a monophonic (channel pressure) aftertouch message. Internally calls `HiseEvent::isChannelPressure()`, which checks whether the event type is `Type::Aftertouch`. Reports an error if called outside a MIDI callback (when `constMessageHolder` is null). This method is typically used inside the `onController` callback to distinguish channel pressure from polyphonic aftertouch before calling `getMonophonicAftertouchPressure()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Both `isMonophonicAfterTouch()` and `isPolyAftertouch()` check the same underlying event type (`Type::Aftertouch`). They both return `true` for ANY aftertouch event regardless of whether it originated as channel pressure or polyphonic aftertouch. The HISE event system does not distinguish between the two at the type level -- both MIDI message types are converted to `Type::Aftertouch` during ingestion. In practice, channel pressure events typically have note number 0 while poly aftertouch events carry the target note number, but neither method checks the note number field.

**Cross References:**
- `$API.Message.getMonophonicAftertouchPressure$`
- `$API.Message.setMonophonicAfterTouchPressure$`
- `$API.Message.isPolyAftertouch$`

## isPolyAftertouch

**Signature:** `Integer isPolyAftertouch()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isPoly = Message.isPolyAftertouch();`

**Description:**
Returns whether the current event is a polyphonic aftertouch message. Internally calls `HiseEvent::isAftertouch()`, which checks whether the event type is `Type::Aftertouch`. Reports an error if called outside a MIDI callback (when `constMessageHolder` is null). This method is typically used inside the `onController` callback before calling `getPolyAfterTouchNoteNumber()` and `getPolyAfterTouchPressureValue()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Returns `true` for both polyphonic aftertouch AND monophonic channel pressure events because both use the same `Type::Aftertouch` enum value internally. `isMonophonicAfterTouch()` has the exact same behavior -- neither method can reliably distinguish the two aftertouch types. See `isMonophonicAfterTouch` pitfall for details.

**Cross References:**
- `$API.Message.getPolyAfterTouchNoteNumber$`
- `$API.Message.getPolyAfterTouchPressureValue$`
- `$API.Message.setPolyAfterTouchNoteNumberAndPressureValue$`
- `$API.Message.isMonophonicAfterTouch$`

## isProgramChange

**Signature:** `Integer isProgramChange()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isPgm = Message.isProgramChange();`

**Description:**
Returns whether the current event is a MIDI program change message. Internally calls `HiseEvent::isProgramChange()`, which checks whether the event type is `Type::ProgramChange`. Reports an error if called outside a MIDI callback (when `constMessageHolder` is null). Program change events are routed through the `onController` callback. Use this method to detect program changes before calling `getProgramChangeNumber()` to retrieve the program number.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.getProgramChangeNumber$`

## makeArtificial

**Signature:** `Integer makeArtificial()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var id = Message.makeArtificial();`

**Description:**
Converts the current event into an artificial event, disconnecting it from the original MIDI input stream. Returns the event ID of the resulting artificial event. This method is idempotent: if the event is already artificial, it returns the existing event ID without creating a duplicate or reassigning IDs.

For note-on events, the method sets the artificial flag, registers the event with the global `EventIdHandler` (which assigns a new sequential event ID), and caches the event ID in the local `artificialNoteOnIds` array indexed by note number.

For note-off events, the method sets the artificial flag, pops the matching note-on from the `EventIdHandler` (using the cached artificial note-on ID for this note number), preserves it in `artificialNoteOnThatWasKilled` for potential `ignoreEvent` reinsert, and assigns the matching note-on's event ID to the note-off. If no matching artificial note-on is found, the note-off is automatically ignored (its ignored flag is set).

For all other event types (Controller, PitchBend, Aftertouch, etc.), the method simply sets the artificial flag without event ID management.

The modified event replaces the original in-place in the audio buffer via `swapWith`. Returns 0 if called outside a mutable callback context.

**Parameters:**

(No parameters.)

**Pitfalls:**
- On note-off events, calling `makeArtificial()` pops the matching note-on from the EventIdHandler. If no matching artificial note-on exists for this note number (e.g., because `makeArtificial()` was never called on the note-on, or a different script processor handled it), the note-off is automatically ignored and won't trigger voice release.
- The method resets `artificialNoteOnThatWasKilled` to an empty event at the start of every call. This means only the most recent `makeArtificial()` call's note-on is preserved for the `ignoreEvent` reinsert logic.

**Cross References:**
- `$API.Message.makeArtificialOrLocal$`
- `$API.Message.isArtificial$`
- `$API.Message.ignoreEvent$`
- `$API.Message.getEventId$`

## makeArtificialOrLocal

**Signature:** `Integer makeArtificialOrLocal()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var id = Message.makeArtificialOrLocal();`

**Description:**
Converts the current event into an artificial event, always creating a new event with a new ID -- even if the event is already artificial. Returns the event ID of the new artificial event. This is the key difference from `makeArtificial()`, which is idempotent and returns the existing ID when the event is already artificial.

The internal behavior is identical to `makeArtificial()` for all event types, except it skips the "already artificial" early-return check. This means calling it on an already-artificial note-on will register a second artificial event with a new ID, and the local `artificialNoteOnIds` cache will be overwritten with the new ID (losing the reference to the previous one).

Use this method when you need to create multiple independent copies/branches of an event, each with its own event ID for independent voice control. Use `makeArtificial()` when you simply want to modify an existing event without creating duplicates.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Calling on an already-artificial note-on overwrites the local `artificialNoteOnIds` cache for that note number. The previous artificial event ID is lost from the local cache, which may break subsequent note-off matching if both IDs need to be tracked.
- Unlike `makeArtificial()`, calling this on an already-artificial event always assigns a new event ID, which means subsequent `Synth.addVolumeFade()` or `Synth.addPitchFade()` calls targeting the old ID will no longer reach the voice.

**Cross References:**
- `$API.Message.makeArtificial$`
- `$API.Message.isArtificial$`
- `$API.Message.ignoreEvent$`
- `$API.Message.getEventId$`

## getFineDetune

**Signature:** `Integer getFineDetune()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var cents = Message.getFineDetune();`

**Description:**
Returns the fine detune amount in cents stored on the current event. This is a per-event property stored as an `int8` (-128 to 127), used alongside coarse detune and transpose to calculate the final playback pitch via `getPitchFactorForEvent()`. The default value is 0 for unmodified events. Works on any event type without type restriction -- only requires an active MIDI callback context. Reports an error if called outside a MIDI callback (when `constMessageHolder` is null).

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setFineDetune$`
- `$API.Message.getCoarseDetune$`
- `$API.Message.getTransposeAmount$`

## getGain

**Signature:** `Integer getGain()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var gainDb = Message.getGain();`

**Description:**
Returns the per-event gain in decibels. The gain is stored as an `int8` in the HiseEvent, with a valid range of -100 to 36 dB (clamped by `setGain()`). A value of -100 represents silence. The default value is 0 for unmodified events. This property works on any event type -- unlike `getVelocity()` which requires a note event, `getGain()` can be called on controllers, pitch bend, and other event types. Sound generators apply this gain factor to the voice amplitude via `getGainFactor()`, which converts decibels to a linear 0.0-1.0+ multiplier. Works on any event type without type restriction -- only requires an active MIDI callback context.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Message.setGain$`
- `$API.Message.getVelocity$`

## ignoreEvent

**Signature:** `undefined ignoreEvent(Integer shouldBeIgnored)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.ignoreEvent(1);`

**Description:**
Sets or clears the ignored flag on the current event. When ignored, the event remains in the buffer but is skipped by downstream processors. The ignored flag is stored in bit 30 of the HiseEvent's timestamp field. Passing a truthy value sets the flag; passing a falsy value clears it (re-enabling the event). Requires a mutable callback context -- reports an error if called outside a MIDI callback or in a read-only context. The null-pointer check on `messageHolder` runs unconditionally (not guarded by `ENABLE_SCRIPTING_SAFE_CHECKS`), ensuring the error is always reported even in builds with safe checks disabled.

When ignoring an artificial note-off whose event ID matches the `artificialNoteOnThatWasKilled` (the note-on that was popped during `makeArtificial()` on this note-off), the method automatically reinserts the matching note-on back into the EventIdHandler and the local `artificialNoteOnIds` cache. This prevents stuck notes by ensuring the note-on remains available for a future note-off to properly terminate the voice.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeIgnored | Integer | no | Truthy to ignore, falsy to re-enable | Boolean-like (0/1) |

**Pitfalls:**
- In deferred mode, ignored events are skipped entirely during processing (`if (m.isIgnored() || m.isArtificial()) continue`). Calling `ignoreEvent(false)` to re-enable an event in deferred mode has no effect because ignored events never reach the deferred callback.
- The note-on reinsert logic only triggers when all four conditions are met: `shouldBeIgnored` is truthy, the event is artificial, the event is a note-off, and its event ID matches `artificialNoteOnThatWasKilled`. If `makeArtificial()` was not called on the corresponding note-off (only on the note-on), the reinsert does not occur.

**Cross References:**
- `$API.Message.makeArtificial$`
- `$API.Message.makeArtificialOrLocal$`
- `$API.Message.isArtificial$`
- `$API.Message.getEventId$`

**Diagram:**
- **Brief:** Artificial Note-Off Ignore Reinsert
- **Type:** timing
- **Description:** Shows the sequence: (1) Note-on arrives in onNoteOn, script calls makeArtificial() which pops the original from EventIdHandler and stores it in artificialNoteOnThatWasKilled. (2) Matching note-off arrives in onNoteOff, script calls makeArtificial() on the note-off, which pops the artificial note-on from EventIdHandler and assigns its event ID to the note-off. (3) Script calls ignoreEvent(true) on the note-off. Because the event ID matches artificialNoteOnThatWasKilled, the method calls reinsertArtificialNoteOn() to restore the note-on into EventIdHandler and the local cache. A future note-off can then properly terminate the voice.

## setAllNotesOffCallback

**Signature:** `undefined setAllNotesOffCallback(Function callback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new WeakCallbackHolder (heap allocation) and increments a reference count. Not safe for the audio thread.
**Minimal Example:** `Message.setAllNotesOffCallback(onAllNotesOff);`

**Description:**
Registers a callback function to be invoked when an AllNotesOff MIDI event is received. AllNotesOff events do not trigger the standard `onNoteOff` or `onController` callbacks -- they are handled separately by this dedicated handler. The callback is invoked synchronously on the audio thread via `callSync()`, so it must be declared as an `inline function` and must not perform any allocations, I/O, or blocking operations. In backend builds, the callback is checked for realtime safety at registration time via `RealtimeSafetyInfo::check()`, and a script error is reported if the callback is not audio-thread safe. The callback takes zero parameters. A diagnostic is registered via `ADD_CALLBACK_DIAGNOSTIC` that validates the expected argument count of 0 at parse time.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callback | Function | yes | The callback to invoke on AllNotesOff events | Must be an inline function; takes 0 arguments |

**Callback Signature:** callback()

**Pitfalls:**
- The callback is invoked on the audio thread. Using a regular `function` instead of `inline function` will be flagged as unsafe in backend builds but may silently fail in frontend builds where the realtime safety check is compiled out.
- AllNotesOff events do not trigger `onNoteOff` or `onController`. If your script needs to respond to AllNotesOff (e.g., to reset internal state), this is the only way to receive it.

**Cross References:**
- `$API.Message.ignoreEvent$`
- `$API.Synth.allNotesOff$`

**Example:**
```javascript:all-notes-off-handler
// Title: Registering an AllNotesOff callback to reset script state
reg activeNotes = 0;

inline function onAllNotesOff()
{
    activeNotes = 0;
}

Message.setAllNotesOffCallback(onAllNotesOff);
```
```json:testMetadata:all-notes-off-handler
{
  "testable": false,
  "skipReason": "Requires AllNotesOff MIDI event which cannot be triggered programmatically from script"
}
```

## setChannel

**Signature:** `undefined setChannel(Number newChannel)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setChannel(2);`

**Description:**
Sets the MIDI channel of the current event. The channel uses 1-based numbering (1-16), matching the standard MIDI convention. Unlike most setter methods on Message, this method includes explicit range validation: values outside 1-16 produce a script error with the message "Channel must be between 1 and 16." Both the null-pointer check and the range validation are guarded by `ENABLE_SCRIPTING_SAFE_CHECKS`, so in builds with safe checks disabled, invalid channel values are passed through to `HiseEvent::setChannel()` unchecked (cast to `uint8`). Works on any event type without type restriction -- only requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newChannel | Number | yes | MIDI channel number | 1-16 |

**Pitfalls:**
- Uses 1-based channel numbering (1-16), not 0-based (0-15). Passing 0 triggers a script error in debug builds and wraps to 0 at the `uint8` level in release builds.

**Cross References:**
- `$API.Message.getChannel$`

## setCoarseDetune

**Signature:** `undefined setCoarseDetune(Number semiToneDetune)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setCoarseDetune(7);`

**Description:**
Sets the coarse detune amount in semitones on the current event. The value is stored as an `int8` in the HiseEvent, so it is truncated to the range -128 to 127 by the cast. Sound generators use this value alongside fine detune and transpose amount to calculate the final playback pitch via `getPitchFactorForEvent()`. Unlike `setTransposeAmount()`, the coarse detune is NOT automatically copied from note-on to note-off by the EventIdHandler. Works on any event type without type restriction -- only requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| semiToneDetune | Number | yes | Coarse detune in semitones | Stored as int8 (-128 to 127) |

**Cross References:**
- `$API.Message.getCoarseDetune$`
- `$API.Message.setFineDetune$`
- `$API.Message.setTransposeAmount$`

## setControllerNumber

**Signature:** `undefined setControllerNumber(Number newControllerNumber)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setControllerNumber(1);`

**Description:**
Sets the MIDI controller number on the current event. Only valid when the current event is a Controller type -- reports an error for PitchBend, Aftertouch, and all other event types (unlike `getControllerNumber()` which works on all three). This restriction exists because PitchBend and Aftertouch events have distinct internal encodings that cannot meaningfully receive a CC number assignment. The value is cast to `uint8` at the HiseEvent level, so the effective range is 0-127. Requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newControllerNumber | Number | yes | New MIDI CC number | 0-127 |

**Pitfalls:**
- Unlike `getControllerNumber()` which works on Controller, PitchBend, and Aftertouch events, `setControllerNumber()` only works on Controller events. Calling it on a PitchBend or Aftertouch event produces a script error. This asymmetry means you cannot remap a pitch wheel event to a CC by setting a controller number on it.

**Cross References:**
- `$API.Message.getControllerNumber$`
- `$API.Message.setControllerValue$`

## setControllerValue

**Signature:** `undefined setControllerValue(Number newControllerValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setControllerValue(64);`

**Description:**
Sets the MIDI controller value on the current event. Only valid when the current event is a Controller type -- reports an error for PitchBend, Aftertouch, and all other event types (unlike `getControllerValue()` which works on all three). This restriction exists because PitchBend uses 14-bit encoding across two bytes and Aftertouch has a separate value accessor. The value is cast to `uint8` at the HiseEvent level via `setControllerValue()`, giving an effective range of 0-127 (higher values wrap). Requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newControllerValue | Number | yes | New CC value | 0-127 |

**Pitfalls:**
- Unlike `getControllerValue()` which works on Controller, PitchBend, and Aftertouch events, `setControllerValue()` only works on Controller events. To modify the value of a PitchBend event, there is no direct setter in the Message API. To modify Aftertouch pressure, use `setMonophonicAfterTouchPressure()` or `setPolyAfterTouchNoteNumberAndPressureValue()`.

**Cross References:**
- `$API.Message.getControllerValue$`
- `$API.Message.setControllerNumber$`
- `$API.Message.setMonophonicAfterTouchPressure$`

## setFineDetune

**Signature:** `undefined setFineDetune(Number cents)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setFineDetune(50);`

**Description:**
Sets the fine detune amount in cents on the current event. The value is stored as an `int8` in the HiseEvent, so it is truncated to the range -128 to 127 by the cast. Sound generators use this value alongside coarse detune and transpose amount to calculate the final playback pitch via `getPitchFactorForEvent()`. Unlike `setTransposeAmount()`, the fine detune is NOT automatically copied from note-on to note-off by the EventIdHandler. Works on any event type without type restriction -- only requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cents | Number | yes | Fine detune in cents | Stored as int8 (-128 to 127) |

**Cross References:**
- `$API.Message.getFineDetune$`
- `$API.Message.setCoarseDetune$`
- `$API.Message.setTransposeAmount$`

## setGain

**Signature:** `undefined setGain(Number gainInDecibels)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setGain(-6);`

**Description:**
Sets the per-event gain in decibels. The value is clamped to the range -100 to 36 dB at the HiseEvent level via `jlimit`. A value of -100 represents silence. Sound generators apply this gain factor to the voice amplitude via `getGainFactor()`, which converts decibels to a linear multiplier. Unlike `setVelocity()` which only works on note-on events, `setGain()` works on any event type (NoteOn, NoteOff, Controller, etc.), making it the preferred way to apply per-event volume adjustments when velocity modification is too restrictive. Works on any event type without type restriction -- only requires a mutable callback context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gainInDecibels | Number | yes | Gain amount in decibels | -100 to 36 (clamped) |

**Pitfalls:**
- The value is clamped silently to -100..36 dB. Values outside this range do not produce an error; they are clamped to the nearest boundary. -100 dB produces effective silence (gain factor of 0.00001), not true zero.

**Cross References:**
- `$API.Message.getGain$`
- `$API.Message.setVelocity$`

## setMonophonicAfterTouchPressure

**Signature:** `undefined setMonophonicAfterTouchPressure(Integer pressure)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `Message.setMonophonicAfterTouchPressure(100);`

**Description:**
Sets the pressure value of a monophonic (channel pressure) aftertouch event. Only valid when the current event is a channel pressure event (checked via `isChannelPressure()` on the mutable `messageHolder`). Reports an error if the event is not a channel pressure event or if called outside a mutable callback context. The value is cast to `uint8` at the HiseEvent level, so the effective range is 0-127. Both the null-pointer check and the event type check are guarded by `ENABLE_SCRIPTING_SAFE_CHECKS` -- in builds with safe checks disabled, calling this on a non-aftertouch event writes to the value byte unconditionally. This method uses `ADD_API_METHOD_1` (not typed), so the parameter type is not enforced at registration.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pressure | Integer | no | New pressure value | 0-127 |

**Pitfalls:**
- Both monophonic (channel pressure) and polyphonic aftertouch use the same underlying `Aftertouch` event type in HISE. The type check uses `isChannelPressure()`, which returns true for ANY aftertouch event. Calling this on a polyphonic aftertouch event will succeed and overwrite the pressure value without any distinction.

**Cross References:**
- `$API.Message.getMonophonicAftertouchPressure$`
- `$API.Message.isMonophonicAfterTouch$`
- `$API.Message.setPolyAfterTouchNoteNumberAndPressureValue$`

## sendToMidiOut

**Signature:** `undefined sendToMidiOut()`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Acquires a SimpleReadWriteLock (ScopedWriteLock) in MainController::sendToMidiOut(). Lock-free in HISE's audio architecture but uses a read-write lock for the output buffer.
**Minimal Example:** `Message.sendToMidiOut();`

**Description:**
Forwards the current event to the plugin's MIDI output. The method first calls `makeArtificial()` internally (MIDI output requires artificial events), then adds the event to the MainController's output MIDI buffer under a `SimpleReadWriteLock`. In backend builds, the method checks that the `EnableMidiOut` project setting is enabled and reports a script error if it is not. This check is compiled out in frontend builds -- the method will attempt to send regardless of the setting (but the output buffer may not be connected to anything if the setting was not enabled at export time).

**Parameters:**

(No parameters.)

**Pitfalls:**
- The `EnableMidiOut` project setting must be enabled for MIDI output to function. In backend builds, a missing setting produces a descriptive error. In frontend (exported) builds, no check occurs -- the method silently does nothing useful if MIDI output was not enabled when the plugin was compiled.
- The method calls `makeArtificial()` internally, which modifies the current event in-place. After calling `sendToMidiOut()`, the event is artificial even if it was not before. This has implications for downstream event matching and the `ignoreEvent` reinsert logic.
- [BUG] The method accesses `messageHolder` directly without a null check (no `ENABLE_SCRIPTING_SAFE_CHECKS` guard). Calling it outside a mutable callback context will dereference a null pointer.

**Cross References:**
- `$API.Message.makeArtificial$`
- `$API.Message.isArtificial$`
- `$API.Message.ignoreEvent$`
