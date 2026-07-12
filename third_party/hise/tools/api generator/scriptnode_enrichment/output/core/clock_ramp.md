---
title: Clock Ramp
description: "A tempo-synced ramp generator that locks to the HISE/DAW clock and outputs a modulation signal."
factoryPath: core.clock_ramp
factory: core
polyphonic: true
tags: [core, modulation, ramp, tempo, clock]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.ramp", type: alternative, reason: "Free-running ramp when tempo sync is not needed" }
commonMistakes:
  - title: "AddToSignal is off by default"
    wrong: "Expecting the clock ramp to appear in the audio signal without enabling AddToSignal"
    right: "Set AddToSignal to Yes if you need the ramp value added to channel 0. By default, the node only outputs via modulation."
    explanation: "Unlike core.ramp which always adds to audio, clock_ramp defaults to modulation-only output. Enable AddToSignal explicitly if you need the audio contribution."
llmRef: |
  core.clock_ramp

  Tempo-synced ramp generator that locks to the HISE/DAW clock. Outputs a normalised 0-1 modulation signal, optionally adding it to channel 0 of the audio.

  Signal flow:
    clock sync -> ramp(Tempo * Multiplier) -> modulation out, optionally audio += ramp

  CPU: negligible, polyphonic

  Parameters:
    Tempo (0-18, default 5/1/4): Musical note value for ramp period
    Multiplier (1-16, default 1): Additional period multiplier
    AddToSignal (No/Yes, default No): Whether to add ramp to audio channel 0
    UpdateMode (Continuous/Synced, default Synced): Phase calculation method
    Inactive (Current/Zero/One, default Current): Output when transport is stopped

  When to use:
    - Tempo-synced LFO modulation
    - Rhythmic effects that must lock to DAW tempo
    - Use core.ramp instead for free-running modulation

  Common mistakes:
    - AddToSignal is off by default - enable it if you need audio output

  See also:
    alternative core.ramp -- free-running ramp
---

The clock_ramp generates a 0-1 ramp synchronised to the HISE or DAW transport clock. The ramp completes one cycle per the selected musical note value (set by Tempo and Multiplier), making it suitable for tempo-locked modulation effects such as rhythmic tremolo, filter sweeps, or step sequencer timing.

The node always outputs its ramp value as a normalised modulation signal. Optionally, when AddToSignal is enabled, the ramp value is also added to channel 0 of the audio buffer. Two update modes control how the phase is calculated: Continuous tracks the absolute DAW position, while Synced restarts on transport changes and note-on events. The Inactive parameter controls what value is output when the transport is stopped.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Tempo:
      desc: "Musical note value for the ramp period (e.g. 1/4, 1/8, 1/16)"
      range: "1/1 ... 1/128T"
      default: "1/4"
    Multiplier:
      desc: "Additional integer multiplier for the ramp period"
      range: "1 - 16"
      default: "1"
    AddToSignal:
      desc: "Whether to add the ramp value to audio channel 0"
      range: "No / Yes"
      default: "No"
    UpdateMode:
      desc: "Phase calculation method: Continuous tracks absolute position, Synced restarts on transport/note events"
      range: "Continuous / Synced"
      default: "Synced"
    Inactive:
      desc: "Output value when the transport is stopped"
      range: "Current / Zero / One"
      default: "Current"
---

```
// core.clock_ramp - tempo-synced ramp generator
// modulation out (always), optionally audio += ramp on ch0

process(input) {
    period = noteDuration(Tempo) * Multiplier
    phase = syncToTransport(UpdateMode, period)

    if (!transportPlaying)
        modulationOut = Inactive    // Current, Zero, or One
    else
        modulationOut = phase       // normalised 0..1

    if (AddToSignal)
        input[ch0] += phase
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Timing
    params:
      - { name: Tempo, desc: "Musical note value for the ramp period. Uses the standard HISE tempo values (1/1 through 1/128T)", range: "1/1 ... 1/128T", default: "1/4" }
      - { name: Multiplier, desc: "Additional integer multiplier applied to the tempo period for slower cycles", range: "1 - 16", default: "1" }
  - label: Output
    params:
      - { name: AddToSignal, desc: "When enabled, adds the ramp value to channel 0 of the audio signal", range: "No / Yes", default: "No" }
  - label: Configuration
    params:
      - { name: UpdateMode, desc: "Continuous tracks the absolute DAW position. Synced restarts the ramp on transport changes and note-on events", range: "Continuous / Synced", default: "Synced" }
      - { name: Inactive, desc: "Value sent via modulation output when the transport is stopped. Current holds the last ramp value, Zero outputs 0, One outputs 1", range: "Current / Zero / One", default: "Current" }
---
::

### Polyphonic behaviour

In polyphonic mode, note-on events resync the ramp to the current transport position, giving each voice a consistent phase relationship to the beat grid.

### Update modes

The Continuous update mode is useful when you need the ramp to reflect the absolute playback position (e.g. for a visual playhead). The Synced mode is better for musical effects where the ramp should restart cleanly on loop boundaries and transport jumps.

**See also:** $SN.core.ramp$ -- free-running ramp when tempo sync is not needed
