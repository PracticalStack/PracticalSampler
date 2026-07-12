# Effect -- Class Analysis

## Brief
Script handle for controlling an audio effect module's parameters, bypass, modulator chains, and state.

## Purpose
The `Effect` object is a script-level handle to an `EffectProcessor` in the HISE module tree. It wraps any effect module (filters, reverbs, delays, dynamics, etc.) and provides a uniform interface for reading and writing parameters by index or name, bypassing the effect, querying output levels and suspension state, managing modulator chains, and serializing/restoring full processor state. Effect handles are obtained via `Synth.getEffect()` (onInit only) or created dynamically via `Synth.addEffect()` or `Builder.create()`. Each instance exposes the wrapped effect's parameters as named constants for index-free attribute access.

## Details

### Effect Processor Types

The Effect handle wraps any subclass of `EffectProcessor`, which has three subclass tiers:

| Base Class | Processing Model | Examples |
|------------|-----------------|----------|
| `MasterEffectProcessor` | Stereo buffer, soft bypass with fade | Reverb, Delay, Convolution, Dynamics |
| `MonophonicEffectProcessor` | Monophonic with stepped modulation | Filters with mod chains |
| `VoiceEffectProcessor` | Polyphonic per-voice processing | Polyphonic Filter |

All three tiers are accessed through the same `Effect` API -- the handle is type-agnostic.

### Dynamic Parameter Constants

Each Effect instance registers the wrapped module's parameter names as named constants at construction time. For example, a SimpleGain effect provides `fx.Gain` (= 0), while a PolyphonicFilter provides `fx.Frequency`, `fx.Q`, `fx.Mode`, etc. This allows `fx.setAttribute(fx.Gain, 0.5)` instead of `fx.setAttribute(0, 0.5)`. The available constants depend entirely on which effect module the handle wraps -- there are no universal constants.

### Suspension System

Effects can opt into automatic silence suspension. See `isSuspended()` for details on the dual-condition logic and threshold behavior.

### Draggable Filter Data

Interactive filter visualization is available for effects implementing the `ProcessorWithCustomFilterStatistics` interface (Script FX, Hardcoded FX, Polyphonic Filter). See `setDraggableFilterData()` for the JSON schema and `getDraggableFilterData()` for retrieval.

### Script Controls vs Full State

Two serialization levels exist: full processor state (`exportState`/`restoreState`) and script-UI-only (`exportScriptControls`/`restoreScriptControls`, Script FX modules only). See individual method entries for constraints and threading implications.

### Shared Handle Pattern

Effect shares its core API surface (attribute access, bypass, state serialization, modulator chain management) with `Modulator`, `ChildSynth`, and `MidiProcessor`. Effect adds `isSuspended()`, `getCurrentLevel()`, and the draggable filter data methods.

## obtainedVia
`Synth.getEffect("EffectId")` (onInit only), `Synth.addEffect(type, id, index)`, or `Builder.create()`

## minimalObjectToken
fx

## Constants
None. Effect has no hardcoded constants. All constants are dynamically generated from the wrapped effect module's parameter list at construction time (see Dynamic Constants in Details).

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| (per-parameter) | int | Each parameter of the wrapped effect is registered as a named constant mapping to its attribute index. Names and count vary by effect type. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `const var fx = Synth.getEffect("MyFX");` in `onNoteOn` | `const var fx = Synth.getEffect("MyFX");` in `onInit` | getEffect() can only be called in onInit. Store the reference as a top-level const. |
| `fx.exportScriptControls()` on a built-in effect | `fx.exportState()` on a built-in effect | exportScriptControls/restoreScriptControls only work on Script FX modules. Use exportState/restoreState for built-in effects. |

## codeExample
```javascript
// Get a reference to an effect in onInit
const var fx = Synth.getEffect("MyFilter");

// Use named constants for parameter access
fx.setAttribute(fx.Frequency, 1000.0);
fx.setAttribute(fx.Q, 0.7);
```

## Alternatives
- `SlotFX` -- dynamically swappable effect container, while Effect is a static handle to a specific module
- `Modulator` -- similar handle pattern but for modulator modules instead of effects
- `DspModule` -- handle for scriptnode DSP modules rather than built-in effect modules

## Related Preprocessors
None.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Effect methods have straightforward preconditions (valid object check) with no timeline dependencies, silent-failure modes, or non-obvious value constraints that would benefit from parse-time diagnostics.
