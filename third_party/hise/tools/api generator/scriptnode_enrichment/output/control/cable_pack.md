---
title: Cable Pack
description: "Reshapes a control signal using a slider pack lookup with discrete steps."
factoryPath: control.cable_pack
factory: control
polyphonic: false
tags: [control, sliderpack, lookup, transform]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.cable_table", type: disambiguation, reason: "Smooth lookup table with interpolation instead of discrete steps" }
  - { id: "control.cable_expr", type: alternative, reason: "Formula-based transform for mathematical expressions" }
commonMistakes:
  - title: "Output jumps between discrete slider values"
    wrong: "Expecting a smooth output curve when sweeping the Value input"
    right: "The lookup uses nearest-neighbour selection with no interpolation. The output steps between discrete slider values. Use cable_table for smooth curves."
    explanation: "Unlike cable_table which interpolates between table entries, cable_pack reads the nearest slider value directly. With 8 sliders, the output has 8 discrete levels."
llmRef: |
  control.cable_pack

  Reshapes a normalised control signal using a slider pack lookup. No interpolation -- nearest-neighbour (step) lookup produces discrete output levels.

  Signal flow:
    Control node -- no audio processing
    Value (0..1) -> normalised index into slider pack -> nearest slider value -> modulation output

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Lookup position in the slider pack.

  Complex data:
    SliderPack: Variable-size slider array, edited visually.

  When to use:
    Discrete-step control signal shaping or sequencer-style value lookup. Unused in surveyed projects but useful when a fixed set of values should be selected by a continuous input.

  Common mistakes:
    No interpolation between sliders -- output jumps between discrete values. Use cable_table for smooth curves.

  See also:
    [disambiguation] control.cable_table -- smooth table lookup with interpolation
    [alternative] control.cable_expr -- formula-based transform
---

Reshapes a normalised control signal using a slider pack lookup. The incoming Value (0..1) is scaled to the slider pack size and the nearest slider value is read directly -- no interpolation is applied. This produces discrete output levels corresponding to each slider position, making it suited for step-sequencer-style value selection or quantised control curves.

The slider pack is edited visually in the node UI, where the current lookup position is highlighted. The number of discrete steps equals the number of sliders in the pack. Unlike [control.cable_table]($SN.control.cable_table$), which interpolates between 512 table entries for smooth curves, cable_pack produces hard steps between slider values.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised lookup position in the slider pack"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    packLookup:
      desc: "Reads the nearest slider value at the normalised position (no interpolation)"
---

```
// control.cable_pack - discrete slider pack lookup
// control in -> nearest slider value out

onValueChange(Value) {
    sliderIndex = round(Value * numSliders)
    output = packLookup(sliderIndex)    // no interpolation
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Normalised lookup position. Scaled to the slider pack size and rounded to the nearest slider index.", range: "0.0 - 1.0", default: "0.0" }
---
::

If no slider pack data is connected, the node produces no output. When the slider pack data is updated, the node re-evaluates the current input position and sends the updated result. The slider pack size determines the number of discrete output levels -- with 8 sliders, the 0..1 input range is divided into 8 equal zones, each mapping to one slider value.

**See also:** $SN.control.cable_table$ -- smooth table lookup with interpolation, $SN.control.cable_expr$ -- formula-based transform
