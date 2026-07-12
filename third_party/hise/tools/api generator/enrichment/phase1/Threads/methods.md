# Threads -- Method Entries

## getCurrentThread

**Signature:** `int getCurrentThread()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var thread = Threads.getCurrentThread();`

**Description:**
Returns the thread constant identifying which thread the caller is currently executing on. The returned value matches one of the `Threads` constants (`Audio`, `Scripting`, `Loading`, `UI`, `Unknown`, or `Free`). Both the real-time audio thread and the offline audio export thread return `Threads.Audio`.

**Parameters:**
None.

**Cross References:**
- `$API.Threads.getCurrentThreadName$`
- `$API.Threads.toString$`

---

## getCurrentThreadName

**Signature:** `String getCurrentThreadName()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var name = Threads.getCurrentThreadName();`

**Description:**
Returns a human-readable name for the current thread. Equivalent to calling `Threads.toString(Threads.getCurrentThread())`. Returns one of: "Message Thread", "Scripting Thread", "Sample Thread", "Audio Thread", "Unknown Thread", or "Free (unlocked)".

**Parameters:**
None.

**Cross References:**
- `$API.Threads.getCurrentThread$`
- `$API.Threads.toString$`

---

## getLockerThread

**Signature:** `int getLockerThread(int threadThatIsLocked)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var locker = Threads.getLockerThread(Threads.Audio);`

**Description:**
Returns the lock type currently held by the specified thread. Pass a thread constant (`Threads.Audio`, `Threads.Scripting`, etc.) and the method returns which lock that thread currently holds. Returns `Threads.Free` if the thread does not hold any lock.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| threadThatIsLocked | Integer | no | Thread constant to query | One of `Threads.Audio`, `Threads.Scripting`, `Threads.Loading`, `Threads.UI` |

**Cross References:**
- `$API.Threads.isLocked$`
- `$API.Threads.isLockedByCurrentThread$`

---

## isAudioRunning

**Signature:** `bool isAudioRunning()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var running = Threads.isAudioRunning();`

**Description:**
Returns `true` if the audio callback is currently active. Returns `false` during load operations or when audio processing has been suspended (e.g., by `killVoicesAndCall`).

**Parameters:**
None.

**Cross References:**
- `$API.Threads.killVoicesAndCall$`
- `$API.Threads.isCurrentlyExporting$`

---

## isCurrentlyExporting

**Signature:** `bool isCurrentlyExporting()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var exporting = Threads.isCurrentlyExporting();`

**Description:**
Returns `true` if the audio export thread is active (offline rendering is in progress). Useful for branching behavior between real-time playback and offline export -- for example, skipping UI updates or using higher-quality processing during export.

**Parameters:**
None.

**Cross References:**
- `$API.Threads.isAudioRunning$`
- `$API.Threads.getCurrentThread$`

---

## isLocked

**Signature:** `bool isLocked(int thread)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var locked = Threads.isLocked(Threads.Audio);`

**Description:**
Returns `true` if the specified thread currently holds any lock. Delegates to `getLockerThread` and checks whether the result is anything other than `Threads.Free`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| thread | Integer | no | Thread constant to query | One of `Threads.Audio`, `Threads.Scripting`, `Threads.Loading`, `Threads.UI` |

**Cross References:**
- `$API.Threads.getLockerThread$`
- `$API.Threads.isLockedByCurrentThread$`

---

## isLockedByCurrentThread

**Signature:** `bool isLockedByCurrentThread(int thread)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var holdsLock = Threads.isLockedByCurrentThread(Threads.Scripting);`

**Description:**
Returns `true` if the calling thread currently holds the lock identified by the given thread constant. Use this to verify lock ownership before performing operations that require a specific lock to be held.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| thread | Integer | no | Lock/thread constant to check | One of `Threads.Audio`, `Threads.Scripting`, `Threads.Loading`, `Threads.UI` |

**Cross References:**
- `$API.Threads.isLocked$`
- `$API.Threads.getLockerThread$`

---

## killVoicesAndCall

