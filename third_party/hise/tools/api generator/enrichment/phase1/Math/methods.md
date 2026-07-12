# Math -- Method Documentation

## abs

**Signature:** `Number abs(Number value)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.abs(-5);`

**Description:**
Returns the absolute (unsigned) value. Preserves integer type: if the input is an integer, the result is an integer; otherwise it returns a double.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to make absolute | -- |

**Pitfalls:**
None.

---

## acos

**Signature:** `Double acos(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var angle = Math.acos(0.5);`

**Description:**
Calculates the arc cosine (inverse cosine) of the value. Returns the angle in radians in the range [0, PI].

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The cosine value | -1.0 to 1.0 |

**Pitfalls:**
- Values outside [-1, 1] return NaN.

**Cross References:**
- `$API.Math.cos$`
- `$API.Math.acosh$`

---

## acosh

**Signature:** `Double acosh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.acosh(2.0);`

**Description:**
Calculates the inverse hyperbolic cosine of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | >= 1.0 |

**Pitfalls:**
- Values less than 1.0 return NaN.

**Cross References:**
- `$API.Math.acos$`
- `$API.Math.cosh$`

---

## asin

**Signature:** `Double asin(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var angle = Math.asin(0.5);`

**Description:**
Calculates the arc sine (inverse sine) of the value. Returns the angle in radians in the range [-PI/2, PI/2].

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The sine value | -1.0 to 1.0 |

**Pitfalls:**
- Values outside [-1, 1] return NaN.

**Cross References:**
- `$API.Math.sin$`
- `$API.Math.asinh$`

---

## asinh

**Signature:** `Double asinh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.asinh(1.0);`

**Description:**
Calculates the inverse hyperbolic sine of the value. Accepts any real number.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.asin$`
- `$API.Math.sinh$`

---

## fmod

**Signature:** `Double fmod(Number value, Number limit)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.fmod(5.5, 2.0);`

**Description:**
Returns the floating-point remainder when dividing value by limit. The result has the same sign as the dividend (value). Delegates to `std::fmod`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The dividend | -- |
| limit | Number | no | The divisor | Non-zero |

**Pitfalls:**
- Returns NaN when limit is zero. Unlike integer division, no error is thrown.

**Cross References:**
- `$API.Math.wrap$`

---

## from0To1

**Signature:** `Double from0To1(Number value, Object rangeObj)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.from0To1(0.5, {"MinValue": 20.0, "MaxValue": 20000.0, "SkewFactor": 0.3});`

**Description:**
Converts a normalised value (0.0-1.0) to a value within the range defined by rangeObj. Supports three JSON naming conventions for the range object: scriptnode (`MinValue`, `MaxValue`, `SkewFactor`, `StepSize`), UI Component (`min`, `max`, `middlePosition`, `stepSize`), and MIDI Automation (`Start`, `End`, `Skew`, `Interval`). Also accepts scriptnode fix objects with matching type layouts. The `Inverted` property is supported across all conventions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The normalised value | 0.0 to 1.0 |
| rangeObj | Object | no | A JSON object or fix object defining the target range | Must use a recognized naming convention |

**Pitfalls:**
- The UI Component convention uses `middlePosition` (converted to a skew factor internally via `setSkewForCentre`) while the scriptnode convention uses `SkewFactor` directly. These are different values for the same audible curve.

**Cross References:**
- `$API.Math.to0To1$`
- `$API.Math.skew$`

**Example:**
```javascript:from0to1-range-conventions
// Title: Converting normalised values with different range conventions
// Scriptnode convention
var freq = Math.from0To1(0.5, {
    "MinValue": 20.0,
    "MaxValue": 20000.0,
    "SkewFactor": 0.3
});

// UI Component convention (middlePosition = centre frequency)
var freq2 = Math.from0To1(0.5, {
    "min": 20.0,
    "max": 20000.0,
    "middlePosition": 1000.0
});

