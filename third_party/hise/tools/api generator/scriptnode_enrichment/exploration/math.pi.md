# math.pi - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:316`
**Base class:** `OpNodeBase<Operations::pi>` (via `OpNode<Operations::pi, 1>`)
**Classification:** audio_processor

## Signal Path

Multiplies the input signal by PI times the Value parameter. The block path uses SIMD-accelerated `hmath::vmuls(b, float_Pi * value)` (line 326). The frame path computes `s *= (float_Pi * value)` per sample (line 332).

With the default Value of 2.0, this multiplies by 2*PI (one full cycle in radians). Typically paired with math.sin to convert a normalised [0, 1] ramp into a sine wave.

Formula: `output = input * PI * Value`

## Gap Answers

### pi-description-typo: The base description says 'PI (3.13)' but the actual value of PI is 3.14159... Is this a typo in the SN_DESCRIPTION macro?

Yes. Line 318 reads `SET_DESCRIPTION("Multiplies the signal with PI (3.13)")`. The value 3.13 is a typo. The actual constant used is `float_Pi` which is 3.14159265358979... The description should say "3.14159" or simply "PI".

## Parameters

- **Value** (default 2.0): Scalar multiplier applied together with PI. The signal is multiplied by `PI * Value`. At default 2.0, this produces a 2*PI multiplier (full radian cycle). This is the only mono-only OpNode math node that actually uses its Value parameter in processing.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

This is the only node among the 10 mono-only OpNode math nodes that uses the Value parameter. All others ignore it. The default value of 2.0 is intentional: math.pi with Value=2.0 followed by math.sin converts a [0, 1] ramp into a complete sine cycle.
