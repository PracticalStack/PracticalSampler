# Timer -- Project Context

## Project Context

### Real-World Use Cases
- **Level meter polling**: A plugin with peak meters creates a Timer at 50ms to poll `getCurrentLevel()` or `Engine.getMasterPeakLevel()` from audio processors, apply decay smoothing, and trigger `repaint()` on meter panels. A single timer instance drives all meters in the plugin via an array of panel references.
- **Deferred sample map loading**: A sampler plugin uses a Timer at 50ms to bridge the gap between audio-thread control callbacks and message-thread sample loading. The control callback sets a `reg` variable; the timer detects the change and calls `loadSampleMap()`, avoiding audio thread hitches from synchronous loading.
- **Operation debounce/throttle**: A plugin with expensive operations (loading impulse responses, rebuilding preset lists) uses a Timer as a cooldown gate. The first trigger executes immediately and starts a cooldown timer; subsequent triggers during cooldown set a flag and restart the timer. When the timer fires, it stops itself and re-executes if flagged.
- **Status display polling**: An FX plugin polls `Engine.getCpuUsage()` on a 500ms Timer and updates a label with the current CPU percentage. This is the simplest Timer use case.
- **Animated activity indicators**: A synth plugin uses a Timer at 50ms to drive a sinusoidal alpha animation on LED indicator panels, checking visibility conditions each tick and repainting only visible panels.

### Complexity Tiers
1. **Status polling** (most common): `setTimerCallback` + `startTimer` only. Callback reads a value and updates a UI component. No state management beyond the timer itself.
2. **One-shot deferred action**: Callback performs an action then calls `this.stopTimer()`. Used for delayed tooltips, deferred loading, and timed dismissals.
3. **Debounce/throttle gate**: Combines `isTimerRunning()` as a guard with `startTimer()` restart and a flag variable. The callback checks the flag, executes conditionally, and stops itself.
4. **Multi-timer coordination**: Two or more Timer instances work together - one polls for state changes, another handles delayed execution. Used for tooltip systems (poll timer + delay timer) and modal dialogs (focus timer + fadeout timer).

### Practical Defaults
- Use 50ms for real-time visual feedback (peak meters, animation). This provides ~20fps updates without excessive CPU.
- Use 100-150ms for UI state synchronization (visibility toggling, tooltip polling). Fast enough to feel responsive, slow enough to be cheap.
- Use 300-500ms for status displays (CPU usage, preset state). These values change slowly and do not need frequent updates.
- Use `this.stopTimer()` inside the callback for one-shot delayed actions rather than creating and destroying Timer objects.

### Integration Patterns
- `Timer callback` -> `panel.repaint()` -- The most common integration. Timer polls audio state and triggers panel redraws for meters, animations, and visualizers.
- `Timer callback` -> `Sampler.loadSampleMap()` -- Timer bridges audio-thread control callbacks to message-thread sample loading, preventing audio hitches.
- `Timer.isTimerRunning()` -> `Timer.startTimer()` -- Guard-and-restart pattern for debouncing expensive operations like IR file loading.
- `Timer callback` -> `Content.getCurrentTooltip()` -- Timer polls the global tooltip state to drive custom tooltip panel display with configurable delay.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating a new Timer for each one-shot action | Reuse a single Timer and call `this.stopTimer()` in the callback | Timer objects are cheap but accumulating abandoned timers wastes resources and complicates debugging. |
| Using a Timer to poll component values that could use a Broadcaster | Attach a Broadcaster to the component value source | Broadcasters fire only on change and are more efficient than periodic polling. Reserve Timers for values that have no change notification (audio levels, CPU usage, system state). |
| Polling at 30ms for a status label that updates visually once per second | Use 300-500ms for slow-changing values | Excessive polling wastes CPU on the message thread. Match the interval to how quickly the displayed value meaningfully changes. |
