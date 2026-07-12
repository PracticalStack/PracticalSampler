<!-- Diagram triage:
  - No diagrams specified in Phase 1 data (class-level diagrams[] is empty, no method-level diagram fields)
-->

# Array

Array is the general-purpose dynamic container in HiseScript, holding any combination of numbers, strings, objects, and nested arrays. It provides JavaScript-compatible functional methods (`map`, `filter`, `find`, `every`, `some`), search and sort operations, and stack-like `push`/`pop` mutations.

```js
const var a = [];         // Declare an empty array
a[0] = 12;                // Set element at position 0
a[1] = "Hello";           // Mixed types are allowed
Console.print(a.length);  // 2
```

Declaring an array as `const var` prevents reassignment of the variable, but the array contents can still be modified freely - `push`, `pop`, `sort`, and index assignment all work on a `const var` array. The constness applies to the binding, not the data.

Arrays are reference-counted. Assigning an array to another variable creates a shared reference, not a copy:

```js
const var a = [1, 2, 3];
const var b = a;
a[0] = 99;
Console.print(b[0]); // 99
```

Use `clone()` when you need an independent copy.

### Iteration

There are two ways to iterate over an array:

| Style | Syntax | When to use |
|-------|--------|-------------|
| Range-based | `for (x in array)` | Preferred. Clearer syntax and faster. Use when you do not need the index. |
| Index-based | `for (i = 0; i < array.length; i++)` | Use when you need the index, or when iterating multiple arrays in parallel. |

The index-based `for` loop is the only place in HiseScript where anonymous variables are permitted - writing `for (i = 0; ...` does not require a prior `var i` or `reg i` declaration.

On the audio thread, prefer `for (x in array)` over `forEach()`. The range-based loop is allocation-free, while `forEach` allocates scope objects internally.

### Key differences from JavaScript

Several Array methods behave differently from their JavaScript equivalents:

- `concat()` modifies the array in-place and returns `undefined` (JS returns a new array)
- Default `sort()` sorts numerically (JS sorts lexicographically via `toString`)
- `indexOf()` accepts a third `typeStrictness` parameter for strict type matching
- `sortNatural()`, `pushIfNotAlreadyThere()`, `reserve()`, `isEmpty()`, and `removeElement()` are HISE-specific additions
- Not implemented: `splice`, `reduce`, `flat`, `flatMap`, `fill`, `copyWithin`

### Audio thread safety

Arrays modified inside MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`) must be pre-allocated with `reserve()` in `onInit`. Without pre-allocation, any `push` that exceeds the current capacity triggers a reallocation warning and a potential audio glitch. Read-only operations (`contains`, `indexOf`, `isEmpty`) are always safe.

> Arrays are dynamic containers optimised for UI and data processing. For performance-critical realtime work, consider the specialised alternatives: `Buffer` (float audio data), `MidiList` (128 integer slots), `FixObjectArray` (typed fixed-size collections), or `UnorderedStack` (fast unordered insert/remove).

## Common Mistakes

- **Default sort does not work on strings**
  **Wrong:** `a.sort()` on an array of strings
  **Right:** `a.sortNatural()` or `a.sort(function(a, b){ ... })`
  *The default sort compares numerically. String elements all compare as equal and remain in their original order.*

- **concat modifies in-place**
  **Wrong:** `var b = a.concat([4, 5])`
  **Right:** `a.concat([4, 5]);` (modifies `a` directly)
  *Unlike JavaScript, `concat` returns `undefined`. Assigning its result captures `undefined`, not the merged array.*

- **Missing reserve before audio-thread push**
  **Wrong:** Calling `push()` in `onNoteOn` without prior `reserve()`
  **Right:** Call `a.reserve(128)` in `onInit`, then `push`/`pop` in callbacks
  *Each push that exceeds capacity triggers a reallocation warning on the audio thread.*

- **forEach allocates on the audio thread**
  **Wrong:** `a.forEach(function(x){ ... })` inside a MIDI callback
  **Right:** `for (x in a) { ... }`
  *`forEach` allocates scope objects internally. The `for...in` loop is allocation-free.*

- **Skipped elements when removing in a forward loop**
  **Wrong:** `for (i = 0; i < a.length; i++) { if (test) a.removeElement(i); }`
  **Right:** `for (i = 0; i < a.length; i++) { if (test) a.removeElement(i--); }`
  *After removal, elements shift left. Without `i--`, the next element moves to the current index and gets skipped.*
