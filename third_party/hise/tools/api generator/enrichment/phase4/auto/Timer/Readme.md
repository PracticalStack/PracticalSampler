<!-- Diagram triage:
  - (no diagrams specified in Phase 1 data)
-->

# Timer

Timer is a standalone periodic callback object created with `Engine.createTimerObject()`. It fires a user-defined function at a fixed millisecond interval on the message thread, making it suitable for UI polling, deferred actions, and periodic state checks - but not for sample-accurate or audio-thread work.

Common use cases include:

- Polling audio levels for peak meters and visualisers
- Deferring message-thread operations (sample map loading) triggered by audio-thread callbacks
- Debouncing or throttling expensive operations during rapid UI interaction
- Driving UI animations and status display updates

```js
const var tm = Engine.createTimerObject();
```

Timer also provides an independent elapsed-time counter via `getMilliSecondsSinceCounterReset()` and `resetCounter()` for measuring durations between events.

Match the interval to the task:

| Use case | Recommended interval |
|----------|---------------------|
| Real-time visual feedback (meters, animation) | 50ms |
| UI state synchronisation (visibility, tooltips) | 100-150ms |
| Status displays (CPU usage, preset state) | 300-500ms |

> [!Tip:Timer callbacks not suspended when hidden] Unlike `ScriptPanel` timers, Timer callbacks are **not** suspended when the plugin interface is hidden. This means they keep running in the background, which creates significant overhead when using multiple plugin instances. To suspend them manually, call `Content.setSuspendTimerCallback()` with a Broadcaster, then attach a listener to each Timer that calls `stopTimer()` on suspend and `startTimer()` on resume. This co-locates the suspend logic with each timer definition rather than requiring a global timer registry.

## Common Mistakes

- **Minimum interval is 40ms**
  **Wrong:** `tm.startTimer(5)`
  **Right:** `tm.startTimer(30)`
  *The minimum interval is 11ms. Values of 10 or less throw a runtime error.*

- **Set callback before starting timer**
  **Wrong:** `tm.startTimer(30)` without calling `setTimerCallback` first
  **Right:** Call `setTimerCallback` before `startTimer`
  *Without a valid callback, the timer silently stops itself on the first tick.*

- **Reuse one timer with state flag**
  **Wrong:** Creating a new Timer for each one-shot action
  **Right:** Reuse a single Timer and call `this.stopTimer()` in the callback
  *Accumulating abandoned Timer objects wastes resources and complicates debugging.*

- **Use Broadcaster instead of polling**
  **Wrong:** Using a Timer to poll a component value that changes infrequently
  **Right:** Attach a Broadcaster to the value source instead
  *Broadcasters fire only on change and are more efficient than periodic polling. Reserve Timers for values with no change notification (audio levels, CPU usage).*
