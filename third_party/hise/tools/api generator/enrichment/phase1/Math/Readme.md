# Math -- Class Analysis

## Brief
Standard math namespace with trigonometry, rounding, clamping, range conversion, and numeric utilities.

## Purpose
The Math object is a globally available utility namespace providing standard mathematical functions for HiseScript. It covers trigonometric functions (sin, cos, tan and their inverses/hyperbolic variants), logarithmic and exponential functions, rounding and truncation, value clamping and wrapping, random number generation, and normalised range conversion via JSON range objects. Most methods (37 of 39) are marked as inlineable for JIT compiler optimization. Several methods (abs, round, sign, range, min, max) preserve integer types when given integer inputs.

## obtainedVia
Global namespace -- always available as `Math`.

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| PI | 3.141592653589793 | double | Pi constant | math-constants |
| E | 2.718281828459045 | double | Euler's number | math-constants |
| SQRT2 | 1.4142135623730951 | double | Square root of 2 | math-constants |
| SQRT1_2 | 0.7071067811865476 | double | Square root of 0.5 (1/sqrt(2)) | math-constants |
| LN2 | 0.6931471805599453 | double | Natural logarithm of 2 | math-constants |
| LN10 | 2.302585092994046 | double | Natural logarithm of 10 | math-constants |
| LOG2E | 1.4426950408889634 | double | Base-2 logarithm of E | math-constants |
| LOG10E | 0.4342944819032518 | double | Base-10 logarithm of E | math-constants |

## Dynamic Constants
None.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Math.from0To1(0.5, {"min": 20, "max": 20000})` | `Math.from0To1(0.5, {"min": 20, "max": 20000, "middlePosition": 1000})` | Without middlePosition or skew, from0To1 uses linear mapping. For frequency ranges, a skewed mapping is almost always needed. |

## codeExample
```javascript
// Math is a global namespace, no instantiation needed
var freq = Math.from0To1(0.5, {"MinValue": 20.0, "MaxValue": 20000.0, "SkewFactor": 0.3});
var clamped = Math.range(freq, 20.0, 20000.0);
var radians = Math.toRadians(180.0); // Math.PI
```

## Alternatives
None.

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods are pure stateless functions with no timeline dependencies, preconditions, or silent-failure modes that would benefit from parse-time diagnostics.
