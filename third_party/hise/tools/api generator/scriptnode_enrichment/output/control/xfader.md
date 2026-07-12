---
title: Crossfader
description: "Distributes fade coefficients across multiple outputs based on a single crossfade position and a selectable fade curve."
factoryPath: control.xfader
factory: control
polyphonic: false
tags: [control, crossfade, mix, routing]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.blend", type: disambiguation, reason: "Interpolates between two values rather than distributing fade coefficients" }
  - { id: "control.branch_cable", type: alternative, reason: "Routes to a single output instead of distributing across all" }
commonMistakes:
  - title: "Harmonics mode can exceed 0..1 range"
    wrong: "Assuming all fade modes produce values between 0 and 1"
    right: "In Harmonics mode, outputs can exceed 1.0. Use a different mode or add clamping if the target requires 0..1."
    explanation: "Most fade modes (Linear, RMS, Cosine, etc.) keep outputs within 0..1, but Harmonics mode multiplies the input by the output index, which can produce values above 1.0 for higher indices."
llmRef: |
  control.xfader

  Distributes fade coefficients across multiple outputs based on a single normalised crossfade position. The Mode property selects the fade curve shape.

  Signal flow:
    Control node -- no audio processing
    Value (0..1) -> compute fade coefficients per mode -> send coefficient to each output slot

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Crossfade position.

  Properties:
    NumParameters: Number of output slots (default 2, up to 9).
    Mode: Fade curve -- Switch, Linear, Overlap, Squared, RMS, Cosine, CosineHalf, Harmonics, Threshold.

  When to use:
    Parallel processing with mix control, dry/wet blending, or multi-band crossfading. 40 instances across surveyed projects (rank 7). Commonly paired with container.split for parallel signal chains.

  Common mistakes:
    Harmonics mode can produce values above 1.0 for higher output indices.

  See also:
    [disambiguation] control.blend -- interpolates between two values
    [alternative] control.branch_cable -- routes to a single output
---

Distributes fade coefficients across multiple output slots based on a single normalised crossfade position. Each output receives a coefficient determined by the selected fade curve mode, allowing smooth crossfading between parallel signal chains. With two outputs in Linear mode, output 1 fades from 1 to 0 while output 2 fades from 0 to 1 as the Value moves from 0 to 1.

The Mode property selects the fade curve shape. The number of outputs is configured via the NumParameters property (default 2, up to 9). This node appears in 40 surveyed networks (rank 7), most commonly paired with [container.split]($SN.container.split$) for parallel processing with mix control.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised crossfade position"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    computeFade:
      desc: "Calculates the fade coefficient for each output using the selected curve mode"
---

```
// control.xfader - multi-output crossfade distributor
// control in -> N fade coefficients out

onValueChange(Value) {
    for each output slot:
        coefficient = computeFade(Value, slotIndex, numOutputs)
        sendToSlot(slotIndex, coefficient)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Crossfade position. At 0.0 the first output is fully active; at 1.0 the last output is fully active.", range: "0.0 - 1.0", default: "0.0" }
---
::

## Fade Modes

The Mode property determines the shape of the fade curve applied to each output:

| Mode | Behaviour |
|------|-----------|
| Switch | Hard switch -- exactly one output is 1.0, all others are 0.0 |
| Linear | Triangular crossfade with linear overlap between adjacent outputs |
| Overlap | Custom overlapping crossfade formula |
| Squared | Linear fade values squared (steeper curve) |
| RMS | Square root of linear fade (equal-power crossfade) |
| Cosine | S-curve (sine-based) crossfade |
| CosineHalf | Half-cosine curve crossfade |
| Harmonics | Each output receives `Value * (index + 1)` -- can exceed 1.0 |
| Threshold | Step function -- output is 1.0 if Value reaches its threshold, else 0.0 |

For most mixing applications, **RMS** provides perceptually even crossfading. **Linear** is the simplest and works well for control signals. **Switch** is useful when only one path should be active at a time.

The xfader is monophonic -- it does not maintain per-voice state. All voices share the same crossfade position and coefficients.

**See also:** $SN.control.blend$ -- interpolates between two values, $SN.control.branch_cable$ -- routes to a single output
