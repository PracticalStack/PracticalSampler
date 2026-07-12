# Threads -- Class Analysis

## Brief
Thread identity queries, lock state inspection, and safe audio-suspended function execution.

## Purpose
Threads is a namespace-style utility class that provides introspection into HISE's threading system. It exposes constants for the four main thread types (Audio, Scripting, Loading, UI) and methods to query which thread is currently executing, whether specific locks are held, and by whom. Its `killVoicesAndCall` method provides a safe mechanism to suspend audio processing, kill all voices, and execute a function on the loading thread -- the standard pattern for operations that modify audio-thread-accessible state. It also offers a profiling API gated behind the `HISE_INCLUDE_PROFILING_TOOLKIT` preprocessor define.

## Details

### Thread Model

HISE uses a lock-free audio architecture coordinated by the `KillStateHandler`. Rather than blocking the audio thread with locks, operations that need to modify shared state suspend audio processing entirely (outputting silence), execute the modification, then resume. The Threads API exposes this system to HiseScript.

### Thread Constants and Lock Priority

The six constants map directly to `LockHelpers::Type` enum values. Locks have ascending priority -- you cannot acquire a lower-priority lock while holding a higher-priority one:

| Constant | Value | Priority | Lock Held During |
|----------|-------|----------|------------------|
| `UI` | 0 | Lowest | Message thread operations |
| `Scripting` | 1 | Low | Script execution and compilation |
| `Loading` | 2 | Medium | Sample loading and removal |
| `Audio` | 4 | Highest | Audio callback processing |
| `Unknown` | 5 | N/A | Sentinel for unrecognized threads |
| `Free` | 6 | N/A | Sentinel for unlocked state |

Note: Value 3 (`IteratorLock`) exists internally but is not exposed to scripting.

### AudioExportThread Mapping

The audio export thread (used during offline rendering) maps to `Threads.Audio` from the scripting perspective. See `getCurrentThread()` for details.

### killVoicesAndCall Internals

See `killVoicesAndCall()` for the full API. The function always targets the `SampleLoadingThread` and acquires the `ScriptLock` before executing the callback.

### Profiling System

See `startProfiling()` for the full API and options schema. Requires the `HISE_INCLUDE_PROFILING_TOOLKIT` preprocessor define.

## obtainedVia
Global namespace object `Threads` -- available directly in all HiseScript contexts.

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| Audio | 4 | int | Audio thread identifier (also used for audio export thread) | ThreadId |
| Scripting | 1 | int | Scripting thread identifier | ThreadId |
| Loading | 2 | int | Sample loading thread identifier | ThreadId |
| UI | 0 | int | Message/UI thread identifier | ThreadId |
| Unknown | 5 | int | Unknown or unrecognized thread | ThreadId |
| Free | 6 | int | No thread / unlocked state | ThreadId |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Threads.killVoicesAndCall(function() { this.doSomething(); })` | `Threads.killVoicesAndCall(function() { doSomething(); })` | The callback takes zero arguments and executes on the loading thread. `this` context may not be valid in the deferred execution context. |

## codeExample
```javascript
// Query current thread identity
var threadId = Threads.getCurrentThread();

if (threadId == Threads.Audio)
    Console.print("Running on audio thread");
```

## Alternatives
- `BackgroundTask` -- concrete task handle for running code on a dedicated background thread with progress tracking
- `ThreadSafeStorage` -- lock-based container for safely passing data between threads

## Related Preprocessors
- `HISE_INCLUDE_PROFILING_TOOLKIT` -- required for `startProfiling` functionality
- `USE_BACKEND` -- enables warning when profiling toolkit is missing from project settings

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Thread identity queries are purely informational with no preconditions, and killVoicesAndCall is a well-guarded synchronous operation that reports errors internally.
