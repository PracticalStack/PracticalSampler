# UnorderedStack -- Method Entries

## asBuffer

**Signature:** `Buffer asBuffer(int getAllElements)`
**Return Type:** `Buffer`
**Call Scope:** safe
**Minimal Example:** `var bf = {obj}.asBuffer(false);`

**Description:**
Returns a Buffer reference to the underlying float array without copying. When `getAllElements` is true, returns a view of all 128 backing slots including unused ones. When false, returns a view of only the occupied elements whose size changes dynamically with insert/remove. Reports a script error if called on an event-mode stack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| getAllElements | Integer | no | If true, return all 128 slots; if false, return only occupied elements | 0 or 1 |

**Pitfalls:**
- Only available in float mode. Calling on an event-mode stack reports a script error.
- The returned Buffer is a live view, not a copy. Modifying the buffer modifies the stack's backing array directly.

**Cross References:**
- `$API.UnorderedStack.insert$`
- `$API.UnorderedStack.copyTo$`

**Example:**
```javascript:asbuffer-views
// Title: Active elements vs full backing array
const var us = Engine.createUnorderedStack();
us.insert(10.0);
us.insert(20.0);
us.insert(30.0);

// Only occupied elements (size = 3)
const var active = us.asBuffer(false);
Console.print(active.length); // 3

// All 128 backing slots
const var all = us.asBuffer(true);
Console.print(all.length); // 128
```
```json:testMetadata:asbuffer-views
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "active.length", "value": 3},
    {"type": "REPL", "expression": "all.length", "value": 128},
    {"type": "REPL", "expression": "active[0]", "value": 10.0}
  ]
}
```

---

## clear

**Signature:** `bool clear()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var wasNonEmpty = {obj}.clear();`

