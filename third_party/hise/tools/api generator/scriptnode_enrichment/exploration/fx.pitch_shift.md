# fx.pitch_shift -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:418` (pitch_shift class), `FXNodes.h:361` (VoiceData helper)
**Base class:** `HiseDspBase`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Real-time pitch shifting using the Signalsmith time-stretcher library. The node resamples the input to change pitch while maintaining duration.

The processing chain has two stages inside `VoiceData::process()`:

1. **Resampling stage:** Input samples are linearly interpolated to produce a resampled buffer. The `uptimeDelta` (= FreqRatio) controls the resampling ratio. For example, at FreqRatio=2.0, the input is read at double speed, producing half as many output samples from the same input duration. Linear interpolation between adjacent samples smooths the resampling. `lastValues[]` stores the final sample of each channel for cross-block continuity.

2. **Time-stretching stage:** The resampled buffer is passed to `stretcher.process(ptrs, numInput, src, numOutput)` which uses the Signalsmith library to time-stretch back to the original duration. This compensates for the duration change from resampling, preserving the original tempo while shifting pitch.

The number of resampled input samples is: `numInput = ceil(numSamples / uptimeDelta - uptime)`. The `uptime` accumulator tracks fractional sample position across blocks via `std::fmod(thisUptime, 1.0)`.

`processFrame()` contains `jassertfalse` -- frame-based processing is NOT supported. This node requires block-based processing only.

The `process()` method in pitch_shift delegates to `VoiceData::process()` via `stretchers.get()`, passing the ProcessData cast to `ProcessDataDyn` and the shared `resampleBuffer`.

## Gap Answers

### signalsmith-integration: How is the Signalsmith timestretcher integrated?

Via the `time_stretcher` class from `hi_streaming/timestretch/time_stretcher.h`. This is an abstraction layer over pluggable stretch engines (registered via `EngineFactoryFunction`). The `configure(numChannels, sampleRate)` call sets up the stretcher. `setResampleBuffer(1.0, nullptr, 0)` is called in `VoiceData::prepare()` -- the ratio of 1.0 means the stretcher handles time correction only, while the resampling is done manually in `VoiceData::process()`. The actual algorithm depends on which engines are registered at runtime, but Signalsmith is the default.

### freqratio-semantics: FreqRatio range and meaning

FreqRatio is a multiplicative pitch factor. 1.0 = no shift. 2.0 = one octave up. 0.5 = one octave down. 0.25 = two octaves down. 4.0 = two octaves up. The skew of 0.43 (set via `setSkewForCentre(1.0)`) makes the knob center at 1.0. At FreqRatio=1.0, `uptimeDelta=1.0`, the resampling is 1:1, and the stretcher receives the same number of samples as output -- effective pass-through. `setFreqRatio()` clamps to `jlimit(0.25, 4.0, newValue)` and rejects 0.0.

### latency: Does the node introduce latency?

The `time_stretcher` class has a `getLatency(ratio)` method, but `pitch_shift` does not call it or report latency to the host. The stretcher internally uses overlap-add windowing which introduces inherent latency (typically one window length). The `skipLatency()` method exists in `time_stretcher` but is not called by `pitch_shift`. Users should expect some latency but it is not compensated.

### quality-and-artifacts: Quality settings

No quality parameters are exposed. The stretcher is configured with defaults via `configure(numChannels, sampleRate)`. The `setFFTSize()` method exists on `time_stretcher` but is not called by `pitch_shift`. Quality depends on the registered engine implementation. At extreme ratios (0.25, 4.0), time-stretching artifacts (phasiness, smearing) are expected. The linear interpolation in the resampling stage adds minimal aliasing.

### polyphonic-memory: Per-voice memory footprint

Yes, per-voice. `PolyData<VoiceData, NumVoices> stretchers` -- each voice has a full `VoiceData` containing a `time_stretcher` instance (with its own internal buffers) and a `span<float, NUM_MAX_CHANNELS> lastValues`. The shared `resampleBuffer` is a `heap<float>` sized to `numChannels * blockSize * 4` -- this is shared across voices (not per-voice). The `time_stretcher` internal buffers are the main memory cost per voice -- likely several KB each depending on FFT size.

## Parameters

- **FreqRatio (enum index 0):** Range 0.25-4.0, step 0 (continuous), skew for centre 1.0, default 1.0. Multiplicative pitch ratio. No TextConverter. Stored as `uptimeDelta` inside per-voice `VoiceData`.

## Polyphonic Behaviour

Per-voice via `PolyData<VoiceData, NumVoices>`. Each voice has independent stretcher state, uptime accumulator, and last-sample buffer. The `resampleBuffer` (scratch memory for resampling) is shared -- this is safe because `process()` accesses only the current voice's stretcher via `stretchers.get()`, and voices are processed sequentially. The `initValue` and `lastSpecs` members are shared across voices.

## CPU Assessment

baseline: high
polyphonic: true
scalingFactors: [{"parameter": "FreqRatio", "impact": "extreme ratios increase stretcher workload", "note": "At 0.25 or 4.0, the stretcher must process 4x or 0.25x the input samples respectively"}]

The time stretcher uses FFT-based overlap-add processing internally. This is significantly more expensive than simple DSP operations. Per-voice instances multiply the cost. Frame processing is explicitly unsupported (jassertfalse).

## Notes

The constructor passes `false` for `addProcessEventFlag` to `polyphonic_base` -- no MIDI event processing. The `initValue` member defaults to 1.0 and is used in `prepare()` to re-apply the ratio after reallocation. If `setFreqRatio(0.0)` is called, it returns early (guard). If called before `prepare()` (empty resampleBuffer or invalid lastSpecs), it also returns early.
