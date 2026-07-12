# Effect -- Project Context

## Project Context

### Real-World Use Cases
- **Channel strip mixer**: A multi-channel mixer uses arrays of Effect handles (EQ, compressor, shaper, filter per channel) to build a channel-strip architecture. Each strip's effects are obtained in a loop (`Synth.getEffect("ChannelEq " + (i+1))`) and stored in arrays for indexed access. The Effect handle's named constants allow readable parameter control across dozens of identical effect instances.
- **Mode-switched FX chains**: A piano plugin with multiple performance modes (stage, cinematic, experimental) uses `setBypassed()` to activate/deactivate entire effect groups per mode. Compiled DSP networks wrapped as HardcodedMasterFX are toggled this way, with parameters controlled via `setAttribute()`.
- **FX state lock system**: A plugin with a preset browser implements "FX lock" - saving effect states via `exportState()` before preset changes and restoring them via `restoreState()` after load. This keeps user-configured master EQ, compressor, and send effects intact across preset switches.
- **Peak metering and gain display**: Effects like SimpleGain are polled via `getCurrentLevel()` in timer callbacks to drive custom peak meter visualizations on ScriptPanels. The left/right channel values feed into smoothed display logic with peak hold and decay.
- **Draggable parametric EQ**: A channel EQ uses `setDraggableFilterData()` to configure interactive filter band visualization with mouse drag mapping (horizontal = frequency, vertical = gain, shift+drag = Q) and FFT spectrum overlay.

### Complexity Tiers
1. **Basic parameter control** (most common): `Synth.getEffect()`, `setAttribute()`, `setBypassed()`, `isBypassed()`. Sufficient for controlling any built-in effect from script. Used by virtually all HISE projects.
2. **State serialization**: Adds `exportState()` / `restoreState()` for FX lock, preset management, and save/load of effect configurations to files. Needed when effect state must persist independently of presets.
3. **Metering and visualization**: Adds `getCurrentLevel()` for peak meters and `isSuspended()` for silence detection UI. Requires timer-polled ScriptPanels with custom paint routines.
4. **Interactive filter display**: Adds `setDraggableFilterData()` / `getDraggableFilterData()` for interactive EQ visualization. Only applicable to effects implementing `ProcessorWithCustomFilterStatistics` (Script FX, Hardcoded FX, Polyphonic Filter).

### Practical Defaults
- Store all Effect references as `const var` at the top of the init scope. Use arrays for repeated effect types: `const var channelEQs = []; for(i = 0; i < NUM_CHANNELS; i++) channelEQs.push(Synth.getEffect("ChannelEq " + (i+1)));`
- Use named constants (`fx.Frequency`, `fx.Gain`) instead of raw integer indices for `setAttribute()` / `getAttribute()`. The constants are generated per-effect-type and provide self-documenting, refactor-safe parameter access.
- For effects with an enable/disable toggle, prefer `setBypassed()` over setting an internal enable parameter. `setBypassed()` provides soft bypass with fade-out on MasterEffectProcessors, avoiding clicks.
- When building a peak meter, poll `getCurrentLevel()` at 30ms intervals in a timer callback and apply smoothing (e.g., exponential decay factor of 0.77) for visually stable display.

### Integration Patterns
- `Synth.getEffect()` -> `Effect.setAttribute()` -- the fundamental pattern for controlling any effect parameter from script. Used in every project that has effects.
- `Effect.exportState()` -> `Effect.restoreState()` -- FX lock pattern: save state before preset load, restore after. Pairs with `UserPresetHandler` pre/post load callbacks.
- `Effect.getCurrentLevel()` -> `ScriptPanel.setTimerCallback()` -- peak meter pattern: poll effect output level in a timer and drive custom paint routines.
- `Effect.setBypassed()` -> `Broadcaster.attachToModuleParameter()` -- bypass state drives UI grey-out via broadcasters listening to the effect's bypass parameter, updating component colours.
- `Effect.setDraggableFilterData()` -> `ScriptFloatingTile` with `DraggableFilterPanel` ContentType -- configures interactive filter visualization that the floating tile renders.
- `Effect.setAttribute()` / `Effect.isBypassed()` with multi-output routing -- when switching between stereo and multi-output modes, the bypass strategy changes: multi-output uses `setBypassed()` for enable/disable while stereo mode uses an internal enable parameter.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Using raw integer indices: `fx.setAttribute(0, 1000.0)` | Using named constants: `fx.setAttribute(fx.Frequency, 1000.0)` | Named constants are generated per effect type. They make code self-documenting and survive parameter reordering. |
| Restoring state and expecting UI to update automatically | Calling `fx.setAttribute(fx.Param, fx.getAttribute(fx.Param))` after `restoreState()` to trigger notifications | `restoreState()` does not send attribute update notifications. Re-set parameters or call `updateValueFromProcessorConnection()` on connected components to sync the UI. |
| Polling `getCurrentLevel()` without smoothing | Applying exponential decay: `data.left = (level > data.left) ? level * 0.7 + data.left * 0.3 : data.left * DECAY` | Raw level values fluctuate rapidly. Without smoothing, meters appear jittery and unusable. |
