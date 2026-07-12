# UnorderedStack -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- prerequisites table, enrichment groups
- `enrichment/resources/survey/class_survey_data.json` -- UnorderedStack entry (createdBy, seeAlso)
- `enrichment/phase1/Engine/Readme.md` -- Engine class analysis (factory context)
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1710-1831 -- class declaration
- `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 7270-7712 -- full implementation
- `HISE/hi_scripting/scripting/api/ScriptingApi.h` line 619 -- factory declaration
- `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` lines 3463-3466 -- factory implementation
- `HISE/hi_tools/hi_tools/CustomDataContainers.h` lines 300-520 -- underlying template class

## Prerequisite: Engine

UnorderedStack is created via `Engine.createUnorderedStack()`. The Engine class is the central factory namespace. The factory method simply constructs a new `ScriptUnorderedStack` with the current script processor as owner. No additional configuration or registration occurs at creation time.

---

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 1711-1831

```cpp
class ScriptUnorderedStack : public ConstScriptingObject,
                             public AssignableObject
```

### Inheritance

- **ConstScriptingObject** -- standard base for scripting API objects. Provides `addConstant()`, `reportScriptError()`, API method registration macros, and debug infrastructure.
- **AssignableObject** -- enables bracket-access (`stack[index]`) from HiseScript. The implementation is read-only: `assign()` reports a script error ("Can't assign via index"), while `getAssignedValue()` reads from the float data array with bounds clamping via `jlimit(0, 128, index)`.

### Object Name

Returns `"UnorderedStack"` via `RETURN_STATIC_IDENTIFIER`.

### Debug Infrastructure

- `getDebugValue()` returns `"Used: N"` where N is the current size.
- `getNumChildElements()` returns `data.size()` (float stack size).
- `getChildElement(int index)` returns an `IndexedValue` wrapped in `LambdaValueInformation`.
- `createPopupComponent()` creates a `Display` component (backend only) showing a grid of values.

---

## Dual-Mode Architecture

The class operates in one of two mutually exclusive modes:

1. **Float mode** (default) -- stores up to 128 `float` values in `hise::UnorderedStack<float, 128> data`
2. **Event mode** -- stores up to 128 `HiseEvent` objects in `hise::UnorderedStack<HiseEvent, 128> eventData`

The mode is controlled by the `bool isEventStack` flag, set via `setIsEventStack()`. This flag is checked in virtually every method to dispatch to the correct underlying stack.

Both stacks exist simultaneously in memory -- only one is "active" at a time based on the flag.

### Buffer Wrappers (Float Mode Only)

Two `VariantBuffer::Ptr` members wrap the float data:

- `elementBuffer` -- initially created with size 0, updated via `updateElementBuffer()` to reflect the current `data.size()`. This provides a view of only the occupied elements.
- `wholeBf` -- always wraps all 128 elements. Provides access to the full backing array including unused slots.

`updateElementBuffer()` is called after every mutation in float mode (`insert`, `remove`, `removeElement`, `clear`). It calls `referToData(data.begin(), data.size())` to resize the buffer view.

`asBuffer(getAllElements)` returns either `wholeBf` (if `getAllElements` is true) or `elementBuffer`. Reports a script error if called on an event stack.

---

## Constructor

**File:** `ScriptingApiObjects.cpp` lines 7286-7311

```cpp
ScriptUnorderedStack(ProcessorWithScriptingContent *p):
    ConstScriptingObject(p, 5),  // 5 constants
    compareFunction(p, this, var(), 2)  // WeakCallbackHolder with 2 args
