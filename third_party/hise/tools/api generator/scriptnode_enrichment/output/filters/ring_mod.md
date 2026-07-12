---
title: Ring Mod
description: "A ring modulator that multiplies the input signal by an internal sine oscillator."
factoryPath: filters.ring_mod
factory: filters
screenshot: /images/v2/reference/scriptnodes/filters/ring_mod.png
polyphonic: true
tags: [filters, ring-mod, ring-modulation, effect, sine]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso: []
commonMistakes:
  - title: "Q controls depth, not resonance"
    wrong: "Expecting Q to control filter resonance as in other filter nodes"
    right: "Q controls modulation depth (wet/dry mix), not resonance."
    explanation: "In this node, Q is repurposed as a wet/dry control. At minimum Q, the output is fully dry. At maximum Q, the output is fully ring-modulated."
llmRef: |
  filters.ring_mod

  Ring modulator that multiplies the input by an internal sine oscillator. Q controls wet/dry mix, not resonance. Placed in the filters factory for architectural reasons but is functionally an effect.

  Signal flow:
    audio in -> multiply by sine oscillator -> wet/dry mix -> audio out

  CPU: low, polyphonic

  Parameters:
    Frequency: 20 - 20000 Hz (default 1000). Internal sine oscillator frequency.
    Q: 0.3 - 9.9 (default 1.0). Modulation depth (0 = dry, max = full ring mod).
    Gain: -18 - 18 dB (default 0). Ignored.
    Smoothing: 0 - 1 s (default 0.01). Parameter interpolation time.
    Mode: RingMod only (default RingMod).
    Enabled: Off / On (default On). Hard bypass.

  When to use:
    Ring modulation effects. Creates inharmonic sum and difference frequencies. Minimum oscillator frequency is 20 Hz, so true tremolo is not achievable.

  Common mistakes:
    Q controls wet/dry mix, not resonance.

  See also: (none)
---

A ring modulator that multiplies the input signal by an internal sine oscillator. This produces inharmonic sum and difference frequencies characteristic of ring modulation. Despite being in the filters factory, this is an effect node - it shares the standard filter parameter interface but repurposes Frequency for the oscillator and Q for modulation depth.

![Ring Mod screenshot](/images/v2/reference/scriptnodes/filters/ring_mod.png)

At minimum Q (0.3), the output is the unmodified dry signal. At maximum Q (9.9), the output is fully ring-modulated. Intermediate values crossfade between dry and modulated signal.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Sine oscillator frequency"
      range: "20 - 20000 Hz"
      default: "1000"
    Q:
      desc: "Modulation depth / wet-dry mix"
      range: "0.3 - 9.9"
      default: "1.0"
  functions:
    sineOsc:
      desc: "Internal sine oscillator at the set frequency"
---

```
// filters.ring_mod - ring modulator
// audio in -> audio out

process(input) {
    modSignal = sineOsc(Frequency)
    depth = scaleToDepth(Q)    // 0.0 at min Q, 1.0 at max Q

    output = input * (1.0 - depth) + input * modSignal * depth
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Frequency, desc: "Frequency of the internal sine oscillator. Sets the modulation frequency.", range: "20 - 20000 Hz", default: "1000" }
      - { name: Q, desc: "Modulation depth. Controls the wet/dry mix between the original signal and the ring-modulated signal. Minimum = fully dry, maximum = fully modulated.", range: "0.3 - 9.9", default: "1.0" }
      - { name: Gain, desc: "Not used. Present for interface consistency.", range: "-18 - 18 dB", default: "0.0" }
  - label: Configuration
    params:
      - { name: Mode, desc: "Fixed to RingMod. Only one mode is available.", range: "RingMod", default: "RingMod" }
      - { name: Smoothing, desc: "Interpolation time for parameter changes.", range: "0.0 - 1.0 s", default: "0.01" }
      - { name: Enabled, desc: "Hard bypass.", range: "Off / On", default: "On" }
---
::

### Oscillator Behaviour

The oscillator phase is shared across all channels but is not reset on note-on. In a polyphonic context, different voices may have different modulation phases depending on when they started.

### Limitations

The minimum oscillator frequency is 20 Hz. For tremolo effects at lower rates, use a [control.pma]($SN.control.pma$) with an external LFO modulating a [core.gain]($SN.core.gain$) node instead.
