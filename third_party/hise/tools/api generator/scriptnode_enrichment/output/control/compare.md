---
title: Compare
description: "Compares two input signals using a selectable comparator and outputs either a binary 1/0 result or the min/max of the two values."
factoryPath: control.compare
factory: control
polyphonic: true
tags: [control, compare, logic, threshold, min, max]
cpuProfile:
  baseline: negligible
  polyphonic: true
  scalingFactors: []
seeAlso:
  - { id: "control.logic_op", type: companion, reason: "Combines binary signals using AND, OR, XOR logic" }
commonMistakes:
  - title: "MIN and MAX return continuous values"
    wrong: "Using the MIN or MAX comparator and expecting a binary 0 or 1 output"
    right: "MIN and MAX return the smaller or larger of the two input values, which can be any value in the 0-1 range."
    explanation: "Only the six comparison operators (EQ, NEQ, GT, LT, GTE, LTE) produce strict binary output. MIN and MAX return continuous values."
llmRef: |
  control.compare

  Compares two input signals and outputs a result based on the selected comparator. Six comparison modes produce binary 1.0/0.0 output; MIN and MAX return continuous values.

  Signal flow:
    Control node - no audio processing
    Left + Right -> Comparator operation -> modulation out (normalised)

  CPU: negligible, polyphonic

  Parameters:
    Left (0.0 - 1.0, default 0.0): first operand
    Right (0.0 - 1.0, default 0.0): second operand
    Comparator (EQ/NEQ/GT/LT/GTE/LTE/MIN/MAX, default EQ): comparison operation

  When to use:
    Use for conditional logic in control networks -- triggering events when a value crosses a threshold, selecting the minimum or maximum of two signals, or creating binary gates from continuous values.

  Common mistakes:
    MIN and MAX return continuous values, not binary 0/1.

  See also:
    [companion] control.logic_op -- combines binary signals with AND, OR, XOR
---

Compares two input signals using a selectable comparator and sends the result as a modulation output. Six comparison modes (EQ, NEQ, GT, LT, GTE, LTE) produce a strict binary output of 1.0 or 0.0. The MIN and MAX modes instead return the smaller or larger of the two input values as a continuous result.

The comparison uses exact floating-point equality with no tolerance, so the EQ and NEQ modes require the two values to be bit-identical to match. Output is only sent when one of the input values actually changes.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Left:
      desc: "First comparison operand"
      range: "0.0 - 1.0"
      default: "0.0"
    Right:
      desc: "Second comparison operand"
      range: "0.0 - 1.0"
      default: "0.0"
    Comparator:
      desc: "Selects the comparison operation"
      range: "EQ / NEQ / GT / LT / GTE / LTE / MIN / MAX"
      default: "EQ"
  functions:
    compare:
      desc: "Applies the selected comparison and returns the result"
---

```
// control.compare - comparison with selectable operator
// control in -> control out

onValueChange(input) {
    if (Comparator == EQ)  output = (Left == Right) ? 1.0 : 0.0
    if (Comparator == NEQ) output = (Left != Right) ? 1.0 : 0.0
    if (Comparator == GT)  output = (Left > Right) ? 1.0 : 0.0
    if (Comparator == LT)  output = (Left < Right) ? 1.0 : 0.0
    if (Comparator == GTE) output = (Left >= Right) ? 1.0 : 0.0
    if (Comparator == LTE) output = (Left <= Right) ? 1.0 : 0.0
    if (Comparator == MIN) output = min(Left, Right)
    if (Comparator == MAX) output = max(Left, Right)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Operands
    params:
      - { name: Left, desc: "First comparison operand.", range: "0.0 - 1.0", default: "0.0" }
      - { name: Right, desc: "Second comparison operand.", range: "0.0 - 1.0", default: "0.0" }
  - label: Operation
    params:
      - { name: Comparator, desc: "Selects the comparison operation. EQ through LTE produce binary output; MIN and MAX return continuous values.", range: "EQ / NEQ / GT / LT / GTE / LTE / MIN / MAX", default: "EQ" }
---
::

Output is only sent when a Left or Right value actually changes, or when the Comparator selection changes. Repeated identical values on either input are suppressed.

**See also:** $SN.control.logic_op$ -- combines binary signals using AND, OR, XOR logic
