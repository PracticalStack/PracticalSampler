---
title: Pitch Shift
description: "Real-time pitch shifting using time-stretch-based resampling, with a range of two octaves up or down."
factoryPath: fx.pitch_shift
factory: fx
polyphonic: true
tags: [fx, pitch, time-stretch]
screenshot: /images/v2/reference/scriptnodes/fx/pitch_shift.png
cpuProfile:
  baseline: high
  polyphonic: true
  scalingFactors:
    - { parameter: FreqRatio, impact: high, note: "Extreme ratios (near 0.25 or 4.0) increase time stretcher workload" }
seeAlso:
  - { id: "core.pitch_mod", type: disambiguation, reason: "Pitch modulation for oscillators versus pitch shifting of audio signals" }
commonMistakes:
  - title: "High CPU cost in polyphonic context"
    wrong: "Using fx.pitch_shift per voice in a polyphonic network with many voices"
    right: "Minimise voice count or use fx.pitch_shift as a monophonic bus effect where possible."
    explanation: "Each voice maintains its own time stretcher with significant memory and CPU overhead. In polyphonic contexts the cost scales linearly with voice count."
  - title: "Requires block-based processing"
    wrong: "Placing fx.pitch_shift inside a container.frame2_block or other frame-processing context"
    right: "Use fx.pitch_shift in a block-processing context only (the default for most containers)."
    explanation: "The node does not support frame-based (per-sample) processing. It must operate in a block-processing container such as container.chain."
  - title: "Introduces processing latency"
    wrong: "Expecting zero-latency pitch shifting for time-critical effects"
    right: "Account for inherent latency from the overlap-add windowing used by the time stretcher."
    explanation: "The time stretcher uses FFT-based overlap-add processing which introduces latency of at least one window length. This latency is not automatically compensated."
llmRef: |
  fx.pitch_shift

  Real-time pitch shifting that resamples the input at a different rate and then time-stretches back to the original duration. Supports a range of two octaves down to two octaves up.

  Signal flow:
    audio in -> resample at FreqRatio speed (linear interpolation) -> time stretch back to original duration -> audio out

  CPU: high, polyphonic. Each voice has its own time stretcher instance.

  Parameters:
    FreqRatio (0.25 - 4.0, skewed centre 1.0, default 1.0) - pitch multiplier. 0.5 = one octave down, 1.0 = no shift, 2.0 = one octave up. Continuous (no stepping).

  When to use:
    Real-time pitch shifting of audio signals within scriptnode. For pitch modulation of oscillators, use core.pitch_mod instead. Be aware of CPU cost in polyphonic contexts.

  Common mistakes:
    High CPU per voice - minimise polyphonic voice count.
    Frame processing not supported - use block-based containers only.
    Inherent latency from overlap-add windowing.

  See also:
    disambiguation core.pitch_mod - pitch modulation for oscillators, not audio pitch shifting
---

Real-time pitch shifting that changes the pitch of an audio signal without altering its duration. The node works in two stages: first it resamples the input at a rate determined by FreqRatio, then it applies time stretching to correct the duration back to the original speed. This produces pitch-shifted audio that plays at the original tempo.

The FreqRatio parameter is a multiplicative pitch factor. At 1.0 the signal passes through unchanged. At 2.0 the pitch is shifted one octave up; at 0.5, one octave down. The full range spans two octaves in either direction (0.25 to 4.0). The knob is skewed so that 1.0 (no shift) sits at the centre of the control range.

Quality is generally good for moderate pitch shifts (within roughly one octave). At extreme ratios approaching 0.25 or 4.0, time-stretching artefacts such as phasiness or spectral smearing become more noticeable. The node introduces some processing latency due to the overlap-add windowing used internally.

## Signal Path

::signal-path
---
glossary:
  parameters:
    FreqRatio:
      desc: "Pitch multiplier: 0.25 = -2 octaves, 0.5 = -1 octave, 1.0 = no shift, 2.0 = +1 octave, 4.0 = +2 octaves"
      range: "0.25 - 4.0"
      default: "1.0"
  functions:
    resample:
      desc: "Reads input at a different rate using linear interpolation"
    timeStretch:
      desc: "Corrects the duration back to the original speed using overlap-add processing"
---

```
// fx.pitch_shift - resample + time stretch
// audio in -> audio out

process(input) {
    // Stage 1: resample at FreqRatio speed
    // FreqRatio > 1 reads faster (higher pitch, shorter duration)
    // FreqRatio < 1 reads slower (lower pitch, longer duration)
    resampled = resample(input, FreqRatio)

    // Stage 2: time stretch back to original duration
    output = timeStretch(resampled)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label:
    params:
      - { name: FreqRatio, desc: "Pitch multiplier. 0.25 shifts two octaves down, 0.5 one octave down, 1.0 is no change, 2.0 one octave up, 4.0 two octaves up. The knob is skewed so 1.0 sits at the centre. Continuous values are supported for fine pitch adjustment.", range: "0.25 - 4.0", default: "1.0" }
---
::

At FreqRatio=1.0 the resampling is 1:1 and the time stretcher receives an equal number of input and output samples, producing an effective pass-through with minimal overhead.

**See also:** $SN.core.pitch_mod$ -- pitch modulation for oscillators (not audio pitch shifting)