**Signature:** `bool killVoicesAndCall(var functionToExecute)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Suspends audio processing, kills all voices, acquires ScriptLock, and defers execution to the SampleLoadingThread.
**Minimal Example:** `Threads.killVoicesAndCall(doHeavyWork);`

**Description:**
Suspends audio processing, kills all active voices, and executes the given function on the SampleLoadingThread with the ScriptLock held. This is the standard pattern for operations that must modify audio-thread-accessible state safely (e.g., swapping sample maps, reconfiguring routing). Returns `true` if the function executed synchronously (caller was already on the loading thread with voices killed), `false` if execution was deferred. The target thread is always SampleLoadingThread -- this is not configurable.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| functionToExecute | Function | no | Zero-argument function to execute once audio is suspended | Must take 0 arguments |

**Callback Signature:** functionToExecute()

**Pitfalls:**
- The return value `false` does not indicate failure -- it means the function was queued for deferred execution on the loading thread rather than running immediately. Both `true` and `false` are success outcomes.

**Cross References:**
- `$API.Threads.isAudioRunning$`

**Example:**
```javascript:kill-voices-and-modify
// Title: Suspend audio to safely modify shared state
inline function onSafeToModify()
{
    Console.print("Audio suspended - safe to modify state");
};

Threads.killVoicesAndCall(onSafeToModify);
```
```json:testMetadata:kill-voices-and-modify
{
  "testable": false,
  "skipReason": "Callback executes on SampleLoadingThread after onInit completes; cannot verify deferred execution during compilation"
}
```

---

## startProfiling

**Signature:** `void startProfiling(var options, var finishCallback)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Starts a recording session involving heap allocations and listener registration.
**Minimal Example:** `Threads.startProfiling(500, onProfilingDone);`

**Description:**
Starts a thread profiling session. Accepts either a plain number (milliseconds to record, clamped to 10-10000) or a JSON options object for fine-grained control. When recording completes, the `finishCallback` is called with a single Base64-encoded string containing the profiling data. Requires the `HISE_INCLUDE_PROFILING_TOOLKIT` preprocessor define -- throws a script error without it.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| options | Object | no | Recording duration in ms (as Number) or JSON options object | Number: 10-10000; Object: see Callback Properties |
| finishCallback | Function | no | Called when recording completes with Base64 profiling data | Must take 1 argument |

**Callback Signature:** finishCallback(base64Data: String)

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| recordingLength | String | Duration string like "1000 ms" |
| recordingTrigger | Integer | Trigger type: 0=Manual, 1=Compilation, 2=MidiInput, 3=MouseClick |
| threadFilter | Array | Thread name strings to include (e.g., "Audio Thread", "Scripting Thread") |
| eventFilter | Array | Source type strings to include (e.g., "Script", "DSP", "Lock", "Paint") |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Audio Thread" | The real-time audio processing thread |
| "UI Thread" | The message/UI thread |
| "Scripting Thread" | The thread executing HiseScript |
| "Loading Thread" | The sample loading thread |
| "Worker Thread" | Background worker thread |
| "Server Thread" | HTTP server thread |

**Pitfalls:**
- Requires `HISE_INCLUDE_PROFILING_TOOLKIT` to be defined in the project settings. Without it, the method throws a script error rather than silently failing.

**Cross References:**
- `$API.Threads.getCurrentThread$`

**Example:**
```javascript:profile-with-options
// Title: Profile specific threads with JSON options
inline function onProfilingComplete(base64Data)
{
    Console.print("Profile complete: " + base64Data.length + " chars");
};

Threads.startProfiling({
    "recordingLength": "500 ms",
    "recordingTrigger": 0,
    "threadFilter": ["Audio Thread", "Scripting Thread"],
    "eventFilter": ["Script", "DSP"]
}, onProfilingComplete);
```
```json:testMetadata:profile-with-options
{
  "testable": false,
  "skipReason": "Requires HISE_INCLUDE_PROFILING_TOOLKIT preprocessor define which may not be enabled"
}
```

---

## toString

**Signature:** `String toString(int thread)`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String return value involves atomic ref-count operations.
**Minimal Example:** `var name = Threads.toString(Threads.Audio);`

**Description:**
Returns a human-readable name for the given thread constant. The mapping is: `Threads.UI` returns "Message Thread", `Threads.Scripting` returns "Scripting Thread", `Threads.Loading` returns "Sample Thread", `Threads.Audio` returns "Audio Thread", `Threads.Unknown` returns "Unknown Thread", `Threads.Free` returns "Free (unlocked)".

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| thread | Integer | no | Thread constant to convert | One of the `Threads` constants |

**Cross References:**
- `$API.Threads.getCurrentThread$`
- `$API.Threads.getCurrentThreadName$`

