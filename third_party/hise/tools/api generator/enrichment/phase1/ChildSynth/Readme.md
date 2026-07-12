# ChildSynth -- Class Analysis

## Brief
Script handle to a child sound generator for attribute control, bypass, modulator chains, routing, and state management.

## Purpose
The `ChildSynth` object is a script handle to a child sound generator module (synth, sampler, group, etc.) within a SynthGroup or SynthChain. It provides attribute get/set, bypass control, modulator chain access (adding modulators, connecting global modulators, setting initial modulation values), effect chain reordering, base64 state export/restore, peak level monitoring, routing matrix access, and conversion to a Sampler reference. Unlike the parent `Synth` namespace, ChildSynth does not provide MIDI generation, voice management, timer callbacks, or module tree search -- it is a focused handle for controlling a specific child processor's parameters and signal chain.

## Details

### Relationship to Synth

ChildSynth shares many method signatures with the `Synth` namespace (setAttribute, getAttribute, addModulator, getRoutingMatrix, etc.) but operates on an arbitrary child sound generator rather than the parent synth that hosts the script processor. Key capabilities that exist only on Synth (not ChildSynth): MIDI event generation, voice management, timer system, module tree search (getModulator, getEffect, etc.). Key capabilities unique to ChildSynth: setEffectChainOrder, setModulationInitialValue, exportState/restoreState, getCurrentLevel, asSampler.

### Modulator Chain Indices

The `chainIndex` parameter used by addModulator, getModulatorChain, addGlobalModulator, addStaticGlobalModulator, and setModulationInitialValue maps to the `ModulatorSynth::InternalChains` enum:

| Index | Chain | Typical Use |
|-------|-------|-------------|
| 0 | MidiProcessor | MIDI processing chain |
| 1 | GainModulation | Volume modulation |
| 2 | PitchModulation | Pitch modulation |
| 3 | EffectChain | Effect processing chain |

For addModulator, indices 1 (Gain) and 2 (Pitch) are the standard modulator chain targets.

### Dynamic Constants

Each ChildSynth instance registers constants derived from the wrapped processor at construction time. For base ModulatorSynth types, these are Gain (0), Balance (1), VoiceLimit (2), KillFadeTime (3). Subclasses (ModulatorSampler, SineSynth, etc.) add additional parameters. A `ScriptParameters` constant is also added containing UI component name-to-index mappings if the target synth has a script interface.

### Global Modulator Connection

See `addGlobalModulator()` (per-voice) and `addStaticGlobalModulator()` (time-variant, single value per block) for the full global modulator connection API. Both require a source reference from a GlobalModulatorContainer.

### Effect Chain Reordering

See `setEffectChainOrder()` for the full effect chain reordering API.

### asSampler Conversion

See `asSampler()` for the dynamic cast to Sampler reference.

## obtainedVia
`Synth.getChildSynth(name)` or `Synth.getChildSynthByIndex(index)` -- both restricted to onInit.

## minimalObjectToken
cs

## Constants
None. All constants are dynamic (derived from the wrapped processor instance).

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| Gain | int (0) | Base ModulatorSynth parameter index for volume (0.0-1.0) |
| Balance | int (1) | Stereo balance parameter index (-100 to 100) |
| VoiceLimit | int (2) | Maximum voice count parameter index |
| KillFadeTime | int (3) | Voice kill fade time parameter index |
| ScriptParameters | Object | UI component name-to-index mappings (empty if target has no script interface) |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var cs = Synth.getChildSynth("Sine");` in `onNoteOn` | `const var cs = Synth.getChildSynth("Sine");` in `onInit` | getChildSynth is restricted to onInit. Store the reference as a top-level const variable. |
| `cs.addModulator(0, "LFO", "MyLFO");` | `cs.addModulator(1, "LFOModulator", "MyLFO");` | Chain index 0 is MidiProcessor, not a modulator chain. Use 1 (Gain) or 2 (Pitch). The type name must match the C++ class name. |

## codeExample
```javascript
// Get a child synth reference in onInit
const var cs = Synth.getChildSynth("MySynth");

// Control attributes using dynamic constants
cs.setAttribute(cs.Gain, 0.5);
cs.setBypassed(false);
```

## Alternatives
- `Synth` -- parent-level API for voice management, MIDI generation, and module tree traversal on the hosting synth
- `Effect` -- handle to an audio effect module in the FX chain (processing, not sound generation)

## Related Preprocessors
None.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: ChildSynth methods have straightforward parameter validation (chain index checks, object validity) with clear error messages. No silent-failure preconditions or timeline dependencies that would benefit from parse-time diagnostics.
