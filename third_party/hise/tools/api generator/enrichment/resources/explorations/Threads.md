# Threads -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` (Threads entry)
- `enrichment/base/Threads.json`
- No prerequisites required (class has no prerequisite chain)
- No base class exploration needed (ApiClass is a simple base)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.h`, line 1860

```cpp
class Threads: public ApiClass,
               public ScriptingObject
{
public:
    Threads(ProcessorWithScriptingContent* p);
    Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Threads"); }

    // API METHODS
    int getCurrentThread() const;
    bool isAudioRunning() const;
    bool isCurrentlyExporting() const;
    bool isLockedByCurrentThread(int thread) const;
    int getLockerThread(int threadThatIsLocked) const;
    bool isLocked(int thread) const;
    void startProfiling(var options, var finishCallback);
    String toString(int thread) const;
    String getCurrentThreadName() const { return toString(getCurrentThread()); }
    bool killVoicesAndCall(const var& functionToExecute);

private:
    WeakCallbackHolder threadProfileCallback;

    using TargetThreadId = MainController::KillStateHandler::TargetThread;
    using LockId = LockHelpers::Type;

    static TargetThreadId getAsThreadId(int x);
    static LockId getAsLockId(int x);

    MainController::KillStateHandler& getKillStateHandler();
    const MainController::KillStateHandler& getKillStateHandler() const;

    struct Wrapper;
};
```

### Inheritance
- `ApiClass` -- HISE's base for namespace-style API classes (no instance creation, accessed as `Threads.method()`)
- `ScriptingObject` -- provides `getScriptProcessor()` for accessing the MainController

### Key Internal Types (aliases)
- `TargetThreadId` = `MainController::KillStateHandler::TargetThread`
- `LockId` = `LockHelpers::Type`

## Constructor

**File:** `HISE/hi_scripting/scripting/api/ScriptingApi.cpp`, line 7864

```cpp
ScriptingApi::Threads::Threads(ProcessorWithScriptingContent* p):
    ApiClass(6),
    ScriptingObject(p),
    threadProfileCallback(p, this, var(), 1)
{
    addConstant("Audio", (int)LockHelpers::Type::AudioLock);
    addConstant("Scripting", (int)LockHelpers::Type::ScriptLock);
    addConstant("Loading", (int)LockHelpers::Type::SampleLock);
    addConstant("UI", (int)LockHelpers::Type::MessageLock);
    addConstant("Unknown", (int)LockHelpers::Type::numLockTypes);
    addConstant("Free", (int)LockHelpers::Type::unused);

    ADD_API_METHOD_0(getCurrentThread);
    ADD_API_METHOD_0(isAudioRunning);
    ADD_API_METHOD_0(isCurrentlyExporting);
    ADD_API_METHOD_1(isLockedByCurrentThread);
    ADD_API_METHOD_1(getLockerThread);
    ADD_API_METHOD_1(isLocked);
    ADD_API_METHOD_1(killVoicesAndCall);
    ADD_API_METHOD_1(toString);
    ADD_API_METHOD_0(getCurrentThreadName);
    ADD_API_METHOD_2(startProfiling);

#if HISE_INCLUDE_PROFILING_TOOLKIT
    auto& dh = getScriptProcessor()->getMainController_()->getDebugSession();
    dh.recordingFlushBroadcaster.addListener(*this, [](Threads& t, DebugSession::ProfileDataSource::ProfileInfoBase::Ptr p)
    {
        if(p != nullptr && t.threadProfileCallback)
        {
            auto b64 = p->toBase64();
            t.threadProfileCallback.call1(b64);
        }
    }, false);
#endif
}
```

### Method Registration
All methods use plain `ADD_API_METHOD_N` -- NO typed method registrations (`ADD_TYPED_API_METHOD_N`). This means all parameter types must be inferred from the C++ signatures.

### Constants (from addConstant calls)

| Name | Value | C++ Source | Description |
|------|-------|------------|-------------|
| `Audio` | `(int)LockHelpers::Type::AudioLock` = 4 | `AudioLock` enum value | Audio thread identifier |
| `Scripting` | `(int)LockHelpers::Type::ScriptLock` = 1 | `ScriptLock` enum value | Scripting thread identifier |
| `Loading` | `(int)LockHelpers::Type::SampleLock` = 2 | `SampleLock` enum value | Sample loading thread identifier |
| `UI` | `(int)LockHelpers::Type::MessageLock` = 0 | `MessageLock` enum value | Message/UI thread identifier |
| `Unknown` | `(int)LockHelpers::Type::numLockTypes` = 5 | Sentinel value | Unknown thread identifier |
| `Free` | `(int)LockHelpers::Type::unused` = 6 | Sentinel value | No thread (unlocked) |

`ApiClass(6)` -- 6 constants registered.

## Upstream Data Providers

### LockHelpers::Type Enum

**File:** `HISE/hi_core/hi_core/LockHelpers.h`, line 63

```cpp
enum class Type
{
    MessageLock = 0,  // UI thread
    ScriptLock,       // held during script execution or compilation
    SampleLock,       // held during sample add/remove
    IteratorLock,     // held during processor chain iteration (never exposed to scripting)
    AudioLock,        // held during audio callback
    numLockTypes,     // sentinel = Unknown
    unused            // sentinel = Free
};
```

Lock priority is ascending: `MessageLock < ScriptLock < SampleLock/IteratorLock < AudioLock`. You cannot acquire a lower-priority lock while holding a higher-priority one on the same thread. `SampleLock` and `IteratorLock` have equal priority (mutually exclusive).

Note: `IteratorLock` (value 3) is NOT exposed as a scripting constant. The constant numbering maps directly to enum values:
- `Threads.UI` = 0 (MessageLock)
- `Threads.Scripting` = 1 (ScriptLock)
- `Threads.Loading` = 2 (SampleLock)
- `Threads.Audio` = 4 (AudioLock)
- `Threads.Unknown` = 5 (numLockTypes)
- `Threads.Free` = 6 (unused)

### KillStateHandler::TargetThread Enum

**File:** `HISE/hi_core/hi_core/MainController.h`, line 1307

```cpp
enum class TargetThread
{
    MessageThread = 0,
    SampleLoadingThread,
    AudioThread,
    AudioExportThread,
    ScriptingThread,
    numTargetThreads,
    UnknownThread,
    Free
};
```

### Thread-to-Lock Mapping (getCurrentThread)

The `getCurrentThread()` method translates from `TargetThread` to `LockId`:

| TargetThread | Returns (LockId) | Scripting Constant |
|---|---|---|
| MessageThread | MessageLock (0) | `Threads.UI` |
| SampleLoadingThread | SampleLock (2) | `Threads.Loading` |
| AudioThread | AudioLock (4) | `Threads.Audio` |
| AudioExportThread | AudioLock (4) | `Threads.Audio` |
| ScriptingThread | ScriptLock (1) | `Threads.Scripting` |
| UnknownThread | numLockTypes (5) | `Threads.Unknown` |
| Free | unused (6) | `Threads.Free` |

Note: `AudioExportThread` maps to `AudioLock`, same as `AudioThread`. From scripting perspective, both appear as `Threads.Audio`.

### KillStateHandler Infrastructure

**File:** `HISE/hi_core/hi_core/MainController.h`

The `KillStateHandler` is the core HISE threading coordinator. Key methods used by Threads:

- `getCurrentThread()` -- identifies which thread the caller is on
- `isAudioRunning()` -- checks if audio callback is active (false during load operations)
- `isCurrentlyExporting()` -- checks if AudioExportThread has a registered thread ID
- `killVoicesAndCall(Processor*, ProcessorFunction, TargetThread)` -- suspends audio, kills voices, executes function on target thread
- `currentThreadHoldsLock(LockHelpers::Type)` -- checks if calling thread holds a specific lock
- `getLockTypeForThread(TargetThread)` -- maps thread to its lock type
- `getThreadForLockType(LockHelpers::Type)` -- reverse mapping

The `getKillStateHandler()` helper method (lines 8041-8045) simply returns `getScriptProcessor()->getMainController_()->getKillStateHandler()`.

## Profiling Infrastructure

### Preprocessor Guard: HISE_INCLUDE_PROFILING_TOOLKIT

The `startProfiling` method and constructor listener are guarded by `#if HISE_INCLUDE_PROFILING_TOOLKIT`. Without this define:
- `startProfiling()` calls `reportScriptError("Profiling is not enabled")`
- No listener is registered on `recordingFlushBroadcaster`

