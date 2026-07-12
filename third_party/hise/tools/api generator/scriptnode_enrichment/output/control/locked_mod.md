---
title: Locked Mod
description: "Adds a normalised modulation output to the parent container when it is locked, exposing internal modulation as a draggable source."
factoryPath: control.locked_mod
factory: control
polyphonic: false
tags: [control, modulation, container]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.locked_mod_unscaled", type: disambiguation, reason: "Unscaled variant -- forwards raw values without range conversion" }
commonMistakes:
  - title: "Must be an immediate child of the container"
    wrong: "Placing locked_mod inside a nested sub-container and expecting it to appear on the outer locked container"
    right: "Place locked_mod as a direct child of the container you want to lock."
    explanation: "The modulation dragger only appears on the node's immediate parent container. For nested locked containers, each level needs its own locked_mod node."
llmRef: |
  control.locked_mod

  Adds a normalised modulation dragger to the parent container when locked. Passes the Value parameter (0..1) through to the container's modulation output with target range conversion applied.

  Signal flow:
    Control node -- no audio processing
    Value (0..1) -> passthrough -> normalised modulation output on locked container

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). The normalised value sent to the container's modulation output.

  When to use:
    Turning a container into a reusable modulation source. Connect any internal signal to Value, lock the container, and drag the modulation output to targets elsewhere in the network. Not observed in the surveyed projects.

  See also:
    [disambiguation] control.locked_mod_unscaled -- unscaled variant
---

Turns a locked container into a modulation source by forwarding the Value parameter to the container's modulation output. When this node is placed as an immediate child of a container and the container is locked (via the lock icon in the toolbar), a modulation dragger appears on the locked container that can be connected to any target in the network.

The output is normalised: the connection system applies the target parameter's range to convert the 0..1 value appropriately. Connect any internal modulation source to the Value parameter to expose it on the parent container.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Normalised input value forwarded to the container's modulation output"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.locked_mod - normalised modulation passthrough
// control in (0..1) -> container modulation out

onValueChange(input) {
    sendToOutput(input)     // normalised: target range applied by connection
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "The normalised value to send to the locked container's modulation output. The connection system applies the target parameter's range.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Setup

To use this node:

1. Add it as an immediate child of a container
2. Connect its Value parameter to any modulation source inside the container
3. Lock the container using the lock icon in the toolbar

The locked container then shows a modulation dragger that can be connected to targets outside the container. This is useful for creating reusable modulation building blocks.

**See also:** $SN.control.locked_mod_unscaled$ -- unscaled variant that forwards raw values without range conversion
