# MessageHolder -- Method Analysis

## getFineDetune

**Signature:** `int getFineDetune()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var cents = {obj}.getFineDetune();`

**Description:**
Returns the fine detune amount in cents stored on this event. The value is an `int8` internally, so the range is -128 to 127 cents. Fine detune is a per-event property set by MIDI processors or scripts via `setFineDetune()` and is used together with coarse detune to compute the final pitch factor for voice playback.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setFineDetune$`
- `$API.MessageHolder.getCoarseDetune$`

---

## getGain

**Signature:** `int getGain()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var gainDb = {obj}.getGain();`

**Description:**
Returns the per-event gain in decibels. The value is stored as an `int8`, giving a range of -128 to 127 dB. This gain value is applied as a per-voice gain factor during voice rendering. A value of 0 means unity gain.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setGain$`

---

## getMonophonicAftertouchPressure

**Signature:** `int getMonophonicAftertouchPressure()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pressure = {obj}.getMonophonicAftertouchPressure();`

**Description:**
Returns the channel pressure (monophonic aftertouch) value from this event. Delegates to `HiseEvent::getChannelPressureValue()`, which reads the `value` byte field. This is meaningful when the event type is Aftertouch and `isMonophonicAfterTouch()` returns true (monophonic channel pressure, where the `number` byte is not used for note discrimination). The return range is 0-255 (uint8).

Note: MessageHolder performs no event-type guard. If called on a non-aftertouch event, this returns whatever is in the `value` byte (e.g., velocity for NoteOn, controller value for CC).

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setMonophonicAfterTouchPressure$`
- `$API.MessageHolder.isMonophonicAfterTouch$`

---

## getNoteNumber

**Signature:** `int getNoteNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var note = {obj}.getNoteNumber();`

**Description:**
Returns the MIDI note number stored in this event. Reads the `number` byte field of the underlying HiseEvent. For NoteOn and NoteOff events, this is the standard MIDI note number (0-127). MessageHolder performs no event-type guard, so calling this on a non-note event returns whatever value is in the `number` byte (e.g., the controller number for CC events).

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setNoteNumber$`
- `$API.MessageHolder.getTransposeAmount$`

---

## getPolyAfterTouchNoteNumber

**Signature:** `int getPolyAfterTouchNoteNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var noteNum = {obj}.getPolyAfterTouchNoteNumber();`

**Description:**
Returns the note number associated with a polyphonic aftertouch event. Delegates to `HiseEvent::getAfterTouchNumber()`, which reads the `number` byte cast to `uint8`. This is meaningful when the event type is Aftertouch and `isPolyAftertouch()` returns true, indicating per-key pressure data. MessageHolder performs no type guard -- calling this on a non-aftertouch event returns whatever is in the `number` byte.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getPolyAfterTouchPressureValue$`
- `$API.MessageHolder.setPolyAfterTouchNoteNumberAndPressureValue$`
- `$API.MessageHolder.isPolyAftertouch$`

---

## getPolyAfterTouchPressureValue

**Signature:** `int getPolyAfterTouchPressureValue()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var pressure = {obj}.getPolyAfterTouchPressureValue();`

**Description:**
Returns the pressure value from a polyphonic aftertouch event. Delegates to `HiseEvent::getAfterTouchValue()`, which reads the `value` byte cast to `uint8`, giving a range of 0-255. This is meaningful when the event type is Aftertouch and `isPolyAftertouch()` returns true. MessageHolder performs no type guard -- calling this on a non-aftertouch event returns whatever is in the `value` byte.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getPolyAfterTouchNoteNumber$`
- `$API.MessageHolder.setPolyAfterTouchNoteNumberAndPressureValue$`
- `$API.MessageHolder.isPolyAftertouch$`

---

## getTimestamp

**Signature:** `int getTimestamp()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ts = {obj}.getTimestamp();`

**Description:**
Returns the sample-accurate timestamp of the event. The timestamp is stored in the lower 28 bits of a `uint32` field (the upper 4 bits are reserved for flags including the artificial and ignored markers). The value represents a sample offset from the start of the current audio buffer, or an absolute sample position when used with MidiPlayer event lists (depending on the timestamp edit format).

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setTimestamp$`
- `$API.MessageHolder.addToTimestamp$`

---

## getTransposeAmount

**Signature:** `int getTransposeAmount()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var transpose = {obj}.getTransposeAmount();`

**Description:**
Returns the transpose amount in semitones stored on this event. The value is an `int8` internally, giving a range of -128 to 127 semitones. Transpose is a per-event property that shifts the effective pitch without changing the original note number. This means NoteOn and NoteOff events automatically match by their original note number even when transposed. The actual sounding note number is `getNoteNumber() + getTransposeAmount()`.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setTransposeAmount$`
- `$API.MessageHolder.getNoteNumber$`

