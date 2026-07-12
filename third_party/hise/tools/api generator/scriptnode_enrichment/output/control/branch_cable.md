---
title: Branch Cable
description: "Routes a single input value to one of multiple output slots selected by an index parameter."
factoryPath: control.branch_cable
factory: control
polyphonic: true
tags: [control, routing, demultiplexer]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.xfader", type: alternative, reason: "Distributes fade coefficients across all outputs instead of routing to one" }
  - { id: "control.input_toggle", type: disambiguation, reason: "Selects one of two inputs rather than routing one value to many outputs" }
commonMistakes:
  - title: "Unselected outputs retain their last value"
    wrong: "Expecting unselected outputs to reset to zero when the Index changes"
    right: "Unselected outputs keep the last value they received. Send an explicit zero before switching if a reset is needed."
    explanation: "When the Index changes, only the newly selected output receives the current value. Other outputs are not cleared -- they hold whatever value was last routed to them."
llmRef: |
  control.branch_cable

  Routes a single input value to one of multiple output slots based on the Index parameter. A demultiplexer for control signals. Output is normalised.

  Signal flow:
    Control node -- no audio processing
    Index (integer) selects output slot -> Value is sent to slot[Index] only

  CPU: negligible, polyphonic

  Parameters:
    Index: 0 - 7 (default 0, step 1). Selects which output slot receives the value.
    Value: 0.0 - 1.0 (default 0.0). The value to route to the selected output.

  Properties:
    NumParameters: Number of output slots (configure in the node UI).

  When to use:
    Routing a control signal to different targets based on a selector. Unused in surveyed projects but valuable for dynamic parameter routing in branching network designs.

  Common mistakes:
    Unselected outputs retain their last received value -- they are not reset to zero.

  See also:
    [alternative] control.xfader -- distributes fade coefficients across all outputs
    [disambiguation] control.input_toggle -- selects one of two inputs
---

Routes a single control value to one of multiple output slots selected by the Index parameter. Only the output slot matching the current Index receives the value -- all other outputs retain whatever value they last received. This acts as a demultiplexer for control signals, directing one modulation source to different targets based on a selector.

The number of output slots is configured via the NumParameters property in the node UI. When the Index changes, the current value is immediately sent to the newly selected output. Out-of-range index values are silently ignored.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Index:
      desc: "Selects which output slot receives the value"
      range: "0 - 7"
      default: "0"
    Value:
      desc: "The value routed to the selected output"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    routeToSlot:
      desc: "Sends the value to the output slot at the given index"
---

```
// control.branch_cable - value demultiplexer
// control in -> one of N outputs

onValueChange(Index, Value) {
    if (Index < NumParameters)
        routeToSlot(Index, Value)   // only slot[Index] receives the value
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Routing
    params:
      - { name: Index, desc: "Selects which output slot receives the value. Out-of-range values are ignored.", range: "0 - 7", default: "0" }
  - label: Signal
    params:
      - { name: Value, desc: "The value to route to the selected output slot.", range: "0.0 - 1.0", default: "0.0" }
---
::

Each voice can route to a different output slot independently in polyphonic mode. Changing the Index immediately re-sends the current value to the newly selected output, so the target always receives an up-to-date value without waiting for the next Value change.

**See also:** $SN.control.xfader$ -- distributes fade coefficients across all outputs, $SN.control.input_toggle$ -- selects one of two inputs
