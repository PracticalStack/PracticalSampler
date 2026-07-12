## callOnBackgroundThread

**Signature:** `void callOnBackgroundThread(Function backgroundTaskFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Stops any running thread (blocking up to timeout ms), allocates WeakCallbackHolder, starts new high-priority thread.
**Minimal Example:** `{obj}.callOnBackgroundThread(onBackgroundWork);`

**Description:**
Starts the given function on a dedicated high-priority background thread. The function receives the BackgroundTask itself as its single argument, which it should use to call `shouldAbort()` regularly and `setProgress()` to report progress. If a task is already running, it is stopped first (blocking up to the configured timeout). The finish callback is called with `(false, false)` immediately when this method is invoked (signaling task start), then with `(true, wasCancelled)` when the function returns or is aborted.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| backgroundTaskFunction | Function | no | The function to execute on the background thread | Must accept 1 argument (the BackgroundTask) |

**Callback Signature:** backgroundTaskFunction(task: BackgroundTask)

**Pitfalls:**
- Calling this while a previous task is running blocks the calling thread for up to `timeout` ms while the old thread is stopped. If the old task does not check `shouldAbort()`, this blocks for the full timeout duration before the new task starts.

**Cross References:**
- `$API.BackgroundTask.killVoicesAndCall$`
- `$API.BackgroundTask.shouldAbort$`
- `$API.BackgroundTask.setFinishCallback$`
- `$API.BackgroundTask.setTimeOut$`

**Example:**
```javascript:background-task-basic
// Title: Running a long operation on a background thread
const var bt = Engine.createBackgroundTask("MyTask");

inline function onBackgroundWork(task)
{
    for (var i = 0; i < 100; i++)
    {
        if (task.shouldAbort())
            return;

        task.setProgress(i / 100.0);
        task.setStatusMessage("Step " + i);
        task.setProperty("lastStep", i);
    }
};

bt.callOnBackgroundThread(onBackgroundWork);
```
```json:testMetadata:background-task-basic
{
  "testable": false,
  "skipReason": "Requires background thread execution with timing-dependent completion"
}
```

## getProgress

**Signature:** `double getProgress()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var p = {obj}.getProgress();`

**Description:**
Returns the current progress value set by `setProgress()`. The value is stored as a `std::atomic<double>`, making this safe to call from any thread including the audio thread. Returns 0.0 if no progress has been set.

**Parameters:**
None.

**Cross References:**
- `$API.BackgroundTask.setProgress$`

## getProperty

**Signature:** `var getProperty(String id)`
**Return Type:** `var`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock::ScopedReadLock to read from the synchronized property store.
**Minimal Example:** `var val = {obj}.getProperty("status");`

**Description:**
Returns the value stored under the given key in the thread-safe property store. Returns undefined if the key has not been set. The property store is protected by a read-write lock, allowing concurrent reads from the UI thread while the background thread writes via `setProperty()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | The property key to look up | -- |

**Cross References:**
- `$API.BackgroundTask.setProperty$`

## getStatusMessage

