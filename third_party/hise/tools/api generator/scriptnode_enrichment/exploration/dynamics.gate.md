# dynamics.gate - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DynamicsNode.h:80-289` (shared template)
**Base class:** `dynamics_wrapper<chunkware_simple::SimpleGate>`, inherits `HiseDspBase` + `display_buffer_base<true>`
**Classification:** audio_processor

## Signal Path

Identical wrapper to dynamics.comp. Input audio -> peak detection -> chunkware SimpleGate algorithm -> gated audio output. Modulation output is inverse gain reduction (1.0 - GR, normalised 0..1).

The gate behaviour comes entirely from the chunkware `SimpleGate` algorithm: signals below the threshold are attenuated according to the ratio. The wrapper code (`dynamics_wrapper`) is identical to comp and limiter.

## Gap Answers

### signal-path-processing: How does the gate process audio?

Same `dynamics_wrapper<T>::processFrame()` as comp. The SimpleGate algorithm applies attenuation to signals falling below the threshold. The Ratio parameter controls the depth of attenuation -- at high ratios, the gate is more aggressive (closer to hard gate behaviour).

### sidechain-modes: What are the 3 Sidechain parameter values?

Identical to comp: Disabled(0), Original(1), Sidechain(2). Same channel-splitting behaviour.

### ratio-behaviour: How is the Ratio parameter applied?

Same inversion as comp: `1.0 / v`. For a gate, higher user ratio values mean deeper attenuation below threshold. Ratio=1 means no gating (unity). Ratio=32 means very aggressive gating.

### modulation-output: Does the node have modulation output?

YES. Identical to comp: `1.0 - obj.getGainReduction()`, normalised 0..1.

### display-buffer-content: What does the DisplayBuffer show?

Same as comp: inverse gain reduction over time.

### description-accuracy: Is "ducking amount as modulation signal" accurate?

Same issue as comp. The modulation is inverse gain reduction, not "ducking amount" directly. For a gate, "gate amount" would be more precise.

## Parameters

Identical to comp (shared template). See dynamics.comp exploration.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

## Notes

- Functionally identical wrapper to comp/limiter. The only difference is the chunkware algorithm type (SimpleGate vs SimpleComp vs SimpleLimit).
- Gate behaviour: unlike comp which reduces gain proportionally above threshold, gate attenuates signals *below* threshold.
