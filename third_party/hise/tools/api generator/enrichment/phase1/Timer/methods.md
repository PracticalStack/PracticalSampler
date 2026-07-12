# Timer -- Method Documentation

## getMilliSecondsSinceCounterReset

**Signature:** `var getMilliSecondsSinceCounterReset()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var ms = {obj}.getMilliSecondsSinceCounterReset();`

**Description:**
Returns the number of milliseconds elapsed since the internal counter was last reset. The counter is reset by `resetCounter()` and also automatically by `startTimer()`. Uses `juce::Time::getMillisecondCounter()` internally, which is a uint32 system counter.

**Parameters:**

(No parameters.)

**Pitfalls:**
- The counter is not initialized until `startTimer()` or `resetCounter()` is called. Calling this method on a freshly created Timer before either of those methods returns an undefined elapsed value with no warning.

**Cross References:**
- `$API.Timer.resetCounter$`
- `$API.Timer.startTimer$`

## isTimerRunning

**Signature:** `bool isTimerRunning()`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var running = {obj}.isTimerRunning();`

**Description:**
Returns whether the timer is currently running. Delegates directly to the internal JUCE `Timer::isTimerRunning()`, which reads a member flag.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Timer.startTimer$`
- `$API.Timer.stopTimer$`

## resetCounter

**Signature:** `void resetCounter()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.resetCounter();`

**Description:**
Resets the internal millisecond counter to the current system time. Subsequent calls to `getMilliSecondsSinceCounterReset()` return the elapsed time since this reset. Also called automatically by `startTimer()`.

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Timer.getMilliSecondsSinceCounterReset$`
- `$API.Timer.startTimer$`

## setTimerCallback

**Signature:** `void setTimerCallback(Function callbackFunction)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates a new WeakCallbackHolder (heap allocation) and increments its reference count.
**Minimal Example:** `{obj}.setTimerCallback(onTimerTick);`

**Description:**
Sets the function to be called on each timer tick. The callback receives zero arguments and executes on the message thread (not the audio thread). Calling this method replaces any previously registered callback. The `this` object inside the callback is set to the Timer instance. If the timer is running and no valid callback is set, the timer auto-stops on the next tick.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| callbackFunction | Function | yes | The function to call on each timer tick. | Must be a function. |

**Callback Signature:** callbackFunction()

**Cross References:**
- `$API.Timer.startTimer$`
- `$API.Timer.stopTimer$`

**Example:**
```javascript:timer-callback-self-stop
// Title: Timer that counts ticks and stops itself
const var t = Engine.createTimerObject();

reg tickCount = 0;

inline function onTimerTick()
{
    tickCount++;

    if (tickCount >= 3)
        t.stopTimer();
};

t.setTimerCallback(onTimerTick);
t.startTimer(50);
```
```json:testMetadata:timer-callback-self-stop
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "delay": 500, "expression": "tickCount >= 3", "value": true},
    {"type": "REPL", "expression": "t.isTimerRunning()", "value": false}
  ]
}
```

## startTimer

**Signature:** `void startTimer(int intervalInMilliSeconds)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** JUCE Timer::startTimer acquires a lock on the internal timer thread.
**Minimal Example:** `{obj}.startTimer(100);`

**Description:**
Starts the timer with the given interval in milliseconds. The minimum allowed interval is 11ms -- values of 10 or less throw a script error ("Go easy on the timer"). If the timer is already running, this changes the interval. Automatically calls `resetCounter()`, resetting the internal millisecond counter.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| intervalInMilliSeconds | Integer | no | The timer interval in milliseconds. | Must be > 10. |

**Pitfalls:**
- Calling `startTimer` automatically resets the internal millisecond counter (calls `resetCounter()` internally). If you are using `getMilliSecondsSinceCounterReset()` to measure elapsed time independently of timer restarts, restarting the timer silently resets your measurement.

**Cross References:**
- `$API.Timer.stopTimer$`
- `$API.Timer.isTimerRunning$`
- `$API.Timer.setTimerCallback$`
- `$API.Timer.resetCounter$`

## stopTimer

**Signature:** `void stopTimer()`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** JUCE Timer::stopTimer acquires a lock on the internal timer thread.
**Minimal Example:** `{obj}.stopTimer();`

**Description:**
Stops the timer. No further callbacks will fire after this call. Calling `stopTimer` on a timer that is not running is a safe no-op. The timer is also stopped automatically when the Timer object is destroyed, when the MainController shuts down, or when the callback becomes invalid (e.g., after script recompilation).

**Parameters:**

(No parameters.)

**Cross References:**
- `$API.Timer.startTimer$`
- `$API.Timer.isTimerRunning$`
