# math.fmod - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:273`
**Base class:** `OpNode<Operations::fmod, NV>` -> `OpNodeBase<Operations::fmod>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Computes the floating-point modulo of each sample against the Value parameter: `s = hmath::fmod(s, value)`. Both block and frame paths iterate per sample (no SIMD acceleration). Both paths include a `value == 0.0f` early-return guard that leaves the signal unchanged when Value is zero.

## Parameters

- **Value** (default 1.0): Modulo divisor. Each sample is wrapped to the remainder after division by this value. At 0.0 the signal passes through unchanged (zero-division guard). Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent modulo divisor.

## CPU Assessment

baseline: medium
polyphonic: true
scalingFactors: []

## Notes

Uses `hmath::fmod` which delegates to `std::fmod`. No SIMD acceleration is available for modulo operations. The zero-guard prevents undefined behaviour but means the node silently passes through when Value is exactly 0.0.
