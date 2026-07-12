# FixObjectArray -- Class Analysis

## Brief
Fixed-size array of typed objects with contiguous memory, bracket indexing, and for-in loop support.

## Purpose
FixObjectArray is a fixed-capacity container of identically-typed objects created by a FixObjectFactory. Each element conforms to the factory's memory layout schema (Integer, Float, Boolean members). The array allocates a single contiguous memory block at creation time with no further allocations, making it suitable for real-time-safe data structures. Elements are accessed via bracket indexing (`arr[i]`), for-in loops, or search methods that use the factory's configurable compare function.

## Details

### Bracket Indexing

FixObjectArray implements `AssignableObject`, enabling direct bracket-index syntax:

- `arr[i]` returns a live reference to the FixObject at index `i` -- modifying the returned object modifies the array's underlying memory.
- `arr[i] = obj` performs a deep copy (memcpy) of the source object's data into slot `i`.

### For-In Loop Support

FixObjectArray is natively supported by HiseScript's `for (x in arr)` construct. The loop always iterates over all `length` elements (the full capacity), unlike FixObjectStack which iterates only over occupied slots. Elements accessed in the loop body are live references.

### Compare Function

Three methods depend on the factory's compare function: `indexOf`, `contains`, and `sort`. The compare function is set via `FixObjectFactory.setCompareFunction()` and automatically propagated to all arrays created from that factory. See `indexOf()`, `contains()`, and `sort()` for per-method details.

### Serialization

`toBase64`/`fromBase64` serialize and restore the raw memory block. See `toBase64()` and `fromBase64()` for size-mismatch behavior and layout-dependency details.

### Copy to Buffer/Array

`copy` extracts a single named property from every element into a target Buffer or Array. See `copy()` for type casting rules and size constraints.

## obtainedVia
`FixObjectFactory.createArray(numElements)`

## minimalObjectToken
a

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| length | (constructor arg) | Integer | Fixed number of elements in the array | Size |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `a.sort()` (without setting compare function) | `f.setCompareFunction("id"); a.sort();` | The default comparator orders by memory address, producing meaningless sort results. Set a property-based or custom compare function on the factory first. |
| `if(a.fromBase64(str)) ...` assuming error on mismatch | Check return value; `fromBase64` returns `false` silently on size mismatch | `fromBase64` does not throw a script error when the decoded data size does not match the array's allocation. Always check the return value. |

## codeExample
```javascript
// Create a factory and array
const var f = Engine.createFixObjectFactory({
    "id": 0,
    "velocity": 0.0,
    "active": false
});

const var a = f.createArray(128);

// Access elements via bracket indexing
a[0].id = 42;
a[0].velocity = 0.8;

// Iterate with for-in
for (obj in a)
    obj.active = true;
```

## Alternatives
- `Array` -- dynamic mixed-type collections without typed schema
- `FixObjectStack` -- variable-occupancy container with insert/remove within fixed capacity
- `UnorderedStack` -- fast insert/remove of up to 128 floats or events without named properties
- `Buffer` -- homogeneous float array for audio sample data

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All error conditions (unknown property in `copy`, buffer size mismatch) produce immediate script errors or return boolean results. No silent-failure preconditions warrant parse-time diagnostics.
