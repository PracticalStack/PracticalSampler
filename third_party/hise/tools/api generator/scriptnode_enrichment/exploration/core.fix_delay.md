# core.fix_delay - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/DelayNode.h:42`
**Base class:** `HiseDspBase`
**Classification:** audio_processor

## Signal Path

Input audio passes through one `DelayLine<>` per channel. Each delay line is a non-interpolating circular buffer that reads from a fixed integer sample offset behind the write head. When the delay time changes, a crossfade of configurable length (FadeTime) smooths the transition between old and new read positions.

Block processing (`process`): iterates channels, calls `DelayLine::processBlock()` on each channel's raw data.
Frame processing (`processFrame`): iterates samples in the frame, calls `DelayLine::getDelayedValue()` per channel.

## Gap Answers

### non-interpolating-meaning

Yes, "non-interpolating" means the delay time is quantised to integer samples. The `setDelayTime()` method converts milliseconds to seconds (`delayTimeSeconds = newValue * 0.001`) and passes this to `DelayLine::setDelayTimeSeconds()`, which internally converts to an integer sample count using the sample rate. The `DelayLine` template uses integer read/write indices with a bitmask (`DELAY_BUFFER_MASK`) for wrapping -- there is no fractional sample interpolation. Fractional delay times are effectively truncated to the nearest integer sample.

### fade-time-units

FadeTime is in **samples**. The `setFadeTime()` method casts the value to int and passes it directly to `DelayLine::setFadeTimeSamples((int)newValue)`. The range 0-1024 with step 1 confirms integer sample units. At 44100 Hz, 512 samples is approximately 11.6 ms.

### channel-handling

Separate delay lines per channel. The `prepare()` method creates one `DelayLine<>` per channel (`delayLines.add(new DelayLine<>())` for each `ps.numChannels`). Each channel's data is processed independently through its own delay line instance. Stereo signals are delayed independently but with the same delay time.

### max-delay-buffer

The `DelayLine` template default parameter is `HISE_MAX_DELAY_TIME_SAMPLES`. The internal buffer is a fixed-size C array (`float delayBuffer[DELAY_BUFFER_SIZE]`), so it is stack/object-allocated at construction time, not dynamically sized. The buffer size must be a power of 2 (the code uses `DELAY_BUFFER_MASK = MaxLength - 1` for wrapping). The DelayTime parameter max of 1000 ms sets the user-facing limit; the actual buffer capacity depends on the `HISE_MAX_DELAY_TIME_SAMPLES` preprocessor define.

## Parameters

- **DelayTime** (0-1000 ms, step 0.1, skew centre 100 ms, default 100 ms): Delay duration in milliseconds. Converted to seconds internally. Applied to all delay lines via `setDelayTimeSeconds()`. Non-interpolating -- quantised to integer samples.
- **FadeTime** (0-1024 samples, step 1, default 512): Crossfade length in samples when delay time changes. Passed directly to `setFadeTimeSamples()`. Prevents clicks when modulating delay time.

## Polyphonic Behaviour

Not polyphonic. No `PolyData` members. Single set of delay lines shared across the node instance.

## CPU Assessment

baseline: low
polyphonic: false
scalingFactors: []

The processing is a simple circular buffer read/write per sample per channel with no transcendental functions or SIMD. The crossfade adds minimal overhead only during delay time transitions.

## Notes

The `SN_EMPTY_HANDLE_EVENT` macro confirms no MIDI processing. The `operator=` copies `delayTimeSeconds` but creates new empty delay lines (used for node duplication). The `prepare()` method calls `reset()` (clearing all delay buffers) then re-applies the current delay time, so changing channel count clears the delay buffer.
