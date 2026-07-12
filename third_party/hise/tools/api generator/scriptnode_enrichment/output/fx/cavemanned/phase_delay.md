---
title: Phase Delay
description: "A first-order allpass filter that shifts the phase of the signal without changing its amplitude, intended for building comb filters."
factoryPath: fx.phase_delay
factory: fx
polyphonic: true
tags: [fx, filter, phase]
screenshot: /images/v2/reference/scriptnodes/fx/phase_delay.png
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "filters.allpass", type: alternative, reason: "Higher-order allpass filter with more parameters" }
  - { id: "PhaseFX", type: module, reason: "Scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)" }
commonMistakes:
  - title: "Not a comb filter on its own"
    wrong: "Using fx.phase_delay alone expecting comb filter output"
    right: "Place fx.phase_delay inside a container.split so its output is summed with the dry signal to produce comb filtering."
    explanation: "The node outputs only the phase-shifted signal. Comb filtering requires mixing this with the original signal, which a parallel container provides automatically."
llmRef: |
  fx.phase_delay

  First-order allpass filter that shifts the phase of the input signal without changing its amplitude. Designed as the building block for comb filter effects.

  Signal flow:
    audio in -> first-order allpass (phase shift 0 to -180 degrees) -> audio out

  CPU: negligible, polyphonic

  Parameters:
    Frequency (20 - 20000 Hz, skewed, default 400 Hz) - allpass corner frequency where phase shift is -90 degrees. Below this frequency the phase shift approaches 0; above it approaches -180 degrees.

  When to use:
    Building comb filters by placing inside a container.split (so the phase-shifted output sums with the dry signal). Also useful for phaser effects when multiple phase_delay stages are modulated together.

  Common mistakes:
    Does not produce comb filtering on its own - must be summed with the dry signal via a parallel container.

  See also:
    alternative filters.allpass - higher-order allpass filter
    [module] PhaseFX - scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)
---

First-order allpass. Shift phase, keep amplitude. Below corner freq: phase → 0. At corner: exactly -90°. Above corner: phase → -180°.

Processes up to two channels independently, each own state. Extra channels past two pass through. Building block for comb filter. Alone only shifts phase — for comb filter notches + peaks, put inside [container.split]($SN.container.split$) so phase-shifted output sums with dry. Frequency-dependent phase diff = constructive + destructive interference = comb. Modulate Frequency → sweep notches for phaser.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Frequency:
      desc: "Allpass corner frequency where phase shift equals -90 degrees"
      range: "20 - 20000 Hz"
      default: "400 Hz"
  functions:
    allpass:
      desc: "First-order allpass filter - preserves amplitude, shifts phase"
---

```
// fx.phase_delay - first-order allpass for comb filtering
// audio in -> audio out

process(input) {
    output = allpass(input, Frequency)
    // Phase shift: 0 deg at DC, -90 deg at Frequency, -180 deg at Nyquist
}
```

::

## Parameters

::parameter-table
---
groups:
  - label:
    params:
      - { name: Frequency, desc: "Corner frequency of the allpass filter. At this frequency the phase shift is exactly -90 degrees. The frequency scale is skewed so the knob centres around 1000 Hz.", range: "20 - 20000 Hz", default: "400 Hz" }
---
::

### Building a Phaser

Phaser: chain multiple phase_delay in series inside [container.split]($SN.container.split$). Modulate Frequency together. Stage count = notch count.

**See also:** $SN.filters.allpass$ -- higher-order allpass filter with additional parameters, $MODULES.PhaseFX$ -- scriptnode allpass phase-shift building block (PhaseFX uses cascaded allpass stages)
