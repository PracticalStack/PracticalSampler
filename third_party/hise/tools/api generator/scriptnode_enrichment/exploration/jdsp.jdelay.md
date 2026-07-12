# jdsp.jdelay -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/JuceNodes.h:301-411`
**Base class:** `jdelay_base<LinearDelay, NV>` where `LinearDelay = juce::dsp::DelayLine<float, Linear>`
**Classification:** audio_processor, polyphonic

## Signal Path

Input audio -> per-channel JUCE DelayLine with linear interpolation -> Output audio.

The jwrapper base converts ProcessData to AudioBlock and calls DelayLine::process(). Each channel is delayed independently by the same delay time. For frame processing, jwrapper falls back to creating a single-sample AudioBlock since DelayLine has no processSample method.

## Gap Answers

### signal-path-processing

jwrapper::process() creates a juce::dsp::AudioBlock and passes it to DelayLine::process(ProcessContextReplacing). The JUCE DelayLine processes each channel through an independent circular buffer. All channels share the same delay time setting. The delay is applied per-block via the jwrapper pattern.

### limit-parameter-behaviour

setParameter<0> (Limit) converts ms to samples (`v * 0.001 * sr`) and calls obj.setMaxDelaySamples(roundToInt(sampleValue)). This can be called at runtime -- the JUCE DelayLine internally resizes its buffer. However, if prepare() has not yet been called (sr <= 0), the value is deferred: stored in `maxSize` and applied in the next prepare(). This deferred mechanism ensures safe initialisation order.

### delay-time-smoothing

Confirmed: there is no smoothing on the DelayTime parameter. setParameter<1> converts ms to samples and calls obj.setDelay(sampleValue) directly. The JUCE DelayLine applies the new delay immediately. Users must add external smoothing (e.g., control.smoothed_parameter) to avoid clicks from abrupt delay time changes.

### linear-interpolation-lowpass

The low-pass filtering effect described is an inherent artefact of linear interpolation, not an explicit filter. When the delay time is not an integer number of samples, linear interpolation between adjacent samples acts as a first-order low-pass filter. The attenuation increases with frequency, reaching -3dB at Nyquist/2 for the worst case (0.5 sample fractional delay). This is more noticeable at lower sample rates and with modulated delay times.

### polyphonic-state

Each voice gets its own DelayLine instance via `PolyData<LinearDelay, NumVoices> objects` (inherited from jwrapper). The PolyData::get() method returns the current voice's DelayLine, ensuring complete voice isolation. Each voice maintains independent delay buffer state.

### cpu-profile

Linear interpolation requires a single multiply-add per sample per channel -- the cheapest of the three interpolation types. Per-block processing with minimal overhead. The Limit parameter affects memory usage (buffer size) but not CPU cost.

## Parameters

- P=0 Limit: converts ms to samples, calls setMaxDelaySamples(). Deferred if sr not yet set. Range is 0-1000ms for monophonic, 0-30ms for polyphonic.
- P=1 DelayTime: converts ms to samples, calls setDelay(). Deferred if sr not yet set. Same range as Limit.

**Important:** When polyphonic (NV > 1), the parameter range is reduced from 0-1000ms to 0-30ms. This is set in createParameters() with `InvertableParameterRange(0.0, 30.0)`. The default Limit becomes 30ms (nr.rng.end) instead of 1000ms.

## Polyphonic Behaviour

Inherits from polyphonic_base(getStaticId(), false) -- the `false` means IsProcessingHiseEvent is NOT registered. Each voice gets an independent LinearDelay instance via PolyData. Voice isolation is complete: separate delay buffers, separate delay time tracking.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors:
  - parameter: "Limit", impact: "memory", note: "Larger Limit allocates more buffer memory but does not increase CPU"

## Notes

The three jdelay variants share the jdelay_base template entirely. The only difference is the DT template parameter which selects the JUCE interpolation type. The constructor sets an initial maximum delay of 1024 samples. The ms-to-samples conversion uses: `sampleValue = v * 0.001 * sr`. FloatSanitizers::sanitizeFloatNumber is called on the converted value to guard against NaN/Inf.