---

## addToTimestamp

**Signature:** `void addToTimestamp(int deltaSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addToTimestamp(256);`

**Description:**
Adds a delta value in samples to the current timestamp of the event. The delta is cast to `int16` before being passed to `HiseEvent::addToTimeStamp()`, which adds the delta to the current timestamp and clamps the result to a minimum of 0. This method is useful for offsetting events relative to their current position, for example when adjusting timing in a MIDI event list from MidiPlayer.

Note: The `int16` cast in the MessageHolder wrapper limits the effective delta range to -32768..32767 samples. Values outside this range will be silently truncated via integer overflow before being applied to the timestamp. The underlying HiseEvent timestamp field uses 28 bits (up to ~268 million), so the delta range is more restrictive than the absolute timestamp range.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| deltaSamples | Number | no | Sample offset to add (positive shifts later, negative shifts earlier) | Effective range -32768..32767 due to int16 cast |

**Pitfalls:**
- [BUG] This method is not registered in the scripting API constructor (missing `ADD_API_METHOD_1` / Wrapper entry). It appears in documentation but is inaccessible from HISEScript. Use `setTimestamp(getTimestamp() + delta)` as a workaround.
- [BUG] The `int16` cast silently truncates delta values outside -32768..32767. Passing a value like 50000 will overflow to a negative number, shifting the timestamp backward instead of forward. (Moot until the registration issue above is fixed.)

**Cross References:**
- `$API.MessageHolder.setTimestamp$`
- `$API.MessageHolder.getTimestamp$`

---

## clone

**Signature:** `var clone()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new MessageHolder object on the heap.
**Minimal Example:** `var copy = {obj}.clone();`

**Description:**
Creates and returns a new MessageHolder that is an independent copy of this one. The internal HiseEvent data is fully copied, so modifying the clone does not affect the original and vice versa. The returned object is a new MessageHolder instance with its own reference-counted lifetime.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.dump$`
- `$API.Engine.createMessageHolder$`
- `$API.Message.store$`

**Example:**
```javascript:clone-independent-copy
// Title: Cloning a MessageHolder for independent modification
const var mh = Engine.createMessageHolder();
mh.setType(mh.NoteOn);
mh.setNoteNumber(60);
mh.setVelocity(100);
mh.setChannel(1);

var copy = mh.clone();
copy.setNoteNumber(72);

Console.print(mh.getNoteNumber());
Console.print(copy.getNoteNumber());
```
```json:testMetadata:clone-independent-copy
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["60", "72"]}
  ]
}
```

---

## dump

**Signature:** `String dump()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Constructs a new String via heap-allocating concatenation.
**Minimal Example:** `var info = {obj}.dump();`

**Description:**
Returns a human-readable string describing the event contents. The format is: `"Type: <type>, Channel: <ch>, Number: <num>, Value: <val>, EventId: <id>, Timestamp: <ts>, "`. For pitch wheel events, the output differs: Number, Value, and EventId fields are replaced with a single `"Value: <pitchWheelValue>, "` showing the 14-bit pitch wheel value. This method is also used by the HISE debugger to display MessageHolder objects (via `getDebugValue()`).

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.clone$`
- `$API.Console.print$`

**Example:**
```javascript:dump-debug-output
// Title: Inspecting a MessageHolder with dump()
const var mh = Engine.createMessageHolder();
mh.setType(mh.NoteOn);
mh.setNoteNumber(64);
mh.setVelocity(80);
mh.setChannel(1);

Console.print(mh.dump());
```
```json:testMetadata:dump-debug-output
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Type: NoteOn, Channel: 1, Number: 64, Value: 80, EventId: 0, Timestamp: 0, "]}
  ]
}
```

---

## getChannel

**Signature:** `int getChannel()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ch = {obj}.getChannel();`

**Description:**
Returns the MIDI channel of the event. HISE supports up to 256 channels internally (stored as `uint8`), extending beyond the standard MIDI 1-16 range. A newly created MessageHolder has channel 0, which is outside the standard MIDI channel range. MessageHolder performs no event-type guard -- the channel field exists on all event types.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setChannel$`

---

## getCoarseDetune

**Signature:** `int getCoarseDetune()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var semitones = {obj}.getCoarseDetune();`

**Description:**
Returns the coarse detune amount in semitones stored on this event. The value is an `int8` internally, giving a range of -128 to 127 semitones. Coarse detune is a per-event property set by MIDI processors or scripts via `setCoarseDetune()` and is used together with fine detune (in cents) to compute the final pitch factor for voice playback.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setCoarseDetune$`
- `$API.MessageHolder.getFineDetune$`

---

## getControllerNumber

