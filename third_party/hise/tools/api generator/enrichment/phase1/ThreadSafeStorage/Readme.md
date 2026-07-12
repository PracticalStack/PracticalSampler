# ThreadSafeStorage -- Class Analysis

## Brief
Lock-based container for safely passing arbitrary data between audio and scripting threads.

## Purpose
ThreadSafeStorage provides a thread-safe wrapper around a single data value (any type: numbers, strings, arrays, objects, JSON). It uses a read-write lock backed by an audio-optimized spin mutex to allow concurrent readers while serializing writes. The class offers both blocking (`load`) and non-blocking (`tryLoad`) read paths, making it suitable for audio-thread consumption where blocking is not acceptable.

## Details

### Lock Semantics

ThreadSafeStorage uses HISE's `SimpleReadWriteLock` with `audio_spin_mutex_shared` as the underlying lock type. This is a lightweight spin-lock optimized for the short critical sections typical of data swaps.

| Operation | Lock Type | Blocks? | Audio-Thread Safe? |
|-----------|-----------|---------|-------------------|
| `store` / `storeWithCopy` / `clear` | Exclusive write (`ScopedMultiWriteLock`) | Yes | No |
| `load` | Shared read (`ScopedReadLock`) | Yes (if writer active) | No |
| `tryLoad` | Try shared read (`ScopedTryReadLock`) | Never | Yes |

### Reference vs Copy Semantics

JUCE `var` uses reference counting for complex types (arrays, objects). This has implications for cross-thread data sharing. See `store()` and `storeWithCopy()` for the full semantics of reference vs deep-copy storage.

### Typical Usage Pattern

1. **UI/scripting thread** writes data using `store()` or `storeWithCopy()`
2. **Audio thread** reads data using `tryLoad(fallbackValue)` -- never blocks
3. **Other threads** read data using `load()` -- blocks until any write completes

Multiple writer threads are supported (the lock uses `ScopedMultiWriteLock`).

## obtainedVia
`Engine.createThreadSafeStorage()`

## minimalObjectToken
tss

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var data = tss.load();` (on audio thread) | `var data = tss.tryLoad(fallbackValue);` | `load()` blocks if a write is in progress. Use `tryLoad()` on the audio thread to avoid blocking. |
| `tss.store(myArray); myArray.push(x);` | `tss.storeWithCopy(myArray); myArray.push(x);` | `store()` shares the reference -- modifying `myArray` after storing creates a race condition. Use `storeWithCopy()` if you intend to keep modifying the original. |

## codeExample
```javascript
// Create a thread-safe storage for passing data to the audio thread
const var tss = Engine.createThreadSafeStorage();

// Store data from the UI thread
tss.store({ "gain": 0.5, "pan": -0.2 });

// Read from audio thread (non-blocking)
const var data = tss.tryLoad({ "gain": 1.0, "pan": 0.0 });
```

## Alternatives
- **Array** -- standard single-thread container, no thread safety
- **UnorderedStack** -- audio-thread-safe insert/remove for floats and events, but not arbitrary data
- **Threads** -- thread identity queries and `killVoicesAndCall()`, not data storage
- **BackgroundTask** -- active background execution with progress; use ThreadSafeStorage to pass results back

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All five methods have straightforward lock-based semantics with no silent failure modes, precondition dependencies, or timeline constraints that warrant parse-time diagnostics.
