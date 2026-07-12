---
title: Locked Mod Unscaled
description: "Adds an unnormalised modulation output to the parent container when it is locked, forwarding raw values without range conversion."
factoryPath: control.locked_mod_unscaled
factory: control
polyphonic: false
tags: [control, modulation, container, unscaled]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.locked_mod", type: disambiguation, reason: "Normalised variant -- applies target range conversion" }
commonMistakes:
  - title: "Must be an immediate child of the container"
    wrong: "Placing locked_mod_unscaled inside a nested sub-container and expecting it to appear on the outer locked container"
    right: "Place locked_mod_unscaled as a direct child of the container you want to lock."
    explanation: "The modulation dragger only appears on the node's immediate parent container. For nested locked containers, each level needs its own locked_mod node."
llmRef: |
  control.locked_mod_unscaled

  Adds an unnormalised modulation dragger to the parent container when locked. Passes the Value parameter through without range conversion -- the raw value reaches the target directly.

  Signal flow:
    Control node -- no audio processing
    Value (raw) -> passthrough -> unnormalised modulation output on locked container

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). The raw value sent to the container's modulation output. Unnormalised -- no range conversion applied.

  When to use:
    Same as locked_mod but when the modulation source produces values in the target's native range (e.g. frequencies in Hz, times in ms). Avoids double-scaling when the source already outputs correctly ranged values. Not observed in the surveyed projects.

  See also:
    [disambiguation] control.locked_mod -- normalised variant
---

The unscaled variant of [locked_mod]($SN.control.locked_mod$). Turns a locked container into a modulation source by forwarding the Value parameter to the container's modulation output without range conversion. The raw value reaches the target parameter directly.

Use this variant when the modulation source inside the container already produces values in the target parameter's native range (e.g. frequencies in Hz or times in milliseconds). The normalised variant [locked_mod]($SN.control.locked_mod$) is more appropriate when working with 0..1 normalised signals.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Raw input value forwarded to the container's modulation output"
      range: "0.0 - 1.0"
      default: "0.0"
---

```
// control.locked_mod_unscaled - unnormalised modulation passthrough
// control in (raw) -> container modulation out (raw)

onValueChange(input) {
    sendToOutput(input)     // unnormalised: raw value reaches target directly
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "The raw value to send to the locked container's modulation output. Unnormalised -- the value reaches the target parameter without range conversion.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Setup

The setup procedure is identical to [locked_mod]($SN.control.locked_mod$): place this node as an immediate child of a container, connect its Value parameter, and lock the container. The only difference is that the modulation output bypasses range conversion, so the raw value flows directly to the connected target.

**See also:** $SN.control.locked_mod$ -- normalised variant that applies target range conversion