**Signature:** `int getControllerNumber()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ccNum = {obj}.getControllerNumber();`

**Description:**
Returns the controller number for this event. Delegates to `HiseEvent::getControllerNumber()`, which returns virtual CC numbers for non-CC controller-like types: 128 for PitchBend events and 129 for Aftertouch events. For standard CC events, it returns the actual MIDI controller number (0-127). For non-controller event types (NoteOn, NoteOff, etc.), it returns whatever value is in the `number` byte field since MessageHolder performs no event-type guard.

The virtual CC number convention (128 = PitchBend, 129 = Aftertouch) is consistent with `setControllerNumber()`, which accepts these values to change the event type.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setControllerNumber$`
- `$API.MessageHolder.getControllerValue$`
- `$API.MessageHolder.isController$`

---

## getControllerValue

**Signature:** `int getControllerValue()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ccVal = {obj}.getControllerValue();`

**Description:**
Returns the controller value for this event. For pitch wheel events, returns the full 14-bit pitch wheel value (0-16383) via `HiseEvent::getPitchWheelValue()`, which reconstructs the value from the `number` and `value` bytes (`number | (value << 7)`). For all other event types, returns the `value` byte as an integer (0-255 range for the raw byte, though standard MIDI CC values are 0-127).

MessageHolder performs no event-type guard beyond the pitch wheel check. Calling this on a NoteOn event returns the velocity, on a NoteOff returns 0, etc.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setControllerValue$`
- `$API.MessageHolder.getControllerNumber$`
- `$API.MessageHolder.isController$`

---

## getEventId

**Signature:** `int getEventId()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var id = {obj}.getEventId();`

**Description:**
Returns the event ID stored on this event. The event ID is a `uint16` internally (0-65535) and wraps around at 65536. HISE automatically assigns sequential event IDs to NoteOn/NoteOff pairs during MIDI processing. For MessageHolder objects created via `Engine.createMessageHolder()`, the event ID is 0 by default. When a MessageHolder is passed to `Synth.addMessageFromHolder()`, the system assigns a new event ID for NoteOn events (the returned value) and looks up the matching ID for NoteOff events.

The event ID on a MessageHolder reflects whatever was stored when the event was captured (via `Message.store()`) or created. It does not update automatically when the holder is re-injected.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Synth.addMessageFromHolder$`
- `$API.Message.store$`

---

## setCoarseDetune

**Signature:** `void setCoarseDetune(Number semiToneDetune)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setCoarseDetune(12);`

**Description:**
Sets the coarse detune amount in semitones for this event. The value is stored as an `int8` internally, so the effective range is -128 to 127 semitones. Values outside this range will be silently truncated by integer cast to `int8`. Coarse detune is a per-event property used together with fine detune (cents) to compute the final pitch factor for voice playback via `HiseEvent::getPitchFactorForEvent()`.

This property is independent of the transpose amount. Transpose preserves note-on/note-off matching by keeping the original note number intact, while coarse detune is purely a pitch modifier applied during voice rendering.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| semiToneDetune | Number | yes | Coarse detune in semitones | Stored as int8 (-128..127) |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getCoarseDetune$`
- `$API.MessageHolder.setFineDetune$`
- `$API.MessageHolder.setTransposeAmount$`

---

## setControllerNumber

**Signature:** `void setControllerNumber(Number newControllerNumber)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setControllerNumber(1);`

**Description:**
Sets the controller number for this event. Has special type coercion for virtual CC numbers: setting the controller number to 128 changes the event type to PitchBend, and setting it to 129 changes the event type to Aftertouch. For all other values (0-127), the value is written directly to the `number` byte via `HiseEvent::setControllerNumber()`, which casts to `uint8`.

This type coercion enables constructing pitch wheel and aftertouch events through the controller API without needing a separate call to `setType()`. The reverse mapping is consistent: `getControllerNumber()` returns 128 for PitchBend events and 129 for Aftertouch events.

MessageHolder performs no event-type guard. Calling this on a NoteOn event will overwrite the note number byte (or change the type if 128/129 is passed).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newControllerNumber | Number | yes | MIDI CC number, or 128 for PitchBend, 129 for Aftertouch | 0-127 for standard CC; 128/129 for type coercion |

**Pitfalls:**
- Setting controller number to 128 or 129 changes the event type (to PitchBend or Aftertouch respectively) rather than storing a controller number. This is intentional but may be surprising if the caller expects only the number field to change.

**Cross References:**
- `$API.MessageHolder.getControllerNumber$`
- `$API.MessageHolder.setControllerValue$`
- `$API.MessageHolder.setType$`

