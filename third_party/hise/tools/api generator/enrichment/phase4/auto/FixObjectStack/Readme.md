<!-- Diagram triage:
  - (no diagrams specified in Phase 1)
-->

# FixObjectStack

FixObjectStack is a variable-occupancy container for typed objects created by a `FixObjectFactory`. Unlike `FixObjectArray` where all slots are always valid, FixObjectStack tracks how many elements are currently in use, allowing dynamic insertion and removal without heap allocation.

The stack uses a swap-and-pop removal strategy that keeps the data dense at the cost of element ordering:

```
[0, 1, 2, 3]    // initial state
[0, 1, 2, 3, X] // insert X
[0, 1, X, 3]    // remove 2 - last element fills the gap
[0, 1, X, 3, Y] // insert Y at the end
```

Create a stack from a factory instance:

```js
const var f = Engine.createFixObjectFactory({
    "note": 0,
    "velocity": 0.0,
    "active": false
});

f.setCompareFunction("note");
const var s = f.createStack(16);
```

The `length` constant holds the total allocated capacity (passed to `createStack`), while `size()` returns the number of currently used elements.

Some methods operate on the full allocated capacity rather than just the used portion:

| Method scope | Behaviour |
|---|---|
| `indexOf`, `contains`, `sort` | Used elements only (0 to `size() - 1`) |
| `fill`, `copy` | All allocated slots (full `length` capacity) |
| `toBase64`, `fromBase64` | Full memory block including unused slots |

> [!Tip:Compare function controls duplicate detection] Duplicate detection and element lookup use the factory's compare function set via `FixObjectFactory.setCompareFunction()`. Set the compare function before creating the stack or immediately after. Removal does not preserve insertion order - call `sort()` after modifications if ordering matters.

## Common Mistakes

- **Reuse temp object for insertions**
  **Wrong:** Creating a new FixObject for every insert
  **Right:** Reuse a single temp object: mutate its properties, then call `insert()`
  *`insert()` copies the data into the stack's preallocated slot. Creating new objects per event defeats the allocation-free design.*

- **copy reads all slots including unused**
  **Wrong:** `s.copy("value", buf);` and assuming only used elements are copied
  **Right:** Iterate from 0 to `s.size()` to copy only the used portion
  *`copy()` reads from all allocated slots including unused ones beyond the position pointer, which contain default values.*

- **Decrement index after removeElement**
  **Wrong:** `for (i = 0; i < stack.size(); i++) { stack.removeElement(i); }`
  **Right:** `stack.removeElement(i--);` after each removal
  *`removeElement()` swaps the last element into the removed slot. Without decrementing `i`, the swapped-in element is skipped.*

- **Sort after removal if order matters**
  **Wrong:** Relying on element order after `removeElement()` or `remove()`
  **Right:** Call `sort()` after modifications if downstream code depends on ordering
  *Swap-and-pop removal does not preserve insertion order. If you need oldest-first or any specific ordering, sort explicitly.*