**Description:**
Removes all elements from the stack. Returns true if the stack was non-empty before clearing (i.e., something was actually removed), false if already empty. Works in both float and event modes.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.UnorderedStack.isEmpty$`
- `$API.UnorderedStack.size$`

---

## contains

**Signature:** `bool contains(var value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var found = {obj}.contains(60.0);`

**Description:**
Returns true if the stack contains the specified value. In float mode, performs exact float equality comparison via linear scan. In event mode, uses the configured compare function to test membership.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to search for. Number in float mode, MessageHolder in event mode | -- |

**Pitfalls:**
- Float comparison uses exact equality. Floating-point precision issues may cause contains to return false for values that appear equal.

**Cross References:**
- `$API.UnorderedStack.insert$`
- `$API.UnorderedStack.remove$`
- `$API.UnorderedStack.setIsEventStack$`
- `$API.UnorderedStack.isEmpty$`

---

## copyTo

**Signature:** `bool copyTo(var target)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Array target path performs heap allocations (Array::add, and new ScriptingMessageHolder in event mode). Buffer and UnorderedStack target paths are allocation-free.
**Minimal Example:** `{obj}.copyTo(targetArray);`

**Description:**
Copies all elements from this stack into the target container. Accepts three target types: Array (clears and fills with float values or new MessageHolder objects), Buffer (float mode only; target must be strictly larger than the stack's current element count), and UnorderedStack (same-mode stacks only; uses fast path without duplicate checking). Returns true on success. Reports a script error for unsupported target types.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| target | NotUndefined | no | Target container: Array, Buffer, or UnorderedStack | Must match stack mode for Buffer/Stack targets |

**Pitfalls:**
- [BUG] Buffer target requires strictly larger size than the stack's current element count (uses `<` instead of `<=`). A buffer with exactly the same number of elements as the stack fails silently and returns false.
- When copying to an Array in event mode, new MessageHolder objects are created for each event. The originals and copies are independent.
- When copying to another UnorderedStack, the target is cleared first and elements are inserted without duplicate checking.

**Cross References:**
- `$API.UnorderedStack.asBuffer$`
- `$API.UnorderedStack.storeEvent$`

**Example:**
```javascript:copyto-targets
// Title: Copying stack contents to different target types
const var us = Engine.createUnorderedStack();
us.insert(1.0);
us.insert(2.0);
us.insert(3.0);

// Copy to Array
var arr = [];
us.copyTo(arr);
Console.print(arr.length); // 3

// Copy to Buffer (must be strictly larger than stack size)
const var bf = Buffer.create(4);
us.copyTo(bf);
Console.print(bf[0]); // 1.0
```
```json:testMetadata:copyto-targets
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "arr.length", "value": 3},
    {"type": "REPL", "expression": "arr[0]", "value": 1.0},
    {"type": "REPL", "expression": "bf[0]", "value": 1.0}
  ]
}
```

---

## insert

**Signature:** `bool insert(var value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var added = {obj}.insert(60.0);`

**Description:**
Inserts a value into the stack if it is not already present (set semantics). Returns true if the value was added, false if it was a duplicate or the stack is full (128 elements). In float mode, accepts a numeric value cast to float. In event mode, accepts a MessageHolder and returns false if the value is not a MessageHolder. Duplicate detection uses exact float equality in float mode and `HiseEvent::operator==` in event mode.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to insert. Number in float mode, MessageHolder in event mode | Max 128 elements |

**Pitfalls:**
- Silently returns false when the stack is full (128 elements). No error message is reported.
- In event mode, passing a non-MessageHolder value silently returns false with no error.

**Cross References:**
- `$API.UnorderedStack.contains$`
- `$API.UnorderedStack.remove$`
- `$API.UnorderedStack.removeElement$`

---

## isEmpty

**Signature:** `bool isEmpty()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isEmpty();`

**Description:**
Returns true if the stack contains no elements, false otherwise. Works in both float and event modes.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.UnorderedStack.size$`
- `$API.UnorderedStack.clear$`
- `$API.UnorderedStack.contains$`

---

## remove

**Signature:** `bool remove(var value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var removed = {obj}.remove(60.0);`

**Description:**
Removes the first matching value from the stack. Returns true if found and removed, false otherwise. In float mode, matches by exact float equality. In event mode, uses the configured compare function. Removal fills the gap by swapping with the last element, so element order is not preserved.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | NotUndefined | no | Value to remove. Number in float mode, MessageHolder in event mode | -- |

**Pitfalls:**
- Removal does not preserve element order. The removed element's slot is filled by moving the last element into its position.

**Cross References:**
- `$API.UnorderedStack.insert$`
- `$API.UnorderedStack.contains$`
- `$API.UnorderedStack.removeElement$`
- `$API.UnorderedStack.removeIfEqual$`

---

## removeElement

**Signature:** `undefined removeElement(var index)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.removeElement(0);`

**Description:**
Removes the element at the specified index by swapping it with the last element and decrementing the size. This is an O(1) operation but does not preserve element order. Works in both float and event modes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index of the element to remove | 0 to size()-1 |

**Pitfalls:**
- Does not preserve element order. The element at `index` is replaced by the last element in the stack.

**Cross References:**
- `$API.UnorderedStack.remove$`
- `$API.UnorderedStack.insert$`

---

## removeIfEqual

**Signature:** `undefined removeIfEqual(var holder)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Method's own code is lock-free with bounded O(128) iteration. Custom compare callbacks execute synchronously within the search loop but are classified as target behavior.
**Minimal Example:** `{obj}.removeIfEqual(messageHolder);`

**Description:**
Event-mode only. Finds the first event matching the provided MessageHolder using the configured compare function, removes it from the stack, and writes the actual removed event back into the holder. This "pop matching" operation preserves event metadata that may differ between the search key and the stored event (e.g., different timestamps or velocities when matching by event ID only). Reports a script error if called on a float-mode stack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| holder | ScriptObject | no | MessageHolder used as search key; receives the removed event on match | Must be a MessageHolder |

**Pitfalls:**
- The holder is modified in-place with the removed event's data, which may differ from the search key. If no match is found, the holder is unchanged.

**Cross References:**
- `$API.UnorderedStack.remove$`
- `$API.UnorderedStack.contains$`
- `$API.UnorderedStack.setIsEventStack$`

---

## setIsEventStack

**Signature:** `undefined setIsEventStack(var isEventStack, var eventCompareFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Custom function path creates a ScriptingMessageHolder (heap allocation) and configures a WeakCallbackHolder. Built-in compare constant path is lightweight but the method is a configuration call.
**Minimal Example:** `{obj}.setIsEventStack(true, {obj}.EventId);`

**Description:**
Switches the stack between float mode (default) and event mode. The second parameter configures the compare function used by `contains()`, `remove()`, and `removeIfEqual()` for event matching. Pass one of the built-in compare constants (`BitwiseEqual`, `EventId`, `NoteNumberAndVelocity`, `NoteNumberAndChannel`) or a custom HiseScript inline function that receives two MessageHolder arguments and returns true for a match.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| isEventStack | Integer | no | True to enable event mode, false for float mode | 0 or 1 |
| eventCompareFunction | NotUndefined | no | Compare constant (Integer) or custom compare function (Function) | Constants: 0-4; or inline function with 2 params |

**Callback Signature:** eventCompareFunction(stackEvent: MessageHolder, searchTarget: MessageHolder)

**Pitfalls:**
- [BUG] The `EqualData` constant (4) is exposed but not implemented in the compare function template. Using it causes all comparisons to return false, so `contains()` always returns false and `remove()`/`removeIfEqual()` never find matches.
- [BUG] The `NoteNumberAndChannel` constant (3) checks note number truthiness (non-zero) rather than equality. Note number 0 (C-2) never matches, and any two non-zero note numbers on the same channel match regardless of pitch.
- Mode should be set once during initialization. Switching modes does not clear the previously active stack's data.

**Cross References:**
- `$API.UnorderedStack.contains$`
- `$API.UnorderedStack.remove$`
- `$API.UnorderedStack.removeIfEqual$`

**Example:**
```javascript:setiseventstack-modes
// Title: Configuring event stack with built-in and custom compare
const var es = Engine.createUnorderedStack();

// Built-in compare by event ID (matches note-on/off pairs)
es.setIsEventStack(true, es.EventId);

// Custom compare function (matches by note number only)
const var es2 = Engine.createUnorderedStack();

inline function compareByNote(a, b)
{
    return a.getNoteNumber() == b.getNoteNumber();
};

es2.setIsEventStack(true, compareByNote);

// --- test-only ---
const var h1 = Engine.createMessageHolder();
h1.setNoteNumber(60);
h1.setVelocity(100);
es.insert(h1);

const var h2 = Engine.createMessageHolder();
h2.setNoteNumber(64);
h2.setVelocity(90);
es2.insert(h2);
// --- end test-only ---
```
```json:testMetadata:setiseventstack-modes
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "es.size()", "value": 1},
    {"type": "REPL", "expression": "es2.size()", "value": 1}
  ]
}
```

---

## size

**Signature:** `int size()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var count = {obj}.size();`

**Description:**
Returns the number of elements currently in the stack. Works in both float and event modes.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.UnorderedStack.isEmpty$`
- `$API.UnorderedStack.clear$`

---

## storeEvent

**Signature:** `undefined storeEvent(var index, var holder)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.storeEvent(0, messageHolder);`

**Description:**
Event-mode only. Copies the event at the specified index into the provided MessageHolder. The stack is not modified. Reports a script error if the index is out of bounds or if called on a float-mode stack.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index of the event to read | 0 to size()-1 |
| holder | ScriptObject | no | MessageHolder to receive the event copy | Must be a MessageHolder |

**Pitfalls:**
None.

**Cross References:**
- `$API.UnorderedStack.removeIfEqual$`
- `$API.UnorderedStack.copyTo$`
- `$API.UnorderedStack.setIsEventStack$`
