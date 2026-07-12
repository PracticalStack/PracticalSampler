# Synth -- Method Entries

## addController

**Signature:** `undefined addController(Integer channel, Integer number, Integer value, Integer timeStampSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** The method creates a HiseEvent on the stack and inserts it into the MIDI buffer via addHiseEventToBuffer -- no allocations, no locks.
**Minimal Example:** `Synth.addController(1, 1, 64, 0);`

**Description:**
Adds a controller event to the MIDI buffer with an explicit channel and sample-accurate timestamp. The event is marked as artificial. Unlike `sendController`, this method allows specifying the MIDI channel and a sample-offset timestamp relative to the current event.

The `number` parameter supports three event types through special constants: standard CC (0-127), pitch bend (128), and aftertouch (129). For pitch bend, the value range extends to 0-16383; for standard CC and aftertouch, the range is 0-127.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 |
| number | Integer | no | CC number, or 128 for pitch bend, 129 for aftertouch | 0-129 |
| value | Integer | no | Controller value | 0-127 for CC/aftertouch, 0-16383 for pitch bend |
| timeStampSamples | Integer | no | Sample offset relative to current event timestamp | >= 0 |

**Pitfalls:**
- The artificial flag is set on events created by `addController` but NOT by `sendController`. If downstream logic filters on the artificial flag, the two methods produce different results for the same CC number and value.

**Cross References:**
- `$API.Synth.sendController$`
- `$API.Synth.sendControllerToChildSynths$`

## addEffect

**Signature:** `ScriptEffect addEffect(String type, String id, Integer index)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new processor via ModuleHandler.addModule, which performs heap allocation, acquires a ScopedTicket, kills voices, and uses the GlobalAsyncModuleHandler.
**Minimal Example:** `var fx = Synth.addEffect("SimpleGain", "NewGain", -1);`

**Description:**
Dynamically adds an effect to the parent synth's effect chain at runtime. Returns a `ScriptEffect` handle to the newly created processor. If an effect with the same `id` already exists in the chain, the existing processor is returned instead of creating a duplicate.

The `type` parameter must be a valid effect processor type name. Available type names can be found via the `Synth.ModuleIds` constant object. The `index` parameter controls insertion position: `-1` appends to the end of the chain, while a non-negative value inserts before the effect at that index.

The operation is thread-safe: it suspends audio processing, kills voices, waits for lock-free access, then adds the module asynchronously via the GlobalAsyncModuleHandler.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| type | String | no | Effect processor type name (e.g., "SimpleGain", "SimpleReverb") | Must be a valid factory type |
| id | String | no | Unique ID for the new effect | If an existing effect has this ID, it is returned |
| index | Integer | no | Insertion position in the effect chain | -1 to append, or 0-based index |

**Pitfalls:**
- If `type` is not a valid factory type name, `addModule` throws an internal string exception ("Module with type X could not be generated") which becomes a script error. The available types depend on the parent synth's factory configuration.

**Cross References:**
- `$API.Synth.removeEffect$`
- `$API.Synth.getEffect$`

**Example:**
```javascript:add-effect-basic
// Title: Dynamically adding an effect to the FX chain
const var gain = Synth.addEffect("SimpleGain", "ScriptGain", -1);
gain.setBypassed(true);
```
```json:testMetadata:add-effect-basic
{
  "testable": false,
  "skipReason": "addEffect modifies the module tree which requires a running synth context with proper factory types"
}
```

## addMessageFromHolder

**Signature:** `Integer addMessageFromHolder(ScriptObject messageHolder)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent copy on the stack, registers with EventHandler, and inserts into the MIDI buffer -- all lock-free operations.
**Minimal Example:** `var id = Synth.addMessageFromHolder(holder);`

**Description:**
Inserts a MIDI event from a `MessageHolder` object into the MIDI processing buffer. The event is marked as artificial regardless of its original state. The return value depends on the event type:

- **Note-on:** Registers the event with the EventHandler and Message object, adds to buffer, returns the assigned event ID.
- **Note-off:** Assigns the matching event ID from the EventHandler via `getEventIdForNoteOff`, adds to buffer, returns the event's timestamp.
- **Other events (CC, pitch bend, etc.):** Adds to buffer, returns 0.

This is a low-level method for injecting pre-constructed MIDI events. The `messageHolder` parameter must be a `MessageHolder` object (created via `Engine.createMessageHolder()` or obtained from `Message.store()`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| messageHolder | ScriptObject | no | A MessageHolder object containing the event to inject | Must be a valid MessageHolder, not empty |

**Pitfalls:**
- The return value meaning changes based on event type: event ID for note-on, timestamp for note-off, 0 for everything else. Callers must know the event type to interpret the return value correctly.
- For note-off events, the method uses `getEventIdForNoteOff` to find the matching note-on. If no matching note-on was registered (e.g., a note-on from a different source), the note-off may get event ID 0 and fail to stop the intended voice.

**Cross References:**
- `$API.Synth.addNoteOn$`
- `$API.Synth.addNoteOff$`
- `$API.Synth.addController$`

## addModulator

**Signature:** `ScriptObject addModulator(Integer chainId, String type, String id)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new processor via ModuleHandler.addModule, which performs heap allocation, acquires a ScopedTicket, kills voices, and uses the GlobalAsyncModuleHandler.
**Minimal Example:** `var mod = Synth.addModulator(1, "LFOModulator", "ScriptLFO");`

**Description:**
Dynamically adds a modulator to the parent synth's gain or pitch modulation chain. Returns a `ScriptModulator` handle to the newly created processor. If a modulator with the same `id` already exists in the target chain, the existing processor is returned instead of creating a duplicate.

The `chainId` parameter uses the C++ `ModulatorSynth::InternalChains` enum values directly: `1` = GainModulation, `2` = PitchModulation. The `type` parameter must be a valid modulator type name from the chain's factory (available via the `Synth.ModuleIds` constant object). The modulator is always appended to the end of the chain (no insertion index parameter).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainId | Integer | no | Target modulation chain | 1 = GainModulation, 2 = PitchModulation |
| type | String | no | Modulator type name (e.g., "LFOModulator", "SimpleEnvelope") | Must be a valid factory type for the target chain |
| id | String | no | Unique ID for the new modulator | If existing, returns that modulator |

**Pitfalls:**
- The `chainId` values (1=Gain, 2=Pitch) do not start at 0. Passing 0 produces a script error "No valid chainType". The base JSON description incorrectly states "PitchModulation = 0" -- the correct value is 2.

**Cross References:**
- `$API.Synth.removeModulator$`
- `$API.Synth.getModulator$`
- `$API.Synth.getModulatorIndex$`
- `$API.Synth.setModulatorAttribute$`

**Example:**
```javascript:add-modulator-basic
// Title: Adding an LFO to the gain chain
const var lfo = Synth.addModulator(1, "LFOModulator", "DynamicLFO");
lfo.setAttribute(lfo.Frequency, 2.0);
lfo.setBypassed(false);
```
```json:testMetadata:add-modulator-basic
{
  "testable": false,
  "skipReason": "addModulator modifies the module tree which requires a running synth context with proper factory types"
}
```

## addNoteOff

**Signature:** `undefined addNoteOff(Integer channel, Integer noteNumber, Integer timeStampSamples)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack, queries EventHandler for matching event ID, and inserts into buffer -- all lock-free.
**Minimal Example:** `Synth.addNoteOff(1, 60, 100);`

**Description:**
Adds an artificial note-off event to the MIDI buffer with an explicit channel and sample-accurate timestamp. The note-off velocity is hardcoded to 127. The method looks up the matching note-on event ID via `getEventIdForNoteOff` and assigns it to the note-off, enabling proper voice management.

The timestamp is relative to the current event: if called inside a MIDI callback, the timestamp is added to the current event's timestamp. The minimum effective timestamp is clamped to 1 (`jmax(1, timeStampSamples)`), meaning a timestamp of 0 becomes 1 sample.

This is a low-level note-off method that matches by channel and note number. For reliable voice management with artificial notes, prefer `noteOffByEventId` or `noteOffDelayedByEventId` which match by event ID.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 |
| noteNumber | Integer | no | MIDI note number | 0-127 |
| timeStampSamples | Integer | no | Sample offset relative to current event timestamp | >= 0 (clamped to minimum 1) |

**Pitfalls:**
- The timestamp is clamped to a minimum of 1 sample even when 0 is passed. This means `addNoteOff` can never produce a note-off at the exact same sample position as the current event. Use `noteOffByEventId` for immediate note-offs.
- The method matches note-offs by channel and note number, not by event ID. With multiple overlapping notes on the same pitch, it may stop the wrong voice. Prefer `noteOffByEventId` for unambiguous voice control.
- Unlike `addNoteOn`, this method does not return a value. There is no way to confirm which event ID was matched.

**Cross References:**
- `$API.Synth.addNoteOn$`
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.noteOffDelayedByEventId$`

## addNoteOn

**Signature:** `Integer addNoteOn(Integer channel, Integer noteNumber, Integer velocity, Integer timeStampSamples)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack, registers with EventHandler, and inserts into buffer -- all lock-free operations.
**Minimal Example:** `var id = Synth.addNoteOn(1, 60, 100, 0);`

**Description:**
Adds an artificial note-on event to the MIDI buffer with explicit channel, velocity, and sample-accurate timestamp. Returns the event ID assigned to the new note, which should be stored for later use with `noteOffByEventId` or `addVolumeFade`.

The event is flagged as artificial and registered with both the EventHandler (for voice management) and the Message object (for event tracking). The timestamp is relative to the current event's timestamp when called inside a MIDI callback.

When `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` is enabled (the default), timestamps are adjusted by subtracting one audio block size when running on the audio thread. This preserves timing compatibility with older patches.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 |
| noteNumber | Integer | no | MIDI note number | 0-127 |
| velocity | Integer | no | Note velocity | 0-127 |
| timeStampSamples | Integer | no | Sample offset relative to current event timestamp | >= 0 |

**Pitfalls:**
- Unlike `playNote` (which rejects velocity 0), `addNoteOn` accepts velocity 0. A note-on with velocity 0 is technically valid in MIDI but may produce silent voices depending on the synth's configuration.
- Always store the returned event ID. Without it, you cannot reliably stop the note later -- `addNoteOff` matches by note number which is ambiguous with overlapping notes.

**Cross References:**
- `$API.Synth.addNoteOff$`
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.noteOffDelayedByEventId$`
- `$API.Synth.addVolumeFade$`
- `$API.Synth.playNote$`
- `$API.Synth.playNoteWithStartOffset$`

**DiagramRef:** synth-midi-event-flow

## removeEffect

**Signature:** `Integer removeEffect(ScriptObject effect)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `ModuleHandler::removeModule`, which checks the current thread and throws if on the audio thread, then schedules asynchronous removal via `GlobalAsyncModuleHandler::removeAsync`. Involves thread checking, async scheduling, and internal handler operations.
**Minimal Example:** `var ok = Synth.removeEffect(fx);`

**Description:**
Removes a previously added effect from the parent synth's effect chain. The `effect` parameter must be a `ScriptEffect` handle (as returned by `addEffect` or `getEffect`). Returns `true` if the removal was successfully scheduled, `false` if the parameter was invalid.

The removal is performed asynchronously and thread-safely via `ModuleHandler::removeModule`:
1. If called from the audio thread, an exception is thrown ("Effects can't be removed from the audio thread!").
2. If the processor pointer is null (already removed or invalid handle), returns `true` immediately (no-op).
3. Otherwise, schedules asynchronous removal via `GlobalAsyncModuleHandler::removeAsync`, which removes the processor from its parent chain without deleting it (the GlobalAsyncModuleHandler manages the deferred deletion).

The method always returns `true` when given a valid `ScriptEffect` handle, even before the removal is actually completed (it is async). It returns `false` only when the parameter is not a `ScriptEffect` object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| effect | ScriptObject | no | A ScriptEffect handle to the effect to remove | Must be a ScriptEffect (dynamic_cast check) |

**Pitfalls:**
- If the `effect` parameter is not a `ScriptEffect` object (e.g., a `ScriptModulator`, a string, or `null`), the method silently returns `false` without any error. There is no type-check error to help the user diagnose passing the wrong object.
- The removal is asynchronous. After `removeEffect` returns `true`, the effect may still be active briefly until the GlobalAsyncModuleHandler processes the removal. Immediately calling `getEffect` or `getAllEffects` may still find the processor.
- Calling from the audio thread throws a string exception, which may crash in non-throwing builds. This is the same pattern used by `removeModulator`.

**Cross References:**
- `$API.Synth.addEffect$`
- `$API.Synth.getEffect$`
- `$API.Synth.getAllEffects$`
- `$API.Synth.removeModulator$`

## removeModulator

**Signature:** `Integer removeModulator(ScriptObject mod)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to `ModuleHandler::removeModule`, which checks the current thread and throws if on the audio thread, then schedules asynchronous removal via `GlobalAsyncModuleHandler::removeAsync`. Involves thread checking, async scheduling, and internal handler operations.
**Minimal Example:** `var ok = Synth.removeModulator(mod);`

