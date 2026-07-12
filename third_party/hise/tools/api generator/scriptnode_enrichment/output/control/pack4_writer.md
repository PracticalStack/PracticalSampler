---
title: Pack4 Writer
description: "Writes four parameter values into a slider pack."
factoryPath: control.pack4_writer
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
  control.pack4_writer

  Writes four individual parameter values into a connected slider pack. Variant of pack2_writer with 4 Value parameters instead of 2. See control.pack2_writer for full details.

  Signal flow:
    Control node - no audio processing
    Value1 -> sliderPack[0], Value2 -> sliderPack[1], Value3 -> sliderPack[2], Value4 -> sliderPack[3]

  CPU: negligible, monophonic

  Parameters:
    Value1-Value4: 0.0 - 1.0 (default 0.0) -> slider pack indices 0-3

  When to use:
    Populating a 4-element slider pack from individual parameter connections or modulation sources.

  See also:
    [disambiguation] control.pack2_writer -- base variant with 2 values
    [companion] control.pack_resizer -- dynamically resizes slider packs
---

The pack4_writer writes four individual parameter values into a connected slider pack. It is a variant of [control.pack2_writer]($SN.control.pack2_writer$) with 4 Value parameters instead of 2. Each parameter maps to a slider index: Value1 to index 0 through Value4 to index 3.

When connected, the slider pack is automatically resized to 4 entries. The node has no modulation output - it writes directly to the slider pack data. See [control.pack2_writer]($SN.control.pack2_writer$) for full behavioural details.

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
  functions:
    write:
      desc: "Writes the parameter value to the corresponding slider pack index"
---

```
// control.pack4_writer - writes 4 parameters to slider pack
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
---
::

**See also:** $SN.control.pack2_writer$ -- base variant with 2 values, $SN.control.pack_resizer$ -- dynamically resizes slider packs
