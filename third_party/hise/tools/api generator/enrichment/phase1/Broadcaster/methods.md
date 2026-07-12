# Broadcaster -- Method Entries

## addComponentPropertyListener

**Signature:** `bool addComponentPropertyListener(var object, var propertyList, var metadata, var optionalFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates OwnedArray entries, creates WeakCallbackHolder, resolves component names via string lookup.
**Minimal Example:** `{obj}.addComponentPropertyListener("Knob1", "text", "propSync", false);`

**Description:**
Adds a target listener that sets properties on the specified UI components whenever the broadcaster fires. Operates in two modes depending on whether `optionalFunction` is provided.

In direct mode (no callback), the broadcaster must have exactly 3 arguments with semantics `(component, propertyId, value)`. The value from the third broadcast argument is applied to the named properties on all target components, skipping any component that matches the source component from `args[0]` to avoid feedback loops.

In callback mode, the callback receives `(targetIndex, ...broadcastArgs)` where `targetIndex` is the integer index of the current target component in the `object` list. The callback must return a value which is then set as the property value on the target component. Returning undefined triggers an error.

On registration, the target is immediately initialized with the broadcaster's current state (or initial values from attached sources), so components receive their initial property values without waiting for the first broadcast.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| propertyList | NotUndefined | no | Property name(s) -- a single property ID string or an array of property ID strings | Must be valid properties on all target components |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field | Non-empty string or object with `id` |
| optionalFunction | NotUndefined | no | Optional transform callback. Pass `false` for direct mode | If a function, must accept `numArgs + 1` parameters and return a value |

**Callback Signature:** optionalFunction(targetIndex: int, component: var, propertyId: var, value: var)

**Pitfalls:**
- In direct mode (no callback), the broadcaster must have exactly 3 arguments. Using a broadcaster with a different argument count silently creates the target but the `callSync` will use incorrect argument indices for `component` (`args[0]`) and `value` (`args[2]`), producing wrong property values.
- The callback mode requires the function to return a value. Returning nothing (implicit undefined) triggers an error `"You need to return a value"` for each target component.
- The feedback-loop skip in direct mode compares `args[0]` against each target using `var::operator==`. If the source broadcaster sends a string component name rather than a component reference, the skip comparison will fail and the source component's property will also be set.

**Cross References:**
- `$API.Broadcaster.attachToComponentProperties$`
- `$API.Broadcaster.removeListener$`
- `$API.Broadcaster.addComponentValueListener$`
- `$API.Broadcaster.addComponentRefreshListener$`

**Example:**
```javascript:component-property-listener-direct
// Title: Syncing a property across multiple components in direct mode
const var bc = Engine.createBroadcaster({
    "id": "PropSync",
    "args": ["component", "property", "value"]
});

// --- setup ---
const var Knob1 = Content.addKnob("Knob1", 0, 0);
const var Knob2 = Content.addKnob("Knob2", 150, 0);
Knob1.set("saveInPreset", false);
Knob2.set("saveInPreset", false);
// --- end setup ---

bc.attachToComponentProperties([Knob1, Knob2], ["enabled"], "source");
bc.addComponentPropertyListener([Knob1, Knob2], ["enabled"], "syncEnabled", false);
```
```json:testMetadata:component-property-listener-direct
{
  "testable": false,
  "skipReason": "Property sync via attachToComponentProperties fires asynchronously through ValueTree listeners. REPL-triggered property changes do not reliably propagate through the async broadcaster dispatch chain within the validation timeout."
}
```

## addComponentRefreshListener

**Signature:** `bool addComponentRefreshListener(var componentIds, String refreshType, var metadata)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates OwnedArray entries, resolves component names via string lookup, creates RefCountedTime slots.
**Minimal Example:** `{obj}.addComponentRefreshListener("Panel1", "repaint", "refresher");`

**Description:**
Adds a target listener that triggers a refresh action on the specified UI components whenever the broadcaster fires. Unlike other `addComponent*` methods, this listener has no callback parameter -- the `refreshType` string determines the action performed.

The listener ignores the broadcast arguments entirely. Whenever the broadcaster sends any message, the specified refresh action is executed on all target components unconditionally.

Valid refresh types:

| Value | Action |
|-------|--------|
| `"repaint"` | Calls `sendRepaintMessage()` on each component (triggers paint routine) |
| `"changed"` | Calls `changed()` on each component (triggers control callback) |
| `"updateValueFromProcessorConnection"` | Refreshes the component's value from its connected processor parameter |
| `"loseFocus"` | Removes focus from each component |
| `"resetValueToDefault"` | Resets each component's value to its default |

On registration, the target is initialized with the broadcaster's current state, so the refresh action fires immediately if the broadcaster has valid (non-undefined) last values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array | Must resolve to at least one valid ScriptComponent |
| refreshType | String | yes | The refresh action to perform when the broadcaster fires | One of: `"repaint"`, `"changed"`, `"updateValueFromProcessorConnection"`, `"loseFocus"`, `"resetValueToDefault"` |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field | Non-empty string or object with `id` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "repaint" | Triggers a repaint of each target component, causing its paint routine or LAF function to execute |
| "changed" | Triggers the control callback for each target component, as if the user had interacted with it |
| "updateValueFromProcessorConnection" | Refreshes each component's displayed value from its connected processor parameter binding |
| "loseFocus" | Removes keyboard/mouse focus from each target component |
| "resetValueToDefault" | Resets each component's value to the default specified in its properties |

**Pitfalls:**
- An invalid `refreshType` string produces a descriptive error `"Unknown refresh mode: ..."`, but only after the `ComponentRefreshItem` object has already been constructed. The item is not added to the broadcaster in this case.
- If `componentIds` resolves to an empty list (all names invalid), an explicit error `"Can't find components for the given componentId object"` is thrown.

**Cross References:**
- `$API.Broadcaster.addComponentPropertyListener$`
- `$API.Broadcaster.addComponentValueListener$`
- `$API.Broadcaster.removeListener$`

**Example:**
```javascript:component-refresh-repaint
// Title: Triggering repaint on a panel when a value changes
// --- setup ---
const var Panel1 = Content.addPanel("Panel1", 0, 0);
Panel1.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "RepaintTrigger",
    "args": ["value"]
});

bc.addComponentRefreshListener("Panel1", "repaint", "panelRepaint");
bc.sendSyncMessage([42]);
```
```json:testMetadata:component-refresh-repaint
{
  "testable": false,
  "skipReason": "addComponentRefreshListener with 'repaint' triggers a visual repaint that has no scriptable observable side-effect. The 'changed' refresh type fires changed() which is a no-op during onInit."
}
```

## addComponentValueListener

**Signature:** `bool addComponentValueListener(var object, var metadata, var optionalFunction)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates OwnedArray entries, creates WeakCallbackHolder, resolves component names via string lookup.
**Minimal Example:** `{obj}.addComponentValueListener("Knob1", "valueSync", false);`

**Description:**
Adds a target listener that sets the value of the specified UI components whenever the broadcaster fires. Operates in two modes depending on whether `optionalFunction` is provided.

In direct mode (no callback), the last argument from the broadcast message is used as the value. It calls `setValue()` on all target components with that value.

In callback mode, the callback receives `(targetIndex, ...broadcastArgs)` where `targetIndex` is the integer index of the current target component in the `object` list. The callback must return a value which is then set via `setValue()` on the target component. Returning undefined triggers an error `"You need to return a value"`.

The callback is constructed with `numArgs + 1` parameters (broadcast args + prepended target index).

On registration, the target is immediately initialized with the broadcaster's current state (or initial values from attached sources), so components receive their initial values without waiting for the first broadcast.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field | Non-empty string or object with `id` |
| optionalFunction | NotUndefined | no | Optional transform callback. Pass `false` for direct mode | If a function, must accept `numArgs + 1` parameters and return a value |

**Callback Signature:** optionalFunction(targetIndex: int, ...broadcastArgs: var)

**Pitfalls:**
- In direct mode, the value is taken from `args.getLast()` regardless of how many broadcast arguments exist. For a 1-argument broadcaster, `args.getLast()` is the sole argument. For a 3-argument broadcaster, it is the third argument. This is position-dependent and may cause confusion when the broadcaster has semantically different arguments.
- The callback mode requires the function to return a value. Returning nothing (implicit undefined) causes an error for each target component.

**Cross References:**
- `$API.Broadcaster.attachToComponentValue$`
- `$API.Broadcaster.addComponentPropertyListener$`
- `$API.Broadcaster.removeListener$`

**Example:**
```javascript:component-value-listener-callback
// Title: Setting component values with a transform callback
const var bc = Engine.createBroadcaster({
    "id": "ValueSync",
    "args": ["component", "value"]
});

// --- setup ---
const var Knob1 = Content.addKnob("Knob1v", 0, 0);
const var Knob2 = Content.addKnob("Knob2v", 150, 0);
Knob1.set("saveInPreset", false);
Knob2.set("saveInPreset", false);
// --- end setup ---

inline function onValueTarget(targetIndex, component, value)
{
    if (targetIndex == 0)
        return value;

    return 1.0 - value;
}

bc.addComponentValueListener(["Knob1v", "Knob2v"], "invertSecond", onValueTarget);
```
```json:testMetadata:component-value-listener-callback
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "bc.sendSyncMessage([\"Knob1v\", 0.75]) || true", "value": true},
    {"type": "REPL", "expression": "Content.getComponent(\"Knob1v\").getValue()", "value": 0.75},
    {"type": "REPL", "expression": "Content.getComponent(\"Knob2v\").getValue()", "value": 0.25}
  ]
}
```

## addDelayedListener

**Signature:** `bool addDelayedListener(int delayInMilliSeconds, var obj, var metadata, var function)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates OwnedArray entries, creates timer objects for deferred execution.
**Minimal Example:** `{obj}.addDelayedListener(200, "debouncer", "debounce", onDebounced);`

**Description:**
Adds a target listener whose callback is executed after a specified delay in milliseconds. Each time the broadcaster fires, the previous pending delayed call is cancelled and a new timer is started. This provides a debounce mechanism -- only the last message within the delay window actually triggers the callback.

If `delayInMilliSeconds` is 0, the method falls back to `addListener()` (no delay, immediate execution).

When the delayed timer fires, it uses the broadcaster's `lastValues` at the time the timer fires (not the values at the time `callSync` was invoked). This means if multiple rapid sends occur, the callback receives the most recent values.

The callback function receives the broadcaster's argument values with `obj` as the `this` reference (if `setReplaceThisReference` is true, which is the default).

Duplicate listeners are rejected with an error `"this object is already registered to the listener"`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| delayInMilliSeconds | Integer | yes | Delay before the callback fires, in milliseconds. 0 falls back to addListener | >= 0 |
| obj | NotUndefined | no | The `this` reference for the callback (if replaceThisReference is enabled). Can be a JSON object, script object, or string | -- |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field | Non-empty string or object with `id` |
| function | Function | no | The callback function to invoke after the delay | Must accept the broadcaster's argument count |

**Callback Signature:** function(...broadcastArgs: var)

**Pitfalls:**
- The delayed callback uses `parent->lastValues` (the broadcaster's current state at timer-fire time), not the values that were passed to `callSync`. In rapid-fire scenarios, the callback only ever sees the final values.
- Each new broadcast replaces the previous pending delayed call. There is no accumulation of pending calls -- only the most recent one executes.
- The delayed callback is bypassed if the broadcaster is in bypassed state when the timer fires.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.removeListener$`

## removeAllSources

**Signature:** `void removeAllSources()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** OwnedArray::clear() deallocates all source objects, invokes destructors that unregister from upstream LambdaBroadcasters, Processor::AttributeListeners, etc.
**Minimal Example:** `{obj}.removeAllSources();`

**Description:**
Removes all attached event sources from the broadcaster. This clears the internal `attachedListeners` OwnedArray, destroying all `ListenerBase` objects (ComponentPropertyListener, ComponentValueListener, ModuleParameterListener, etc.). After this call, the broadcaster no longer receives automatic events from previously attached sources.

Registered listener targets (TargetBase items in `items`) are not affected -- they remain registered and will be called if messages are sent manually via `sendSyncMessage`, `sendAsyncMessage`, or dot-assignment. Use `removeAllListeners` to also remove targets.

Each destroyed source object's destructor unregisters from its upstream provider (e.g., removes itself from a processor's attribute listener list, unsubscribes from `LambdaBroadcaster` instances). This cleanup is handled automatically.

This is a bulk removal operation. For selective removal by metadata, use `removeSource`.

**Parameters:**

(None)