**Description:**
Removes a previously added modulator from the parent synth's gain or pitch modulation chain. The `mod` parameter must be a `ScriptModulator` handle (as returned by `addModulator` or `getModulator`). Returns `true` if the removal was successfully scheduled, `false` if the parameter was invalid.

The removal mechanism is identical to `removeEffect` -- it delegates to `ModuleHandler::removeModule`:
1. If called from the audio thread, an exception is thrown ("Effects can't be removed from the audio thread!" -- note the error message says "Effects" even for modulators, since the same `removeModule` function is shared).
2. If the processor pointer is null, returns `true` immediately (no-op).
3. Otherwise, schedules asynchronous removal via `GlobalAsyncModuleHandler::removeAsync`.

The method always returns `true` when given a valid `ScriptModulator` handle. It returns `false` only when the parameter is not a `ScriptModulator` object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| mod | ScriptObject | no | A ScriptModulator handle to the modulator to remove | Must be a ScriptModulator (dynamic_cast check) |

**Pitfalls:**
- If the `mod` parameter is not a `ScriptModulator` object (e.g., a `ScriptEffect`, a string, or `null`), the method silently returns `false` without any error. There is no type-check error to help the user diagnose passing the wrong object.
- [BUG] The audio-thread exception message says "Effects can't be removed from the audio thread!" even when removing a modulator. This is because both `removeEffect` and `removeModulator` share the same `ModuleHandler::removeModule` implementation.
- The removal is asynchronous. After `removeModulator` returns `true`, the modulator may still be active briefly until the GlobalAsyncModuleHandler processes the removal.

**Cross References:**
- `$API.Synth.addModulator$`
- `$API.Synth.getModulator$`
- `$API.Synth.getAllModulators$`
- `$API.Synth.removeEffect$`

## noteOffDelayedByEventId

**Signature:** `undefined noteOffDelayedByEventId(Integer eventId, Integer timestamp)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack, pops the matching note-on from EventHandler (a fixed-size array lookup), and inserts into the MIDI buffer via addHiseEventToBuffer -- all lock-free operations.
**Minimal Example:** `Synth.noteOffDelayedByEventId(eventId, 512);`

**Description:**
Sends a note-off message for the specified event ID with a sample-accurate delay. This is the canonical note-off method that all other note-off-by-ID methods delegate to.

The method performs these steps:
1. Pops the matching note-on from the EventHandler's ring buffer via `popNoteOnFromEventId((uint16)eventId)`.
2. If the note-on was found (not empty): creates a NoteOff event with the same note number and channel, sets the event ID, computes the final timestamp (relative to the current event plus the delay), marks it artificial if the original was artificial, and inserts it into the MIDI buffer.
3. If the note-on was already popped (empty): calls `setArtificialTimestamp(eventId, timestamp)` to adjust the timestamp of a previously issued note-off. This allows rescheduling an already-queued note-off.

The `timestamp` parameter is a sample offset. When `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` is enabled (default), the timestamp is adjusted by subtracting one audio block size on the audio thread (clamped to 0). The final timestamp is then added to the current event's timestamp if called within a MIDI callback.

Requires a MIDI processor context (`parentMidiProcessor` must be non-null).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID of the note to stop | > 0; cast to uint16 internally |
| timestamp | Integer | no | Sample delay before the note-off is issued | >= 0 (adjusted by backwards-compatible timestamp logic) |

**Pitfalls:**
- Under `ENABLE_SCRIPTING_SAFE_CHECKS`, attempting to kill a non-artificial event produces the script error "Hell breaks loose if you kill real events artificially!". Only artificial notes can be stopped via event ID.
- The `eventId` is cast to `uint16` before the EventHandler lookup. If the original event ID exceeds the uint16 range (65535), truncation occurs and the wrong event slot is queried.
- When the note-on has already been popped (e.g., by a prior `noteOffByEventId` call), the method does not fail -- it silently calls `setArtificialTimestamp` to update the timestamp of the existing note-off. This is by design for rescheduling scenarios, but it means a second call does not generate a second note-off event.

**Cross References:**
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.addVolumeFade$`

**DiagramRef:** synth-midi-event-flow

**Example:**
```javascript:delayed-note-off
// Title: Play a note and schedule a delayed note-off 500 samples later
reg eventId = 0;

inline function onNoteOn()
{
    eventId = Synth.playNote(Message.getNoteNumber(), Message.getVelocity());
    
    // Schedule note-off 500 samples after the current event
    Synth.noteOffDelayedByEventId(eventId, 500);
}
```
```json:testMetadata:delayed-note-off
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with a playing note"
}
```

## noteOffFromUI

**Signature:** `undefined noteOffFromUI(Integer channel, Integer noteNumber)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `CustomKeyboardState::injectMessage()` which posts a MIDI message through the message-thread keyboard state pipeline. The `injectMessage` method involves internal locking on the keyboard state's critical section.
**Minimal Example:** `Synth.noteOffFromUI(1, 60);`

**Description:**
Injects a MIDI note-off event through the virtual keyboard input pipeline, as if the user released a key on the on-screen MIDI keyboard. Unlike `noteOffByEventId` or `addNoteOff` which insert events directly into the MIDI processor buffer, this method routes through `CustomKeyboardState::injectMessage()`, which processes the event through the same path as physical keyboard input and UI keyboard clicks.

This is designed for UI-driven note control -- for example, a custom on-screen keyboard or a scripted sequencer that should behave identically to the built-in virtual keyboard. The injected event is a standard JUCE `MidiMessage::noteOff` and enters the MIDI pipeline at the input stage, not as an artificial event in the processor chain.

No validation is performed on the channel or note number parameters. Invalid values are passed directly to `MidiMessage::noteOff`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 (standard MIDI range) |
| noteNumber | Integer | no | MIDI note number to release | 0-127 |

**Pitfalls:**
- Events injected via `noteOffFromUI` are NOT marked as artificial. They enter the standard MIDI input pipeline and are processed as real MIDI events. This means `handleNoteCounter` will update the `keyDown` bitfield and `numPressedKeys` counter, and `isKeyDown`/`getNumPressedKeys` will reflect these events. In contrast, events from `noteOffByEventId` or `addNoteOff` are artificial and do not affect keyboard state tracking.
- There is no event ID matching. The note-off is matched to a note-on by channel and note number through the normal MIDI pipeline. If the note was originally started with `playNoteFromUI`, this works correctly. If the note was started with `playNote` or `addNoteOn` (artificial events), using `noteOffFromUI` to stop it may not match correctly.
- No parameter validation is performed. Out-of-range channel or note number values produce undefined behavior in the JUCE `MidiMessage` constructor.

**Cross References:**
- `$API.Synth.playNoteFromUI$`
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.addNoteOff$`

## playNote

**Signature:** `Integer playNote(Integer noteNumber, Integer velocity)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Delegates to `internalAddNoteOn` which creates a HiseEvent on the stack, registers with EventHandler (fixed-size array write), and inserts into the MIDI buffer -- all lock-free operations.
**Minimal Example:** `var id = Synth.playNote(60, 100);`

**Description:**
Plays an artificial note-on event and returns its event ID. This is the simplest note generation method -- it uses fixed defaults for channel (1), timestamp (0), and start offset (0). The returned event ID must be stored to later stop the note via `noteOffByEventId` or `noteOffDelayedByEventId`.

The method delegates to `internalAddNoteOn(1, noteNumber, velocity, 0, 0)`. The generated event is marked as artificial and registered with both the EventHandler (for voice management) and the Message object (for event tracking). The note inherits the current event's timestamp when called inside a MIDI callback.

Velocity 0 is explicitly rejected with a script error "A velocity of 0 is not valid!". This is a deliberate guard -- use `noteOffByEventId` to stop notes, not a zero-velocity note-on.

Requires a MIDI processor context (`parentMidiProcessor` must be non-null). Calling from a modulator or effect script produces "Only valid in MidiProcessors".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Integer | no | MIDI note number to play | 0-127 |
| velocity | Integer | no | Note velocity | 1-127 (0 rejected with error) |

**Pitfalls:**
- Always store the returned event ID. Without it, there is no reliable way to stop the note later. Using the deprecated `noteOff(noteNumber)` to stop notes is unreliable when multiple voices play the same pitch.
- The channel is hardcoded to 1. If you need a specific MIDI channel, use `addNoteOn` or `playNoteWithStartOffset` instead.
- Unlike `addNoteOn`, `playNote` rejects velocity 0 with a script error. This is intentional -- a note-on with velocity 0 is ambiguous (it could mean "note off" in standard MIDI). Use the dedicated note-off methods instead.

**Cross References:**
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.noteOffDelayedByEventId$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.playNoteWithStartOffset$`
- `$API.Synth.addVolumeFade$`

**DiagramRef:** synth-midi-event-flow

**Example:**
```javascript:play-note-octave-layer
// Title: Layer an artificial note one octave above each incoming note
reg artificialId = 0;

inline function onNoteOn()
{
    // Play an artificial note one octave up
    artificialId = Synth.playNote(Message.getNoteNumber() + 12, Message.getVelocity());
}

inline function onNoteOff()
{
    // Stop the artificial note when the real note is released
    Synth.noteOffByEventId(artificialId);
}
```
```json:testMetadata:play-note-octave-layer
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events"
}
```

## playNoteFromUI

**Signature:** `undefined playNoteFromUI(Integer channel, Integer noteNumber, Integer velocity)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `CustomKeyboardState::injectMessage()` which posts a MIDI message through the message-thread keyboard state pipeline. The `injectMessage` method involves internal locking on the keyboard state's critical section.
**Minimal Example:** `Synth.playNoteFromUI(1, 60, 100);`

**Description:**
Injects a MIDI note-on event through the virtual keyboard input pipeline, as if the user pressed a key on the on-screen MIDI keyboard. Unlike `playNote` or `addNoteOn` which insert artificial events directly into the MIDI processor buffer, this method routes through `CustomKeyboardState::injectMessage()`, which processes the event through the same path as physical MIDI keyboard input and UI keyboard clicks.

This is designed for UI-driven note triggering -- for example, a custom on-screen keyboard built with ScriptPanels, a step sequencer, or any UI element that should simulate real keyboard input. The injected event is a standard JUCE `MidiMessage::noteOn` and enters the MIDI pipeline at the input stage.

No event ID is returned because the event enters the standard MIDI pipeline as a real (non-artificial) event. Use `noteOffFromUI` to stop notes started with this method.

No validation is performed on parameters. Invalid channel, note number, or velocity values are passed directly to `MidiMessage::noteOn`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 (standard MIDI range) |
| noteNumber | Integer | no | MIDI note number | 0-127 |
| velocity | Integer | no | Note velocity (cast to uint8) | 0-127 |

**Pitfalls:**
- No event ID is returned. You cannot use `noteOffByEventId` to stop notes started with `playNoteFromUI`. You must use `noteOffFromUI` with the same channel and note number to release the note.
- Events injected via `playNoteFromUI` are NOT marked as artificial. They enter the standard MIDI input pipeline and are processed as real MIDI events. The `keyDown` bitfield and `numPressedKeys` counter are updated by these events (via `handleNoteCounter`), and `isKeyDown`/`getNumPressedKeys`/`isLegatoInterval` will reflect them.
- No parameter validation is performed. Out-of-range values are passed directly to JUCE's `MidiMessage::noteOn`, which casts the velocity to `uint8`. Values outside 0-127 will be truncated.

**Cross References:**
- `$API.Synth.noteOffFromUI$`
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`

## playNoteWithStartOffset

**Signature:** `Integer playNoteWithStartOffset(Integer channel, Integer number, Integer velocity, Integer offset)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Delegates to `internalAddNoteOn` which creates a HiseEvent on the stack, registers with EventHandler (fixed-size array write), and inserts into the MIDI buffer -- all lock-free operations.
**Minimal Example:** `var id = Synth.playNoteWithStartOffset(1, 60, 100, 4410);`

**Description:**
Plays an artificial note-on event with an explicit MIDI channel and sample start offset, returning the event ID. The start offset tells the sampler to begin playback at the specified sample position within the audio file rather than from the beginning. This is essential for sample-accurate audio slicing, re-triggering from specific positions, and skip-ahead playback.

The method delegates to `internalAddNoteOn(channel, number, velocity, 0, offset)`. The timestamp is fixed at 0 (the note plays immediately, inheriting the current event's timestamp). The returned event ID must be stored for later note-off operations.

Velocity 0 is rejected with a script error "A velocity of 0 is not valid!", matching `playNote`'s behavior.

The start offset is clamped to `UINT16_MAX` (65535 samples). Passing a value greater than 65535 produces a script error "Max start offset is 65536 (2^16)" (note: the error message is off by one -- the actual maximum accepted value is 65535).

Requires a MIDI processor context (`parentMidiProcessor` must be non-null).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| channel | Integer | no | MIDI channel | 1-16 |
| number | Integer | no | MIDI note number | 0-127 |
| velocity | Integer | no | Note velocity | 1-127 (0 rejected with error) |
| offset | Integer | no | Sample start offset for playback | 0-65535 (stored as uint16) |

**Pitfalls:**
- [BUG] The start offset is cast to `uint16` after validation. The error message says "Max start offset is 65536" but the actual maximum accepted value is 65535 (`UINT16_MAX`). Passing exactly 65536 triggers the error.
- The timestamp is fixed at 0. If you need both a start offset AND a non-zero timestamp, you must use `addNoteOn` (which has explicit timestamp but no start offset) combined with `Message.setStartOffset()` on the event, or construct the event manually via `MessageHolder`.

**Cross References:**
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.noteOffDelayedByEventId$`
- `$API.Synth.addVolumeFade$`

