---
title: Blend
description: "Linearly interpolates between two input values based on the Alpha parameter."
factoryPath: control.blend
factory: control
polyphonic: true
tags: [control, interpolation, mix]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.input_toggle", type: disambiguation, reason: "Hard switch between two values instead of blending" }
  - { id: "control.xfader", type: alternative, reason: "Crossfade across multiple outputs with selectable curve shapes" }
commonMistakes: []
llmRef: |
  control.blend

  Linearly interpolates between two input values (Value1, Value2) using Alpha as the blend ratio. Output is unnormalised.

  Signal flow:
    Control node -- no audio processing
    Alpha + Value1 + Value2 -> linear interpolation -> modulation output

  CPU: negligible, polyphonic

  Parameters:
    Alpha: 0.0 - 1.0 (default 0.0). Blend ratio. 0 = Value1, 1 = Value2.
    Value1: 0.0 - 1.0 (default 0.0). First input value.
    Value2: 0.0 - 1.0 (default 0.0). Second input value.

  When to use:
    Smoothly crossfade between two modulation values using a single control. 16 instances across surveyed projects (rank 26). Common for morphing between two parameter settings or mixing two control signals.

  See also:
    [disambiguation] control.input_toggle -- hard switch between two values
    [alternative] control.xfader -- multi-output crossfade with curve options
---

Linearly interpolates between two input values using the Alpha parameter as the blend ratio. When Alpha is 0, the output equals Value1. When Alpha is 1, the output equals Value2. Intermediate Alpha values produce a proportional mix using the formula `Value1 + (Value2 - Value1) * Alpha`. The output is unnormalised, so the raw interpolated result is forwarded without range conversion.

Each voice maintains its own independent blend state in polyphonic mode. Any change to Alpha, Value1, or Value2 immediately recalculates and sends the blended output.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Alpha:
      desc: "Blend ratio between the two values"
      range: "0.0 - 1.0"
      default: "0.0"
    Value1:
      desc: "First input value (selected when Alpha = 0)"
      range: "0.0 - 1.0"
      default: "0.0"
    Value2:
      desc: "Second input value (selected when Alpha = 1)"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    lerp:
      desc: "Linear interpolation: Value1 + (Value2 - Value1) * Alpha"
---

```
// control.blend - linear interpolation between two values
// control in -> blended value out

onValueChange(Alpha, Value1, Value2) {
    output = lerp(Value1, Value2, Alpha)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Alpha, desc: "Blend ratio. At 0.0 the output equals Value1; at 1.0 it equals Value2.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value1, desc: "First input value.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value2, desc: "Second input value.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.control.input_toggle$ -- hard switch between two values, $SN.control.xfader$ -- multi-output crossfade with selectable curves
