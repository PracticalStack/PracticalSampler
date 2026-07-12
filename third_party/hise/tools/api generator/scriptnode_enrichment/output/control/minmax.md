---
title: MinMax
description: "Maps a normalised 0-1 input to a custom output range with configurable skew, step quantisation, and polarity inversion."
factoryPath: control.minmax
factory: control
polyphonic: true
tags: [control, minmax, range, mapping, scaling, quantisation]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.normaliser", type: companion, reason: "The inverse operation -- converts an unscaled value back to normalised 0-1" }
  - { id: "control.pma_unscaled", type: alternative, reason: "Simpler linear scaling without range mapping or skew" }
commonMistakes:
  - title: "Output is unscaled"
    wrong: "Connecting the modulation output to a target and expecting the target's range to be applied on top"
    right: "The output bypasses the target's range conversion. Set Minimum and Maximum to match the exact values you want the target to receive."
    explanation: "Because the output is unnormalised, the connection system does not apply the target parameter's range. The values from Minimum to Maximum are sent directly to the target."
llmRef: |
  control.minmax

  Maps a normalised 0-1 input to a configurable output range. Applies skew for non-linear mapping, optional step quantisation, and polarity inversion. Output is unnormalised.

  Signal flow:
    Control node - no audio processing
    Value (0..1) -> invert if Polarity=Inverted -> range map [Minimum, Maximum] with Skew -> snap to Step -> modulation out (unnormalised)

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): normalised input
    Minimum (0.0 - 1.0, default 0.0): output range lower bound
    Maximum (0.0 - 1.0, default 1.0): output range upper bound
    Skew (0.1 - 10.0, default 1.0): non-linear curve factor (1.0 = linear)
    Step (0.0 - 1.0, default 0.0): quantisation interval (0 = continuous)
    Polarity (Normal / Inverted, default Normal): inverts input before mapping

  When to use:
    Use when you need to dynamically change the range of a modulation signal at runtime. Unlike static target ranges, the Minimum and Maximum parameters can be modulated. Also useful as a normalised-to-unscaled converter.

  See also:
    [companion] control.normaliser -- inverse operation, unscaled to normalised
    [alternative] control.pma_unscaled -- simpler linear scaling
---

Maps a normalised 0-1 input value to a custom output range defined by Minimum and Maximum, with optional non-linear curve shaping via Skew and discrete step quantisation. The output is unnormalised, meaning it bypasses the target parameter's own range conversion and delivers the mapped value directly.

This node is useful when you need a modulation range that can change at runtime. While scriptnode parameters normally have static target ranges, the Minimum and Maximum parameters of this node can themselves be modulated, giving you dynamic range control. Another common use is converting a normalised modulation signal to specific real-world values (such as a frequency range in Hz or a delay time in milliseconds).

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised 0-1 input to be remapped"
      range: "0.0 - 1.0"
      default: "0.0"
    Minimum:
      desc: "Lower bound of the output range"
      range: "0.0 - 1.0"
      default: "0.0"
    Maximum:
      desc: "Upper bound of the output range"
      range: "0.0 - 1.0"
      default: "1.0"
    Skew:
      desc: "Non-linear curve factor for the mapping"
      range: "0.1 - 10.0"
      default: "1.0"
    Polarity:
      desc: "Inverts the input before range mapping when set to Inverted"
      range: "Normal / Inverted"
      default: "Normal"
  functions:
    rangeMap:
      desc: "Converts the 0-1 input to the [Minimum, Maximum] range using the Skew curve"
    snapToStep:
      desc: "Quantises the output to the nearest multiple of Step (when Step > 0)"
---

```
// control.minmax - maps normalised input to custom range
// control in (0..1) -> control out (unnormalised)

onValueChange(input) {
    v = Value
    if (Polarity == Inverted)
        v = 1.0 - v
    v = rangeMap(v, Minimum, Maximum, Skew)
    output = snapToStep(v)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Normalised input to be remapped to the output range.", range: "0.0 - 1.0", default: "0.0" }
  - label: Range
    params:
      - { name: Minimum, desc: "Lower bound of the output range.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Maximum, desc: "Upper bound of the output range.", range: "0.0 - 1.0", default: "1.0" }
  - label: Shaping
    params:
      - { name: Skew, desc: "Non-linear curve factor. Values below 1.0 push the response toward the upper end; values above 1.0 push it toward the lower end. The slider is skewed so that 1.0 (linear) sits near the centre.", range: "0.1 - 10.0", default: "1.0" }
      - { name: Step, desc: "Quantisation interval. When set above 0, the output snaps to the nearest multiple of this value. Set to 0 for continuous output.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Polarity, desc: "When set to Inverted, the input is flipped before range mapping: a Value of 0 produces the Maximum and a Value of 1 produces the Minimum.", range: "Normal / Inverted", default: "Normal" }
---
::

The [control.normaliser]($SN.control.normaliser$) node performs the inverse operation: it takes an unscaled value and converts it back to normalised 0-1 using the range defined on its Value parameter.

**See also:** $SN.control.normaliser$ -- inverse operation converting unscaled to normalised, $SN.control.pma_unscaled$ -- simpler linear scaling without range mapping
