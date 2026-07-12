# Modulator -- Class Analysis

## Brief
Script handle for controlling modulator modules (LFOs, envelopes, constants) with intensity, bypass, attributes, and global modulator connections.

## Purpose
The `Modulator` object is a script-level handle that wraps any C++ `Modulator` in the HISE module tree -- including LFOs, envelopes, velocity modulators, constant modulators, and modulator chains. It provides methods to control modulation intensity and bipolar mode, get/set per-module attributes by index or name, manage bypass state, connect to the global modulation system, add child modulators to internal chains, and serialize/restore full module state. Instances are obtained via `Synth.getModulator()` (onInit only) or returned by modulator-creation methods on Synth, Effect, ChildSynth, and other Modulator instances.

## Details

### Modulation Modes and Intensity Ranges

The modulation mode is set by the parent chain, not the modulator itself. The mode determines the value range for `setIntensity`/`getIntensity`:

| Mode | Intensity Range | Internal Storage | Behavior |
|------|----------------|-----------------|----------|
| GainMode | 0.0 -- 1.0 | Direct | Multiplied with signal |
| PitchMode | -12.0 -- 12.0 (semitones) | value / 12.0 | Added to pitch buffer |
| PanMode | -1.0 -- 1.0 | Direct | Panning offset |
| GlobalMode | -1.0 -- 1.0 | Direct | Intensity ignored in processing |
| OffsetMode | -1.0 -- 1.0 | Direct | Added per-modulator with intensity |
| CombinedMode | 0.0 -- 1.0 | Direct | Per-modulator GainMode or OffsetMode |

For PitchMode, the scripting API presents values in semitones (-12 to 12) while the C++ engine stores them as a normalized factor (-1.0 to 1.0). The conversion is automatic.

### Dynamic Parameter Constants

Unlike most API objects, Modulator's constants are populated dynamically from the wrapped module's parameter list. Each parameter name is registered as a constant mapping to its index. For example, an LFO modulator exposes constants like `Frequency`, `FadeIn`, `TempoSync`, etc. These can be used with `setAttribute`/`getAttribute` instead of raw integer indices.

If the wrapped modulator is a script processor (has UI controls), an additional `ScriptParameters` constant provides a mapping of control names to indices.

### Bracket-Operator Access (AssignableObject)

Modulator implements `AssignableObject`, enabling bracket-operator syntax for attribute access:

```javascript
mod["Frequency"] = 2.5;  // equivalent to mod.setAttribute(mod.Frequency, 2.5)
```

Note: Reading via bracket operator currently returns 1.0 for all indices (incomplete implementation in C++ source).

### Global Modulator System

The global modulator system allows modulators in a `GlobalModulatorContainer` to be received by other modules across the module tree. See `connectToGlobalModulator()`, `addGlobalModulator()`, `addStaticGlobalModulator()`, and `getGlobalModulatorId()` for the full API.

### MatrixModulator Integration

See `setMatrixProperties()` for range data configuration on MatrixModulator instances.

### Modulator Type Hierarchy

The C++ Modulator base class has three main subclass branches:
- **VoiceStartModulator** -- evaluates once at note-on (e.g., Velocity, Random)
- **TimeVariantModulator** -- evaluates per audio block (e.g., LFO, ControlModulator)
- **EnvelopeModulator** -- evaluates per voice per block (e.g., AHDSR, TableEnvelope)

`ModulatorChain` is also a Modulator (inherits from EnvelopeModulator), so `getModulatorChain()` returns a Modulator handle that can itself have attributes set and modulators added.

### getCurrentLevel Behavior

See `getCurrentLevel()` for display value retrieval and pitch-mode conversion details.

## obtainedVia
`Synth.getModulator(name)` -- retrieves a reference to a named modulator in the parent synth's module tree. Must be called in `onInit`.

## minimalObjectToken
mod

## Constants
None. Constants are populated dynamically from the wrapped modulator's parameter list at construction time. The specific constants depend on the modulator type (e.g., an LFO has `Frequency`, `FadeIn`; an AHDSR has `Attack`, `Decay`, etc.).

## Dynamic Constants
| Name | Type | Description |
|------|------|-------------|
| (parameter names) | int | Each parameter of the wrapped modulator is registered as a constant mapping to its attribute index. Names vary by modulator type. |
| ScriptParameters | Object | Maps UI control names to indices. Only present when the wrapped modulator is a script processor with UI controls. |

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `var mod = Synth.getModulator("LFO1");` in `onNoteOn` | `const var mod = Synth.getModulator("LFO1");` in `onInit` | getModulator() is restricted to onInit. Store references as top-level const variables. |
| `mod.connectToGlobalModulator("Container", "LFO")` on a regular LFO | Use on a GlobalTimeVariantModulator or GlobalVoiceStartModulator | connectToGlobalModulator only works on global receiver modulator types. |
| `mod.exportScriptControls()` on a non-script modulator | Use only on Script Voice Start Modulator, Script Time Variant Modulator, or Script Envelope Modulator | exportScriptControls/restoreScriptControls require the modulator to be a ProcessorWithScriptingContent. |

## codeExample
```javascript
// Get a reference to an LFO in the module tree
const var mod = Synth.getModulator("LFO1");

// Set attributes using the dynamic parameter constants
mod.setAttribute(mod.Frequency, 2.5);
mod.setIntensity(0.8);
```

## Alternatives
- `ScriptModulationMatrix` -- manages a dynamic many-to-many modulation routing system, while Modulator is a handle to a single modulator module for direct parameter control
- `Effect` -- controls audio processing modules (filters, reverbs) in the FX chain, while Modulator controls modulation sources (LFOs, envelopes) that shape parameters over time

## Related Preprocessors
None.

## Diagrams
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Most methods validate with checkValidObject and dynamic_cast checks that produce runtime errors. The two silent-failure cases (getGlobalModulatorId returning empty string, setMatrixProperties as no-op) are logged as missing-validation bugs in issues.md rather than diagnostic candidates, since the correct fix is adding runtime validation, not parse-time checks. No timeline dependencies or state preconditions that would benefit from parse-time diagnostics.
