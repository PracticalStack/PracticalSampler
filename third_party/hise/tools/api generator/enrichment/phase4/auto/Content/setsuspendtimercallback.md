Registers a callback that fires when panel timers are suspended or resumed. HISE automatically suspends ScriptPanel timer callbacks and deferred MIDI callbacks when no plugin interface is open, to save CPU. However, timer objects created with `Engine.createTimerObject()` keep running by default. Use this method to manually start and stop those timers when the interface visibility changes.

The callback receives a single boolean: `true` when timers should be suspended (no interface visible), `false` when they should resume.

For complex projects, attach a Broadcaster to this callback slot and then register all timer objects as listeners at their definition, rather than managing them in a single callback function.

> [!Tip:Use moon icon to simulate suspension in IDE] During development, use the moon icon tool in the Interface Designer to simulate the suspension process, since you cannot close and reopen plugin interfaces while working in the HISE IDE.