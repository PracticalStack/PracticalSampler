# control.bipolar - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1980`
**Base class:** `multi_parameter<NV, ParameterType, multilogic::bipolar>` (via alias at line 2737)
**Classification:** control_source

## Signal Path

Value parameter (0..1) -> centre around 0 (subtract 0.5) -> optional gamma curve -> scale by Scale -> re-centre around 0.5 (add 0.5) -> modulation output.

The `getValue()` method at line 1993:

```
double v = value - 0.5f;
if (gamma != 1.0)
    v = hmath::pow(hmath::abs(v * 2.0), gamma) * hmath::sign(v) * 0.5;
v *= scale;
v += 0.5;
return v;
```

Output is NOT explicitly clamped. The normalised modulation flag is true, so the connection system treats the output as 0..1. With extreme Scale and Gamma combinations, the output CAN exceed 0..1.

## Gap Answers

### bipolar-formula: What is the exact formula?

The exact formula (line 1996-2003):
1. Subtract 0.5 from value to centre around 0
2. If gamma != 1.0: take `abs(v * 2.0)`, raise to power `gamma`, multiply by `sign(v) * 0.5`
3. Multiply by `scale`
4. Add 0.5 to re-centre

The gamma application operates on the absolute deviation from centre, preserving the sign. This creates a symmetric non-linear curve around 0.5.

### bipolar-gamma-application: How is Gamma applied?

Gamma is applied as `pow(abs(v * 2.0), gamma)` where v is the centred value (value - 0.5). The `* 2.0` normalises the deviation to 0..1 range, then `pow()` applies the curve, then `* sign(v) * 0.5` restores the sign and rescales. So for the positive half: gamma > 1.0 creates a concave curve (values pulled toward centre), gamma < 1.0 creates a convex curve (values pushed toward extremes). The range 0.5..2.0 with skew matches this interpretation.

### bipolar-output-range: Is the output clamped to 0..1?

No explicit clamping. The output CAN exceed 0..1 with certain parameter combinations. For example: Value=1.0, Scale=1.0, Gamma=1.0 produces 1.0. Value=1.0, Scale=-1.0 produces 0.0. But Value=0.0, Scale=2.0 (Scale range allows up to 1.0 so this would not happen via UI). Within the parameter ranges (Scale -1..1, Value 0..1, Gamma 0.5..2.0), the output stays approximately within 0..1. Since `isNormalisedModulation()` returns true, the connection system treats values as 0..1.

## Parameters

- **Value** (P=0): Input signal 0..1. Default 0.0. Note: member default is `value = 0.5` (line 2044).
- **Scale** (P=1): Bipolar scaling factor -1..1. Default 0.0 (no deviation from centre). Stored as `scale`.
- **Gamma** (P=2): Non-linear curve shaping 0.5..2.0 with skew for centre at 1.0. Default 1.0 (linear). Stored as `gamma`.

## Polyphonic Behaviour

Uses `PolyData<multilogic::bipolar, NV>`. Each voice has independent value, scale, gamma, and dirty flag.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: [{"parameter": "Gamma", "impact": "minimal", "note": "pow() call only when gamma != 1.0"}]

## Notes

The member default for `value` is 0.5 (line 2044) while the parameter default registered in `createParameters` is 0.0 (line 2025). This means the initial state before any parameter is set differs from what the parameter default would produce. This is likely intentional -- 0.5 as the member default means the node starts at centre.
