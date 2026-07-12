---
title: Bipolar
description: "Creates a bipolar modulation signal centred around the midpoint from a normalised 0-1 input."
factoryPath: control.bipolar
factory: control
polyphonic: true
tags: [control, bipolar, modulation, centre, scaling]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: [{ parameter: "Gamma", impact: "minimal", note: "Additional power function when Gamma is not 1.0" }]
seeAlso:
  - { id: "control.pma", type: alternative, reason: "Linear scaling and offset without centring around midpoint" }
  - { id: "control.intensity", type: alternative, reason: "Scales modulation depth from the top (1.0) rather than from the centre" }
commonMistakes:
  - title: "Scale of 0 outputs constant 0.5"
    wrong: "Leaving Scale at its default of 0 and expecting the input to pass through"
    right: "Set Scale to 1.0 for the output to follow the input, or to -1.0 to invert it around the centre."
    explanation: "When Scale is 0, the output is always 0.5 regardless of the input value. The Scale parameter controls how far the output deviates from the midpoint."
llmRef: |
  control.bipolar

  Creates a bipolar modulation signal from a normalised 0-1 input. Centres the value around 0.5, applies an optional gamma curve, and scales the deviation by Scale.

  Signal flow:
    Control node - no audio processing
    Value (0..1) -> centre around 0 -> gamma curve -> scale by Scale -> re-centre around 0.5 -> modulation out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): normalised input
    Scale (-1.0 - 1.0, default 0.0): how far the output deviates from 0.5
    Gamma (0.5 - 2.0, default 1.0): non-linear curve shaping (1.0 = linear)

  When to use:
    Use when modulation should swing symmetrically around a centre value. For example, modulating pitch detune, stereo width, or filter cutoff in both directions from a neutral point.

  Common mistakes:
    Scale defaults to 0, which produces constant 0.5 output.

  See also:
    [alternative] control.pma -- linear scaling without centring
    [alternative] control.intensity -- scales modulation depth from the top
---

Creates a bipolar modulation signal from a normalised 0-1 input by centring the value around the midpoint (0.5), applying an optional non-linear gamma curve, and scaling the deviation. This is useful whenever modulation should swing symmetrically in both directions from a neutral centre, such as pitch detune, stereo pan, or filter cutoff modulation.

The Scale parameter controls the depth and direction of the deviation from 0.5. A Scale of 1.0 means the full input range is preserved; a Scale of -1.0 inverts the modulation. The Gamma parameter applies a power curve to the deviation, allowing finer control near the centre (Gamma > 1.0) or near the extremes (Gamma < 1.0).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised input signal"
      range: "0.0 - 1.0"
      default: "0.0"
    Scale:
      desc: "Bipolar scaling depth and direction"
      range: "-1.0 - 1.0"
      default: "0.0"
    Gamma:
      desc: "Non-linear curve applied to the deviation from centre"
      range: "0.5 - 2.0"
      default: "1.0"
  functions:
    bipolarTransform:
      desc: "Centres value around 0, applies gamma curve, scales, and re-centres around 0.5"
---

```
// control.bipolar - bipolar modulation from 0..1 input
// control in -> control out

onValueChange(input) {
    v = Value - 0.5
    if (Gamma != 1.0)
        v = pow(abs(v * 2.0), Gamma) * sign(v) * 0.5
    output = v * Scale + 0.5
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Normalised input signal. A value of 0.5 produces no deviation from centre regardless of Scale.", range: "0.0 - 1.0", default: "0.0" }
  - label: Shaping
    params:
      - { name: Scale, desc: "Controls how far the output deviates from 0.5. Positive values preserve the input direction; negative values invert it. At 0, the output is always 0.5.", range: "-1.0 - 1.0", default: "0.0" }
      - { name: Gamma, desc: "Applies a power curve to the deviation from centre. Values above 1.0 pull the response toward the centre (finer control near the midpoint). Values below 1.0 push it toward the extremes.", range: "0.5 - 2.0", default: "1.0" }
---
::

The Gamma curve is applied symmetrically around the centre point. It operates on the absolute deviation, preserving the sign, so the positive and negative halves of the modulation are shaped identically.

**See also:** $SN.control.pma$ -- linear scaling and offset without centring, $SN.control.intensity$ -- scales modulation depth from the top
