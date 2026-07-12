Timer::setTimerCallback(Function callbackFunction) -> undefined

Thread safety: UNSAFE -- creates a new WeakCallbackHolder (heap allocation) and increments its reference count
Sets the function called on each timer tick. Callback receives zero arguments
and executes on the message thread. Replaces any previously registered callback.
The `this` object inside the callback is the Timer instance.
If no valid callback is set when the timer fires, the timer auto-stops.
Callback signature: callbackFunction()

Required setup:
  const var tm = Engine.createTimerObject();

Dispatch/mechanics:
  Creates WeakCallbackHolder(processor, this, callbackFunction, 0)
  -> incRefCount() to prevent GC
  -> setThisObject(this) for callback's `this` context
  -> addAsSource(this, "timerCallback") for IDE debug

Pair with:
  startTimer -- must set callback before starting (or timer auto-stops)
  stopTimer -- stop the timer from within or outside the callback

Anti-patterns:
  - Do NOT call startTimer before setTimerCallback -- the WeakCallbackHolder
    is invalid and the timer silently stops on first tick

Source:
  ScriptingApiObjects.cpp:5374  TimerObject::setTimerCallback()
    -> new WeakCallbackHolder(getScriptProcessor(), this, callbackFunction, 0)
    -> tc.incRefCount(), tc.setThisObject(this), tc.addAsSource(this, "timerCallback")
