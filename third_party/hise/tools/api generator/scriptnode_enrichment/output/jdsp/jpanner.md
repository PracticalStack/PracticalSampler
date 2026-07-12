---
title: Panner
description: "A stereo panner with seven selectable panning laws."
factoryPath: jdsp.jpanner
factory: jdsp
polyphonic: true
tags: [jdsp, panning, stereo]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "StereoFX", type: module, reason: "Stereo panning with multiple pan laws" }
commonMistakes: []
llmRef: |
  jdsp.jpanner

  A stereo panner with seven selectable panning laws. Adjusts the left/right channel balance based on the Pan position and the chosen Rule. Polyphonic -- each voice can have an independent pan position.

  Signal flow:
    audio in -> apply panning gain per channel -> audio out

  CPU: low, polyphonic

  Parameters:
    Pan (-1.0 to 1.0, default 0.0) - stereo position: -1 = full left, 0 = centre, 1 = full right
    Rule (0-6 enum, default 1 = Balanced) - panning law:
      0 = Linear (-6 dB centre)
      1 = Balanced (0 dB, both channels at unity when centred)
      2 = Sine 3 dB (-3 dB constant power)
      3 = Sine 4.5 dB (-4.5 dB compromise)
      4 = Sine 6 dB (-6 dB sine variant)
      5 = Sqrt 3 dB (-3 dB square root constant power)
      6 = Sqrt 4.5 dB (-4.5 dB square root compromise)

  When to use:
    Stereo positioning of audio signals. Use Balanced for simple L/R panning. Use constant-power rules (Sine3dB, Sqrt3dB) when the panned signal must maintain perceived loudness across the stereo field.

  See also:
    [module] StereoFX -- module-tree stereo panner with mid/side width control
---

A stereo panner that adjusts the left and right channel gains based on the Pan position and a selectable panning Rule. The panning rule determines how the gain is distributed across the stereo field, affecting the perceived loudness at different pan positions.

Each voice maintains its own pan position, so the node can be used in polyphonic contexts where voices are spread across the stereo field. The default panning rule is Balanced, which keeps both channels at unity gain when centred and smoothly fades one channel to zero as the pan moves to the opposite side.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Pan:
      desc: "Stereo position"
      range: "-1.0 (left) - 1.0 (right)"
      default: "0.0"
    Rule:
      desc: "Panning law that determines gain distribution"
      range: "Linear / Balanced / Sine3dB / Sine4.5dB / Sine6dB / Sqrt3dB / Sqrt4p5dB"
      default: "Balanced"
  functions:
    panGain:
      desc: "Calculates left and right gain coefficients based on Pan position and Rule"
---

```
// jdsp.jpanner - stereo panning
// audio in -> audio out (per voice)

process(left, right) {
    gainL, gainR = panGain(Pan, Rule)
    left  *= gainL
    right *= gainR
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Panning
    params:
      - { name: Pan, desc: "The position in the stereo field. -1.0 is fully left, 0.0 is centre, 1.0 is fully right.", range: "-1.0 - 1.0", default: "0.0" }
      - { name: Rule, desc: "The panning law used to calculate channel gains. See the table below for details.", range: "Linear / Balanced / Sine3dB / Sine4.5dB / Sine6dB / Sqrt3dB / Sqrt4p5dB", default: "Balanced" }
---
::

### Panning Rules

| Rule | Centre Attenuation | Description |
|------|-------------------|-------------|
| Linear | -6 dB | Standard 6 dB panning. Sums to constant level in mono. |
| Balanced | 0 dB | Both channels at unity when centred; one fades to zero. |
| Sine 3 dB | -3 dB | Constant power panning using a sine curve. Maintains perceived loudness across the stereo field. |
| Sine 4.5 dB | -4.5 dB | Compromise between 3 dB and 6 dB using a sine curve. |
| Sine 6 dB | -6 dB | 6 dB panning using a sine curve. |
| Sqrt 3 dB | -3 dB | Constant power panning using a square root curve. |
| Sqrt 4.5 dB | -4.5 dB | Compromise between 3 dB and 6 dB using a square root curve. |

> [!Tip:Choose the right panning law] Use **Balanced** for simple L/R placement where both channels should be at full volume when centred. Use **Sine 3 dB** or **Sqrt 3 dB** (constant power) when the panned signal must maintain consistent perceived loudness regardless of position - this is the standard choice for music mixing.

**See also:** $MODULES.StereoFX$ -- module-tree stereo panner with mid/side width control