**Cross References:**
- `$API.Broadcaster.removeSource$`
- `$API.Broadcaster.removeAllListeners$`
- `$API.Broadcaster.reset$`

## sendMessage

**Disabled:** deprecated
**Disabled Reason:** Superseded by `sendSyncMessage` and `sendAsyncMessage`. The boolean `isSync` parameter is ambiguous and hard to guess. The C++ emits a `debugError` at runtime but does not yet use the `ADD_API_METHOD_N_DEPRECATED` macro. Use `sendSyncMessage` for synchronous dispatch or `sendAsyncMessage` for asynchronous dispatch.
**deprecatedInFavourOf:** sendSyncMessage, sendAsyncMessage

## sendAsyncMessage

**Signature:** `void sendAsyncMessage(var args)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls sendMessageInternal(args, false) which acquires SimpleReadWriteLock::ScopedWriteLock for lastValues update, posts a HiPriorityCallbackExecution job to JavascriptThreadPool (heap allocation for lambda capture and job queue entry).
**Minimal Example:** `{obj}.sendAsyncMessage(["ready", 42]);`

**Description:**
Sends a message asynchronously to all registered listener targets. This is a thin wrapper that calls `sendMessageInternal(args, false)`.

The `args` parameter must be an array whose length matches the number of broadcaster arguments (from `defaultValues.size()`), or a single value if the broadcaster has exactly one argument. A count mismatch produces a descriptive script error.

The full dispatch flow:
1. Change detection compares each value in `args` against `lastValues`. If nothing changed and queue mode is disabled and `forceSend` is false, the message is silently suppressed.
2. If changed: acquires write lock on `lastValueLock`, updates `lastValues`.
3. If bypassed: stores values but returns without dispatching.
4. Posts a `HiPriorityCallbackExecution` job to `JavascriptThreadPool`. The job executes on the scripting thread at elevated priority.
5. An `asyncPending` atomic flag coalesces rapid consecutive async sends -- if a previous async job has not yet executed, subsequent sends are suppressed (unless queue mode is enabled).
6. With queue mode enabled, each async send captures its own `lastValues` snapshot, ensuring all values are dispatched in order.

If `setForceSynchronousExecution(true)` has been called, the `forceSync` flag overrides async to synchronous -- the message is dispatched directly on the calling thread instead of being posted to the job queue.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| args | NotUndefined | no | Message arguments -- an array matching the broadcaster's argument count, or a single value for single-argument broadcasters | Array length must equal `defaultValues.size()`, or be a non-array for 1-arg broadcasters |

**Pitfalls:**
- Without queue mode, rapid consecutive async sends are coalesced. Only the most recent values are dispatched when the job executes. Enable queue mode (`setEnableQueue(true)`) if every value transition must be observed.
- Change detection suppresses duplicate values silently. If you need to re-send the same values, use `resendLastMessage` which sets `forceSend`.
- If any of the arguments are undefined/void, `sendInternal` silently returns without calling any listeners (unless `setSendMessageForUndefinedArgs` was called, which only affects `initItem`, not `sendInternal`).

**Cross References:**
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendMessageWithDelay$`
- `$API.Broadcaster.resendLastMessage$`
- `$API.Broadcaster.setEnableQueue$`
- `$API.Broadcaster.setForceSynchronousExecution$`

**Example:**
```javascript:send-async-message
// Title: Broadcasting values asynchronously
const var bc = Engine.createBroadcaster({
    "id": "StatusBroadcaster",
    "args": ["status", "count"]
});

var lastStatus = "";

inline function onStatusChange(status, count)
{
    lastStatus = status;
}

bc.addListener("handler", "statusLogger", onStatusChange);
bc.sendAsyncMessage(["ready", 1]);
```
```json:testMetadata:send-async-message
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "lastStatus", "value": "ready"}
  ]
}
```

## reset

**Signature:** `void reset()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls sendInternal which acquires SimpleReadWriteLock::ScopedReadLock, iterates targets, copies args per target (non-realtime path), and invokes JavaScript callbacks.
**Minimal Example:** `{obj}.reset();`

**Description:**
Dispatches the broadcaster's original `defaultValues` to all registered listener targets. This calls `sendInternal(defaultValues)` directly, bypassing the normal `sendMessageInternal` flow -- there is no change detection, no bypass check, no async path, and `lastValues` is not updated.

The `defaultValues` array was established during construction from the `Engine.createBroadcaster()` parameter. For the recommended JSON format with `args`, all default values are `undefined`. For the legacy format with named properties, default values are the property values from the constructor argument.

Because `sendInternal` checks for undefined arguments and silently returns if any argument is undefined/void, calling `reset()` on a broadcaster created with the standard `{ "id": "...", "args": [...] }` format has no effect on listeners -- the undefined check triggers first and suppresses the dispatch. The `setSendMessageForUndefinedArgs(true)` flag does NOT affect `sendInternal`'s own undefined check (that flag is only used in `initItem`).

Note that `reset()` does NOT clear `lastValues`, does NOT remove listeners or sources, and does NOT reset the bypass state. It is purely a "re-dispatch defaults to targets" operation.

This method is accessible from the debug popup (the reset button in the BroadcasterMap visualization).

**Parameters:**

(None)

**Pitfalls:**
- For broadcasters created with `{ "id": "...", "args": [...] }` (the standard format), `reset()` has no visible effect because `sendInternal` silently suppresses messages when any argument is undefined -- and all default values are undefined in this format.
- `reset()` does NOT update `lastValues`. After calling `reset()`, reading `bc.argName` still returns the last sent value, not the default value.
- `reset()` bypasses the `bypassed` check. Even if the broadcaster is bypassed, `reset()` dispatches to all targets because it calls `sendInternal` directly, not `sendMessageInternal`.

**Cross References:**
- `$API.Broadcaster.resendLastMessage$`
- `$API.Broadcaster.removeAllListeners$`
- `$API.Broadcaster.removeAllSources$`
- `$API.Broadcaster.setSendMessageForUndefinedArgs$`

## removeListener

**Signature:** `bool removeListener(var idFromMetadata)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** OwnedArray::removeObject deallocates the target, destructor may release WeakCallbackHolder and stop timers. Metadata construction from `var` involves string hashing.
**Minimal Example:** `var ok = {obj}.removeListener("myListener");`

**Description:**
Removes the first listener target whose metadata matches `idFromMetadata`. Returns `true` if a matching listener was found and removed, `false` otherwise.

Matching is performed by constructing a `Metadata` object from the `idFromMetadata` parameter and comparing its `hash` (a `hashCode64` of the ID string) against each registered target's metadata hash. This means matching is by metadata ID string, not by object reference or callback reference.

If the parameter is a string, it is used directly as the metadata ID for comparison. If it is a JSON object, its `"id"` field is extracted and hashed.

Only the first matching target is removed. If multiple targets share the same metadata ID (which is unusual but technically possible), only the first one found in iteration order is removed.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| idFromMetadata | NotUndefined | no | The metadata identifier to match -- a string ID or a JSON object with an `id` field | Must match the metadata used when the listener was registered |

**Pitfalls:**
- Matching is by metadata hash, not by object reference or callback function reference. Passing the original `object` or `function` argument from `addListener` does not work for removal -- you must pass the metadata string or object that was used as the metadata parameter.
- Returns `false` silently when no match is found. There is no error or warning for a failed removal.

**Cross References:**
- `$API.Broadcaster.removeAllListeners$`
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.removeSource$`

## removeSource

**Signature:** `bool removeSource(var metadata)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** OwnedArray::removeObject deallocates the source, destructor unregisters from upstream providers (Processor::AttributeListener, LambdaBroadcaster, etc.). Metadata construction involves string hashing.
**Minimal Example:** `var ok = {obj}.removeSource("paramSource");`

**Description:**
Removes the first attached source whose metadata matches the `metadata` parameter. Returns `true` if a matching source was found and removed, `false` otherwise.

Matching works identically to `removeListener` -- a `Metadata` object is constructed from the parameter and compared by hash. The parameter should be the same metadata string or JSON object that was passed to the `attach*` method's `optionalMetadata` parameter.

Only the first matching source is removed. The source's destructor automatically unregisters from its upstream event provider (e.g., removes itself from a processor's attribute listener list, unsubscribes from `LambdaBroadcaster` instances).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| metadata | NotUndefined | no | The metadata identifier to match -- a string ID or a JSON object with an `id` field | Must match the metadata used when the source was attached |

**Pitfalls:**
- Matching is by metadata hash, not by source type or configuration. The metadata parameter passed to the `attach*` methods is what identifies the source for removal.
- Returns `false` silently when no match is found. There is no error or warning.
- Some `attach*` methods use `optionalMetadata` which may default to an auto-generated internal metadata if omitted. If no explicit metadata was provided when attaching, the auto-generated ID may be difficult to discover for selective removal. In that case, `removeAllSources` may be more practical.

**Cross References:**
- `$API.Broadcaster.removeAllSources$`
- `$API.Broadcaster.removeListener$`

## resendLastMessage

**Signature:** `void resendLastMessage(var isSync)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Delegates to sendMessageInternal which acquires SimpleReadWriteLock, posts to JavascriptThreadPool (async path), or iterates targets (sync path). The forceSend bypass adds a ScopedValueSetter allocation.
**Minimal Example:** `{obj}.resendLastMessage(AsyncNotification);`

**Description:**
Re-dispatches the broadcaster's current `lastValues` to all registered listener targets, bypassing the change-detection gate. Normally, `sendMessageInternal` suppresses a message if the values have not changed since the last send. This method sets a scoped `forceSend = true` flag so the message is always dispatched regardless of whether values changed.

The `isSync` parameter controls dispatch mode. It is processed through `ApiHelpers::isSynchronous()` which recognizes the notification type constants: pass `SyncNotification` for synchronous execution on the calling thread, or `AsyncNotification`/`AsyncHiPriorityNotification` for asynchronous execution on the scripting thread.

If `setForceSynchronousExecution(true)` has been called on the broadcaster, the `forceSync` flag overrides the parameter and forces synchronous execution regardless.

This method is commonly used after unbypassing a broadcaster (e.g., `setBypassed(false, true, ...)` calls it internally) or when external state has changed and listeners need to be resynchronized with the current values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| isSync | NotUndefined | no | Dispatch mode -- pass `SyncNotification` for synchronous or `AsyncNotification` for asynchronous dispatch | Use notification type constants, not raw booleans |

**Pitfalls:**
- The `forceSend` flag bypasses change detection but does NOT bypass the undefined-argument check in `sendInternal`. If any of the `lastValues` are undefined (e.g., after a `reset` when default values are undefined), the message is silently suppressed despite `forceSend` being true.
- The `forceSend` flag also does NOT bypass the `bypassed` check. If the broadcaster is bypassed, `resendLastMessage` stores values but does not dispatch them, even with `forceSend`.

**Cross References:**
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.setBypassed$`
- `$API.Broadcaster.reset$`


**Example:**
```javascript:delayed-listener-debounce
// Title: Debouncing rapid value changes with a delayed listener
const var bc = Engine.createBroadcaster({
    "id": "DebounceBC",
    "args": ["value"]
});

var lastReceived = -1;

inline function onDebounced(value)
{
    lastReceived = value;
}

bc.addDelayedListener(100, "handler", "debounce", onDebounced);
bc.sendSyncMessage([1]);
bc.sendSyncMessage([2]);
bc.sendSyncMessage([3]);
```
```json:testMetadata:delayed-listener-debounce
{
  "testable": false,
  "skipReason": "addDelayedListener timer callback causes HISE Debug crash during validation - suspected debug-only assertion in delayed timer dispatch"
}
```

## addListener

