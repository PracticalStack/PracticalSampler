# BackgroundTask -- C++ Source Exploration

## Resources Consulted

- `enrichment/resources/survey/class_survey.md` -- No prerequisites listed for BackgroundTask
- `enrichment/resources/survey/class_survey_data.json` -- BackgroundTask entry (lines 134-163)
- No prerequisite Readmes needed
- No base class explorations needed (not a component class)

## Source Locations

- **Header:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.h` lines 556-710
- **Implementation:** `HISE/hi_scripting/scripting/api/ScriptingApiObjects.cpp` lines 7716-8160
- **Factory:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp` line 2569-2572

## Class Declaration

```cpp
struct ScriptBackgroundTask : public ConstScriptingObject,
                              public Thread
{
    // ...
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("BackgroundTask"); }
    // ...
};
```

**Inheritance:**
- `ConstScriptingObject` -- standard HISE scripting API base (provides `getScriptProcessor()`, `reportScriptError()`, method registration macros, debug popup infrastructure)
- `juce::Thread` -- JUCE thread class. The BackgroundTask IS a thread, not merely a wrapper. It inherits `startThread()`, `stopThread()`, `signalThreadShouldExit()`, `threadShouldExit()`, `isThreadRunning()`, `wait()`, `getThreadName()`, `getCurrentThread()`, etc.

## Factory / obtainedVia

Created via `Engine.createBackgroundTask(name)`:

```cpp
var ScriptingApi::Engine::createBackgroundTask(String name)
{
    return new ScriptingObjects::ScriptBackgroundTask(getScriptProcessor(), name);
}
```

The `name` parameter becomes the JUCE thread name (used for Perfetto profiling, debug display, abort check counter naming).

## Constructor Analysis

```cpp
ScriptBackgroundTask(ProcessorWithScriptingContent* p, const String& name) :
    ConstScriptingObject(p, 0),         // 0 constants
    Thread(name),                        // JUCE thread with given name
    currentTask(p, this, var(), 1),      // WeakCallbackHolder for 1-arg callback
    finishCallback(p, this, var(), 2),   // WeakCallbackHolder for 2-arg callback
    recordingSession(new ProfiledRecordingSession(...))
```

**No constants:** Constructor passes `0` to `ConstScriptingObject`, meaning no `addConstant()` calls.

**Pre-compile listener registration:**
```cpp
dynamic_cast<JavascriptProcessor*>(p)->getScriptEngine()->preCompileListeners.addListener(
    *this, recompiled, false);
```
This ensures that when the script is recompiled, the static `recompiled()` function is called, which calls `sendAbortSignal(true)` -- stopping any running background thread before recompilation proceeds.

**Method registrations (all ADD_API_METHOD_N, untyped except one):**

| Registration | Method |
|---|---|
| `ADD_API_METHOD_1` | sendAbortSignal |
| `ADD_API_METHOD_0` | shouldAbort |
| `ADD_API_METHOD_2` | setProperty |
| `ADD_API_METHOD_1` | getProperty |
| `ADD_API_METHOD_3` | runProcess |
| `ADD_TYPED_API_METHOD_1` | setFinishCallback (VarTypeChecker::Function) |
| `ADD_API_METHOD_1` | callOnBackgroundThread |
| `ADD_API_METHOD_1` | killVoicesAndCall |
| `ADD_API_METHOD_0` | getProgress |
| `ADD_API_METHOD_1` | setProgress |
| `ADD_API_METHOD_1` | setTimeOut |
| `ADD_API_METHOD_1` | setStatusMessage |
| `ADD_API_METHOD_0` | getStatusMessage |
| `ADD_API_METHOD_1` | setForwardStatusToLoadingThread |

**Only one typed method:** `setFinishCallback` with `VarTypeChecker::Function`.

**Callback diagnostic:** `ADD_CALLBACK_DIAGNOSTIC(finishCallback, setFinishCallback, 0)` -- registers the finishCallback WeakCallbackHolder for diagnostic tracking.

## Wrapper Struct (API Method Wrappers)