**Signature:** `String getStatusMessage()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock::ScopedReadLock and returns a String (atomic ref-count on StringHolder).
**Minimal Example:** `var msg = {obj}.getStatusMessage();`

**Description:**
Returns the current status message set by `setStatusMessage()`. The message is protected by a read-write lock, allowing the UI thread to safely poll the message while the background thread updates it.

**Parameters:**
None.

**Cross References:**
- `$API.BackgroundTask.setStatusMessage$`

## killVoicesAndCall

**Signature:** `bool killVoicesAndCall(Function loadingFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Stops any running background thread, then uses KillStateHandler to kill voices and execute on the sample loading thread.
**Minimal Example:** `{obj}.killVoicesAndCall(onLoadingWork);`

**Description:**
Kills all active voices and executes the given function on the sample loading thread. Unlike `callOnBackgroundThread()`, the function takes zero arguments and does not have access to the BackgroundTask for progress or abort checking. This method uses HISE's KillStateHandler infrastructure, which suspends audio processing (outputting silence) during execution. Returns true if the operation was successfully queued. Does not trigger the finish callback.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| loadingFunction | Function | no | The function to execute on the sample loading thread | Must accept 0 arguments |

**Callback Signature:** loadingFunction()

**Pitfalls:**
- Does not trigger the finish callback set via `setFinishCallback()`. Use this for operations that need voice-safe execution, not for tasks that report completion through the finish callback protocol.
- The function receives no arguments, unlike `callOnBackgroundThread` where the function receives the task as its argument. There is no way to report progress or check abort from within the loading function.

**Cross References:**
- `$API.BackgroundTask.callOnBackgroundThread$`
- `$API.BackgroundTask.setFinishCallback$`

**Example:**
```javascript:kill-voices-and-load
// Title: Voice-safe loading operation on the sample loading thread
const var bt = Engine.createBackgroundTask("Loader");

inline function onLoadingWork()
{
    // Runs on the sample loading thread with all voices killed.
    // No task argument available -- cannot report progress or check abort.
    Console.print("Loading complete");
};

bt.killVoicesAndCall(onLoadingWork);
```
```json:testMetadata:kill-voices-and-load
{
  "testable": false,
  "skipReason": "Requires KillStateHandler voice-killing infrastructure and sample loading thread"
}
```

## runProcess

**Signature:** `void runProcess(String command, var args, Function logFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Stops any running thread, allocates ChildProcessData, starts a new high-priority thread that spawns an OS child process.
**Minimal Example:** `{obj}.runProcess("python", ["-c", "print('hello')"], onProcessLog);`

**Description:**
Spawns an OS child process on the background thread and streams its output line by line to the log callback. The command and arguments are combined into a single command line. The log function is called on each line of output with `(task, false, lineText)`, and once on completion with `(task, true, exitCode)`. The process respects `shouldAbort()` and is killed if abort is signaled. Both stdout and stderr are captured and merged into a single stream. The finish callback (if set) fires after the process completes, following the same `(false, false)` at start and `(true, wasCancelled)` at end protocol as `callOnBackgroundThread()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| command | String | no | The executable path or command name | -- |
| args | Array | no | Command-line arguments | Array of strings, or a single String (tokenized by spaces respecting quotes) |
| logFunction | Function | no | Callback for process output and completion | Must accept 3 arguments |

**Callback Signature:** logFunction(task: BackgroundTask, isFinished: bool, data: var)

**Pitfalls:**
- The `args` parameter has two parsing modes: an Array passes each element as a separate argument (preserving spaces within elements), while a String is tokenized by spaces (respecting quotes). Use an Array when arguments contain spaces.
- stdout and stderr are merged into a single stream with no way to distinguish between them.
- The `data` parameter changes type depending on `isFinished`: a String (line of text) during output, and an int (exit code) on completion. Check `isFinished` before using `data`.

**Cross References:**
- `$API.BackgroundTask.callOnBackgroundThread$`
- `$API.BackgroundTask.shouldAbort$`
- `$API.BackgroundTask.setFinishCallback$`
- `$API.BackgroundTask.setTimeOut$`

**Example:**
```javascript:run-process-logging
// Title: Spawning a process and logging output
const var bt = Engine.createBackgroundTask("ProcessRunner");

inline function onProcessLog(task, isFinished, data)
{
    if (isFinished)
        Console.print("Process exited with code: " + data);
    else
        Console.print("Output: " + data);
};

bt.runProcess("echo", ["hello", "world"], onProcessLog);
```
```json:testMetadata:run-process-logging
{
  "testable": false,
  "skipReason": "Requires OS process spawning and background thread execution"
}
```

## sendAbortSignal

**Signature:** `void sendAbortSignal(Integer blockUntilStopped)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** When blockUntilStopped is true, calls stopThread() which blocks the calling thread for up to timeout ms and extends the script engine timeout. Non-blocking mode only sets an atomic flag.
**Minimal Example:** `{obj}.sendAbortSignal(false);`

**Description:**
Signals the background thread to stop. When `blockUntilStopped` is false, sets the thread exit flag (non-blocking) so that `shouldAbort()` returns true on the next check. When `blockUntilStopped` is true, blocks the calling thread until the background thread exits or the timeout expires, and extends the script engine timeout during the wait. Does nothing if no thread is currently running.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| blockUntilStopped | Integer | no | If true, blocks until the thread stops or timeout expires | Boolean (0 or 1) |

**Pitfalls:**
- Calling with `blockUntilStopped = true` from within the background task function itself causes a deadlock (thread waiting for itself to stop). This is detected and throws a script error: "Can't stop with blocking on the worker thread".

**Cross References:**
- `$API.BackgroundTask.shouldAbort$`
- `$API.BackgroundTask.setTimeOut$`

## setFinishCallback

**Signature:** `void setFinishCallback(Function newFinishCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates WeakCallbackHolder, sets source tracking properties.
**Minimal Example:** `{obj}.setFinishCallback(onTaskFinished);`

**Description:**
Sets the callback that fires when a background task starts and finishes. The callback receives two boolean arguments: `isFinished` and `wasCancelled`. It is called with `(false, false)` when `callOnBackgroundThread()` or `runProcess()` starts, `(true, false)` on normal completion, and `(true, true)` when the task was aborted. The BackgroundTask is available as `this` inside the callback. Not triggered by `killVoicesAndCall()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newFinishCallback | Function | yes | The finish callback function | Must accept 2 arguments |

**Callback Signature:** newFinishCallback(isFinished: bool, wasCancelled: bool)

**Pitfalls:**
- Not triggered by `killVoicesAndCall()` -- only by `callOnBackgroundThread()` and `runProcess()`.

**Cross References:**
- `$API.BackgroundTask.callOnBackgroundThread$`
- `$API.BackgroundTask.runProcess$`
- `$API.BackgroundTask.killVoicesAndCall$`

**Example:**
```javascript:finish-callback-protocol
// Title: Finish callback showing start/complete/cancel states
const var bt = Engine.createBackgroundTask("StatusTask");

inline function onTaskFinished(isFinished, wasCancelled)
{
    if (!isFinished)
        Console.print("Task started");
    else if (wasCancelled)
        Console.print("Task was cancelled");
    else
        Console.print("Task completed successfully");
};

bt.setFinishCallback(onTaskFinished);
```
```json:testMetadata:finish-callback-protocol
{
  "testable": false,
  "skipReason": "Finish callback requires background thread lifecycle events to fire"
}
```

## setForwardStatusToLoadingThread

**Signature:** `void setForwardStatusToLoadingThread(Integer enabled)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setForwardStatusToLoadingThread(true);`

**Description:**
When enabled, calls to `setProgress()` and `setStatusMessage()` additionally update HISE's built-in sample loading overlay system. This displays the background task's progress and status using the same UI that appears during sample loading. Set this before starting the task with `callOnBackgroundThread()` or `runProcess()`. The loading overlay flag is automatically set when the background thread starts and cleared when it finishes.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| enabled | Integer | no | Whether to forward progress and status to the loading overlay | Boolean (0 or 1) |

**Cross References:**
- `$API.BackgroundTask.setProgress$`
- `$API.BackgroundTask.setStatusMessage$`

## setProgress

**Signature:** `void setProgress(Double p)`
**Return Type:** `undefined`
**Call Scope:** safe
**Call Scope Note:** Writes to std::atomic<double> (lock-free). When loading thread forwarding is enabled, additionally updates the SampleManager preload progress.
**Minimal Example:** `{obj}.setProgress(0.5);`

**Description:**
Sets the progress value for this task, clamped to the range 0.0 to 1.0. The value is stored as an atomic double and can be polled from any thread via `getProgress()`. When loading thread forwarding is enabled via `setForwardStatusToLoadingThread(true)`, the value is also forwarded to HISE's sample loading overlay.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| p | Double | no | Progress value | 0.0-1.0 (clamped) |

**Cross References:**
- `$API.BackgroundTask.getProgress$`
- `$API.BackgroundTask.setForwardStatusToLoadingThread$`

## setProperty

**Signature:** `void setProperty(String id, var value)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock::ScopedWriteLock to write to the synchronized property store. Also constructs a juce::Identifier from the string key.
**Minimal Example:** `{obj}.setProperty("result", 42);`

**Description:**
Stores a key-value pair in the thread-safe property store. The store is protected by a read-write lock, allowing the background thread to publish intermediate results that the UI thread can poll via `getProperty()`. Any value type can be stored.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| id | String | no | The property key | -- |
| value | NotUndefined | no | The value to store | Any type |

**Cross References:**
- `$API.BackgroundTask.getProperty$`

## setStatusMessage

**Signature:** `void setStatusMessage(String m)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Acquires SimpleReadWriteLock::ScopedWriteLock and performs String assignment.
**Minimal Example:** `{obj}.setStatusMessage("Loading samples...");`

**Description:**
Sets the status message for this task. The message is protected by a read-write lock and can be polled from any thread via `getStatusMessage()`. When loading thread forwarding is enabled via `setForwardStatusToLoadingThread(true)`, the message is also forwarded to HISE's sample loading overlay.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| m | String | no | The status message text | -- |

**Cross References:**
- `$API.BackgroundTask.getStatusMessage$`
- `$API.BackgroundTask.setForwardStatusToLoadingThread$`

## setTimeOut

**Signature:** `void setTimeOut(Integer newTimeout)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setTimeOut(1000);`

**Description:**
Sets the timeout in milliseconds used for thread stop operations. This value affects: the `sendAbortSignal(true)` blocking duration, the `stopThread()` wait when starting a new task over an existing one, and the interval added to the script engine timeout by each `shouldAbort()` call. The default is 500 ms.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| newTimeout | Integer | no | Timeout in milliseconds | Default: 500 |

**Cross References:**
- `$API.BackgroundTask.shouldAbort$`
- `$API.BackgroundTask.sendAbortSignal$`

## shouldAbort

**Signature:** `bool shouldAbort()`
**Return Type:** `Integer`
**Call Scope:** safe
**Call Scope Note:** In exported plugins: checks an atomic bool and extends script timeout (both lock-free). In the HISE IDE, additionally performs String-based warning logging, but backend-only code is excluded from callScope classification per the compiled-out rule.
**Minimal Example:** `var abort = {obj}.shouldAbort();`

**Description:**
Checks whether the background thread has been signaled to stop and extends the script engine timeout. Returns true if `sendAbortSignal()` was called or a script recompilation was triggered. Each call extends the script engine's execution timeout by `timeout + 10` ms, preventing the watchdog from killing the script during long-running background operations. In the HISE IDE, a warning is logged if the gap between consecutive calls exceeds the configured timeout. Call this regularly in loops within the background task function.

**Parameters:**
None.

**Pitfalls:**
- Failing to call this regularly in a background task loop has two consequences: the task cannot be cancelled via `sendAbortSignal()`, and the script engine timeout is not extended, risking watchdog termination of the script.

**Cross References:**
- `$API.BackgroundTask.sendAbortSignal$`
- `$API.BackgroundTask.setTimeOut$`
