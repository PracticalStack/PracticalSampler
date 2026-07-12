# control.cable_expr - C++ Exploration

**Source:** `hi_dsp_library/dsp_nodes/CableNodes.h:1066` (cable_expr class)
**Base class:** `mothernode`, `parameter_node_base<ParameterClass>`, `no_mod_normalisation`, `no_processing`
**Classification:** control_source

## Signal Path

The cable_expr node transforms a control value using a SNEX expression before forwarding it to the connected parameter. The expression is compiled via JIT and receives the input as the `input` variable.

Value parameter (raw) -> ExpressionClass::op(input) -> transformed value -> output (unnormalised)

## Gap Answers

### expr-available-functions: What functions/variables are available in the expression?

The expression is wrapped as `double get(double input){ return <EXPRESSION>; }` (confirmed in snex-overview.md). Available:
- `input` variable: the incoming Value parameter
- `Math.*` functions: all hmath functions (sin, cos, pow, abs, etc.) -- these are converted to `hmath::` for C++ export
- Standard arithmetic: +, -, *, /, ternary operator
- Type casts: `(double)8`, `(float)x`
- No additional variables beyond `input`. Unlike math.expr which has both `input` and `value`, cable_expr only has `input`.

### expr-compilation-failure: What happens when the expression fails to compile?

Passthrough. From the SNEX infrastructure (snex-overview.md): "If the expression fails to compile, `JitExpression::getValue()` returns the input value unchanged (passthrough)." The `isValid()` and `getErrorMessage()` methods allow checking compilation status.

### expr-debug-property: What does the Debug property do?

The Debug property is not visible in the C++ cable_expr class itself (lines 1066-1097). The `SN_DEFAULT_INIT(ExpressionClass)` macro (line 1085) forwards initialise() to the ExpressionClass, which is the JIT expression wrapper. The Debug property is likely handled by the JitExpression class in the scripting layer, where it enables console output of expression results during evaluation. From the cable_expr C++ alone, Debug is not referenced -- it is a property of the expression runtime system.

### expr-compilation-requirement: Is compilation required for exported plugins?

Yes. cable_expr uses `ExpressionClass` as a template parameter. In the interpreted path, this is a JIT-compiled class. For C++ export, the expression string is converted to valid C++ via `JitExpression::convertToValidCpp()` (replaces `Math.` with `hmath::`). The expression becomes a compile-time `op()` method in the exported C++ code. The node cannot work in an exported plugin without compilation because the JIT engine is not available at runtime in exported builds.

## Parameters

- **Value** (single parameter via SN_ADD_SET_VALUE): Input to the expression. Range 0..1, default 0.0. Marked as unscaled input via `no_mod_normalisation(getStaticId(), { "Value" })` (line 1082).

## Properties

- **Code**: The SNEX expression string. Default: "input" (passthrough).
- **Debug**: Boolean. Enables debug output for the expression.

## CPU Assessment

baseline: negligible
polyphonic: false
scalingFactors: []

## Notes

The cable_expr node stores `lastValue` (line 1096) but does not actually use it in `setValue()`. The `setValue()` method (line 1087-1093) calls `obj.op(input)` and forwards the result directly. The `lastValue` member appears to be vestigial.
