# FixObjectFactory -- Method Analysis

## create

**Signature:** `FixObject create()`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap memory via the internal block allocator (HeapBlock construction).
**Minimal Example:** `var obj = {obj}.create();`

**Description:**
Creates a single FixObject with the layout defined by this factory's prototype. The object's members are initialized to the default values from the prototype description. The factory retains a reference to the created object, preventing garbage collection while the factory exists.

Returns `undefined` if the factory's layout description was invalid (see Pitfalls).

**Parameters:**

(none)

**Pitfalls:**
- [BUG] If the factory was created with an invalid layout description (e.g., containing String or Object values), this method silently returns `undefined` with no error message. The layout validation failure occurs during `Engine.createFixObjectFactory()` but is not reported until a create method is called -- and even then, no error is thrown. Verify the prototype uses only Integer, Float, Boolean values, or fixed-size arrays of these types.

**Cross References:**
- `$API.FixObjectFactory.createArray$`
- `$API.FixObjectFactory.createStack$`
- `$API.FixObjectFactory.getTypeHash$`

## createArray

**Signature:** `FixObjectArray createArray(int numElements)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a contiguous heap block for all elements.
**Minimal Example:** `var arr = {obj}.createArray(16);`

**Description:**
Creates a fixed-size array of FixObjects with the given number of elements. All elements share the factory's layout and are initialized to default values from the prototype. The array inherits the factory's current compare function, used for `sort`, `indexOf`, and `contains` operations. The factory retains a reference to the created array.

Returns `undefined` if the factory's layout description was invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numElements | Integer | no | Number of elements in the array | Must be > 0 |

**Pitfalls:**
- [BUG] If the factory was created with an invalid layout description, this method silently returns `undefined` with no error message. See `create` pitfalls for details.
- The array inherits the compare function set on the factory at creation time. However, calling `setCompareFunction` later also updates this array retroactively -- the inheritance is not a snapshot but a live propagation.

**Cross References:**
- `$API.FixObjectFactory.create$`
- `$API.FixObjectFactory.createStack$`
- `$API.FixObjectFactory.setCompareFunction$`

## setCompareFunction

**Signature:** `void setCompareFunction(var newCompareFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Constructs string objects internally and iterates the container list to propagate the comparator. When set to a JavaScript function, every subsequent comparison invokes a synchronous script callback.
**Minimal Example:** `{obj}.setCompareFunction("score");`

**Description:**
Sets the comparison function used by all arrays and stacks created by this factory for `sort`, `indexOf`, and `contains` operations. Accepts three input modes:

1. **Single property name (String):** A member name from the layout (e.g., `"score"`). Creates an optimized C++ comparator that reads the property's memory directly with no script callback overhead. Supports Integer, Float, and Boolean properties, including array-typed members.

2. **Multi-property (comma-separated String):** Two to four member names separated by commas (e.g., `"category,score"`). Compares properties in priority order -- the first property that differs determines the result. Limited to 2-4 properties; more than 4 produces a script error suggesting a custom function.

3. **Custom function (Function):** A JavaScript function receiving two FixObject arguments that must return -1, 0, or 1 (less than, equal, greater than). Provides full flexibility but incurs synchronous script callback overhead on every comparison.

Passing any other value (e.g., `0`, `false`) resets to the default comparator, which uses byte-level equality for matching and pointer address for ordering.

The new comparator propagates retroactively to all arrays and stacks previously created by this factory.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newCompareFunction | NotUndefined | no | Property name string, comma-separated property names, a comparison function, or any other value to reset | String must reference valid layout member names; comma-separated limited to 2-4 properties |

**Callback Signature:** newCompareFunction(a: FixObject, b: FixObject)

**Pitfalls:**
- Using a JavaScript comparison function makes all sorting and search operations non-realtime-safe due to synchronous script callback overhead (`callSync`) on every comparison. Use the string-based property comparator for performance-critical or audio-thread-adjacent code.
- The comparator propagates to ALL previously created arrays and stacks, not just future ones. This is by design but may be unexpected if different containers need different comparators -- there is no way to set a per-container comparator independently of the factory.

**Cross References:**
- `$API.FixObjectFactory.createArray$`
- `$API.FixObjectFactory.createStack$`

**Example:**
```javascript:compare-function-modes
// Title: Property-based and custom comparison for sorting
const var factory = Engine.createFixObjectFactory({
    "id": 0,
    "score": 0.0
});

var arr = factory.createArray(3);

// Set values for sorting
arr[0].id = 3;
arr[0].score = 1.5;
arr[1].id = 1;
arr[1].score = 3.0;
arr[2].id = 2;
arr[2].score = 0.5;

// Sort by score using optimized property comparator
factory.setCompareFunction("score");
arr.sort();

Console.print(arr[0].score); // 0.5
Console.print(arr[1].score); // 1.5
Console.print(arr[2].score); // 3.0

// Sort by custom function (descending score)
inline function descByScore(a, b)
{
    if (a.score > b.score) return -1;
    if (a.score < b.score) return 1;
    return 0;
};

factory.setCompareFunction(descByScore);
arr.sort();

Console.print(arr[0].score); // 3.0
```

```json:testMetadata:compare-function-modes
{
  "testable": true,
  "verifyScript": [
    {"type": "log-output", "values": ["0.5", "1.5", "3.0", "3.0"]}
  ]
}
```

## getTypeHash

**Signature:** `int getTypeHash()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var hash = {obj}.getTypeHash();`

**Description:**
Returns an integer hash computed from the factory's member names and data types during construction. The hash is derived by concatenating each member's identifier with its type byte, then calling `String::hashCode()`. Two factories with identical layout descriptions (same property names, same types, same order) produce the same hash. Useful for verifying type compatibility between factories or containers at runtime.

**Parameters:**

(none)

**Cross References:**
- `$API.FixObjectFactory.create$`
- `$API.FixObjectFactory.createArray$`
- `$API.FixObjectFactory.createStack$`

## createStack

**Signature:** `FixObjectStack createStack(int numElements)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a contiguous heap block for all elements.
**Minimal Example:** `var stack = {obj}.createStack(32);`

**Description:**
Creates a fixed-capacity stack of FixObjects. A stack extends the fixed array model with insert/remove semantics and a position pointer tracking the number of active elements. The capacity is pre-allocated but only elements up to the current position are considered active. The stack uses a swap-and-pop pattern for removal, and checks for duplicates before insertion via `indexOf`.

The stack inherits the factory's current compare function. The factory retains a reference to the created stack.

Returns `undefined` if the factory's layout description was invalid.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| numElements | Integer | no | Maximum capacity of the stack | Must be > 0 |

**Pitfalls:**
- [BUG] If the factory was created with an invalid layout description, this method silently returns `undefined` with no error message. See `create` pitfalls for details.
- The stack inherits the compare function set on the factory at creation time. Calling `setCompareFunction` later also updates this stack retroactively.

**Cross References:**
- `$API.FixObjectFactory.create$`
- `$API.FixObjectFactory.createArray$`
- `$API.FixObjectFactory.setCompareFunction$`
