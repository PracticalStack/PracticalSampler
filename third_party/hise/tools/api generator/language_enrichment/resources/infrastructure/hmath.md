# hmath Library Reference

Distilled from C++ source for the scriptnode node enrichment pipeline.
All information derived exclusively from HISE source code.

Source files consulted:
- `hi_dsp_library/snex_basics/snex_Math.h`
- `hi_dsp_library/dsp_nodes/MathNodes.h`

---

## 1. Overview

`hmath` is a static struct in the `snex` namespace (`snex::hmath`) that provides
all math functions available in SNEX and used internally by scriptnode math nodes.
A global instance `Math` is declared at namespace scope for convenience in SNEX code.

The library provides three tiers of functions:
1. **Scalar functions** -- standard math on float/double/int values
2. **Block (SIMD) functions** -- vectorized operations on `block` (`dyn<float>`)
3. **Fast approximations** -- JUCE FastMathApproximations wrappers

All scalar functions have overloads for float, double, and (where applicable) int.
Block functions operate in-place and return a reference to the modified block.

---

## 2. Constants

| Name | Type | Value | Description |
|------|------|-------|-------------|
| `PI` | double | 3.14159265358979... | Pi |
| `E` | double | 2.71828182845904... | Euler's number |
| `SQRT2` | double | 1.41421356237309... | Square root of 2 |
| `FORTYTWO` | double | 42.0 | Unit test helper only |

---

## 3. Scalar Functions

All scalar functions are `static forcedinline` or `static constexpr`. Each has
float and double overloads unless noted otherwise.

### Trigonometric

| Function | Signature | Description |
|----------|-----------|-------------|
| `sin(a)` | float/double -> float/double | Sine (std::sin) |
| `cos(a)` | float/double -> float/double | Cosine (std::cos) |
| `tan(a)` | float/double -> float/double | Tangent (std::tan) |
| `asin(a)` | float/double -> float/double | Arc sine |
| `acos(a)` | float/double -> float/double | Arc cosine |
| `atan(a)` | float/double -> float/double | Arc tangent |
| `sinh(a)` | float/double -> float/double | Hyperbolic sine |
| `cosh(a)` | float/double -> float/double | Hyperbolic cosine |
| `tanh(a)` | float/double -> float/double | Hyperbolic tangent |
| `atanh(a)` | float/double -> float/double | Inverse hyperbolic tangent |

### Exponential / Logarithmic

| Function | Signature | Description |
|----------|-----------|-------------|
| `exp(a)` | float/double -> float/double | e^a |
| `log(a)` | float/double -> float/double | Natural logarithm |
| `log10(a)` | float/double -> float/double | Base-10 logarithm |
| `pow(base, exp)` | (float,float)/(double,double) | Power function |
| `sqr(a)` | float/double -> float/double | Square (a*a, NOT sqrt) |
| `sqrt(a)` | float/double -> float/double | Square root |

### Rounding / Clamping

| Function | Signature | Description |
|----------|-----------|-------------|
| `round(a)` | float/double/int -> same | Round to nearest integer. Uses roundf(). Int overload is identity. |
| `ceil(a)` | float/double -> float/double | Ceiling |
| `floor(a)` | float/double -> float/double | Floor |
| `range(v, lo, hi)` | float/double/int -> same | Clamp v to [lo, hi] via jlimit |
| `min(a, b)` | float/double/int -> same | Minimum of two values |
| `max(a, b)` | float/double/int -> same | Maximum of two values |
| `abs(a)` | float/double/int -> same | Absolute value. float/double use sign()*value. int uses ternary. |
| `sign(a)` | float/double -> same | Returns -1 or +1. double: (value >= 0.0)*2.0 - 1.0. float: ternary. |

**sign() asymmetry:** The double overload returns +1 for zero (`value >= 0.0`).
The float overload returns -1 for zero (`value > 0.0f`). This means
`hmath::sign(0.0)` is `1.0` but `hmath::sign(0.0f)` is `-1.0f`.

### Modulo / Wrapping

| Function | Signature | Description |
|----------|-----------|-------------|
| `fmod(x, y)` | float/double/int -> same | Floating-point modulo. int uses `%`. |
| `wrap(v, limit)` | float/double/int -> same | Wraps value into [0, limit) range |

