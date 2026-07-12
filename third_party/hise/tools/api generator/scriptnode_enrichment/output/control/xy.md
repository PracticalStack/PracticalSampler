---
title: XY Controller
description: "A two-axis controller that routes X and Y values to separate output targets."
factoryPath: control.xy
factory: control
polyphonic: false
tags: [control, xy, multi-output, ui]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.sliderbank", type: alternative, reason: "Multi-output control with more than two targets" }
llmRef: |
  control.xy

  A two-axis controller with separate X and Y outputs. Each axis passes its value directly to a dedicated output slot with no transformation.

  Signal flow:
    Control node - no audio processing
    X -> output slot 0 (unipolar)
    Y -> output slot 1 (bipolar)

  CPU: negligible, monophonic

  Parameters:
    X (0.0 - 1.0, default 0.0): Horizontal axis value, routed to output slot 0
    Y (-1.0 - 1.0, default 0.0): Vertical axis value (bipolar), routed to output slot 1

  When to use:
    Use for two-dimensional parameter control such as filter cutoff/resonance, pan/width, or any pair of parameters that benefit from simultaneous XY manipulation.

  See also:
    [alternative] control.sliderbank -- multi-output control with more than two targets
---

XY Controller provides a two-dimensional control surface with separate X and Y outputs. Each axis value passes directly to its own output slot with no transformation, making it straightforward to control two related parameters simultaneously - for example, filter cutoff and resonance, or pan position and stereo width.

The X axis uses a standard unipolar range (0 to 1) while the Y axis uses a bipolar range (-1 to 1), following common XY pad conventions.

## Signal Path

::signal-path
---
glossary:
  parameters:
    X:
      desc: "Horizontal axis value routed to output slot 0"
      range: "0.0 - 1.0"
      default: "0.0"
    Y:
      desc: "Vertical axis value routed to output slot 1"
      range: "-1.0 - 1.0"
      default: "0.0"
---

```
// control.xy - two-axis controller
// X,Y parameters -> two separate control outputs

onValueChange() {
    output[0] = X    // unipolar
    output[1] = Y    // bipolar
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: ""
    params:
      - { name: X, desc: "Horizontal axis value. Routed directly to output slot 0 (unipolar).", range: "0.0 - 1.0", default: "0.0" }
      - { name: Y, desc: "Vertical axis value. Routed directly to output slot 1 (bipolar).", range: "-1.0 - 1.0", default: "0.0" }
---
::

The two outputs are named X and Y in the connection list, making it clear which axis drives which target. Each output can be connected to any parameter independently.

**See also:** $SN.control.sliderbank$ -- multi-output control with more than two targets