**Signature:** `bool addListener(var object, var metadata, var function)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Allocates OwnedArray entries, creates WeakCallbackHolder, performs realtime safety check on realtime-safe broadcasters.
**Minimal Example:** `{obj}.addListener("handler", "myListener", onBroadcast);`

**Description:**
Adds a general-purpose callback listener to the broadcaster. This is the primary method for receiving broadcast messages. When the broadcaster sends a message, the callback function is invoked with the broadcast arguments.

The `object` parameter serves two purposes: it is used as the `this` reference in the callback function (when `setReplaceThisReference` is enabled, which is the default), and it is passed as the metadata's identification object for later removal via `removeListener`. The `object` can be a JSON object, a script object, or a simple string.

If the broadcaster has `realtimeSafe` mode enabled, the callback function is validated for audio-thread safety. In backend (HISE IDE), a full `RealtimeSafetyInfo::check()` is performed. In frontend (exported plugin), only inline functions are accepted. Non-realtime-safe callbacks produce an error.

Duplicate listeners (same metadata) are rejected with an error `"this object is already registered to the listener"`.

On registration, the target is immediately initialized with the broadcaster's current state (or initial values from attached sources).

Listeners are sorted by metadata priority (higher priority values execute first).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| object | NotUndefined | no | The `this` reference for the callback function. Also used for listener identification. Can be a JSON object, script object, or string | -- |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field. Used for identification and priority sorting | Non-empty string or object with `id` |
| function | Function | no | The callback function invoked when the broadcaster fires | Must accept the broadcaster's argument count. Must be realtime-safe if broadcaster is in realtime mode |

**Callback Signature:** function(...broadcastArgs: var)

**Pitfalls:**
- On a realtime-safe broadcaster, adding a non-inline function in an exported plugin throws an error. In the HISE IDE, a more detailed safety analysis is performed. This is not a silent failure -- the error is descriptive.
- The `object` parameter replaces `this` inside the callback by default. If you need access to the original `this` scope, call `setReplaceThisReference(false)` before adding listeners.

**Cross References:**
- `$API.Broadcaster.addDelayedListener$`
- `$API.Broadcaster.removeListener$`
- `$API.Broadcaster.setReplaceThisReference$`
- `$API.Broadcaster.setRealtimeMode$`

**Example:**
```javascript:add-listener-basic
// Title: Basic broadcaster listener with state tracking
const var bc = Engine.createBroadcaster({
    "id": "StatusBC",
    "args": ["status", "count"]
});

var log = [];

inline function onStatus(status, count)
{
    log.push(status + ":" + count);
}

bc.addListener("handler", "statusLogger", onStatus);
bc.sendSyncMessage(["ready", 1]);
bc.sendSyncMessage(["done", 2]);
```
```json:testMetadata:add-listener-basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "log.length", "value": 2},
    {"type": "REPL", "expression": "log[0]", "value": "ready:1"},
    {"type": "REPL", "expression": "log[1]", "value": "done:2"}
  ]
}
```

## addModuleParameterSyncer

**Signature:** `bool addModuleParameterSyncer(String moduleId, var parameterIndex, var metadata)`
**Return Type:** `Integer`
**Call Scope:** unsafe
**Call Scope Note:** Looks up processor by name (string construction), allocates OwnedArray entry, forces synchronous execution mode.
**Minimal Example:** `{obj}.addModuleParameterSyncer("SimpleGain1", "Gain", "gainSync");`

**Description:**
Adds a target listener that synchronizes a module parameter from the broadcaster's last argument value. When the broadcaster fires, the last element of the broadcast arguments is cast to float, sanitized (NaN/Inf protection), and applied to the specified module parameter via `setAttribute()`.

The `parameterIndex` can be either a string (parameter name, resolved via `getParameterIndexForIdentifier()`) or an integer (direct parameter index).

As a side effect, this method forces the broadcaster into synchronous execution mode by calling `setForceSynchronousExecution(true)`. This ensures parameter changes are applied immediately on the calling thread rather than being deferred.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleId | String | yes | The ID of the target processor module | Must match an existing module in the processor tree |
| parameterIndex | NotUndefined | no | Parameter identifier -- either a string parameter name or an integer index | Must resolve to a valid parameter index on the target module |
| metadata | NotUndefined | no | Listener metadata -- a string ID or JSON object with at least an `id` field | Non-empty string or object with `id` |

**Pitfalls:**
- This method forces the entire broadcaster into synchronous execution mode. All subsequent sends (including `sendAsyncMessage`) will execute synchronously. This is a global broadcaster-level side effect, not scoped to this syncer.
- The value is always taken from `args.getLast()`, so the parameter position in the broadcast argument list determines which value is synced. For a 3-argument broadcaster `(processorId, parameterId, value)`, this correctly uses the value. For differently structured broadcasters, the last argument may not be the intended value.
- If the module is deleted at runtime (weak reference becomes null), the syncer silently skips the `setAttribute` call without error.

**Cross References:**
- `$API.Broadcaster.attachToModuleParameter$`
- `$API.Broadcaster.setForceSynchronousExecution$`
- `$API.Broadcaster.removeListener$`

**Example:**
```javascript:module-parameter-syncer
// Title: Syncing a broadcaster value to a module parameter
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "SyncGain", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "GainSync",
    "args": ["processorId", "parameterId", "value"]
});

bc.addModuleParameterSyncer("SyncGain", "Gain", "gainTarget");
bc.sendSyncMessage(["SyncGain", "Gain", -12.0]);

const var gain = Synth.getEffect("SyncGain");
```
```json:testMetadata:module-parameter-syncer
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "gain.getAttribute(gain.Gain)", "value": -12.0}
  ]
}
```

## attachToComplexData

**Signature:** `void attachToComplexData(String dataTypeAndEvent, var moduleIds, var dataIndexes, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), looks up processors by name, creates ComplexDataListener, registers with ExternalDataHolder.
**Minimal Example:** `{obj}.attachToComplexData("Table.Content", "LookupTable1", 0, "tableSource");`

**Description:**
Registers the broadcaster as a source that fires whenever a complex data object changes. The broadcaster must have exactly 3 arguments with semantics `(processorId, index, value)`.

The `dataTypeAndEvent` parameter uses the format `"DataType.EventType"` where:
- **DataType** is one of the `ExternalData` type names: `"AudioFile"`, `"Table"`, `"SliderPack"`, `"FilterCoefficients"`, `"DisplayBuffer"`, etc.
- **EventType** determines what kind of change triggers the broadcaster:
  - `"Content"` -- fires when the data content changes (for Tables: curve edited, for AudioFiles: file loaded, etc.). The value argument is the base64-encoded content string.
  - `"Display"` or `"DisplayIndex"` -- fires when the display value changes (e.g., the current playback position or table lookup position). The value argument is a numeric display value.

The `moduleIds` parameter accepts a single module ID string or an array of module ID strings. Each module must implement `ExternalDataHolder` (processors that own complex data objects like tables, slider packs, or audio files).

The `dataIndexes` parameter accepts a single integer index or an array of integer indices, specifying which data slot(s) on the module to observe. Indices are validated against the module's actual data object count.

Queue mode is automatically enabled when multiple processors or multiple indices are specified.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| dataTypeAndEvent | String | yes | Data type and event type in `"DataType.EventType"` format | Both parts must be non-empty; DataType must match a valid ExternalData type name |
| moduleIds | NotUndefined | no | Module ID(s) -- a single string or array of strings identifying ExternalDataHolder processors | Each must resolve to a processor that implements ExternalDataHolder |
| dataIndexes | NotUndefined | no | Data slot index(es) -- a single integer or array of integers | Each index must be valid for the specified data type on the target module |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Content" | Fires when the data content changes. The value argument is a base64-encoded string representation of the data |
| "Display" | Fires when the display value changes. The value argument is a numeric display value (e.g. playback position) |
| "DisplayIndex" | Same behavior as "Display" -- fires on display value changes with a numeric value argument |

**Pitfalls:**
- The broadcaster must have exactly 3 arguments. Using a broadcaster with a different argument count produces a descriptive error.
- If the `dataTypeAndEvent` string does not contain a dot separator, or either part is empty, a descriptive error is thrown.
- The data type name must match the exact string returned by `ExternalData::getDataTypeName()`. Common mistake: using `"Audio"` instead of `"AudioFile"`.

**Cross References:**
- `$API.Broadcaster.addListener$`

**Diagram:**
- **Brief:** Complex Data Event Flow
- **Type:** topology
- **Description:** Shows the flow from an ExternalDataHolder module through the ComplexDataListener source to the broadcaster's target listeners. The source monitors either content changes or display value changes depending on the EventType, and forwards (processorId, index, value) triples to the broadcaster.

**Example:**
```javascript:attach-to-complex-data-table
// Title: Watching a table's content changes
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Modulators.Velocity, "WatchTable", 0, builder.ChainIndexes.Gain);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "TableWatch",
    "args": ["processorId", "index", "value"]
});

var lastProcId = "";
var lastIndex = -1;

inline function onTableChange(processorId, index, value)
{
    lastProcId = processorId;
    lastIndex = index;
}

bc.addListener("handler", "tableLogger", onTableChange);
bc.attachToComplexData("Table.Content", "WatchTable", 0, "tableSource");
```
```json:testMetadata:attach-to-complex-data-table
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastProcId", "value": "WatchTable"},
    {"type": "REPL", "expression": "lastIndex", "value": 0}
  ]
}
```

## attachToComponentMouseEvents

**Signature:** `void attachToComponentMouseEvents(var componentIds, var callbackLevel, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), resolves components by name, creates InternalMouseListener objects that attach to components.
**Minimal Example:** `{obj}.attachToComponentMouseEvents("Panel1", "Clicks Only", "mouseSource");`

**Description:**
Registers the broadcaster as a source that fires whenever mouse events occur on the specified components. The broadcaster must have exactly 2 arguments with semantics `(component, event)`.

The `callbackLevel` parameter is a string that determines which mouse events are captured. The valid strings are the display names from `MouseCallbackComponent::getCallbackLevels()`:

| Level String | Events Captured |
|-------------|----------------|
| `"No Callbacks"` | No events (disables) |
| `"Context Menu"` | Right-click context menu only |
| `"Clicks Only"` | Mouse clicks |
| `"Clicks & Hover"` | Clicks and mouse enter/exit |
| `"Clicks, Hover & Dragging"` | Clicks, enter/exit, and drag |
| `"All Callbacks"` | All mouse events including movement |

The `componentIds` parameter accepts a single component (name string or reference) or an array of components.

Each target component gets an `InternalMouseListener` registered via `ScriptComponent::attachMouseListener()`. When a mouse event occurs on the component, the broadcaster receives `(componentReference, mouseEventObject)`.

As a side effect, this method sets `forceSend = true` on the broadcaster, meaning change detection is bypassed and every mouse event is always dispatched even if the event object properties happen to match a previous event.

Mouse events have no initial values (getNumInitialCalls() returns 0), so existing listeners are not called on attachment.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| callbackLevel | String | no | The mouse callback level determining which events are captured | Must be one of the valid callback level strings (see table above) |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "No Callbacks" | Disables mouse event listening for the attached components |
| "Context Menu" | Only captures right-click popup menu events |
| "Clicks Only" | Captures mouse click events (down and up) |
| "Clicks & Hover" | Captures click events plus mouse enter and exit (hover) events |
| "Clicks, Hover & Dragging" | Captures clicks, hover, and drag events |
| "All Callbacks" | Captures all mouse events including mouse move |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. Using a broadcaster with a different argument count produces a descriptive error.
- The callback level string must match exactly (including spaces and ampersand). An invalid string produces the error `"illegal callback level: ..."`.
- The `callbackLevel` value is converted to string via `callbackLevel.toString()` before matching, so passing an integer or other type will fail to match any valid level string.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.attachToContextMenu$`

**Example:**
```javascript:attach-mouse-events
// Title: Listening for mouse click events on a panel
// --- setup ---
const var ClickPanel = Content.addPanel("ClickPanel", 0, 0);
ClickPanel.set("width", 100);
ClickPanel.set("height", 100);
ClickPanel.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "MouseWatch",
    "args": ["component", "event"]
});

var clickCount = 0;

inline function onMouseEvent(component, event)
{
    if (isDefined(event.clicked))
        clickCount = clickCount + 1;
}

bc.addListener("handler", "clickCounter", onMouseEvent);
bc.attachToComponentMouseEvents("ClickPanel", "Clicks Only", "mouseSource");
```
```json:testMetadata:attach-mouse-events
{
  "testable": false,
  "skipReason": "Mouse events require physical user interaction or simulated mouse events that cannot be triggered programmatically from script"
}
```

## removeAllListeners

**Signature:** `void removeAllListeners()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** OwnedArray::clear() deallocates all target objects, invokes destructors that may release WeakCallbackHolder ref-counts and stop timers.
**Minimal Example:** `{obj}.removeAllListeners();`

**Description:**
Removes all registered listener targets from the broadcaster. This clears the internal `items` OwnedArray, destroying all `TargetBase` objects (ScriptTarget, DelayedItem, ComponentPropertyItem, ComponentValueItem, ComponentRefreshItem, ModuleParameterSyncer, OtherBroadcasterTarget). After this call, subsequent messages sent by the broadcaster are dispatched to zero targets.

Attached sources (ListenerBase items in `attachedListeners`) are not affected -- they continue detecting events and sending messages to the broadcaster. Use `removeAllSources` to also detach sources.

This is a bulk removal operation. For selective removal by metadata ID, use `removeListener`.

**Parameters:**

(None)

**Cross References:**
- `$API.Broadcaster.removeListener$`
- `$API.Broadcaster.removeAllSources$`
- `$API.Broadcaster.reset$`
- `$API.Broadcaster.addListener$`

## attachToComponentProperties

**Signature:** `void attachToComponentProperties(var componentIds, var propertyIds, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), resolves components by name (string lookup), creates ComponentPropertyListener with ValueTree property listeners.
**Minimal Example:** `{obj}.attachToComponentProperties("Knob1", "text", "propSource");`

