---
title: Input Toggle
description: "Switches between two input values and forwards the selected one to the modulation output."
factoryPath: control.input_toggle
factory: control
polyphonic: true
tags: [control, switch, multiplexer]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.blend", type: disambiguation, reason: "Smooth interpolation between two values instead of hard switching" }
  - { id: "control.branch_cable", type: alternative, reason: "Routes a single value to one of many outputs instead of selecting one of two inputs" }
commonMistakes:
  - title: "Inactive value changes are not forwarded"
    wrong: "Expecting the output to update when changing Value2 while Input selects Value1"
    right: "Only the currently active value is forwarded. Changes to the inactive value are stored but not sent until it becomes active."
    explanation: "The node only sends output when the active value changes or when the Input selector switches. Updating the inactive value has no effect on the output until the toggle switches to it."
llmRef: |
  control.input_toggle

  Selects between two stored values (Value1, Value2) based on the Input parameter and forwards the selected one. Output is unnormalised.

  Signal flow:
    Control node -- no audio processing
    Input (< 0.5 = Value1, >= 0.5 = Value2) -> forward selected value -> modulation output

  CPU: negligible, polyphonic

  Parameters:
    Input: 0 / 1 (default 0, step 1.0). Selector displayed as "Input 1" / "Input 2".
    Value1: 0.0 - 1.0 (default 0.0). First value (unnormalised input).
    Value2: 0.0 - 1.0 (default 0.0). Second value (unnormalised input).

  When to use:
    Hard-switching between two modulation values. 3 instances across surveyed projects (rank 66). Use when a binary toggle should select between two distinct parameter settings.

  Common mistakes:
    Changes to the inactive value are stored but not forwarded until the Input switches to it.

  See also:
    [disambiguation] control.blend -- smooth interpolation between two values
    [alternative] control.branch_cable -- routes one value to many outputs
---

Selects between two stored values based on the Input parameter and forwards the active one to the modulation output. When Input is set to "Input 1" (below 0.5), Value1 is forwarded. When Input is set to "Input 2" (0.5 or above), Value2 is forwarded. The selected value passes through without any range conversion.

Unlike [control.blend]($SN.control.blend$), which smoothly interpolates between two values, input_toggle performs a hard switch. Only changes to the currently active value produce output -- modifying the inactive value updates its stored state but does not trigger the modulation output until the toggle switches to it.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Input:
      desc: "Selector: Input 1 or Input 2"
      range: "0 / 1"
      default: "0"
    Value1:
      desc: "First input value (unnormalised)"
      range: "0.0 - 1.0"
      default: "0.0"
    Value2:
      desc: "Second input value (unnormalised)"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.input_toggle - two-way value selector
// control in -> selected value out

onValueChange(Input, Value1, Value2) {
    if (Input < 0.5)
        output = Value1
    else
        output = Value2
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Input, desc: "Selects which value to forward. Displayed as 'Input 1' / 'Input 2'.", range: "Input 1 / Input 2", default: "Input 1" }
      - { name: Value1, desc: "First input value. Receives raw values without range conversion.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value2, desc: "Second input value. Receives raw values without range conversion.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.control.blend$ -- smooth interpolation between two values, $SN.control.branch_cable$ -- routes a single value to one of many outputs
