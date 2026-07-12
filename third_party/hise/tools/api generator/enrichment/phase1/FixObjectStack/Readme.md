# FixObjectStack -- Class Analysis

## Brief
Variable-occupancy preallocated stack of typed objects with insert, remove, and duplicate detection.

## Purpose
FixObjectStack is a fixed-capacity container for typed objects created by a FixObjectFactory. Unlike FixObjectArray where all slots are always valid, FixObjectStack tracks an internal position pointer that separates used elements from unused capacity. Elements can be dynamically inserted and removed without heap allocation, making it suitable for tracking active voices, events, or any variable-size collection of structured data. Duplicate detection and element lookup use the factory's compare function.

## Details

### Position-Based Occupancy

The stack maintains an internal `position` pointer (initially 0). Elements at indices `[0, position)` are considered "used". The `size()` method returns `position`, not the allocated capacity. The total capacity is available via the `length` constant.

### Insert and Duplicate Detection

`insert()` checks for duplicates using the factory's compare function before adding. If the object already exists, it returns false. The compare function determines what "already exists" means -- with a property-based comparator, only the specified properties are checked. See `insert()` and `set()` for full insertion semantics.

### Removal Strategy (Swap-and-Pop)

`removeElement()` uses a swap-and-pop pattern: it copies the last used element into the removed slot and decrements the position pointer. This means removal does NOT preserve element order. If order matters, use `sort()` after modifications. See `remove()` and `removeElement()` for full removal details.

### Capacity Overflow Behavior

`insert()` clamps the position to `numElements - 1` when full, silently overwriting the last element. `set()` returns false when full and the object is not found. These are different overflow behaviors for the two insertion methods. See `insert()` and `set()` pitfalls for details on the off-by-one capacity issue.

### Inherited Method Scope

Methods inherited from FixObjectArray interact differently with the position pointer:

| Method | Scope |
|--------|-------|
| `indexOf`, `contains` | Used portion only (respects virtual `size()`) |
| `sort` | Used portion only |
| `fill` | All allocated slots (ignores position) |
| `copy` | All allocated slots (ignores position) |
| `toBase64`, `fromBase64` | Full memory block (includes unused slots) |

### set() vs insert()

`set()` is an upsert operation: it replaces an existing match or inserts at the end if not found. `insert()` only adds if no match exists. Both use the compare function for matching.

## obtainedVia
`factory.createStack(numElements)` where `factory` is a FixObjectFactory instance.

## minimalObjectToken
s

## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| length | (constructor arg) | int | The total allocated capacity passed to createStack | Capacity |

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `s.insert(obj); // assumes order preserved after remove` | `s.insert(obj); s.sort();` | removeElement uses swap-and-pop which does not preserve insertion order. Sort after modifications if order matters. |
| `s.copy("value", buf);` | Use a loop from 0 to `s.size()` | copy() reads from all allocated slots including unused ones beyond position, producing stale default values for unused slots. |

## codeExample
```javascript
// Create a factory and stack for tracking active notes
const var f = Engine.createFixObjectFactory({
    "note": 0,
    "velocity": 0.0,
    "active": false
});

f.setCompareFunction("note");
const var s = f.createStack(16);
```

## Alternatives
- **FixObjectArray** -- Use when all slots are always valid (fixed iteration count).
- **Array** -- Use for dynamic mixed-type collections with no fixed capacity.
- **UnorderedStack** -- Use for simple floats or HISE events (up to 128) with built-in event matching.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: insert() silently overwrites the last element at capacity, but this is intentional clamping behavior, not a diagnosable precondition. copy/fill operating on full capacity is documented behavior, not an error condition.
