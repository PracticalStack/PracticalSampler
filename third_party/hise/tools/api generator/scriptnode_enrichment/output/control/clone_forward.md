---
title: Clone Forward
description: "Forwards the same unscaled value to all clones in a clone container."
factoryPath: control.clone_forward
factory: control
polyphonic: false
tags: [control, clone]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors:
    - { parameter: "NumClones", impact: "linear", note: "One forwarding call per active clone" }
seeAlso:
  - { id: "control.clone_cable", type: alternative, reason: "Distributes different values to each clone using a formula" }
  - { id: "control.clone_pack", type: alternative, reason: "Per-clone values driven by a slider pack" }
  - { id: "container.clone", type: companion, reason: "The clone container that this node controls" }
commonMistakes:
  - title: "Value bypasses target parameter range"
    wrong: "Expecting the target parameter's range to scale the incoming value"
    right: "Ensure the source value is already in the correct range for the target parameter."
    explanation: "clone_forward uses unnormalised modulation. The raw value is written directly to each clone's target parameter without range conversion. This is by design, allowing a single source to drive parameters with matching ranges across all clones."
llmRef: |
  control.clone_forward

  Forwards the same unscaled value identically to all clones in a container.clone. Unlike clone_cable, there is no distribution logic - every clone receives the exact same raw value.

  Signal flow:
    Control node - no audio processing
    Value -> forward unchanged -> per-clone output (unnormalised)

  CPU: negligible, monophonic

  Parameters:
    NumClones: 1 - 16 (integer, default 1)
      Auto-synced from the parent clone container.
    Value: 0.0 - 1.0 (default 0.0)
      Unnormalised input. The raw value is forwarded to all clones without range conversion.

  When to use:
    Broadcasting a single parameter value to all clones simultaneously - e.g. a shared filter cutoff, gain, or effect parameter that should be identical across all clones.

  Common mistakes:
    Value bypasses target parameter range scaling. Ensure the source is already in the correct range.

  See also:
    [alternative] control.clone_cable -- distributes different values per clone
    [alternative] control.clone_pack -- per-clone values via slider pack
    [companion] container.clone -- the clone container this node controls
---

The clone forward node sends the same raw value to every clone in a [container.clone]($SN.container.clone$). Unlike [control.clone_cable]($SN.control.clone_cable$), there is no distribution formula - all clones receive an identical copy of the input value.

The Value parameter uses unnormalised modulation, meaning the raw value is forwarded directly to each clone's target parameter without any range conversion. This is useful when a single control source (such as a macro parameter or another modulation node) needs to drive the same parameter across all clones with the exact same value.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Unscaled input value forwarded to all clones"
      range: "0.0 - 1.0"
      default: "0.0"
    NumClones:
      desc: "Number of active clones to address"
      range: "1 - 16"
      default: "1"
  functions:
    forward:
      desc: "Sends the same raw value to each clone"
---

```
// control.clone_forward - broadcasts value to all clones
// control in -> per-clone control out (unnormalised)

onValueChange(Value) {
    for each clone [0..NumClones]:
        output[clone] = forward(Value)
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
      - { name: Value, desc: "The input value forwarded to all clones. Uses unnormalised modulation - the raw value is sent without range conversion.", range: "0.0 - 1.0", default: "0.0" }
---
::

The NumClones parameter is automatically kept in sync with the parent clone container. This node does not process MIDI events.

**See also:** $SN.control.clone_cable$ -- distributes different values per clone, $SN.control.clone_pack$ -- per-clone values via a slider pack, $SN.container.clone$ -- the clone container