```

### API Method Registration

All methods use plain `ADD_API_METHOD_N` (not typed):

```
ADD_API_METHOD_0(isEmpty)
ADD_API_METHOD_0(size)
ADD_API_METHOD_1(asBuffer)
ADD_API_METHOD_1(insert)
ADD_API_METHOD_1(remove)
ADD_API_METHOD_1(removeElement)
ADD_API_METHOD_1(contains)
ADD_API_METHOD_0(clear)
ADD_API_METHOD_2(setIsEventStack)
ADD_API_METHOD_2(storeEvent)
ADD_API_METHOD_1(removeIfEqual)
ADD_API_METHOD_1(copyTo)
```

No `ADD_TYPED_API_METHOD_N` registrations exist. All parameter types must be inferred.

### Constants (5 total)

```cpp
addConstant("BitwiseEqual",           (int)CompareFunctions::BitwiseEqual);        // 0
addConstant("EventId",                (int)CompareFunctions::EventId);             // 1
addConstant("NoteNumberAndVelocity",  (int)CompareFunctions::NoteNumberAndVelocity); // 2
addConstant("NoteNumberAndChannel",   (int)CompareFunctions::NoteNumberAndChannel);  // 3
addConstant("EqualData",              (int)CompareFunctions::EqualData);           // 4
```

All constants are integers representing compare function modes for event-mode matching.

---

## CompareFunctions Enum

**File:** `ScriptingApiObjects.h` lines 1716-1724

```cpp
enum class CompareFunctions
{
    BitwiseEqual,           // 0
    EventId,                // 1
    NoteNumberAndVelocity,  // 2
    NoteNumberAndChannel,   // 3
    EqualData,              // 4
    Custom                  // 5 (not exposed as constant)
};
```

### MCF Template -- Compare Function Implementations

**File:** `ScriptingApiObjects.h` lines 1787-1803

```cpp
struct MCF
{
    template <CompareFunctions CompareType> static bool equals(const HiseEvent& e1, const HiseEvent& e2)
    {
        switch (CompareType)
        {
        case CompareFunctions::BitwiseEqual:          return e1 == e2;
        case CompareFunctions::EventId:               return e1.getEventId() == e2.getEventId();
        case CompareFunctions::NoteNumberAndChannel:  return e1.getNoteNumber() && e2.getNoteNumber() &&
                                                             e1.getChannel() == e2.getChannel();
        case CompareFunctions::NoteNumberAndVelocity: return e1.isNoteOn() && e2.isNoteOn() &&
                                                             e1.getNoteNumber() == e2.getNoteNumber() &&
                                                             e1.getVelocity() == e2.getVelocity();
        default: jassertfalse;                        return false;
        }
    }
};
```

### Behavioral Tracing of Each Compare Mode

**BitwiseEqual (0):** Uses `HiseEvent::operator==` which compares all bytes of the event. This is the strictest comparison -- events must be identical in every field (type, channel, note number, velocity, event ID, timestamp, etc.).

**EventId (1):** Compares only the event ID field (`getEventId()`). Useful for matching note-on/note-off pairs since HISE assigns matching event IDs to paired note events.

**NoteNumberAndVelocity (2):** Requires BOTH events to be note-on events (`isNoteOn()`), then compares note number AND velocity. Will not match note-off events at all.

**NoteNumberAndChannel (3):** Compares note number AND channel. Note: there is a potential bug in the implementation -- `e1.getNoteNumber() && e2.getNoteNumber()` evaluates note numbers as boolean (non-zero check) rather than comparing them. This means note 0 (C-2) would never match, and any two non-zero note numbers on the same channel would match regardless of their values. However, the `&&` chain short-circuits, so `e1.getNoteNumber() && e2.getNoteNumber() && e1.getChannel() == e2.getChannel()` would only work correctly if the intent is to check that both notes are non-zero AND on the same channel. This appears to be a bug -- it should likely be `e1.getNoteNumber() == e2.getNoteNumber() && e1.getChannel() == e2.getChannel()`.

**EqualData (4):** Falls through to the `default` case in the MCF template, which hits `jassertfalse` and returns `false`. This means `EqualData` is defined as a constant but its comparison logic is not implemented -- it will always fail to match in debug builds (assertion) and silently return false in release builds.

**Custom (5):** Not exposed as a constant. Selected when `setIsEventStack` receives a function object instead of an integer. Uses a `WeakCallbackHolder` to call a user-defined HiseScript function.

### setIsEventStack() Implementation

**File:** `ScriptingApiObjects.cpp` lines 7683-7712

The second parameter (`eventCompareFunction`) serves dual purpose:
- If it's an **object** (function): Sets `compareFunctionType = Custom`, creates a `WeakCallbackHolder`, increments its ref count, and creates a `ScriptingMessageHolder` for the comparison callback's first argument.
- If it's a **number**: Casts to `CompareFunctions` enum and assigns the corresponding `MCF::equals<>` template instantiation to the `hcf` std::function.

---

## Underlying C++ Template: `hise::UnorderedStack<T, SIZE, LockType>`

**File:** `HISE/hi_tools/hi_tools/CustomDataContainers.h` lines 300-520

This is the core data structure. Key characteristics:

### Design
- Fixed-size array of SIZE elements on the stack (no heap allocation)
- Elements packed contiguously from index 0 to `position - 1`
- `position` tracks the count of active elements
- Default `LockType=DummyCriticalSection` (no locking in the scripting API usage)

### Algorithmic Properties
- **insert()**: O(n) -- calls `contains()` first to prevent duplicates. Returns false if already present. Clamps position to SIZE-1 (silent no-op if full).
- **insertWithoutSearch()**: O(1) -- skips the duplicate check (debug assertion only). Used internally by `copyTo`.
- **remove()**: O(n) -- linear search for element, then calls `removeElement`.
- **removeElement(index)**: O(1) -- swaps the element at `index` with the last element, decrements position. This is the "unordered" part -- removal does not preserve order.
- **contains()**: O(n) -- linear search via `indexOf()`.
- **clear()**: memset to zero + reset position.
- **size()**: O(1) -- returns `position`.
- **isEmpty()**: O(1) -- checks `position == 0`.
- **operator[]**: Returns copy of element at index, or default-constructed element if out of bounds.
- **begin()/end()**: Raw pointer iteration over active elements.

### Capacity
The scripting API instantiates with SIZE=128, so max 128 elements for both float and event modes.

### Duplicate Prevention
`insert()` checks `contains()` before inserting. If the value already exists, it returns false and does not insert. This is a set-like behavior. The scripting wrapper's `insert()` delegates directly to this.

---

## Method Implementation Details (Infrastructure-Relevant)

### insert(var value)

Float mode: casts `value` to float via `(float)value` (implicit var-to-float), delegates to `data.insert()`, calls `updateElementBuffer()`.

Event mode: requires `value` to be a `ScriptingMessageHolder*` (dynamic_cast). Calls `m->getMessageCopy()` to extract the HiseEvent, then delegates to `eventData.insert()`. Returns false if the value is not a MessageHolder.

### remove(var value)

Float mode: delegates to `data.remove((float)value)`, which does a contains-check + linear search + removeElement.

Event mode: calls `getIndexForEvent(value)` using the configured compare function, then `eventData.removeElement(index)`.

### contains(var value)

Float mode: `data.contains((float)value)`.

Event mode: `getIndexForEvent(value) != -1`.

### getIndexForEvent(var value) -- Private Helper

Central event-matching function used by `remove`, `removeIfEqual`, and `contains` (via the event path).

For **Custom** compare: iterates all events, calls the WeakCallbackHolder synchronously with `[compareHolder, value]` as args. The `compareHolder` (internal MessageHolder) is loaded with each stack event in turn, while `value` is the search target. Returns the index where the callback returns true.

For **built-in** compare: iterates all events, calls `hcf(e1, eventData[i])` where e1 is extracted via `m->getMessageCopy()`.

### removeIfEqual(var holder)

Event-only (reports error in float mode). Finds matching event via `getIndexForEvent()`, removes it from the stack, and WRITES the removed event back into the holder MessageHolder. This is a "pop matching" operation -- the caller gets the actual stack event, not the search key.

### storeEvent(int index, var holder)

Event-only. Copies the event at `index` into the provided MessageHolder. Bounds-checked with `isPositiveAndBelow(index, size())`.

### copyTo(var target)

Supports three target types:
1. **Array**: Clears and fills. Float mode appends var(float). Event mode creates new `ScriptingMessageHolder` objects for each event.
2. **Buffer**: Float-only. Checks `data.size() < b->size` (strictly less -- note this means the buffer must be strictly larger than the stack size). Clears buffer and copies via `FloatVectorOperations::copy`.
3. **UnorderedStack**: Same-mode stacks only. Uses `clearQuick()` + `insertWithoutSearch()` for each element.

Reports "No valid container" for any other target type.

### clear()

Returns `bool` -- true if the stack was non-empty before clearing (i.e., something was actually cleared). This is unusual -- most clear methods return void.

### AssignableObject Implementation

- `assign(index, value)` -- always reports error "Can't assign via index". Stack is read-only via bracket access.
- `getAssignedValue(index)` -- reads from `data.begin()[jlimit(0, 128, index)]`. Only works for float mode. The clamping means out-of-bounds indices silently return element 0 or 127.

---

## Display Component (Backend Only)

**File:** `ScriptingApiObjects.cpp` lines 7313-7401

A debug visualization component (popup in HISE IDE):

- Float mode: 8-column grid, 70px cells, 16 rows (128 total cells)
- Event mode: 1-column list, 500px cells, 128 rows
- Active elements shown with white text on 0.2 alpha fill
- Empty slots shown with 0.05 alpha fill
- Refreshes at 30ms timer interval
- Uses `WeakReference<ScriptUnorderedStack>` -- handles recompilation gracefully with "Refresh this window" message
- Wrapped in a Viewport if height exceeds 400px

---

## Factory Method

**File:** `ScriptingApi.cpp` lines 3463-3466

```cpp
ScriptingObjects::ScriptUnorderedStack* ScriptingApi::Engine::createUnorderedStack()
{
    return new ScriptingObjects::ScriptUnorderedStack(getScriptProcessor());
}
```

Simple constructor call. No parameters, no registration, no singleton pattern.

---

## Threading Considerations

The underlying `hise::UnorderedStack` template uses `DummyCriticalSection` by default (as used in `ScriptUnorderedStack`), meaning NO locking. The scripting API wrapper adds no synchronization either.

This makes the stack safe for audio-thread use (no lock contention) but requires that access is single-threaded. Typical use is within MIDI callbacks (onNoteOn, onNoteOff, onController) which run on the audio thread.

The `WeakCallbackHolder` used for custom compare functions calls `callSync()`, which executes the callback synchronously on the current thread.

---

## Preprocessor Guards

- `#if USE_BACKEND` -- guards the `Display` popup component creation in `createPopupComponent()`
- No other preprocessor guards affect the class