```cpp
struct Wrapper
{
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, sendAbortSignal);
    API_METHOD_WRAPPER_0(ScriptBackgroundTask, shouldAbort);
    API_VOID_METHOD_WRAPPER_2(ScriptBackgroundTask, setProperty);
    API_METHOD_WRAPPER_1(ScriptBackgroundTask, getProperty);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setFinishCallback);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, callOnBackgroundThread);
    API_METHOD_WRAPPER_1(ScriptBackgroundTask, killVoicesAndCall);
    API_METHOD_WRAPPER_0(ScriptBackgroundTask, getProgress);
    API_VOID_METHOD_WRAPPER_3(ScriptBackgroundTask, runProcess);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setProgress);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setTimeOut);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setStatusMessage);
    API_METHOD_WRAPPER_0(ScriptBackgroundTask, getStatusMessage);
    API_VOID_METHOD_WRAPPER_1(ScriptBackgroundTask, setForwardStatusToLoadingThread);
};
```

## Private Member Variables

| Member | Type | Purpose |
|---|---|---|
| `recordingSession` | `ScopedPointer<ProfiledRecordingSession>` | Perfetto/debug profiling session for worker thread |
| `forwardToLoadingThread` | `bool` (default false) | When true, progress/status forwarded to sample loading notification |
| `progress` | `std::atomic<double>` (default 0.0) | Thread-safe progress value, clamped 0.0-1.0 |
| `message` | `String` | Status message, guarded by lock |
| `timeOut` | `int` (default 500) | Thread stop timeout in ms |
| `lastAbortCheck` | `Time` | Timestamp of last shouldAbort() call (backend only) |
| `lock` | `SimpleReadWriteLock` | Protects message and synchronisedData |
| `synchronisedData` | `NamedValueSet` | Thread-safe key-value storage |
| `currentTask` | `WeakCallbackHolder` | The background function (1 arg: task reference) |
| `finishCallback` | `WeakCallbackHolder` | Finish callback (2 args: isFinished, wasCancelled) |
| `abortId` | `Identifier` | Perfetto counter track name |
| `numAbortChecks` | `int` (default 0) | Perfetto counter for abort checks |
| `childProcessData` | `ScopedPointer<ChildProcessData>` | OS process data when runProcess is active |
| `realtimeSafe` | `bool` (default true) | Unused/legacy flag |

## Thread-Safe Property Storage

The `setProperty`/`getProperty` methods use a `NamedValueSet` (JUCE key-value map) protected by a `SimpleReadWriteLock`:

```cpp
void setProperty(String id, var value) {
    auto i = Identifier(id);
    SimpleReadWriteLock::ScopedWriteLock sl(lock);
    synchronisedData.set(i, value);
}

var getProperty(String id) {
    auto i = Identifier(id);
    SimpleReadWriteLock::ScopedReadLock sl(lock);
    return synchronisedData.getWithDefault(i, var());
}
```

The same lock also protects `message` (status message). This means getStatusMessage and getProperty can be called from the message thread while the background thread sets values, with thread-safe access.

Note: `SimpleReadWriteLock` is HISE's custom read-write lock (not std::shared_mutex). It is non-blocking for readers when no writer is active.

## WeakCallbackHolder Pattern

Both `currentTask` and `finishCallback` use `WeakCallbackHolder`, which safely calls HiseScript functions from non-scripting threads:

- `currentTask` is constructed with arg count 1 (the task itself is passed as argument)
- `finishCallback` is constructed with arg count 2 (isFinished: bool, wasCancelled: bool)

The `callSync()` method on WeakCallbackHolder executes the function synchronously on the calling thread but with proper script engine locking.

Key detail: `currentTask.addAsSource(this, "backgroundFunction")` and `finishCallback.addAsSource(this, "onTaskFinished")` register these callbacks in the source tracking system for debugging.

## Thread Lifecycle (run() method)

```cpp
void run() override
{
    // Perfetto profiling setup (PERFETTO guard)
    // ProfiledRecordingSession init

    if (currentTask || childProcessData)
    {
        if (forwardToLoadingThread)
            getScriptProcessor()->getMainController_()->getSampleManager().setPreloadFlag();

        if (childProcessData != nullptr)
        {
            childProcessData->run();
            childProcessData = nullptr;
        }
        else
        {
            var t(this);
            auto r = currentTask.callSync(&t, 1);
            // Backend-only error logging
        }

        currentTask.clear();

        if (forwardToLoadingThread)
            getScriptProcessor()->getMainController_()->getSampleManager().clearPreloadFlag();
    }

    callFinishCallback(true, threadShouldExit());
}
```

