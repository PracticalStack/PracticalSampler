---
title: Phasor FM
description: "A polyphonic ramp generator that uses the input signal as frequency modulation."
factoryPath: core.phasor_fm
factory: core
polyphonic: true
tags: [core, oscillator, phasor, fm, synthesis]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "core.phasor", type: disambiguation, reason: "Same ramp generator without FM input" }
  - { id: "core.fm", type: companion, reason: "FM sine operator that can be stacked with phasor_fm" }
commonMistakes:
  - title: "FM source must precede phasor_fm"
    wrong: "Placing the FM source after phasor_fm in the signal chain"
    right: "The FM modulator must be placed before phasor_fm because it reads channel 0 as its modulation input before overwriting it with the ramp output."
    explanation: "The node reads channel 0 as the FM signal, then replaces it with the ramp. If the modulator comes after, it never reaches the phasor_fm input."
llmRef: |
  core.phasor_fm

  A ramp generator identical to core.phasor but reads the input signal on channel 0 as frequency modulation before overwriting it with the ramp output. The input scales the phase increment per sample.

  Signal flow:
    read channel 0 as FM -> generate ramp -> overwrite channel 0

  CPU: negligible, polyphonic

  Parameters:
    Gate (Off/On, default On): Enables output; rising edge resets phase
    Frequency (20-20000 Hz, default 220): Base ramp frequency, overridden by MIDI note-on
    Freq Ratio (1-16, default 1): Integer pitch multiplier
    Phase (0-100%, default 0%): Phase offset

  When to use:
    - Building FM synthesis patches where you need a ramp (not sine) as the carrier
    - Custom waveshaping with frequency modulation
    - Use core.fm instead when you want a sine carrier with FM

  Common mistakes:
    - FM source must be placed before phasor_fm in the chain

  See also:
    disambiguation core.phasor -- same ramp without FM
    companion core.fm -- FM sine operator
---

The phasor_fm node works like [core.phasor]($SN.core.phasor$) but reads the existing signal on channel 0 as a frequency modulator before replacing it with the ramp output. The input value scales the phase increment per sample: a value of 1.0 means normal speed, 2.0 doubles the rate, and 0.0 freezes the phase. Negative values reverse the ramp direction.

This node is the building block for FM synthesis patches where you need a ramp carrier rather than a sine. Feed the ramp output into a waveshaper or table lookup to create the final carrier waveform.

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
      desc: "Base ramp frequency in Hz (overridden by MIDI note-on)"
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
// core.phasor_fm - ramp generator with FM input
// reads channel 0 as modulator, replaces with ramp

process(input) {
    if (!Gate) return input

    fmSignal = input[ch0]
    phase += (Frequency * Freq Ratio) / sampleRate * fmSignal
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

### Limitations

With extreme modulation values the phase can advance multiple cycles per sample, producing erratic jumps in the output even though all values remain in the 0-1 range.

**See also:** $SN.core.phasor$ -- same ramp generator without FM input, $SN.core.fm$ -- FM sine operator that can be stacked with this node
