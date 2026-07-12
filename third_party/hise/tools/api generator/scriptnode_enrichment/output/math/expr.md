---
title: Expression
description: "A programmable math node that evaluates a user-defined SNEX expression for each audio sample."
factoryPath: math.expr
factory: math
polyphonic: true
tags: [math, expr, expression, snex, waveshaper, formula]
cpuProfile:
  baseline: low
  polyphonic: true
  scalingFactors:
    - { parameter: "Code", impact: "variable", note: "CPU depends on the complexity of the expression" }
seeAlso:
  - { id: "math.table", type: alternative, reason: "Visual waveshaping via an editable curve instead of a formula" }
  - { id: "control.cable_expr", type: companion, reason: "Expression node for control cable values instead of audio" }
  - { id: "Math", type: api, reason: "HiseScript Math class provides the same mathematical functions available in script" }
commonMistakes:
  - title: "Type mismatch in math function calls"
    wrong: "Writing Math.min(input, 3.0) where arguments have mixed float/double types"
    right: "Use explicit float literals: Math.min(input, 3.0f) so both arguments match."
    explanation: "The SNEX compiler requires both arguments to be the same type. Since input is a float, numeric literals must use the f suffix (e.g. 3.0f) or an explicit cast."
  - title: "Forgetting that value is the parameter name"
    wrong: "Trying to use a custom variable name or declaring variables in the expression"
    right: "Use input for the audio sample and value for the parameter. No variable declarations are allowed."
    explanation: "The expression is a single line with two predefined variables: input (the current sample) and value (the Value parameter). Variable declarations and multi-line code are not supported."
llmRef: |
  math.expr

  Programmable per-sample expression node. The user writes a single-line SNEX formula that receives input (audio sample) and value (parameter). JIT-compiled at edit time.

  Signal flow:
    audio in -> SNEX expression(input, value) -> audio out

  CPU: low (baseline), variable depending on expression complexity, polyphonic

  Parameters:
    Value: 0.0 - 1.0 (default 1.0). Passed as the second argument to the expression.

  Properties:
    Code: The SNEX expression (default "input", passthrough).
    Debug: Toggles debug output for inspecting values or compiler warnings.

  When to use:
    Quick custom waveshaping or math operations that are not covered by the built-in math nodes. Prefer built-in nodes when available (they may use vectorised operations). For stateful processing or multi-line code, use dedicated SNEX nodes.

  Common mistakes:
    Use f suffix for float literals (3.0f not 3.0) to avoid type mismatch errors.
    Only input and value are available - no variable declarations.

  See also:
    [alternative] math.table - visual waveshaping curve
    [companion] control.cable_expr - expression node for control cables
    [api] Math -- HiseScript Math class with the same mathematical functions
---

A programmable math node that evaluates a user-defined SNEX expression for every audio sample. The expression receives two variables: `input` (the current audio sample) and `value` (the Value parameter), and the result replaces the sample. This makes it a general-purpose tool for custom waveshaping, signal transformation, or any per-sample math operation that is not covered by the built-in math nodes.

The expression is JIT-compiled when edited, so there is no interpretation overhead at runtime. However, it runs per-sample without vectorised operations, so the built-in math nodes (which can use vectorised processing) may be more efficient for simple operations like addition or multiplication. For stateful processing or multi-line code, use a dedicated SNEX node instead. A related variant, [control.cable_expr]($SN.control.cable_expr$), evaluates expressions on control cable values rather than audio samples.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Second argument passed to the expression"
      range: "0.0 - 1.0"
      default: "1.0"
  functions:
    expression:
      desc: "User-defined SNEX formula evaluated per sample"
---

```
// math.expr - user-defined expression
// audio in -> audio out

process(input) {
    output = expression(input, Value)
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "An additional input value available inside the expression as value. Useful for adding a controllable parameter to the formula.", range: "0.0 - 1.0", default: "1.0" }
---
::

## Writing Expressions

The expression is a single line of SNEX code. The two predefined variables are `input` (the current audio sample, a float) and `value` (the Value parameter, also a float). The result of the expression becomes the output sample.

Standard math operators (`+`, `-`, `*`, `/`), comparison operators (`>`, `==`, `>=`, `!=`), and the ternary operator (`x ? y : z`) are all available. The full `Math` function library mirrors the [Math]($API.Math$) scripting API: `Math.sin()`, `Math.tanh()`, `Math.abs()`, `Math.range()`, `Math.min()`, `Math.max()`, and so on.

### Examples

```javascript
// clipper (same as math.clip)
output = Math.range(input, -1.0f * value, 1.0f * value)

// sig2mod (convert bipolar to unipolar)
output = 0.5f * input + 0.5f

// sine waveshaper with dry/wet mix
output = (1.0f - value) * input + value * Math.sin(3.14159f * 2.0f * value * input)

// rectifier
output = input > 0.0f ? value : -1.0f * value
```

### Value Types

SNEX is strictly typed. The `input` and `value` variables are single-precision floats. Numeric literals default to double precision, so use the `f` suffix (e.g. `3.0f`) or an explicit cast `(float)3.0` to avoid type mismatch errors. The compiler shows a yellow warning state for implicit casts and a red error state for ambiguous function calls.

> [!Tip:Use the Debug button to diagnose issues] Click the debug button in the node interface to see compiler warnings (yellow state) or input/output value pairs (green state). This helps verify that the expression behaves as expected.

### Properties

- **Code** -- The SNEX expression to evaluate. Defaults to `input` (passthrough).
- **Debug** -- Toggles debug mode. When enabled, shows compiler warnings or prints input/output value pairs depending on the compilation state.

### Limitations

The expression must compile to a valid single-line formula. Variable declarations and multi-statement code are not supported. If you need more complex processing, consider using a dedicated SNEX node for full control over the processing callback.

When compiling a network to a hardcoded effect for export, the expression is baked in at compile time. This means the Code property cannot be changed at runtime in exported plugins.

**See also:** $SN.math.table$ -- visual waveshaping via an editable curve, $SN.control.cable_expr$ -- expression node for control cable values, $API.Math$ -- HiseScript Math class with the same mathematical functions
