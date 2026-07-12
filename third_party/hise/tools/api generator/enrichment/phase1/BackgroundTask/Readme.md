# BackgroundTask -- Class Analysis

## Brief
Task handle for running long operations on a background thread with progress, abort, and process spawning.

## Purpose
BackgroundTask provides a managed background thread for executing long-running HiseScript functions, spawning OS processes, or performing sample-loading-thread operations without blocking the UI or audio threads. It wraps a JUCE Thread with progress reporting (0.0-1.0), status messages, abort signaling, thread-safe key-value property storage, and a finish callback that fires when the task completes or is cancelled. Created via `Engine.createBackgroundTask()`, each instance is a dedicated named thread that automatically stops on script recompilation.

## Details

### Execution Modes

BackgroundTask supports three distinct execution modes. See `callOnBackgroundThread()`, `runProcess()`, and `killVoicesAndCall()` for full details on each mode.

| Mode | Method | Thread | Function Args | Finish Callback |
|------|--------|--------|---------------|-----------------|
| Background function | `callOnBackgroundThread(f)` | Dedicated high-priority thread | 1 (the task itself) | Yes |
| OS process | `runProcess(cmd, args, logFn)` | Dedicated high-priority thread | Log: 3 (task, isFinished, data) | Yes |
| Sample loading | `killVoicesAndCall(f)` | Sample loading thread (KillStateHandler) | 0 | No |

### Timeout and Abort Mechanism

See `shouldAbort()` for the dual-purpose abort check and timeout extension mechanism, and `setTimeOut()` for configuring the timeout value (default 500 ms). In the HISE IDE, a warning is logged if the gap between consecutive `shouldAbort()` calls exceeds the timeout.

### Finish Callback Protocol

The finish callback set via `setFinishCallback()` receives two boolean arguments: `(isFinished, wasCancelled)`.

- Called with `(false, false)` when `callOnBackgroundThread()` or `runProcess()` starts (task beginning)
- Called with `(true, false)` when the task completes normally
- Called with `(true, true)` when the task was aborted via `sendAbortSignal()` or recompilation

### Thread-Safe Property Storage

See `setProperty()` and `getProperty()` for the built-in read-write-locked key-value store that allows the background thread to publish intermediate results for UI polling.

### Loading Thread Forwarding

See `setForwardStatusToLoadingThread()` for enabling `setProgress()` and `setStatusMessage()` forwarding to HISE's sample loading overlay system.

### OS Process Execution

See `runProcess()` for full details on spawning child processes. Output is streamed line by line to a log callback; the process respects `shouldAbort()` and is killed on abort.

### Recompilation Safety

BackgroundTask registers a pre-compile listener that calls `sendAbortSignal(true)` on recompilation, ensuring no background thread survives past a script recompile.

## obtainedVia
`Engine.createBackgroundTask(name)`

## minimalObjectToken
bt

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| Long loop without `shouldAbort()` | Call `task.shouldAbort()` regularly in loops | Without shouldAbort() calls, the script engine timeout is not extended and the task cannot be cancelled. The HISE IDE warns if the gap between checks exceeds the timeout. |
| `task.sendAbortSignal(true)` from inside the background function | Use `task.sendAbortSignal(false)` or just return from the function | Calling sendAbortSignal with blocking=true from the background thread itself causes a deadlock (thread waiting for itself to stop). This is detected and throws a script error. |

## codeExample
```javascript
// Create a background task and run a function on it
const var bt = Engine.createBackgroundTask("MyTask");

bt.setFinishCallback(function(isFinished, wasCancelled)
{
    if (isFinished && !wasCancelled)
        Console.print("Task completed");
});

bt.callOnBackgroundThread(function(task)
{
    for (var i = 0; i < 100; i++)
    {
        if (task.shouldAbort())
            return;

        task.setProgress(i / 100.0);
        task.setStatusMessage("Processing " + i);
    }
});
```

## Alternatives
- **Timer** -- fires a periodic callback on the message thread for UI polling, not background execution
- **Threads** -- utility namespace for querying thread identity and lock state, not a task handle
- **ThreadSafeStorage** -- passive container for cross-thread data sharing; use alongside BackgroundTask to pass structured results

## Related Preprocessors
- `PERFETTO` -- enables Perfetto tracing for thread profiling and abort check counters
- `USE_BACKEND` -- enables timeout gap warnings in shouldAbort() and error logging in run()

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: The only forced-type method (setFinishCallback) already has a callback diagnostic registered via ADD_CALLBACK_DIAGNOSTIC. The shouldAbort() timeout gap warning is a runtime backend-only check, not a parse-time diagnostic. No additional parse-time diagnostics are warranted.
