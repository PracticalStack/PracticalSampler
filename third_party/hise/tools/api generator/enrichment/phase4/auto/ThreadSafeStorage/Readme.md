<!-- Diagram triage:
  - (no diagrams specified in Phase 1 data)
-->

# ThreadSafeStorage

ThreadSafeStorage is a lock-based container for safely passing data between threads. It holds a single value of any type (numbers, strings, arrays, objects) and provides both blocking and non-blocking read paths.

A typical workflow has three steps:

1. The UI or scripting thread writes data using `store()` or `storeWithCopy()`.
2. The audio thread reads data using `tryLoad()` with a fallback value - this never blocks.
3. Other threads read data using `load()`, which waits for any in-progress write to finish.

```js
const var tss = Engine.createThreadSafeStorage();
```

The two most common integration patterns are bridging data out of a LAF paint callback (where you cannot modify component state directly) and passing precomputed parameter snapshots to the audio thread without stalling it.

> ThreadSafeStorage uses reference semantics by default. When you `store()` an array or object, the storage holds a reference to the same data - mutations to the original also affect the stored copy. Use `storeWithCopy()` to break this sharing when you intend to keep modifying the source data after storing.

## Common Mistakes

- **Use tryLoad on the audio thread**
  **Wrong:** `var data = tss.load();` on the audio thread
  **Right:** `var data = tss.tryLoad(fallbackValue);` on the audio thread
  *`load()` blocks if a write is in progress. On the audio thread this causes glitches or dropouts. `tryLoad()` returns the fallback immediately instead of waiting.*

- **Use storeWithCopy when mutating after store**
  **Wrong:** `tss.store(myArray); myArray.push(x);`
  **Right:** `tss.storeWithCopy(myArray); myArray.push(x);`
  *`store()` shares the reference - modifying `myArray` afterwards creates a race condition with readers on other threads.*

- **Write from paint, read from events**
  **Wrong:** Reading ThreadSafeStorage inside a LAF paint callback via `load()`
  **Right:** Store data in the paint callback, read from a Broadcaster listener or timer
  *LAF paint callbacks should be fast and side-effect-free. Use ThreadSafeStorage as a write-only mailbox from paint routines, reading from a separate event-driven context.*
