# Convolution Reverb - C++ Exploration (Step 3)

## Source Files

- `hi_core/hi_modules/effects/fx/Convolution.h` (119 lines) - ConvolutionEffect class
- `hi_core/hi_modules/effects/fx/Convolution.cpp` (289 lines) - ConvolutionEffect implementation
- `hi_dsp_library/dsp_basics/ConvolutionBase.h` (386 lines) - ConvolutionEffectBase, MultithreadedConvolver, GainSmoother
- `hi_dsp_library/dsp_basics/ConvolutionBase.cpp` (687 lines) - Core DSP: processBase, reloadInternal, IR preparation

**Base class:** `MasterEffectProcessor`, `AudioSampleProcessor`, `ConvolutionEffectBase`

## Signal Path

The processing happens in `processBase()` (ConvolutionBase.cpp:372-572), called from `applyEffect()` (Convolution.cpp:237-266).

**Steady-state signal flow:**

1. Input sanitised (NaN/Inf removed)
2. If reloading or processing disabled (no ramp): apply dry gain only, return early
3. Convolve input L/R through separate `TwoStageFFTConvolver` engines into wet buffer
4. Apply smoothed dry gain to the original input buffer (in-place)
5. If predelay > 0: pass wet buffer through per-channel delay lines (per-sample)
6. Apply smoothed wet gain to wet buffer
7. Add wet to dry with a fixed 0.5x multiplier: `output = dry * DryGain + wet * WetGain * 0.5`

**Key detail:** The 0.5x multiplier (ConvolutionBase.cpp:563-566) is hardcoded and applied after the wet gain smoother. At default settings (DryGain = -100 dB, WetGain = 0 dB), the output is the convolved signal at half amplitude with no dry signal.

## Gap Answers

### signal-path-order

**Question:** What is the complete processing order in applyEffect/processBase?

**Answer:** `applyEffect()` wraps the input buffer into a `ProcessDataDyn` and calls `processBase()`. The processing order is:

1. Acquire read lock on `swapLock`
2. Sanitise input channels
3. Early exit if reloading or (not processing and not ramping) - applies dry gain only
4. Convolve: three code paths (see ir-crossfade below for the crossfade path)
   - Normal path: `convolverL->process(inputL, wetL)` and same for R
   - Smooth-input path (first block after re-enabling): fade-in ramp on input, clean pipeline, convolve
   - Crossfade path (during IR swap): run both old and new engines, squared crossfade
5. Apply dry gain (smoothedGainerDry) to original input buffer in-place
6. Mix wet into output:
   - During ramp (ProcessInput toggle): per-sample ramp with `0.5 * wetGain * rampValue`
   - Steady state: optional predelay, then smoothedGainerWet on wet buffer, then `addWithMultiply(dry, wet, 0.5f)`

The dry and wet paths are independent - the dry gain is applied to the original input while the wet gain is applied to a separate buffer, then they are summed.

### ir-loading-pipeline

**Question:** How is the impulse response prepared before convolution?

**Answer:** IR preparation happens in `reloadInternal()` (ConvolutionBase.cpp:574-684), called on the message thread (async update). The pipeline:

1. Copy the original buffer from the AudioSampleProcessor (thread-safe read lock)
2. Resample to host sample rate using Lagrange interpolation (`prepareImpulseResponse`)
3. If damping < 0 dB: apply exponential fadeout (`applyExponentialFadeout`) - multiplies each sample by `base + (1-base) * exp(i / (-length/4))` where base = linear damping value
4. If HiCut < 20000 Hz: apply high-frequency damping (`applyHighFrequencyDamping`) - two cascaded one-pole low-pass filters with frequency that exponentially decreases along the IR from 20 kHz down to HiCut. Processed in 64-sample chunks.
5. Sanitise the resampled buffer (remove denormals)
6. Create two new convolver engines (L/R) with the selected FFT type
7. Initialise with headSize = nextPowerOfTwo(lastBlockSize), tailSize = min(8192, max(headSize, nextPowerOfTwo(irLength - headSize)))
8. Prime engines by processing a silent buffer (up to 2048 samples) to warm the pipeline
9. Swap old engines to fadeOut slots, install new engines under write lock

Damping and HiCut are applied to the IR offline. Changing either parameter triggers a full IR reload, not real-time filtering.

### ir-crossfade

**Question:** How does the module handle impulse response changes at runtime?

**Answer:** When a new IR is loaded (or Damping/HiCut/Latency/FFTType change), `reloadInternal()` swaps the old convolver engines into `fadeOutConvolverL/R` and installs new engines. During `processBase()`, if `fadeOutConvolverL != nullptr`, both old and new engines process the input simultaneously:

- The new engine receives input multiplied by a fade-in curve: `gain = fadeValue^2` (squared for smooth onset)
- The old engine receives the unmodified input
- The old engine's output is multiplied by a fade-out curve: `gain = (1 - fadeValue)^2`
- Outputs are summed

`fadeValue` increments by `fadeDelta = 1 / (0.02 * sampleRate)` per sample, giving a ~20ms crossfade. Once `fadeValue >= 1.0`, the old engines are queued for deletion on the background thread.

