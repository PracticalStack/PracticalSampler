# SlotFX -- Project Context

## Project Context

### Real-World Use Cases
- **User-selectable FX rack**: A synthesizer or sampler with multiple effect slots where the user picks effects from a menu (chorus, delay, reverb, saturator, etc.). Each slot is a SlotFX module, and a data-driven configuration object maps effect names to module IDs, parameter bindings, and default states. This is the most common SlotFX pattern -- it turns a static module tree into a flexible, per-preset effect chain.
- **Builder-constructed channel strip with compiled networks**: A multi-channel instrument that uses the Builder API to create `HardcodedMasterFX` or `HardcodedPolyphonicFX` modules at init, then immediately loads compiled scriptnode DSP networks into them via the SlotFX interface. Each channel gets its own EQ, compressor, shaper, and filter -- all as compiled networks loaded through `setEffect()`. This pattern combines Builder's programmatic construction with SlotFX's runtime loading.
- **Multi-bus effect routing**: Instruments with separate voice-level, master, and send effect buses, each containing multiple SlotFX slots. Voice slots handle per-voice effects (chorus, phaser, saturation), master slots handle post-mix processing (dynamics, EQ, convolution), and send slots handle parallel effects (delay, reverb). All share the same slot management code but target different points in the signal chain.

### Complexity Tiers
1. **Single slot** (simplest): One `Synth.getSlotFX()` call, one `setEffect()` to load an effect, control it via the returned `Effect` handle. Methods needed: `setEffect`, `getCurrentEffect`.
2. **FX rack with presets**: Multiple SlotFX modules managed as an array, driven by a ComboBox or popup menu. Requires a data structure mapping UI selections to effect type names, parameter IDs, and default states. Methods needed: `setEffect`, `clear`, plus `Effect.restoreState()` and `Effect.setAttribute()` on the returned handle.
3. **Builder + compiled networks** (advanced): Programmatic module tree construction where `HardcodedMasterFX` modules are created by the Builder, then loaded with compiled DSP networks via `b.get(module, b.InterfaceTypes.SlotFX).setEffect("networkName")`. May also use `getParameterProperties()` to discover network parameters at runtime.

### Practical Defaults
- Use `setEffect("EmptyFX")` or `clear()` to reset a slot to unity-gain passthrough. Both achieve the same result; `setEffect("EmptyFX")` is more common in practice because it fits naturally into switch/case selection logic.
- Always use the `Effect` handle returned by `setEffect()` to control the loaded effect. Do not call `Synth.getEffect()` with the internal child name -- the internal naming convention (`SlotId_EffectType`) is an implementation detail.
- Store SlotFX references in arrays when managing multiple slots. Use a loop with indexed module names (`"EffectSlot" + (i + 1)`) to initialize them.
- When building an FX rack, define effect configurations as a data object mapping each effect to its `moduleID`, `parameterIds`, parameter `range` arrays, and a `defaultState` string. This keeps the slot management code generic.

### Integration Patterns
- `SlotFX.setEffect()` -> `Effect.restoreState()` -- After loading an effect, restore a saved state string to configure it with default or preset-specific parameters.
- `SlotFX.setEffect()` -> `Effect.setAttribute()` -- After loading, set individual parameters on the returned Effect handle.
- `Builder.create(b.Effects.HardcodedMasterFX, ...)` -> `Builder.get(module, b.InterfaceTypes.SlotFX)` -> `SlotFX.setEffect("networkName")` -- Programmatic channel strip construction that loads compiled DSP networks.
- `SlotFX.setEffect()` -> `ScriptSlider.set("processorId", ...)` -- Bind UI knobs to the loaded effect's parameters by setting `processorId` and `parameterId` properties on slider components.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Synth.getEffect("MySlot_SimpleReverb")` after `setEffect` | `const var fx = slot.setEffect("SimpleReverb");` | `setEffect` returns the Effect handle directly. Looking up the child by its internal name is fragile and depends on an undocumented naming convention. |
| Separate switch/case per effect with duplicated knob binding | Data-driven config object with generic `setEffect` + parameter binding loop | A data object mapping effect names to moduleIDs, parameter arrays, and ranges eliminates per-effect code duplication and scales to any number of effects. |
| Calling `slot.setEffect()` repeatedly with the same effect name on preset load | Check `slot.getCurrentEffectId()` first, or rely on the built-in same-effect guard | The C++ layer already skips reloading if the same effect type is loaded, but checking first avoids unnecessary audio thread coordination. |
