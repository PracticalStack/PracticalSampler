---
title: Pack3 Writer
description: "Writes three parameter values into a slider pack."
factoryPath: control.pack3_writer
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
  control.pack3_writer

  Writes three individual parameter values into a connected slider pack. Variant of pack2_writer with 3 Value parameters instead of 2. See control.pack2_writer for full details.

  Signal flow:
    Control node - no audio processing
    Value1 -> sliderPack[0], Value2 -> sliderPack[1], Value3 -> sliderPack[2]

  CPU: negligible, monophonic

  Parameters:
    Value1: 0.0 - 1.0 (default 0.0) -> slider pack index 0
    Value2: 0.0 - 1.0 (default 0.0) -> slider pack index 1
    Value3: 0.0 - 1.0 (default 0.0) -> slider pack index 2

  When to use:
    Populating a 3-element slider pack from individual parameter connections or modulation sources.

  See also:
    [disambiguation] control.pack2_writer -- base variant with 2 values
    [companion] control.pack_resizer -- dynamically resizes slider packs
---

The pack3_writer writes three individual parameter values into a connected slider pack. It is a variant of [control.pack2_writer]($SN.control.pack2_writer$) with 3 Value parameters instead of 2. Each parameter maps to a slider index: Value1 to index 0, Value2 to index 1, Value3 to index 2.

When connected, the slider pack is automatically resized to 3 entries. The node has no modulation output - it writes directly to the slider pack data. See [control.pack2_writer]($SN.control.pack2_writer$) for full behavioural details.

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
  functions:
    write:
      desc: "Writes the parameter value to the corresponding slider pack index"
---

```
// control.pack3_writer - writes 3 parameters to slider pack
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
---
::

**See also:** $SN.control.pack2_writer$ -- base variant with 2 values, $SN.control.pack_resizer$ -- dynamically resizes slider packs
