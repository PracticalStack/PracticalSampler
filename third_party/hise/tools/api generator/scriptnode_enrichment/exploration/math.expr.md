# math.expr - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/MathNodes.h:786-805`
**Base class:** `OpNode<expression_base<ExpressionClass>, NV>` -> `OpNodeBase` -> `mothernode, polyphonic_base`
**Classification:** audio_processor

## Signal Path

User-defined per-sample expression processor. For each sample across all channels:
1. The SNEX expression's `ExpressionClass::op(float input, float value)` is called.
2. `input` is the current sample value; `value` is the Value parameter.
3. The return value replaces the sample.

The `expression_base` struct provides both block and frame paths, but both delegate to per-sample `ExpressionClass::op()` calls. There is no SIMD block path -- the block `op()` method iterates channels and samples calling the expression per-sample (OP_BLOCK2SINGLE pattern).

## Gap Answers

### available-variables-and-functions: What variables and functions are available inside the SNEX expression?

The expression is compiled as an `ExpressionClass` with a static `op(float input, float value)` method. Inside the expression, `input` is the current audio sample and `value` is the Value parameter. The full `hmath` function library is available (sin, cos, tanh, pow, sqrt, abs, etc.) since the expression is JIT-compiled SNEX code.

### value-parameter-interaction: How does the Value parameter interact with the expression?

The Value parameter is passed as the second argument to `ExpressionClass::op(input, value)`. Since math.expr uses `OpNode<expression_base, NV>`, the Value parameter is stored in `PolyData<float, NV>` and is per-voice when running polyphonically.

### debug-property-function: What does the Debug property do?

The Debug property is handled entirely by the SNEX JIT layer, not by the DSP code in MathNodes.h. It is not visible in the `expression_base` struct. Its behaviour is determined by the ExpressionClass compilation pipeline.

### compilation-timing: When does the expression get compiled?

Compilation is handled by the SNEX layer when the Code property changes. The `expression_base` struct itself is stateless -- it simply forwards to `ExpressionClass::op()`. The DSP code does not manage compilation timing or error handling.

### processing-model: Per-sample only or block path too?

Per-sample only. The `expression_base::op()` block method iterates all channels and samples, calling `ExpressionClass::op(s, value)` for each sample individually. The `expression_base::opSingle()` frame method iterates the frame samples the same way. There is no vectorised block path.

### expression-syntax: What is the exact syntax?

The expression must resolve to a static `op(float input, float value) -> float` function. The Code property value is compiled by the SNEX JIT compiler. The default Code value "input" passes the signal through unchanged. Standard C-like math syntax applies. The exact syntax rules are governed by the SNEX compiler, not the DSP code.

## Parameters

- **Value** (default 1.0, range [0, 1]): Passed as the second argument to the expression's `op(input, value)` function. Per-voice in polyphonic context.

## Polyphonic Behaviour

The node is polyphonic via `OpNode<..., NV>`. The Value parameter is stored in `PolyData<float, NV>`, providing independent values per voice. The expression itself is stateless (static `op()` method), so there is no per-voice expression state beyond the Value parameter.

## CPU Assessment

baseline: low
polyphonic: true
scalingFactors: [{"parameter": "Code", "impact": "variable", "note": "CPU depends entirely on the complexity of the user-supplied expression"}]

## Notes

The `expression_base` is a thin adapter that wraps a user-supplied `ExpressionClass` into the OpNode framework. All the interesting behaviour (compilation, syntax, available functions) is in the SNEX JIT layer, not in MathNodes.h.
