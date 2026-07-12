# Threads -- Project Context

## Project Context

### Real-World Use Cases
- **Background thread synchronization**: Plugins that perform heavy operations on background threads (file scanning, preset conversion, sample map reconfiguration) use `Threads` constants as lock identifiers to safely access shared UI or data state from non-UI threads. The constants define the lock vocabulary that BackgroundTask's scoped locking system consumes.
- **Safe audio-state reconfiguration**: Plugins with multiple playback modes (e.g., switching between mic configurations, enabling/disabling effect chains) use the kill-voices-and-call pattern to suspend audio, reconfigure processor states, and resume. This ensures no audio glitches during bulk state changes.

### Complexity Tiers
1. **Constants only** (most common): Use `Threads.Audio`, `Threads.Scripting`, etc. as lock identifiers passed to `BackgroundTask.lock()` for scoped thread-safe access to shared state. No Threads methods called directly.
2. **Thread-safe state modification**: Use `Threads.killVoicesAndCall()` (or `BackgroundTask.killVoicesAndCall()`) to safely reconfigure audio-thread-accessible processor state. Involves `killVoicesAndCall`, `isAudioRunning`.
3. **Diagnostic and profiling**: Use `getCurrentThread`, `isLocked`, `getLockerThread`, `isLockedByCurrentThread` for debugging threading issues. Use `startProfiling` for performance analysis (requires `HISE_INCLUDE_PROFILING_TOOLKIT`).

### Practical Defaults
- Use `BackgroundTask.killVoicesAndCall(fn)` rather than `Threads.killVoicesAndCall(fn)` when the operation is part of a larger background workflow with progress tracking. Use `Threads.killVoicesAndCall(fn)` for simple one-shot state modifications triggered from UI callbacks.
- Use `Threads.Scripting` as the lock type for `BackgroundTask.lock()` when the background thread needs to read or write script-accessible state (component data, arrays, viewport row data).

### Integration Patterns
- `Threads.Scripting` -> `BackgroundTask.lock()` -- Pass the Threads constant to acquire a scoped lock inside a background task, ensuring safe read/write access to shared script state from non-scripting threads.
- `Threads.killVoicesAndCall()` -> processor `setAttribute()` / `setBypassed()` -- The standard pattern for bulk-reconfiguring multiple processors: suspend audio first, then modify bypass states, gain levels, and routing in the callback.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `setBypassed()` on multiple processors from a UI callback without suspending audio | Wrapping bulk processor reconfiguration in `Threads.killVoicesAndCall()` | Changing bypass/attribute state on several processors without suspending audio can cause brief audio glitches as each change takes effect individually while voices are still active. |