**Example:**
```javascript:set-controller-number-type-coercion
// Title: Constructing a pitch bend event via virtual CC number
const var mh = Engine.createMessageHolder();
mh.setType(mh.Controller);
mh.setChannel(1);

// Setting CC number 128 converts the event type to PitchBend
mh.setControllerNumber(128);
mh.setControllerValue(8192);

Console.print(mh.dump());
```
```json:testMetadata:set-controller-number-type-coercion
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Type: PitchBend, Channel: 1, Value: 8192, Timestamp: 0, "]}
  ]
}
```

---

## setControllerValue

**Signature:** `void setControllerValue(Number newControllerValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setControllerValue(64);`

**Description:**
Sets the controller value for this event. If the event is currently a PitchBend type, the value is stored as a 14-bit pitch wheel value (0-16383) via `HiseEvent::setPitchWheelValue()`, which splits the value across the `number` and `value` bytes (`number = value & 127`, `value = (value >> 7) & 127`). For all other event types, the value is written directly to the `value` byte as `uint8` via `HiseEvent::setControllerValue()`.

MessageHolder performs no event-type guard beyond the pitch wheel check. Calling this on a NoteOn event overwrites the velocity byte.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newControllerValue | Number | yes | Controller value. 0-127 for standard CC, 0-16383 for PitchBend | Range depends on event type |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getControllerValue$`
- `$API.MessageHolder.setControllerNumber$`

---

## setFineDetune

**Signature:** `void setFineDetune(Number cents)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setFineDetune(50);`

**Description:**
Sets the fine detune amount in cents for this event. The value is stored as an `int8` internally, so the effective range is -128 to 127 cents. Values outside this range will be silently truncated by integer cast to `int8`. Fine detune is a per-event property used together with coarse detune (semitones) to compute the final pitch factor for voice playback via `HiseEvent::getPitchFactorForEvent()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cents | Number | yes | Fine detune in cents | Stored as int8 (-128..127) |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getFineDetune$`
- `$API.MessageHolder.setCoarseDetune$`

---

## setGain

**Signature:** `void setGain(Number gainInDecibels)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setGain(-6);`

**Description:**
Sets the per-event gain in decibels. The value is clamped to the range -100..36 dB by `jlimit` in `HiseEvent::setGain()` and then stored as `int8`. A value of 0 means unity gain. This gain is applied as a per-voice gain factor during voice rendering (converted via `Decibels::decibelsToGain()`). Values outside the clamped range are silently constrained without error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| gainInDecibels | Number | yes | Gain in decibels | Clamped to -100..36 |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getGain$`
- `$API.MessageHolder.setVelocity$`

---

## setMonophonicAfterTouchPressure

**Signature:** `void setMonophonicAfterTouchPressure(int pressure)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setMonophonicAfterTouchPressure(100);`

**Description:**
Sets the channel pressure (monophonic aftertouch) value on this event. Delegates to `HiseEvent::setChannelPressureValue()`, which writes the value to the `value` byte as `uint8`. This is meaningful when the event type is Aftertouch and represents monophonic (channel-wide) aftertouch pressure.

MessageHolder performs no event-type guard. Calling this on a non-aftertouch event will overwrite whatever the `value` byte represents for that event type (e.g., velocity for NoteOn, controller value for CC). To construct a proper monophonic aftertouch event, set the type to `Aftertouch` first.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| pressure | Number | no | Channel pressure value | Stored as uint8 (0-255) |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getMonophonicAftertouchPressure$`
- `$API.MessageHolder.isMonophonicAfterTouch$`
- `$API.MessageHolder.setPolyAfterTouchNoteNumberAndPressureValue$`

---

## setNoteNumber

**Signature:** `void setNoteNumber(Number newNoteNumber)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setNoteNumber(60);`

**Description:**
Sets the MIDI note number for this event. The value is clamped to the range 0-127 by `jmin<uint8>()` in `HiseEvent::setNoteNumber()` and stored in the `number` byte. The underlying `HiseEvent::setNoteNumber()` has a debug-only assertion (`jassert(isNoteOnOrOff())`) that fires in HISE IDE debug builds if called on non-note events, but this is not enforced at runtime -- the assignment still proceeds.

MessageHolder's wrapper performs no additional validation. Calling this on a non-note event (e.g., CC, PitchBend) will overwrite the `number` byte, which has different semantics for those event types (controller number for CC, low byte of pitch wheel value for PitchBend).

Note: Changing the note number directly (rather than using `setTransposeAmount()`) means that NoteOff matching must be handled manually -- the original note number is overwritten, so a matching NoteOff must use the same modified number.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newNoteNumber | Number | yes | MIDI note number | Clamped to 0-127 |

**Pitfalls:**
- Changing the note number directly breaks automatic NoteOn/NoteOff matching. Use `setTransposeAmount()` instead for pitch shifting that preserves note pairing.

**Cross References:**
- `$API.MessageHolder.getNoteNumber$`
- `$API.MessageHolder.setTransposeAmount$`

---

## setPolyAfterTouchNoteNumberAndPressureValue

**Signature:** `void setPolyAfterTouchNoteNumberAndPressureValue(int noteNumber, int aftertouchAmount)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setPolyAfterTouchNoteNumberAndPressureValue(60, 100);`

**Description:**
Sets both the note number and pressure value for a polyphonic aftertouch event in a single call. Delegates to `HiseEvent::setAfterTouchValue()`, which writes `number = (uint8)noteNumber` and `value = (uint8)aftertouchAmount`. Both values are cast to `uint8`, giving a range of 0-255.

This is the only way to set polyphonic aftertouch data on a MessageHolder, since `setNoteNumber()` clamps to 0-127 and has different semantics. To construct a complete polyphonic aftertouch event, call `setType(mh.Aftertouch)` first, then this method.

MessageHolder performs no event-type guard. Calling this on a non-aftertouch event will overwrite the `number` and `value` bytes with their aftertouch-specific interpretation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Number | no | Note number for polyphonic aftertouch | Stored as uint8 (0-255) |
| aftertouchAmount | Number | no | Pressure value | Stored as uint8 (0-255) |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getPolyAfterTouchNoteNumber$`
- `$API.MessageHolder.getPolyAfterTouchPressureValue$`
- `$API.MessageHolder.isPolyAftertouch$`
- `$API.MessageHolder.setMonophonicAfterTouchPressure$`

