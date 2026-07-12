---
title: Local Cable Unscaled
description: "Routes an unnormalised control value between nodes sharing the same ID within a single network."
factoryPath: routing.local_cable_unscaled
factory: routing
polyphonic: false
tags: [routing, control, local, cable, unscaled]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "routing.local_cable", type: disambiguation, reason: "Same behaviour but with normalised 0..1 values" }
commonMistakes:
  - title: "Use in unscaled parameter contexts only"
    wrong: "Using local_cable_unscaled where the target expects a normalised 0..1 input"
    right: "Use routing.local_cable for normalised connections. Use local_cable_unscaled only when connecting to parameters or sources that operate in raw value ranges."
    explanation: "The unscaled variant bypasses range conversion on both input and output. If the target expects normalised values, the raw values will be misinterpreted."
llmRef: |
  routing.local_cable_unscaled

  Routes an unnormalised control value between nodes sharing the same LocalId within a single DspNetwork. Identical to local_cable but values bypass range conversion on both input and output.

  Signal flow:
    Control node -- no audio processing
    Value param (raw) -> broadcast to peers with same LocalId -> modulation output (raw, unnormalised)

  CPU: negligible, monophonic

  Parameters:
    Value: arbitrary range (nominal 0.0 - 1.0, default 0.0). Raw value routed without range conversion.

  When to use:
    Replacing visual cable connections for unscaled parameter contexts. The IDE automatically selects this variant when replacing an unscaled connection with a local cable.

  See also:
    [disambiguation] routing.local_cable -- normalised variant
---

A variant of [local_cable]($SN.routing.local_cable$) that passes values through without range conversion. Both the input and the modulation output bypass normalisation, so the raw value from the source arrives unchanged at the target. Use this when connecting parameters or modulation sources that operate in their native value ranges rather than the normalised 0..1 range.

All other behaviour is identical to local_cable -- bidirectional routing within a single network, recursion guard, and the same LocalId-based peer system. When using the connection editor to replace a cable with a local cable, the IDE automatically detects whether the connection is scaled or unscaled and creates the appropriate variant.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Raw control value routed without normalisation"
      range: "arbitrary"
      default: "0.0"
  functions:
    broadcastToPeers:
      desc: "Sends the raw value to all local_cable_unscaled nodes with the same LocalId"
---

```
// routing.local_cable_unscaled - unscaled network-scoped value router
// control in (raw) -> broadcast -> modulation out (raw)

onValueChange(Value) {
    broadcastToPeers(Value)  // raw value, no range conversion
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Control value routed without range conversion. The modulation output sends the raw value rather than normalising to 0..1.", range: "0.0 - 1.0 (nominal)", default: "0.0" }
---
::

**See also:** $SN.routing.local_cable$ -- normalised variant for standard 0..1 connections