**Description:**
Registers the broadcaster as a source that fires whenever one of the specified properties changes on any of the specified components. The broadcaster must have exactly 3 arguments with semantics `(component, propertyId, value)`.

Each component gets an internal `valuetree::PropertyListener` registered on its property ValueTree. When a watched property changes, the broadcaster receives `(componentReference, propertyIdString, newValue)` asynchronously via `sendMessageInternal`. The listener callback fires synchronously on the ValueTree notification, but the broadcaster send is asynchronous (non-sync).

The `componentIds` parameter accepts a single component (name string or reference) or an array of components. The `propertyIds` parameter accepts a single property ID string or an array of property ID strings. All specified properties are validated against each component -- if any property ID is invalid for any component, an error is thrown.

Unlike most other `attachTo*` methods, this method does NOT call `checkMetadataAndCallWithInitValues()` after adding the source. This means attaching the source does not immediately dispatch initial values to already-registered targets. However, targets added after this source are still initialized via `initItem()` which calls `ComponentPropertyListener::callItem()`, providing current property values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| propertyIds | NotUndefined | no | Property ID(s) -- a single property name string or an array of property name strings | Must be valid properties on all target components |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Pitfalls:**
- The broadcaster must have exactly 3 arguments. Using a broadcaster with a different argument count produces a descriptive error before the listener is created.
- This method does not dispatch initial property values to existing targets on attachment. If listeners are added before this source is attached, they will not receive the current property values until the next actual property change. Add the source before adding listeners for consistent initialization, or call `resendLastMessage` after attachment.
- Invalid property IDs are validated against all components in the list. If a property exists on some components but not others, the entire attachment fails with `"Illegal property id: ..."`.

**Cross References:**
- `$API.Broadcaster.addComponentPropertyListener$`

**Example:**
```javascript:attach-component-properties
// Title: Watching a component's text property for changes
// --- setup ---
const var PropKnob1 = Content.addKnob("PropKnob1", 0, 0);
PropKnob1.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "PropWatcher",
    "args": ["component", "property", "value"]
});

var lastProp = "";
var lastVal = "";

inline function onPropChange(component, property, value)
{
    lastProp = property;
    lastVal = value;
}

bc.addListener("handler", "propLogger", onPropChange);
bc.attachToComponentProperties("PropKnob1", "text", "propSource");
Content.getComponent("PropKnob1").set("text", "Hello");
```
```json:testMetadata:attach-component-properties
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 300, "expression": "lastProp", "value": "text"},
    {"type": "REPL", "expression": "lastVal", "value": "Hello"}
  ]
}
```

## attachToComponentValue

**Signature:** `void attachToComponentValue(var componentIds, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), resolves components by name, creates ComponentValueListener, attaches value listeners to components.
**Minimal Example:** `{obj}.attachToComponentValue("Knob1", "valueSource");`

**Description:**
Registers the broadcaster as a source that fires whenever the value of any of the specified components changes. The broadcaster must have exactly 2 arguments with semantics `(component, value)`.

Each component gets a value listener attached via `ScriptComponent::attachValueListener()`. When a component's value changes (e.g., user interaction, `setValue()` + `changed()`), the broadcaster receives `(componentReference, newValue)`.

The `componentIds` parameter accepts a single component (name string or reference) or an array of components.

On attachment, `checkMetadataAndCallWithInitValues()` is called, which dispatches initial values to all existing targets. The initial values are `(componentReference, currentValue)` for each component, so existing listeners immediately receive the current state of all watched components.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. Using a broadcaster with a different argument count produces a descriptive error. Note: the listener is created and added BEFORE the argument count check, so the listener exists in the attachedListeners array even if the error fires. However, `reportScriptError` aborts compilation, so this is academic.
- The value listener uses the component's `attachValueListener()` mechanism, which overwrites any previously attached broadcaster value listener on that component. If two broadcasters try to watch the same component's value, only the last one receives updates.

**Cross References:**
- `$API.Broadcaster.addComponentValueListener$`
- `$API.Broadcaster.attachToComponentProperties$`
- `$API.Broadcaster.attachToComponentVisibility$`

**Example:**
```javascript:attach-component-value
// Title: Watching a knob's value changes via broadcaster
// --- setup ---
const var ValKnob1 = Content.addKnob("ValKnob1", 0, 0);
ValKnob1.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "ValueWatch",
    "args": ["component", "value"]
});

var lastValue = -1.0;

inline function onValueChange(component, value)
{
    lastValue = value;
}

bc.addListener("handler", "valueLogger", onValueChange);
bc.attachToComponentValue("ValKnob1", "valueSource");
```
```json:testMetadata:attach-component-value
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastValue", "value": 0.0}
  ]
}
```

## attachToComponentVisibility

**Signature:** `void attachToComponentVisibility(var componentIds, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), resolves components by name, creates ComponentVisibilityListener with recursive ValueTree property listeners.
**Minimal Example:** `{obj}.attachToComponentVisibility("Panel1", "visSource");`

**Description:**
Registers the broadcaster as a source that fires whenever the visibility of any of the specified components changes. The broadcaster must have exactly 2 arguments with semantics `(id, isVisible)`.

The visibility check is recursive: it walks up the component's parent hierarchy in the ValueTree, ANDing the `visible` property of each ancestor. A component is considered visible only if it and all of its parent components are visible. This means hiding a parent panel also triggers visibility change events for all watched child components.

Each component gets a `valuetree::RecursivePropertyListener` registered on the ROOT of the component's property ValueTree (not just the component's own node). This listener watches the `visible` property on the entire tree and fires when any ancestor's visibility changes.

The broadcast arguments are `(componentIdString, isVisibleBool)` where the ID is the string from the ValueTree's `id` property.

On attachment, `checkMetadataAndCallWithInitValues()` is called. The initial value dispatch fires once per watched component with its current effective visibility state.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Pitfalls:**
- The first broadcast argument is the component's **string ID** (from the ValueTree), not a component reference. This differs from `attachToComponentValue` which passes the component reference object. Listeners that expect a component reference will receive a string instead.
- The recursive visibility check means a parent hiding/showing can cause multiple broadcasts -- one for every watched child of that parent. If many components under the same parent are watched, a single parent visibility toggle generates N broadcasts (one per child).
- The broadcast is sent asynchronously via `sendAsyncMessage()`, so the listener callback may execute after a short delay from the actual visibility change.

**Cross References:**
- `$API.Broadcaster.attachToComponentValue$`
- `$API.Broadcaster.attachToComponentProperties$`

**Example:**
```javascript:attach-component-visibility
// Title: Tracking a component's visibility state
// --- setup ---
const var VisPanel1 = Content.addPanel("VisPanel1", 0, 0);
VisPanel1.set("saveInPreset", false);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "VisWatch",
    "args": ["id", "isVisible"]
});

var visLog = [];

inline function onVisChange(id, isVisible)
{
    visLog.push(id + ":" + isVisible);
}

bc.addListener("handler", "visLogger", onVisChange);
bc.attachToComponentVisibility("VisPanel1", "visSource");
```
```json:testMetadata:attach-component-visibility
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "visLog.length", "value": 1},
    {"type": "REPL", "expression": "visLog[0]", "value": "VisPanel1:1"}
  ]
}
```

## attachToContextMenu

**Signature:** `void attachToContextMenu(var componentIds, var stateFunction, var itemList, var optionalMetadata, var useLeftClick)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), resolves components by name, creates ContextMenuListener, attaches mouse listeners with PopupMenuOnly level, enables queue mode.
**Minimal Example:** `{obj}.attachToContextMenu("Panel1", false, ["Item A", "Item B"], "menuSource", false);`

**Description:**
Registers the broadcaster as a source that fires when a context menu item is selected on any of the specified components. The broadcaster must have exactly 2 arguments with semantics `(component, menuItemIndex)`.

This method creates a popup context menu on the specified components. When the user right-clicks (or left-clicks if `useLeftClick` is true), a popup menu appears with the items from `itemList`. When the user selects an item, the broadcaster fires with `(componentReference, selectedItemIndex)` where the index is 0-based.

The `stateFunction` is an optional callback that controls the appearance and behavior of individual menu items. It is called with two arguments `(type, index)` and must return a value. The `type` string determines what aspect of the menu item is being queried:

| Type | Return Type | Default | Purpose |
|------|------------|---------|---------|
| `"active"` | bool | `false` | Whether the menu item shows a tick/checkmark |
| `"enabled"` | bool | `false` | Whether the menu item is interactable (not greyed out) |
| `"text"` | String | `""` | Dynamic text override for the menu item |

The state function's return values are cached at initialization and refreshed automatically after each menu selection (via `sendInternal` post-processing) or manually via `refreshContextMenuState()`.

Pass `false` for `stateFunction` to skip state management (all items enabled, no ticks, no dynamic text).

The `itemList` parameter accepts a single string or an array of strings representing the menu item labels.

Queue mode is automatically enabled when this source is attached, ensuring every menu selection is dispatched even during rapid successive selections.

Context menus have no initial values (getNumInitialCalls() returns 0), so existing listeners are not called on attachment.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| componentIds | NotUndefined | no | Component reference(s) -- a single component (name string or reference) or an array of components | Must resolve to valid ScriptComponent(s) |
| stateFunction | NotUndefined | no | State callback for menu item appearance, or `false` to disable state management | If a function, must accept 2 arguments (type: String, index: int) and return a value |
| itemList | NotUndefined | no | Menu item labels -- a single string or an array of strings | At least one item |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |
| useLeftClick | NotUndefined | no | If `true`, the context menu opens on left-click instead of right-click | Boolean |

**Callback Signature:** stateFunction(type: String, index: int)

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "active" | Queried per menu item to determine if a checkmark/tick is shown next to the item |
| "enabled" | Queried per menu item to determine if the item is interactable (true) or greyed out (false) |
| "text" | Queried per menu item to get a dynamic text override; returning an empty string uses the static itemList label |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. Using a broadcaster with a different argument count produces a descriptive error.
- The `stateFunction` results are cached at initialization time and refreshed only after menu selections or explicit `refreshContextMenuState()` calls. If external state changes affect what should be ticked or enabled, call `refreshContextMenuState()` to update the cached values before the user opens the menu.
- The `useLeftClick` parameter is cast to `bool` directly from the var. Passing `0` or `false` uses right-click; passing `1`, `true`, or any truthy value uses left-click.
- When `stateFunction` is `false` (not a callable), the `WeakCallbackHolder` is not valid, and the state queries return default values (`false` for active/enabled, `""` for text). All menu items appear enabled with no ticks.

**Cross References:**
- `$API.Broadcaster.refreshContextMenuState$`
- `$API.Broadcaster.attachToComponentMouseEvents$`

**Example:**
```javascript:attach-context-menu
// Title: Adding a context menu to a panel with state management
const var bc = Engine.createBroadcaster({
    "id": "MenuBC",
    "args": ["component", "menuItemIndex"]
});

// --- setup ---
const var MenuPanel = Content.addPanel("MenuPanel", 0, 0);
MenuPanel.set("width", 200);
MenuPanel.set("height", 200);
MenuPanel.set("saveInPreset", false);
// --- end setup ---

var selectedItem = -1;
reg activeIndex = 0;

inline function menuState(type, index)
{
    if (type == "active")
        return index == activeIndex;

    if (type == "enabled")
        return true;

    return "";
}

inline function onMenuSelect(component, menuItemIndex)
{
    selectedItem = menuItemIndex;
    activeIndex = menuItemIndex;
}

bc.addListener("handler", "menuHandler", onMenuSelect);
bc.attachToContextMenu(MenuPanel, menuState, ["Option A", "Option B", "Option C"], "menuSource", false);
```
```json:testMetadata:attach-context-menu
{
  "testable": false,
  "skipReason": "Context menu requires physical user interaction (right-click) that cannot be triggered programmatically from script"
}
```

## attachToEqEvents

**Signature:** `void attachToEqEvents(var moduleIds, var eventTypes, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), looks up processors by name, creates EqListener, registers with ProcessorFilterStatistics eventBroadcaster.
**Minimal Example:** `{obj}.attachToEqEvents("ParametricEQ1", "BandAdded", "eqSource");`

