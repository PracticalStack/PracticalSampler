# math.div - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:247`
**Base class:** `OpNode<Operations::div, NV>` -> `OpNodeBase<Operations::div>` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

Divides every sample by a scalar Value parameter. Implemented as multiplication by the reciprocal: `factor = (value > 0.0f) ? 1.0f / value : 0.0f`. Block path uses `hmath::vmuls(b, factor)` (SIMD-accelerated). Frame path applies the same reciprocal multiplication per sample.

Non-positive Value values (zero or negative) produce silence (factor = 0.0f). This is a safety guard against division by zero, but it also means negative divisors are treated as zero rather than performing negative division.

## Parameters

- **Value** (default 1.0): Divisor applied to each sample. Only positive values perform division; zero and negative values produce silence. Polyphonic (per-voice).

## Polyphonic Behaviour

Value is stored in `PolyData<float, NumVoices>`. Each voice can have an independent divisor.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

## Notes

The reciprocal is computed once per block/frame call, not per sample, so the division cost is minimal. The `value > 0.0f` guard means this node cannot be used for phase inversion via negative divisors.