---

## Key Observations

1. **Set semantics**: `insert()` prevents duplicates via the underlying template's `contains()` check. This is implicit -- the API description says "inserts at end" but does not mention duplicate prevention.

2. **EqualData constant is broken**: The `EqualData` compare mode is exposed as a constant (value 4) but has no implementation in the MCF template -- it falls through to `jassertfalse; return false`.

3. **NoteNumberAndChannel has a bug**: The comparison uses `e1.getNoteNumber() && e2.getNoteNumber()` (boolean truth check) instead of `e1.getNoteNumber() == e2.getNoteNumber()` (equality). This means it only checks that both note numbers are non-zero and that channels match.

4. **removeIfEqual is a pop+match**: It removes the matching event AND writes the removed event (from the stack, not the search key) back into the holder. This preserves event metadata that may differ between the search key and the stored event.

5. **clear() returns bool**: Returns true if something was cleared, false if already empty. Useful for conditional logic.

6. **Read-only bracket access**: `stack[i]` reads float values only (even in event mode, it reads from the float data array). Assignment via bracket is explicitly rejected.

7. **copyTo buffer size check**: Uses strict less-than (`data.size() < b->size`), meaning if the buffer is exactly the same size as the stack, the copy fails silently (returns false).

8. **No capacity constant**: The 128-element limit is hardcoded but not exposed as a constant to scripts.