**Description:**
Registers the broadcaster as a source that fires when EQ-related events occur on the specified module(s). The broadcaster must have exactly 2 arguments with semantics `(eventType, value)`.

The `moduleIds` parameter accepts a single module ID string or an array of module ID strings. Each module must implement `ProcessorFilterStatistics::Holder` (typically parametric EQ modules). If a module does not support EQ events, an error is thrown.

The `eventTypes` parameter specifies which EQ events to subscribe to. It accepts a single string, an array of strings, or an empty string/empty array to subscribe to all event types.

Valid event type strings:

| Event Type | Description |
|-----------|-------------|
| `"BandAdded"` | A new filter band was added to the EQ |
| `"BandRemoved"` | A filter band was removed from the EQ |
| `"BandSelected"` | A filter band was selected (e.g., by user clicking) |
| `"FFTEnabled"` | The FFT display was enabled or disabled |

When an event fires, the broadcaster receives `(eventTypeString, eventValue)` asynchronously. The `value` varies by event type (e.g., band index for add/remove/select, boolean for FFT toggle).

EQ events have no initial values (getNumInitialCalls() returns 0), so existing listeners are not called on attachment.

Invalid event type strings produce a descriptive error listing the supported types.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleIds | NotUndefined | no | Module ID(s) -- a single string or array of strings identifying EQ processor modules | Each must resolve to a processor implementing ProcessorFilterStatistics::Holder |
| eventTypes | NotUndefined | no | Event type(s) to subscribe to -- a single string, an array of strings, or an empty string/array to subscribe to all | Each must be one of: `"BandAdded"`, `"BandRemoved"`, `"BandSelected"`, `"FFTEnabled"` |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "BandAdded" | Fires when a new filter band is added to the EQ. The value argument is the band index |
| "BandRemoved" | Fires when a filter band is removed from the EQ. The value argument is the removed band index |
| "BandSelected" | Fires when a filter band is selected by user interaction. The value argument is the selected band index |
| "FFTEnabled" | Fires when the FFT spectrum display is toggled. The value argument indicates the new FFT state |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. Using a broadcaster with a different argument count produces a descriptive error.
- Passing an empty string or empty array for `eventTypes` subscribes to all four event types. This is a convenience behavior, not an error.
- The module must implement `ProcessorFilterStatistics::Holder`. Passing a non-EQ module ID produces `"ModuleId is not an EQ"`.

**Cross References:**
- `$API.Broadcaster.addListener$`

**Example:**
```javascript:attach-eq-events
// Title: Listening for EQ band selection events
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.CurveEq, "TestEQ", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "EqWatch",
    "args": ["eventType", "value"]
});

var lastEventType = "";

inline function onEqEvent(eventType, value)
{
    lastEventType = eventType;
}

bc.addListener("handler", "eqLogger", onEqEvent);
bc.attachToEqEvents("TestEQ", "", "eqSource");
```
```json:testMetadata:attach-eq-events
{
  "testable": false,
  "skipReason": "EQ band events require user interaction with the EQ editor UI (adding, removing, selecting bands) that cannot be triggered programmatically from script"
}
```

## attachToInterfaceSize

**Signature:** `void attachToInterfaceSize(var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), subscribes to ScriptingContent::interfaceSizeBroadcaster (LambdaBroadcaster), creates InterfaceSizeListener.
**Minimal Example:** `{obj}.attachToInterfaceSize("sizeSource");`

**Description:**
Registers the broadcaster as a source that fires whenever the plugin's interface size changes. The broadcaster must have exactly 2 arguments with semantics `(width, height)`.

The source subscribes to the internal `ScriptingContent::interfaceSizeBroadcaster` (a `LambdaBroadcaster<int, int>`). When the interface is resized, the broadcaster receives `(newWidth, newHeight)` asynchronously via `sendAsyncMessage()`.

On attachment, `checkMetadataAndCallWithInitValues()` is called. The InterfaceSizeListener provides exactly 1 initial call with the current interface dimensions from `getContentWidth()` and `getContentHeight()`, so existing listeners immediately receive the current size.

This source is particularly useful for responsive layouts where components need to reposition or resize when the plugin window changes size.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Pitfalls:**
- [BUG] The broadcaster must have exactly 2 arguments. Using a broadcaster with a different argument count produces the error `"If you want to attach a broadcaster to visibility events, it needs two parameters (width and height)"`. Note: the error message mentions "visibility events" which is misleading -- it should say "interface size events". This is a copy-paste error in the error message text.
- Interface size changes are dispatched asynchronously, so the callback may execute slightly after the actual resize.

**Cross References:**
- `$API.Broadcaster.addListener$`

**Example:**
```javascript:attach-interface-size
// Title: Responding to interface size changes
const var bc = Engine.createBroadcaster({
    "id": "SizeWatch",
    "args": ["width", "height"]
});

var currentWidth = 0;
var currentHeight = 0;

inline function onSizeChange(width, height)
{
    currentWidth = width;
    currentHeight = height;
}

bc.addListener("handler", "sizeLogger", onSizeChange);
bc.attachToInterfaceSize("sizeSource");
```
```json:testMetadata:attach-interface-size
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "currentWidth > 0", "value": true},
    {"type": "REPL", "expression": "currentHeight > 0", "value": true}
  ]
}
```

## attachToModuleParameter

**Signature:** `void attachToModuleParameter(var moduleIds, var parameterIds, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), looks up processors by name, resolves parameter indices, creates ModuleParameterListener with Processor::AttributeListener or Processor::OtherListener, enables queue mode.
**Minimal Example:** `{obj}.attachToModuleParameter("SimpleGain1", "Gain", "paramSource");`

**Description:**
Registers the broadcaster as a source that fires whenever a module parameter changes on the specified processor(s). The broadcaster must have exactly 3 arguments with semantics `(processorId, parameterId, value)`.

The `moduleIds` parameter accepts a single module ID string or an array of module ID strings. When multiple modules are specified, they must all be the same processor type (enforced with an error check).

The `parameterIds` parameter accepts a single parameter identifier or an array of parameter identifiers. Each identifier can be:
- A **string** parameter name -- resolved via `getParameterIndexForIdentifier()` on the processor
- An **integer** parameter index -- used directly as the attribute index

Three special string identifiers are supported:
- `"Bypassed"` -- monitors the processor's bypass state. The callback receives the bypass state as a float (1.0 = bypassed, 0.0 = not bypassed).
- `"Enabled"` -- monitors the inverse of bypass state. The callback receives 1.0 when the processor is enabled (not bypassed) and 0.0 when disabled.
- `"Intensity"` -- only valid when the processor is a `Modulator`. Monitors the modulator's intensity value via the `Modulation::intensityBroadcaster`.

When a parameter changes, the broadcaster receives `(processorIdString, parameterIdOrIndex, newValueFloat)` asynchronously via `sendAsyncMessage()`. The parameter ID in the callback is either the resolved string name or the original integer index, depending on how `parameterIds` was specified.

On attachment, `checkMetadataAndCallWithInitValues()` is called. The initial dispatch provides one call per parameter per processor (plus one per special ID per processor), giving existing listeners the current parameter values.

Queue mode is automatically enabled after attachment, ensuring parameter change events from rapid automation are all dispatched.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleIds | NotUndefined | no | Module ID(s) -- a single string or array of strings identifying processor modules | Each must resolve to a valid processor. Multiple modules must be the same type |
| parameterIds | NotUndefined | no | Parameter identifier(s) -- a single string name, integer index, or an array of mixed strings/integers | String names must resolve to valid parameters. Special IDs: `"Bypassed"`, `"Enabled"`, `"Intensity"` (Modulator only) |
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "Bypassed" | Monitors bypass state. Fires with 1.0 when bypassed, 0.0 when active |
| "Enabled" | Monitors the inverse of bypass state. Fires with 1.0 when enabled, 0.0 when bypassed |
| "Intensity" | Monitors a Modulator's intensity value. Only valid on Modulator-type processors |

**Pitfalls:**
- [BUG] The broadcaster must have exactly 3 arguments. The error message incorrectly says "mouse events" instead of "module parameter events" -- this is a copy-paste error in the C++ source.
- The `moduleIds` parameter must be string IDs, not scripting object references. Passing a module object (e.g., from `Synth.getEffect()`) triggers `"The module list parameter must be a list of ID strings, not object references..."`.
- When multiple modules are specified, they must be the same processor type. Mixing different processor types produces an error.
- The `"Intensity"` special ID only works when the processor is a `Modulator`. Using it on a non-Modulator processor silently falls through to the normal parameter lookup, which will fail with `"unknown parameter ID: Intensity"` since most processors don't have a parameter named "Intensity".
- Queue mode is enabled as a side effect, meaning subsequent manually sent messages also go through the queue mechanism.

**Cross References:**
- `$API.Broadcaster.addModuleParameterSyncer$`
- `$API.Broadcaster.addListener$`

**Diagram:**
- **Brief:** Module Parameter Event Flow
- **Type:** topology
- **Description:** Shows how the ModuleParameterListener registers with Processor::AttributeListener (or OtherListener on legacy dispatch) and Processor::BypassListener. Normal parameter changes flow through onAttributeUpdate/otherChange, special IDs (Bypassed/Enabled) flow through bypassStateChanged, and Intensity flows through Modulation::intensityBroadcaster. All paths converge at sendAsyncMessage to the parent broadcaster.

**Example:**
```javascript:attach-module-parameter
// Title: Monitoring a module parameter and bypass state
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "WatchGain", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "ParamWatch",
    "args": ["processorId", "parameterId", "value"]
});

var lastParamId = "";
var lastParamValue = -1.0;

inline function onParamChange(processorId, parameterId, value)
{
    lastParamId = parameterId;
    lastParamValue = value;
}

bc.addListener("handler", "paramLogger", onParamChange);
bc.attachToModuleParameter("WatchGain", ["Gain"], "paramSource");

const var gain = Synth.getEffect("WatchGain");
```
```json:testMetadata:attach-module-parameter
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastParamId", "value": "Gain"},
    {"type": "REPL", "expression": "lastParamValue", "value": 0.0}
  ]
}
```

## attachToNonRealtimeChange

**Signature:** `void attachToNonRealtimeChange(var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Call Scope Note:** Calls throwIfAlreadyConnected(), subscribes to MainController::realtimeBroadcaster (LambdaBroadcaster<bool>), creates NonRealtimeSource, enables realtime mode on the broadcaster.
**Minimal Example:** `{obj}.attachToNonRealtimeChange("realtimeSource");`

**Description:**
Registers the broadcaster as a source that fires when the audio engine switches between realtime and non-realtime rendering modes. The broadcaster must have exactly 1 argument with semantics `(isNonRealtime)`.

This event fires when the DAW or host switches between realtime playback and offline rendering (e.g., bounce/export). The callback receives `true` when entering non-realtime (offline) mode and `false` when returning to realtime mode.

The source subscribes to `MainController::realtimeBroadcaster` (a `LambdaBroadcaster<bool>`). When the non-realtime state changes, the broadcaster sends a **synchronous** message via `sendSyncMessage()`. This is critical because the realtime mode flag is used for audio path decisions that must take effect immediately.

As a side effect, this method automatically enables **realtime mode** on the broadcaster by calling `setRealtimeMode(true)`. This means:
- The broadcaster skips all lock acquisitions during send
- The broadcaster sends without value comparison or async queueing
- Listener callbacks must be realtime-safe (inline functions in exported plugins)
- All subsequent listeners added to this broadcaster will be validated for realtime safety

Non-realtime change events have no initial values (getNumInitialCalls() returns 0), so existing listeners are not called on attachment. However, `callItem()` queries the current state from `SampleManager::isNonRealtime()` and dispatches it to each target when a new target is added via `initItem()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionalMetadata | NotUndefined | no | Optional source metadata -- a string ID or JSON object | -- |

**Pitfalls:**
- The broadcaster must have exactly 1 argument. Using a broadcaster with a different argument count produces a descriptive error.
- This method forcefully enables realtime mode on the broadcaster. All listeners added after this point must be realtime-safe. Adding a non-inline function listener in an exported plugin will produce an error.
- The synchronous dispatch means the callback executes on whatever thread triggers the non-realtime state change. This can be the audio thread, so callbacks must not allocate, lock, or perform I/O.

**Cross References:**
- `$API.Broadcaster.setRealtimeMode$`
- `$API.Broadcaster.attachToProcessingSpecs$`