**wrap() behavior depends on compile flag `SNEX_WRAP_ALL_NEGATIVE_INDEXES`:**

- When **enabled**: negative values are properly wrapped using
  `fmod(limit - fmod(abs(value), limit), limit)`. Always produces [0, limit).
- When **disabled** (default path in most builds): uses `fmod(value + limit, limit)`.
  This only handles values down to `-limit`. Values below `-limit` produce
  incorrect results (negative output).

### Range Conversion

| Function | Signature | Description |
|----------|-----------|-------------|
| `sig2mod(v)` | float/double -> same | Bipolar to unipolar: v * 0.5 + 0.5. Maps [-1,1] to [0,1]. |
| `mod2sig(v)` | float/double -> same | Unipolar to bipolar: v * 2.0 - 1.0. Maps [0,1] to [-1,1]. |
| `norm(v, min, max)` | float/double -> same | Normalize: (v - min) / (max - min). No division-by-zero guard. |
| `map(input, start, end)` | float/double -> same | Denormalize: maps [0,1] to [start,end] via jmap. |

### Audio Utilities

| Function | Signature | Description |
|----------|-----------|-------------|
| `db2gain(a)` | float/double -> same | Decibels to linear gain (JUCE Decibels::decibelsToGain) |
| `gain2db(a)` | float/double -> same | Linear gain to decibels (JUCE Decibels::gainToDecibels) |
| `smoothstep(input, lo, hi)` | float/double -> same | Hermite smoothstep: double-clamped t*t*(3-2t) |
| `sanitize(a)` | float/double -> same | Replace NaN/Inf with 0 (FloatSanitizers) |
| `isinf(a)` | float/double -> int | Test for infinity |
| `isnan(a)` | float/double -> int | Test for NaN |

### Random

| Function | Signature | Description |
|----------|-----------|-------------|
| `random()` | -> float | Random float in [0, 1) |
| `randomDouble()` | -> double | Random double in [0, 1) |
| `randInt(lo, hi)` | (int, int) -> int | Random int in [lo, hi). Defaults: [0, INT_MAX). |

---

## 4. Block (SIMD-Vectorized) Functions

These operate on `block` (`dyn<float>`) in-place and return a reference to the
modified block. They use JUCE `FloatVectorOperations` which dispatches to SSE/NEON
intrinsics when available. This makes them significantly faster than per-sample
loops for large buffers.

### Arithmetic

| Function | Signature | JUCE Backend | Description |
|----------|-----------|-------------|-------------|
| `vmul(b1, b2)` | (block&, block&) -> block& | FVO::multiply | Element-wise multiply |
| `vadd(b1, b2)` | (block&, block&) -> block& | FVO::add | Element-wise add |
| `vsub(b1, b2)` | (block&, block&) -> block& | FVO::subtract | Element-wise subtract |
| `vmov(b1, b2)` | (block&, block&) -> block& | FVO::copy | Copy b2 into b1 |
| `vmuls(b1, s)` | (block&, float) -> block& | FVO::multiply | Multiply by scalar |
| `vadds(b1, s)` | (block&, float) -> block& | FVO::add | Add scalar |
| `vmovs(b1, s)` | (block&, float) -> block& | FVO::fill | Fill with scalar |

Binary operations assert `b1.size() == b2.size()` (debug only).

### Clamping / Absolute

| Function | Signature | JUCE Backend | Description |
|----------|-----------|-------------|-------------|
| `vclip(b, lo, hi)` | (block&, float, float) -> block& | FVO::clip | Clamp all samples to [lo, hi] |
| `vabs(b)` | (block&) -> block& | FVO::abs | Absolute value of all samples |
| `min(b, s)` | (block&, float) -> block& | FVO::min | Per-sample minimum with scalar |
| `min(b1, b2)` | (block&, block&) -> block& | FVO::min | Per-sample minimum of two blocks |
| `max(b, s)` | (block&, float) -> block& | FVO::max | Per-sample maximum with scalar |
| `max(b1, b2)` | (block&, block&) -> block& | FVO::max | Per-sample maximum of two blocks |
| `range(b, lo, hi)` | (block&, float, float) -> block& | FVO::clip | Same as vclip |
| `abs(b)` | (block&) -> block& | FVO::abs | Same as vabs |

