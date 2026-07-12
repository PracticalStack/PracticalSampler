# GlobalCable -- Project Context

## Project Context

### Real-World Use Cases
- **DSP network to script communication**: A plugin with a custom DSP engine (e.g., a granular processor in scriptnode) uses GlobalCable to stream real-time parameter values back to the script layer for UI visualization. The DSP network writes to a `routing.global_cable` node, and a script callback reads the cable value on a timer to drive visual feedback (peak meters, modulation displays, envelope visualizations).
- **UI component to DSP network routing**: Buttons and sliders set `processorId` to `"GlobalCable"` and `parameterId` to a cable name, creating a zero-code data path from the interface directly into scriptnode parameters. This is especially useful when a single UI control needs to reach a parameter deep inside a DSP network without a chain of script forwarding.
- **Envelope parameter bus**: A custom envelope editor panel uses multiple cables (one per envelope stage: attack, decay, sustain, retrigger) to bridge hidden slider values to corresponding `routing.global_cable` nodes inside a DSP network. The sliders are invisible -- the user drags directly on a ScriptPanel, which updates slider values that flow through cables into the DSP engine.
- **Global modulator to cable bridge**: A synth with a modulation matrix uses `connectToGlobalModulator` to pipe each modulator's output (LFOs, envelopes, velocity, pitch wheel) into named cables, making them available as cable sources throughout the project. This is a setup-time pattern that runs once during module tree construction.

### Complexity Tiers
1. **Basic read/write** (most common): `getCable`, `getValue`, `setValue`/`setValueNormalised`. Enough for simple inter-script communication or timer-polled value reading from DSP networks.
2. **Callback-driven**: Adds `registerCallback` with `AsyncNotification` for reactive updates when cable values change. Useful when you need UI elements to respond to DSP value changes without polling.
3. **Data channel**: Adds `sendData`/`registerDataCallback` for passing structured JSON through cables. Used for complex data like envelope shapes or multi-parameter state objects.
4. **Module integration**: Adds `connectToModuleParameter`, `connectToMacroControl`, `connectToGlobalModulator` to wire cables directly into the HISE module tree. This eliminates script middlemen for DSP parameter routing.

### Practical Defaults
- Use `AsyncNotification` for value callbacks unless you specifically need audio-thread timing. Most cable callbacks drive UI updates, which belong on the message thread.
- When bridging DSP networks to script, prefer timer-polled `getValue()` over `registerCallback` with `SyncNotification`. The timer approach avoids the realtime-safety requirement and coalesces updates naturally at the display refresh rate.
- Use `setValueNormalised` when working cable-to-cable or cable-to-module, since the internal transport is always 0..1. Only use `setRange`/`setValue` when the cable's endpoints need a human-readable range (e.g., sliders in Hz or dB).
- For UI component connections via `processorId="GlobalCable"`, the `parameterId` property is the cable name string. This is a one-way path: component value changes flow into the cable.

### Integration Patterns
- `GlobalCable.getValue()` inside `ScriptPanel.setTimerCallback()` -- poll a cable value at the display refresh rate to drive visual feedback (peak meters, modulation rings, envelope gain visualization).
- `GlobalCable.setValueNormalised()` inside slider `setControlCallback()` -- forward a slider value through a cable into a DSP network's `routing.global_cable` node.
- `GlobalCable.connectToGlobalModulator()` after `Builder.create()` -- wire each modulator in a `GlobalModulatorContainer` to a named cable during module tree construction, making modulation signals available throughout the project.
- `GlobalCable.registerDataCallback()` with `ScriptPanel.repaint()` -- receive structured JSON data from a DSP network (e.g., envelope state snapshots) and use it to drive complex UI rendering.
- UI component `processorId="GlobalCable"` with `parameterId="CableName"` -- zero-code connection from buttons/sliders directly into the cable system.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Polling `getValue()` in `onTimer` at 30ms for visual feedback without smoothing | Apply exponential smoothing (`smoothed = smoothed * 0.6 + newValue * 0.4`) before rendering | Raw cable values from DSP networks can change abruptly between timer ticks. Smoothing produces visually stable animations without requiring faster polling. |
| Creating many cables with separate `getGlobalRoutingManager()` calls | Call `Engine.getGlobalRoutingManager()` once, store in `const var rm`, then call `rm.getCable()` for each cable | The routing manager is a singleton; repeated `getGlobalRoutingManager()` calls work but are wasteful. Cache the reference at init time. |
| Using `registerCallback` with `SyncNotification` for UI updates | Use `AsyncNotification` or timer-polled `getValue()` for anything that triggers repaints | Synchronous callbacks run on the calling thread (possibly the audio thread). UI operations like `repaint()` or `Console.print()` are not realtime-safe and will silently fail or cause audio glitches. |