**DiagramRef:** synth-midi-event-flow


## addPitchFade

**Signature:** `undefined addPitchFade(Integer eventId, Integer fadeTimeMilliseconds, Integer targetCoarsePitch, Integer targetFinePitch)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack and inserts it into the MIDI buffer via addHiseEventToBuffer -- no allocations, no locks.
**Minimal Example:** `Synth.addPitchFade(eventId, 500, 12, 0);`

**Description:**
Applies a pitch fade to an active voice identified by its event ID. The fade smoothly transitions from the voice's current pitch to the target pitch over the specified time in milliseconds. The target pitch is specified in two components: `targetCoarsePitch` in semitones and `targetFinePitch` in cents (0-100).

Unlike `addVolumeFade`, this method has no auto-kill behavior -- it only changes pitch. The fade event inherits the current event's timestamp but does not add any additional sample offset.

The coarse and fine pitch values are cast to `uint8`, so negative values or values above 255 will wrap. For downward pitch shifts, the coarse pitch must be expressed relative to the voice's original tuning context.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID of the target note (from addNoteOn/playNote return value) | > 0 |
| fadeTimeMilliseconds | Integer | no | Duration of the pitch transition | >= 0 |
| targetCoarsePitch | Integer | no | Target pitch offset in semitones | Cast to uint8 |
| targetFinePitch | Integer | no | Target pitch fine-tune in cents | Cast to uint8 (0-100 typical) |

**Pitfalls:**
- The `targetCoarsePitch` and `targetFinePitch` are cast to `uint8` internally. Values outside 0-255 will silently wrap due to truncation. Negative semitone values will not work as expected.

**Cross References:**
- `$API.Synth.addVolumeFade$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.playNote$`
- `$API.Synth.noteOffByEventId$`

## addToFront

**Signature:** `undefined addToFront(Integer addToFront)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a boolean flag and in frontend builds unsuspends the content update dispatcher -- no allocations or locks.
**Minimal Example:** `Synth.addToFront(true);`

**Description:**
Designates this script processor's interface as the main plugin UI. When `addToFront` is `true`, the script processor's `Content` area is used as the front-end interface -- it becomes the visible UI in the compiled plugin and is returned by `getFirstInterfaceScriptProcessor()`.

This must be called in `onInit`. Only one script processor in the entire module tree should have `addToFront(true)`. In a typical project, the top-level Script Processor calls `Synth.addToFront(true)` to establish the main interface.

In exported frontend builds (`USE_FRONTEND`), calling this also unsuspends the content update dispatcher, enabling UI updates.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| addToFront | Integer | no | Whether to mark this processor as the front interface | Boolean (0 or 1) |

**Pitfalls:**
- The method performs a `dynamic_cast` to `JavascriptMidiProcessor` internally. If the Synth object belongs to a non-MIDI script processor (e.g., a Script FX or modulator), this cast may fail. In practice, `addToFront` is only meaningful for Script Processors (JavascriptMidiProcessor).
- If multiple script processors call `addToFront(true)`, `getFirstInterfaceScriptProcessor` returns the first one found during iteration. The behavior is undefined -- only one processor should be the front interface.

**Cross References:**
- `$API.Synth.deferCallbacks$`

## addVolumeFade

**Signature:** `undefined addVolumeFade(Integer eventId, Integer fadeTimeMilliseconds, Integer targetVolume)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack and inserts it into the MIDI buffer via addHiseEventToBuffer -- no allocations, no locks. The auto-kill path (targetVolume == -100) also pops a note-on from the EventHandler and inserts a note-off, all lock-free operations.
**Minimal Example:** `Synth.addVolumeFade(eventId, 500, -12);`

**Description:**
Applies a volume fade to an active voice identified by its event ID. The fade smoothly transitions from the voice's current volume to the target volume (in decibels) over the specified time in milliseconds. The volume fade event inherits the current event's timestamp.

When `targetVolume` is exactly `-100`, the method triggers a "fade to silence and kill" sequence: in addition to the volume fade event, it pops the original note-on from the EventHandler and inserts an automatic note-off event timed to arrive one sample after the fade completes. This is the recommended way to fade out and release an artificial note in a single call. The auto-kill only works on artificial events -- attempting it on a real (non-artificial) event produces a script error.

Requires a MIDI processor context (`parentMidiProcessor` must be non-null). Calling from a modulator or effect script produces a "Only valid in MidiProcessors" error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID of the target note (from addNoteOn/playNote return value) | > 0 |
| fadeTimeMilliseconds | Integer | no | Duration of the volume transition in milliseconds | >= 0 |
| targetVolume | Integer | no | Target volume in decibels. -100 triggers fade-to-silence-and-kill | Cast to uint8 internally |

**Pitfalls:**
- The `targetVolume` parameter is cast to `uint8` when creating the volume fade event via `HiseEvent::createVolumeFade`. However, the special `-100` check occurs before this cast, so the auto-kill behavior works correctly despite the cast. For non-kill volume targets, values are truncated to 0-255.
- The auto-kill note-off timestamp is calculated as `(1.0 + fadeTimeMs / 1000.0 * sampleRate)`, meaning it arrives one sample after the fade would complete. There is no way to customize this offset.

**Cross References:**
- `$API.Synth.addPitchFade$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.playNote$`
- `$API.Synth.noteOffByEventId$`

**DiagramRef:** synth-midi-event-flow

**Example:**
```javascript:volume-fade-and-kill
// Title: Fade out an artificial note over 200ms and auto-release it
reg eventId = 0;

inline function onNoteOn()
{
    eventId = Synth.playNote(Message.getNoteNumber(), Message.getVelocity());
    // Fade to silence and auto-kill after 200ms
    Synth.addVolumeFade(eventId, 200, -100);
}
```
```json:testMetadata:volume-fade-and-kill
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with a playing note to demonstrate the fade"
}
```

## attachNote

**Signature:** `Integer attachNote(Integer originalNoteId, Integer artificialNoteId)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Delegates to MidiProcessorChain::attachNote which inserts into a pre-allocated buffer -- no allocations, no locks.
**Minimal Example:** `var ok = Synth.attachNote(Message.getEventId(), artificialId);`

**Description:**
Links an artificial note to a real (or other) note so that when the original note receives a note-off, the artificial note is automatically stopped as well. Returns `true` if the attachment was successful, `false` otherwise.

This is used for note-layering scenarios where a script generates one or more artificial notes in response to an incoming note-on, and all artificial notes should be released together when the original key is released. Without `attachNote`, the script must manually track event IDs and issue note-offs in `onNoteOff`.

Requires `Synth.setFixNoteOnAfterNoteOff(true)` to be called first (typically in `onInit`). If the attached note buffer has not been enabled, a script error is reported. Also requires a MIDI processor context -- `parentMidiProcessor` must be non-null (returns `false` silently if null).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| originalNoteId | Integer | no | Event ID of the original (real) note to attach to | Valid active event ID |
| artificialNoteId | Integer | no | Event ID of the artificial note to be auto-stopped | Must be an artificial event ID |

**Pitfalls:**
- If `parentMidiProcessor` is null (i.e., called from a non-MIDI processor), the method silently returns `false` without any error message. This differs from other MIDI-requiring methods (like `addVolumeFade`) which report "Only valid in MidiProcessors".
- The method does not validate whether the event IDs correspond to currently active notes. Passing invalid or expired event IDs silently returns the result of `MidiProcessorChain::attachNote`, which may be `false` without explanation.

**Cross References:**
- `$API.Synth.setFixNoteOnAfterNoteOff$`
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.noteOffByEventId$`

**Example:**
```javascript:attach-note-layering
// Title: Auto-release artificial notes when the original key is released
// In onInit:
Synth.setFixNoteOnAfterNoteOff(true);

reg artificialId = 0;

inline function onNoteOn()
{
    // Generate an artificial note one octave up
    artificialId = Synth.addNoteOn(1, Message.getNoteNumber() + 12, Message.getVelocity(), 0);
    
    // Attach so releasing the original key also stops the artificial note
    Synth.attachNote(Message.getEventId(), artificialId);
}
```
```json:testMetadata:attach-note-layering
{
  "testable": false,
  "skipReason": "Requires active MIDI callback context with incoming note events"
}
```

## createBuilder

**Signature:** `ScriptObject createBuilder()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new ScriptBuilder object on the heap via `new ScriptingObjects::ScriptBuilder(...)`.
**Minimal Example:** `var builder = Synth.createBuilder();`

**Description:**
Creates and returns a `Builder` object that allows programmatic construction and modification of the module tree at runtime. The Builder API provides methods to create, configure, and connect processors (synths, effects, modulators) without using the HISE IDE.

The returned `Builder` object is a factory for module tree operations. Typical workflow: call `builder.clear()` to reset, use `builder.create()` to add modules, configure them, then call `builder.flush()` to apply all changes atomically. The Builder's methods handle thread-safe module creation internally.

This method has no restrictions on when it can be called, but the Builder object's own methods (especially `flush()`) perform heavyweight operations that suspend audio processing.

**Parameters:**

(None)

**Cross References:**
- `$API.Builder.create$`
- `$API.Builder.flush$`
- `$API.Builder.clear$`

**Example:**
```javascript:create-builder-basic
// Title: Create a Builder and add an effect to the module tree
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "MyGain", 0, builder.ChainIndexes.FX);
builder.flush();
```
```json:testMetadata:create-builder-basic
{
  "testable": false,
  "skipReason": "Builder operations modify the module tree and require a running synth context with factory types"
}
```

## deferCallbacks

**Signature:** `undefined deferCallbacks(Integer makeAsynchronous)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Stops the synth timer (which may involve ModulatorSynth internals) and in the non-deferred-to-deferred transition, starts a JUCE Timer which allocates internally.
**Minimal Example:** `Synth.deferCallbacks(true);`

**Description:**
Switches the script processor's MIDI callback execution between audio thread (default) and message thread (deferred) modes. When `makeAsynchronous` is `true`, all MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`) are deferred to the message thread.

Deferred mode has three significant consequences:

1. **MIDI messages become read-only.** You cannot modify the incoming MIDI event (e.g., `Message.setNoteNumber()` will not work). The event has already been processed on the audio thread before the callback fires.
2. **Timer switches to JUCE Timer.** Instead of the sample-accurate synth timer (`HISE_EVENT_RASTER` resolution, 4 slots per synth), the processor uses a standard JUCE `Timer` with millisecond resolution and no slot limit.
3. **Audio-thread safety is relaxed.** Since callbacks no longer run on the audio thread, you can safely perform allocations, string operations, and UI updates directly in callbacks.

This is typically called in `onInit`. The method delegates to `JavascriptMidiProcessor::deferCallbacks()`, which also stops any running timer (synth timer if transitioning to deferred, JUCE timer if transitioning to non-deferred) to avoid orphaned timer callbacks.

Only works when the Synth object belongs to a `JavascriptMidiProcessor`. The internal `dynamic_cast` will fail if called from other script processor types (modulators, effects), which would cause undefined behavior.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| makeAsynchronous | Integer | no | Whether to defer callbacks to the message thread | Boolean (0 or 1) |

**Pitfalls:**
- Switching from deferred to non-deferred mode stops any running JUCE Timer but does not automatically restart a synth timer. If a timer was running before the mode switch, you must call `Synth.startTimer()` again after switching back to non-deferred mode.
- The method performs an unchecked `dynamic_cast<JavascriptMidiProcessor*>` on the script processor. If somehow called from a non-JavascriptMidiProcessor context, this produces a null pointer dereference. In practice this is unlikely since only JavascriptMidiProcessor hosts expose the Synth namespace with full MIDI capabilities.

**Cross References:**
- `$API.Synth.startTimer$`
- `$API.Synth.stopTimer$`
- `$API.Synth.addToFront$`

## getAllEffects

**Signature:** `Array getAllEffects(String regex)`
**Return Type:** `Array`
**Call Scope:** init
**Call Scope Note:** Checks `objectsCanBeCreated()` which restricts to onInit. Also allocates ScriptEffect wrapper objects on the heap.
**Minimal Example:** `var effects = Synth.getAllEffects(".*");`

**Description:**
Returns an array of `ScriptEffect` handles for all effect processors in the parent synth's subtree whose IDs match the given wildcard pattern. Uses owner-rooted search (`Processor::Iterator<EffectProcessor>(owner)`) -- only effects within the parent synth's module tree are returned, not effects in sibling or parent synths.