Key observations:
1. The task function receives the BackgroundTask itself as its argument (`var t(this)`)
2. `callFinishCallback(true, threadShouldExit())` is always called at the end -- `isFinished=true`, `wasCancelled` is true only if abort was signaled
3. When `forwardToLoadingThread` is enabled, the sample manager's preload flag is set/cleared, making the loading overlay appear
4. Either `currentTask` (script function) or `childProcessData` (OS process) runs, never both

## callOnBackgroundThread Flow

```cpp
void callOnBackgroundThread(var backgroundTaskFunction)
{
    if (HiseJavascriptEngine::isJavascriptFunction(backgroundTaskFunction))
    {
        callFinishCallback(false, false);    // Signal "starting" to finish callback
        stopThread(timeOut);                  // Stop any existing thread

        childProcessData = nullptr;           // Clear any child process

        currentTask = WeakCallbackHolder(getScriptProcessor(), this, backgroundTaskFunction, 1);
        currentTask.incRefCount();
        currentTask.addAsSource(this, "backgroundFunction");
        ThreadStarters::startHigh(this);      // Start thread at high priority
    }
}
```

The finish callback is called with `(false, false)` at START (isFinished=false, wasCancelled=false), then `(true, wasCancelled)` at END. This allows UI code to show/hide a loading state.

## killVoicesAndCall Flow

```cpp
bool killVoicesAndCall(var loadingFunction)
{
    // Validates function, stops existing thread
    currentTask = WeakCallbackHolder(getScriptProcessor(), this, loadingFunction, 0);
    // Note: 0 args -- the function takes no parameters

    WeakReference<ScriptBackgroundTask> safeThis(this);

    auto f = [safeThis](Processor* p)
    {
        if (safeThis != nullptr)
        {
            auto r = safeThis->currentTask.callSync(nullptr, 0, nullptr);
            safeThis->currentTask.clear();
            if (!r.wasOk()) debugError(p, r.getErrorMessage());
        }
        return SafeFunctionCall::OK;
    };

    return getScriptProcessor()->getMainController_()->getKillStateHandler()
        .killVoicesAndCall(dynamic_cast<Processor*>(getScriptProcessor()), f,
            MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}
```

Key differences from callOnBackgroundThread:
- Does NOT start a juce::Thread -- instead uses KillStateHandler infrastructure
- The function takes 0 arguments (not 1)
- Runs on the sample loading thread, not a dedicated background thread
- Returns bool (success of killVoicesAndCall)
- Does NOT trigger finish callback
- Uses WeakReference for safety

## ChildProcessData (Inner Class)

Handles OS process execution for `runProcess()`:

```cpp
struct ChildProcessData
{
    ChildProcessData(ScriptBackgroundTask& parent_, const String& command_,
                     const var& args_, const var& pf);
    void run();

private:
    void callLog(var* a);

    ScriptBackgroundTask& parent;
    juce::ChildProcess childProcess;
    WeakCallbackHolder processLogFunction;
    StringArray args;
};
```

**Constructor** builds the command line:
- First element is the command itself
- If `args_` is an Array, each element is added as a string
- If `args_` is a String, it's tokenized by spaces (respecting quotes)
- Empty strings are removed, all strings are trimmed
- The processLogFunction WeakCallbackHolder has 3 args and is set to high priority

**run() method:**
- Starts the child process with both stdout and stderr capture
- Reads output character by character
- On each newline, calls the log function with 3 args: `(task, isFinished=false, lineText)`
- Checks `parent.shouldAbort()` in the loop -- kills process if abort signaled
- Waits 1ms between character reads, 10ms after each line
- After process exits, reads remaining output and sends final call with `(task, isFinished=true, exitCode)`

**Log function signature:** `function(task, isFinished, data)` where:
- `task` = the BackgroundTask reference
- `isFinished` = false during output, true on completion
- `data` = line of text (String) during output, exit code (int) on completion

## shouldAbort() -- Timeout Extension Pattern

```cpp
bool shouldAbort()
{
    // Backend-only: warns if time between abort checks exceeds timeout
    #if USE_BACKEND
    auto delta = now.getMilliseconds() - lastAbortCheck.getMilliseconds();
    if (delta > timeOut)
        // WARNING logged to console with goto link to source location
    #endif

    if (auto engine = ...) {
        engine->extendTimeout(timeOut + 10);   // Keep extending script timeout
    } else {
        signalThreadShouldExit();              // No engine = force exit
    }

    return threadShouldExit();
}
```

