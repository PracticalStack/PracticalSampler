<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# BackgroundTask

HiseScript serialises all callbacks onto a single scripting thread, with a priority system that ensures control callbacks execute before paint routines and recompilation discards pending callbacks. This avoids race conditions but means a long-running task can block the UI. BackgroundTask offloads heavyweight work to a dedicated thread, keeping the scripting thread responsive.

BackgroundTask supports three execution modes:

| Mode | Method | Use case |
|------|--------|----------|
| Background function | `callOnBackgroundThread()` | General-purpose work with progress and abort |
| OS process | `runProcess()` | Spawning command-line tools with streamed output |
| Voice-safe loading | `killVoicesAndCall()` | Reconfiguring processors that need audio silence |

Create an instance with `Engine.createBackgroundTask()`, configure it (timeout, finish callback, loading overlay), then launch work with one of the three methods. A single task instance should be reused across multiple operations - the previous operation is automatically stopped before a new one starts.

Progress reporting uses `setProgress()` and `setStatusMessage()`, which can optionally forward to HISE's built-in loading overlay via `setForwardStatusToLoadingThread()`. The thread-safe property store lets the background thread publish intermediate results for UI polling.

```js
const var bt = Engine.createBackgroundTask("MyTask");
```

> [!Tip:Experimental API with dedicated OS thread] BackgroundTask is highly experimental - test your use case thoroughly, as subtle multithreading issues can arise. Each instance is a dedicated OS thread that is automatically stopped on script recompilation.

## Common Mistakes

- **Call shouldAbort regularly in loops**
  **Wrong:** Long loop without `shouldAbort()`
  **Right:** Call `task.shouldAbort()` regularly in loops
  *Without `shouldAbort()` calls, the script engine timeout is not extended and the task cannot be cancelled. The HISE IDE warns if the gap between checks exceeds the timeout.*

- **Avoid sendAbortSignal from background thread**
  **Wrong:** `task.sendAbortSignal(true)` from inside the background function
  **Right:** Use `task.sendAbortSignal(false)` or return from the function
  *Calling `sendAbortSignal` with blocking from the background thread causes a deadlock (the thread waits for itself to stop). This is detected and throws a script error.*

- **Reuse task instance per subsystem**
  **Wrong:** Creating a new BackgroundTask for every operation
  **Right:** Reuse a single task instance per subsystem
  *Each BackgroundTask is a dedicated OS thread. Creating many wastes resources. Reuse one instance and let the abort-restart pattern handle cancellation of previous work.*

- **Use killVoicesAndCall for processor changes**
  **Wrong:** Using `callOnBackgroundThread()` to change processor state (bypass, attributes, routing)
  **Right:** Use `killVoicesAndCall()` for processor reconfiguration
  *`callOnBackgroundThread()` runs on a generic background thread without voice protection. Modifying audio-thread-owned state causes glitches. `killVoicesAndCall()` suspends audio output first.*

- **Increase timeout for I/O operations**
  **Wrong:** Default 500ms timeout for I/O-heavy tasks
  **Right:** Use `setTimeOut(2000)` or higher for file scanning or process execution
  *File system operations and child processes naturally have longer gaps between `shouldAbort()` calls. A short timeout triggers spurious warnings and risks premature thread termination.*
