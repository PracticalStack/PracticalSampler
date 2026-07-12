---
title: FM
description: "An FM operator that reads the input signal as a modulator and outputs a sine carrier."
factoryPath: core.fm
factory: core
polyphonic: true
tags: [core, oscillator, fm, synthesis]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
forumReferences:
  - { tid: 14257, reason: "FM operator signal flow and feedback pattern" }
seeAlso:
  - { id: "core.phasor_fm", type: companion, reason: "FM ramp carrier for custom waveforms via waveshaping" }
  - { id: "core.oscillator", type: alternative, reason: "Standalone oscillator with multiple waveforms but no FM input" }
commonMistakes:
  - title: "FM source must precede fm node"
    wrong: "Placing the modulator signal after core.fm in the chain"
    right: "The modulator must come before core.fm because it reads channel 0 as the FM source and then replaces it with the sine carrier output."
    explanation: "The node reads the input as modulation, then overwrites it. Place another oscillator or fm node before this one to build FM stacks."
  - title: "Carrier is always sine"
    wrong: "Expecting to change the carrier waveform with a Mode parameter"
    right: "The carrier is always a sine wave. For custom carrier waveforms, use core.phasor_fm with a waveshaper."
    explanation: "This node has no waveform selection. It produces a sine carrier only. Use core.phasor_fm plus a table or waveshaper for other carrier shapes."
  - title: "Output is mono -- stereo requires container.multi"
    wrong: "Expecting stereo output from a single core.fm node in a stereo network"
    right: "Place two core.fm instances inside a container.multi to produce independent left and right channels."
    explanation: "The fm node processes a single channel. In a stereo network it only writes to channel 0. Use container.multi to split processing per channel."
llmRef: |
  core.fm

  FM synthesis operator. Reads the input signal on channel 0 as a frequency modulator and replaces it with a sine carrier. Polyphonic with MIDI pitch tracking. Stack multiple fm nodes to build complex FM algorithms.

  Signal flow:
    read channel 0 as modulator -> sine carrier with phase modulation -> overwrite channel 0

  CPU: low, polyphonic

  Parameters:
    Frequency (20-20000 Hz, default 20): Carrier frequency, overridden by MIDI note-on
    Modulator (0.0-1.0, default 0.0): FM modulation depth
    FreqMultiplier (1-12, default 1): Integer carrier frequency multiplier
    Gate (Off/On, default On): Enables output; off resets phase

  When to use:
    - Building FM synthesisers by stacking operators
    - Classic Chowning-style FM patches
    - Use core.phasor_fm when you need a non-sine carrier

  Common mistakes:
    - Modulator must be placed before this node in the chain
    - Carrier is always sine - no waveform selection
    - Mono output -- use container.multi for stereo

  Forum references: tid:14257 (FM feedback pattern with frame_block)

  See also:
    companion core.phasor_fm -- FM ramp for custom waveforms
    alternative core.oscillator -- standalone multi-waveform oscillator
---

The fm node is an FM synthesis operator. It reads the audio signal on channel 0 as a frequency modulator, applies it to a sine carrier via phase modulation, and replaces channel 0 with the carrier output. Stack multiple fm nodes in sequence to build classic FM synthesis algorithms - each operator reads the previous one's output as its modulator.

The node is polyphonic and responds to MIDI note-on for pitch tracking. The Modulator parameter controls the modulation depth (FM index), and FreqMultiplier sets the carrier's harmonic ratio relative to the base frequency.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Carrier base frequency in Hz (overridden by MIDI note-on)"
      range: "20 - 20000 Hz"
      default: "20"
    Modulator:
      desc: "FM modulation depth controlling how much the input affects the carrier phase"
      range: "0.0 - 1.0"
      default: "0.0"
    FreqMultiplier:
      desc: "Integer multiplier for the carrier frequency (harmonic ratio)"
      range: "1 - 12"
      default: "1"
    Gate:
      desc: "Enables or disables output; off resets the carrier phase"
      range: "Off / On"
      default: "On"
  functions:
    sineLookup:
      desc: "Retrieves a sine value from a lookup table at the current phase position"
---

```
// core.fm - FM synthesis operator
// reads channel 0 as modulator, outputs sine carrier

process(input) {
    if (!Gate) return silence

    modSignal = input[ch0]
    carrier = sineLookup(phase)
    phase += (Frequency * FreqMultiplier) / sampleRate
    phase += modSignal * Modulator
    input[ch0] = carrier    // replaces channel 0
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Carrier
    params:
      - { name: Frequency, desc: "Carrier base frequency. Overridden by MIDI note-on in a polyphonic context", range: "20 - 20000 Hz", default: "20" }
      - { name: FreqMultiplier, desc: "Integer multiplier for the carrier frequency, setting the harmonic ratio", range: "1 - 12", default: "1" }
  - label: Modulation
    params:
      - { name: Modulator, desc: "FM modulation depth. Higher values produce more complex spectra", range: "0.0 - 1.0", default: "0.0" }
  - label: Control
    params:
      - { name: Gate, desc: "Enables or disables the carrier. Turning off resets the phase to zero", range: "Off / On", default: "On" }
---
::

### Modulator parameter

The Modulator parameter is not the modulation source itself -- it is a gain multiplier applied to the incoming audio signal before FM processing. The actual modulation source is whatever audio signal arrives on channel 0 of the input. A typical 2-operator chain is: `oscillator -> ahdsr -> fm -> ahdsr`. For 3+ operators, chain additional fm nodes.

### FM feedback

To create FM feedback (a carrier modulating itself), route the output signal back to the input using a feedback path and wrap the chain in a `container.framex_block`. Without the frame container, the feedback delay equals the full buffer size. With it, the delay is reduced to a single sample. Compile the network for acceptable CPU performance when using feedback.

### Stereo output

The fm node processes a single channel. To produce stereo FM synthesis, place two fm nodes inside a [container.multi]($SN.container.multi$) which routes each instance to a separate channel.

**See also:** $SN.core.phasor_fm$ -- FM ramp carrier for custom waveforms via waveshaping, $SN.core.oscillator$ -- standalone multi-waveform oscillator