### dry-wet-mixing

**Question:** How are DryGain and WetGain applied? Is there any fixed gain compensation?

**Answer:** Both gains use the `GainSmoother` class with fast-mode exponential smoothing (coefficient 0.99). DryGain is applied to the original input buffer in-place. WetGain is applied to the wet buffer. The wet buffer is then added to the output with a fixed 0.5x multiplier:

```
output[i] = input[i] * smoothedDryGain + wetBuffer[i] * smoothedWetGain * 0.5
```

The 0.5x is hardcoded at ConvolutionBase.cpp:563. This means even at WetGain = 0 dB (linear 1.0), the effective wet contribution is halved (-6 dB). The default DryGain of -100 dB (effectively silent) means the module is 100% wet by default at -6 dB effective level.

The constructor (ConvolutionBase.cpp:189-190) confirms: `smoothedGainerWet` is initialised to gain 1.0 and `smoothedGainerDry` to gain 0.0.

### latency-parameter-usage

**Question:** How does the Latency parameter affect the convolution engine?

**Answer:** The `latency` member is stored as an int in `setInternalAttribute` (Convolution.cpp:143-145) and triggers `setImpulse(sendNotificationAsync)` which reloads the IR. However, in `reloadInternal()`, the head block size is derived from `lastBlockSize` (the audio buffer size), not from `latency`:

```cpp
int headSize = lastBlockSize;          // line 610
headSize = nextPowerOfTwo(headSize);   // line 621
```

The `latency` member is never read in the signal path or IR reload. It is stored and serialised but has no effect on processing. The metadata range (0-1) is the default uninitialised range, inconsistent with the description "in samples, power of two". The `jassert(isPowerOfTwo(latency))` fires on the default value of 0 in debug builds.

### impulselength-vestigial

**Question:** Is the ImpulseLength parameter truly non-functional?

**Answer:** Yes. In `getAttribute()` (Convolution.cpp:122), ImpulseLength returns a hardcoded `1.0f` regardless of any stored state. In `setInternalAttribute()` (Convolution.cpp:147-148), the handler calls `setImpulse(sendNotificationAsync)` but stores nothing - the reload uses the full buffer range from the AudioSampleProcessor. The parameter is fully vestigial: it is serialised for backwards compatibility but has no effect on DSP or UI. The description correctly says "deprecated".

### processinput-toggle

**Question:** What happens when ProcessInput is toggled?

**Answer:** Toggling ProcessInput calls `enableProcessing()` (ConvolutionBase.cpp:231-244), which sets a `rampFlag` with direction (`rampUp`). In `processBase()`, the ramp path (lines 513-534) applies a per-sample gain ramp:

- Ramp time: 60ms (`CONVOLUTION_RAMPING_TIME_MS = 60`)
- Ramp shape: quadratic (`rampValue *= rampValue`)
- Ramp gain: `0.5 * wetGain * rampValue` (same 0.5x compensation)

When disabling: ramps from current level to zero over 60ms. When re-enabling: sets `smoothInputBuffer = true` (first block gets a linear fade-in on the input to avoid a click from the pipeline), then ramps from zero to target over 60ms.

### background-thread-behaviour

**Question:** How does UseBackgroundThread change the convolution processing?

**Answer:** The `TwoStageFFTConvolver` splits the IR into a head (short, low-latency) and tail (long, bulk of the IR). The head convolver always runs on the audio thread. The tail convolver's processing is dispatched via `startBackgroundProcessing()`:

- **Off** (default): `doBackgroundProcessing()` runs synchronously on the audio thread. Simpler but higher audio thread load.
- **On**: Tail processing is queued to a dedicated `BackgroundThread` and processed asynchronously. The audio thread calls `waitForBackgroundProcessing()` which busy-waits (with jassert) if the background work is not done.

The background thread is disabled in non-realtime mode (e.g. offline rendering) regardless of the toggle. The thread uses a lock-free queue and runs at realtime priority.

### ffttype-options

**Question:** What are the available FFTType options (0-4)?

**Answer:** The enum `audiofft::ImplementationType` defines:

| Value | Name | Platform |
|-------|------|----------|
| 0 | BestAvailable | Auto-selects the best available on the current platform |
| 1 | IPP | Intel Performance Primitives (Intel CPUs, if installed) |
| 2 | AppleAccelerate | macOS/iOS only (Accelerate framework) |
| 3 | Ooura | Platform-independent, pure C++ fallback |
| 4 | FFTW3 | FFTW library (if linked) |

The default is 0 (BestAvailable), which resolves to the highest-performance available implementation at compile time. On Windows without IPP/FFTW3, this is Ooura. On macOS, AppleAccelerate. Changing FFTType triggers a synchronous IR reload (`sendNotificationSync`).

### interface-audiosampleprocessor

**Question:** How does the AudioSampleProcessor interface provide the impulse response?

**Answer:** `ConvolutionEffect` inherits `AudioSampleProcessor` which provides `getBuffer()` returning a `MultiChannelAudioBuffer`. The effect also implements `MultiChannelAudioBuffer::Listener` with two callbacks:

