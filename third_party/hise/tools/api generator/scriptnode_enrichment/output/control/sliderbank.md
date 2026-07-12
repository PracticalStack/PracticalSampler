---
title: Slider Bank
description: "Scales an input value by a slider pack and distributes the results to multiple output targets."
factoryPath: control.sliderbank
factory: control
polyphonic: false
tags: [control, multi-output, slider-pack]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.clone_pack", type: companion, reason: "Drives clone containers with per-element values" }
  - { id: "control.normaliser", type: alternative, reason: "Single-output passthrough without per-element scaling" }
commonMistakes:
  - title: "Exceeding the eight output limit"
    wrong: "Setting NumParameters above 8 and expecting all outputs to work"
    right: "Keep NumParameters at 8 or below"
    explanation: "The node supports a maximum of 8 output connections. Outputs beyond index 7 are silently ignored."
llmRef: |
  control.sliderbank

  Multiplies an input value by each element of a slider pack and sends the scaled results to up to 8 separate output targets.

  Signal flow:
    Control node - no audio processing
    Value * sliderPack[i] -> output[i] for each connected target

  CPU: negligible, monophonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): Input value multiplied by each slider pack element

  Properties:
    NumParameters: Number of output slots (max 8)

  When to use:
    Use when a single control value needs to drive multiple parameters at different intensities, such as distributing a macro knob to several targets with individual scaling.

  See also:
    [companion] control.clone_pack -- drives clone containers with per-element values
    [alternative] control.normaliser -- single-output passthrough without per-element scaling
---

Slider Bank multiplies a single input value by each element of a slider pack and sends the result to a separate output for each element. This makes it straightforward to distribute one control signal to multiple targets, each with its own scaling factor defined by the corresponding slider in the pack.

The node supports up to 8 outputs. When an individual slider in the pack changes, only the corresponding output is updated rather than recalculating all outputs.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Input value multiplied by each slider pack element"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    scale:
      desc: "Multiplies the input value by the slider pack element at each index"
---

```
// control.sliderbank - per-element scaling to multiple outputs
// control in -> multiple control out

onValueChange(input) {
    for each output i:
        output[i] = Value * sliderPack[i]
}

onSliderChange(index) {
    output[index] = Value * sliderPack[index]
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: ""
    params:
      - { name: Value, desc: "Input value multiplied by each slider pack element to produce per-output results.", range: "0.0 - 1.0", default: "0.0" }
---
::

The NumParameters property controls how many output slots are available. The slider pack is automatically resized to match this count. Each output can be connected to a different target parameter, allowing a single knob to control multiple destinations with individual scaling.

**See also:** $SN.control.clone_pack$ -- drives clone containers with per-element values, $SN.control.normaliser$ -- single-output passthrough without per-element scaling
