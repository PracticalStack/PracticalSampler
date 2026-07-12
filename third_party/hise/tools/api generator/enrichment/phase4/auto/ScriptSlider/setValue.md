Sets the slider value, schedules UI refresh, and notifies registered value listeners. Use this for direct programmatic updates when you do not need an undo step.

> [!Warning:Values not restored after recompile] Values assigned in `onInit` are treated as initial setup and are not restored across recompiles.
