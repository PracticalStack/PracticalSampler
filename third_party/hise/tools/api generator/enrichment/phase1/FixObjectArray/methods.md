# FixObjectArray -- Method Documentation

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clear();`

**Description:**
Resets all elements in the array to their default values. Internally delegates to `fill(var())`, which calls `ObjectReference::clear()` on every slot. Each element's properties are set back to the defaults defined by the factory's prototype (e.g., 0 for integers, 0.0 for floats, false for booleans). The array size remains unchanged.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Unlike `FixObjectStack.clear()`, this does not change the array's `size()` or iteration range. All `length` elements remain valid and iterable after clearing -- they are simply reset to default values.

**Cross References:**
- `$API.FixObjectArray.fill$`
- `$API.FixObjectStack.clear$`

## contains

**Signature:** `Integer contains(var obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Linear search over all elements using the compare function. O(n) where n is the array length.
**Minimal Example:** `var found = {obj}.contains(element);`

**Description:**
Returns 1 if the array contains an element matching the given object, 0 otherwise. Delegates to `indexOf(obj) != -1`. The comparison uses the factory's compare function -- by default this is byte-level equality (memcmp). If a property-based or custom compare function has been set on the factory, that comparator is used instead.

The argument must be a FixObject (ObjectReference) from the same factory layout. Passing any other type (number, string, array, JSON object) always returns 0 because the dynamic_cast to ObjectReference fails in the indexOf implementation.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to search for | Must be a FixObject from the same factory layout |

**Pitfalls:**
- [BUG] Passing a plain JSON object (e.g. `{"id": 5}`) instead of a FixObject silently returns 0. The method does not report an error -- the argument type check fails quietly.

**Cross References:**
- `$API.FixObjectArray.indexOf$`
- `$API.FixObjectFactory.setCompareFunction$`

## copy

**Signature:** `Integer copy(String propertyName, var target)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Populates the target Array or Buffer, which involves heap operations (Array::ensureStorageAllocated, Array::set).
**Minimal Example:** `var ok = {obj}.copy("velocity", targetBuffer);`

**Description:**
Extracts a single named property from every element in the array and writes the values into a target Buffer or Array. Returns 1 on success, 0 on failure.

When the target is a Buffer, each property value is cast to float. The Buffer must have the same size as the array's `length` constant -- a size mismatch produces a script error. When the target is a regular Array, it is resized to fit and populated with the property values (integers remain integers, floats remain floats).

The property name must match one of the members defined in the factory's layout prototype. An unknown property name produces a script error.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| propertyName | String | no | Name of the FixObject property to extract from each element | Must match a property in the factory layout |
| target | ScriptObject | no | Destination Buffer or Array to receive the extracted values | Buffer must have size == array length |

**Pitfalls:**
- When copying to a Buffer, all values are cast to float. Integer properties lose precision for values outside the float32 representable range (beyond +/-16777216).
- [BUG] Passing a target that is neither a Buffer nor an Array silently returns 0 with no error message.

**Cross References:**
- `$API.FixObjectArray.fill$`
- `$API.FixObjectFactory.createArray$`

**Example:**
```javascript:copy-to-buffer
// Title: Extract a property column into a Buffer
const var f = Engine.createFixObjectFactory({
    "id": 0,
    "velocity": 0.0
});

const var a = f.createArray(4);

a[0].velocity = 0.25;
a[1].velocity = 0.5;
a[2].velocity = 0.75;
a[3].velocity = 1.0;

const var buf = Buffer.create(4);
a.copy("velocity", buf);

Console.print(buf[2]); // 0.75
```
```json:testMetadata:copy-to-buffer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "buf[0]", "value": 0.25},
    {"type": "REPL", "expression": "buf[2]", "value": 0.75}
  ]
}
```

## fill

**Signature:** `undefined fill(var obj)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.fill(templateElement);`

**Description:**
Fills every element in the array with the given value. If the argument is a FixObject (ObjectReference), its data is deep-copied (memcpy) into every slot. If the argument is anything else (including no argument or a non-FixObject value), every element is reset to its default values -- equivalent to calling `clear()`.

This makes `fill` a dual-purpose method: pass a FixObject to broadcast a template across all slots, or pass any non-FixObject value (e.g., `0`) to reset the array to defaults.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | NotUndefined | no | A FixObject to copy into every slot, or any other value to reset all elements to defaults | FixObject must be from the same factory layout |

**Pitfalls:**
- [BUG] Passing a plain JSON object (e.g., `{"id": 5}`) does not fill elements with those values. It triggers the non-FixObject branch, resetting all elements to defaults instead. No error is reported.

**Cross References:**
- `$API.FixObjectArray.clear$`
- `$API.FixObjectArray.copy$`

## fromBase64

**Signature:** `Integer fromBase64(String b64)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ok = {obj}.fromBase64(savedState);`

**Description:**
Restores the array's raw memory from a Base64-encoded string. Decodes the string into a memory block and validates that the decoded size matches the array's total allocation (`elementSize * numElements`) exactly. On match, the decoded bytes are copied directly into the array's memory via memcpy, overwriting all element data. Returns 1 on success.

If the decoded size does not match, the array is left unchanged and the method returns 0. No script error is thrown on size mismatch -- this is a silent failure that must be checked via the return value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| b64 | String | no | Base64-encoded string previously produced by `toBase64()` | Decoded size must match array allocation exactly |

**Pitfalls:**
- Size mismatch on restore is silent. If the factory layout changed between saving and loading (different properties, different element count), `fromBase64` returns 0 without any error or warning. Always check the return value.

**Cross References:**
- `$API.FixObjectArray.toBase64$`

## indexOf

**Signature:** `Integer indexOf(var obj)`
**Return Type:** `Integer`
**Call Scope:** warning
**Call Scope Note:** Linear search over all elements using the compare function. O(n) where n is the array length.
**Minimal Example:** `var idx = {obj}.indexOf(element);`

**Description:**
Returns the index of the first element matching the given object, or -1 if not found. Performs a linear search through `size()` elements, using the factory's compare function for equality testing (`compareFunction(item, obj) == 0`).

The argument must be a FixObject (ObjectReference). If the argument is any other type, the dynamic_cast to ObjectReference fails and the method returns -1 immediately without searching.

With the default comparator, equality is byte-level (memcmp of the entire element). With a property-based comparator (e.g., `factory.setCompareFunction("id")`), only the specified properties are compared.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| obj | ScriptObject | no | The FixObject to search for | Must be a FixObject from the same factory layout |

**Pitfalls:**
- [BUG] Passing a plain JSON object instead of a FixObject silently returns -1. No error is reported -- the type check fails quietly.
- With the default byte-level comparator, two objects that differ in any property (even unused ones) are considered unequal. Set a property-based compare function on the factory for field-specific matching.

**Cross References:**
- `$API.FixObjectArray.contains$`
- `$API.FixObjectFactory.setCompareFunction$`

## size

**Signature:** `Integer size()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var n = {obj}.size();`

**Description:**
Returns the fixed number of elements in the array. This always equals the `length` constant set at creation time and never changes. For FixObjectArray, `size()` and the `length` constant are interchangeable.

Note: FixObjectStack overrides this method to return the current occupancy (`position`) rather than the total capacity. When writing code that works with both container types, `size()` is the portable way to get the number of "active" elements.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.FixObjectStack.size$`

