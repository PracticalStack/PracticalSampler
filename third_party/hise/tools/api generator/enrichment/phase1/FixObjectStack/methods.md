# FixObjectStack -- Method Entries

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clear();`

**Description:**
Resets all allocated slots to their default values and sets the position pointer to zero. Unlike `clearQuick()`, this writes default values into every slot including those beyond the current position, ensuring no stale data remains in memory.

**Parameters:**
(none)

**Pitfalls:**
- Iterates all allocated slots (the full `length` capacity), not just the used portion. On large stacks this does more work than `clearQuick()`.

**Cross References:**
- `$API.FixObjectStack.clearQuick$`

---

## clearQuick

**Signature:** `undefined clearQuick()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clearQuick();`

**Description:**
Resets the position pointer to zero without modifying element data. Elements remain in memory with their previous values but are no longer accessible through `size()`, `indexOf()`, or `contains()`. Subsequent inserts overwrite the old data. Use this for performance when the stack will be repopulated.

**Parameters:**
(none)

**Pitfalls:**
- Old data persists in memory. `toBase64()` still serializes the full memory block including slots that were logically removed by `clearQuick()`.

**Cross References:**
- `$API.FixObjectStack.clear$`

---

## contains

**Signature:** `Integer contains(ScriptObject obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** O(n) linear scan over used elements via the compare function.
**Minimal Example:** `var found = {obj}.contains(myObj);`

**Description:**
Returns true (1) if the stack contains an element matching `obj` according to the factory's compare function, false (0) otherwise. Only searches the used portion of the stack (indices 0 to `size() - 1`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to search for | Must be a FixObject from the same factory |

**Pitfalls:**
- Returns false (not an error) when `obj` is not a valid FixObject. Non-FixObject arguments silently fail the internal type check.

**Cross References:**
- `$API.FixObjectStack.indexOf$`

---

## copy

**Signature:** `undefined copy(String propertyName, AudioData target)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations. Iterates full capacity (O(length)).
**Minimal Example:** `{obj}.copy("velocity", targetBuffer);`

**Description:**
Copies the value of the named property from each element into a target Buffer or Array. Reads from ALL allocated slots (the full `length` capacity), not just the used portion up to `size()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Name of the property to extract from each element | Must match a property in the factory layout |
| target | AudioData | no | A Buffer or Array to receive the values | Must have at least `length` elements |

**Pitfalls:**
- Reads from all allocated slots including unused ones beyond `size()`. Unused slots contain default values (0 for int/float, false for bool). To copy only the used portion, iterate manually from 0 to `size()`.

**Cross References:**
- `$API.FixObjectStack.size$`
- `$API.FixObjectStack.fill$`

**Example:**
```javascript:copy-used-portion-workaround
// Title: Copy only used portion of a stack property
const var f = Engine.createFixObjectFactory({
    "note": 0,
    "velocity": 0.0
});

const var s = f.createStack(16);
const var obj = f.create();

obj.note = 60;
obj.velocity = 0.8;
s.insert(obj);
obj.note = 72;
obj.velocity = 0.6;
s.insert(obj);

// copy() reads all 16 slots -- use manual loop for used portion only
var velocities = [];
for (i = 0; i < s.size(); i++)
    velocities.push(s[i].velocity);

Console.print(velocities.length); // 2, not 16
```
```json:testMetadata:copy-used-portion-workaround
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "s.size()", "value": 2},
    {"type": "REPL", "expression": "velocities.length", "value": 2},
    {"type": "REPL", "expression": "velocities[0]", "value": 0.8}
  ]
}
```

---

## fill

**Signature:** `undefined fill(ScriptObject obj)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.fill(templateObj);`

**Description:**
Copies the data from `obj` into ALL allocated slots (the full `length` capacity). Does not update the position pointer. After fill, the stack's `size()` remains unchanged.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject whose data is copied into every slot | Must be a FixObject from the same factory |

**Pitfalls:**
- Does not update the position pointer. After `fill()`, `size()` still returns the previous value, even though all slots now contain valid data.
- Fills beyond the used portion. On a stack with `size()` of 3 and `length` of 16, all 16 slots receive the data.

**Cross References:**
- `$API.FixObjectStack.clear$`
- `$API.FixObjectStack.copy$`

---

## fromBase64

**Signature:** `undefined fromBase64(String b64)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Base64 decoding involves String parsing and memory block allocation.
**Minimal Example:** `{obj}.fromBase64(savedState);`

**Description:**
Restores the raw memory block from a Base64-encoded string produced by `toBase64()`. Overwrites ALL allocated slots (the full memory block) but does NOT restore the position pointer. After calling `fromBase64`, the position remains at its current value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded string from a previous `toBase64()` call | Must match the stack's memory layout size |

**Pitfalls:**
- [BUG] Does not save or restore the position pointer. The serialization format (inherited from FixObjectArray) only contains raw element data. After `fromBase64()`, `size()` returns the pre-restore value. To correctly restore a stack, save `size()` separately before `toBase64()` and re-insert elements after `fromBase64()`, or track the used count externally.

**Cross References:**
- `$API.FixObjectStack.toBase64$`

---

## indexOf

**Signature:** `Integer indexOf(ScriptObject obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** O(n) linear scan over used elements via the compare function.
**Minimal Example:** `var idx = {obj}.indexOf(myObj);`

**Description:**
Returns the index of the first element matching `obj` according to the factory's compare function, or -1 if not found. Only searches the used portion of the stack (indices 0 to `size() - 1`).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to search for | Must be a FixObject from the same factory |

**Pitfalls:**
- Returns -1 (not an error) when `obj` is not a valid FixObject. Non-FixObject arguments silently fail the internal type check.

**Cross References:**
- `$API.FixObjectStack.contains$`
- `$API.FixObjectStack.remove$`

---

## insert

**Signature:** `Integer insert(ScriptObject obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** O(n) linear scan for duplicate detection via indexOf.
**Minimal Example:** `var ok = {obj}.insert(myObj);`

**Description:**
Inserts a copy of `obj` at the end of the used portion if no duplicate exists. Duplicates are detected using the factory's compare function. Returns true (1) on success, false (0) if the object already exists or is not a valid FixObject.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to insert | Must be a FixObject from the same factory |

**Pitfalls:**
- [BUG] Off-by-one in capacity management. The position pointer is clamped to `length - 1` after writing, so the element written to the last slot is never counted by `size()`. The effective capacity is `length - 1`, not `length`. For a stack created with `createStack(N)`, only N-1 elements are usable. On a capacity-1 stack, `insert()` always writes an invisible element.
- Returns false for non-FixObject arguments without reporting an error.

**Cross References:**
- `$API.FixObjectStack.set$`
- `$API.FixObjectStack.remove$`
- `$API.FixObjectStack.contains$`

**Example:**
```javascript:insert-with-duplicates
// Title: Insert with duplicate detection
const var f = Engine.createFixObjectFactory({
    "note": 0,
    "velocity": 0.0
});

f.setCompareFunction("note");
const var s = f.createStack(8);
const var obj = f.create();

obj.note = 60;
obj.velocity = 0.8;
var result1 = s.insert(obj);

// Same note value -- rejected as duplicate
obj.velocity = 0.5;
var result2 = s.insert(obj);

Console.print(result1); // 1 (true -- inserted)
Console.print(result2); // 0 (false -- duplicate note=60)
Console.print(s.size()); // 1
```
```json:testMetadata:insert-with-duplicates
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "result1", "value": 1},
    {"type": "REPL", "expression": "result2", "value": 0},
    {"type": "REPL", "expression": "s.size()", "value": 1},
    {"type": "REPL", "expression": "s[0].velocity", "value": 0.8}
  ]
}
```

---

## isEmpty

**Signature:** `Integer isEmpty()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var empty = {obj}.isEmpty();`

**Description:**
Returns true (1) if the stack has no used elements (position is 0), false (0) otherwise.

**Parameters:**
(none)

**Cross References:**
- `$API.FixObjectStack.size$`
- `$API.FixObjectStack.clear$`

---

## remove

**Signature:** `Integer remove(ScriptObject obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** O(n) linear scan for element lookup via indexOf.
**Minimal Example:** `var ok = {obj}.remove(myObj);`

**Description:**
Finds and removes the first element matching `obj` according to the factory's compare function. Uses swap-and-pop removal: the last used element is moved into the gap. Returns true (1) if the element was found and removed, false (0) if not found.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to remove | Must be a FixObject from the same factory |

**Pitfalls:**
- Does not preserve element order. The last used element replaces the removed one. If iteration order matters, call `sort()` after removal.

**Cross References:**
- `$API.FixObjectStack.removeElement$`
- `$API.FixObjectStack.insert$`
- `$API.FixObjectStack.indexOf$`

---

## removeElement

**Signature:** `Integer removeElement(Integer index)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ok = {obj}.removeElement(0);`

**Description:**
Removes the element at the given index using swap-and-pop: the last used element is copied into the slot at `index`, and the position pointer is decremented. Returns true (1) on success, false (0) if the index is out of range.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| index | Integer | no | Zero-based index of the element to remove | 0 to `size() - 1` |

**Pitfalls:**
- Does not preserve element order. The last used element replaces the removed one. If iteration order matters, call `sort()` after removal.

**Cross References:**
- `$API.FixObjectStack.remove$`

---

## set

**Signature:** `undefined set(ScriptObject obj)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** O(n) linear scan for existence check via indexOf.
**Minimal Example:** `{obj}.set(myObj);`

**Description:**
Upsert operation: if an element matching `obj` exists (by the factory's compare function), replaces it in place. If no match exists and the stack is not full, inserts at the end. The C++ method returns `bool` (false when full and not found), but the scripting wrapper discards the return value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to set or insert | Must be a FixObject from the same factory |

**Pitfalls:**
- [BUG] The scripting wrapper uses `API_VOID_METHOD_WRAPPER_1` instead of `API_METHOD_WRAPPER_1`, discarding the `bool` return value. From script, `set()` always returns `undefined`, making it impossible to detect when the stack is full and the insert was rejected.
- [BUG] Shares the insert off-by-one: the capacity check uses `position < length - 1`, so the effective capacity for new elements is `length - 1`.

**Cross References:**
- `$API.FixObjectStack.insert$`
- `$API.FixObjectStack.contains$`

**Example:**
```javascript:set-upsert
// Title: Upsert behavior of set()
const var f = Engine.createFixObjectFactory({
    "note": 0,
    "velocity": 0.0
});

f.setCompareFunction("note");
const var s = f.createStack(8);
const var obj = f.create();

// Insert new entry
obj.note = 60;
obj.velocity = 0.8;
s.set(obj);

// Update existing entry (same note, different velocity)
obj.velocity = 0.5;
s.set(obj);

Console.print(s.size());       // 1 (replaced, not added)
Console.print(s[0].velocity);  // 0.5 (updated)
```
```json:testMetadata:set-upsert
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "s.size()", "value": 1},
    {"type": "REPL", "expression": "s[0].velocity", "value": 0.5}
  ]
}
```

---

## size

**Signature:** `Integer size()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var n = {obj}.size();`

**Description:**
Returns the number of used elements in the stack (the current position pointer value). This is NOT the allocated capacity -- use the `length` constant for that.

**Parameters:**
(none)

**Cross References:**
- `$API.FixObjectStack.isEmpty$`

---

## sort

**Signature:** `undefined sort()`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** O(n log n) comparison sort over used elements.
**Minimal Example:** `{obj}.sort();`

**Description:**
Sorts the used portion of the stack (indices 0 to `size() - 1`) using the factory's compare function. Unused slots beyond the position pointer are not affected.

**Parameters:**
(none)

**Cross References:**
- `$API.FixObjectStack.removeElement$`
- `$API.FixObjectStack.remove$`

---

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Base64 encoding creates a new String with heap allocation.
**Minimal Example:** `var state = {obj}.toBase64();`

**Description:**
Serializes the entire raw memory block (all allocated slots) as a Base64-encoded string. Includes data from unused slots beyond the position pointer. Does NOT encode the position value itself.

**Parameters:**
(none)

**Pitfalls:**
- [BUG] Serializes the full memory block including unused slots. The position pointer is not included in the output. Use a separate mechanism to save and restore the used count alongside the Base64 string.

**Cross References:**
- `$API.FixObjectStack.fromBase64$`