Critical pattern: Each call to `shouldAbort()` extends the script engine timeout by `timeOut + 10` ms. This prevents the script engine watchdog from killing the script while a background task is legitimately running. If the script engine is gone (recompilation), it signals thread exit.

The backend warning checks that the gap between consecutive shouldAbort() calls doesn't exceed the timeout -- if it does, the background work is at risk of being killed.

## sendAbortSignal -- Blocking Safety

```cpp
void sendAbortSignal(bool blockUntilStopped)
{
    if (isThreadRunning())
    {
        if (blockUntilStopped)
        {
            if (Thread::getCurrentThread() == this && blockUntilStopped)
            {
                signalThreadShouldExit();
                reportScriptError("Can't stop with blocking on the worker thread");
            }
            else
            {
                // Extend timeout while waiting
                engine->extendTimeout(timeOut + 10);
                stopThread(timeOut);
            }
        }
        else
            signalThreadShouldExit();
    }
}
```

Safety: Calling sendAbortSignal(true) from within the background task itself would deadlock (thread waiting for itself to stop). This is detected and reported as a script error.

## Recompilation Handler

```cpp
static void recompiled(ScriptBackgroundTask& task, bool unused)
{
    task.sendAbortSignal(true);
}
```

Registered via `preCompileListeners`. On recompile, the thread is stopped with blocking. This ensures no background thread survives past script recompilation.

## Destructor

```cpp
~ScriptBackgroundTask()
{
    recordingSession = nullptr;
    stopThread(timeOut);
}
```

Cleans up profiling session, then stops thread with the configured timeout.

## setProgress / setStatusMessage -- Loading Thread Forwarding

When `forwardToLoadingThread` is true:
- `setProgress(p)` also sets `getMainController_()->getSampleManager().getPreloadProgress()`
- `setStatusMessage(m)` also calls `getMainController_()->getSampleManager().setCurrentPreloadMessage(m)`

This reuses HISE's built-in sample loading overlay to display background task progress.

## setFinishCallback Setup

```cpp
void setFinishCallback(var newFinishCallback)
{
    if (HiseJavascriptEngine::isJavascriptFunction(newFinishCallback))
    {
        finishCallback = WeakCallbackHolder(getScriptProcessor(), this, newFinishCallback, 2);
        finishCallback.incRefCount();
        finishCallback.setThisObject(this);
        finishCallback.addAsSource(this, "onTaskFinished");
    }
}
```

The `setThisObject(this)` call makes the BackgroundTask available as `this` inside the callback.

## TaskViewer (Debug Component, USE_BACKEND only implied)

An inner `struct TaskViewer` provides a debug popup in the HISE IDE:
- Inherits `Component`, `ComponentForDebugInformation`, `PooledUIUpdater::SimpleTimer`
- Shows thread name, active status, progress bar, and cancel button
- The cancel button calls `signalThreadShouldExit()` directly
- Created via `createPopupComponent()` override

## Preprocessor Guards

- `#if PERFETTO` -- Perfetto tracing in `run()` and `shouldAbort()` for thread profiling
- `#if USE_BACKEND` -- Backend-only warning in `shouldAbort()` for timeout gap detection; error logging in `run()`
- `PROFILE_ONLY(...)` -- Macro for profiling-only code in constructor

## Threading Model Summary

| Operation | Thread |
|---|---|
| `callOnBackgroundThread` function | Dedicated juce::Thread at high priority |
| `killVoicesAndCall` function | Sample loading thread (via KillStateHandler) |
| `runProcess` log function | Dedicated juce::Thread (same as callOnBackgroundThread) |
| `setFinishCallback` callback | Called from the background thread at end of run() |
| `setProperty`/`getProperty` | Any thread (lock-protected) |
| `setProgress`/`getProgress` | Any thread (atomic) |
| `setStatusMessage`/`getStatusMessage` | Any thread (lock-protected) |
| `shouldAbort` | Background thread (extends script timeout) |
| `sendAbortSignal` | Any thread (but NOT from background thread with blocking=true) |

## ThreadStarters Utility

```cpp
struct ThreadStarters
{
    static void startHigh(juce::Thread* t) { t->startThread(juce::Thread::Priority::high); }
    // ...
};
```

Both `callOnBackgroundThread` and `runProcess` use `ThreadStarters::startHigh()`.
