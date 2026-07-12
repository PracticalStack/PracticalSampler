---
title: Ramp
description: "A free-running ramp generator that outputs both a modulation signal and an additive audio signal."
factoryPath: core.ramp
factory: core
polyphonic: true
tags: [core, modulation, ramp, lfo]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.clock_ramp", type: alternative, reason: "Tempo-synced ramp for DAW-locked modulation" }
commonMistakes:
  - title: "Ramp adds to the audio signal"
    wrong: "Expecting the ramp to only output via modulation and leave the audio untouched"
    right: "The ramp value is added to the audio signal on all channels. If you only need modulation output, place the ramp outside the main signal path or use math.clear after it."
    explanation: "The node serves as both a modulation source and an audio generator. The ramp value is always added to the audio buffer, which can introduce a DC offset or unwanted signal if not accounted for."
llmRef: |
  core.ramp

  Free-running ramp generator that cycles from 0 to 1 at a configurable period. Outputs both a normalised modulation signal and an additive contribution to the audio buffer.

  Signal flow:
    ramp(PeriodTime) -> audio += ramp value, modulation output = ramp value

  CPU: negligible, polyphonic

  Parameters:
    PeriodTime (0.1-1000 ms, default 100): Duration of one ramp cycle
    LoopStart (0-100%, default 0%): Phase position the ramp wraps back to after reaching 1.0
    Gate (Off/On, default On): Enables the ramp; rising edge resets phase

  When to use:
    - Free-running LFO-style modulation at a fixed rate
    - Driving waveshapers or table lookups with a periodic ramp
    - Use core.clock_ramp instead when the ramp must lock to DAW tempo

  Common mistakes:
    - Ramp adds to the audio signal on all channels, not just modulation

  See also:
    alternative core.clock_ramp -- tempo-synced ramp
---

The ramp node generates a linear 0-1 sawtooth signal at a configurable period and outputs it in two ways: as a normalised modulation signal (via the modulation output) and as an additive contribution to the audio buffer on all channels. This dual output makes it suitable both as a modulation source for parameter control and as a signal generator for waveshaping pipelines.

When the ramp reaches 1.0 it wraps back to the LoopStart position and continues cycling. Setting LoopStart above zero shortens the effective ramp range, cycling between LoopStart and 1.0.

## Signal Path

::signal-path
---
glossary:
  parameters:
    PeriodTime:
      desc: "Duration of one complete ramp cycle"
      range: "0.1 - 1000 ms"
      default: "100"
    LoopStart:
      desc: "Phase position the ramp wraps back to after reaching 1.0"
      range: "0 - 100%"
      default: "0%"
    Gate:
      desc: "Enables the ramp; rising edge resets phase to zero"
      range: "Off / On"
      default: "On"
---

```
// core.ramp - free-running ramp generator
// audio in -> audio out (input + ramp), modulation out

process(input) {
    if (!Gate) return input    // audio passes through, no modulation

    phase += 1.0 / (PeriodTime * sampleRate)
    if (phase > 1.0) phase = LoopStart

    output = input + phase    // additive on all channels
    modulationOut = phase     // normalised 0..1
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Timing
    params:
      - { name: PeriodTime, desc: "Duration of one complete ramp cycle in milliseconds", range: "0.1 - 1000 ms", default: "100" }
      - { name: LoopStart, desc: "Phase position the ramp wraps back to after reaching 1.0. Increase to shorten the ramp range", range: "0 - 100%", default: "0%" }
  - label: Control
    params:
      - { name: Gate, desc: "Enables or disables the ramp. A rising edge resets the phase to zero. When off, audio passes through unmodified", range: "Off / On", default: "On" }
---
::

**See also:** $SN.core.clock_ramp$ -- tempo-synced ramp for DAW-locked modulation