The `regex` parameter uses HISE's wildcard matching (`RegexFunctions::matchesWildcard`), which supports `*` as a glob wildcard (not full regex syntax). For example, `"Delay*"` matches "Delay1", "DelayFX", etc. Use `".*"` to match all effects.

Each matched effect is wrapped in a new `ScriptEffect` object. The returned array can be empty if no effects match the pattern. The method is restricted to `onInit` via `objectsCanBeCreated()`.

Has `WARN_IF_AUDIO_THREAD` guard for debug builds.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Wildcard pattern to match against effect processor IDs | Supports `*` glob wildcard |

**Pitfalls:**
- [BUG] The method does not report an `onInit`-only error when called outside `onInit`. Instead, if `objectsCanBeCreated()` returns false, it falls through to `RETURN_IF_NO_THROW({})` and returns an empty object rather than an array. This differs from other `get*()` methods that call `reportIllegalCall()` -- `getAllEffects` just silently returns nothing.

**Cross References:**
- `$API.Synth.getEffect$`
- `$API.Synth.addEffect$`
- `$API.Synth.removeEffect$`

## getAllModulators

**Signature:** `Array getAllModulators(String regex)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates ScriptingModulator wrapper objects on the heap via `new ScriptingObjects::ScriptingModulator(...)`. Also constructs an Array on the heap.
**Minimal Example:** `var mods = Synth.getAllModulators("LFO*");`

**Description:**
Returns an array of `ScriptModulator` handles for all modulators in the **entire module tree** whose IDs match the given wildcard pattern. Unlike most `get*()` methods which search only the parent synth's subtree, `getAllModulators` uses a global-rooted search starting from `getMainSynthChain()`.

The `regex` parameter uses HISE's wildcard matching (`RegexFunctions::matchesWildcard`), which supports `*` as a glob wildcard. Use `".*"` to match all modulators across the entire project.

This method does NOT check `objectsCanBeCreated()` and does NOT have a `WARN_IF_AUDIO_THREAD` guard. It can technically be called outside `onInit`, but this is not recommended since it allocates wrapper objects on every call. Each matched modulator is wrapped in a new `ScriptModulator` object.

The returned array can be empty if no modulators match the pattern.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| regex | String | no | Wildcard pattern to match against modulator IDs | Supports `*` glob wildcard |

**Pitfalls:**
- This method searches the entire module tree (global-rooted), not just the parent synth's subtree. This is different from `getModulator` which only searches the owner subtree. A pattern like `"LFO*"` will return LFOs from all synths in the project, not just the current one.
- No `onInit` restriction is enforced, but calling this at runtime is unsafe because it allocates wrapper objects. Store the results in `onInit` and reuse them.

**Cross References:**
- `$API.Synth.getModulator$`
- `$API.Synth.addModulator$`
- `$API.Synth.removeModulator$`
- `$API.Synth.getModulatorIndex$`
- `$API.Synth.setModulatorAttribute$`
- `$API.Synth.getAllEffects$`

## getAttribute

**Signature:** `Double getAttribute(Integer attributeIndex)`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** Directly calls `owner->getAttribute(attributeIndex)` which reads a float member -- no allocations, no locks, no iteration.
**Minimal Example:** `var gain = Synth.getAttribute(0);`

**Description:**
Returns the value of a parameter on the parent synth identified by its attribute index. The attribute indices correspond to the parent synth's `Parameters` enum. For a standard `ModulatorSynth`, the indices are:

| Index | Parameter | Range |
|-------|-----------|-------|
| 0 | Gain | 0.0-1.0 |
| 1 | Balance | -100 to 100 |
| 2 | VoiceLimit | Integer |
| 3 | KillFadeTime | Milliseconds |

Subclasses of `ModulatorSynth` (e.g., `ModulatorSampler`) extend this enum with additional parameters at higher indices. The exact indices depend on which synth type the script processor resides in.

This method operates directly on the parent synth (`owner`). To get attributes of other processors, use `Synth.getEffect()`, `Synth.getModulator()`, etc. and call `getAttribute()` on the returned handle.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| attributeIndex | Integer | no | Parameter index from the parent synth's Parameters enum | Processor-type-specific |

**Cross References:**
- `$API.Synth.setAttribute$`

## getAudioSampleProcessor

**Signature:** `ScriptObject getAudioSampleProcessor(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptAudioSampleProcessor wrapper on the heap. Has WARN_IF_AUDIO_THREAD guard. String comparison in the iterator loop involves atomic ref-count operations.
**Minimal Example:** `var asp = Synth.getAudioSampleProcessor("AudioLooper1");`

**Description:**
Returns a `ScriptAudioSampleProcessor` handle to the processor with the given name that holds AudioFile data. Uses owner-rooted search (`Processor::Iterator<ProcessorWithExternalData>(owner)`) -- searches only within the parent synth's subtree.

The method iterates all `ProcessorWithExternalData` instances in the subtree, matches by processor ID, and then checks that the processor has at least one `AudioFile` data object (`getNumDataObjects(ExternalData::DataType::AudioFile) > 0`). If the processor exists but has no AudioFile data, the iteration continues past it without matching. If no matching processor is found at all, a script error is reported.

Unlike most other `get*()` methods, `getAudioSampleProcessor` does NOT check `objectsCanBeCreated()`. It has no explicit `onInit` restriction -- however, it allocates a wrapper object on the heap, so it should only be called in `onInit` in practice. The `WARN_IF_AUDIO_THREAD` guard will flag audio-thread calls in debug builds.

Processors that expose AudioFile data include `AudioLooper`, `ConvolutionReverb`, and scriptnode containers with AudioFile nodes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID to search for | Must match exactly (case-sensitive) |

**Pitfalls:**
- If a processor with the given name exists but has no AudioFile data objects, it is silently skipped. The method continues searching and eventually reports "name was not found" -- the error message does not distinguish between "processor not found" and "processor found but has no AudioFile data". This can be confusing when the processor exists but is the wrong type.
- [BUG] Unlike `getTableProcessor` and `getSliderPackProcessor`, this method does NOT enforce the `onInit` restriction. Calling it at runtime allocates a new wrapper object, which is unsafe on the audio thread.

**Cross References:**
- `$API.Synth.getTableProcessor$`
- `$API.Synth.getSliderPackProcessor$`
- `$API.Synth.getDisplayBufferSource$`

## getChildSynth

**Signature:** `ScriptObject getChildSynth(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var child = Synth.getChildSynth("ChildSynth1");`

**Description:**
Returns a `ChildSynth` handle to the child sound generator with the given name within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<ModulatorSynth>(owner)`) -- only child synths within the parent synth's module tree are found.

The parent synth must be a container type (SynthChain or SynthGroup) that has child sound generators. This method is restricted to `onInit` via `objectsCanBeCreated()`. If the named processor is not found, a script error is reported with a descriptive message. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against the entire tree and provides a fuzzy suggestion if no match is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the child synth to find | Must match exactly (case-sensitive) |

**Cross References:**
- `$API.Synth.getChildSynthByIndex$`
- `$API.Synth.getNumChildSynths$`

**DiagramRef:** synth-module-tree-search

## setUseUniformVoiceHandler

**Disabled:** deprecated
**Disabled Reason:** Fully deprecated. The implementation immediately throws a script error: "This function is deprecated. Just remove that call and enjoy global envelopes..." No work is performed. Global envelopes are now handled automatically by the engine without requiring this call.

## setVoiceGainValue

**Signature:** `undefined setVoiceGainValue(Integer voiceIndex, Double gainValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes a float member on the target voice object via a direct indexed array access -- no allocations, no locks.
**Minimal Example:** `Synth.setVoiceGainValue(0, 0.5);`

**Description:**
Applies a gain factor to a specific voice identified by its voice index. The gain value is stored as the voice's `scriptGainValue` member and is applied during the voice rendering pipeline as an additional gain multiplier alongside the modulation chain output.

The `voiceIndex` is a zero-based index into the parent synth's voice array. The voice index is typically obtained from the `voiceIndex` variable available in Script Voice Start Modulators. Negative indices are clamped to 0 via `jmax(0, voiceIndex)`.

If `voiceIndex` is greater than or equal to the number of allocated voices (`voices.size()`), the method silently does nothing -- no error is reported and no out-of-bounds access occurs.

This method is intended for use inside voice start modulators or similar voice-indexed callback contexts where direct per-voice gain control is needed beyond the standard modulator chain.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| voiceIndex | Integer | no | Zero-based index into the synth's voice array | >= 0 (negative values clamped to 0); must be < voice count |
| gainValue | Double | no | Gain factor to apply to the voice | Linear gain (not dB); 1.0 = unity |

**Pitfalls:**
- No validation or error is produced when `voiceIndex` is out of range -- the method silently does nothing. If you pass an index from a different synth's voice context, the call is a silent no-op.
- Negative voice indices are silently clamped to 0 via `jmax(0, voiceIndex)`. Passing -1 does not produce an error -- it sets the gain on voice 0.

**Cross References:**
- `$API.Synth.setVoicePitchValue$`
- `$API.Synth.addVolumeFade$`

## setVoicePitchValue

**Signature:** `undefined setVoicePitchValue(Integer voiceIndex, Double pitchValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes a float member on the target voice object via a direct indexed array access -- no allocations, no locks.
**Minimal Example:** `Synth.setVoicePitchValue(0, 1.5);`

**Description:**
Applies a pitch factor to a specific voice identified by its voice index. The pitch value is stored as the voice's `scriptPitchValue` member and is applied during the voice rendering pipeline as an additional pitch multiplier alongside the modulation chain output.

The `pitchValue` is a linear pitch ratio where 1.0 = original pitch, 0.5 = one octave down, 2.0 = one octave up. The valid documented range is 0.5 to 2.0, though no clamping is applied at the scripting API level -- the value is passed directly to the voice's `setScriptPitchValue` method. Values outside this range may produce unpredictable pitch behavior depending on the synth's rendering implementation.

The `voiceIndex` is a zero-based index into the parent synth's voice array. Negative indices are clamped to 0 via `jmax(0, voiceIndex)`. If `voiceIndex` is greater than or equal to the number of allocated voices, the method silently does nothing.

This method is intended for use inside voice start modulators or similar voice-indexed callback contexts where direct per-voice pitch control is needed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| voiceIndex | Integer | no | Zero-based index into the synth's voice array | >= 0 (negative values clamped to 0); must be < voice count |
| pitchValue | Double | no | Pitch ratio to apply to the voice | 0.5-2.0 documented range (no clamping applied) |

**Pitfalls:**
- No validation or error is produced when `voiceIndex` is out of range -- the method silently does nothing. Same behavior as `setVoiceGainValue`.
- Negative voice indices are silently clamped to 0 via `jmax(0, voiceIndex)`.
- The `pitchValue` parameter is cast from `double` to `float` internally before storage. Very precise pitch values may lose precision in the conversion.

**Cross References:**
- `$API.Synth.setVoiceGainValue$`
- `$API.Synth.addPitchFade$`

## startTimer

**Signature:** `undefined startTimer(Double seconds)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** In non-deferred mode, writes to atomic timer interval storage and sets the next callback time -- no allocations, no locks. In deferred mode, starts a JUCE Timer which may allocate internally, making the deferred path `unsafe`, but the dominant non-deferred path is safe.
**Minimal Example:** `Synth.startTimer(0.1);`

**Description:**
Starts or restarts the periodic timer for this script processor. The `onTimer` callback will be invoked repeatedly at the specified interval. The timer behavior depends on whether callbacks are deferred:

**Non-deferred mode (default, audio thread):**
- Uses sample-accurate synth timer slots on the parent `ModulatorSynth`
- Timer events are generated as `HiseEvent::TimerEvent` in the audio event buffer
- Events are rastered to `HISE_EVENT_RASTER` for sample alignment
- There are exactly 4 timer slots per synth -- if all 4 are occupied by other script processors, a script error "All 4 timers are used" is reported
- If this processor already has a timer running, it reuses the existing slot (the interval is updated)
- The first timer callback is timed relative to the current event's timestamp

**Deferred mode (message thread, after `deferCallbacks(true)`):**
- Stops any running synth timer slot first
- Starts a standard JUCE `Timer` with millisecond resolution (`(int)(seconds * 1000)`)
- No slot limit, no sample-accurate timing
- The processor's chain index is set to -1 (releasing its synth timer slot)

The minimum interval is 4 milliseconds (0.004 seconds). Passing a smaller value produces a script error "Go easy on the timer!" under `ENABLE_SCRIPTING_SAFE_CHECKS`. The timer can be started from any callback, not just `onInit`.

Requires a MIDI processor context (`parentMidiProcessor` must be non-null). Calling from a modulator or effect script produces "Timers only work in MIDI processors!".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| seconds | Double | no | Timer interval in seconds | >= 0.004 (4ms minimum) |

**Pitfalls:**
- In non-deferred mode, only 4 timer slots are available per synth, shared across all script processors in that synth. If multiple script processors in the same synth each start timers, the 5th one fails with "All 4 timers are used". The slots are released when `stopTimer` is called or the timer interval is set to 0.
- The minimum interval check (< 0.004) is guarded by `ENABLE_SCRIPTING_SAFE_CHECKS`. If this preprocessor flag is disabled, intervals below 4ms are accepted but may cause extreme CPU load due to very frequent timer events.
- In deferred mode, the interval is converted to milliseconds by casting `(int)(seconds * 1000)`, which truncates sub-millisecond precision. An interval of 0.0045 seconds becomes 4ms, not 4.5ms.
- Calling `startTimer` while a timer is already running updates the interval without stopping and restarting -- there is no gap in timer events. In non-deferred mode, the next callback time is recalculated from the current uptime.

**Cross References:**
- `$API.Synth.stopTimer$`
- `$API.Synth.isTimerRunning$`
- `$API.Synth.getTimerInterval$`
- `$API.Synth.deferCallbacks$`

**Example:**
```javascript:start-timer-basic
// Title: Using a timer to periodically update a UI element
Synth.startTimer(0.05);

inline function onTimer()
{
    // Called every 50ms -- update UI, check state, etc.
    Console.print("Timer tick");
    Synth.stopTimer();
}
```
```json:testMetadata:start-timer-basic
{
  "testable": false,
  "skipReason": "Requires MIDI processor context with onTimer callback to verify timer firing"
}
```

## stopTimer

**Signature:** `undefined stopTimer()`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** In non-deferred mode, writes 0.0 to the synth timer interval and next callback time -- no allocations, no locks. In deferred mode, stops a JUCE Timer and clears the synth timer slot.
**Minimal Example:** `Synth.stopTimer();`

**Description:**
Stops the periodic timer for this script processor. Can be called from any callback, including from within the `onTimer` callback itself (which is the standard pattern for one-shot timer behavior).

The behavior depends on whether callbacks are deferred:

**Non-deferred mode (default, audio thread):**
- Calls `owner->stopSynthTimer(parentMidiProcessor->getIndexInChain())` to clear the timer interval and next callback time for this processor's timer slot
- Resets the processor's chain index to -1 via `parentMidiProcessor->setIndexInChain(-1)`, releasing the timer slot for reuse by other script processors

**Deferred mode (message thread):**
- Stops the synth timer slot via `owner->stopSynthTimer(jp->getIndexInChain())`
- Stops the JUCE Timer via `jp->stopTimer()`

After stopping, `isTimerRunning()` returns `false` and `getTimerInterval()` returns `0.0`.

**Parameters:**

(None)

**Pitfalls:**
- [BUG] In non-deferred mode, the `parentMidiProcessor->setIndexInChain(-1)` call on line 5801 is outside the `if(parentMidiProcessor != nullptr)` guard. If `parentMidiProcessor` is null (e.g., called from a non-MIDI processor context), `stopSynthTimer` is correctly skipped but then `setIndexInChain(-1)` dereferences the null pointer, causing undefined behavior. In practice this is unlikely because `startTimer` checks `parentMidiProcessor` before allowing the timer to start, but a direct call to `stopTimer` without a prior `startTimer` from a non-MIDI processor could trigger this.
- Unlike `startTimer`, `stopTimer` has no null-pointer guard that reports an error when called from a non-MIDI processor. `startTimer` reports "Timers only work in MIDI processors!" but `stopTimer` does not.

**Cross References:**
- `$API.Synth.startTimer$`
- `$API.Synth.isTimerRunning$`
- `$API.Synth.getTimerInterval$`
- `$API.Synth.deferCallbacks$`


## noteOffByEventId

**Signature:** `undefined noteOffByEventId(Integer eventId)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Delegates directly to `noteOffDelayedByEventId(eventId, 0)`, which creates a HiseEvent on the stack, pops the matching note-on from EventHandler (a fixed-size array lookup), and inserts into the MIDI buffer -- all lock-free operations.
**Minimal Example:** `Synth.noteOffByEventId(eventId);`

**Description:**
Sends a note-off message for the specified event ID with zero sample delay. This is the primary method for stopping artificial notes generated by `playNote`, `addNoteOn`, or `playNoteWithStartOffset`. It delegates directly to `noteOffDelayedByEventId(eventId, 0)`.

The method pops the matching note-on from the EventHandler's ring buffer, creates a corresponding note-off event with the same channel and note number, and inserts it into the MIDI buffer. The note-off inherits the current event's timestamp (no additional delay).

If the note has already been killed (the event slot is empty in the EventHandler), the method falls through to `setArtificialTimestamp`, which adjusts the timestamp of the previously issued note-off. This allows calling `noteOffByEventId` multiple times on the same event ID without error -- the second call updates the timestamp rather than failing.

Requires a MIDI processor context (`parentMidiProcessor` must be non-null). Calling from a modulator or effect script produces a "Can't call this outside of MIDI script processors" error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID of the note to stop (as returned by playNote/addNoteOn/playNoteWithStartOffset) | > 0; must refer to an artificial event |

**Pitfalls:**
- Under `ENABLE_SCRIPTING_SAFE_CHECKS` (default on), attempting to kill a non-artificial event produces a script error "Hell breaks loose if you kill real events artificially!". This is a safety guard -- only script-generated notes can be stopped via event ID. Real MIDI notes are stopped by the incoming note-off from the controller.
- When `HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS` is enabled (default), the zero timestamp passed internally to `noteOffDelayedByEventId` may be adjusted by subtracting one audio block size on the audio thread, then clamped to 0. This means the note-off always lands at sample position 0 of the current block.

**Cross References:**
- `$API.Synth.noteOffDelayedByEventId$`
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.addVolumeFade$`
- `$API.Synth.isArtificialEventActive$`

**DiagramRef:** synth-midi-event-flow

## isArtificialEventActive

**Signature:** `Integer isArtificialEventActive(Integer eventId)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Calls `EventHandler.isArtificialEventId()` which performs a single array lookup into a pre-allocated fixed-size array using modular index -- no allocations, no locks.
**Minimal Example:** `var active = Synth.isArtificialEventActive(eventId);`

**Description:**
Checks whether an artificial event with the given event ID is currently active (i.e., the note-on has been registered and not yet released). Returns `true` if the event is still active, `false` otherwise.

The method queries the `EventIdHandler`'s `artificialEvents` ring buffer using `eventId % HISE_EVENT_ID_ARRAY_SIZE` as the index. It checks if the event at that slot is non-empty (`!isEmpty()`). This means the method works on the modular event ID space -- event IDs wrap around the ring buffer, so very old event IDs whose slots have been recycled may produce false positives.

Only artificial events (those generated by script via `playNote`, `addNoteOn`, `playNoteWithStartOffset`, etc.) are tracked in this buffer. Real (non-artificial) MIDI events are not tracked and always return `false`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| eventId | Integer | no | Event ID to check (as returned by addNoteOn/playNote) | Cast to uint16 internally |

**Pitfalls:**
- The event ID is cast to `uint16` before lookup. If the original event ID exceeds the uint16 range (65535), truncation occurs and the method may check the wrong slot. In practice, event IDs are always within uint16 range.
- The ring buffer uses modular indexing (`eventId % HISE_EVENT_ID_ARRAY_SIZE`). If the buffer has wrapped and the slot has been reused by a newer event, `isArtificialEventActive` returns `true` for the old event ID even though the original event is long gone. This is unlikely in practice but possible with very long-running sessions generating many artificial events.

**Cross References:**
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.noteOffByEventId$`
- `$API.Synth.noteOffDelayedByEventId$`

## isKeyDown

**Signature:** `Integer isKeyDown(Integer noteNumber)`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads a bit from a BigInteger bitfield -- a single array element access, no allocations, no locks.
**Minimal Example:** `var down = Synth.isKeyDown(60);`

**Description:**
Checks whether the specified MIDI note number is currently held down (pressed). Returns `true` if the key is pressed, `false` otherwise.

The method reads the `keyDown` BigInteger bitfield at the position corresponding to the note number. This bitfield is maintained by `handleNoteCounter`, which is called on every incoming MIDI event before script callbacks fire: note-on events set the bit, note-off events clear it, and All Notes Off clears the entire bitfield.

Only real (non-artificial) MIDI events affect the bitfield -- script-generated notes via `playNote`, `addNoteOn`, etc. are excluded because `handleNoteCounter` ignores artificial events.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| noteNumber | Integer | no | MIDI note number to query | 0-127 |

**Pitfalls:**
- Only real MIDI note events are tracked. If you generate notes via `Synth.playNote()` or `Synth.addNoteOn()`, `isKeyDown` does NOT reflect those artificial notes as pressed.

**Cross References:**
- `$API.Synth.getNumPressedKeys$`
- `$API.Synth.isLegatoInterval$`
- `$API.Synth.isSustainPedalDown$`

## isLegatoInterval

**Signature:** `Integer isLegatoInterval()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads an `Atomic<int>` via `.get()` and compares to 1 -- a single atomic load plus integer comparison, no allocations, no locks.
**Minimal Example:** `var legato = Synth.isLegatoInterval();`

**Description:**
Returns `true` if the current note event represents a legato interval -- that is, when there is already another key held down at the time of the new note-on. Specifically, the implementation is `numPressedKeys.get() != 1`, which returns `true` when zero or two or more keys are pressed, and `false` only when exactly one key is pressed.

In a typical `onNoteOn` callback, `handleNoteCounter` has already incremented the pressed key count before the callback fires. So when a second note arrives while the first is held:

- First note arrives: `numPressedKeys` = 1, `isLegatoInterval()` returns `false` (this is the first/only note)
- Second note arrives while first held: `numPressedKeys` = 2, `isLegatoInterval()` returns `true` (legato transition)

Note that `isLegatoInterval()` also returns `true` when `numPressedKeys` is 0 (no keys pressed), which can occur in `onNoteOff` after the last key is released. The method name is somewhat misleading in this edge case -- it indicates "not a single-key press" rather than strictly "a legato transition".

Only real (non-artificial) MIDI events affect the pressed key count.

**Parameters:**

(None)

**Pitfalls:**
- Returns `true` when zero keys are pressed (after the last note-off), not just during legato transitions. The check is `numPressedKeys != 1`, not `numPressedKeys > 1`. In an `onNoteOff` callback, `isLegatoInterval()` returns `true` after the last key is released because the count is 0. This can lead to incorrect legato detection if used in `onNoteOff` without checking the count separately.
- Only real MIDI events affect the count. Script-generated artificial notes do not change `numPressedKeys`.

**Cross References:**
- `$API.Synth.getNumPressedKeys$`
- `$API.Synth.isKeyDown$`

## isSustainPedalDown

**Signature:** `Integer isSustainPedalDown()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads a plain boolean member variable -- no allocations, no locks, no atomic operations.
**Minimal Example:** `var sustain = Synth.isSustainPedalDown();`

**Description:**
Returns `true` if the sustain pedal (MIDI CC #64) is currently pressed, `false` otherwise.

The method reads the `sustainState` boolean member, which is set externally via `setSustainPedal(bool)`. The sustain state is updated by the host script processor's MIDI processing code when it detects a CC #64 event crossing the pedal-down/pedal-up threshold. This happens before the `onController` callback fires, so the state is already updated when your script reads it.

The sustain state reflects only the real MIDI pedal status. It is not affected by script-generated controller events (`Synth.sendController(64, ...)` or `Synth.addController(..., 64, ...)`).

**Parameters:**

(None)

**Cross References:**
- `$API.Synth.isKeyDown$`
- `$API.Synth.getNumPressedKeys$`
- `$API.Synth.isLegatoInterval$`
- `$API.Synth.sendController$`

## isTimerRunning

**Signature:** `Integer isTimerRunning()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** In non-deferred mode, reads a stored double from the owner synth's timer interval array and compares to 0. In deferred mode, calls JUCE Timer's isTimerRunning() which reads a stored atomic. No allocations, no locks.
**Minimal Example:** `var running = Synth.isTimerRunning();`

**Description:**
Returns `true` if the timer for this script processor is currently running, `false` otherwise. The behavior differs based on the deferred callbacks mode:

- **Non-deferred (default):** Checks `owner->getTimerInterval(parentMidiProcessor->getIndexInChain()) != 0.0`. A non-zero interval means the synth timer for this processor's slot is active. Returns `false` if `parentMidiProcessor` is null (e.g., called from a non-MIDI processor context).
- **Deferred mode:** Delegates to the JUCE `Timer::isTimerRunning()` on the JavascriptMidiProcessor.

**Parameters:**

(None)

**Cross References:**
- `$API.Synth.getTimerInterval$`
- `$API.Synth.startTimer$`
- `$API.Synth.stopTimer$`

## noteOff

**Disabled:** deprecated
**Disabled Reason:** Deprecated in favor of `Synth.noteOffByEventId`. The implementation calls `addNoteOff(1, noteNumber, 0)` but then immediately reports a script error "noteOff is deprecated. Use noteOfByEventId instead" (note: the error message has a typo -- "noteOfByEventId" instead of "noteOffByEventId"). Note-off by note number is unreliable when multiple voices play the same pitch; event-ID-based note-off is unambiguous.


## getTimerInterval

**Signature:** `Double getTimerInterval()`
**Return Type:** `Double`
**Call Scope:** safe
**Call Scope Note:** In non-deferred mode, reads a stored double from the owner synth's timer interval array via a direct index lookup. In deferred mode, reads a JUCE Timer interval (stored int) and divides by 1000. No allocations, no locks.
**Minimal Example:** `var interval = Synth.getTimerInterval();`

**Description:**
Returns the current timer interval in seconds. The behavior differs based on the deferred callbacks mode:

- **Non-deferred (default):** Queries `owner->getTimerInterval(parentMidiProcessor->getIndexInChain())`, which returns the interval stored in the owner synth's `synthTimerIntervals` array for this script processor's timer slot. Returns `0.0` if `parentMidiProcessor` is null (e.g., called from a non-MIDI processor context).
- **Deferred mode:** Queries the JUCE Timer's interval in milliseconds via `jp->getTimerInterval()` and converts to seconds by dividing by `1000.0`.

If no timer has been started, the return value is `0.0`.

**Parameters:**

(None)

**Cross References:**
- `$API.Synth.isTimerRunning$`
- `$API.Synth.startTimer$`
- `$API.Synth.stopTimer$`

## getWavetableController

**Signature:** `ScriptObject getWavetableController(String processorId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptWavetableController wrapper on the heap. No objectsCanBeCreated() guard -- not restricted to onInit, but should only be called in onInit due to heap allocation.
**Minimal Example:** `var wt = Synth.getWavetableController("WavetableSynth1");`

**Description:**
Returns a `ScriptWavetableController` handle that provides script-level access to wavetable synthesiser features (wavetable position, gain table, etc.) for the processor with the given name.

Unlike most `get*()` methods, this uses a **global-rooted search** (`ProcessorHelpers::getFirstProcessorWithName` from `getMainSynthChain()`) -- it searches the entire module tree, not just the parent synth's subtree. This also means it does NOT check `objectsCanBeCreated()` and has no explicit `onInit` restriction.

The method first searches for any processor with the given name, then checks if it is a `WavetableSynth` via `dynamic_cast`. If the processor is not found or is not a `WavetableSynth`, a script error is reported. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `WavetableSynth` types.

Despite having no `onInit` restriction, this method allocates a wrapper object on the heap and should only be called in `onInit`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processorId | String | no | Processor ID of the WavetableSynth to control | Must match exactly (case-sensitive) |

**Pitfalls:**
- [BUG] The error message when the processor is not a WavetableSynth says "[id] does not have a routing matrix" instead of a message about wavetable capabilities. This is a copy-paste error from `getRoutingMatrix`.
- This method searches the entire module tree (global-rooted), not just the parent synth's subtree. A WavetableSynth in a sibling synth or different hierarchy level will be found.
- Unlike `getMidiPlayer` which has a two-step error reporting (distinguishes "not found" from "wrong type"), `getWavetableController` has no separate null check on the initial lookup. If the processor name does not exist at all, `dynamic_cast<WavetableSynth*>(nullptr)` returns null and the same "routing matrix" error is shown, making it impossible to distinguish between a missing processor and a type mismatch.

**Cross References:**
- `$API.Synth.getEffect$`
- `$API.Synth.getRoutingMatrix$`
- `$API.Synth.getChildSynth$`

**DiagramRef:** synth-module-tree-search


## getSampler

**Signature:** `ScriptObject getSampler(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var sampler = Synth.getSampler("Sampler1");`

**Description:**
Returns a `Sampler` handle to the `ModulatorSampler` with the given name within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<ModulatorSampler>(owner)`) -- only samplers within the parent synth's module tree are found.

The method iterates all `ModulatorSampler` instances in the subtree and matches by processor ID. If no match is found, a script error is reported: "[name] was not found." Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `ModulatorSampler` types.

The returned `Sampler` handle provides methods for sample map loading, sample editing, round-robin configuration, and AHDSR envelope access.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the sampler to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- [BUG] The `reportIllegalCall` error message when called outside `onInit` says `"getScriptingAudioSampleProcessor()"` instead of `"getSampler()"` -- a copy-paste error from another method.

**Cross References:**
- `$API.Synth.getEffect$`
- `$API.Synth.getModulator$`
- `$API.Synth.getChildSynth$`

**DiagramRef:** synth-module-tree-search

## getSliderPackProcessor

**Signature:** `ScriptObject getSliderPackProcessor(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var sp = Synth.getSliderPackProcessor("SliderPackProc1");`

**Description:**
Returns a `ScriptSliderPackProcessor` handle to the processor with the given name that holds slider pack data. Uses owner-rooted search (`Processor::Iterator<ExternalDataHolder>(owner)`) -- only processors within the parent synth's subtree are found.

The method iterates all `ExternalDataHolder` instances in the subtree and matches by processor ID. Unlike `getTableProcessor`, this method does NOT check the data type -- it returns a handle for any `ExternalDataHolder` that matches the name, regardless of whether the processor actually has slider pack data. This means it may return a handle to a processor that only has Table or AudioFile data.

If no processor with the given name is found, a script error is reported: "[name] was not found." Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `ProcessorWithExternalData` types.

Processors that expose slider pack data include LFO modulators (with table/slider pack mode), Step modulators, and scriptnode containers with slider pack nodes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the slider pack processor to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- The method matches any `ExternalDataHolder` by name without verifying that the processor actually has slider pack data. If you pass the name of a processor that only has Table data (e.g., a Table envelope), you get a `ScriptSliderPackProcessor` handle that cannot access any slider pack -- methods on it may fail or return invalid data. Compare with `getAudioSampleProcessor`, which explicitly checks `getNumDataObjects(ExternalData::DataType::AudioFile) > 0`.

**Cross References:**
- `$API.Synth.getTableProcessor$`
- `$API.Synth.getAudioSampleProcessor$`
- `$API.Synth.getDisplayBufferSource$`

## getSlotFX

**Signature:** `ScriptObject getSlotFX(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var slot = Synth.getSlotFX("SlotFX1");`

**Description:**
Returns a `ScriptSlotFX` handle to the slot effect processor with the given name within the parent synth's subtree. Uses owner-rooted search with a **dual search strategy**: first searches for `HotswappableProcessor` (the traditional SlotFX module), then falls back to `DspNetwork::Holder` (scriptnode-based slot container).

This dual search means the method finds both traditional SlotFX modules and scriptnode DspNetwork holders by name. If a processor is found in either search, a `ScriptSlotFX` handle is returned. If neither search finds a match, a script error is reported: "[name] was not found."

Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `HotswappableProcessor` types (note: the diagnostic only checks the first type, not `DspNetwork::Holder`).

The returned `ScriptSlotFX` handle provides methods for swapping effect algorithms at runtime, which is useful for A/B effect comparison or preset-switchable effect chains.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the slot effect to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- [BUG] The compile-time diagnostic (ModuleDiagnoser) only searches `HotswappableProcessor` types. If you have a `DspNetwork::Holder` that the runtime method would find via the fallback search, the diagnostic may report a false "not found" warning in the HISE IDE while the code actually works at runtime.

**Cross References:**
- `$API.Synth.getEffect$`
- `$API.Synth.addEffect$`
- `$API.ScriptSlotFX$`

**DiagramRef:** synth-module-tree-search

## getTableProcessor

**Signature:** `ScriptObject getTableProcessor(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var tp = Synth.getTableProcessor("TableEnvelope1");`

**Description:**
Returns a `ScriptTableProcessor` handle to the processor with the given name that holds table data. Uses owner-rooted search (`Processor::Iterator<ExternalDataHolder>(owner)`) -- only processors within the parent synth's subtree are found.

The method iterates all `ExternalDataHolder` instances in the subtree and matches by processor ID. Like `getSliderPackProcessor`, it does NOT verify that the matched processor actually has table data -- it returns a handle for any `ExternalDataHolder` that matches the name.

If no processor with the given name is found, a script error is reported: "[name] was not found." Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `ProcessorWithExternalData` types.

Processors that expose table data include Table envelopes, Velocity modulators (in table mode), and scriptnode containers with table nodes. The returned handle provides methods to set and get table points, change the table content, and connect to table display components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the table processor to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- The method matches any `ExternalDataHolder` by name without verifying that the processor actually has table data. If you pass the name of a processor that only has slider pack or audio file data, you get a `ScriptTableProcessor` handle that cannot access any tables. Compare with `getAudioSampleProcessor`, which explicitly checks `getNumDataObjects(ExternalData::DataType::AudioFile) > 0`.
- [BUG] The `reportIllegalCall` error message when called outside `onInit` says `"getScriptingTableProcessor()"` instead of `"getTableProcessor()"`. The internal C++ method name leaks into the user-facing error message.

**Cross References:**
- `$API.Synth.getSliderPackProcessor$`
- `$API.Synth.getAudioSampleProcessor$`
- `$API.Synth.getDisplayBufferSource$`


## getIdList

**Signature:** `Array getIdList(String type)`
**Return Type:** `Array`
**Call Scope:** init
**Minimal Example:** `var ids = Synth.getIdList("LFO Modulator");`

**Description:**
Returns an array of processor IDs for all processors of the given type within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<Processor>(owner)`) -- only processors within the parent synth's module tree are found.

The `type` parameter matches against each processor's type name (`Processor::getName()`), which is the human-readable processor class name (e.g., "LFO Modulator", "Simple Gain", "Sine Wave Generator"), NOT the user-assigned processor ID. The return value is an array of user-assigned IDs of all matching processors. The script processor itself is excluded from the results to prevent self-references.

If no processors match the given type name, an empty array is returned. Restricted to `onInit` via `objectsCanBeCreated()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| type | String | no | Processor type name (e.g., "LFO Modulator", "Simple Gain") | Must match the processor's getName() string exactly |

**Pitfalls:**
- The `type` parameter is the processor's type name (e.g., "LFO Modulator"), not the user-assigned ID (e.g., "LFO1"). Passing a processor ID instead of a type name returns an empty array with no error, making the mistake hard to diagnose.
- [BUG] When called outside `onInit`, the method silently returns `undefined` instead of reporting an error via `reportIllegalCall`. Other `get*()` methods report a clear "can only be called in onInit" message. This inconsistency means callers get no feedback when misusing this method at runtime.

**Cross References:**
- `$API.Synth.getModulator$`
- `$API.Synth.getEffect$`
- `$API.Synth.getChildSynth$`
- `$API.Synth.getMidiProcessor$`
- `$API.Synth.getAllEffects$`
- `$API.Synth.getAllModulators$`

**Example:**
```javascript:get-id-list-usage
// Title: List all LFO modulators in the parent synth's subtree
const var lfoIds = Synth.getIdList("LFO Modulator");

for (id in lfoIds)
    Console.print("Found LFO: " + id);
```
```json:testMetadata:get-id-list-usage
{
  "testable": false,
  "skipReason": "Requires a module tree with specific processor types present"
}
```

## getMidiPlayer

**Signature:** `ScriptObject getMidiPlayer(String playerId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptedMidiPlayer wrapper on the heap. No objectsCanBeCreated() guard -- not restricted to onInit, but should only be called in onInit due to heap allocation.
**Minimal Example:** `var player = Synth.getMidiPlayer("MidiPlayer1");`

**Description:**
Returns a `MidiPlayer` handle to the MIDI player processor with the given name. Unlike most `get*()` methods, this uses a **global-rooted search** (`ProcessorHelpers::getFirstProcessorWithName` from `getMainSynthChain()`) -- it searches the entire module tree, not just the parent synth's subtree.

The method performs a two-step validation: first it checks if any processor with the given name exists (reports "was not found" if not), then it checks if the found processor is actually a `MidiPlayer` (reports "is not a MIDI Player" if not). This two-step error reporting is more informative than methods that just report "not found" for type mismatches.

This method does NOT check `objectsCanBeCreated()` and does NOT have a `WARN_IF_AUDIO_THREAD` guard. It can technically be called outside `onInit`, but this is not recommended since it allocates a wrapper object on every call. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| playerId | String | no | Processor ID of the MIDI player to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- This method searches the entire module tree (global-rooted), not just the parent synth's subtree. A MIDI player in a sibling synth or a different hierarchy level will be found. This is different from `getEffect`, `getModulator`, etc. which only search the owner subtree.

**Cross References:**
- `$API.Synth.getMidiProcessor$`
- `$API.Synth.getEffect$`

**DiagramRef:** synth-module-tree-search

## getMidiProcessor

**Signature:** `ScriptObject getMidiProcessor(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var mp = Synth.getMidiProcessor("Arpeggiator1");`

**Description:**
Returns a `ScriptMidiProcessor` handle to the MIDI processor with the given name within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<MidiProcessor>(owner)`) -- only MIDI processors within the parent synth's module tree are found.

The method has a **self-exclusion check**: you cannot get a reference to the script processor that owns this Synth object. Passing your own processor's ID produces a script error "You can't get a reference to yourself!". This check runs before the `onInit` restriction, so the self-reference error appears even outside `onInit`.

The self-exclusion exists because a script processor getting a handle to itself would create a circular reference -- the handle wraps the same processor that is executing the script.

Restricted to `onInit` via `objectsCanBeCreated()`. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name and provides a fuzzy suggestion if no match is found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the MIDI processor to find | Must match exactly; cannot be the current script processor's own ID |

**Pitfalls:**
- The self-exclusion check compares the `name` parameter against `getProcessor()->getId()` before checking `objectsCanBeCreated()`. If you pass the name of the current script processor, you always get the self-reference error, even when called outside `onInit`. This ordering is harmless but means the error message changes depending on whether you pass your own name vs. another valid name when calling from the wrong scope.

**Cross References:**
- `$API.Synth.getModulator$`
- `$API.Synth.getEffect$`
- `$API.Synth.getMidiPlayer$`

**DiagramRef:** synth-module-tree-search

## getChildSynthByIndex

**Signature:** `ScriptObject getChildSynthByIndex(Integer index)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var child = Synth.getChildSynthByIndex(0);`

**Description:**
Returns a `ChildSynth` handle to the child sound generator at the given index within the parent synth's child processor list. Unlike `getChildSynth` which searches by name, this method uses a numeric index for direct positional access.

The parent synth must be a container type (SynthChain or SynthGroup) that implements the `Chain` interface. If the parent synth is not a Chain, the method silently returns an invalid (null-wrapped) handle with no error. If the index is out of bounds (negative or >= the number of child processors), the method also silently returns an invalid handle. Use `Synth.getNumChildSynths()` to determine the valid index range.

Restricted to `onInit` via `objectsCanBeCreated()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based position of the child synth in the container's child list | 0 to getNumChildSynths()-1 |

**Pitfalls:**
- [BUG] When the parent synth is not a Chain type or the index is out of bounds, the method silently returns an invalid handle instead of reporting a script error. The returned object exists but wraps a null pointer -- calling methods on it will produce confusing errors. Other `get*()` methods report "name was not found" in similar cases.
- [BUG] The `reportIllegalCall` error message when called outside `onInit` says `"getChildSynth()"` instead of `"getChildSynthByIndex()"` -- a copy-paste error from the name-based variant.

**Cross References:**
- `$API.Synth.getChildSynth$`
- `$API.Synth.getNumChildSynths$`

## getDisplayBufferSource

**Signature:** `ScriptObject getDisplayBufferSource(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var dbs = Synth.getDisplayBufferSource("SineGenerator1");`

**Description:**
Returns a `DisplayBufferSource` handle to the processor with the given name that holds DisplayBuffer data. Uses owner-rooted search (`Processor::Iterator<ProcessorWithExternalData>(owner)`) -- only processors within the parent synth's subtree are found.

The method searches for a `ProcessorWithExternalData` instance matching the given name, then verifies that it has at least one DisplayBuffer data object (`getNumDataObjects(ExternalData::DataType::DisplayBuffer) > 0`). If the processor exists but has no DisplayBuffer data, a specific error is reported: "No display buffer available". If the processor is not found at all, the standard "name was not found" error is reported.

Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name.

Processors that expose DisplayBuffer data include oscillators, analysers, and scriptnode containers with display buffer nodes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the display buffer source | Must match exactly (case-sensitive) |

**Pitfalls:**
- [BUG] The `reportIllegalCall` error message when called outside `onInit` says `"getScriptingTableProcessor()"` instead of `"getDisplayBufferSource()"` -- a copy-paste error from the table processor method.

**Cross References:**
- `$API.Synth.getAudioSampleProcessor$`
- `$API.Synth.getTableProcessor$`
- `$API.Synth.getSliderPackProcessor$`

## getEffect

**Signature:** `ScriptObject getEffect(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var fx = Synth.getEffect("Delay1");`

**Description:**
Returns a `ScriptEffect` handle to the effect processor with the given name within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<EffectProcessor>(owner)`) -- only effects within the parent synth's module tree are found, including effects in child synths.

The method searches all `EffectProcessor` instances in the subtree and matches by processor ID. If no match is found, a script error is reported. Restricted to `onInit` via `objectsCanBeCreated()`. Has `WARN_IF_AUDIO_THREAD` guard for debug builds. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name and provides a fuzzy suggestion if no match is found.

The returned `ScriptEffect` handle provides methods for bypassing, setting attributes, and querying the effect's state. Store the reference in a `const var` at the top level of your script for reuse across callbacks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the effect to find | Must match exactly (case-sensitive) |

**Cross References:**
- `$API.Synth.getAllEffects$`
- `$API.Synth.addEffect$`
- `$API.Synth.removeEffect$`

**DiagramRef:** synth-module-tree-search

## getModulator

**Signature:** `ScriptObject getModulator(String name)`
**Return Type:** `ScriptObject`
**Call Scope:** init
**Minimal Example:** `var mod = Synth.getModulator("LFO1");`

**Description:**
Returns a `ScriptModulator` handle to the modulator with the given name within the parent synth's subtree. Uses owner-rooted search (`Processor::Iterator<Modulator>(owner)`) -- only modulators within the parent synth's module tree are found, including modulators in child synths' chains.

The method searches all `Modulator` instances in the subtree and matches by processor ID. This includes all modulator types: LFOs, envelopes, voice start modulators, time variant modulators, and any other `Modulator` subclass. If no match is found, a script error is reported with the descriptive message "name was not found".

Restricted to `onInit` via `objectsCanBeCreated()`. In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against the entire tree and provides a fuzzy suggestion if no match is found.

The returned `ScriptModulator` handle provides methods for setting attributes, bypassing, adjusting intensity, and connecting to global modulators. Store the reference in a `const var` at the top level of your script for reuse across callbacks.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| name | String | no | Processor ID of the modulator to find | Must match exactly (case-sensitive) |

**Cross References:**
- `$API.Synth.getAllModulators$`
- `$API.Synth.addModulator$`
- `$API.Synth.removeModulator$`
- `$API.Synth.getModulatorIndex$`
- `$API.Synth.setModulatorAttribute$`

**DiagramRef:** synth-module-tree-search

## getModulatorIndex

**Signature:** `Integer getModulatorIndex(Integer chainId, String id)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** String comparison in the iteration loop involves atomic ref-count operations on the String holder. The method iterates over all modulators in the chain, making it O(n) in the chain length.
**Minimal Example:** `var idx = Synth.getModulatorIndex(1, "LFO1");`

**Description:**
Returns the zero-based index of a modulator within the specified modulation chain of the parent synth. The `chainId` parameter uses the C++ `ModulatorSynth::InternalChains` enum values: `1` = GainModulation, `2` = PitchModulation. Any other value produces a script error.

The method iterates the chain's processor handler to find a modulator whose ID matches the `id` parameter. If found, returns its index (0-based position in the chain). If not found, a script error is reported with the message "Modulator [id] was not found in [chainName]".

This method does NOT check `objectsCanBeCreated()` -- it has no `onInit` restriction. It accesses the chain directly through `owner->gainChain` or `owner->pitchChain` without allocating wrapper objects, making it callable at runtime. However, the string comparison involves atomic ref-count operations.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainId | Integer | no | Target modulation chain | 1 = GainModulation, 2 = PitchModulation |
| id | String | no | Processor ID of the modulator to find | Must match exactly (case-sensitive) |

**Pitfalls:**
- The `chainId` values (1=Gain, 2=Pitch) do not start at 0. Passing 0 triggers a `jassertfalse` and a script error "No valid chainType". This is the same pitfall as `addModulator` and `setModulatorAttribute`.
- The returned index is positional within the chain's handler, which may differ from the visual order in the HISE IDE if modulators have been dynamically added or removed.

**Cross References:**
- `$API.Synth.addModulator$`
- `$API.Synth.setModulatorAttribute$`
- `$API.Synth.getModulator$`

## getNumChildSynths

**Signature:** `Integer getNumChildSynths()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Calls `dynamic_cast<Chain*>(owner)->getHandler()->getNumProcessors()` which returns a stored count -- no allocations, no locks, no iteration.
**Minimal Example:** `var count = Synth.getNumChildSynths();`

**Description:**
Returns the number of child sound generators in the parent synth's child processor list. The parent synth must be a container type that implements the `Chain` interface -- specifically a `SynthChain` or `SynthGroup`. If the parent synth is not a Chain (e.g., a standalone `ModulatorSynth` with no children), a script error is reported: "getNumChildSynths() can only be called on Chains!".

This method has no `onInit` restriction and no `WARN_IF_AUDIO_THREAD` guard. It simply reads the processor count from the chain handler, making it safe to call from any context. Use the return value to determine valid index bounds for `getChildSynthByIndex`.

**Parameters:**

(None)

**Pitfalls:**
- Calling this on a non-Chain synth (e.g., a script processor inside a standalone ModulatorSynth that is not a SynthChain or SynthGroup) produces a script error at runtime. There is no way to query whether the parent synth is a Chain type before calling this method.

**Cross References:**
- `$API.Synth.getChildSynth$`
- `$API.Synth.getChildSynthByIndex$`

## getNumPressedKeys

**Signature:** `Integer getNumPressedKeys()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** Reads an `Atomic<int>` via `.get()` -- a single atomic load, no allocations, no locks.
**Minimal Example:** `var keys = Synth.getNumPressedKeys();`

**Description:**
Returns the number of currently pressed MIDI keys (physical note-on events that have not yet received a note-off). This counts real (non-artificial) key presses only -- script-generated notes via `playNote`, `addNoteOn`, etc. are excluded because `handleNoteCounter` ignores artificial events.

The count is maintained via the `handleNoteCounter` method, which is called on every incoming MIDI event before the script callbacks fire. Note-on events increment the counter and set the corresponding bit in the `keyDown` bitfield; note-off events decrement the counter (clamped to 0) and clear the bit. An All Notes Off message resets both to zero.

This is distinct from the number of playing voices -- a single key press can trigger multiple voices (round-robin, unison), and voices may still be playing after key release (sustain pedal, envelope tail). Use this method to detect legato playing, key release states, or to count held keys.

**Parameters:**

(None)

**Cross References:**
- `$API.Synth.isLegatoInterval$`
- `$API.Synth.isKeyDown$`
- `$API.Synth.isSustainPedalDown$`

## sendController

**Signature:** `undefined sendController(Integer number, Integer value)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Creates a HiseEvent on the stack and inserts it into the MIDI buffer via addHiseEventToBuffer -- no allocations, no locks.
**Minimal Example:** `Synth.sendController(1, 64);`

**Description:**
Sends a controller event into the MIDI buffer with the timestamp of the current event. The `number` parameter supports three event types through special constants: standard CC (0-127), pitch bend (128), and aftertouch (129). For pitch bend, the value range extends to 0-16383; for standard CC and aftertouch, the range is 0-127.

Unlike `addController`, this method does NOT set the artificial flag on the generated event and does NOT allow specifying a MIDI channel or custom timestamp. The event inherits the timestamp from the current MIDI event being processed (if any). The channel is not explicitly set, so the event uses the default channel from the HiseEvent constructor (channel 0 in the raw struct, which translates to channel 1 in MIDI terms).

Requires a MIDI processor context (`parentMidiProcessor` must be non-null). Calling from a modulator or effect script produces "Only valid in MidiProcessors".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| number | Integer | no | CC number, or 128 for pitch bend, 129 for aftertouch | >= 0 |
| value | Integer | no | Controller value | 0-127 for CC/aftertouch, 0-16383 for pitch bend |

**Pitfalls:**
- The generated event is NOT marked as artificial, unlike `addController` which does set the artificial flag. If downstream logic filters on the artificial flag, `sendController` and `addController` produce different results for identical CC numbers and values.
- There is no explicit channel parameter. The event uses the default HiseEvent channel, which may not match the channel of the incoming MIDI event. Use `addController` when you need to specify the channel.

**Cross References:**
- `$API.Synth.addController$`
- `$API.Synth.sendControllerToChildSynths$`

## sendControllerToChildSynths

**Signature:** `undefined sendControllerToChildSynths(Integer controllerNumber, Integer controllerValue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Delegates directly to sendController, which creates a HiseEvent on the stack and inserts it into the MIDI buffer -- no allocations, no locks.
**Minimal Example:** `Synth.sendControllerToChildSynths(1, 64);`

**Description:**
Identical to `sendController` -- exists only for backwards compatibility. The implementation is a direct forwarding call: `sendController(controllerNumber, controllerValue)`. Despite the name suggesting it sends to child synths specifically, it behaves exactly the same as `sendController`.

All validation, event creation, and buffer insertion behavior is inherited from `sendController`. See that method's documentation for details on CC number ranges, pitch bend handling, and the lack of artificial flag.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| controllerNumber | Integer | no | CC number, or 128 for pitch bend, 129 for aftertouch | >= 0 |
| controllerValue | Integer | no | Controller value | 0-127 for CC/aftertouch, 0-16383 for pitch bend |

**Cross References:**
- `$API.Synth.sendController$`
- `$API.Synth.addController$`

## setAttribute

**Signature:** `undefined setAttribute(Integer attributeIndex, Double newAttribute)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `Processor::setAttribute()` with `sendNotification`, which dispatches a change message. The dispatch mechanism involves the processor's dispatcher and potentially notification-chain work including string lookups and var comparisons.
**Minimal Example:** `Synth.setAttribute(0, 0.5);`

**Description:**
Sets a parameter on the parent synth identified by its attribute index. The attribute indices correspond to the parent synth's `Parameters` enum. For a standard `ModulatorSynth`, the indices are:

| Index | Parameter | Range |
|-------|-----------|-------|
| 0 | Gain | 0.0-1.0 |
| 1 | Balance | -100 to 100 |
| 2 | VoiceLimit | Integer |
| 3 | KillFadeTime | Milliseconds |

Subclasses of `ModulatorSynth` (e.g., `ModulatorSampler`) extend this enum with additional parameters at higher indices. The exact indices depend on which synth type the script processor resides in.

The method calls `owner->setAttribute(attributeIndex, newAttribute, sendNotification)`, which writes the value via `setInternalAttribute` and then dispatches a change notification to update the UI and any connected listeners.

This method operates directly on the parent synth (`owner`). To set attributes of other processors, use `Synth.getEffect()`, `Synth.getModulator()`, etc. and call `setAttribute()` on the returned handle.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| attributeIndex | Integer | no | Parameter index from the parent synth's Parameters enum | Processor-type-specific |
| newAttribute | Double | no | New parameter value | Range depends on the specific parameter |

**Pitfalls:**
- No validation is performed on `attributeIndex`. Passing an index outside the valid range for the parent synth's parameter enum passes through to `setInternalAttribute`, which may silently write to an invalid parameter slot or be ignored depending on the synth subclass implementation.

**Cross References:**
- `$API.Synth.getAttribute$`

## setClockSpeed

**Signature:** `undefined setClockSpeed(Integer clockSpeed)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes a single enum value to the owner synth's clockSpeed member -- no allocations, no locks, no dispatch.
**Minimal Example:** `Synth.setClockSpeed(4);`

**Description:**
Sets the internal clock speed for the parent synth's MIDI clock generation. The clock speed determines the musical subdivision at which the synth generates internal timing events. This affects transport-synchronized features like arpeggiators and sequencers connected to the synth's clock.

Valid values are musical division denominators:

| Value | Division | Internal Enum |
|-------|----------|---------------|
| 0 | Inactive (disable clock) | `ModulatorSynth::Inactive` |
| 1 | Bar (whole note) | `ModulatorSynth::Bar` |
| 2 | Half note | `ModulatorSynth::Half` |
| 4 | Quarter note | `ModulatorSynth::Quarters` |
| 8 | Eighth note | `ModulatorSynth::Eighths` |
| 16 | Sixteenth note | `ModulatorSynth::Sixteens` |
| 32 | Thirty-second note | `ModulatorSynth::ThirtyTwos` |

Any other value produces a script error "Unknown clockspeed. Use 1,2,4,8,16 or 32".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| clockSpeed | Integer | no | Musical division value for internal clock | 0, 1, 2, 4, 8, 16, or 32 |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| 0 | Disables the internal clock entirely |
| 1 | Clock ticks once per bar (whole note) |
| 2 | Clock ticks on half notes |
| 4 | Clock ticks on quarter notes (most common musical beat) |
| 8 | Clock ticks on eighth notes |
| 16 | Clock ticks on sixteenth notes |
| 32 | Clock ticks on thirty-second notes |

**Pitfalls:**
- [BUG] The error message says "Use 1,2,4,8,16 or 32" but omits 0 (Inactive), which is a valid value that disables the clock. Passing 0 works correctly but is not mentioned in the error guidance.

**Cross References:**
- `$API.Synth.startTimer$`
- `$API.Synth.stopTimer$`

## setFixNoteOnAfterNoteOff

**Signature:** `undefined setFixNoteOnAfterNoteOff(Integer shouldBeFixed)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a boolean flag on the MidiProcessorChain via a direct member write -- no allocations, no locks.
**Minimal Example:** `Synth.setFixNoteOnAfterNoteOff(true);`

**Description:**
Enables or disables the attached note buffer in the parent synth's MIDI processor chain. When enabled (`true`), the system provides additional safety checks to prevent stuck notes that can occur when a note-off is processed before its corresponding note-on (a timing edge case with artificial notes).

This must be called before using `Synth.attachNote()`. If `attachNote()` is called without first enabling this feature, a runtime script error is thrown. Typically called once in `onInit`.

When `parentMidiProcessor` is null (i.e., called from a non-MIDI script processor), the method silently does nothing.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeFixed | Integer | no | Whether to enable the attached note buffer | Boolean (0 or 1) |

**Pitfalls:**
- If `parentMidiProcessor` is null (e.g., called from a modulator or effect script), the method silently does nothing without any error. Other MIDI-requiring methods report "Only valid in MidiProcessors", but this one has no such guard.

**Cross References:**
- `$API.Synth.attachNote$`
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`

## setMacroControl

**Signature:** `undefined setMacroControl(Integer macroIndex, Double newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `ModulatorSynthChain::setMacroControl()` with `sendNotification`, which dispatches change notifications to all connected macro targets. This involves iterating connected parameters and calling setAttribute on each.
**Minimal Example:** `Synth.setMacroControl(1, 64.0);`

**Description:**
Sets one of the eight macro controllers to a new value. Macro controllers are a global parameter mapping system that allows a single value to drive multiple processor parameters with configurable ranges.

The `macroIndex` is 1-based (1 to 8). The value is internally adjusted to 0-based before passing to the `ModulatorSynthChain::setMacroControl` method. The `newValue` range is 0.0 to 127.0, consistent with MIDI CC value scaling.

This method only works when the parent synth is a `ModulatorSynthChain`. If the parent synth is a different type (e.g., a standalone `ModulatorSynth`, `SynthGroup`, or any non-chain container), a script error is reported: "setMacroControl() can only be called on ModulatorSynthChains".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| macroIndex | Integer | no | Macro controller index (1-based) | 1-8 |
| newValue | Double | no | New macro value | 0.0-127.0 |

**Pitfalls:**
- The `macroIndex` is 1-based, not 0-based. Passing 0 triggers the error "macroIndex must be between 1 and 8!". This is a common mistake since most HISE indices are 0-based.
- Only works when the parent synth is a `ModulatorSynthChain`. Calling from a script processor inside a non-chain synth produces an error, even though the script has a valid Synth object. There is no way to query the parent synth type before calling.

**Cross References:**
- `$API.Synth.getAttribute$`
- `$API.Synth.setAttribute$`

## setModulatorAttribute

**Signature:** `undefined setModulatorAttribute(Integer chainId, Integer modulatorIndex, Integer attributeIndex, Double newValue)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `setAttribute` or `setIntensity` on the target modulator with `dontSendNotification`, then calls `sendOtherChangeMessage` to dispatch an attribute change event. The dispatch involves the processor's notification system.
**Minimal Example:** `Synth.setModulatorAttribute(1, 0, 0, 0.5);`

**Description:**
Sets an attribute on a modulator within the parent synth's gain or pitch modulation chain, identified by chain type and positional index. This is an alternative to obtaining a `ScriptModulator` handle and calling `setAttribute` on it -- useful for quick access when you know the chain and index.

The `chainId` parameter uses the C++ `ModulatorSynth::InternalChains` enum values: `1` = GainModulation, `2` = PitchModulation. The `modulatorIndex` is the zero-based position of the modulator in the chain (as returned by `getModulatorIndex`).

Two special `attributeIndex` values provide direct access to common modulator properties:

| attributeIndex | Property | Behavior |
|----------------|----------|----------|
| -12 | Intensity | Sets via `Modulation::setIntensity()`. For pitch chain, the value is converted from semitones to ratio: `pow(2, newValue/12.0)`, clamped to 0.5-2.0 |
| -13 | Bypassed | Sets bypass state: `newValue == 1.0` enables bypass |
| >= 0 | Standard attribute | Sets via `setAttribute(attributeIndex, newValue, dontSendNotification)` |

After setting the attribute, the method calls `sendOtherChangeMessage` to notify the UI and any connected listeners.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| chainId | Integer | no | Target modulation chain | 1 = GainModulation, 2 = PitchModulation |
| modulatorIndex | Integer | no | Zero-based index of the modulator in the chain | >= 0, must be within chain bounds |
| attributeIndex | Integer | no | Parameter index, or -12 for Intensity, -13 for Bypassed | Processor-type-specific for positive values |
| newValue | Double | no | New parameter value | Range depends on parameter; Intensity: 0.0-1.0 for gain, -12.0 to 12.0 semitones for pitch |

**Pitfalls:**
- The `chainId` values (1=Gain, 2=Pitch) do not start at 0. Passing 0 triggers a `jassertfalse` and a script error "No valid chainType - 1= GainModulation, 2=PitchModulation". This is the same pitfall as `addModulator` and `getModulatorIndex`.
- For pitch chain intensity (`attributeIndex == -12`), the value is in semitones and is converted to a ratio with `pow(2, newValue/12.0)`, clamped to 0.5-2.0 (i.e., -12 to +12 semitones). For gain chain intensity, the value is passed directly to `setIntensity` without conversion.
- The standard `setAttribute` call uses `dontSendNotification` (unlike `Synth.setAttribute` which uses `sendNotification`), but then manually calls `sendOtherChangeMessage`. This means the notification path is different from calling `setAttribute` directly on a ScriptModulator handle.

**Cross References:**
- `$API.Synth.getModulatorIndex$`
- `$API.Synth.addModulator$`
- `$API.Synth.getModulator$`

**Example:**
```javascript:set-modulator-attribute-intensity
// Title: Set the intensity of the first gain modulator to 50%
Synth.setModulatorAttribute(1, 0, -12, 0.5);

// Bypass the second pitch modulator
Synth.setModulatorAttribute(2, 1, -13, 1.0);
```
```json:testMetadata:set-modulator-attribute-intensity
{
  "testable": false,
  "skipReason": "Requires a module tree with modulators present in gain/pitch chains"
}
```

## setShouldKillRetriggeredNote

**Signature:** `undefined setShouldKillRetriggeredNote(Integer killNote)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Sets a boolean member on the owner synth -- a direct member write, no allocations, no locks, no dispatch.
**Minimal Example:** `Synth.setShouldKillRetriggeredNote(true);`

**Description:**
Controls whether the parent synth automatically kills (stops) a voice when a new note-on arrives for the same pitch while the previous voice is still playing. When enabled (`true`, which is the default), a retriggered note immediately kills the previous voice on the same key. When disabled (`false`), both voices coexist and play simultaneously, allowing note stacking on the same pitch.

The method delegates to `ModulatorSynth::setKillRetriggeredNote(killNote)`, which writes to the `shouldKillRetriggeredNote` boolean member. The behavior is evaluated during voice allocation when a new note-on event is processed.

This is useful for polyphonic instruments where you want multiple velocity layers on the same note (e.g., piano sustain resonance), or for effects where overlapping same-pitch voices create a desired sound (e.g., pad layering).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| killNote | Integer | no | Whether to kill retriggered notes | Boolean (0 or 1); default is true |

**Cross References:**
- `$API.Synth.playNote$`
- `$API.Synth.addNoteOn$`
- `$API.Synth.noteOffByEventId$`

## getRoutingMatrix

**Signature:** `ScriptObject getRoutingMatrix(String processorId)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a ScriptRoutingMatrix wrapper on the heap. No objectsCanBeCreated() guard, no WARN_IF_AUDIO_THREAD guard. Should only be called in onInit.
**Minimal Example:** `var rm = Synth.getRoutingMatrix("MySynth");`

**Description:**
Returns a `ScriptRoutingMatrix` handle to the routing matrix of the processor with the given name. This provides script-level access to configure multi-channel output routing for any processor that implements the `RoutableProcessor` interface (synths, effects, containers).

Unlike most `get*()` methods, this uses a **global-rooted search** (`ProcessorHelpers::getFirstProcessorWithName` from `getMainSynthChain()`) -- it searches the entire module tree, not just the parent synth's subtree. This also means it does NOT check `objectsCanBeCreated()` and has no explicit `onInit` restriction.

The method performs a two-step validation: first it checks if any processor with the given name exists (reports "[id] was not found" if not), then it checks if the found processor implements `RoutableProcessor` (reports "[id] does not have a routing matrix" if not). In backend builds, a compile-time diagnostic (`ModuleDiagnoser`) validates the module name against `RoutableProcessor` types.

Despite having no `onInit` restriction, this method allocates a wrapper object on the heap and should only be called in `onInit`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| processorId | String | no | Processor ID of the module whose routing matrix to access | Must match exactly (case-sensitive) |

**Pitfalls:**
- This method searches the entire module tree (global-rooted), not just the parent synth's subtree. A processor in a sibling synth or different hierarchy level will be found.
- No `onInit` restriction is enforced despite allocating a wrapper object. Calling this at runtime is unsafe on the audio thread but produces no error or warning outside debug builds.

**Cross References:**
- `$API.Synth.getEffect$`
- `$API.Synth.getChildSynth$`
- `$API.RoutingMatrix$`

**DiagramRef:** synth-module-tree-search

