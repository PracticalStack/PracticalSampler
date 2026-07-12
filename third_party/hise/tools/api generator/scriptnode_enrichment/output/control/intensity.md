---
title: Intensity
description: "Applies the HISE gain modulation intensity formula to scale how much a modulation signal affects the output."
factoryPath: control.intensity
factory: control
polyphonic: true
tags: [control, intensity, modulation, depth, gain]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.pma", type: alternative, reason: "General-purpose scaling and offset for modulation signals" }
  - { id: "control.bipolar", type: alternative, reason: "Scales modulation symmetrically around the midpoint" }
commonMistakes:
  - title: "Intensity of 0 outputs 1.0, not 0.0"
    wrong: "Setting Intensity to 0 and expecting the output to be silent or zero"
    right: "Intensity = 0 means no modulation effect, so the output is always 1.0. Set Intensity to 1.0 for the input to fully control the output."
    explanation: "The formula interpolates between 1.0 (no effect) and the input value. At Intensity = 0 the output is always 1.0, matching how HISE gain modulators behave when their intensity is turned down."
llmRef: |
  control.intensity

  Applies the HISE gain modulation intensity formula: output = (1 - Intensity) + Intensity * Value. Interpolates between 1.0 (no modulation) and the input value.

  Signal flow:
    Control node - no audio processing
    Value (0..1) + Intensity (0..1) -> intensity formula -> modulation out (0..1)

  CPU: negligible, polyphonic

  Parameters:
    Value (0.0 - 1.0, default 0.0): modulation input
    Intensity (0.0 - 1.0, default 1.0): modulation depth (0 = no effect, 1 = full)

  When to use:
    Use when you want to replicate the behaviour of the HISE module tree's gain modulation intensity control inside scriptnode. Useful for adding a user-facing "modulation amount" knob that fades between no effect and full modulation.

  See also:
    [alternative] control.pma -- general-purpose multiply-add scaling
    [alternative] control.bipolar -- symmetric scaling around centre
---

Applies the standard HISE gain modulation intensity formula to a modulation signal. The output interpolates between 1.0 (no modulation effect) and the input value based on the Intensity parameter. This replicates the behaviour of the intensity control found on gain modulators in the HISE module tree.

The formula is: `output = (1.0 - Intensity) + Intensity * Value`. When Intensity is 0, the output is always 1.0 regardless of the input. When Intensity is 1.0, the output equals the input value directly. This makes it straightforward to add a user-facing "modulation amount" control that smoothly fades between unmodulated and fully modulated states.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised modulation input"
      range: "0.0 - 1.0"
      default: "0.0"
    Intensity:
      desc: "Modulation depth -- 0 means no effect, 1 means full modulation"
      range: "0.0 - 1.0"
      default: "1.0"
---

```
// control.intensity - HISE gain modulation intensity
// control in -> control out

onValueChange(input) {
    output = (1.0 - Intensity) + Intensity * Value
}
```

::

## Parameters

::parameter-table
---
groups:
  - params:
      - { name: Value, desc: "Modulation input signal, typically connected to a modulation source.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Intensity, desc: "Controls how much the input value affects the output. At 0.0 the output is always 1.0; at 1.0 the output equals the input.", range: "0.0 - 1.0", default: "1.0" }
---
::

**See also:** $SN.control.pma$ -- general-purpose multiply-add scaling, $SN.control.bipolar$ -- symmetric modulation around the midpoint
