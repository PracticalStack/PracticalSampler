---
title: Public Mod
description: "Exposes a value as a modulation output on the parent network, allowing compiled nodes to act as modulation sources."
factoryPath: routing.public_mod
factory: routing
polyphonic: false
tags: [routing, modulation, public, compiled]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso: []
llmRef: |
  routing.public_mod

  Exposes a value as a modulation output on the parent network. Used inside nested or compiled networks to create custom modulation sources. The Value parameter is written to a shared modulation output that the parent network can read. Not in the audio signal path.

  Signal flow:
    Control node - no audio processing. Value parameter -> parent modulation output.

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). The value to expose as modulation output.

  When to use:
    Rarely used (rank 75, 3 instances). Required when compiling a DSP network as a custom modulation source. Connect any internal modulation signal to the Value parameter, then compile the network. The compiled node will show a modulation dragger with this output.

  See also:
    None.
---

Exposes a value as a modulation output on the parent network. This node is used inside compiled or nested DSP networks to create custom modulation sources. It sits outside the audio signal path -- all audio processing callbacks are empty.

Connect any modulation source inside the network to the Value parameter. When the network is compiled and loaded as a `project.*` node, it will display a modulation dragger that outputs whatever value is sent to this node internally. Only one public_mod node per network is supported. The value update is immediate -- there is no buffering or smoothing between the Value parameter changing and the modulation output updating. The output only fires a change notification when the value actually differs from the previous one.

This node is only useful inside networks that will be compiled or nested. In a top-level network it has no effect since there is no parent to receive the modulation output.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "The value to expose as modulation output on the parent network"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    writeToModOutput:
      desc: "Writes the value to the parent network's modulation output"
---

```
// routing.public_mod - modulation output bridge
// control value -> parent modulation output

onValueChange(Value) {
    writeToModOutput(Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "The value to expose as the network's public modulation output. Connect a modulation source to this parameter.", range: "0.0 - 1.0", default: "0.0" }
---
::

