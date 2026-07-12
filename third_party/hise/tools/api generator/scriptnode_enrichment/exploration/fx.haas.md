# fx.haas -- C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/FXNodes.h:523`, `FXNodes_impl.h:224`
**Base class:** `HiseDspBase`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Stereo panning effect using inter-channel delay (Haas effect). One channel is delayed by up to 20ms while the other passes through undelayed, creating the psychoacoustic perception of stereo position.

The node is hardcoded to 2 channels: `static const int NumChannels = 2`. Uses `DelayLine<2048, DummyCriticalSection>` per channel (2048 samples max buffer -- at 44.1kHz this is ~46ms, at 96kHz ~21ms).

`process()` calls `processBlock()` on each delay line with raw channel pointers. `processFrame()` calls `getDelayedValue()` per channel. Both channels always pass through their respective delay lines.

The Position parameter controls which channel is delayed:
- Position = 0: both delay lines set to 0 samples -- pass-through
- Position > 0 (right panning): left channel delayed by `|position| * 0.02` seconds, right channel undelayed
- Position < 0 (left panning): right channel delayed by `|position| * 0.02` seconds, left channel undelayed

Maximum delay: `1.0 * 0.02 = 0.02` seconds = 20ms. This is hardcoded in `setPosition()`.

## Gap Answers

### delay-implementation: How is the delay implemented?

Uses `DelayLine<2048, DummyCriticalSection>` from `hi_dsp_library/dsp_basics/DelayLine.h`. This is a circular buffer delay line with optional crossfade for delay time changes. Buffer size is 2048 samples. `DummyCriticalSection` means no locking (audio-thread safe). `setDelayTimeSeconds()` converts seconds to samples using the stored sample rate. The delay line supports `AllowFade=true` (default from template), but `setFadeTimeSamples(0)` is called in `reset()`, disabling crossfade on reset. No interpolation for sub-sample delay -- `setDelayTimeSamples()` takes integer samples.

### channel-handling: How does the node handle stereo?

Hardcoded to 2 channels via `static const int NumChannels = 2`. ProcessType is `ProcessData<2>`. The node requires stereo input. With mono input the behaviour is undefined (the process signature requires exactly 2 channels). With >2 channels, only the first 2 are processed.

### position-mapping: Position semantics

Position -1 to 1 maps linearly to delay. `d = abs(position) * 0.02` (seconds). At position=0, both channels have 0 delay (bypass). At position=1.0, left channel is delayed 20ms, right is dry -- sound appears to come from the right. At position=-1.0, right channel is delayed 20ms, left is dry -- sound appears to come from the left. The mapping is linear (no skew applied to the delay calculation).

### polyphonic-state: Per-voice delay buffers?

Yes. `PolyData<DelayType, NumVoices> delay` where `DelayType = std::array<DelayLine<2048, DummyCriticalSection>, 2>`. Each voice has two delay line instances, each with a 2048-sample buffer. Memory per voice: 2 * 2048 * sizeof(float) = 16KB. At 256 voices: ~4MB total.

## Parameters

- **Position (enum index 0):** Range -1.0 to 1.0, step 0.1, default 0.0. Controls stereo position via inter-channel delay. Negative = sound from left, positive = sound from right. Linear mapping to 0-20ms delay.

## Polyphonic Behaviour

Full per-voice state. Each voice has an independent pair of delay lines (left and right). The `position` member is a plain `double`, NOT wrapped in PolyData -- it is shared across all voices. However, `setPosition()` iterates all voices via `for (auto& d_ : delay)` to set delay times, so the delay time setting is applied to all voices' delay lines uniformly.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: []

Per-sample delay line processing (read + write to circular buffer). Very efficient. Memory footprint is the main concern for polyphonic use (16KB per voice).

## Notes

The `haas` constructor passes `false` for `addProcessEventFlag` to `polyphonic_base`, meaning the node does NOT process MIDI events. The delay buffer size of 2048 samples limits the maximum usable sample rate: at 96kHz, 2048 samples = 21.3ms, still above the 20ms max delay. At 192kHz, 2048 = 10.7ms, which is below 20ms -- the delay line would clip. However, the `DelayLine::setDelayTimeSeconds()` likely clamps internally.
