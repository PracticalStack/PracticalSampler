# MidiProcessor -- Project Context

## Project Context

### Real-World Use Cases
- **Channel mute/solo system**: A multi-channel instrument uses an array of MidiMuter modules (one per channel) controlled via `setAttribute` to implement mute/solo logic. The MidiProcessor handles are stored in an array and toggled by index, driven by a state object and broadcaster events. This is the most common MidiProcessor use case.
- **Articulation switching**: A sampler instrument with multiple articulations (sustain, staccato, legato) uses MidiMuter processors to gate which sample groups receive MIDI. The script toggles muters on/off and adjusts script processor parameters via `setAttribute` with dynamic constants when the user selects an articulation.
- **Built-in module parameter control**: Arpeggiator, Transposer, and ChannelFilter modules are controlled from a UI script. The script obtains a MidiProcessor handle and uses dynamic constants (`arp.NumStepSlider`, `arp.SequenceComboBox`) to set parameters from knob callbacks.
- **State serialization for preset systems**: A plugin with custom preset management uses `exportState()`/`restoreState()` to snapshot and restore MIDI processor state as base64 strings. This is used for effect locking (preserving processor state across preset changes) and for migrating/importing preset data.

### Complexity Tiers
1. **Basic parameter control** (most common): `Synth.getMidiProcessor()`, `setAttribute()` with dynamic constants, `setBypassed()`. Sufficient for controlling built-in modules like Transposer or Arpeggiator from a UI callback.
2. **Multi-channel mute arrays**: Build arrays of MidiProcessor handles in a loop, toggle bypass or attributes by index. Used for mute/solo systems and articulation switching.
3. **State serialization**: `exportState()`/`restoreState()` for custom preset management, effect locking, or migration tools. `exportScriptControls()`/`restoreScriptControls()` for saving only UI control values of script processors without recompilation.

### Practical Defaults
- Use `Synth.getMidiProcessor()` in `onInit` and store the handle in a `const var`. Never call it repeatedly at runtime.
- Use dynamic constants (`mp.Intensity`) rather than raw indices (`0`) for `setAttribute`/`getAttribute`. The constants are module-specific and self-documenting.
- When building arrays of similar processors (muters, transposers), use a loop with string concatenation: `Synth.getMidiProcessor("MidiMuter" + (i + 1))`.
- Use `exportState()`/`restoreState()` for full module state snapshots. Use `exportScriptControls()`/`restoreScriptControls()` only for script processors when you want to preserve UI control values without triggering recompilation.

### Integration Patterns
- `Synth.getMidiProcessor()` -> `MidiProcessor.setAttribute()` with dynamic constants -- the standard pattern for controlling any MIDI module from a UI script.
- `Synth.getMidiPlayer()` -> `MidiPlayer.asMidiProcessor()` -> `MidiProcessor.exportState()` -- cast a MidiPlayer to its MidiProcessor base to use generic state serialization.
- `MidiProcessor.setBypassed()` driven by `Broadcaster.addListener()` -- mute/solo state managed through a broadcaster event bus, with listeners toggling processor bypass.
- `MidiProcessor.setAttribute()` with `ScriptParameters` object -- control another script processor's UI components by index, useful for modular MIDI processing pipelines.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Synth.getMidiProcessor("MidiMuter" + i)` in a callback | `const var muters = []; for(i = 0; i < N; i++) muters.push(Synth.getMidiProcessor("MidiMuter" + (i+1)));` in onInit | getMidiProcessor is onInit-only. Cache all references at init time, then index into the array at runtime. |
| `mp.setAttribute(0, value)` | `mp.setAttribute(mp.SomeParameter, value)` | Raw indices are fragile and unreadable. Dynamic constants are module-specific and self-documenting. |
| Using `exportScriptControls()` on all MIDI processors | Check if the target is a script processor first, or use `exportState()` for built-in modules | `exportScriptControls` only works on script processors (JavascriptMidiProcessor). Built-in modules like Arpeggiator or Transposer will throw an error. |
