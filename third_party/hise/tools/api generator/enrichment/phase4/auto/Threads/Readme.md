<!-- Diagram triage:
  - No diagrams specified in Phase 1 data.
-->

# Threads

Threads provides introspection into HISE's threading model and constants for identifying the four main thread types. Use it to query which thread your code is running on, inspect lock states for debugging, and safely suspend audio processing when you need to reconfigure processor state.

HISE runs four threads simultaneously:

| Constant | Value | Thread |
|----------|-------|--------|
| `Threads.Audio` | 4 | Audio thread - renders audio buffers from the DAW. Highest priority; keeping this unblocked is critical. |
| `Threads.Scripting` | 1 | Scripting thread - executes all non-synchronous script callbacks. |
| `Threads.Loading` | 2 | Loading thread - fetches samples from disk, loads user presets, swaps sample maps. |
| `Threads.UI` | 0 | Message thread - renders the interface and handles mouse/keyboard input. |

Two additional sentinel constants are used when querying lock state:

- `Threads.Unknown` (5) - any unrecognised thread (e.g. a custom background task)
- `Threads.Free` (6) - no thread holds the lock (idle state)

### When scripts run off the Scripting thread

Most script callbacks execute on the Scripting thread, but three exceptions apply:

1. **Non-deferred MIDI callbacks** run on the Audio thread.
2. **Custom LAF paint methods** run on the Message thread.
3. **User preset loading** runs control callbacks on the Loading thread.

These multithreaded callbacks are not synchronised by default. HISE deliberately prefers potential data race conditions over deadlocks and priority inversions. The exception is user preset loading, which locks the Scripting thread by default during the operation.

### Synchronising threads

The `Threads` class itself does not provide locking methods - it only offers constants and query functions. To lock threads, use the `.lock(Threads.xxx)` scoped statement, which guarantees the lock is released when the scope exits even if a script error occurs. This follows the same RAII-style pattern used internally by the engine.

HiseScript provides several other scoped statement primitives that work alongside `.lock()` for debugging and conditional execution within background tasks and timer callbacks:

- `.trace("label")` - adds a named trace event to the profiling timeline
- `.set(variable, value)` - assigns a value within a scoped context
- `.print("message")` - prints a debug message
- `.if(condition):lock(Threads.xxx)` - conditionally acquires a lock

When two threads contend for the same lock, the thread that cannot acquire it stalls until the holder releases it. In practice, a timer callback competing with a tight background-task loop may wait significantly longer than the background task, because the background task leaves almost no idle time between lock operations for the timer to acquire the lock.

> [!Tip:Use constants, not raw integer values] Always use the `Threads` constants rather than raw integer values when identifying threads. The numeric values are an implementation detail and should not be hard-coded.

## Common Mistakes

- **Use killVoicesAndCall for processor changes**
  **Wrong:** Calling `setBypassed()` on multiple processors from a UI callback without suspending audio.
  **Right:** Wrapping bulk processor reconfiguration in `Threads.killVoicesAndCall()`.
  *Changing bypass or attribute state on several processors without suspending audio can cause brief audio glitches as each change takes effect individually while voices are still active.*

- **Capture this context in closure**
  **Wrong:** `Threads.killVoicesAndCall(function() { this.doSomething(); })`
  **Right:** `Threads.killVoicesAndCall(function() { doSomething(); })`
  *The callback takes zero arguments and executes on the Loading thread. The `this` context may not be valid in the deferred execution context.*
