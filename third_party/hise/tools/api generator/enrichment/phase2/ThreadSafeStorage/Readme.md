# ThreadSafeStorage -- Project Context

## Project Context

### Real-World Use Cases
- **LAF paint-to-event bridge**: A data browser built with ScriptedViewport needs to show a tooltip or label with the currently hovered row's text. The LAF `drawTableCell` callback knows what is being hovered but cannot modify component state during painting. A ThreadSafeStorage acts as a mailbox: the paint callback writes the hovered text, and a Broadcaster mouse-event listener reads it to update a visible label. This pattern applies whenever a LAF callback needs to communicate data to logic outside the paint routine.
- **Audio-thread parameter snapshot**: A plugin that computes complex parameter sets on the UI thread (e.g., building a filter coefficient array or a gain map from user input) stores the result in a ThreadSafeStorage. The audio callback reads it non-blockingly via `tryLoad()` with a safe fallback, ensuring the audio thread never stalls waiting for UI computation to finish.

### Complexity Tiers
1. **Simple value passing** (most common): `store()` and `load()` for passing primitive values or small objects between callbacks. No need for `storeWithCopy()` or `tryLoad()`.
2. **Audio-thread consumption**: Adds `tryLoad()` with an appropriate fallback value for non-blocking reads from the audio callback. Requires understanding that the fallback must be a valid default, not just a sentinel.
3. **Mutable source data**: Adds `storeWithCopy()` when the writer continues to modify the original data after storing (e.g., building an array incrementally across multiple callbacks before the reader consumes it).

### Practical Defaults
- Use `store()` by default. Only use `storeWithCopy()` when the caller will continue to mutate the stored object after the call.
- Use `tryLoad()` for any read that might run on the audio thread. Use `load()` only when you are certain the reader is on the UI/scripting thread.
- Choose the fallback value for `tryLoad()` to be a valid operational default (e.g., `0.0` for gain, `[]` for a data list), not a sentinel like `-1` that requires special handling downstream.

### Integration Patterns
- `ScriptLookAndFeel` paint callback → `ThreadSafeStorage.store()` → `Broadcaster` listener calls `ThreadSafeStorage.load()` -- bridges data out of paint routines that cannot modify component state.
- UI callback → `ThreadSafeStorage.store()` → audio `onControl` / timer → `ThreadSafeStorage.tryLoad()` -- passes precomputed parameter snapshots to the audio thread without blocking.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Reading ThreadSafeStorage inside a LAF paint callback via `load()` | Store in the paint callback, read from a Broadcaster listener or timer | LAF paint callbacks should be fast and side-effect-free. Use ThreadSafeStorage as a write-only mailbox from paint routines, reading from a separate event-driven context. |
