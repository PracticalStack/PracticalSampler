# BackgroundTask -- Project Context

## Project Context

### Real-World Use Cases
- **File scanning and indexing**: A plugin with a large sample library uses BackgroundTask to scan directories, index audio files, and build browsable lists on a background thread -- keeping the UI responsive while thousands of files are enumerated and categorized. The task reports progress via the loading overlay so users see scan status.
- **Mode switching with processor reconfiguration**: A multi-mode instrument (e.g. switching between performance modes that change mic routing, effect bypass states, and sample purge settings across many processors) uses `killVoicesAndCall()` to safely reconfigure the module tree without audio glitches. This is the preferred pattern when the reconfiguration touches audio-thread-owned state.
- **Batch preset conversion**: A development-time workflow uses BackgroundTask to iterate through a preset collection, loading each preset, transforming metadata, and re-saving -- with progress forwarded to the loading overlay and abort checking in the loop.

### Complexity Tiers
1. **Voice-safe loading** (simplest): `killVoicesAndCall()` only. No progress, no abort -- just a safe way to run a function that modifies audio state. Needs `createBackgroundTask()` and one method call.
2. **Background work with progress**: `callOnBackgroundThread()` with `shouldAbort()` and `setProgress()` in a loop. Optionally `setForwardStatusToLoadingThread(true)` to reuse the sample loading overlay. This is the most common tier.
3. **Shared task with abort-restart**: A single BackgroundTask instance shared across multiple UI triggers, using `sendAbortSignal(true)` before each new `callOnBackgroundThread()` call to cancel in-progress work and restart. Requires careful timeout configuration via `setTimeOut()`.

### Practical Defaults
- Use `setForwardStatusToLoadingThread(true)` when the task takes more than a second -- it provides a free progress UI via the built-in loading overlay.
- Use `setTimeOut(4000)` or higher for file scanning tasks that may take several seconds per abort check cycle. The default 500ms is too short for I/O-heavy operations that naturally have longer gaps between `shouldAbort()` calls.
- Create one shared BackgroundTask per subsystem (e.g. one "FileScanner" task for all file browsing operations) rather than creating a new task for each operation. Reuse the same instance and let the abort-restart pattern handle cancellation.

### Integration Patterns
- `BackgroundTask.callOnBackgroundThread()` with `Threads.Scripting` lock blocks -- use `.lock(Threads.Scripting)` inside the background function when calling APIs that require the scripting thread (e.g. `Engine.loadUserPreset()`, component property changes).
- `BackgroundTask.killVoicesAndCall()` for module reconfiguration -- the loading function calls methods like `setBypassed()`, `setAttribute()`, and purge/routing changes that need voice-safe execution.
- `BackgroundTask` + `Broadcaster` -- a broadcaster listener triggers `callOnBackgroundThread()` on the shared task, connecting data change events to background processing (e.g. a preset load event triggers a background rescan).

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new BackgroundTask for every operation | Reuse a single task instance per subsystem | Each BackgroundTask is a dedicated JUCE thread. Creating many wastes resources. Reuse one and let abort-restart handle cancellation of previous work. |
| Using `callOnBackgroundThread` when modifying processor state (bypass, attributes, routing) | Use `killVoicesAndCall` for processor reconfiguration | `callOnBackgroundThread` runs on a generic background thread without voice protection. Changing audio-thread-owned state from there causes glitches. `killVoicesAndCall` suspends audio first. |
| Default 500ms timeout for I/O-heavy tasks | Set `setTimeOut(2000)` or higher for file scanning | File system operations naturally have longer gaps between `shouldAbort()` calls. A short timeout triggers spurious warnings in the HISE IDE and risks premature thread termination. |
