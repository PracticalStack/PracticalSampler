# MessageHolder -- Class Analysis

## Brief
Persistent MIDI event container for storing, constructing, and re-injecting HiseEvents across callback boundaries.

## Purpose
MessageHolder wraps HISE's internal 16-byte `HiseEvent` struct as an owned copy, persisting event data independently of callback scope. Unlike the transient `Message` object (which holds a pointer to the live event valid only during a MIDI callback), MessageHolder stores event data by value and can be kept across callbacks, stored in arrays, passed to `Synth.addMessageFromHolder()`, or used with `UnorderedStack` in event-stack mode. It is created via `Engine.createMessageHolder()` or returned by `MidiPlayer.getEventList()`, `File.loadAsMidiFile()`, and similar APIs.

## Details

### Relationship to Message

MessageHolder and Message both wrap the same `HiseEvent` struct but differ fundamentally in ownership and lifecycle:

| Aspect | Message | MessageHolder |
|--------|---------|---------------|
| Storage | Pointer to live event in audio buffer | Owned value copy |
| Lifetime | Valid only during callback scope | Persistent -- lives as long as script holds reference |
| Safety checks | Null pointer, event type, range validation | Minimal -- only `setType()` validates range |
| Write access | Conditional (read-only in voice start modulators) | Always writable |
| Instance model | Singleton per script processor | Multiple instances |
| Event ID system | `makeArtificial()` integrates with EventIdHandler | No direct EventIdHandler interaction |

MessageHolder exposes a superset of Message's event type constants (all 14 `HiseEvent::Type` values) but lacks Message's virtual CC constants (`PITCH_BEND_CC`, `AFTERTOUCH_CC`).

### No Safety Checks

MessageHolder performs no runtime safety validation on getters/setters:

- No null pointer checks (the event is always valid as an owned value)
- No event type guards (calling `getNoteNumber()` on a CC event returns whatever is in the number byte)
- No range validation on setters (except `setType()` which checks 0..13)
- No `ENABLE_SCRIPTING_SAFE_CHECKS` guards
- No `FRONTEND_ONLY` silent returns

This is by design -- MessageHolder is a raw data container. The user must ensure they read/write fields appropriate for the event type.

### Controller Number Type Coercion

`setControllerNumber()` and `getControllerNumber()` use virtual CC numbers (128 = PitchBend, 129 = Aftertouch) for type coercion. See `setControllerNumber()` and `getControllerNumber()` for full details.

### isController() Broadened Semantics

`isController()` returns true for CC, PitchBend, and Aftertouch events. See `isController()` for the full definition and rationale.

### Re-injection via Synth.addMessageFromHolder()

When a MessageHolder is passed to `Synth.addMessageFromHolder()`, the system:

1. Makes a value copy of the event (the original MessageHolder is not modified)
2. Marks the copy as artificial (sets bit 31 of timestamp)
3. For NoteOn: registers with EventIdHandler, assigns a new event ID, returns the event ID
4. For NoteOff: looks up the matching event ID for the note-off, returns the timestamp
5. For other events: adds to buffer, returns 0
6. Reports an error if the MessageHolder contains a default (Empty type) event

### Default State

A newly created MessageHolder (from `Engine.createMessageHolder()`) contains a default-constructed `HiseEvent` with type Empty (0) and all fields zeroed. It must have its type set (at minimum) before being passed to `Synth.addMessageFromHolder()`.

### Integration Points

MessageHolder serves as the universal serialization format for MIDI events across several APIs:

- **MidiPlayer**: `getEventList()` returns and `flushMessageList()` accepts arrays of MessageHolder objects
- **UnorderedStack**: Event-stack mode uses MessageHolder as the scripting interface for push/pop/query operations
- **File**: `loadAsMidiFile()` returns and `writeMidiFile()` accepts MessageHolder arrays
- **Message.store()**: Copies the live callback event into a MessageHolder for deferred use

## obtainedVia
`Engine.createMessageHolder()` -- also returned by `MidiPlayer.getEventList()`, `File.loadAsMidiFile()`, `UnorderedStack.copyTo()`, and `MessageHolder.clone()`.

## minimalObjectToken
mh

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| Empty | 0 | int | Empty/uninitialized event type | EventType |
| NoteOn | 1 | int | Note-on event type | EventType |
| NoteOff | 2 | int | Note-off event type | EventType |
| Controller | 3 | int | MIDI CC event type | EventType |
| PitchBend | 4 | int | Pitch bend event type | EventType |
| Aftertouch | 5 | int | Aftertouch event type (both mono and poly) | EventType |
| AllNotesOff | 6 | int | All-notes-off event type | EventType |
| SongPosition | 7 | int | Song position pointer event type | EventType |
| MidiStart | 8 | int | MIDI start event type | EventType |
| MidiStop | 9 | int | MIDI stop event type | EventType |
| VolumeFade | 10 | int | Internal volume fade event type | EventType |
| PitchFade | 11 | int | Internal pitch fade event type | EventType |
| TimerEvent | 12 | int | Timer callback event type | EventType |
| ProgramChange | 13 | int | Program change event type | EventType |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Synth.addMessageFromHolder(mh)` without setting the type | `mh.setType(mh.NoteOn); mh.setNoteNumber(60); Synth.addMessageFromHolder(mh);` | A default-constructed MessageHolder has type Empty. Passing it to addMessageFromHolder triggers a "Event is empty" error. Set the type and relevant fields first. |

## codeExample
```javascript
// Create a persistent event container
const var mh = Engine.createMessageHolder();
```

## Alternatives
- **Message** -- transient callback-scoped reference to the live event; use `Message.store(mh)` to capture into a MessageHolder
- **UnorderedStack** -- event-stack mode uses MessageHolder as the scripting interface for buffering MIDI events
- **MidiPlayer** -- returns arrays of MessageHolder objects from its event list API

## Related Preprocessors
None.

## Diagrams

### message-holder-lifecycle
- **Brief:** MessageHolder Lifecycle and Integration Points
- **Type:** topology
- **Description:** Shows the lifecycle of a MessageHolder object through the HISE event system. Source nodes: Engine.createMessageHolder() (factory), Message.store() (capture from callback), MidiPlayer.getEventList() (from sequence), File.loadAsMidiFile() (from disk). Center node: MessageHolder (persistent HiseEvent copy). Sink nodes: Synth.addMessageFromHolder() (re-inject into audio buffer, marks artificial), MidiPlayer.flushMessageList() (write back to sequence), File.writeMidiFile() (save to disk), UnorderedStack.insert() (event-stack buffering). The key insight is that MessageHolder always stores a value copy -- data flows in and out via copy, never by reference.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: MessageHolder is a simple data container with no timeline dependencies, no callback-scope restrictions, and no silent-failure preconditions. The only error case (Empty type passed to addMessageFromHolder) is caught at runtime by Synth, not by MessageHolder itself.
