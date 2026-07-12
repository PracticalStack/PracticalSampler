---
title: PMA
description: "Scales and offsets a normalised modulation signal using a multiply-add formula with clamped output."
factoryPath: control.pma
factory: control
polyphonic: true
tags: [control, pma, multiply, add, scaling, modulation]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.pma_unscaled", type: disambiguation, reason: "Unscaled variant that passes raw values without clamping" }
  - { id: "control.bipolar", type: alternative, reason: "Centres modulation around the midpoint instead of scaling linearly" }
  - { id: "control.intensity", type: alternative, reason: "Scales modulation depth using the HISE intensity formula" }
commonMistakes:
  - title: "Output is clamped to 0-1"
    wrong: "Setting Multiply to 2.0 and Add to 0.5 expecting the output to exceed 1.0"
    right: "Use control.pma_unscaled if you need unclamped output, or adjust Multiply and Add so the result stays within 0-1."
    explanation: "The output is always clamped to the 0-1 range. Any result above 1.0 or below 0.0 is clipped, which can flatten the modulation curve at the extremes."
llmRef: |
  control.pma

  Scales and offsets a normalised modulation signal. Computes Value * Multiply + Add, clamped to 0-1. PMA stands for Parameter Multiply Add.

  Signal flow:
    Control node - no audio processing
    Value (0..1) -> * Multiply -> + Add -> clamp(0, 1) -> modulation out

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): input signal, typically connected to a modulation source
    Multiply (-1.0 - 1.0, default 1.0): scale factor
    Add (-1.0 - 1.0, default 0.0): constant offset applied after multiplication

  When to use:
    Commonly used (rank 35, 11 instances). Use when you need to scale or offset a normalised modulation signal before it reaches its target. Typical uses include controlling modulation intensity, inverting a signal (Multiply = -1), or adding a constant baseline.

  Common mistakes:
    Output is clamped to 0-1 -- use pma_unscaled for unclamped output.

  See also:
    [disambiguation] control.pma_unscaled -- unscaled variant without clamping
    [alternative] control.bipolar -- centres modulation around the midpoint
    [alternative] control.intensity -- scales depth using HISE intensity formula
---

The PMA node (Parameter Multiply Add) scales and offsets a normalised modulation signal. It takes a 0-1 input value, multiplies it by a configurable factor, adds a constant offset, and clamps the result to the 0-1 range. This is one of the most commonly used control nodes for adjusting modulation depth, inverting signals, or combining parameters.

Each parameter change triggers an independent output update. If Value, Multiply, and Add all change in sequence, three separate output values are sent to connected targets.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Primary normalised input signal"
      range: "0.0 - 1.0"
      default: "0.0"
    Multiply:
      desc: "Scale factor applied to the input"
      range: "-1.0 - 1.0"
      default: "1.0"
    Add:
      desc: "Constant offset added after multiplication"
      range: "-1.0 - 1.0"
      default: "0.0"
  functions:
    clamp:
      desc: "Restricts the result to the 0-1 range"
---

```
// control.pma - scales and offsets a modulation signal
// control in -> control out

onValueChange(input) {
    output = clamp(Value * Multiply + Add, 0.0, 1.0)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Input signal, typically connected to a modulation source.", range: "0.0 - 1.0", default: "0.0" }
  - label: Transform
    params:
      - { name: Multiply, desc: "Scale factor applied to the input value. Set to -1 to invert the signal.", range: "-1.0 - 1.0", default: "1.0" }
      - { name: Add, desc: "Constant offset added after multiplication. Shifts the output up or down within the clamped range.", range: "-1.0 - 1.0", default: "0.0" }
---
::

The output is always clamped to the 0-1 range regardless of the Multiply and Add values. If you need the result to exceed this range (for example when working with frequency or pitch values), use [control.pma_unscaled]($SN.control.pma_unscaled$) instead. With the default settings (Multiply = 1.0, Add = 0.0), the node passes the input value through unchanged.

**See also:** $SN.control.pma_unscaled$ -- unscaled variant without clamping, $SN.control.bipolar$ -- centres modulation around the midpoint, $SN.control.intensity$ -- scales depth using the HISE intensity formula
