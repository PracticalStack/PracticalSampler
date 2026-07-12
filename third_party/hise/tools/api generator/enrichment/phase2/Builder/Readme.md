# Builder -- Project Context

## Project Context

### Real-World Use Cases
- **Repetitive channel strip construction**: A drum machine or multi-layer synth that needs N identical processing channels builds the entire tree in a loop. Each iteration creates the same processor chain (container, synth, MIDI processors, effects, routing) with indexed IDs. The Builder makes adding a new effect to every channel a one-line change rather than N manual IDE operations.
- **Modulation matrix scaffolding**: A wavetable synth creates a GlobalModulatorContainer with global modulator sources (envelopes, LFO, MIDI CC, velocity), then builds per-oscillator groups with MatrixModulator instances for each modulation target (gain, pitch, filter frequency, pan). The Builder loops over a target definition table to create and configure the matrix modulators.
- **Master and send effect chains**: After building per-channel processors, the Builder adds a master effect chain (EQ, compressor, limiter) to the root container and a SendContainer with shared delay/reverb effects, then loops over existing channels via `getExisting()` to add per-channel send gains.

### Complexity Tiers
1. **Basic**: `create()`, `setAttributes()`, `flush()` - create a handful of modules with configured parameters. Suitable for simple synths or FX plugins.
2. **Intermediate**: Add `get()` for typed references (routing matrix manipulation, SlotFX DSP network loading, modulator configuration), `clearChildren()` for targeted chain cleanup, and `connectToScript()` for external MIDI script linking.
3. **Advanced**: Loop-based construction of N identical channels, `getExisting()` to reference previously-created modules in multi-pass builds, `RoutingMatrix` manipulation for multi-channel routing, conditional build flags for selective rebuilding of subsections.

### Practical Defaults
- Use `b.ChainIndexes.Direct` (-1) when adding sound generators to container synths, and `b.ChainIndexes.FX` (3) for effects. These are the two most common chain targets.
- Always bracket builds with `b.clear()` at the start and `b.flush()` at the end. The clear-build-flush pattern ensures a clean slate and proper UI updates.
- Comment out the Builder include or build function call by default. Uncomment only when the module tree needs modification, run once, then re-comment. This activate-modify-deactivate workflow avoids rebuilding the tree on every script compile.
- For large builds with distinct subsections (channels, master FX, sends), use boolean flags to enable/disable each section independently during development.

### Integration Patterns
- `Builder.create()` -> `Builder.get(idx, "RoutingMatrix")` -> `RoutingMatrix.setNumChannels()` / `addConnection()` - configure multi-channel audio routing after creating a processor.
- `Builder.create()` -> `Builder.get(idx, "SlotFX")` -> `SlotFX.setEffect(networkName)` - load a compiled DSP network into a HardcodedFX slot.
- `Builder.create()` -> `Builder.get(idx, "Modulator")` -> `Modulator.setMatrixProperties()` - configure MatrixModulator range and converter properties for modulation matrix targets.
- `Builder.create()` -> `Builder.connectToScript(idx, path)` - link a created ScriptProcessor to an external .js file for per-channel MIDI processing.
- `Builder.getExisting()` -> `Builder.create()` - reference modules from a previous build pass and add new children to them.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Running Builder code on every compile | Commenting out the build call after use | The Builder is a development tool, not a runtime initializer. Rebuilding on every compile is slow and unnecessary - the tree persists in the XML preset. |
| Building N identical channels by hand in the IDE | Using a loop with indexed IDs | Manual construction is error-prone and tedious for repetitive structures. A loop trivially creates N identical processor chains. |
| Creating all modules in one monolithic function | Splitting into per-section build functions with conditional flags | Selective rebuilding (e.g. `BUILD_CHANNELS`, `BUILD_MASTER_FX`, `BUILD_SENDS` flags) lets you iterate on one section without rebuilding the entire tree. |
| Using `Synth.addEffect()` for complex trees | Using Builder API | The older `Synth.addEffect()` / `Synth.addModulator()` methods are simpler but lack routing matrix access, SlotFX loading, and batch attribute setting. Builder is the modern replacement for programmatic tree construction. |
