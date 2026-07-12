Timer (object)
Obtain via: Engine.createTimerObject()

Periodic message-thread timer with configurable interval, callback function,
and elapsed-time counter. Fires on the JUCE message thread (not audio thread).
Used for UI polling, delayed actions, debouncing, and periodic state checks.

Complexity tiers:
  1. Status polling: setTimerCallback, startTimer. Callback reads a value and
     updates a UI component. No state management beyond the timer itself.
  2. One-shot deferred action: + stopTimer inside callback. Delayed tooltips,
     deferred loading, timed dismissals.
  3. Debounce/throttle gate: + isTimerRunning as guard with startTimer restart
     and a flag variable. Callback checks flag, executes conditionally, stops.
  4. Multi-timer coordination: + getMilliSecondsSinceCounterReset, resetCounter.
     Two or more Timer instances for tooltip systems, modal dialogs, elapsed
     time measurement.

Practical defaults:
  - Use 50ms for real-time visual feedback (peak meters, animation) -- ~20fps.
  - Use 100-150ms for UI state sync (visibility toggling, tooltip polling).
  - Use 300-500ms for status displays (CPU usage, preset state).
  - Use this.stopTimer() inside callback for one-shot actions rather than
    creating and destroying Timer objects.

Common mistakes:
  - Starting timer without setTimerCallback -- WeakCallbackHolder is invalid,
    timer immediately stops itself on first tick. No error reported.
  - startTimer(5) or any interval <= 10 -- throws "Go easy on the timer".
    Minimum allowed interval is 11ms.
  - Creating a new Timer for each one-shot action -- reuse a single Timer and
    call this.stopTimer() in the callback.
  - Using Timer to poll values that have change notifications -- use Broadcaster
    instead. Reserve Timer for values without change events (audio levels, CPU).
  - Polling at 30ms for a status label that updates once per second -- match
    interval to how quickly the displayed value meaningfully changes.

Example:
  const var tm = Engine.createTimerObject();

  tm.setTimerCallback(function()
  {
      // Periodic UI update logic
  });

  tm.startTimer(30);

Methods (6):
  getMilliSecondsSinceCounterReset  isTimerRunning
  resetCounter                      setTimerCallback
  startTimer                        stopTimer
