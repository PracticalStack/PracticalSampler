# Message -- Class Analysis

## Brief
Transient MIDI event accessor for reading and modifying note, controller, and event properties inside callbacks.

## Purpose
Message provides getters and setters for the current MIDI event during `onNoteOn`, `onNoteOff`, and `onController` callbacks. It wraps HISE's internal 16-byte `HiseEvent` struct, offering access to note number, velocity, channel, detune, gain, start offset, event ID, and controller values. The Message object is a singleton per script processor -- not created by user code -- and its internal pointer is only valid during callback execution. Setter methods modify the event in-place in the audio buffer, affecting all downstream processors.

## Details

### HiseEvent Architecture

HISE replaces JUCE's `MidiMessage` with a fixed-size 16-byte `HiseEvent` struct optimized for real-time processing. The struct packs note/controller data, transpose, gain, detune, event ID, start offset, and timestamp (with flag bits) into four 32-bit words. This fixed size enables zero-allocation buffer operations.

### Mutable vs Read-Only Context

Message uses a dual-pointer pattern internally:

| Pointer | Set When | Allows |
|---------|----------|--------|
| `messageHolder` (mutable) | `onNoteOn`, `onNoteOff`, `onController` on a `JavascriptMidiProcessor` | Read + Write |
| `constMessageHolder` (const) | Always set during any MIDI callback, including voice start modulators | Read only |

When `messageHolder` is null (read-only context), all setter methods report an error. This prevents voice start modulators from modifying events they receive as const references.

### Event Type Routing

All event types route through `onController`:

| HiseEvent Type | Callback | Virtual CC Number |
|----------------|----------|-------------------|
| NoteOn | `onNoteOn` | N/A |
| NoteOff | `onNoteOff` | N/A |
| Controller | `onController` | 0-127 (actual CC) |
| PitchBend | `onController` | 128 |
| Aftertouch | `onController` | 129 |
| ProgramChange | `onController` | N/A (use `isProgramChange()`) |
| AllNotesOff | `setAllNotesOffCallback` handler | N/A |

### Artificial Event System

Events created by scripts (via `makeArtificial()`, `Synth.addNoteOn()`, etc.) are marked with a flag in bit 31 of the timestamp. The system manages note-on/note-off pairing through the global `EventIdHandler`. See `makeArtificial()`, `makeArtificialOrLocal()`, `isArtificial()`, and `ignoreEvent()` for full behavioral details and the note-on reinsert mechanism.

### Event ID System

Event IDs are unsigned 16-bit integers (0-65535) that wrap around. The `EventIdHandler` on `MainController` assigns sequential IDs to note-on events and copies matching IDs to note-off events before script processing begins. Artificial events receive IDs from the same counter. Overlapping notes on the same note number are handled via an internal stack.

### Pitch Wheel Value Range

Pitch wheel uses 14-bit encoding (0-16383), stored across two bytes of the HiseEvent. See `getControllerValue()` for the full dispatch logic and range differences across event types.

### Start Offset vs Timestamp

`setStartOffset()` and `delayEvent()` serve different purposes. See each method's documentation for details on their distinct behaviors regarding sample playback position vs event timing.

## obtainedVia
Globally available as `Message` in MIDI callback scripts (onNoteOn, onNoteOff, onController). Not user-created.

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| NoteOn | 1 | int | Note-on event type | EventType |
| NoteOff | 2 | int | Note-off event type | EventType |
| Controller | 3 | int | MIDI CC event type | EventType |
| PitchBend | 4 | int | Pitch bend event type | EventType |
| Aftertouch | 5 | int | Aftertouch event type (both mono and poly) | EventType |
| AllNotesOff | 6 | int | All-notes-off event type | EventType |
| VolumeFade | 10 | int | Internal volume fade event type | EventType |
| PitchFade | 11 | int | Internal pitch fade event type | EventType |
| PITCH_BEND_CC | 128 | int | Virtual CC number for pitch wheel in onController | VirtualCC |
| AFTERTOUC_CC | 129 | int | Virtual CC number for aftertouch in onController (note: historical typo in name) | VirtualCC |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `Message.getNoteNumber()` inside `onController` | Check event type first with `Message.getControllerNumber()` | Note getters require NoteOn/NoteOff events. Calling them on controller events triggers a script error. |
| Calling `Message.setVelocity()` inside `onNoteOff` | Only call `setVelocity()` in `onNoteOn` | setVelocity requires a NoteOn event specifically, not just any note event. |
| Reading `Message` properties outside a MIDI callback | Use `Message.store()` to copy into a MessageHolder | The Message object's internal pointer is only valid during callback execution. Accessing it outside callbacks returns errors or undefined values. |

## codeExample
```javascript
// Message is globally available in MIDI callbacks -- no creation needed.
// In onNoteOn:
function onNoteOn()
{
    local ch = Message.getChannel();
    local note = Message.getNoteNumber();
    local vel = Message.getVelocity();

    if (vel < 20)
        Message.setVelocity(20); // Enforce minimum velocity
}
```

## Alternatives
- **MessageHolder** -- persistent container for storing MIDI events beyond callback scope via `Message.store()`
- **MidiList** -- 128-slot persistent array indexed by note number for note-level data storage

## Related Preprocessors
`USE_BACKEND` -- `sendToMidiOut()` validates EnableMidiOut project setting; `setAllNotesOffCallback()` checks realtime safety.
`ENABLE_SCRIPTING_SAFE_CHECKS` -- guards null-pointer and event-type checks in all getter/setter methods.
`FRONTEND_ONLY` -- `getEventId()`, `getTransposeAmount()`, `setTransposeAmount()` return silent defaults in exported plugins instead of throwing errors.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Message methods already have comprehensive runtime error reporting via reportIllegalCall() for wrong-context and wrong-event-type usage. The existing safe checks cover all the precondition failures that a diagnostic could flag at parse time.
