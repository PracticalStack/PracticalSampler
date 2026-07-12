# Math -- C++ Source Exploration

## Resources Consulted
- `enrichment/resources/survey/class_survey_data.json` -- Math entry
- `enrichment/base/Math.json` -- 39 methods
- No prerequisites (standalone utility namespace)
- No base class exploration needed (ApiClass is the direct base)

## Class Declaration

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineMathObject.cpp` (lines 37-408)
**Forward declaration:** `HISE/hi_scripting/scripting/engine/HiseJavascriptEngine.h` (line 784)

```cpp
struct HiseJavascriptEngine::RootObject::MathClass : public ApiClass
```

- Nested struct inside `HiseJavascriptEngine::RootObject`
- Inherits from `ApiClass` (the base for namespace-style API objects)
- Registered as `"Math"` via `getObjectName()` returning `RETURN_STATIC_IDENTIFIER("Math")`
- Entirely self-contained in a single .cpp file (declaration + implementation)

## Registration

**File:** `HISE/hi_scripting/scripting/engine/JavascriptEngineAdditionalMethods.cpp` (line 166)

```cpp
registerApiClass(new RootObject::MathClass());
```

This is a global namespace object -- always available, no factory method needed. Accessed directly as `Math.methodName()`.

## Constructor -- Constants

```cpp
MathClass() : ApiClass(8)   // 8 = number of constants
{
    addConstant("PI", double_Pi);
    addConstant("E", exp(1.0));
    addConstant("SQRT2", sqrt(2.0));
    addConstant("SQRT1_2", sqrt(0.5));
    addConstant("LN2", log(2.0));
    addConstant("LN10", log(10.0));
    addConstant("LOG2E", std::log2((double)exp(1.0)));
    addConstant("LOG10E", log10(exp(1.0)));
}
```

All constants are double precision. `double_Pi` is a JUCE constant. The rest are computed at construction time from standard math functions.

| Name | Value (approx) | Description |
|------|----------------|-------------|
| PI | 3.14159265358979... | Pi (from JUCE `double_Pi`) |
| E | 2.71828182845904... | Euler's number |
| SQRT2 | 1.41421356237309... | Square root of 2 |
| SQRT1_2 | 0.70710678118654... | Square root of 0.5 (= 1/sqrt(2)) |
| LN2 | 0.69314718055994... | Natural log of 2 |
| LN10 | 2.30258509299404... | Natural log of 10 |
| LOG2E | 1.44269504088896... | Base-2 log of E |
| LOG10E | 0.43429448190325... | Base-10 log of E |

## Constructor -- Method Registration

All 39 methods are registered. Two patterns are used:

### ADD_INLINEABLE_API_METHOD_N (37 methods)

These methods are marked as "inlineable" via `setFunctionIsInlineable()`. This means the HiseScript JIT compiler can potentially inline these calls for better performance.

**Inlineable methods:** abs, round, min, max, range, sign, toDegrees, toRadians, sin, asin, sinh, asinh, cos, acos, cosh, acosh, tan, atan, tanh, atanh, log, log10, exp, pow, sqr, sqrt, ceil, floor, fmod, smoothstep, wrap, from0To1, to0To1, skew, isinf, isnan, sanitize, clamp, trunc

### ADD_API_METHOD_N (2 methods)

These are NOT inlineable:

- `random` (ADD_API_METHOD_0) -- returns different value each call, cannot be inlined
- `randInt` (ADD_API_METHOD_2) -- same reason

### Wrapper Struct

All methods have corresponding `API_METHOD_WRAPPER_N` entries in the `Wrapper` struct (lines 98-143). None use `ADD_TYPED_API_METHOD_N` -- all parameters are untyped `var`.

## Type Preservation Pattern

Several methods preserve integer types when given integer inputs:

| Method | Int path | Double path |
|--------|----------|-------------|
| abs | `std::abs((int)value)` | `std::abs((double)value)` |
| round | `roundToInt((int)value)` | `roundToInt((double)value)` |
| sign | `sign_((int)value)` | `sign_((double)value)` |
| range | `jlimit<int>(...)` | `jlimit<double>(...)` |
| min | `jmin((int)first, (int)second)` | `jmin((double)first, (double)second)` |
| max | `jmax((int)first, (int)second)` | `jmax((double)first, (double)second)` |

For min/max, BOTH inputs must be int for the int path (uses `&&`). For range, the check is on the value parameter only (`value.isInt()`).

All other methods cast to `double` unconditionally.

## Helper: sign_ Template

```cpp
template <typename Type> static Type sign_(Type n) noexcept {
    return n > 0 ? (Type)1 : (n < 0 ? (Type)-1 : 0);
}
```

Returns +1, -1, or 0. Note: this differs from `hmath::sign()` which uses `(value >= 0.0) * 2.0 - 1.0` (returns +1 for zero). The MathClass version returns 0 for zero input.

## Helper: getRange Static Method

**Lines 328-407** -- Complex static method used by `from0To1` and `to0To1`.

Converts a JSON range object to `scriptnode::InvertableParameterRange`. Supports two input types:

### FixObject path (lines 332-373)

When `rangeObj` is a `fixobj::ObjectReference`, it reads raw float pointers using type hash matching:

| Hash | Layout | Fields |
|------|--------|--------|
| 1207537023 | scriptnode range | MinValue, MaxValue, SkewFactor, StepSize, Inverted(float) |
| -1419086716 | UI component range | min, max, middlePosition, stepSize, Inverted(float) |
| -748746349 | MIDI automation range | Start, End, Skew, Interval, Inverted |
| -575529029 | scriptnode 3-field | MinValue, MaxValue, SkewFactor |
| 1468876904 | UI component 3-field | min, max, middlePosition |
| 2138798677 | MIDI automation 3-field | Start, End, Skew |
| -1567604795 | scriptnode step | MinValue, MaxValue, StepSize |
| -1126239209 | UI component step | min, max, stepSize |
| 1610048532 | MIDI automation step | Start, End, Interval |

Key behavior: hashes 1468876904 and -1419086716 (the "middlePosition" variants) call `setSkewForCentre()` instead of setting `skew` directly. This converts a midpoint value to a skew factor.

Unknown hashes throw an error string.

### DynamicObject path (lines 375-404)

When `rangeObj` is a plain JSON object (`DynamicObject`), it checks for three naming conventions:

1. **scriptnode** (has `MaxValue`): reads `MinValue`, `MaxValue`, `StepSize`, `SkewFactor`
2. **UI Component** (has `max`): reads `min`, `max`, `stepSize`, `middlePosition` (converts middlePosition to skew)
3. **MIDI Automation** (has `Start`): reads `Start`, `End`, `StepSize`, `Skew`

All three read `Inverted` from `PropertyIds::Inverted`.

## InvertableParameterRange

**File:** `HISE/hi_dsp_library/node_api/helpers/NodeProperty.h` (lines 40-99)

Wraps `juce::NormalisableRange<double>` with an `inv` (inversion) flag. Key methods used by Math:

- `convertFrom0to1(double input, bool applyInversion)` -- used by `from0To1`
- `convertTo0to1(double input, bool applyInversion)` -- used by `to0To1`

Both are called with `applyInversion = true`.

## hmath Struct (Upstream Implementation Reference)

**File:** `HISE/hi_dsp_library/snex_basics/snex_Math.h` (lines 48-328)

The `hmath` struct in the SNEX math library provides the underlying implementations for several MathClass methods. MathClass calls `hmath::` directly for:

- `hmath::fmod(double, double)` -> `std::fmod`
- `hmath::wrap(double, double)` -> `fmod(value + limit, limit)` (or negative-safe variant under `SNEX_WRAP_ALL_NEGATIVE_INDEXES`)
- `hmath::smoothstep(double, double, double)` -> Hermite interpolation: `t*t*(3-2*t)` where `t = clamp((input-lower)/(upper-lower), 0, 1)`
- `hmath::sanitize(double)` -> `FloatSanitizers::sanitizeDoubleNumber(a)` (replaces NaN/Inf with 0.0)

Note: `hmath` is also available as a static global `Math` object in SNEX/scriptnode C++ code, providing the same function set. The scripting Math class is the HiseScript equivalent.

## smoothstep Algorithm Detail

```cpp
var smoothstep(var input, var lower, var upper) {
    return var(upper > lower ? hmath::smoothstep((double)input, (double)lower, (double)upper) : 0.0);
}
```

Guard: returns 0.0 when `upper <= lower` (avoids division by zero). The hmath implementation:

```
t = clamp((input - lower) / (upper - lower), 0.0, 1.0)
result = clamp(t * t * (3.0 - 2.0 * t), 0.0, 1.0)
```

Standard cubic Hermite smoothstep with double clamping.

## wrap Behavior

Default path (without `SNEX_WRAP_ALL_NEGATIVE_INDEXES`):
```cpp
fmod(value + limit, limit)
```

This means negative values are handled by adding `limit` once. For values more negative than `-limit`, the result may still be negative. The SNEX negative-safe variant handles arbitrary negatives but is compile-time guarded.

## skew -- NormalisableRange Integration

```cpp
var skew(var start, var end, var midPoint) {
    NormalisableRange<double> rng(start, end);
    rng.setSkewForCentre(midPoint);
    return rng.skew;
}
```

Creates a temporary JUCE `NormalisableRange`, calls `setSkewForCentre()` with the midpoint, and returns the computed skew factor. This is a utility to calculate the skew value needed for a range where `midPoint` maps to 0.5 in normalized space.

## clamp vs range

`clamp` is an alias for `range`:

```cpp
var clamp(var value, var lowerLimit, var upperLimit) {
    return range(value, lowerLimit, upperLimit);
}
```

Both limit a value to [lower, upper]. `clamp` exists for JavaScript compatibility.

## trunc Implementation

```cpp
var trunc(var value) {
    return var((int)(double)value);
}
```

Casts to double then to int -- truncates toward zero. Note: always returns int, unlike `floor`/`ceil` which return double.

## Threading and Lifecycle

- No threading constraints. All methods are pure functions (stateless).
- `random` and `randInt` use `Random::getSystemRandom()` which is a thread-local JUCE random.
- No onInit-only restrictions.
- Safe to call from any callback (onInit, onNoteOn, onControl, onTimer, paint routines, etc.).

## Preprocessor Guards

None. The Math class has no conditional compilation. All methods are always available.