Additionally, inside the profiling-enabled path, there is a `#if USE_BACKEND` check that warns if the project settings do not include `HISE_INCLUDE_PROFILING_TOOLKIT=1`, meaning the profiling will not work in the exported plugin.

### DebugSession Options

**File:** `HISE/hi_tools/hi_dev/DebugSession.h`, line 657

The `startProfiling` method accepts either:
1. A `DynamicObject` (JSON options) -- parsed via `Options::fromDynamicObject()`
2. A plain number -- treated as milliseconds to profile (clamped to 10-10000ms)

Options struct fields:
- `trigger` -- `TriggerType` enum: `Manual` (0), `Compilation` (1), `MidiInput` (2), `MouseClick` (3)
- `millisecondsToRecord` -- double, default 1000.0
- `threadFilter` -- array of `ThreadIdentifier::Type` names
- `eventFilter` -- array of `ProfileDataSource::SourceType` names

JSON property names for the options object:
- `"recordingLength"` -- string like "1000 ms" (parsed as double)
- `"recordingTrigger"` -- integer (TriggerType enum value)
- `"threadFilter"` -- array of thread name strings
- `"eventFilter"` -- array of source type name strings

### ProfileDataSource::SourceType Enum

```
Undefined, Lock, Script, ScriptCallback, Broadcaster, TimerCallback,
Server, Paint, DSP, Scriptnode, Trace, BackgroundTask, RecordingSession
```

