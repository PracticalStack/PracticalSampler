# control.intensity - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1874`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::intensity>` (via alias at line 2741)
**Classification:** control_source

## Signal Path

Value parameter (0..1) + Intensity parameter (0..1) -> HISE intensity formula -> modulation output (normalised).

The `getValue()` method at line 1891:

```
return 1.0 * (1.0 - intensityValue) + intensityValue * value;
```

Simplified: `output = (1.0 - intensity) + intensity * value`, which is the standard HISE gain modulation formula.

## Gap Answers

### intensity-formula: What is the exact HISE intensity formula?

The formula at line 1895 is `1.0 * (1.0 - intensityValue) + intensityValue * value`. This simplifies to `1.0 - intensityValue + intensityValue * value`. This matches the HISE Gain modulation mode formula from the modulation infrastructure: `base - intensity + intensity * mod * base` where base = 1.0. The formula linearly interpolates between 1.0 (no modulation effect) and the modulation value.

### intensity-zero-behaviour: When Intensity is 0.0, does the output become 1.0?

Yes. When `intensityValue = 0.0`, the formula becomes `1.0 * (1.0 - 0.0) + 0.0 * value = 1.0`. So Intensity=0 means no modulation effect (output is always 1.0), following the HISE convention.

### intensity-output-clamping: Is the output clamped to 0..1?

No explicit clamping in `getValue()`. However, the `setParameter<1>()` method at line 1903 clamps intensityValue: `intensityValue = jlimit(0.0, 1.0, v)`. Since Value is also expected in 0..1 range, the formula `(1-I) + I*V` with I in [0,1] and V in [0,1] always produces a result in [0,1]. So the output is inherently bounded without needing explicit clamping.

## Parameters

- **Value** (P=0): Modulation input 0..1. Stored as `value`. Default 0.0. Note: member default is `value = 1.0` (line 1883).
- **Intensity** (P=1): Modulation depth 0..1, clamped via `jlimit`. Default 1.0. Stored as `intensityValue`.

## Polyphonic Behaviour

Uses `PolyData<multilogic::intensity, NV>`. Each voice has independent value, intensityValue, and dirty flag.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

## Notes

The member default for `value` is 1.0 (line 1883) while the parameter default is 0.0 (line 1914). With member defaults (before any parameter callback), the output would be `(1-1) + 1*1 = 1.0`. After the parameter system sets Value to its default 0.0, the output becomes `(1-1) + 1*0 = 0.0`. This initial value difference is standard for control nodes -- the parameter system sets defaults during initialisation.
