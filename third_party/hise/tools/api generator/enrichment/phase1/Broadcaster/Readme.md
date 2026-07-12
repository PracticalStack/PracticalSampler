# Broadcaster -- Class Analysis

## Brief
Structured event system for multi-source, multi-target message passing with metadata, priority, and debug visualization.

## Purpose
Broadcaster is a reactive event hub that connects event sources (component value/property changes, module parameter changes, mouse events, complex data updates, and more) to event targets (script callbacks, component property setters, component value setters, refresh triggers, module parameter syncers, and other broadcasters). Messages carry a fixed number of named arguments defined at creation time. Sources are attached via `attachTo*` methods; targets are registered via `add*Listener` / `add*` methods. Each source and target carries metadata (id, colour, priority, tags) used for identification, priority-sorted execution order, and visual debugging in the BroadcasterMap panel. Change detection suppresses duplicate messages by default, with queue mode available for guaranteed delivery.

## Details

### Architecture

Broadcaster maintains two internal collections: **sources** (`attachedListeners` -- things that detect external changes and push messages into the broadcaster) and **targets** (`items` -- things that receive messages when the broadcaster fires). Both derive from `ItemBase`, which carries the metadata system.

The message flow is: external event -> source internal listener -> `sendAsyncMessage`/`sendSyncMessage` on the broadcaster -> `sendInternal` iterates targets -> each target `callSync` executes.

### Argument System

A broadcaster is created with a fixed set of named arguments. Four creation formats are accepted by `Engine.createBroadcaster()`:

| Format | Example | Result |
|--------|---------|--------|
| JSON with `id` and `args` | `{ id: "myBc", args: ["component", "value"] }` | Named args, defaults to undefined, metadata extracted from JSON |
| JSON with named properties | `{ component: 0, value: 0 }` | Property names become arg IDs, property values become defaults |
| Array of strings | `["component", "value"]` | String elements become arg IDs, defaults to undefined |
| Single value | `0` | One unnamed argument with the given default |

The argument names become dot-accessible properties: `bc.value = 42` triggers a synchronous send, and `var x = bc.value` reads the last value.

### Callable Object Interface

Broadcaster implements `WeakCallbackHolder::CallableObject`, allowing it to be called like a function: `bc(componentRef, 42)`. When no sources are attached, the function-call syntax sends synchronously; when sources are attached, it sends asynchronously.

### Metadata System

Both sources and targets are identified by metadata. Metadata can be a string (becomes the id) or a JSON object with these properties:

| Property | Type | Default | Purpose |
|----------|------|---------|---------|
| `id` | String | (required) | Identifier for removal and debug display |
| `comment` | String | `""` | Description shown in BroadcasterMap |
| `colour` | int | auto-generated from hash | Display colour (-1 for auto) |
| `priority` | int | `0` | Execution order (higher = earlier) |
| `tags` | Array | `[]` | Tag identifiers for filtering |
| `visible` | bool | `true` | Whether shown in BroadcasterMap |

Targets are sorted by descending priority -- higher priority values execute first.

### Change Detection and Undefined Suppression

By default, the broadcaster suppresses messages when no argument values have changed since the last send. This uses `var::operator!=`, which performs value comparison for primitives and reference comparison for objects. Queue mode (`setEnableQueue`) disables this suppression.

Additionally, if any argument value is `undefined`, the message is suppressed entirely and no callbacks fire. This can be overridden with `setSendMessageForUndefinedArgs(true)`.

### Threading Model

| Mode | Mechanism | Lock behavior |
|------|-----------|--------------|
| Synchronous | Executes on calling thread | Read lock on `lastValueLock` per target |
| Asynchronous | Posts `HiPriorityCallbackExecution` to `JavascriptThreadPool` | Write lock to update `lastValues`, then queued |
| Realtime-safe | Synchronous without locks | No locks, no value comparison, direct iteration |

Rapid async sends are coalesced via an `asyncPending` atomic gate (only the latest state is delivered) unless queue mode is enabled.

### Bypass System

When bypassed, the broadcaster stores new values but does not dispatch to targets. Unbypassing with `sendMessageIfEnabled = true` resends the last stored values. The `bypass()` HiseScript scoped statement provides RAII-style bypass/unbypass within a code block.

### Source Argument Count Requirements

Each `attachTo*` method requires the broadcaster to have been created with a specific number of arguments:

| Source Method | Required Args | Semantics |
|---------------|---------------|-----------|
| `attachToComponentProperties` | 3 | (component, propertyId, value) |
| `attachToComponentValue` | 2 | (component, value) |
| `attachToComponentVisibility` | 2 | (id, isVisible) |
| `attachToInterfaceSize` | 2 | (width, height) |
| `attachToComponentMouseEvents` | 2 | (component, event) |
| `attachToContextMenu` | 2 | (component, menuItemIndex) |
| `attachToModuleParameter` | 3 | (processorId, parameterId, value) |
| `attachToRadioGroup` | 1 | (selectedIndex) |
| `attachToComplexData` | 3 | (processorId, index, value) |
| `attachToEqEvents` | 2 | (eventType, value) |
| `attachToRoutingMatrix` | 2 | (processorId, matrix) |
| `attachToProcessingSpecs` | 2 | (sampleRate, blockSize) |
| `attachToNonRealtimeChange` | 1 | (isNonRealtime) |
| `attachToSampleMap` | 3 | (eventType, samplerId, data) |
| `attachToOtherBroadcaster` | -- | Inherits source broadcaster arg count |

### Initial Value Dispatch

When a source is attached, it immediately fires its current state to all already-registered targets. When a new target is added and sources are already attached, those sources immediately fire their current state to the new target. This ensures UI state is synchronized at init time without manual resend calls.

