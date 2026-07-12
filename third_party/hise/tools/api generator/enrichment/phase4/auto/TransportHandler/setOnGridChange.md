Registers a callback that fires on each grid tick. The callback receives three arguments:

- The grid index (integer, adjusted for the local multiplier)
- A timestamp (sample offset within the current audio block for sample-accurate scheduling)
- A boolean indicating whether this is the first grid tick in the current playback session or after a discontinuity

> [!Warning:Enable grid before registering callback] The grid must be enabled via `setEnableGrid(true, tempoFactor)` before this callback can fire. Without it, the callback is registered but never triggered - no error is reported.
