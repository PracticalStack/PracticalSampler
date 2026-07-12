<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# FixObjectArray

FixObjectArray is a fixed-size container of identically-typed objects created by a `FixObjectFactory`. Every element conforms to the factory's layout schema (Integer, Float, Boolean members), and the entire array is allocated as a single contiguous memory block at creation time with no further allocations.

```js
const var f = Engine.createFixObjectFactory({
    "id": 0,
    "velocity": 0.0,
    "active": false
});

const var a = f.createArray(128);
```

Elements are accessed via bracket indexing or for-in loops. Both return live references - modifying the returned object writes directly into the array's underlying memory. Assigning via `a[i] = obj` performs a deep copy of the source object's data into that slot.

A for-in loop always iterates over all elements (the full capacity), unlike `FixObjectStack` which iterates only over occupied slots. Use FixObjectArray when all slots are always meaningful (e.g. a pool where inactive elements simply have a zeroed property). Use `FixObjectStack` when elements need to be dynamically inserted and removed.

Three methods depend on the factory's compare function: `indexOf()`, `contains()`, and `sort()`. Set the compare function on the factory via `FixObjectFactory.setCompareFunction()` before using any of these - the default byte-level comparator is rarely what you want.

> [!Tip:Fixed length, all slots always valid] The `length` constant is set at creation time and never changes. All elements are valid and iterable at all times - there is no concept of empty or unoccupied slots.

## Common Mistakes

- **Set compare function before sorting**
  **Wrong:** `a.sort()` (without setting a compare function)
  **Right:** `f.setCompareFunction("id"); a.sort();`
  *The default comparator orders by raw memory layout, producing meaningless results. Set a property-based compare function on the factory first.*

- **Check fromBase64 return value**
  **Wrong:** `if (a.fromBase64(str)) ...` assuming an error is thrown on mismatch
  **Right:** Check the return value; `fromBase64()` returns `false` silently on size mismatch.
  *`fromBase64()` does not throw a script error when the decoded data does not match the array's allocation size. Always check the return value.*

- **Match Buffer size to array length**
  **Wrong:** Creating a Buffer of a different size than the array for use with `copy()`
  **Right:** `const var buf = Buffer.create(a.length);`
  *`copy()` requires the Buffer size to exactly match the array's `length` constant. Use the same size for both.*