Source type name strings (from `getSourceTypeName`):
`"Undefined"`, `"Lock"`, `"Script"`, `"Scriptnode"`, `"Callback"`, `"TimerCallback"`, `"Broadcaster"`, `"Paint"`, `"DSP"`, `"Trace"`, `"Server"`, `"Background Task"`, `"Threads"`

### ThreadIdentifier::Type Enum (for threadFilter)

```
Undefined, LoadingThread, AudioThread, ScriptingThread, ServerThread, UIThread, WorkerThread
```

Thread name strings: `"Unknown"`, `"Audio Thread"`, `"UI Thread"`, `"Scripting Thread"`, `"Loading Thread"`, `"Worker Thread"`, `"Server Thread"`

### Profiling Callback Flow

1. `startProfiling()` stores the `finishCallback` in `threadProfileCallback` (WeakCallbackHolder)
2. If options is a DynamicObject with `trigger == Manual`, calls `dh.startRecording(ms, holder)`
3. If options is a plain number, calls `dh.startRecording(ms, holder)` directly
4. When recording completes, `recordingFlushBroadcaster` fires
5. The listener (registered in constructor) converts profile data to Base64 and calls `threadProfileCallback.call1(b64)`

## Method Implementation Details

### getAsLockId / getAsThreadId (private helpers)

```cpp
LockId getAsLockId(int x) { return (LockId)x; }
TargetThreadId getAsThreadId(int x) {
    return MainController::KillStateHandler::getThreadForLockType(getAsLockId(x));
}
```

These are simple casts. The integer constants exposed to scripting ARE the LockHelpers::Type enum values directly.

### killVoicesAndCall Implementation

```cpp
bool killVoicesAndCall(const var& functionToExecute)
{
    WeakCallbackHolder wc(getScriptProcessor(), this, functionToExecute, 0);
    return getKillStateHandler().killVoicesAndCall(
        dynamic_cast<Processor*>(getScriptProcessor()),
        [wc](Processor* p) {
            WeakCallbackHolder copy = std::move(wc);
            if(copy) {
                LockHelpers::SafeLock sl(p->getMainController(), LockId::ScriptLock);
                auto ok = copy.callSync(nullptr, 0, nullptr);
                if(!ok.wasOk())
                    debugError(p, ok.getErrorMessage());
                return SafeFunctionCall::OK;
            }
            return SafeFunctionCall::nullPointerCall;
        },
        TargetThreadId::SampleLoadingThread
    );
}
```

Key observations:
- Always targets `SampleLoadingThread` (not configurable)
- Acquires `ScriptLock` before executing the callback
- Uses `callSync` (synchronous execution)
- Returns true if executed synchronously, false if deferred
- The function takes 0 arguments (WeakCallbackHolder constructed with argCount=0)

### isLocked Implementation

```cpp
bool isLocked(int thread) const
{
    auto t = (LockId)getLockerThread(thread);
    return t != LockId::unused;
}
```

Checks if `getLockerThread` returns anything other than `unused` (Free).

### getLockerThread Implementation

```cpp
int getLockerThread(int threadThatIsLocked) const
{
    return (int)getKillStateHandler().getLockTypeForThread(getAsThreadId(threadThatIsLocked));
}
```

Converts the lock ID to a thread ID, then asks KillStateHandler which lock type that thread holds.

### toString Implementation

Returns human-readable thread names:

| LockId | String |
|--------|--------|
| MessageLock | "Message Thread" |
| ScriptLock | "Scripting Thread" |
| SampleLock | "Sample Thread" |
| IteratorLock | "Iterator Thread (never used)" |
| AudioLock | "Audio Thread" |
| numLockTypes | "Unknown Thread" |
| unused | "Free (unlocked)" |

### getCurrentThreadName

Inline in header: `return toString(getCurrentThread());` -- simply combines the two methods.

## Factory / obtainedVia

Threads is a namespace-style API class (inherits ApiClass). It is NOT created by any factory method. It is available as a global `Threads` object in HiseScript, similar to `Math`, `Console`, etc. Constructed internally during script processor initialization.

## Threading / Lifecycle Constraints

- All methods can be called from any thread (the whole purpose is thread introspection)
- `killVoicesAndCall` is the only method with side effects -- it suspends audio processing
- `startProfiling` requires `HISE_INCLUDE_PROFILING_TOOLKIT` compile flag; errors without it
- The profiling callback (`finishCallback`) is called asynchronously when recording completes

## Related Preprocessors

- `HISE_INCLUDE_PROFILING_TOOLKIT` -- gates profiling functionality in constructor and `startProfiling`
- `USE_BACKEND` -- gates the warning about missing profiling toolkit in project settings (backend IDE only)
