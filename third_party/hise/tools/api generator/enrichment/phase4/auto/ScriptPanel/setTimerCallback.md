Registers a zero-parameter callback that fires periodically when the timer is active. After registering the callback, call `startTimer(intervalMs)` to begin. A 30 ms interval is a good default for smooth visual updates. The timer is automatically stopped on recompilation.

Inside the callback, `this` refers to the panel instance. A common pattern is to poll a value (audio level, playback position, modulator output), store it in `this.data`, and call `this.repaint()`. To avoid redundant repaints, compare the new value against the stored value and only repaint when it changes.

> [!Warning:Register callback before startTimer] Always register the callback with `setTimerCallback()` before calling `startTimer()`. Calling `startTimer()` without a registered callback silently starts the timer with no effect.
