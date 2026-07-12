---
title: Logic Op
description: "Combines two binary input signals using a selectable logic operator (AND, OR, XOR) and outputs 1.0 or 0.0."
factoryPath: control.logic_op
factory: control
polyphonic: true
tags: [control, logic, boolean, and, or, xor, gate]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.compare", type: companion, reason: "Produces binary signals from value comparisons that can feed into logic_op" }
commonMistakes:
  - title: "No output until both inputs are set"
    wrong: "Connecting only one input and expecting an output immediately"
    right: "Both Left and Right must receive at least one value before any output is produced."
    explanation: "After a voice reset, both inputs are in an undefined state. The node waits until both have received a value before evaluating the logic operation and sending output."
llmRef: |
  control.logic_op

  Combines two binary input signals using a selectable logic operator. Inputs are converted to boolean via a > 0.5 threshold. Output is strictly 1.0 or 0.0.

  Signal flow:
    Control node - no audio processing
    Left (> 0.5 = true) + Right (> 0.5 = true) -> Operator (AND/OR/XOR) -> modulation out (1.0 or 0.0)

  CPU: negligible, polyphonic

  Parameters:
    Left (0.0 - 1.0, default 0.0): first logic input, thresholded at 0.5
    Right (0.0 - 1.0, default 0.0): second logic input, thresholded at 0.5
    Operator (AND/OR/XOR, default AND): logic operation

  When to use:
    Use for combining binary conditions in control networks. Feed the outputs of compare nodes or toggle switches into Left and Right to create compound conditions.

  Common mistakes:
    No output until both inputs have received a value after reset.

  See also:
    [companion] control.compare -- produces binary signals from value comparisons
---

Combines two binary input signals using a selectable logic operator and outputs 1.0 (true) or 0.0 (false). Each input is converted to a boolean value using a threshold: values above 0.5 are treated as true, values at or below 0.5 as false.

The three available operators are AND (both must be true), OR (at least one must be true), and XOR (exactly one must be true). This node is typically used to combine the binary outputs of [control.compare]($SN.control.compare$) nodes or toggle switches to create compound conditions in a control network.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Left:
      desc: "First logic input, converted to boolean via > 0.5 threshold"
      range: "0.0 - 1.0"
      default: "0.0"
    Right:
      desc: "Second logic input, converted to boolean via > 0.5 threshold"
      range: "0.0 - 1.0"
      default: "0.0"
    Operator:
      desc: "Selects the logic operation"
      range: "AND / OR / XOR"
      default: "AND"
  functions:
    threshold:
      desc: "Converts a 0-1 value to boolean: true if > 0.5, false otherwise"
---

```
// control.logic_op - boolean logic on two inputs
// control in -> control out (binary)

onValueChange(input) {
    left  = threshold(Left)    // > 0.5 = true
    right = threshold(Right)

    if (Operator == AND) output = (left AND right) ? 1.0 : 0.0
    if (Operator == OR)  output = (left OR right) ? 1.0 : 0.0
    if (Operator == XOR) output = (left XOR right) ? 1.0 : 0.0
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Inputs
    params:
      - { name: Left, desc: "First logic input. Values above 0.5 are treated as true.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Right, desc: "Second logic input. Values above 0.5 are treated as true.", range: "0.0 - 1.0", default: "0.0" }
  - label: Operation
    params:
      - { name: Operator, desc: "Selects the logic operation applied to the two boolean inputs.", range: "AND / OR / XOR", default: "AND" }
---
::

After a voice reset, both inputs are in an undefined state. The node waits until both Left and Right have each received at least one value before producing any output, preventing spurious results when only one input has been initialised. Changes to either input or to the Operator selection trigger a new output evaluation, provided both inputs have been defined.

**See also:** $SN.control.compare$ -- produces binary signals from value comparisons