### Analysis

| Function | Signature | Description |
|----------|-----------|-------------|
| `peak(b)` | (block&) -> float | Returns max(abs(min), abs(max)) of the block. Uses FVO::findMinAndMax. |

---

## 5. Fast Approximation Functions

Wrappers around `juce::dsp::FastMathApproximations`. These trade precision for
speed -- suitable for audio-rate processing where exact values are not critical.
Each has float and double overloads.

| Function | Approximates |
|----------|-------------|
| `fastsin(a)` | sin |
| `fastcos(a)` | cos |
| `fasttan(a)` | tan |
| `fasttanh(a)` | tanh |
| `fastsinh(a)` | sinh |
| `fastcosh(a)` | cosh |
| `fastexp(a)` | exp |

No fast approximations exist for: asin, acos, atan, log, log10, pow, sqrt.

---

## 6. Wrapped Structs (for Template Node Use)

The `hmath::wrapped` namespace contains small structs with a static `op()`
method, used as template arguments for nodes that need a math function as a
type parameter.

| Struct | Maps to |
|--------|---------|
| `hmath::wrapped::sin` | `hmath::sin()` |
| `hmath::wrapped::tanh` | `hmath::tanh()` |

Each has a `name[]` constexpr char array and float/double `op()` overloads.

---

## 7. Mapping: hmath Functions to math.* Nodes

The `math` namespace in `MathNodes.h` defines operation structs in
`math::Operations`, each with `op()` (block processing) and `opSingle()`
(frame processing) methods. These are wrapped by `OpNode<OpType, NV>` into
full scriptnode nodes.

### Node Architecture

All math operation nodes use the same template:
```
OpNode<Operations::xxx, NV> : OpNodeBase<Operations::xxx> : mothernode, polyphonic_base
```

Each node has a single "Value" parameter (polyphonic, stored in `PolyData<float, NV>`).
The operation struct's `getDefaultValue()` determines the parameter's default.

### Polyphonic vs Mono-Only Nodes

Nodes defined with `DEFINE_OP_NODE` support polyphonic operation (NV voices).
Nodes defined with `DEFINE_MONO_OP_NODE` are always mono (NV=1), meaning the
Value parameter is not per-voice.

### Node-to-hmath Mapping Table

| Node | Default Value | Poly | hmath functions used | Formula |
|------|--------------|------|---------------------|---------|
| `math.mul` | 1.0 | Yes | `vmuls` | `s *= value` |
| `math.add` | 0.0 | Yes | `vadds` | `s += value` |
| `math.sub` | 0.0 | Yes | `vadds` (negated) | `s -= value` |
| `math.div` | 1.0 | Yes | `vmuls` (reciprocal) | `s *= (1/value)` if value>0, else 0 |
| `math.clip` | 1.0 | Yes | `vclip` | clamp to [-value, value] |
| `math.tanh` | 1.0 | Yes | (none, uses tanhf directly) | `s = tanh(s * value)` |
| `math.fmod` | 1.0 | Yes | `hmath::fmod` | `s = fmod(s, value)` |
| `math.square` | 1.0 | Yes | `vmul(b, b)` | `s *= s` |
| `math.intensity` | 0.0 | Yes | `vmuls`, `vadds` | `s = (1-value) + value*s` |
| `math.sqrt` | 1.0 | Yes | (none, uses sqrtf) | `s = sqrt(s)` |
| `math.pow` | 1.0 | Yes | (none, uses powf) | `s = pow(s, value)` |
| `math.pi` | 2.0 | Mono | `vmuls` | `s *= PI * value` |
| `math.sin` | 2.0 | Mono | (none, uses sinf) | `s = sin(s)` |
| `math.sig2mod` | 0.0 | Mono | (none) | `s = s*0.5 + 0.5` |
| `math.mod2sig` | 0.0 | Mono | (none) | `s = s*2.0 - 1.0` |
| `math.rect` | 0.0 | Mono | (none) | `s = (s >= 0.5) ? 1.0 : 0.0` |
| `math.abs` | 0.0 | Mono | `vabs` | `s = abs(s)` |
| `math.mod_inv` | 0.0 | Mono | `vmuls`, `vadds` | `s = 1.0 - s` |
| `math.inv` | 0.0 | Mono | `vmuls` | `s = -s` |
| `math.clear` | 0.0 | Mono | `vmovs` | `s = 0.0` |
| `math.fill1` | 0.0 | Mono | `vmovs` | `s = 1.0` |