**Example:**
```javascript:attach-nonrealtime-change
// Title: Adapting behavior for offline rendering
const var bc = Engine.createBroadcaster({
    "id": "RtWatch",
    "args": ["isNonRealtime"]
});

reg isOffline = 0;

inline function onRealtimeChange(isNonRealtime)
{
    isOffline = isNonRealtime;
}

bc.addListener("handler", "rtLogger", onRealtimeChange);
bc.attachToNonRealtimeChange("rtSource");
```
```json:testMetadata:attach-nonrealtime-change
{
  "testable": false,
  "skipReason": "Non-realtime change events require switching between realtime and offline rendering modes, which cannot be triggered programmatically from script"
}
```

## attachToOtherBroadcaster

**Signature:** `void attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachToOtherBroadcaster(sourceBc, false, false, "chainSource");`

**Description:**
Chains this broadcaster to one or more source broadcasters so that messages from the source(s) are forwarded to this broadcaster's listeners. When a source broadcaster fires, the message is dispatched to this broadcaster via an `OtherBroadcasterTarget` added to the source's items list.

An optional `argTransformFunction` can transform the source broadcaster's arguments before forwarding. If the transform function returns an array, that array replaces the message arguments. If it returns a non-array value, the original source arguments are forwarded unchanged. Pass `false` to skip the transform and forward arguments directly.

The `async` parameter controls whether forwarded messages are dispatched synchronously or asynchronously to this broadcaster. When `false`, the source broadcaster's `sendInternal` call blocks until this broadcaster finishes processing. When `true`, the forwarded message is posted as an async job.

On attachment, the source broadcaster(s)' current `lastValues` are dispatched to all existing listeners on this broadcaster as initial values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| otherBroadcaster | ScriptObject | no | Source broadcaster or array of source broadcasters | Must be Broadcaster instance(s) |
| argTransformFunction | Function | no | Optional transform function. Pass `false` to forward args unchanged | If a function, receives the source broadcaster's args and should return an array |
| async | Integer | yes | Whether forwarded messages dispatch asynchronously | `true` or `false` |
| optionalMetadata | NotUndefined | no | Source metadata -- a string ID or JSON object | Non-empty string or object with `id` |

**Callback Signature:** argTransformFunction(...sourceArgs: var)

**Pitfalls:**
- If the transform function returns a non-array value (e.g., a number or string), the original source arguments are forwarded unchanged rather than wrapping the return value. To transform arguments, the function must return an array whose length matches this broadcaster's argument count.
- This method does not validate that the source broadcaster's argument count matches this broadcaster's argument count. If counts differ and no transform function is provided, the forwarded message will trigger an argument count mismatch error at runtime when `sendMessageInternal` validates against `defaultValues.size()`.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendAsyncMessage$`

**Example:**
```javascript:attach-to-other-broadcaster
// Title: Chaining broadcasters with argument transformation
const var source = Engine.createBroadcaster({
    "id": "Source",
    "args": ["x", "y"]
});

const var target = Engine.createBroadcaster({
    "id": "Target",
    "args": ["sum"]
});

var log = [];

target.addListener("logger", "sumLog", function(sum)
{
    log.push(sum);
});

inline function transformArgs(x, y)
{
    return [x + y];
}

target.attachToOtherBroadcaster(source, transformArgs, false, "chainSource");
source.sendSyncMessage([3, 4]);
source.sendSyncMessage([10, 20]);
```
```json:testMetadata:attach-to-other-broadcaster
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "log.length", "value": 3},
    {"type": "REPL", "expression": "log[1]", "value": 7},
    {"type": "REPL", "expression": "log[2]", "value": 30}
  ]
}
```

## attachToProcessingSpecs

**Signature:** `void attachToProcessingSpecs(var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachToProcessingSpecs("specSource");`

**Description:**
Attaches this broadcaster to the audio engine's processing specification changes. The broadcaster fires asynchronously whenever the sample rate or buffer size changes (e.g., when the DAW changes its audio settings or during offline rendering). The broadcaster must have exactly 2 arguments with semantics `(sampleRate, blockSize)`.

Internally subscribes to `MainController::specBroadcaster`, a `LambdaBroadcaster<double, int>`. When the audio engine calls `prepareToPlay`, the `ProcessingSpecSource` stores the new values and calls `sendAsyncMessage` on this broadcaster.

This method does not provide initial values on attachment (`getNumInitialCalls()` returns 0), so listeners will not receive the current sample rate and block size until the next `prepareToPlay` cycle. To get initial values, call `resendLastMessage` after attaching.

Queue mode is explicitly disabled by this method (`enableQueue = false`), overriding any prior `setEnableQueue(true)` call.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| optionalMetadata | NotUndefined | no | Source metadata -- a string ID or JSON object | Non-empty string or object with `id` |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. A mismatch produces a runtime error: "If you want to attach a broadcaster to processing specs, it needs two parameters (sampleRate, blockSize)".
- No initial values are dispatched on attachment. Listeners added before or after this call will not receive the current sample rate and block size until the audio engine reinitializes. Use `resendLastMessage` if you need immediate state.
- This method explicitly sets `enableQueue = false`, which will override a prior `setEnableQueue(true)` call. This is intentional because processing spec changes are infrequent and coalescing is appropriate.

**Cross References:**
- `$API.Broadcaster.attachToNonRealtimeChange$`
- `$API.Broadcaster.resendLastMessage$`

**Example:**
```javascript:attach-processing-specs
// Title: Monitoring sample rate and buffer size changes
const var bc = Engine.createBroadcaster({
    "id": "SpecMonitor",
    "args": ["sampleRate", "blockSize"]
});

var lastRate = 0;
var lastBlock = 0;

bc.addListener("handler", "specHandler", function(sampleRate, blockSize)
{
    lastRate = sampleRate;
    lastBlock = blockSize;
});

bc.attachToProcessingSpecs("specSource");
```
```json:testMetadata:attach-processing-specs
{
  "testable": false,
  "skipReason": "Processing spec changes require audio engine reinitialization which cannot be triggered programmatically from script"
}
```

## attachToRadioGroup

**Signature:** `void attachToRadioGroup(int radioGroupIndex, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachToRadioGroup(1, "radioSource");`

**Description:**
Attaches this broadcaster to a radio button group. The broadcaster fires whenever a button in the specified radio group is clicked, sending the zero-based index of the selected button within the group. The broadcaster must have exactly 1 argument with semantics `(selectedIndex)`.

On attachment, the constructor scans all UI components for buttons whose `radioGroup` property matches the given index. If any button is currently on (has a truthy value), its position in the discovered list becomes the initial `currentIndex`. If no button is on but the broadcaster's default value is valid, that default is used as the initial index.

When a radio group button is clicked, the broadcaster's `call()` method receives the click via the `CallableObject` interface. It looks up the clicked button in its internal list and dispatches the index asynchronously via `sendAsyncMessage`. The `RadioGroupListener` also provides one initial call dispatching `currentIndex` to all listeners on attachment.

The function-call syntax on the broadcaster (`bc(button, isOn)`) has special handling when a `RadioGroupListener` is attached: it interprets the first argument as the clicked button and the second as the on/off state, looking up the button index internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| radioGroupIndex | Integer | yes | The radio group index to monitor | Must be > 0 and match at least one component's `radioGroup` property |
| optionalMetadata | NotUndefined | no | Source metadata -- a string ID or JSON object | Non-empty string or object with `id` |

**Pitfalls:**
- Passing `radioGroupIndex` of 0 produces a runtime error: "illegal radio group index 0". Radio group indices must be positive integers.
- If no components have the specified `radioGroup` property value, a runtime error is thrown: "No buttons with radio group N found".
- The broadcaster must have exactly 1 argument. Unlike other attach methods, this is not validated at the `attachToRadioGroup` call site -- instead, a mismatch will cause issues downstream when `sendAsyncMessage` validates argument count.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.sendAsyncMessage$`

**Example:**
```javascript:attach-to-radio-group
// Title: Monitoring radio button group selection
// --- setup ---
const var Btn1 = Content.addButton("Btn1", 0, 0);
Btn1.set("radioGroup", 1);
Btn1.set("saveInPreset", false);
const var Btn2 = Content.addButton("Btn2", 130, 0);
Btn2.set("radioGroup", 1);
Btn2.set("saveInPreset", false);
const var Btn3 = Content.addButton("Btn3", 260, 0);
Btn3.set("radioGroup", 1);
Btn3.set("saveInPreset", false);
Btn1.setValue(1);
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "RadioGroup1",
    "args": ["selectedIndex"]
});

var selectedLog = [];

bc.addListener("logger", "radioLog", function(selectedIndex)
{
    selectedLog.push(selectedIndex);
});

bc.attachToRadioGroup(1, "radioSource");
```
```json:testMetadata:attach-to-radio-group
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "selectedLog.length", "value": 1},
    {"type": "REPL", "expression": "selectedLog[0]", "value": 0}
  ]
}
```

## attachToRoutingMatrix

**Signature:** `void attachToRoutingMatrix(var moduleIds, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachToRoutingMatrix("SimpleGain1", "matrixSource");`

**Description:**
Attaches this broadcaster to the routing matrix of one or more processors. The broadcaster fires asynchronously whenever the routing matrix configuration changes (channels are connected or disconnected). The broadcaster must have exactly 2 arguments with semantics `(processorId, matrix)`.

The `processorId` argument receives the string ID of the processor whose matrix changed. The `matrix` argument receives a `ScriptRoutingMatrix` object that provides scripting access to the routing configuration.

Internally, a `MatrixListener` (a `SafeChangeListener`) is registered on each processor's `RoutableProcessor::getMatrix()`. When the matrix changes, the listener sends the processor ID and the `ScriptRoutingMatrix` wrapper via `sendAsyncMessage`.

Queue mode is automatically enabled after attachment, which ensures that rapid matrix changes from multiple processors are all dispatched rather than coalesced.

On attachment, initial values are dispatched for each monitored processor -- one call per processor with its current ID and matrix object.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| moduleIds | NotUndefined | no | Processor ID string or array of processor ID strings | Each must reference a processor with a routing matrix (RoutableProcessor) |
| optionalMetadata | NotUndefined | no | Source metadata -- a string ID or JSON object | Non-empty string or object with `id` |

**Pitfalls:**
- The broadcaster must have exactly 2 arguments. A mismatch produces a runtime error: "If you want to attach a broadcaster to a routing matrix, it needs two parameters (processorId, matrix)".
- If a specified module does not have a routing matrix (is not a `RoutableProcessor`), a runtime error is thrown: "the modules must have a routing matrix". Effects and sound generators have routing matrices; modulators do not.
- Queue mode is forced on by this method. If you previously called `setEnableQueue(false)`, this attachment overrides it.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.RoutingMatrix$`

**Example:**
```javascript:attach-to-routing-matrix
// Title: Monitoring routing matrix changes on a processor
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.Effects.SimpleGain, "SimpleGain1", 0, builder.ChainIndexes.FX);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "MatrixWatch",
    "args": ["processorId", "matrix"]
});

var lastProcessorId = "";

bc.addListener("handler", "matrixHandler", function(processorId, matrix)
{
    lastProcessorId = processorId;
});

