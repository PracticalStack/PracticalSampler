---
title: Pack8 Writer
description: "Writes eight parameter values into a slider pack."
factoryPath: control.pack8_writer
factory: control
polyphonic: false
tags: [control, slider-pack]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.pack2_writer", type: disambiguation, reason: "Base variant with 2 values" }
  - { id: "control.pack_resizer", type: companion, reason: "Dynamically resizes the target slider pack" }
  - { id: "control.clone_pack", type: companion, reason: "Reads slider pack values and distributes them to clones" }
llmRef: |
  control.pack8_writer

  Writes eight individual parameter values into a connected slider pack. Variant of pack2_writer with 8 Value parameters instead of 2. See control.pack2_writer for full details.

  Signal flow:
    Control node - no audio processing
    Value1 -> sliderPack[0] ... Value8 -> sliderPack[7]

  CPU: negligible, monophonic

  Parameters:
    Value1-Value8: 0.0 - 1.0 (default 0.0) -> slider pack indices 0-7

  When to use:
    Populating an 8-element slider pack from individual parameter connections or modulation sources. Largest fixed-size variant available.

  See also:
    [disambiguation] control.pack2_writer -- base variant with 2 values
    [companion] control.pack_resizer -- dynamically resizes slider packs
---

The pack8_writer writes eight individual parameter values into a connected slider pack. It is a variant of [control.pack2_writer]($SN.control.pack2_writer$) with 8 Value parameters instead of 2. Each parameter maps to a slider index: Value1 to index 0 through Value8 to index 7. This is the largest fixed-size variant in the pack writer family.

When connected, the slider pack is automatically resized to 8 entries. The node has no modulation output - it writes directly to the slider pack data. See [control.pack2_writer]($SN.control.pack2_writer$) for full behavioural details.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value1:
      desc: "Value written to slider pack index 0"
      range: "0.0 - 1.0"
      default: "0.0"
    Value2:
      desc: "Value written to slider pack index 1"
      range: "0.0 - 1.0"
      default: "0.0"
    Value3:
      desc: "Value written to slider pack index 2"
      range: "0.0 - 1.0"
      default: "0.0"
    Value4:
      desc: "Value written to slider pack index 3"
      range: "0.0 - 1.0"
      default: "0.0"
    Value5:
      desc: "Value written to slider pack index 4"
      range: "0.0 - 1.0"
      default: "0.0"
    Value6:
      desc: "Value written to slider pack index 5"
      range: "0.0 - 1.0"
      default: "0.0"
    Value7:
      desc: "Value written to slider pack index 6"
      range: "0.0 - 1.0"
      default: "0.0"
    Value8:
      desc: "Value written to slider pack index 7"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    write:
      desc: "Writes the parameter value to the corresponding slider pack index"
---

```
// control.pack8_writer - writes 8 parameters to slider pack
// control in -> slider pack data

onValueChange(ValueN) {
    write(sliderPack[N-1], ValueN)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Values
    params:
      - { name: Value1, desc: "Value written to slider pack index 0.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value2, desc: "Value written to slider pack index 1.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value3, desc: "Value written to slider pack index 2.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value4, desc: "Value written to slider pack index 3.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value5, desc: "Value written to slider pack index 4.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value6, desc: "Value written to slider pack index 5.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value7, desc: "Value written to slider pack index 6.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Value8, desc: "Value written to slider pack index 7.", range: "0.0 - 1.0", default: "0.0" }
---
::

**See also:** $SN.control.pack2_writer$ -- base variant with 2 values, $SN.control.pack_resizer$ -- dynamically resizes slider packs