### Nodes That Ignore the Value Parameter

Several mono-only nodes ignore their Value parameter entirely in processing:
`sin`, `sig2mod`, `mod2sig`, `rect`, `abs`, `mod_inv`, `inv`, `clear`, `fill1`.
The parameter exists (it is part of the OpNode template) but the operation struct's
`op()`/`opSingle()` methods do not use the `value` argument.

### Block vs Frame Processing Paths

- Nodes using `OP_BLOCK2SINGLE`: the block path iterates channels and delegates
  to `opSingle` per channel. No SIMD acceleration. Applies to: `tanh`, `sin`,
  `sig2mod`, `mod2sig`, `rect`, `sqrt`, `pow`, `abs` (note: abs has a custom
  block path using `vabs`, overriding the macro pattern).
- Nodes with custom `OP_BLOCK`: use vectorized hmath block functions (`vmuls`,
  `vadds`, `vclip`, `vmul`, `vmovs`). These are SIMD-accelerated. Applies to:
  `mul`, `add`, `sub`, `div`, `clip`, `square`, `pi`, `intensity`, `mod_inv`,
  `inv`, `clear`, `fill1`.

### Special Nodes (Not OpNode-based)

| Node | Description |
|------|-------------|
| `math.map` | Range mapper with 4 parameters: InputStart, InputEnd, OutputStart, OutputEnd. Uses `hmath::range` for clamping. Not based on OpNode template. |
| `math.table` | LUT node using Table external data (512 samples, normalized input 0-1). Uses `data::base` for external data. |
| `math.pack` | LUT node using SliderPack external data (variable size, input scaled by pack size). Same template as table with DataSize=0. |
| `math.expr` | Expression node using a user-supplied ExpressionClass with static `op(float, float)`. Template: `OpNode<expression_base<ExpressionClass>, NV>`. |
| `math.neural` | Neural network inference node. Requires `HISE_INCLUDE_RT_NEURAL`. Not hmath-related. |

---

## 8. Linux Compatibility Note

On Linux (`JUCE_LINUX`), the `std_` macro resolves to empty (global namespace),
while on other platforms it resolves to `std`. This handles differences in math
function availability between standard library implementations. The float
overloads use explicit `sinf`, `cosf`, etc. suffixed names.

---

## 9. Known Issues

### clip node opSingle bug

In `MathNodes.h` line 410, the frame-processing path for `math.clip` contains:
```cpp
s *= jlimit(-value, value, s);
```
This multiplies `s` by the clamped value of `s`, rather than simply clamping `s`.
The block path (line 403) correctly uses `hmath::vclip(b, -value, value)` which
is a pure clamp. The frame path produces `s * clamp(s, -value, value)` instead
of `clamp(s, -value, value)`.

### div node only guards positive values

In `MathNodes.h` lines 254/266, `math.div` checks `value > 0.0f` and returns 0
for non-positive divisors. Negative divisors are treated as zero rather than
performing a negative division. This means `math.div` with Value=-2 produces
silence, not signal inversion with halved amplitude.

### sign(0.0f) returns -1

The float overload of `hmath::sign` uses `value > 0.0f` (strict greater-than),
so `sign(0.0f)` returns `-1.0f`. The double overload uses `value >= 0.0`, so
`sign(0.0)` returns `+1.0`. This inconsistency also affects `hmath::abs(float)`
which is defined as `value * sign(value)` -- `abs(0.0f)` returns `0.0f * -1.0f`
which is `-0.0f` (negative zero). Functionally equivalent to zero in IEEE 754
but could cause issues in sign-sensitive comparisons.