bc.attachToRoutingMatrix("SimpleGain1", "matrixSource");
```
```json:testMetadata:attach-to-routing-matrix
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastProcessorId", "value": "SimpleGain1"}
  ]
}
```

## attachToSampleMap

**Signature:** `void attachToSampleMap(var samplerIds, var eventTypes, var optionalMetadata)`
**Return Type:** `undefined`
**Call Scope:** init
**Minimal Example:** `{obj}.attachToSampleMap("Sampler1", "SampleMapChanged", "smSource");`

**Description:**
Attaches this broadcaster to samplemap events on one or more sampler modules. The broadcaster fires asynchronously when the specified events occur on any of the monitored samplers. The broadcaster must have exactly 3 arguments with semantics `(eventType, samplerId, data)`.

The `eventType` argument receives a string identifying the event kind. The `samplerId` argument receives the string ID of the sampler that triggered the event. The `data` argument varies by event type:

- **SampleMapChanged**: `data` is the samplemap reference string (e.g., `"{PROJECT_FOLDER}MySampleMap.xml"`), or an empty string when the samplemap is cleared.
- **SamplesAddedOrRemoved**: `data` is the integer count of sounds currently in the sampler.
- **SampleChanged**: `data` is a JSON object with properties `sound` (a `ScriptingSamplerSound` reference), `id` (the integer property index), and `value` (the new property value).

The `eventTypes` parameter accepts string values `"SampleMapChanged"` or `"SamplesAddedOrRemoved"`, or integer values for `SampleChanged` events (where the integer is an index into the `SampleIds` list identifying which sample property to monitor). Multiple event types can be passed as an array.

Queue mode is automatically enabled when monitoring multiple samplers, multiple event types, or any `SampleChanged` events.

On attachment, initial values are dispatched for each sampler: one call each for `SampleMapChanged` (with the current samplemap reference) and `SamplesAddedOrRemoved` (with the current sound count), if those event types are registered.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| samplerIds | NotUndefined | no | Sampler module ID string or array of sampler ID strings | Must reference ModulatorSampler processors |
| eventTypes | NotUndefined | no | Event type string, integer, or array of event type strings/integers | Valid strings: `"SampleMapChanged"`, `"SamplesAddedOrRemoved"`. Integers index into `SampleIds` for `SampleChanged` |
| optionalMetadata | NotUndefined | no | Source metadata -- a string ID or JSON object | Non-empty string or object with `id` |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "SampleMapChanged" | Fires when the samplemap is loaded, swapped, or cleared. Data is the samplemap reference string |
| "SamplesAddedOrRemoved" | Fires when samples are added to or removed from the sampler. Data is the integer sound count |

**Callback Properties:**

| Property | Type | Description |
|----------|------|-------------|
| sound | ScriptingSamplerSound | Reference to the changed sample (SampleChanged events only) |
| id | Integer | The sample property index from SampleIds (SampleChanged events only) |
| value | var | The new property value (SampleChanged events only) |

**Pitfalls:**
- The broadcaster must have exactly 3 arguments. A mismatch produces a runtime error: "If you want to attach a broadcaster to a samplemap, it needs three parameters (samplerId, eventType, data)".
- The `SampleChanged` event type cannot be specified by string -- it requires an integer index into the `SampleIds` list. Passing the string `"SampleChanged"` does not match any event type and will produce an error.
- [BUG] If a specified module is not a `ModulatorSampler`, a runtime error is thrown. The error message for single-module mode incorrectly says "the modules must have a routing matrix" instead of "the modules must be samplers" -- a copy-paste error from `attachToRoutingMatrix`.
- Invalid event type values (strings not matching `"SampleMapChanged"` or `"SamplesAddedOrRemoved"`, or out-of-range integers) cause an error: "unknown eventTypes: ...".

**Cross References:**
- `$API.Broadcaster.addListener$`

**Example:**
```javascript:attach-to-samplemap
// Title: Monitoring samplemap load events on a sampler
// --- setup ---
const var builder = Synth.createBuilder();
builder.clear();
builder.create(builder.SoundGenerators.StreamingSampler, "Sampler1", 0, builder.ChainIndexes.Direct);
builder.flush();
// --- end setup ---

const var bc = Engine.createBroadcaster({
    "id": "SampleMapWatch",
    "args": ["eventType", "samplerId", "data"]
});

var eventLog = [];

bc.addListener("logger", "smLog", function(eventType, samplerId, data)
{
    eventLog.push(eventType + ":" + samplerId);
});

bc.attachToSampleMap("Sampler1", ["SampleMapChanged", "SamplesAddedOrRemoved"], "smSource");
```
```json:testMetadata:attach-to-samplemap
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "eventLog.length", "value": 2}
  ]
}
```

## callWithDelay

**Signature:** `void callWithDelay(int delayInMilliseconds, var argArray, var function)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new `DelayedFunction` timer object (heap allocation), acquires `delayFunctionLock` (CriticalSection).
**Minimal Example:** `{obj}.callWithDelay(500, [1, 2], onDelayedCall);`

**Description:**
Calls a function after a specified delay in milliseconds. This is a standalone delayed call mechanism that does NOT go through the broadcaster's listener system -- it directly invokes the provided function with the given arguments after the delay expires.

This method has exclusive replacement semantics: if a new delayed call is scheduled while a previous one is still pending, the previous call is cancelled (its timer is stopped) and replaced by the new one. Only one delayed function can be pending at a time per broadcaster. This makes it suitable for debouncing patterns where only the most recent action should fire.

The `argArray` must be an array. Its elements are passed as positional arguments to the function when the timer fires. The function is wrapped in a `WeakCallbackHolder` with 0 expected args (the args are passed via the raw `call()` method).

When the timer fires, it checks the broadcaster's bypass state -- if bypassed, the callback is silently skipped. The callback executes under the `delayFunctionLock` to prevent concurrent replacement.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| delayInMilliseconds | Integer | yes | Delay before the function executes, in milliseconds | Must be a positive integer |
| argArray | Array | no | Array of arguments to pass to the function | Must be an array |
| function | Function | no | The function to call after the delay | Must be a valid JavaScript function |

**Callback Signature:** function(...args: var)

**Pitfalls:**
- This method does NOT dispatch through the listener system. Listeners registered via `addListener` will NOT receive anything from `callWithDelay`. It is a standalone timer-based function call mechanism.
- Passing a non-array value for `argArray` produces a runtime error: "argArray must be an array". Even for a single argument, wrap it in an array: `[value]`.
- If the function is not a valid JavaScript function (e.g., passing `false` or a number), the `DelayedFunction` is not created and the call silently does nothing (no error thrown). The old pending function is still stopped.
- Each new call to `callWithDelay` cancels any previously pending delayed function. There is no way to queue multiple delayed calls through this mechanism.

**Cross References:**
- `$API.Broadcaster.sendMessageWithDelay$`
- `$API.Broadcaster.addDelayedListener$`
- `$API.Broadcaster.setBypassed$`

**Example:**
```javascript:call-with-delay-debounce
// Title: Debouncing rapid updates with callWithDelay
const var bc = Engine.createBroadcaster({
    "id": "Debouncer",
    "args": ["value"]
});

var result = -1;

inline function onDelayed(x)
{
    result = x;
}

// Only the last call within the delay window fires
bc.callWithDelay(100, [42], onDelayed);
bc.callWithDelay(100, [99], onDelayed);
```
```json:testMetadata:call-with-delay-debounce
{
  "testable": false,
  "skipReason": "callWithDelay causes HISE Debug crash during validation - suspected debug-only assertion in DelayedFunction timer callback"
}
```

## isBypassed

**Signature:** `bool isBypassed()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var bypassed = {obj}.isBypassed();`

**Description:**
Returns whether the broadcaster is currently bypassed. When bypassed, `sendMessageInternal` stores new values in `lastValues` but does not dispatch them to listeners. Returns `true` if the broadcaster is bypassed, `false` otherwise.

This is a simple read of the `bypassed` boolean member with no locks or allocations.

**Parameters:**

(None)

**Cross References:**
- `$API.Broadcaster.setBypassed$`

## refreshContextMenuState

**Signature:** `void refreshContextMenuState()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Calls `stateCallback.callSync()` which invokes JavaScript callbacks; string allocations for dynamic text values.
**Minimal Example:** `{obj}.refreshContextMenuState();`

**Description:**
Refreshes the cached state values for all context menu items attached to this broadcaster via `attachToContextMenu`. This recalculates the tick state, enabled state, and dynamic text for each menu item by calling the `stateFunction` callback with type strings `"active"`, `"enabled"`, and `"text"` for each item index.

Context menu state values are cached internally and refreshed automatically after each menu selection (in `sendInternal` post-processing). Call this method explicitly when external state changes should be reflected in the menu before the user opens it -- for example, after modifying a data model that determines which items are ticked or enabled.

If no `ContextMenuListener` is attached to this broadcaster, the method iterates an empty list and silently does nothing.

**Parameters:**

(None)

**Pitfalls:**
- This method has no effect if `attachToContextMenu` has not been called first. It iterates `attachedListeners` looking for `ContextMenuListener` instances and silently skips if none are found. There is no error or warning.
- The cached states are also automatically refreshed after each context menu selection. Calling `refreshContextMenuState` is only necessary when external state changes should be visible before the next menu interaction.

**Cross References:**
- `$API.Broadcaster.attachToContextMenu$`

## sendMessageWithDelay

**Signature:** `void sendMessageWithDelay(var args, int delayInMilliseconds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Starts a JUCE Timer (heap allocation for timer registration). Stores `pendingData` as a `var` member (potential allocation). The deferred send dispatches asynchronously via `sendMessage(pendingData, false)`.
**Minimal Example:** `{obj}.sendMessageWithDelay(["ready", 1], 500);`

**Description:**
Sends a message to all listeners after a specified delay. The method stores the arguments internally and starts a JUCE `Timer`. When the timer fires, it dispatches the stored arguments asynchronously via `sendMessage(pendingData, false)`.

This method has exclusive replacement semantics: if called again before the previous timer fires, the old pending data is replaced and the timer is restarted. Only one delayed message can be pending at a time per broadcaster.

If `setForceSynchronousExecution(true)` has been called on this broadcaster, the delay is bypassed entirely and the message is sent synchronously and immediately.

Note that `sendMessageWithDelay` uses the broadcaster's own Timer (inherited via `private Timer`), which is separate from the `DelayedFunction` timer used by `callWithDelay`. They do not interfere with each other.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| args | Array | no | The message arguments to send after the delay. Must match the broadcaster's argument count. For single-argument broadcasters, pass the value directly (not wrapped in an array). | Length must equal `defaultValues.size()` |
| delayInMilliseconds | Integer | no | The delay in milliseconds before the message is dispatched. | Must be > 0 for meaningful delay |

**Pitfalls:**
- The delayed send always dispatches asynchronously (via `sendMessage(pendingData, false)`) regardless of any previous sync/async preference. The `forceSync` flag is the only way to override this -- when active, the delay is skipped entirely and the message fires synchronously and immediately.
- Calling `sendMessageWithDelay` multiple times before the timer fires replaces the pending data and restarts the timer. Only the last call's arguments are sent. This is useful for debouncing but can cause data loss if every intermediate value matters -- use `setEnableQueue(true)` and `sendAsyncMessage` instead if all values must be delivered.

**Cross References:**
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.callWithDelay$`
- `$API.Broadcaster.setForceSynchronousExecution$`

## sendSyncMessage

**Signature:** `void sendSyncMessage(var args)`
**Return Type:** `undefined`
**Call Scope:** warning
**Call Scope Note:** Safe on the scripting thread and UI thread. When called from the audio thread, requires `setRealtimeMode(true)` -- otherwise a backend-only script error is thrown. The method itself performs change detection (value comparison) and may acquire `lastValueLock` (SimpleReadWriteLock write lock) in non-realtime mode.
**Minimal Example:** `{obj}.sendSyncMessage(["ready", 1]);`

**Description:**
Sends a synchronous message to all registered listeners. This is functionally equivalent to using dot-assignment syntax (`bc.argName = value`) but allows setting all arguments at once in a single call.

The method delegates directly to `sendMessageInternal(args, true)`. The full dispatch flow is:

1. Validates argument count matches the broadcaster's definition.
2. In realtime-safe mode: updates `lastValues` directly (no lock) and calls `sendInternal` immediately.
3. In normal mode: compares new values against `lastValues`. If nothing changed (and queue is disabled and `forceSend` is false), the message is suppressed. Otherwise, acquires a write lock, updates `lastValues`, and calls `sendInternal` synchronously.
4. If the broadcaster is bypassed, `lastValues` are updated but no listener callbacks are invoked.

For single-argument broadcasters, pass the value directly (not wrapped in an array).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| args | Array | no | The message arguments. For multi-argument broadcasters, pass an array with one element per argument. For single-argument broadcasters, pass the value directly. | Length must equal the broadcaster's argument count |

**Pitfalls:**
- Identical consecutive values are silently suppressed unless queue mode is enabled via `setEnableQueue(true)`. The comparison uses `var::operator!=` which performs value comparison for primitives and reference comparison for objects. Sending the same object reference twice is suppressed even if the object's properties have changed.
- In the HISE IDE (backend), calling this from the audio thread without `setRealtimeMode(true)` throws a script error. In exported plugins (frontend), no such check exists -- the send proceeds but may cause priority inversion or unsafe operations.

**Cross References:**
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.sendMessageWithDelay$`
- `$API.Broadcaster.setRealtimeMode$`
- `$API.Broadcaster.setEnableQueue$`

**Example:**
```javascript:sync-message-basic
// Title: Synchronous message dispatch with listener verification
const var bc = Engine.createBroadcaster({
    "id": "StatusBc",
    "args": ["status", "count"]
});

var lastStatus = "";
var lastCount = 0;

bc.addListener("logger", "Captures status",
function(status, count)
{
    lastStatus = status;
    lastCount = count;
});

bc.sendSyncMessage(["active", 5]);
```
```json:testMetadata:sync-message-basic
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lastStatus", "value": "active"},
    {"type": "REPL", "expression": "lastCount", "value": 5},
    {"type": "REPL", "expression": "bc.status", "value": "active"},
    {"type": "REPL", "expression": "bc.count", "value": 5}
  ]
}
```

## setBypassed

**Signature:** `void setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** When `sendMessageIfEnabled` is true and the broadcaster is being unbypassed, `resendLastMessage` is called which may acquire `lastValueLock` (write lock), post async jobs to the JavascriptThreadPool, and invoke listener callbacks with potential string/object allocations.
**Minimal Example:** `{obj}.setBypassed(true, false, false);`

**Description:**
Controls the bypass state of the broadcaster. When bypassed, `sendMessageInternal` stores new `lastValues` but skips listener dispatch. New targets added via `addListener` (etc.) are also not initialized with current values while bypassed.

The three parameters interact as follows:

- `shouldBeBypassed`: Set `true` to bypass, `false` to unbypass. If the new state equals the current state, the method returns immediately with no action.
- `sendMessageIfEnabled`: Only relevant when unbypassing (transitioning from bypassed to active). When `true`, the broadcaster resends its last stored values to all listeners, ensuring they synchronize with any values that arrived during the bypass period.
- `async`: Controls the dispatch mode of the resend when unbypassing. **Warning:** Despite the parameter name, this value is passed directly to `resendLastMessage(var sync)` which interprets it via `ApiHelpers::isSynchronous()`. Due to the boolean fallback path, `true` means **synchronous** and `false` means **asynchronous**. Use `SyncNotification` or `AsyncNotification` constants for clarity.

The `bypass()` scoped statement in HiseScript provides a RAII alternative: the broadcaster is bypassed for the duration of the block and automatically restored on exit.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldBeBypassed | Integer | no | `true` to bypass, `false` to unbypass. | Boolean |
| sendMessageIfEnabled | Integer | no | When `true` and unbypassing, resends last values to all listeners. | Boolean |
| async | Integer | no | Dispatch mode for the resend. Despite the name, `true` = synchronous, `false` = asynchronous. Prefer `SyncNotification`/`AsyncNotification` constants. | Boolean or notification constant |

**Pitfalls:**
- [BUG] The `async` parameter name is inverted relative to its behavior. Passing `true` causes synchronous dispatch, and `false` causes asynchronous dispatch. This is because the value is passed directly to `resendLastMessage(var sync)` where `ApiHelpers::isSynchronous()` interprets `true` as synchronous. Use `SyncNotification` or `AsyncNotification` constants to make intent explicit and avoid confusion.
- Bypassing does not discard incoming values. While bypassed, `sendMessageInternal` still updates `lastValues` with new data -- it just skips the dispatch to listeners. When unbypassed with `sendMessageIfEnabled = true`, only the most recent values are sent, not a history of all values received during bypass.
- The bypass state is not checked during the realtime-safe fast path's value update in `sendMessageInternal`. Values are written to `lastValues` before the bypass check, so `lastValues` always reflects the most recent send attempt regardless of bypass state.

**Cross References:**
- `$API.Broadcaster.isBypassed$`
- `$API.Broadcaster.resendLastMessage$`

**Example:**
```javascript:bypass-and-resume
// Title: Bypass suppresses messages, unbypass resends last values
const var bc = Engine.createBroadcaster({
    "id": "BypassTest",
    "args": ["value"]
});

var received = [];

bc.addListener("logger", "Tracks values",
function(value)
{
    received.push(value);
});

bc.sendSyncMessage([10]);
bc.setBypassed(true, false, false);
bc.sendSyncMessage([20]);
bc.sendSyncMessage([30]);
bc.setBypassed(false, true, true);
```
```json:testMetadata:bypass-and-resume
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "received.length", "value": 2},
    {"type": "REPL", "expression": "received[0]", "value": 10},
    {"type": "REPL", "expression": "received[1]", "value": 30}
  ]
}
```

## setEnableQueue

**Signature:** `void setEnableQueue(bool shouldUseQueue)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setEnableQueue(true);`

**Description:**
Enables or disables queue mode on the broadcaster. When queue mode is enabled, two behaviors change:

1. **Change detection is bypassed.** Messages are dispatched to listeners even when the new values are identical to the previous values. Without queue mode, sending the same values twice in a row is silently suppressed.
2. **Async coalescing is bypassed.** Without queue mode, rapid async sends are coalesced via the `asyncPending` atomic flag -- only the most recent values are dispatched when the async job executes. With queue mode, every async send captures its own `lastValues` snapshot and posts an independent job, guaranteeing delivery of every message.

Several attach methods automatically enable queue mode because their event sources may fire multiple times before the scripting thread processes:
- `attachToModuleParameter` -- multiple parameter changes may batch
- `attachToRoutingMatrix` -- matrix changes may batch
- `attachToContextMenu` -- explicitly enabled
- `attachToComplexData` -- when multiple processors or indices
- `attachToSampleMap` -- when multiple samplers, event types, or SampleChanged events

Calling `setEnableQueue(false)` after an attach method that auto-enables it will disable queue mode, but this may cause missed events if the source fires rapidly.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldUseQueue | Integer | no | `true` to enable queue mode, `false` to disable. | Boolean |

**Pitfalls:**
- Queue mode affects both change detection and async coalescing. Enabling it for a broadcaster that sends frequently via `sendAsyncMessage` may cause a large backlog of jobs on the `JavascriptThreadPool`, since each send posts an independent job instead of coalescing.
- Disabling queue mode on a broadcaster where an attach method auto-enabled it can cause silent message loss if the event source fires multiple times between scripting thread ticks.

**Cross References:**
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.attachToModuleParameter$`
- `$API.Broadcaster.setForceSynchronousExecution$`
- `$API.Broadcaster.setRealtimeMode$`

