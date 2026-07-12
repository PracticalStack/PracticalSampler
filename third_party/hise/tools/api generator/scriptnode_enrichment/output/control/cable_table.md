---
title: Cable Table
description: "Reshapes a control signal using a visual lookup table with linear interpolation."
factoryPath: control.cable_table
factory: control
polyphonic: false
tags: [control, table, lookup, transform]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.cable_expr", type: alternative, reason: "Formula-based transform for mathematical expressions" }
  - { id: "control.cable_pack", type: disambiguation, reason: "Slider pack lookup with discrete steps instead of smooth interpolation" }
commonMistakes: []
llmRef: |
  control.cable_table

  Reshapes a normalised control signal using a 512-point lookup table with linear interpolation. The table is edited visually in the node UI.

  Signal flow:
    Control node -- no audio processing
    Value (0..1) -> normalised table lookup with interpolation -> modulation output

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Lookup position in the table.

  Complex data:
    Table: 512-point lookup table, edited visually.

  When to use:
    Visually shaping a control signal curve without writing an expression. 2 instances across surveyed projects (rank 83). Prefer over cable_expr when the transform is easier to draw than to express mathematically.

  See also:
    [alternative] control.cable_expr -- formula-based transform
    [disambiguation] control.cable_pack -- discrete step lookup via slider pack
---

Reshapes a normalised control signal using a visual lookup table. The incoming Value (0..1) is used as a normalised position in a 512-point table, with linear interpolation between adjacent entries. The looked-up value is sent to the modulation output. This provides a visual, drawable alternative to [control.cable_expr]($SN.control.cable_expr$) for shaping control curves.

The table is edited directly in the node UI, where a vertical cursor shows the current lookup position. The output range depends on the table contents, which are typically constrained to 0..1 by the table editor.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised lookup position in the table"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    tableLookup:
      desc: "Reads the table at the normalised position with linear interpolation"
---

```
// control.cable_table - visual lookup table transform
// control in -> table lookup -> control out

onValueChange(Value) {
    output = tableLookup(Value)    // 512-point table, interpolated
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Normalised lookup position. 0.0 reads the start of the table; 1.0 reads the end.", range: "0.0 - 1.0", default: "0.0" }
---
::

If no table data is connected, the node produces no output. When the table data is updated (e.g. by editing the curve), the node automatically re-evaluates the current input position and sends the updated result.

**See also:** $SN.control.cable_expr$ -- formula-based transform, $SN.control.cable_pack$ -- discrete step lookup via slider pack
