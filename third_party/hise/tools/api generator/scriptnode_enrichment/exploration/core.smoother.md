# core.smoother - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CoreNodes.h:2228`
**Base class:** `mothernode`, `polyphonic_base`
**Classification:** audio_processor

## Signal Path

Applies a low-pass smoothing filter to channel 0 of the input signal. In `process()` (line 2304): `smoothers.get().smoothBuffer(data[0].data, data.getNumSamples())` -- operates on channel 0 only using `hise::Smoother::smoothBuffer()`. In `processFrame()` (line 2300): `data[0] = smoothers.get().smooth(data[0])` -- again channel 0 only.

The `hise::Smoother` class is a one-pole IIR lowpass filter (exponential smoothing). It processes only the first channel -- other channels pass through unmodified.

On MIDI note-on, `handleHiseEvent()` calls `reset()` which resets the smoother to the DefaultValue.

## Gap Answers

### filter-type: What filter is used?

One-pole IIR lowpass (exponential smoothing). The `hise::Smoother` class provides `smooth(float input)` for per-sample and `smoothBuffer(float*, int)` for block processing. `setSmoothingTime()` sets the coefficient, `resetToValue()` sets the state.

### processing-model: Per-sample or per-block? Channel handling?

Block processing on channel 0 only via `smoothBuffer()`. Other channels are not processed. For frame processing, `data[0]` is filtered; other frame elements are untouched.

### default-value-usage: What does DefaultValue do?

`setDefaultValue()` (line 2323) calls `resetToValue(defaultValue, smoothingTimeMs)` on all voice smoothers. `reset()` (line 2291) also resets to `defaultValue`. This sets the smoother's internal state so the output starts at DefaultValue and smoothly transitions to the input signal. On voice start (reset) or note-on, the output jumps to DefaultValue and then tracks the input.

## Parameters

- **SmoothingTime** (0-2000 ms, default 100): Filter time constant. Higher = slower tracking.
- **DefaultValue** (0-1, default 0): Initial output value on voice start/reset.

## Polyphonic Behaviour

`PolyData<hise::Smoother, NumVoices> smoothers` provides per-voice smoother state. Reset on note-on via `handleHiseEvent()`.

## CPU Assessment

baseline: negligible
polyphonic: true
scalingFactors: []
