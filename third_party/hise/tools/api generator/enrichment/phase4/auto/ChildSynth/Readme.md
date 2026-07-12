<!-- Diagram triage:
  - (no diagrams defined in Phase 1 data)
-->

# ChildSynth

ChildSynth is a script handle to a child sound generator (synth, sampler, or group) within a SynthGroup or SynthChain. Obtain a reference with `Synth.getChildSynth()` or `Synth.getChildSynthByIndex()` during `onInit`, then use it to control that child's parameters, bypass state, modulation, routing, and effect chain from the parent script.

```js
const var cs = Synth.getChildSynth("MySynth");
```

You can right-click the top bar of a module in the HISE IDE and select "Create generic script reference" to auto-generate this line.

Each instance exposes dynamic constants that map to the wrapped processor's parameters. The base set is:

| Constant | Index | Description |
|----------|-------|-------------|
| `cs.Gain` | 0 | Volume (0.0 - 1.0) |
| `cs.Balance` | 1 | Stereo balance (-100 to 100) |
| `cs.VoiceLimit` | 2 | Maximum voice count |
| `cs.KillFadeTime` | 3 | Voice kill fade time |

Subclass processors (samplers, sine synths, etc.) add further constants beyond index 3. Use these constants rather than raw numbers with `setAttribute` and `getAttribute` - they are self-documenting and remain correct even when subclasses shift parameter indices.

The modulator chain methods (`addModulator`, `addGlobalModulator`, `addStaticGlobalModulator`, `getModulatorChain`, `setModulationInitialValue`) all take a `chainIndex` parameter:

| Index | Chain |
|-------|-------|
| 1 | GainModulation |
| 2 | PitchModulation |

> [!Tip:Cache references in onInit as const var] ChildSynth references must be obtained during `onInit` - calling
> `Synth.getChildSynth()` or `Synth.getChildSynthByIndex()` at runtime
> throws a script error. Store references as `const var` at the top level
> and reuse them in callbacks.

## Common Mistakes

- **Cache references in onInit**
  **Wrong:** `var cs = Synth.getChildSynth("Sine");` in `onNoteOn`
  **Right:** `const var cs = Synth.getChildSynth("Sine");` in `onInit`
  *`getChildSynth` is restricted to onInit. Store the reference as a top-level const variable and reuse it in all callbacks.*

- **Use correct chain index and type name**
  **Wrong:** `cs.addModulator(0, "LFO", "MyLFO");`
  **Right:** `cs.addModulator(1, "LFOModulator", "MyLFO");`
  *Chain index 0 is MidiProcessor, not a modulator chain. Use 1 (Gain) or 2 (Pitch). The type name must match the C++ class name (e.g. "LFOModulator", not "LFO").*

- **Use static modulator for block-level targets**
  **Wrong:** Using `addGlobalModulator` for all modulation connections
  **Right:** Use `addStaticGlobalModulator` for targets that do not need per-voice resolution
  *Static global modulators use one value per audio block instead of per-voice, which is significantly more CPU-efficient for effect parameters and synth-level controls.*

- **Smooth getCurrentLevel for meters**
  **Wrong:** Passing `getCurrentLevel` results directly to a UI meter
  **Right:** Apply decay smoothing in the timer callback
  *Raw peak values fluctuate rapidly. Use a formula like `level = Math.max(newPeak, level * 0.94)` for a stable meter display.*
