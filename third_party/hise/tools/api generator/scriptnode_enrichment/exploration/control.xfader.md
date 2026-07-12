# control.xfader - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1817` (xfader class)
**Fader modes:** `hi_dsp_library/dsp_basics/logic_classes.h:647` (faders namespace)
**Base class:** `parameter_node_base<ParameterClass>`, `templated_mode`, `no_processing`
**Classification:** control_source

## Signal Path

The xfader distributes fade coefficients across multiple output slots based on a single normalised Value input (0..1). Each output receives a fade coefficient computed by the selected FaderClass. The distribution formula depends on the mode (Linear, Switch, RMS, etc.).

Value parameter (0..1) -> FaderClass::getFadeValue<P>(numOutputs, value) -> per-output coefficient
Each output slot receives its computed fade coefficient

## Gap Answers

### xfader-mode-variants: What fade curve modes are available in the 'faders' namespace?

The `faders` namespace (logic_classes.h:647-807) defines these static modes:
1. **switcher** -- hard switch: exactly one output is 1.0, all others are 0.0
2. **overlap** -- overlapping crossfade with custom formulas for 2, 3, 4 outputs
3. **harmonics** -- each output gets `value * (index + 1)`, can exceed 1.0
4. **threshold** -- step function: output[i] = 1.0 if value >= (i / numOutputs), else 0.0
5. **linear** -- triangular crossfade: each output fades linearly, adjacent outputs overlap
6. **cosine** -- S-curve (sine-based) applied to linear fade values
7. **cosine_half** -- half-cosine curve applied to linear fade values
8. **squared** -- linear fade values squared (steeper curve)
9. **rms** -- square root of linear fade values (equal-power crossfade)

Additionally, `faders::dynamic` (line 809) provides a runtime-switchable mode that wraps all of the above via a `FaderMode` enum: Switch, Linear, Overlap, Squared, RMS, Cosine, CosineHalf, Harmonics, Threshold.

### xfader-distribution-formula: How does Value distribute fade coefficients across N outputs?

For **linear** mode with 2 outputs: `output[0] = 1 - |1 - 1*(v + 1)| = 1-v`, `output[1] = 1 - |1 - 1*(v + 0)| = v`. So output[0] = 1-Value, output[1] = Value.

For **linear** mode with N outputs: each output has a triangular window centered at position `Index/(N-1)` along the 0..1 range. Formula (line 741-745): `v = 1.0 - abs(1.0 - u * (input + offset))` where `u = N-1` and `offset = (1 - Index) / u`, clamped to 0..1.

For **switcher** mode: `indexToActivate = min(N-1, floor(value * N))`, only that output is 1.0.

For **rms** mode: `sqrt(linearFadeValue)` for each output (equal-power crossfade).

### xfader-output-range: Do individual output values stay in 0..1?

Depends on mode. For linear, switcher, squared, rms, cosine, cosine_half, threshold, and overlap: values are clamped to 0..1 via `jlimit(0.0, 1.0, v)` or naturally constrained. For **harmonics**: `value * (index + 1)` which can exceed 1.0 for index > 0 (e.g., output[1] = value * 2, so value=0.8 gives 1.6).

### xfader-numparameters-range: What is the valid range for NumParameters?

The `callFadeValue<P>()` template is unrolled from P=0 to P=8 (line 1840-1848), so the maximum is 9 outputs. However, the dynamic_list has a theoretical limit of 32 (NumMaxSlots in dynamic_chain). For the compile-time `parameter::list`, the number is determined by the template arguments. The practical maximum in the interpreted path is controlled by the NumParameters property and the unrolled call limit of 9.

## Parameters

- **Value** (single parameter via SN_ADD_SET_VALUE): Normalised input 0..1. Controls the crossfade position.

## Properties

- **NumParameters**: Number of output slots. Default 2.
- **Mode**: Fade curve type. Maps to `faders` namespace via `templated_mode` with namespace "faders". Default is "Linear" (set in `faders::dynamic` constructor, line 826).

## Conditional Behaviour

The FaderClass template parameter determines the fade curve. At runtime with `faders::dynamic`, the Mode property selects between Switch, Linear, Overlap, Squared, RMS, Cosine, CosineHalf, Harmonics, and Threshold modes. For C++ export, the mode is baked into the template argument.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The xfader stores its last value in a `ModValue lastValue` (line 1865), calling `setModValueIfChanged()` -- but this ModValue is not used for modulation output. It appears to be used for UI display purposes only. The actual output goes through the parameter list's `call<P>()` method. The xfader is NOT polyphonic (no NV template parameter, no `polyphonic_base` inheritance).