### Queue Mode

When enabled, queue mode changes three behaviors: (1) all messages are dispatched even if values have not changed, (2) the async coalescing gate is bypassed so every message is delivered, and (3) each queued message captures its own value snapshot. Queue mode is automatically enabled by certain attach methods (`attachToModuleParameter`, `attachToRoutingMatrix`, `attachToContextMenu`, `attachToComplexData` with multiple targets, `attachToSampleMap` with multiple samplers or event types).

### Target Types

| Target | Created by | Behavior |
|--------|------------|----------|
| ScriptTarget | `addListener` | General callback; `obj` replaces `this` in callback (configurable via `setReplaceThisReference`) |
| DelayedItem | `addDelayedListener` | Deferred callback; fires after N ms using latest `lastValues` |
| ComponentPropertyItem | `addComponentPropertyListener` | Sets component properties directly, or via transform callback |
| ComponentValueItem | `addComponentValueListener` | Sets component values directly, or via transform callback |
| ComponentRefreshItem | `addComponentRefreshListener` | Triggers repaint/changed/loseFocus/etc. on components |
| ModuleParameterSyncer | `addModuleParameterSyncer` | Sets module parameter from last broadcast arg; forces sync execution |
| OtherBroadcasterTarget | `attachToOtherBroadcaster` | Forwards messages to another broadcaster with optional arg transform |

## obtainedVia
`Engine.createBroadcaster(defaultValues)`

## minimalObjectToken
bc

## Constants
(None -- Broadcaster has no `addConstant` calls in its constructor.)

## Dynamic Constants
(None)

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a broadcaster with wrong arg count for the intended attach method (e.g. 3 args then calling `attachToComponentValue` which needs 2) | Match the broadcaster arg count to the attach method requirements (see Source Argument Count table in Details) | Each `attachTo*` method validates `defaultValues.size()` and throws a script error if the count does not match. The error is clear at runtime, but the mismatch is a common setup mistake. |
| Passing an empty string `""` as metadata where `mustBeValid` is true | Use a non-empty string like `"myListener"` or a JSON object `{ id: "myListener" }` | Empty metadata strings fail validation in methods that require valid metadata. Use a non-empty identifier string or a JSON object with an `id` property. |
| Mutating an object in-place and resending, expecting change detection to trigger | Create a new object instance or enable queue mode via `setEnableQueue(true)` | Change detection uses `var::operator!=`, which is reference-based for objects. Mutating properties on the same object reference does not count as a change. |
| Sending a message with `undefined` arguments and expecting callbacks to fire | Call `setSendMessageForUndefinedArgs(true)` before sending | By default, if any argument is `undefined`, the message is silently suppressed with no callbacks and no error. |
| Providing a callback to `addComponentPropertyListener` or `addComponentValueListener` with the wrong arg count or no return value | The callback receives `targetIndex` as an extra first argument (N+1 total) and must return the value to set | Forgetting the extra `targetIndex` parameter shifts all arguments. Forgetting the return value triggers "You need to return a value". |

## codeExample
```javascript
// Create a broadcaster with two named arguments
const var bc = Engine.createBroadcaster({
    id: "myBroadcaster",
    args: ["component", "value"]
});

// Attach to component value changes as source
bc.attachToComponentValue(["Knob1", "Knob2"], "knobSource");

// Register a listener as target
bc.addListener("", "valueLogger", function(component, value)
{
    Console.print(component + ": " + value);
});
```

## Alternatives
- `GlobalCable` -- for simple normalised value routing across processors without structured arguments or metadata. Better when you need a single value channel with range mapping.
- `Timer` -- for periodic polling at a fixed interval. Use when you need time-based updates rather than event-driven reactions.

## Related Preprocessors
None.

## Diagrams

### source-target-flow
- **Brief:** Source-Target Message Flow
- **Type:** topology
- **Description:** A Broadcaster sits at the center of a hub topology. On the left side, source attachments (ListenerBase subclasses such as ComponentValueListener, ModuleParameterListener, MouseEventListener, etc.) detect external changes and push messages into the broadcaster via sendAsyncMessage or sendSyncMessage. On the right side, target items (TargetBase subclasses such as ScriptTarget, ComponentPropertyItem, ComponentValueItem, ComponentRefreshItem, ModuleParameterSyncer, OtherBroadcasterTarget) receive messages via callSync when sendInternal iterates the sorted items array. The broadcaster lastValues array sits in the middle, updated by sources (with change detection) and read by targets. Metadata with priority values determines the target execution order (descending sort).

### threading-dispatch
- **Brief:** Sync vs Async Dispatch Paths
- **Type:** timing
- **Description:** When a message arrives at sendMessageInternal, two paths diverge. The sync path acquires a read lock on lastValueLock and calls sendInternal directly on the calling thread, iterating targets sequentially. The async path checks the asyncPending atomic gate (skipping if already pending, unless queue mode is enabled), posts a HiPriorityCallbackExecution job to JavascriptThreadPool, and returns immediately. The queued job later acquires a read lock and calls sendInternal on the scripting thread. In realtime-safe mode, both locks are skipped entirely and sendInternal runs without any synchronization.

## Diagnostic Ideas
Reviewed: Yes
Count: 5
- Broadcaster.attachToComplexData -- precondition: broadcaster arg count mismatch (logged)
- Broadcaster.addComponentRefreshListener -- value-check: invalid refreshType string (logged)
- Broadcaster.attachToComponentMouseEvents -- value-check: invalid callbackLevel string (logged)
- Broadcaster.attachToEqEvents -- value-check: invalid event type string (logged)
- Broadcaster.refreshContextMenuState -- timeline-dependency: no-op without prior attachToContextMenu (logged)
