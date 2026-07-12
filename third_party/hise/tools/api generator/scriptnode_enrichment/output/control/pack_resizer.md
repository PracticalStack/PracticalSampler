---
title: Pack Resizer
description: "Dynamically resizes a connected slider pack at runtime."
factoryPath: control.pack_resizer
factory: control
polyphonic: false
tags: [control, slider-pack]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.pack2_writer", type: companion, reason: "Writes individual values into a slider pack" }
  - { id: "control.clone_pack", type: companion, reason: "Reads slider pack values for clone distribution" }
commonMistakes:
  - title: "Avoid audio-rate modulation of NumSliders"
    wrong: "Connecting an audio-rate modulation source to the NumSliders parameter"
    right: "Drive NumSliders from UI controls or low-rate control nodes only."
    explanation: "Resizing involves memory allocation, which is not suitable for audio-rate changes. Use parameter changes from the UI thread or low-rate control sources."
llmRef: |
  control.pack_resizer

  Dynamically resizes a connected slider pack to the specified number of entries. Existing values are preserved when the pack grows; entries are truncated when it shrinks.

  Signal flow:
    Control node - no audio processing
    NumSliders -> resize slider pack

  CPU: negligible, monophonic

  Parameters:
    NumSliders: 0 - 128 (integer, default 0, effective minimum 1)
      Number of entries in the slider pack. Values below 1 are clamped to 1.

  When to use:
    Making a slider pack's size controllable at runtime, e.g. matching the number of active clones, frequency bands, or steps in a sequencer.

  Common mistakes:
    Avoid audio-rate modulation of NumSliders - resizing involves memory allocation.

  See also:
    [companion] control.pack2_writer -- writes individual values into a slider pack
    [companion] control.clone_pack -- reads slider pack values for clone distribution
---

The pack resizer dynamically changes the number of entries in a connected slider pack. When the pack grows, existing values are preserved and new entries receive default values. When it shrinks, entries beyond the new size are removed.

The NumSliders parameter accepts values from 0 to 128, but values below 1 are clamped to 1 - the slider pack always has at least one entry. This node has no modulation output; it modifies the slider pack's dimensions directly.

## Signal Path

::signal-path
---
glossary:
  parameters:
    NumSliders:
      desc: "Target size for the slider pack"
      range: "0 - 128 (effective 1 - 128)"
      default: "0"
  functions:
    resize:
      desc: "Sets the slider pack entry count, preserving existing values"
---

```
// control.pack_resizer - resizes a slider pack
// control in -> slider pack resize

onValueChange(NumSliders) {
    resize(sliderPack, clamp(NumSliders, 1, 128))
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: NumSliders, desc: "Number of entries in the slider pack. Values below 1 are clamped to 1. Existing values are preserved when increasing the size.", range: "0 - 128", default: "0" }
---
::

Unlike the pack writer nodes, pack_resizer does not auto-resize the slider pack when first connected -- it only resizes when the NumSliders parameter changes.

**See also:** $SN.control.pack2_writer$ -- writes individual values into a slider pack, $SN.control.clone_pack$ -- reads slider pack values for clone distribution
