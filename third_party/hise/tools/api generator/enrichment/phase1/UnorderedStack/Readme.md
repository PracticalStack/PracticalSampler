# UnorderedStack -- Class Analysis

## Brief
Lock-free fixed-capacity container for 128 floats or HISE events with set-like insert/remove semantics.

## Purpose
UnorderedStack is a fast, fixed-capacity container designed for audio-thread-safe tracking of active values or MIDI events. It stores up to 128 elements (either floating-point numbers or HiseEvent objects) without preserving insertion order. Insertions prevent duplicates (set semantics), and removal fills gaps by swapping with the last element for O(1) deletion. In event mode, configurable compare functions control how events are matched for removal and lookup, supporting both built-in comparators (by event ID, note+channel, note+velocity, bitwise equality) and custom HiseScript callbacks.

## Details

### Dual-Mode Operation

The stack operates in one of two modes, selected via `setIsEventStack()`:

| Mode | Storage | Element Type | Default |
|------|---------|--------------|---------|
| Float | `hise::UnorderedStack<float, 128>` | Numbers | Yes |
| Event | `hise::UnorderedStack<HiseEvent, 128>` | MessageHolder objects | No |

Both backing arrays exist in memory simultaneously; the `isEventStack` flag controls which is used. Mode should be set once during initialization.

### Event Compare Functions

When in event mode, the compare function determines how `remove()`, `removeIfEqual()`, and `contains()` match events:

| Constant | Value | Match Criteria |
|----------|-------|----------------|
| `BitwiseEqual` | 0 | All event fields must be identical |
| `EventId` | 1 | Event ID only (matches note-on/off pairs) |
| `NoteNumberAndVelocity` | 2 | Note-on events with same note number and velocity |
| `NoteNumberAndChannel` | 3 | Same note number and channel |
| `EqualData` | 4 | Not implemented -- always returns false |

A custom compare function can be passed instead of a constant. It receives two MessageHolder arguments (stack element, search target) and must return true for a match.

### Buffer Access (Float Mode)

See `asBuffer()` for the full buffer-view API. Returns a live view (not a copy) of the underlying float array, with options for occupied-only or full 128-slot access.

### Set Semantics

See `insert()` for the full duplicate-detection and capacity behavior.

### Copy Targets

See `copyTo()` for the three supported target types (Array, Buffer, UnorderedStack) and their constraints.

### Bracket Access

`stack[index]` reads float values (even in event mode, reads from the float array). Writing via bracket is not supported.

## obtainedVia
`Engine.createUnorderedStack()`

## minimalObjectToken
us

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| BitwiseEqual | 0 | int | Compare all event fields for exact equality | CompareFunction |
| EventId | 1 | int | Compare event IDs only (matches note-on/off pairs) | CompareFunction |
| NoteNumberAndVelocity | 2 | int | Match note-on events with same note number and velocity | CompareFunction |
| NoteNumberAndChannel | 3 | int | Match events with same note number and channel | CompareFunction |
| EqualData | 4 | int | Not implemented -- always returns false | CompareFunction |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `us.insert(messageHolder)` without calling `setIsEventStack` | Call `us.setIsEventStack(true, us.EventId)` first | Float mode is the default; inserting a MessageHolder into a float-mode stack silently fails (returns false). |
| `us.storeEvent(0, holder)` in float mode | Switch to event mode first or use bracket access for floats | `storeEvent` reports a script error when called on a float-mode stack. |
| `us.removeIfEqual(holder)` in float mode | Use `us.remove(value)` for float removal | `removeIfEqual` reports a script error when called on a float-mode stack. |

## codeExample
```javascript
// Float mode (default)
const us = Engine.createUnorderedStack();
us.insert(60.0);
us.insert(64.0);
us.insert(67.0);
Console.print(us.size()); // 3
Console.print(us.contains(64.0)); // true

// Event mode
const es = Engine.createUnorderedStack();
es.setIsEventStack(true, es.EventId);
```

## Alternatives
- Array -- ordered, dynamic, mixed-type collections on the UI thread; UnorderedStack is for fast unordered insert/remove safe for the audio thread.
- MidiList -- static 128-slot lookup by note number; UnorderedStack is a dynamic set of active values/events with membership tests.
- FixObjectStack -- multi-property typed objects with fixed layout; UnorderedStack is for simple floats or HISE events with built-in matching.
- Buffer -- fixed-size float arrays with DSP operations; UnorderedStack has dynamic set semantics with insert/remove.
- MessageHolder -- element type for event-mode push/pop; UnorderedStack is the container.

## Related Preprocessors
`USE_BACKEND` (debug popup display only)

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods return clear bool success/failure values and report script errors for mode mismatches. No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
