# Timer -- Class Analysis

## Brief
Periodic message-thread timer with configurable interval, callback function, and elapsed-time counter.

## Purpose
Timer is a standalone periodic callback object created via `Engine.createTimerObject()`. It wraps JUCE's message-thread Timer to fire a user-defined JavaScript callback at a configurable millisecond interval (minimum 11ms). It also provides an independent elapsed-time counter (`getMilliSecondsSinceCounterReset` / `resetCounter`) for measuring durations between events. Timer is commonly used for UI polling, delayed actions, and periodic state checks that do not require audio-thread precision.

## obtainedVia
`Engine.createTimerObject()`

## minimalObjectToken
tm

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `tm.startTimer(5)` | `tm.startTimer(30)` | The minimum interval is 11ms. Values <= 10 throw a runtime exception ("Go easy on the timer"). |
| `tm.startTimer(30)` without `setTimerCallback` | Call `setTimerCallback` before `startTimer` | If no callback is set, the WeakCallbackHolder is invalid and the timer immediately stops itself on the first tick. |

## codeExample
```javascript
const var tm = Engine.createTimerObject();

tm.setTimerCallback(function()
{
    // Periodic UI update logic
});

tm.startTimer(30);
```

## Alternatives
- `TransportHandler` -- fires callbacks synced to the host transport (tempo, beat, grid) rather than a fixed millisecond interval.
- `BackgroundTask` -- runs a one-shot or long-running function on a background thread, while Timer fires periodically on the message thread.
- `Broadcaster` -- fires when an attached source (component value, module parameter, etc.) changes, rather than at a fixed interval.
- `ScriptPanel.setTimerCallback` -- integrated panel timer for animation; Timer is the standalone equivalent for non-UI periodic tasks.

## Related Preprocessors
None.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 1
- Timer.startTimer -- timeline dependency (logged)
