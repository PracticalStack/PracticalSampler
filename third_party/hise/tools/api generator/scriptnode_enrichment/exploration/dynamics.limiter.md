# dynamics.limiter - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DynamicsNode.h:80-289` (shared template)
**Base class:** `dynamics_wrapper<chunkware_simple::SimpleLimit>`, inherits `HiseDspBase` + `display_buffer_base<true>`
**Classification:** audio_processor

## Signal Path

Identical wrapper to dynamics.comp/gate. Input audio -> peak detection -> chunkware SimpleLimit algorithm -> limited audio output. Modulation output is inverse gain reduction (1.0 - GR, normalised 0..1).

The limiter behaviour comes from the chunkware `SimpleLimit` algorithm, which differs from SimpleComp by using faster envelope detection optimised for peak limiting.

## Gap Answers

### signal-path-processing: How does the limiter process audio?

Same `dynamics_wrapper<T>::processFrame()` as comp/gate. The SimpleLimit algorithm implements peak limiting with fast attack characteristics. Unlike the compressor which uses a standard attack/release envelope, the limiter is designed for transparent peak control.

### sidechain-modes: What are the 3 Sidechain parameter values?

Identical to comp/gate: Disabled(0), Original(1), Sidechain(2).

### ratio-behaviour: How is the Ratio parameter applied?

Same inversion as comp/gate: `1.0 / v`. For limiting, high ratios approach hard limiting (brickwall). Ratio=1 means no limiting.

### modulation-output: Does the node have modulation output?

YES. Identical to comp/gate: `1.0 - obj.getGainReduction()`, normalised 0..1.

### display-buffer-content: What does the DisplayBuffer show?

Same as comp/gate: inverse gain reduction over time.

### description-accuracy: Is "ducking amount as modulation signal" accurate?

Same issue. "Limiting amount" would be more precise.

## Parameters

Identical to comp/gate (shared template). See dynamics.comp exploration.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

- Same `dynamics_wrapper<T>` template as comp and gate.
- The key difference from comp is in the chunkware SimpleLimit algorithm, which typically has faster attack characteristics and may include lookahead behaviour internally.
- The Attack/Release parameters still apply but the limiter algorithm may internally override or constrain the attack time for transparent peak control.
