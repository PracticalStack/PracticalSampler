# Modulator -- Project Context

## Project Context

### Real-World Use Cases
- **Synthesizer modulation routing**: A multi-oscillator synth uses Modulator handles to build a custom modulation matrix -- source modulators (LFOs, envelopes, MIDI CC) from a GlobalModulatorContainer are dynamically routed to targets (filter cutoff, pitch, volume, pan) via `addGlobalModulator` and `addStaticGlobalModulator`. The matrix is user-configurable at runtime with connection add/remove, intensity control, and inversion.
- **Modulation value display**: Plugins use `getModulatorChain` and `getCurrentLevel` in timer callbacks to show real-time modulation activity on knob overlays -- an arc visualization shows how much modulation is being applied to a parameter, smoothed for visual stability.
- **Conditional module activation**: Virtually every plugin uses `setBypassed` to toggle modulators, effects, and processors based on UI state -- filter key-follow toggles, EQ stereo/mid-side mode switching, effect enable buttons, and articulation switching all use this pattern.
- **Module state persistence**: Plugins use `exportState`/`restoreState` to implement effect locking (preserving FX settings across preset changes), FX preset save/load to disk, and custom factory preset systems that serialize the full state of modulators, effects, and MIDI processors.
- **Pitch wheel range control**: Synths and samplers use pitch-mode modulators with `setIntensity` to let users configure pitch bend range -- the intensity value in semitones (-12 to 12) directly maps to the bend range.

### Complexity Tiers
1. **Basic reference and control** (most common): `Synth.getModulator`, `setAttribute`, `setIntensity`, `setBypassed`. Sufficient for controlling existing modulators from UI callbacks -- filter toggles, LFO rate changes, envelope parameters.
2. **State management**: Adds `exportState`/`restoreState` for preset locking, FX snapshots, and custom preset systems that need to capture and restore complete module configurations.
3. **Dynamic modulation routing**: Adds `addGlobalModulator`, `addStaticGlobalModulator`, `addModulator`, `getModulatorChain`. Building runtime-configurable modulation matrices where connections are created/removed on demand. Requires understanding of GlobalModulatorContainer, chain indices, and static vs. dynamic routing.
4. **MatrixModulator integration**: Adds `setMatrixProperties`, `setIsBipolar`. Configuring modulators created by the Builder API for the built-in ModulationMatrix system with range data and bipolar mode.

### Practical Defaults
- Use `const var` references at `onInit` scope for all modulator handles. `Synth.getModulator` is restricted to `onInit` and the reference should never change.
- Use `setIntensity(12.0)` for pitch modulators when you want full-range semitone control. The pitch mode API presents values in semitones while internally normalizing to -1.0..1.0.
- Use `addStaticGlobalModulator` instead of `addGlobalModulator` when the modulation source does not need continuous per-block updates (e.g., velocity, random, note-number sources). Static routing is more CPU-efficient.
- A 30ms timer interval is a good default for `getCurrentLevel` display polling -- fast enough for responsive UI without excessive CPU overhead.
- When creating per-voice envelope modulators at runtime via `addModulator`, set `EcoMode` to reduce CPU. A value of 32 provides a good balance between accuracy and performance.

### Integration Patterns
- `Synth.getModulator()` -> `Modulator.setAttribute()` -- the fundamental pattern for controlling modulator parameters from UI callbacks. Store the reference in `onInit`, call `setAttribute` from control callbacks.
- `Modulator.getModulatorChain()` -> `Modulator.getCurrentLevel()` -- access a processor's modulation chain, then poll its output level for UI visualization in a timer callback.
- `Modulator.addGlobalModulator()` / `Modulator.addStaticGlobalModulator()` -> `Modulator.setIntensity()` -- create a dynamic modulation connection, then set its depth. Paired with `Synth.removeModulator()` for connection removal.
- `Modulator.exportState()` -> `File.writeObject()` / `File.readObject()` -> `Modulator.restoreState()` -- serialize module state to base64, persist to disk as part of a JSON object, and restore later.
- `Builder.create(MatrixModulator)` -> `Builder.get(Modulator)` -> `Modulator.setMatrixProperties()` -- when using the Builder API to create MatrixModulators, retrieve the Modulator handle and configure its range data.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Setting `setIntensity(1.0)` on a pitch modulator and expecting 1 semitone | `setIntensity(12.0)` for full range, or the desired semitone value directly | PitchMode intensity is in semitones (-12 to 12), not a 0-1 normalized range. `1.0` gives only 1 semitone of range. |
| Creating modulation connections without checking if one already exists | Track active connections in a data structure and check before `addGlobalModulator` | Duplicate connections stack -- each call adds another modulator to the chain, causing doubled modulation depth. |
| Using `addGlobalModulator` for velocity or note-number sources | Use `addStaticGlobalModulator` for voice-start sources | Voice-start modulators (velocity, key number, random) only produce a value at note-on. Using the dynamic variant wastes CPU polling a value that never changes mid-note. |
| Calling `getCurrentLevel` directly in `onControl` or `onNoteOn` | Use a timer callback (30ms interval) to poll display values | `getCurrentLevel` returns the display value which updates once per audio buffer. Polling it from audio-rate callbacks is wasteful; polling from non-periodic callbacks gives stale or inconsistent readings. |
