---
title: Phasor
description: "A polyphonic ramp generator that outputs a naive 0-1 sawtooth on channel 0."
factoryPath: core.phasor
factory: core
polyphonic: true
tags: [core, oscillator, phasor, synthesis]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.phasor_fm", type: alternative, reason: "Adds FM modulation from the input signal" }
  - { id: "core.oscillator", type: alternative, reason: "Band-limited waveforms with multiple modes" }
commonMistakes:
  - title: "Phasor replaces channel 0"
    wrong: "Expecting the phasor to add its ramp to the existing signal like core.oscillator"
    right: "The phasor overwrites channel 0 with its ramp output. Place it at the start of a chain or after math.clear."
    explanation: "Unlike core.oscillator which adds to the signal, core.phasor replaces whatever is on channel 0. Other channels are left untouched."
llmRef: |
  core.phasor

  Generates a naive 0-1 ramp that replaces channel 0 of the input signal. Responds to MIDI note-on for pitch tracking. Intended as a building block for waveshaping and table lookup synthesis.

  Signal flow:
    channel 0 overwritten with ramp(Frequency * Freq Ratio)

  CPU: negligible, polyphonic

  Parameters:
    Gate (Off/On, default On): Enables output; rising edge resets phase
    Frequency (20-20000 Hz, default 220): Ramp frequency, overridden by MIDI note-on
    Freq Ratio (1-16, default 1): Integer pitch multiplier
    Phase (0-100%, default 0%): Phase offset

  When to use:
    - As the starting point for waveshaping or table lookup synthesis
    - When you need a raw 0-1 ramp at audio rate
    - Use core.oscillator instead when you need finished waveforms with anti-aliasing

  See also:
    alternative core.phasor_fm -- adds FM modulation from input
    alternative core.oscillator -- band-limited waveforms
---

The phasor generates a linear ramp from 0 to 1 at the specified frequency and writes it directly to channel 0 of the audio signal. It is intended as a building block for custom synthesis: feed the ramp output into a waveshaper or table lookup to produce arbitrary waveform shapes.

The ramp is naive (no anti-aliasing), so it will produce aliasing at high frequencies. This is acceptable when the output is used as an index for further processing rather than listened to directly. Only channel 0 is written; other channels pass through unmodified. The output range is strictly 0 to just below 1.0, with a hard discontinuity at the wrap point. Each voice maintains its own phase accumulator, and MIDI note-on messages set the frequency.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Gate:
      desc: "Enables or disables the ramp; rising edge resets phase"
      range: "Off / On"
      default: "On"
    Frequency:
      desc: "Ramp frequency in Hz (overridden by MIDI note-on)"
      range: "20 - 20000 Hz"
      default: "220"
    Freq Ratio:
      desc: "Integer multiplier applied to the base frequency"
      range: "1 - 16"
      default: "1"
    Phase:
      desc: "Phase offset for the ramp start position"
      range: "0 - 100%"
      default: "0%"
---

```
// core.phasor - naive ramp generator
// replaces channel 0

process(input) {
    if (!Gate) return input

    phase += (Frequency * Freq Ratio) / sampleRate
    phase = wrap(phase) + Phase
    input[ch0] = phase    // replaces channel 0
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Gate, desc: "Enables or disables the ramp output. A rising edge resets the phase to zero", range: "Off / On", default: "On" }
      - { name: Frequency, desc: "Base ramp frequency. Overridden by MIDI note-on in a polyphonic context", range: "20 - 20000 Hz", default: "220" }
      - { name: "Freq Ratio", desc: "Integer multiplier applied to the base frequency", range: "1 - 16", default: "1" }
      - { name: Phase, desc: "Phase offset for the ramp start position", range: "0 - 100%", default: "0%" }
---
::

**See also:** $SN.core.phasor_fm$ -- adds FM modulation from the input signal, $SN.core.oscillator$ -- band-limited waveforms with multiple modes
