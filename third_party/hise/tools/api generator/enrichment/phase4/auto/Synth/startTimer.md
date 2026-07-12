Starts or restarts the periodic timer for this script processor. The `onTimer` callback fires repeatedly at the specified interval (in seconds, minimum 0.004). If a timer is already running, calling this again updates the interval without any gap in events.

In non-deferred mode the timer runs on the audio thread for sample-accurate timing. In deferred mode it uses a UI-thread timer with millisecond resolution.

> [!Warning:Only four timer slots per synth] Only 4 timer slots are available per synth, shared across all script processors in that synth. If a fifth processor tries to start a timer, it fails with "All 4 timers are used". Call `stopTimer` to release a slot when you no longer need it.
