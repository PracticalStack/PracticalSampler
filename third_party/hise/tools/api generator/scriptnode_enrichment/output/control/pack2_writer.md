---
title: Pack2 Writer
description: "Writes two parameter values into a slider pack."
factoryPath: control.pack2_writer
factory: control
polyphonic: false
tags: [control, slider-pack]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.pack_resizer", type: companion, reason: "Dynamically resizes the target slider pack" }
  - { id: "control.clone_pack", type: companion, reason: "Reads slider pack values and distributes them to clones" }
commonMistakes:
  - title: "Connected slider pack is auto-resized"
    wrong: "Expecting the slider pack to retain its original size after connecting a pack writer"
    right: "The slider pack is automatically resized to match the number of Value parameters when the node is connected."
    explanation: "Connecting a pack2_writer to a slider pack resizes it to exactly 2 entries. If you need a larger pack, use a higher variant (pack3_writer through pack8_writer) or control the size separately with control.pack_resizer."
llmRef: |
  control.pack2_writer

  Writes two individual parameter values into a connected slider pack. Each Value parameter maps directly to a slider pack index. The node has no modulation output - it writes to the slider pack's complex data.

  Signal flow:
    Control node - no audio processing
    Value1 -> sliderPack[0]
    Value2 -> sliderPack[1]

  CPU: negligible, monophonic

  Parameters:
    Value1: 0.0 - 1.0 (default 0.0) -> writes to slider pack index 0
    Value2: 0.0 - 1.0 (default 0.0) -> writes to slider pack index 1

  When to use:
    Populating a 2-element slider pack from individual parameter connections or modulation sources. Useful when each slider value needs to come from a different source.

  Common mistakes:
    Connected slider pack is auto-resized to 2 entries.

  See also:
    [companion] control.pack_resizer -- dynamically resizes slider packs
    [companion] control.clone_pack -- reads slider pack values for clone distribution
---

The pack2_writer writes two individual parameter values into a connected slider pack. Each Value parameter maps directly to the corresponding slider index: Value1 writes to index 0, Value2 writes to index 1. The node has no modulation output - it writes directly to the slider pack data.

This is useful when you need to populate a slider pack from separate modulation sources or parameter connections, rather than editing slider values manually. Each parameter can be independently modulated, allowing dynamic control of individual slider entries.

When connected to a slider pack, the pack is automatically resized to 2 entries to match the number of Value parameters. For more entries, use a higher variant such as [control.pack3_writer]($SN.control.pack3_writer$) through [control.pack8_writer]($SN.control.pack8_writer$).

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
  functions:
    write:
      desc: "Writes the parameter value to the corresponding slider pack index"
---

```
// control.pack2_writer - writes parameters to slider pack
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
---
::

The node has no modulation output -- it writes to complex data (the slider pack), not to parameter targets. UI updates for the slider pack display are asynchronous. Variants from [control.pack3_writer]($SN.control.pack3_writer$) to [control.pack8_writer]($SN.control.pack8_writer$) provide the same functionality with 3 to 8 Value parameters respectively.

**See also:** $SN.control.pack_resizer$ -- dynamically resizes slider packs, $SN.control.clone_pack$ -- reads slider pack values for clone distribution