---

## getVelocity

**Signature:** `int getVelocity()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var vel = {obj}.getVelocity();`

**Description:**
Returns the velocity value stored on this event. Reads the `value` byte field of the underlying HiseEvent as a `uint8`, giving a range of 0-255 (though standard MIDI velocity is 0-127). For NoteOn events, this is the note-on velocity. For NoteOff events, this returns the release velocity (typically 0 in HISE's event system). MessageHolder performs no event-type guard -- calling this on a non-note event returns whatever is in the `value` byte (e.g., the controller value for CC events, the pressure value for aftertouch).

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.setVelocity$`
- `$API.MessageHolder.getNoteNumber$`

---

## ignoreEvent

**Signature:** `void ignoreEvent(bool shouldBeIgnored)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.ignoreEvent(true);`

**Description:**
Sets or clears the ignored flag on the stored event. The ignored flag is stored as bit 30 of the HiseEvent's 32-bit timestamp field. When an event is marked as ignored, it remains in the MIDI buffer but is skipped during processing. Passing `true` sets the flag (event will be ignored), passing `false` clears it (event will be processed normally).

This is useful when storing events from callbacks via `Message.store()` and later re-injecting modified versions via `Synth.addMessageFromHolder()`. The original event can be ignored in the callback using `Message.ignoreEvent()`, while the MessageHolder retains the event data for later modification and re-injection. Note that `Synth.addMessageFromHolder()` makes a copy of the event from the MessageHolder, so the ignored flag on the MessageHolder's stored event does not directly affect the re-injected copy.

Unlike `Message.ignoreEvent()`, which operates on the live event in the audio buffer and directly affects the current callback's processing, `MessageHolder.ignoreEvent()` modifies only the stored copy. The flag has practical effect only if the MessageHolder's event is later re-injected into the processing chain or if external code checks the flag via `HiseEvent::isIgnored()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeIgnored | Integer | no | Whether to mark the event as ignored (true/1) or clear the flag (false/0) | Boolean-like: any truthy value sets the flag |

**Pitfalls:**
None.

**Cross References:**
- `$API.Synth.addMessageFromHolder$`
- `$API.Message.store$`
- `$API.Message.ignoreEvent$`

**Example:**
```javascript:ignore-and-reinject
// Title: ignoreEvent flag does not corrupt event data or timestamp
const var mh = Engine.createMessageHolder();

mh.setType(mh.NoteOn);
mh.setNoteNumber(60);
mh.setVelocity(100);
mh.setChannel(1);
mh.setTimestamp(512);

// Setting and clearing the ignored flag must not alter the timestamp
// (the flag uses reserved bits in the timestamp field)
mh.ignoreEvent(true);
Console.print(mh.getTimestamp()); // 512 -- flag bits don't bleed through

mh.ignoreEvent(false);
Console.print(mh.getTimestamp()); // 512 -- still intact after clearing
```
```json:testMetadata:ignore-and-reinject
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["512", "512"]}
  ]
}
```

---

## isController

**Signature:** `bool isController()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isCC = {obj}.isController();`

**Description:**
Returns true if this event is a controller-like message: a standard MIDI CC, a pitch wheel event, OR an aftertouch event. The implementation checks `e.isController() || e.isPitchWheel() || e.isAftertouch()`, which means it returns true for event types Controller (3), PitchBend (4), and Aftertouch (5).

This broadened definition matches the HISE convention where the `onController` callback receives all three event types. It is broader than `HiseEvent::isController()` alone, which only checks for type Controller.

This method is unique to MessageHolder -- the `Message` class does not need it because the callback type (onNoteOn, onNoteOff, onController) already identifies the event type.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getControllerNumber$`
- `$API.MessageHolder.getControllerValue$`
- `$API.MessageHolder.isNoteOn$`
- `$API.MessageHolder.isNoteOff$`
- `$API.MessageHolder.isMonophonicAfterTouch$`
- `$API.MessageHolder.isPolyAftertouch$`
- `$API.MessageHolder.setType$`

---

## isMonophonicAfterTouch

**Signature:** `bool isMonophonicAfterTouch()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isMono = {obj}.isMonophonicAfterTouch();`

**Description:**
Returns true if this event is an aftertouch event. Delegates to `HiseEvent::isChannelPressure()`, which checks `type == Type::Aftertouch`.

Note: In the HiseEvent system, both monophonic (channel pressure) and polyphonic aftertouch share the same event type (`Aftertouch = 5`). This means `isMonophonicAfterTouch()` and `isPolyAftertouch()` both return true for the same events -- the methods do not actually distinguish between mono and poly aftertouch at the type level. The practical distinction comes from the MIDI-to-HiseEvent conversion: channel pressure copies the pressure value into the `value` byte with `number` set to 0, while polyphonic aftertouch sets `number` to the target note and `value` to the pressure amount.

**Parameters:**
None.

**Pitfalls:**
- `isMonophonicAfterTouch()` and `isPolyAftertouch()` are functionally identical -- both return true for any event with type Aftertouch (5). They cannot be used to distinguish between channel pressure and polyphonic aftertouch. To determine which type of aftertouch an event carries, check the note number field: monophonic aftertouch typically has note number 0, while polyphonic aftertouch has a non-zero note number.

**Cross References:**
- `$API.MessageHolder.isPolyAftertouch$`
- `$API.MessageHolder.getMonophonicAftertouchPressure$`
- `$API.MessageHolder.setMonophonicAfterTouchPressure$`
- `$API.MessageHolder.isController$`
- `$API.MessageHolder.setType$`

---

## isNoteOff

**Signature:** `bool isNoteOff()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var off = {obj}.isNoteOff();`

**Description:**
Returns true if this event is a NoteOff event (type == 2). Delegates directly to `HiseEvent::isNoteOff()`, which checks `type == Type::NoteOff`.

This method is unique to MessageHolder -- the `Message` class does not need it because the `onNoteOn` and `onNoteOff` callbacks already identify the event type. On MessageHolder, this is essential for inspecting events stored from arbitrary callbacks or returned by `MidiPlayer.getEventList()`.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.isNoteOn$`
- `$API.MessageHolder.isController$`
- `$API.MessageHolder.setType$`

---

## isNoteOn

**Signature:** `bool isNoteOn()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var on = {obj}.isNoteOn();`

**Description:**
Returns true if this event is a NoteOn event (type == 1). Delegates directly to `HiseEvent::isNoteOn()`, which checks `type == Type::NoteOn`. In HISE's event system, NoteOn events with velocity 0 are still classified as NoteOn (they are not automatically converted to NoteOff as in some MIDI implementations).

This method is unique to MessageHolder -- the `Message` class does not need it because the `onNoteOn` and `onNoteOff` callbacks already identify the event type. On MessageHolder, this is essential for inspecting events stored from arbitrary callbacks, filtering event lists from `MidiPlayer.getEventList()`, or checking the event type before calling `Synth.addMessageFromHolder()`.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.isNoteOff$`
- `$API.MessageHolder.isController$`
- `$API.MessageHolder.setType$`
- `$API.Synth.addMessageFromHolder$`

---

## isPolyAftertouch

**Signature:** `bool isPolyAftertouch()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var isPoly = {obj}.isPolyAftertouch();`

**Description:**
Returns true if this event is an aftertouch event. Delegates to `HiseEvent::isAftertouch()`, which checks `type == Type::Aftertouch`.

Note: This method is functionally identical to `isMonophonicAfterTouch()` -- both check the same type field. See `isMonophonicAfterTouch()` for details on the monophonic/polyphonic aftertouch ambiguity in the HiseEvent system.

**Parameters:**
None.

**Pitfalls:**
- See `isMonophonicAfterTouch` -- both methods return the same result for all events. They do not distinguish between channel pressure and polyphonic aftertouch.

**Cross References:**
- `$API.MessageHolder.isMonophonicAfterTouch$`
- `$API.MessageHolder.getPolyAfterTouchNoteNumber$`
- `$API.MessageHolder.getPolyAfterTouchPressureValue$`
- `$API.MessageHolder.setPolyAfterTouchNoteNumberAndPressureValue$`
- `$API.MessageHolder.isController$`
- `$API.MessageHolder.setType$`

---

## setChannel

**Signature:** `void setChannel(Number newChannel)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setChannel(1);`

**Description:**
Sets the MIDI channel for this event. The value is stored as a `uint8` internally, supporting HISE's extended 256-channel range (0-255) beyond the standard MIDI 1-16. No range validation is performed -- the value is cast directly to `uint8`.

For standard MIDI usage, channels should be set in the 1-16 range. A channel of 0 is valid internally but falls outside the standard MIDI channel numbering, which starts at 1.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newChannel | Number | yes | MIDI channel number | 1-16 for standard MIDI, 0-255 for extended range |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getChannel$`

---

## setStartOffset

**Signature:** `void setStartOffset(Number offset)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setStartOffset(4410);`

**Description:**
Sets the start offset in samples for this event. The offset is cast to `uint16` before being stored, giving an effective range of 0-65535 samples. Unlike the timestamp (which delays the event's position in the audio buffer), the start offset tells the sound generator to skip the given number of samples when the voice starts. This is used for sample-accurate voice start positioning -- for example, skipping the attack phase of a sample, or aligning playback to a sub-buffer position within a longer sample.

The start offset is stored in a dedicated `uint16` field in the HiseEvent struct, separate from the timestamp. There is no corresponding `getStartOffset()` method on MessageHolder, so the value is write-only from the scripting side. The value can be read back indirectly via `dump()` (which does not include start offset) or by passing the event to `Synth.addMessageFromHolder()` where the sound generator reads it internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| offset | Number | yes | Sample offset for voice start | Cast to uint16 (0-65535) |

**Pitfalls:**
- Negative values or values above 65535 will be silently truncated by the `uint16` cast, wrapping around without error.
- There is no `getStartOffset()` method on MessageHolder. Once set, the value cannot be read back through the scripting API.

**Cross References:**
- `$API.MessageHolder.setTimestamp$`
- `$API.Synth.addMessageFromHolder$`

---

## setTimestamp

**Signature:** `void setTimestamp(Number timestampSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setTimestamp(512);`

**Description:**
Sets the sample-accurate timestamp for this event. The timestamp represents a sample offset from the start of the current audio buffer (or an absolute sample position when working with MidiPlayer event lists, depending on the timestamp edit format).

The underlying `HiseEvent::setTimeStamp()` clamps the value to the range 0 to 0x3FFFFFFF (1,073,741,823) and preserves the upper 2 flag bits (artificial and ignored markers) stored in the same `uint32` field. This means `setTimestamp()` will not accidentally clear or set the artificial/ignored flags.

This method is unique to MessageHolder -- the `Message` class provides `getTimestamp()` for reading but has no setter. Instead, Message uses `delayEvent()` which adds a relative delta. MessageHolder's `setTimestamp()` sets an absolute value, which is more appropriate for constructing events from scratch or rewriting event lists.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| timestampSamples | Number | yes | Absolute timestamp in samples | Clamped to 0..1073741823 |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getTimestamp$`
- `$API.MessageHolder.addToTimestamp$`
- `$API.Synth.addMessageFromHolder$`

**Example:**
```javascript:set-timestamp-event-sequence
// Title: Constructing a timed note sequence with explicit timestamps
const var mh1 = Engine.createMessageHolder();
mh1.setType(mh1.NoteOn);
mh1.setNoteNumber(60);
mh1.setVelocity(100);
mh1.setChannel(1);
mh1.setTimestamp(0);

const var mh2 = Engine.createMessageHolder();
mh2.setType(mh2.NoteOn);
mh2.setNoteNumber(64);
mh2.setVelocity(80);
mh2.setChannel(1);
mh2.setTimestamp(44100);

Console.print(mh1.getTimestamp());
Console.print(mh2.getTimestamp());
```
```json:testMetadata:set-timestamp-event-sequence
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["0", "44100"]}
  ]
}
```

---

## setTransposeAmount

**Signature:** `void setTransposeAmount(Number transposeValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setTransposeAmount(12);`

**Description:**
Sets the transpose amount in semitones for this event. The value is cast to `int8` internally, giving an effective range of -128 to 127 semitones. Values outside this range will be silently truncated by the integer cast.

Transpose is a per-event pitch modifier that preserves the original note number. This is the recommended way to shift pitch in HISE, because NoteOn and NoteOff events automatically match by their original note number regardless of the transpose amount. The actual sounding note number is `getNoteNumber() + getTransposeAmount()`.

This contrasts with `setNoteNumber()`, which overwrites the original note number and requires manual NoteOff matching when the note number has been changed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| transposeValue | Number | yes | Transpose offset in semitones | Stored as int8 (-128..127) |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getTransposeAmount$`
- `$API.MessageHolder.setNoteNumber$`
- `$API.MessageHolder.setCoarseDetune$`

---

## setType

**Signature:** `void setType(Number type)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setType({obj}.NoteOn);`

**Description:**
Sets the MIDI event type for this MessageHolder. The type parameter must be one of the 14 `HiseEvent::Type` constants (0-13) exposed on the MessageHolder instance. The value is validated against the valid range -- values outside 0..13 trigger a script error ("Unknown Type: N").

This method is unique to MessageHolder -- the `Message` class has no type setter because the live event's type is determined by the incoming MIDI data and the callback context. On MessageHolder, `setType()` is essential for constructing events from scratch: a newly created MessageHolder has type Empty (0), which must be changed to a valid type before the event can be used with `Synth.addMessageFromHolder()`.

The event type determines which fields are semantically meaningful (note number/velocity for NoteOn/NoteOff, controller number/value for Controller, etc.), but MessageHolder does not enforce field-type consistency -- all getters and setters work regardless of the current event type.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| type | Number | yes | Event type constant | 0-13 (use MessageHolder constants: Empty, NoteOn, NoteOff, Controller, PitchBend, Aftertouch, AllNotesOff, SongPosition, MidiStart, MidiStop, VolumeFade, PitchFade, TimerEvent, ProgramChange) |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 (Empty) | Uninitialized event; rejected by Synth.addMessageFromHolder() |
| 1 (NoteOn) | Note-on; uses number (note), value (velocity), eventId fields |
| 2 (NoteOff) | Note-off; matched to NoteOn by eventId when re-injected |
| 3 (Controller) | MIDI CC; uses number (CC#), value (CC value) fields |
| 4 (PitchBend) | Pitch wheel; 14-bit value stored across number and value bytes |
| 5 (Aftertouch) | Channel or poly aftertouch; see isMonophonicAfterTouch/isPolyAftertouch |
| 6 (AllNotesOff) | All-notes-off; kills all active voices |
| 7 (SongPosition) | Song position pointer |
| 8 (MidiStart) | MIDI start message |
| 9 (MidiStop) | MIDI stop message |
| 10 (VolumeFade) | Internal volume fade; uses pitch wheel value field for fade time |
| 11 (PitchFade) | Internal pitch fade; uses pitch wheel value field for fade time |
| 12 (TimerEvent) | Timer callback event |
| 13 (ProgramChange) | MIDI program change |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.isNoteOn$`
- `$API.MessageHolder.isNoteOff$`
- `$API.MessageHolder.isController$`
- `$API.MessageHolder.isMonophonicAfterTouch$`
- `$API.MessageHolder.isPolyAftertouch$`
- `$API.Synth.addMessageFromHolder$`

**Example:**
```javascript:set-type-construct-cc
// Title: Constructing a CC event from scratch
const var mh = Engine.createMessageHolder();
mh.setType(mh.Controller);
mh.setChannel(1);
mh.setControllerNumber(1);
mh.setControllerValue(64);

Console.print(mh.dump());
Console.print(mh.isController());
```
```json:testMetadata:set-type-construct-cc
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["Type: Controller, Channel: 1, Number: 1, Value: 64, EventId: 0, Timestamp: 0, ", "1"]}
  ]
}
```

---

## setVelocity

**Signature:** `void setVelocity(Number newVelocity)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setVelocity(100);`

**Description:**
Sets the velocity for this event. The value is cast to `uint8` and written directly to the `value` byte of the underlying HiseEvent via `HiseEvent::setVelocity()`. The effective range is 0-255 (though standard MIDI velocity is 0-127).

Unlike `Message.setVelocity()`, which checks that the event is a NoteOn and that the callback scope is valid (guarded by `ENABLE_SCRIPTING_SAFE_CHECKS`), MessageHolder performs no validation. Calling `setVelocity()` on any event type simply overwrites the `value` byte -- for a Controller event this changes the controller value, for Aftertouch this changes the pressure value, etc. This is consistent with MessageHolder's design as a raw data container.

Note: Setting velocity to 0 on a NoteOn event does NOT automatically convert it to a NoteOff in HISE's event system. `isNoteOn()` will still return true.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newVelocity | Number | yes | Velocity value | Cast to uint8 (0-255); standard MIDI range is 0-127 |

**Pitfalls:**
None.

**Cross References:**
- `$API.MessageHolder.getVelocity$`
- `$API.MessageHolder.setNoteNumber$`
- `$API.MessageHolder.setGain$`
