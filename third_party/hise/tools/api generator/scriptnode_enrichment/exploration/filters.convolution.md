# filters.convolution -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/ConvolutionNode.h:43` (convolution struct), `hi_dsp_library/dsp_basics/ConvolutionBase.h:269` (ConvolutionEffectBase)
**Base class:** `data::base`, `ConvolutionEffectBase` (NOT FilterNodeBase -- completely separate architecture)
**Classification:** audio_processor

## Signal Path

Input audio is convolved with an impulse response (IR) loaded from an AudioFile slot. The convolution uses partitioned FFT via `MultithreadedConvolver` (uniform partitioned convolution). Separate convolver instances for left and right channels.

Processing flow (`processBase`):
1. Sanitize input (remove NaN/inf)
2. If not processing (gate off or reloading), apply dry gain only and return
3. Convolve input through `convolverL->process()` and `convolverR->process()` into wet buffer
4. Apply dry gain to original signal
5. If ramping (gate just toggled), apply crossfade ramp to wet signal
6. If predelay > 0, run wet through delay lines
7. Apply wet gain smoother
8. Add wet buffer to output with 0.5 multiplier

The convolution is a 100% wet effect mixed at 50% with the dry signal (hardcoded 0.5 multiplier). The wet/dry ratio is not user-controllable.

Frame-based processing (`processFrame`) triggers `jassertfalse` -- convolution does NOT support frame processing. It will assert in debug builds and do nothing in release.

## Gap Answers

### convolution-processing-model: How is the convolution implemented?

Uniform partitioned FFT convolution via `MultithreadedConvolver`. The head size equals the audio block size (rounded to next power of two). The tail size is up to 8192 samples. The convolution introduces latency equal to the head block size.

The IR is resampled if its sample rate differs from the processing sample rate. Damping and HiCut are applied to the IR during loading (pre-processing), not in real-time.

### convolution-multithread-behaviour: What does Multithread do?

When Multithread=1 (and not in non-realtime mode), the tail portion of the convolution runs on a background thread (`MultithreadedConvolver::BackgroundThread`). The head (first partition) always runs on the audio thread. This reduces audio thread CPU load at the cost of slightly higher overall CPU and potential latency from the background thread scheduling.

When in non-realtime mode (offline rendering), multithreading is automatically disabled regardless of the parameter setting.

### convolution-gate-behaviour: What does Gate=0 do?

Gate=0 calls `enableProcessing(false)` which sets `processFlag=false` and starts a ramp-down. During the ramp, the wet signal fades out over `CONVOLUTION_RAMPING_TIME_MS` milliseconds using a quadratic curve (`rampValue *= rampValue`). After the ramp completes, processing is fully bypassed (only dry gain is applied). This saves CPU and provides click-free gating.

Gate=1 starts a ramp-up with the same duration, and optionally applies input smoothing (fade-in) on the first block to avoid feeding a transient into the convolver.

### convolution-damping-implementation: How does Damping work?

Damping applies an exponential decay envelope to the IR during loading (pre-processing, not real-time). The formula: `multiplier = base + invBase * exp(i / factor)` where `base = targetValue` (converted from dB to linear), `invBase = 1 - targetValue`, `factor = -numSamples / 4`. This shortens the perceived reverb tail. Changing Damping triggers an IR reload.

### convolution-hicut-implementation: How does HiCut work?

HiCut applies a low-pass filter to the IR during loading (pre-processing). It uses two cascaded `SimpleOnePole` filters with frequency that decreases exponentially over the IR length: `freq = base + invBase * exp(i/factor)` where base = cutoffFrequency/20000. This simulates frequency-dependent absorption -- higher frequencies decay faster in the tail. Changing HiCut triggers an IR reload.

### convolution-ir-format: What formats and channels are supported?

The AudioFile slot accepts any format supported by HISE's `MultiChannelAudioBuffer`. The node explicitly disables SampleMap and SFZ providers (`setDisabledXYZProviders`), so only direct audio file references are accepted.

Processing handles mono and stereo: if numChannels > 1, separate convolvers are used for L and R channels. The IR is expected to have at least 2 channels; mono IRs would need to be used for both channels.

### convolution-not-polyphonic: Why is it monophonic only?

`static constexpr bool isPolyphonic() { return false; }`. Convolution is inherently expensive and stateful (the FFT pipeline maintains large internal buffers). Running a separate convolver per polyphonic voice would be prohibitively expensive. This node is designed for bus/master processing.

If placed in a polyphonic voice chain, it processes only the summed voice output (not per-voice).

### description-accuracy: Confirm characterisation.

The description "A convolution reverb node" is accurate. It is correctly placed in the filters factory for architectural reasons (it uses filter-adjacent infrastructure) but is functionally a reverb/IR effect.

## Parameters

- **Gate:** 0 or 1. Enables/disables convolution processing with click-free ramp. When off, saves CPU. Default: 1.
- **Predelay:** 0-1000 ms (step 1). Delay applied to the wet signal before mixing. Uses DelayLine<4096>. Default: 0.
- **Damping:** -100 to 0 dB (skew for center -12). Exponential decay applied to the IR (pre-processing). 0 dB = no damping. Lower values = shorter tail. Default: 0.
- **HiCut:** 20-20000 Hz (skew for center 1000). Frequency-dependent decay applied to the IR (pre-processing). 20000 Hz = no filtering. Default: 20000.
- **Multithread:** 0 or 1. Offloads tail convolution to background thread. Default: 0.

## CPU Assessment

baseline: high
polyphonic: false
scalingFactors: [{ "parameter": "IR length", "impact": "linear", "note": "Longer IRs increase FFT partition count and tail processing time" }]

FFT convolution is inherently expensive. The Multithread parameter can shift tail processing off the audio thread.

## Notes

Key differences from standard filter nodes:
- Does NOT use FilterNodeBase or MultiChannelFilter
- Does NOT support frame processing (asserts false)
- NOT polyphonic
- Uses AudioFile ComplexData (not FilterCoefficients)
- Parameters are completely different (no Frequency/Q/Gain/Mode)
- Damping and HiCut are pre-processing (applied during IR loading), not real-time parameters
- IR reload uses crossfading between old and new convolvers for click-free transitions
- The wet/dry mix is hardcoded at approximately 50%
