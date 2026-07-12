Switches MIDI callback execution between audio thread (default) and message thread (deferred) modes. When set to `true`, all MIDI callbacks (`onNoteOn`, `onNoteOff`, `onController`, `onTimer`) are deferred to the message thread. This has three consequences:

1. MIDI messages become read-only - you cannot modify incoming events.
2. The timer switches from sample-accurate synth timer to a millisecond-resolution timer.
3. You can safely perform allocations, string operations, and UI updates directly in callbacks.

Use deferred mode for interface scripts that only route parameters and update the UI. Use non-deferred mode (the default) for scripts that need to modify MIDI events in real time, such as arpeggiators or legato scripts.
