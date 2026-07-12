---
title: Cable Expression
description: "Transforms a control value using a custom SNEX expression before forwarding it to the modulation output."
factoryPath: control.cable_expr
factory: control
polyphonic: false
tags: [control, expression, snex, transform]
cpuProfile:
  baseline: negligible
  polyphonic: false
  scalingFactors: []
seeAlso:
  - { id: "control.cable_table", type: alternative, reason: "Visual lookup table for reshaping -- no compilation required" }
  - { id: "control.pma", type: disambiguation, reason: "Fixed multiply-add transform without custom expressions" }
commonMistakes:
  - title: "Network must be compiled for export"
    wrong: "Using cable_expr in an uncompiled network and exporting the plugin"
    right: "Compile the network to C++ before exporting. The expression is JIT-compiled at design time but requires C++ compilation for exported plugins."
    explanation: "The expression is evaluated via JIT compilation in the HISE IDE. Exported plugins do not include the JIT engine, so the network must be compiled to C++ where the expression becomes a native function."
llmRef: |
  control.cable_expr

  Transforms a control value using a custom SNEX expression. The expression receives the input as the `input` variable and can use Math.* functions. Output is unnormalised.

  Signal flow:
    Control node -- no audio processing
    Value param -> SNEX expression evaluation -> modulation output

  CPU: negligible, monophonic

  Parameters:
    Value: 0.0 - 1.0 (default 0.0). Input passed as `input` to the expression. Unnormalised input.

  Properties:
    Code: SNEX expression string (default "input" -- passthrough).
    Debug: Boolean. Enables debug output.

  When to use:
    Custom mathematical transformations of control signals. 57 instances across surveyed projects (rank 5). The most frequently used cable transform node. Use for scaling, clamping, or any formula that cannot be expressed with built-in nodes.

  Common mistakes:
    Requires C++ compilation for exported plugins -- the JIT engine is not available at runtime.

  See also:
    [alternative] control.cable_table -- visual lookup table, no compilation needed
    [disambiguation] control.pma -- fixed multiply-add transform
---

Transforms a control value using a custom SNEX expression. The expression receives the incoming value as the `input` variable and returns the transformed result. Any `Math.*` function (sin, cos, pow, abs, etc.) and standard arithmetic operators are available in the expression. The output is unnormalised, forwarding the raw expression result without range conversion.

This is the most frequently used cable transform node in surveyed projects (57 instances, rank 5). It handles custom scaling, clamping, or any mathematical transformation that cannot be expressed with the built-in control nodes.

## Signal Path

::signal-path
---
glossary:
  parameters:
    Value:
      desc: "Input value passed as 'input' to the expression (unnormalised)"
      range: "0.0 - 1.0"
      default: "0.0"
  functions:
    evaluate:
      desc: "Evaluates the SNEX expression with the current input value"
---

```
// control.cable_expr - custom expression transform
// control in -> expression -> control out

onValueChange(Value) {
    output = evaluate(Value)    // e.g. "Math.pow(input, 2.0)"
}
```

::

## Parameters

::parameter-table
---
groups:
  - label: Signal
    params:
      - { name: Value, desc: "Input to the expression. Available as the `input` variable. Receives raw values without range conversion.", range: "0.0 - 1.0", default: "0.0" }
---
::

### Expression Syntax

The Code property accepts any valid SNEX expression. Internally, the expression is wrapped as:

```
double get(double input) { return <your expression>; }
```

For example, setting Code to `Math.pow(input, 2.0)` applies a quadratic curve. Setting it to `input > 0.5 ? 1.0 : 0.0` creates a threshold gate.

If the expression fails to compile, the node falls back to passthrough behaviour -- the input value is forwarded unchanged. The Debug property enables console output of expression results during development.

> [!Warning:Compilation required for export] Networks containing cable_expr must be compiled to C++ before exporting as a plugin. The JIT expression engine is only available in the HISE IDE.

**See also:** $SN.control.cable_table$ -- visual lookup table without compilation, $SN.control.pma$ -- fixed multiply-add transform