- `bufferWasLoaded()`: called when a new audio file is loaded - calls `setImpulse(sendNotificationSync)`
- `bufferWasModified()`: called when the buffer content changes (e.g. sample range edit) - calls `setImpulse(sendNotificationSync)`

Both trigger `reloadInternal()` which copies the buffer, resamples, applies damping/HiCut, and initialises new convolver engines. The AudioSampleProcessor interface handles file reference resolution, pool management, and the waveform display.

### convolution-performance

**Question:** What is the CPU cost profile?

**Answer:** The `TwoStageFFTConvolver` uses a two-stage non-uniform partitioned convolution:

- **Head convolver**: processes the first `nextPowerOfTwo(blockSize)` samples of the IR using a standard FFT convolver. Runs every audio callback. CPU cost scales with block size.
- **Tail convolver**: processes the remainder of the IR using a larger FFT size (up to 8192 samples). Can run on a background thread.

**CPU scaling factors:**
- IR length: longer IRs mean larger tail convolution work
- Block size: determines head convolver size (lower latency = smaller head = more overhead per sample)
- UseBackgroundThread: moves tail to background, reducing audio thread load but not total CPU
- FFTType: platform-optimised FFT (IPP, Accelerate) can be significantly faster than Ooura

**Baseline tier:** `very_high` - FFT-based convolution is inherently expensive, processing every sample through the frequency domain. Even with the two-stage optimisation, it is one of the most CPU-intensive module types.

## Processing Chain Detail

1. **Input sanitisation** - removes NaN/Inf values. Cost: negligible.
2. **Early exit check** - if reloading or disabled. Cost: negligible.
3. **Convolution (head)** - FFT-based convolution of the head portion. Cost: high (per-block FFT).
4. **Convolution (tail)** - FFT-based convolution of the tail portion. Cost: very_high (larger FFT, bulk of work). Optionally on background thread.
5. **Dry gain** - smoothed gain on original input. Cost: negligible.
6. **Predelay** - per-sample delay line read/write (if > 0). Cost: low.
7. **Wet gain** - smoothed gain on wet buffer. Cost: negligible.
8. **Output mix** - additive mix with 0.5x multiplier. Cost: negligible.

## Conditional Behaviour

| Condition | Effect |
|-----------|--------|
| ProcessInput = Off | Fades out over 60ms, then only dry gain is applied. Re-enabling fades in with input smoothing. |
| UseBackgroundThread = On | Tail convolution dispatched to background thread. Disabled in non-realtime mode. |
| Predelay > 0 | Convolved signal passes through per-sample delay lines before wet gain. |
| Damping < 0 dB | Exponential fadeout applied to IR during reload (offline). |
| HiCut < 20000 Hz | Two cascaded one-pole LPs with decreasing frequency applied to IR during reload (offline). |
| IR crossfade active | Both old and new engines process simultaneously with 20ms squared crossfade. |

## Interface Usage

### AudioSampleProcessor

Provides the impulse response audio buffer. The `MultiChannelAudioBuffer` holds the loaded audio file data. Two listener callbacks (`bufferWasLoaded`, `bufferWasModified`) trigger IR reload. During reload, the buffer is copied under a read lock, resampled to the host sample rate, processed (damping, HiCut), and used to initialise new convolver engines.

The AudioSampleProcessor also provides the waveform display component in the editor and handles audio file pool management.

## Vestigial / Notable

- **ImpulseLength**: getAttribute returns hardcoded 1.0f. setInternalAttribute triggers reload but stores nothing. Fully vestigial, correctly described as deprecated.
- **Latency**: Stored but never consumed in the signal path or IR reload. The head block size is derived from the audio buffer size, not the Latency parameter. The metadata range (0-1) and description ("samples, power of two") are inconsistent.

## CPU Assessment

| Stage | Cost |
|-------|------|
| Input sanitisation | negligible |
| Head convolution (per-block FFT) | high |
| Tail convolution (larger FFT) | very_high |
| Dry gain smoothing | negligible |
| Predelay delay lines | low |
| Wet gain smoothing | negligible |
| Output mixing | negligible |

**Baseline: very_high.** Convolution is the most CPU-intensive built-in effect. The two-stage architecture reduces latency impact but does not reduce total work. UseBackgroundThread redistributes tail cost off the audio thread. Longer IRs increase tail convolution cost. Platform-optimised FFT implementations (IPP, Accelerate) provide measurable speedups over the Ooura fallback.

## UI Components

The editor (`ConvolutionEditor`) is a custom ProcessorEditorBody with sliders, buttons, and peak meters. No FloatingTile content types found.

## Notes

- The 0.5x wet multiplier is a fixed gain compensation, similar to SimpleReverb. It is not user-adjustable.
- The same DSP base class (`ConvolutionEffectBase`) is shared with the scriptnode `fx.convolution` node.
- The crossfade mechanism on IR change is well-designed: both engines run simultaneously with squared fade curves, preventing clicks during IR swaps.
- The tail block size is capped at 8192 samples regardless of IR length, which means very long IRs use multiple tail partitions within the convolver.
- The background thread uses realtime priority and a lock-free queue for job dispatch.
