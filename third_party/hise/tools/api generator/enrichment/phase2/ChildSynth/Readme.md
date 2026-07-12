# ChildSynth -- Project Context

## Project Context

### Real-World Use Cases
- **Multi-oscillator synthesizer**: A synth with multiple oscillator groups uses ChildSynth references to control per-oscillator volume, pitch, detune, and FM depth from the parent script. Each oscillator group is a child SynthGroup with its own modulator and effect chains, and the parent interface drives all parameters through setAttribute calls on an array of ChildSynth handles.
- **Channel-strip drum machine**: A drum machine with per-channel processing stores an array of ChildSynth references (one per channel) and iterates them in a loop to set gain, pan, bypass state, and effect parameters. This pattern scales to 12+ channels with consistent per-channel control.
- **Articulation-based sampler**: A sampled instrument with multiple articulations (sustain, staccato, legato transitions) uses ChildSynth references to bypass inactive samplers and chains `asSampler()` for sample map loading when switching patches or articulations.
- **Custom modulation matrix**: A synthesizer with a user-configurable modulation matrix uses `addGlobalModulator()` and `addStaticGlobalModulator()` to dynamically connect global LFOs, envelopes, and CC sources to per-oscillator parameter chains at runtime.

### Complexity Tiers
1. **Basic parameter control** (most common): `setAttribute`, `getAttribute`, `setBypassed`, `isBypassed` - controlling child synth parameters and bypass state from the parent script interface.
2. **Level monitoring and routing**: `getCurrentLevel`, `getRoutingMatrix` - driving VU meters from child synth peak levels, configuring multi-output channel routing.
3. **State management and type casting**: `exportState`, `restoreState`, `asSampler` - snapshotting processor state for preset locking, casting to Sampler for sample map operations.
4. **Dynamic modulation**: `addModulator`, `addGlobalModulator`, `addStaticGlobalModulator` - programmatic modulator chain construction for custom modulation matrices.

### Practical Defaults
- Store ChildSynth references in `const var` arrays when controlling multiple channels or oscillators. Build the array in a loop: `for (i = 0; i < NUM_CHANNELS; i++) channels[i] = Synth.getChildSynth("Channel" + (i + 1));`
- Use the dynamic constants (`cs.Gain`, `cs.Balance`) rather than raw index numbers for `setAttribute`/`getAttribute`. They are self-documenting and correct even when subclasses shift parameter indices.
- Prefer `addStaticGlobalModulator` over `addGlobalModulator` when per-voice resolution is not needed. Static global modulators use one value per audio block rather than per-voice, which is significantly more CPU-efficient for targets like effect parameters or synth-level controls.
- Always check `asSampler()` return value with `isDefined()` before calling Sampler methods. The method returns undefined silently when the child is not a ModulatorSampler.

### Integration Patterns
- `ChildSynth.setAttribute()` -> UI knob `setControlCallback()` - control child synth parameters from parent interface knobs by mapping knob values to child synth attribute indices.
- `ChildSynth.getCurrentLevel()` -> `ScriptPanel.setTimerCallback()` - drive VU meters and peak meters by polling child synth display levels in a panel timer callback.
- `ChildSynth.asSampler()` -> `Sampler.loadSampleMap()` - chain to Sampler reference for loading sample maps on articulation-specific child samplers.
- `ChildSynth.exportState()` / `restoreState()` -> `UserPresetHandler` lifecycle - snapshot effect or EQ state before a preset change and restore it afterward to implement "FX lock" features.
- `ChildSynth.getRoutingMatrix()` -> `RoutingMatrix.addConnection()` - configure multi-output routing per child synth for multi-channel output plugins.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Calling `Synth.getChildSynth()` inside a timer or control callback | Store references as `const var` arrays in `onInit` | `getChildSynth` is restricted to onInit. Build all references at initialization time and reuse them. |
| Using `addGlobalModulator` for all modulation connections | Use `addStaticGlobalModulator` for targets that don't need per-voice resolution | Static global modulators consume less CPU. Reserve per-voice modulation for pitch and gain chains where voice independence matters. |
| Passing `getCurrentLevel` results directly to UI without smoothing | Apply decay smoothing in the timer callback | Raw peak values fluctuate rapidly. Use a smoothing formula like `level = Math.max(newPeak, level * 0.94)` for stable meter display. |
