# Array -- Class Analysis

## Brief
Dynamic mixed-type container with JavaScript-style iteration, search, sort, and mutation methods.

## Purpose
Array is the general-purpose dynamic container in HiseScript, holding any combination of numbers, strings, objects, and nested arrays. It provides JavaScript-compatible functional iteration methods (map, filter, find, forEach, every, some), search and sort operations, and stack-like push/pop mutations. Unlike a standard JavaScript Array, some methods behave differently -- notably `concat` modifies in-place and default `sort` works numerically rather than lexicographically.

## Details

### Architecture

Array is a built-in JavaScript engine type backed by `juce::var` with an underlying `juce::Array<var>`. It is NOT an ApiClass subclass -- methods are registered via JUCE's `DynamicObject::setMethod()` mechanism. All array instances share methods through the engine's prototype chain lookup.

### Method Dispatch

Array methods use two dispatch paths:

1. **Standard methods** (`contains`, `push`, `pop`, `indexOf`, etc.) -- resolved through the normal `DynamicObject` prototype chain via `findRootClassProperty`.
2. **Scoped functions** (`find`, `findIndex`, `some`, `map`, `filter`, `forEach`, `every`, `sort`) -- intercepted before prototype lookup because they need access to the JavaScript `Scope` to invoke user-provided callback functions.

The `clone` method is inherited from ObjectClass (not ArrayClass) and calls `juce::var::clone()` for deep copying.

### Functional Iteration (callForEach)

All scoped functional methods share a common iteration engine. The user callback receives up to 3 arguments depending on its declared parameter count:
- `element` -- the current array element
- `index` -- the current index
- `array` -- the array being iterated

Elements that are `undefined` or `void` are silently skipped during iteration.

### Sort Behaviors

See `sort()` and `sortNatural()` for full details and examples.

| Method | Algorithm | Behavior |
|--------|-----------|----------|
| `sort()` (no args) | `juce::Array::sort` with VariantComparator | Numeric only. Strings compare as equal (unsorted). Arrays/objects throw. |
| `sort(fn)` | `std::stable_sort` | Custom comparator. Return negative if a < b. Stable (preserves equal-element order). |
| `sortNatural()` | `std::sort` with `String::compareNatural` | Converts to String, sorts with embedded number awareness ("item2" < "item10"). |

### Key Differences from JavaScript

See individual method entries for full details on each difference.

- `concat` modifies the array in-place (JS returns a new array)
- Default `sort()` sorts numerically (JS sorts lexicographically via toString)
- `indexOf` has a third `typeStrictness` parameter (0 = loose, 1 = strict type+value matching)
- `includes` is an alias for the HISE-native `contains` method
- HISE-specific additions: `pushIfNotAlreadyThere`, `reserve`, `isEmpty`, `removeElement`, `sortNatural`
- Not implemented from JS: `splice`, `reduce`, `flat`, `flatMap`, `fill`, `copyWithin`, `entries`, `values`, `keys`

### The `length` Property

Accessed as `array.length` (read-only). Handled as a special case in the expression evaluator, not as a method. Returns the number of elements.

### Audio Thread Safety

| Category | Methods |
|----------|---------|
| Always warns | `join` (string allocation) |
| Warns if reallocation needed | `push`, `pushIfNotAlreadyThere` |
| Safe (read-only) | `contains`, `includes`, `indexOf`, `lastIndexOf`, `isEmpty`, `isArray` |
| Safe (shrinking) | `pop`, `shift`, `remove`, `removeElement`, `clear` |
| No guard but allocates | `reverse`, `insert`, `concat`, `slice`, `map`, `filter` |

Use `reserve(n)` in onInit to pre-allocate capacity, then `push`/`pop` on the audio thread stays within the allocated buffer.

## obtainedVia
`var a = [];` or `var a = [1, 2, 3];` -- array literal syntax, or `var a = Array.isArray(x) ? x : [x];`

## minimalObjectToken
a

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `a.sort()` on string array | `a.sortNatural()` or `a.sort(function(x,y){ ... })` | Default sort only works on numbers. Strings all compare as equal and remain unsorted. |
| `var b = a.concat([4,5])` | `a.concat([4,5]); var b = a;` | `concat` modifies in-place and returns void, not a new array. Unlike JavaScript. |

## codeExample
```javascript
// Create and populate an array
const a = [3, 1, 4, 1, 5, 9];

// Functional iteration
const doubled = a.map(function(x){ return x * 2; });
const big = a.filter(function(x){ return x > 3; });

// Search
Console.print(a.indexOf(4));    // 2
Console.print(a.contains(9));   // true

// Sort numerically
a.sort();
Console.print(a.join(", "));    // "1, 1, 3, 4, 5, 9"
```

## Alternatives
- **Buffer** -- fixed-size float array for audio sample data with DSP math operations
- **MidiList** -- exactly 128 integer slots indexed by MIDI note number
- **FixObjectArray** -- fixed-size array of typed objects with named properties
- **FixObjectStack** -- LIFO stack of typed objects with insert/remove
- **UnorderedStack** -- fast insert/remove of up to 128 floats or events without order
- **ThreadSafeStorage** -- safe cross-thread data passing

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Array methods are straightforward value operations with no timeline dependencies, no silent-failure preconditions, and no state that could become stale between calls.
