# math.intensity - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:151`
**Base class:** `OpNode<Operations::intensity, NV>` -> `OpNodeBase<Operations::intensity>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Applies the HISE intensity formula: a linear crossfade between unity (1.0) and the input signal. The formula is `s = (1 - value) + value * s`. Block path computes this using two SIMD operations: `hmath::vmuls(b, alpha)` followed by `hmath::vadds(b, invAlpha)` where `alpha = value` and `invAlpha = 1.0f - alpha`. Frame path uses the equivalent per-sample formula.

At Value=0.0, the output is always 1.0 regardless of input. At Value=1.0, the output equals the input. Intermediate values produce a mix between unity and the input signal.

## Parameters

- **Value** (default 0.0): Intensity/depth control. 0.0 = output is always 1.0 (no modulation effect), 1.0 = output equals input (full depth). Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent intensity depth.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

This is designed for controlling modulation depth. When used on a modulation signal (0 to 1), it scales the modulation range: at Value=0.5, a 0-to-1 input becomes 0.5-to-1, halving the modulation depth while keeping the ceiling at 1.0.