Console.print(freq);
Console.print(freq2);
```
```json:testMetadata:from0to1-range-conventions
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "freq > 20.0 && freq < 3000.0", "value": true},
    {"type": "REPL", "expression": "Math.abs(freq2 - 1000.0) < 1.0", "value": true}
  ]
}
```

---

## isinf

**Signature:** `Integer isinf(Number value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = Math.isinf(1.0 / 0.0);`

**Description:**
Checks whether the value is infinite. Returns 1 (true) if the value is positive or negative infinity, 0 (false) otherwise.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to check | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.isnan$`
- `$API.Math.sanitize$`

---

## isnan

**Signature:** `Integer isnan(Number value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = Math.isnan(0.0 / 0.0);`

**Description:**
Checks whether the value is NaN (Not a Number). Returns 1 (true) if the value is NaN, 0 (false) otherwise.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to check | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.isinf$`
- `$API.Math.sanitize$`

---

## log

**Signature:** `Double log(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.log(Math.E);`

**Description:**
Calculates the natural logarithm (base e) of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | > 0.0 |

**Pitfalls:**
- Returns -Infinity for zero, NaN for negative values.

**Cross References:**
- `$API.Math.log10$`
- `$API.Math.exp$`

---

## log10

**Signature:** `Double log10(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.log10(1000.0);`

**Description:**
Calculates the base-10 logarithm of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | > 0.0 |

**Pitfalls:**
- Returns -Infinity for zero, NaN for negative values.

**Cross References:**
- `$API.Math.log$`
- `$API.Math.exp$`

---

## max

**Signature:** `Number max(Number first, Number second)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.max(3, 7);`

**Description:**
Returns the larger of two numbers. Preserves integer type when both inputs are integers; returns a double if either input is a double.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| first | Number | no | The first value | -- |
| second | Number | no | The second value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.min$`

---

## min

**Signature:** `Number min(Number first, Number second)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.min(3, 7);`

**Description:**
Returns the smaller of two numbers. Preserves integer type when both inputs are integers; returns a double if either input is a double.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| first | Number | no | The first value | -- |
| second | Number | no | The second value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.max$`

---

## pow

**Signature:** `Double pow(Number base, Number exp)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.pow(2.0, 10.0);`

**Description:**
Calculates the power of base raised to the exponent. Delegates to `std::pow`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| base | Number | no | The base value | -- |
| exp | Number | no | The exponent | -- |

**Pitfalls:**
None.

---

## randInt

**Signature:** `Integer randInt(Number low, Number high)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = Math.randInt(0, 128);`

**Description:**
Returns a random integer in the range [low, high). The upper bound is exclusive. Uses JUCE's thread-local system random.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| low | Number | no | The lower bound (inclusive) | -- |
| high | Number | no | The upper bound (exclusive) | Must be > low |

**Pitfalls:**
- The upper bound is exclusive: `Math.randInt(0, 128)` returns values 0-127, never 128.

**Cross References:**
- `$API.Math.random$`

---

## random

**Signature:** `Double random()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.random();`

**Description:**
Returns a random double between 0.0 (inclusive) and 1.0 (exclusive). Uses JUCE's thread-local system random, so it is safe to call from any thread including the audio thread.

**Parameters:**
None.

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.randInt$`

---

## range

**Signature:** `Number range(Number value, Number lowerLimit, Number upperLimit)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.range(150, 0, 127);`

**Description:**
Limits the value to the given range [lowerLimit, upperLimit]. Preserves integer type when the value is an integer. `Math.clamp()` is an alias for this method.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to limit | -- |
| lowerLimit | Number | no | The lower bound | -- |
| upperLimit | Number | no | The upper bound | Must be >= lowerLimit |

**Pitfalls:**
- Integer type preservation is based only on the value parameter, not the limits. `Math.range(5, 0.0, 10.0)` uses the integer path because 5 is an int, which silently truncates fractional limits.

**Cross References:**
- `$API.Math.clamp$`

---

## round

**Signature:** `Integer round(Number value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = Math.round(2.7);`

**Description:**
Rounds the value to the nearest integer. Always returns an integer (unlike `ceil` and `floor` which return doubles). Uses JUCE's `roundToInt` which rounds half-up.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to round | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.ceil$`
- `$API.Math.floor$`
- `$API.Math.trunc$`

---

## sanitize

**Signature:** `Double sanitize(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sanitize(0.0 / 0.0);`

**Description:**
Replaces NaN and Infinity values with 0.0. Useful for guarding against non-finite values that can propagate through arithmetic chains and corrupt audio output.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to sanitize | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.isnan$`
- `$API.Math.isinf$`

---

## sign

**Signature:** `Number sign(Number value)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sign(-42);`

**Description:**
Returns the sign of the value: 1 for positive, -1 for negative, 0 for zero. Preserves integer type when the input is an integer.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to check | -- |

**Pitfalls:**
None.

---

## sin

**Signature:** `Double sin(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sin(Math.PI / 2.0);`

**Description:**
Calculates the sine of the value (radian based).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The angle in radians | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.asin$`
- `$API.Math.sinh$`

---

## sinh

**Signature:** `Double sinh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sinh(1.0);`

**Description:**
Calculates the hyperbolic sine of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.sin$`
- `$API.Math.asinh$`

---

## skew

**Signature:** `Double skew(Number start, Number end, Number midPoint)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var s = Math.skew(20.0, 20000.0, 1000.0);`

**Description:**
Returns the skew factor for a range where midPoint maps to 0.5 in normalised space. Creates a temporary JUCE `NormalisableRange` and calls `setSkewForCentre()`. The returned skew factor can be used in range objects passed to `Math.from0To1` and `Math.to0To1`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Number | no | The range start value | -- |
| end | Number | no | The range end value | Must be > start |
| midPoint | Number | no | The value that should map to 0.5 normalised | Must be between start and end |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.from0To1$`
- `$API.Math.to0To1$`

---

## smoothstep

**Signature:** `Double smoothstep(Number input, Number lower, Number upper)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.smoothstep(0.3, 0.0, 1.0);`

**Description:**
Calculates a smooth transition between the lower and upper bounds using cubic Hermite interpolation: `t*t*(3-2*t)` where `t = clamp((input-lower)/(upper-lower), 0, 1)`. Returns 0.0 when input <= lower, 1.0 when input >= upper, and a smooth S-curve between. Returns 0.0 when upper <= lower (guard against division by zero).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| input | Number | no | The input value to interpolate | -- |
| lower | Number | no | The lower edge of the transition | -- |
| upper | Number | no | The upper edge of the transition | Must be > lower for meaningful output |

**Pitfalls:**
- Returns 0.0 (not an error) when upper <= lower.

---

## sqr

**Signature:** `Double sqr(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sqr(5.0);`

**Description:**
Calculates the square (x*x) of the value. Always returns a double.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to square | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.sqrt$`

---

## sqrt

**Signature:** `Double sqrt(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.sqrt(16.0);`

**Description:**
Calculates the square root of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value | >= 0.0 |

**Pitfalls:**
- Negative values return NaN.

**Cross References:**
- `$API.Math.sqr$`

---

## tan

**Signature:** `Double tan(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.tan(Math.PI / 4.0);`

**Description:**
Calculates the tangent of the value (radian based).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The angle in radians | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.atan$`
- `$API.Math.tanh$`

---

## tanh

**Signature:** `Double tanh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.tanh(1.0);`

**Description:**
Calculates the hyperbolic tangent of the value. Output is always in the range (-1, 1), making it useful as a soft-clipping function for audio signals.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.tan$`
- `$API.Math.atanh$`

---

## to0To1

**Signature:** `Double to0To1(Number value, Object rangeObj)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var n = Math.to0To1(1000.0, {"MinValue": 20.0, "MaxValue": 20000.0, "SkewFactor": 0.3});`

**Description:**
Converts a value within a range to a normalised value (0.0-1.0). The inverse of `Math.from0To1`. Accepts the same three range object conventions: scriptnode (`MinValue`, `MaxValue`, `SkewFactor`, `StepSize`), UI Component (`min`, `max`, `middlePosition`, `stepSize`), and MIDI Automation (`Start`, `End`, `Skew`, `Interval`). Also accepts scriptnode fix objects. The `Inverted` property is supported across all conventions.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value within the range | -- |
| rangeObj | Object | no | A JSON object or fix object defining the range | Must use a recognized naming convention |

**Pitfalls:**
- The UI Component convention uses `middlePosition` (converted to a skew factor internally via `setSkewForCentre`) while the scriptnode convention uses `SkewFactor` directly. These are different values for the same audible curve.

**Cross References:**
- `$API.Math.from0To1$`
- `$API.Math.skew$`

---

## toDegrees

**Signature:** `Double toDegrees(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var deg = Math.toDegrees(Math.PI);`

**Description:**
Converts an angle from radians to degrees. Uses JUCE's `radiansToDegrees`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The angle in radians | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.toRadians$`

---

## toRadians

**Signature:** `Double toRadians(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var rad = Math.toRadians(180.0);`

**Description:**
Converts an angle from degrees to radians. Uses JUCE's `degreesToRadians`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The angle in degrees | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.toDegrees$`

---

## trunc

**Signature:** `Integer trunc(Number value)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var x = Math.trunc(2.9);`

**Description:**
Truncates the value toward zero by removing the decimal part. Casts to double then to int. Always returns an integer, unlike `floor` which returns a double and rounds toward negative infinity.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to truncate | -- |

**Pitfalls:**
- `trunc` rounds toward zero while `floor` rounds toward negative infinity. For negative values: `Math.trunc(-2.7)` returns -2, `Math.floor(-2.7)` returns -3.0.

**Cross References:**
- `$API.Math.floor$`
- `$API.Math.ceil$`
- `$API.Math.round$`

---

## wrap

**Signature:** `Double wrap(Number value, Number limit)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.wrap(5.5, 4.0);`

**Description:**
Wraps the value around the limit so the result is always in [0, limit). Computed as `fmod(value + limit, limit)`. Useful for cyclic parameters like phase angles or circular buffer indices.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to wrap | -- |
| limit | Number | no | The wrap boundary | > 0.0 |

**Pitfalls:**
- [BUG] For values more negative than `-limit`, the single `value + limit` offset may not bring the result into [0, limit). For example, `Math.wrap(-5.0, 3.0)` computes `fmod(-2.0, 3.0)` which returns -2.0, not the expected 1.0.

**Cross References:**
- `$API.Math.fmod$`


---

## atan

**Signature:** `Double atan(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var angle = Math.atan(1.0);`

**Description:**
Calculates the arc tangent (inverse tangent) of the value. Returns the angle in radians in the range [-PI/2, PI/2].

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The tangent value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.tan$`
- `$API.Math.atanh$`

---

## atanh

**Signature:** `Double atanh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.atanh(0.5);`

**Description:**
Calculates the inverse hyperbolic tangent of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | -1.0 to 1.0 (exclusive) |

**Pitfalls:**
- Values outside (-1, 1) return NaN or Infinity.

**Cross References:**
- `$API.Math.atan$`
- `$API.Math.tanh$`

---

## ceil

**Signature:** `Double ceil(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.ceil(2.3);`

**Description:**
Rounds the value up to the nearest integer. Always returns a double, even for integer input.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to round up | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.floor$`
- `$API.Math.round$`
- `$API.Math.trunc$`

---

## clamp

**Signature:** `Number clamp(Number value, Number lowerLimit, Number upperLimit)`
**Return Type:** `Number`
**Call Scope:** safe
**Minimal Example:** `var x = Math.clamp(150, 0, 127);`

**Description:**
Limits the value to the given range [lowerLimit, upperLimit]. This is a JavaScript-compatible alias for `Math.range()` -- both methods are identical.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to clamp | -- |
| lowerLimit | Number | no | The lower bound | -- |
| upperLimit | Number | no | The upper bound | Must be >= lowerLimit |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.range$`

---

## cos

**Signature:** `Double cos(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.cos(Math.PI);`

**Description:**
Calculates the cosine of the value (radian based).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The angle in radians | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.acos$`
- `$API.Math.cosh$`

---

## cosh

**Signature:** `Double cosh(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.cosh(1.0);`

**Description:**
Calculates the hyperbolic cosine of the value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The input value | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.cos$`
- `$API.Math.acosh$`

---

## exp

**Signature:** `Double exp(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.exp(1.0);`

**Description:**
Calculates e raised to the power of the value (the exponential function).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The exponent | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.log$`
- `$API.Math.log10$`

---

## floor

**Signature:** `Double floor(Number value)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var x = Math.floor(2.7);`

**Description:**
Rounds the value down to the nearest integer. Always returns a double, even for integer input.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| value | Number | no | The value to round down | -- |

**Pitfalls:**
None.

**Cross References:**
- `$API.Math.ceil$`
- `$API.Math.round$`
- `$API.Math.trunc$`
