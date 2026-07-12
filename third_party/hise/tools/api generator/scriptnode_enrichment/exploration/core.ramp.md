# core.ramp - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:662`
**Base class:** `data::display_buffer_base<UseRingBuffer>`, `polyphonic_base`
**Classification:** audio_processor (also modulation source)

## Signal Path

The ramp generates a 0..1 sawtooth signal based on PeriodTime and ADDS it to the audio signal. In `process()` (line 717): `s += (float)thisUptime` for each sample on each channel. When uptime exceeds 1.0, it wraps back to `loopStart`.

The ramp also outputs its current value as a normalized modulation signal via `handleModulation()` using a per-voice `ModValue`. The `isNormalisedModulation()` returns true.

When `enabled` is false (Gate=0), no processing occurs and the audio passes through unmodified.

## Gap Answers

### modulation-output: How does modulation work?

Both audio and modulation. The ramp value is added to the audio signal AND sent as a modulation output via `ModValue::setModValue()` at the end of each process block. `handleModulation()` (line 748) reads from the current voice's ModValue. The modulation is normalized 0..1.

### loop-start-behaviour: What does LoopStart do?

When the ramp value exceeds 1.0, it wraps back to LoopStart instead of 0.0. See line 733: `if (thisUptime > 1.0) thisUptime = thisState.loopStart;`. This means the ramp cycles between LoopStart and 1.0. Default LoopStart is 0.0 (full 0..1 cycle).

### gate-reset: What happens on Gate transitions?

In `setGate()` (line 806): when Gate transitions from off to on (`shouldBeOn && !s.enabled`), phase resets to 0.0. When Gate=0, `enabled` is set to false and the process loop is skipped entirely -- audio passes through unmodified and no modulation is output.

### processing-model: Audio or modulation only?

Both. The ramp is added to the audio signal (`s += (float)thisUptime`) AND output via modulation. The audio output is additive, not replacing.

## Parameters

- **PeriodTime** (0.1-1000 ms, default 100): Duration of one ramp cycle. Converted to phase increment via `1.0 / (periodTime_sec * sampleRate)`.
- **LoopStart** (0-1, default 0): Phase position where the ramp wraps back to after reaching 1.0.
- **Gate** (0/1, default 1): Enables the ramp. Rising edge resets phase.

## Polyphonic Behaviour

`PolyData<State, NumVoices> state` where State contains OscData (uptime/delta), loopStart, enabled flag, and ModValue. Each voice has independent ramp state.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
