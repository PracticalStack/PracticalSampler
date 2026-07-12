---
title: Clone Pack
description: "Sends per-clone values from a slider pack, scaled by a global multiplier."
factoryPath: control.clone_pack
factory: control
polyphonic: false
tags: [control, clone, slider-pack]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "NumClones", impact: "linear", note: "One multiply and send per active clone" }
seeAlso:
  - { id: "control.clone_cable", type: alternative, reason: "Distributes values using a mathematical formula instead of explicit per-clone data" }
  - { id: "control.clone_forward", type: alternative, reason: "Sends the same value to all clones" }
  - { id: "control.pack_resizer", type: companion, reason: "Dynamically resizes the slider pack to match the clone count" }
  - { id: "container.clone", type: companion, reason: "The clone container that this node controls" }
commonMistakes:
  - title: "Slider pack size must match clone count"
    wrong: "Having fewer slider pack entries than active clones"
    right: "Ensure the slider pack has at least as many entries as there are active clones, or use control.pack_resizer to keep them in sync."
    explanation: "Clones beyond the slider pack size receive no update and retain their last value. The slider pack is not automatically resized to match the clone count."
llmRef: |
  control.clone_pack

  Sends per-clone values from a slider pack, each multiplied by a global Value parameter. Each slider index maps to the corresponding clone index.

  Signal flow:
    Control node - no audio processing
    sliderPack[i] * Value -> per-clone output (normalised)

  CPU: negligible, monophonic

  Parameters:
    NumClones: 1 - 16 (integer, default 1)
      Auto-synced from the parent clone container.
    Value: 0.0 - 1.0 (default 1.0)
      Global multiplier applied to all slider pack values.

  When to use:
    Arbitrary per-clone parameter control where each clone needs an explicitly set value - e.g. custom detuning curves, per-voice gain patterns, or any parameter distribution that does not fit a mathematical formula.

  Common mistakes:
    Slider pack size must match clone count. Extra clones receive no update.

  See also:
    [alternative] control.clone_cable -- formula-based distribution
    [alternative] control.clone_forward -- same value to all clones
    [companion] control.pack_resizer -- dynamically resizes the slider pack
    [companion] container.clone -- the clone container this node controls
---

The clone pack node sends individually specified values to each clone in a [container.clone]($SN.container.clone$). Each slider in the connected slider pack maps to the corresponding clone index. The Value parameter acts as a global multiplier applied to all slider values before they are sent.

This provides fully arbitrary per-clone control, unlike [control.clone_cable]($SN.control.clone_cable$) which uses mathematical distribution formulas. It is ideal for cases where each clone needs a hand-crafted or script-driven value that does not follow a regular pattern.

Editing a single slider in the pack updates only the corresponding clone, not all clones. This makes it efficient for real-time slider pack manipulation.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Global multiplier applied to all slider values"
      range: "0.0 - 1.0"
      default: "1.0"
    NumClones:
      desc: "Number of active clones to address"
      range: "1 - 16"
      default: "1"
  functions:
    multiply:
      desc: "Multiplies the slider value at each index by the global Value"
---

```
// control.clone_pack - slider pack to per-clone values
// slider pack + control in -> per-clone control out (normalised)

onValueChange(Value) {
    for each clone [0..NumClones]:
        output[clone] = multiply(sliderPack[clone], Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Configuration
    params:
      - { name: NumClones, desc: "Number of active clones. Automatically synchronised from the parent clone container.", range: "1 - 16", default: "1" }
  - label: Signal
    params:
      - { name: Value, desc: "Global multiplier applied to all slider pack values before sending to clones.", range: "0.0 - 1.0", default: "1.0" }
---
::

### Slider Pack Sizing

The slider pack is not automatically resized when the clone count changes. Use [control.pack_resizer]($SN.control.pack_resizer$) if you need dynamic resizing, or set the pack size manually to match the maximum clone count. If the slider pack has fewer entries than the active clone count, extra clones are not updated. If it has more entries than clones, extra entries are ignored. Output values are normalised (0-1), so target parameter ranges are applied automatically.

**See also:** $SN.control.clone_cable$ -- formula-based per-clone distribution, $SN.control.clone_forward$ -- same value to all clones, $SN.control.pack_resizer$ -- dynamically resizes the slider pack, $SN.container.clone$ -- the clone container
