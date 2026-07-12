---
title: SNEX Oscillator
description: "A custom oscillator node using SNEX code with built-in frequency tracking and polyphonic voice management."
factoryPath: core.snex_osc
factory: core
polyphonic: true
tags: [core, snex, oscillator, synthesis]
cpuProfile:
  baseline: variable
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.oscillator", type: alternative, reason: "Built-in oscillator with standard waveforms" }
  - { id: "core.snex_node", type: alternative, reason: "Full SNEX callback set for non-oscillator DSP" }
commonMistakes:
  - title: "Forgetting that output is additive"
    wrong: "Expecting the oscillator to replace the existing signal"
    right: "The oscillator output is added to channel 0. Use math.clear before the node if you need a clean signal."
    explanation: "core.snex_osc adds its output to the existing signal on channel 0. If the input already contains audio, the oscillator mixes with it rather than replacing it."
  - title: "Setting Frequency after MIDI note-on"
    wrong: "Relying on the Frequency parameter for pitch while also receiving MIDI"
    right: "MIDI note-on events override the Frequency parameter. Use PitchMultiplier for harmonic offsets."
    explanation: "Each MIDI note-on sets the oscillator frequency to the note's frequency, overriding any manual Frequency value. The last source to set frequency wins."
llmRef: |
  core.snex_osc

  A polyphonic oscillator node where the waveform generation is defined by user-written SNEX code. The wrapper handles frequency tracking, phase accumulation, MIDI note-on integration, and per-voice state. Output is additive on channel 0.

  Signal flow:
    MIDI note-on -> frequency tracking -> SNEX tick/process -> ch0 += oscillator output

  CPU: variable (depends on user SNEX code), polyphonic

  Parameters:
    Frequency (20 - 20000 Hz, default 220): Base frequency, overridden by MIDI note-on.
    PitchMultiplier (1 - 16, default 1): Integer frequency multiplier.
    Additional parameters defined by user SNEX code (index offset +2).

  When to use:
    - Custom oscillator waveforms not available from core.oscillator
    - Polyphonic synthesis with per-voice state
    - Additive oscillator layering (output adds to existing signal)

  See also:
    [alternative] core.oscillator -- built-in oscillator with standard waveforms
    [alternative] core.snex_node -- full SNEX callback set for non-oscillator DSP
---

The SNEX oscillator provides a framework for writing custom oscillator waveforms in [SNEX]($LANG.snex$) while the node handles frequency tracking, phase accumulation, MIDI integration, and polyphonic voice management. Each voice maintains its own phase state independently.

The SNEX code defines two callbacks: `tick` (generates one sample at a given phase position) and `process` (generates a block of samples). The node manages the phase counter and advances it based on the current frequency and sample rate. MIDI note-on events set the oscillator frequency directly -- overriding the Frequency parameter. The oscillator output is additive: it is added to the existing signal on channel 0 rather than replacing it. The oscillator is fully polyphonic -- each voice tracks its own phase independently. When compiled to C++, the SNEX oscillator class receives the voice count as a template argument, enabling per-voice custom state beyond the built-in phase tracking.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Base oscillator frequency (overridden by MIDI note-on)"
      range: "20 - 20000 Hz"
      default: "220"
    PitchMultiplier:
      desc: "Integer frequency multiplier applied to the base frequency"
      range: "1 - 16"
      default: "1"
  functions:
    snexTick:
      desc: "User-defined SNEX callback that generates one sample at the given phase"
    advancePhase:
      desc: "Increments the internal phase counter based on frequency and sample rate"
---

```
// core.snex_osc - custom oscillator via SNEX
// MIDI + params in -> audio out (additive on ch0)

process(input) {
    freq = Frequency * PitchMultiplier
    // MIDI note-on overrides Frequency

    for each sample {
        input[ch0] += snexTick(phase)
        advancePhase(freq)
    }
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Pitch
    params:
      - { name: Frequency, desc: "Base oscillator frequency. Overridden by MIDI note-on events.", range: "20 - 20000 Hz", default: "220" }
      - { name: PitchMultiplier, desc: "Integer multiplier applied to the frequency.", range: "1 - 16", default: "1" }
---
::

Any additional parameters defined in the SNEX code are exposed starting from parameter index 2 onwards.

**See also:** $SN.core.oscillator$ -- built-in oscillator with standard waveforms, $SN.core.snex_node$ -- full SNEX callback set for non-oscillator DSP