## sort

**Signature:** `undefined sort()`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Uses std::sort with the factory's compare function. O(n log n) where n is the array size. If a custom JavaScript compare function is set, each comparison invokes a script callback.
**Minimal Example:** `{obj}.sort();`

**Description:**
Sorts the array elements in place using the factory's compare function. Creates a local `Sorter` struct that delegates to the `compareFunction` and passes it to `std::sort` via a `SortFunctionConverter` adapter. Only the first `size()` elements are sorted (this distinction matters for FixObjectStack, which may have fewer active elements than capacity).

The sort is only meaningful when a property-based or custom compare function has been set on the factory via `setCompareFunction()`. With the default comparator, ordering is by pointer address, which produces arbitrary (but stable per session) results.

**Parameters:**

(No parameters.)

**Pitfalls:**
- Calling `sort()` without first setting a compare function on the factory produces meaningless ordering (pointer address comparison). No warning or error is emitted.

**Cross References:**
- `$API.FixObjectArray.indexOf$`
- `$API.FixObjectArray.contains$`
- `$API.FixObjectFactory.setCompareFunction$`

**Example:**
```javascript:sort-by-property
// Title: Sort a FixObjectArray by a named property
const var f = Engine.createFixObjectFactory({
    "id": 0,
    "velocity": 0.0
});

const var a = f.createArray(4);

a[0].id = 3;
a[1].id = 1;
a[2].id = 4;
a[3].id = 2;

f.setCompareFunction("id");
a.sort();

Console.print(a[0].id); // 1
Console.print(a[3].id); // 4
```
```json:testMetadata:sort-by-property
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "a[0].id", "value": 1},
    {"type": "REPL", "expression": "a[1].id", "value": 2},
    {"type": "REPL", "expression": "a[2].id", "value": 3},
    {"type": "REPL", "expression": "a[3].id", "value": 4}
  ]
}
```

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var encoded = {obj}.toBase64();`

**Description:**
Serializes the array's entire raw memory block into a Base64-encoded string. Creates a `MemoryBlock` from the contiguous data pointer with the full allocation size (`elementSize * numElements`), then returns the JUCE Base64 encoding.

The encoded string captures all element data in memory layout order. This is a binary snapshot -- it encodes raw bytes, not JSON-style property values. The resulting string can be stored and later restored with `fromBase64()`.

**Parameters:**

(No parameters.)

**Pitfalls:**
- The encoded string is layout-dependent. If the factory prototype changes (different properties, different order, different types), a previously saved Base64 string will have a different size and `fromBase64()` will reject it silently.

**Cross References:**
- `$API.FixObjectArray.fromBase64$`
