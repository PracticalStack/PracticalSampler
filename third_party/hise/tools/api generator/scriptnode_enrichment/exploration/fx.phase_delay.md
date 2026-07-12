# fx.phase_delay -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:238`, `FXNodes_impl.h:157`
**Base class:** `polyphonic_base`
**Classification:** audio_processor

## Signal Path

First-order allpass filter implemented via the `AllpassDelay` class from `DelayLine.h`. This is NOT a delay line -- it is a single-sample allpass filter that shifts the phase of the input signal without changing its amplitude.

The `AllpassDelay::getNextSample()` algorithm:

```
y = input * (-delay) + currentValue
currentValue = y * delay + input
return y
```

This is the standard first-order allpass difference equation: `y[n] = -a * x[n] + x[n-1] + a * y[n-1]` where `a` is the delay coefficient.

The Frequency parameter sets the allpass corner frequency. In `setFrequency()`:

```
normalised = frequency / (sampleRate * 0.5)   // note: sr is stored as sampleRate * 0.5
coefficient = (1 - normalised) / (1 + normalised)
```

This is the standard bilinear transform coefficient for a first-order allpass. At the corner frequency, the phase shift is -90 degrees. Below corner: approaching 0 degrees. Above corner: approaching -180 degrees.

Processing iterates channels (max 2 via `index::clamped<2>`), each with its own allpass delay. Both `process()` (block) and `processFrame()` (sample) call `getNextSample()` per sample.

The description says "for comb filtering" because when this allpass output is mixed with the dry signal (e.g. in a container.split), the frequency-dependent phase shift creates constructive/destructive interference -- a comb filter pattern. The node itself does NOT mix dry+wet; it only provides the phase-shifted signal.

## Gap Answers

### phase-delay-algorithm: What type of phase delay is implemented?

A first-order allpass filter using the bilinear transform. The `AllpassDelay` class (DelayLine.h:105) implements a single-pole, single-zero allpass with coefficient computed from the normalised frequency. Not a delay line, not a higher-order filter. The allpass preserves amplitude at all frequencies but shifts phase continuously from 0 to -180 degrees.

### comb-filter-topology: Full comb filter or just phase delay component?

Just the phase delay component. There is no feedback, no dry/wet mix, no summing with the original signal. The node outputs only the allpass-filtered signal. To create a comb filter, place this node inside a `container.split` so the output is summed with the dry signal. The description "for comb filtering" describes the intended use case, not the node's standalone behaviour.

### frequency-to-delay-mapping: What does Frequency control?

The allpass corner frequency. At this frequency, phase shift is exactly -90 degrees. `setFrequency()` normalises: `freq / (sr * 0.5)` then computes the allpass coefficient via `(1 - norm) / (1 + norm)`. This is the standard bilinear transform mapping. Note that `sr` is stored as `sampleRate * 0.5` in `prepare()`, so the division `frequency / sr` already produces the normalised frequency relative to Nyquist.

### processing-model: Per-sample or per-block?

Per-sample within a block loop. `process()` iterates channels, then iterates samples calling `getNextSample()` per sample. No SIMD, no block optimisation possible due to the feedback nature of the allpass. `processFrame()` does the same per-sample across channels.

## Parameters

- **Frequency (enum index 0):** Range 20-20000 Hz, step 0.1, skew for centre 1000 Hz, default 400 Hz. TextConverter: Frequency. Sets the allpass corner frequency where phase shift = -90 degrees.

## Polyphonic Behaviour

Uses `span<PolyData<AllpassDelay, NumVoices>, 2>` -- two allpass delays (one per channel), each with per-voice state. Each voice has independent allpass filter state (current value). The delay coefficient is shared across voices (set via `setFrequency()` which iterates all voices).

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []

Two multiply-add operations per sample per channel. Minimal state (one float per channel per voice). Very efficient.

## Notes

Channel count is clamped to 2 via `index::clamped<2>`. With more than 2 input channels, only the first 2 are processed; remaining channels pass through unmodified. With 1 channel, only one allpass is used.
