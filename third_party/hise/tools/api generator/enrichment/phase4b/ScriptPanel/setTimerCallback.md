# setTimerCallback | UNSAFE

Registers a callback function that fires periodically when the timer is active. The callback takes no parameters and fires on the message thread. After registering, call `startTimer(ms)` to begin. The timer is automatically stopped on recompilation.

```
setTimerCallback(Function timerFunction)
```

### Callback Signature

```
timerFunction()
```

No parameters. `this` refers to the panel inside inline functions.

## Dispatch / Mechanics

1. Stores a `WeakCallbackHolder` with 0 parameters
2. `startTimer(ms)` activates the JUCE `SuspendableTimer` at the given interval
3. On each tick, `timerCallback()` calls `timerRoutine.call(nullptr, 0)` on the message thread
4. On recompilation, `preRecompileCallback()` stops the timer and clears the callback

## Required Setup

The `allowCallbacks` property does NOT need to be set for timer callbacks. Timer and mouse callbacks are independent systems.

## Pair With

- `startTimer()` / `stopTimer()` - control timer activation
- `setPaintRoutine()` - typically used with timer for animation
- `repaint()` - trigger visual update from timer callback

## Source

`ScriptingApiContent.cpp` line ~4220