## setForceSynchronousExecution

**Signature:** `void setForceSynchronousExecution(bool shouldExecuteSynchronously)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setForceSynchronousExecution(true);`

**Description:**
Forces all message dispatch to execute synchronously, regardless of whether `sendAsyncMessage`, `sendMessageWithDelay`, or an async attach source initiated the send. When enabled, the `forceSync` flag overrides the `isSync` parameter in `sendMessageInternal`, causing every send path to behave as synchronous.

This affects multiple entry points:
- `sendAsyncMessage` -- becomes synchronous
- `sendMessageWithDelay` -- the delay is bypassed entirely and the message is sent synchronously and immediately
- `resendLastMessage` -- the async option is forced to synchronous
- Attach source callbacks that normally dispatch asynchronously -- forced to synchronous
- `sendSyncMessage` -- no change (already synchronous)

This is automatically enabled by `addModuleParameterSyncer` to ensure module parameter synchronization happens immediately without async delays.

Setting this to `true` on a broadcaster that receives messages from the audio thread requires `setRealtimeMode(true)` as well, otherwise the backend safety check will throw a script error when attempting synchronous dispatch from the audio thread.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldExecuteSynchronously | Integer | no | `true` to force all sends to be synchronous, `false` to allow normal sync/async behavior. | Boolean |

**Pitfalls:**
- Enabling force-synchronous execution on a broadcaster that receives async events from multiple sources can cause long callback chains on the calling thread. If the callbacks are expensive, this blocks the thread that triggered the event (which may be the audio thread if realtime mode is also enabled).

**Cross References:**
- `$API.Broadcaster.setRealtimeMode$`
- `$API.Broadcaster.setEnableQueue$`
- `$API.Broadcaster.sendAsyncMessage$`
- `$API.Broadcaster.sendMessageWithDelay$`
- `$API.Broadcaster.addModuleParameterSyncer$`

## setRealtimeMode

**Signature:** `void setRealtimeMode(bool enableRealTimeMode)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setRealtimeMode(true);`

**Description:**
Enables or disables realtime-safe mode on the broadcaster. When enabled, synchronous sends take a lock-free fast path:

- `sendMessageInternal` skips the `SimpleReadWriteLock` write lock and updates `lastValues` directly
- `sendInternal` skips the per-target read lock and iterates items without copying args
- The `isRealtimeSafe()` method returns `true`, which is checked by the runtime safety analysis system and by listener validation

This mode is required for any broadcaster that receives synchronous messages from the audio thread. In the HISE IDE (backend), attempting a synchronous send from the audio thread without realtime mode throws a script error. In exported plugins, no check exists but the non-realtime path involves locks that risk priority inversion.

When `addListener` is called on a realtime-safe broadcaster, callback functions are validated:
- In backend: `RealtimeSafetyInfo::check()` validates the callback for audio-thread safety
- In frontend: only inline functions are accepted (checked via the function's `isRealtimeSafe()` flag)

This is automatically enabled by `attachToNonRealtimeChange()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| enableRealTimeMode | Integer | no | `true` to enable lock-free realtime-safe dispatch, `false` to use normal locked dispatch. | Boolean |

**Pitfalls:**
- Realtime mode removes thread-safety protections (locks). If the broadcaster is accessed from multiple threads simultaneously (e.g., audio thread sends while UI thread adds a listener), there is no synchronization. This mode assumes the broadcaster's listener list is stable and only modified during initialization.
- The undefined-argument check in `sendInternal` unconditionally acquires a `SimpleReadWriteLock::ScopedReadLock` before the realtime-safe branch. This is a lightweight atomic operation (not a blocking mutex), so it does not break audio-thread safety, but it means even realtime-safe sends incur atomic read-lock overhead for the undefined check.

**Cross References:**
- `$API.Broadcaster.setForceSynchronousExecution$`
- `$API.Broadcaster.setEnableQueue$`
- `$API.Broadcaster.attachToNonRealtimeChange$`
- `$API.Broadcaster.addListener$`

## setReplaceThisReference

**Signature:** `void setReplaceThisReference(bool shouldReplaceThisReference)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setReplaceThisReference(false);`

**Description:**
Controls whether the `obj` parameter passed to `addListener` replaces the `this` reference inside the listener callback function. The default is `true`, meaning that `this` inside the callback refers to the object passed as the first argument to `addListener` rather than the original script scope.

When set to `false`, the intention is that `this` retains its original binding (the script scope or namespace where the callback was defined). This would be useful when callbacks are methods on a script object and should access the object's own properties via `this`.

**Note:** In the current implementation, this flag is stored internally but never consulted during callback dispatch. `ScriptTarget::callSync` always uses the `obj` from `addListener` as the `this` reference in `var::NativeFunctionArgs(obj, ...)`. Calling `setReplaceThisReference(false)` has no effect on runtime behavior.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldReplaceThisReference | Integer | no | `true` (default) to replace `this` with the listener object, `false` to keep original `this` binding. Currently has no effect. | Boolean |

**Pitfalls:**
- [BUG] The `replaceThisReference` member is set by this method but never read by any dispatch code. `ScriptTarget::callSync` unconditionally uses the `obj` parameter from `addListener` as the `this` reference. Setting this to `false` is a no-op.

**Cross References:**
- `$API.Broadcaster.addListener$`

## setSendMessageForUndefinedArgs

**Signature:** `void setSendMessageForUndefinedArgs(bool shouldSendWhenUndefined)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setSendMessageForUndefinedArgs(true);`

**Description:**
Controls whether listeners are initialized with current values when any of the broadcaster's arguments are still undefined. By default, when a new listener is added (via `addListener`, `addComponentPropertyListener`, etc.), the broadcaster calls the listener immediately with the current `lastValues` to synchronize state. If any `lastValues` entry is undefined or void, this initialization call is skipped -- the listener receives no callback until the first real message with fully defined values.

Setting this to `true` forces the initialization call to proceed even when some arguments are undefined. This is useful when:
- The broadcaster has multiple arguments and some are intentionally undefined at init time
- Listener callbacks are designed to handle undefined arguments gracefully
- You need the listener to run its initialization logic regardless of the current broadcaster state

**Important scope limitation:** This flag ONLY affects the `initItem()` path -- the initialization call when a new listener is added to a broadcaster that has no attached sources. It does NOT affect `sendInternal()`, which has a hardcoded undefined check that always returns early (skipping all listener callbacks) if any argument is undefined or void. Explicitly calling `sendSyncMessage` or `sendAsyncMessage` with undefined arguments is always suppressed, regardless of this setting.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| shouldSendWhenUndefined | Integer | no | `true` to allow listener initialization when arguments are undefined, `false` (default) to skip initialization. | Boolean |

**Pitfalls:**
- This flag does not affect `sendInternal()`. Calling `sendSyncMessage` or `sendAsyncMessage` with undefined arguments is always silently suppressed regardless of this setting. Only the automatic initialization of newly-added listeners is affected.
- When attached sources are present (via any `attachTo*` method), the initialization path uses the source's `getInitialArgs()` instead of `lastValues`, so this flag has no effect in that case. The flag is only relevant for broadcasters without attached sources.

**Cross References:**
- `$API.Broadcaster.addListener$`
- `$API.Broadcaster.sendSyncMessage$`
- `$API.Broadcaster.sendAsyncMessage$`

