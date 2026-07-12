<!-- Diagram triage:
  - No diagrams specified in Phase 1 data
-->

# Math

The `Math` object is a globally available collection of mathematical functions for HiseScript. It is a feature-complete clone of the standard JavaScript [Math object](https://www.w3schools.com/Js/js_math.asp), extended with audio-specific utilities for range conversion, value wrapping, and numeric sanitation.

Its capabilities fall into several groups:

1. **Trigonometry** - `sin`, `cos`, `tan` and their inverse/hyperbolic variants, plus degree/radian conversion
2. **Rounding and truncation** - `round`, `floor`, `ceil`, `trunc`
3. **Clamping and wrapping** - `range`/`clamp` for hard limits, `wrap` for cyclic values, `fmod` for remainders
4. **Logarithmic and exponential** - `log`, `log10`, `exp`, `pow`, `sqrt`, `sqr`
5. **Random numbers** - `random` (double) and `randInt` (integer)
6. **Range conversion** - `from0To1`, `to0To1`, and `skew` for bidirectional mapping between normalised and real parameter values
7. **Numeric safety** - `sanitize`, `isnan`, `isinf` for guarding against non-finite values
8. **Interpolation** - `smoothstep` for S-curve transitions

Unless specified otherwise, all functions accept and return `double` values. Several methods (`abs`, `round`, `sign`, `range`, `min`, `max`) preserve integer types when given integer input.

The editor's autocomplete also exposes standard mathematical constants:

| Constant | Value | Description |
|----------|-------|-------------|
| `Math.PI` | 3.14159... | Pi |
| `Math.E` | 2.71828... | Euler's number |
| `Math.SQRT2` | 1.41421... | Square root of 2 |
| `Math.SQRT1_2` | 0.70711... | 1 / square root of 2 |
| `Math.LN2` | 0.69315... | Natural logarithm of 2 |
| `Math.LN10` | 2.30259... | Natural logarithm of 10 |
| `Math.LOG2E` | 1.44270... | Base-2 logarithm of e |
| `Math.LOG10E` | 0.43429... | Base-10 logarithm of e |

> The `Math` class is also available in SNEX and in scriptnode expression nodes, so the same functions and constants work across all three contexts.

## Common Mistakes

- **Wrong:** `Math.from0To1(0.5, {"min": 20, "max": 20000})`
  **Right:** `Math.from0To1(0.5, {"min": 20, "max": 20000, "middlePosition": 1000})`
  *Without `middlePosition` or a skew factor, `from0To1` applies a linear mapping. For frequency ranges spanning several orders of magnitude, a skewed curve is almost always needed to make the control feel natural.*

- **Wrong:** `Math.pow(peak, 0.5)` for meter display
  **Right:** `Math.pow(peak, 0.25)` for meter display
  *A square root does not compress enough for audio peak meters. The fourth root (exponent 0.25) better matches human loudness perception, compressing loud values and expanding quiet ones.*

- **Wrong:** `x + amount * Math.random()` for bipolar randomisation
  **Right:** `x + amount * (2.0 * Math.random() - 1.0)` for bipolar randomisation
  *`Math.random()` returns values in [0, 1). For offsets that should go both positive and negative, scale to [-1, 1) first with `2.0 * Math.random() - 1.0`.*

**See also:** $SN.math.expr$ -- Scriptnode expression node for per-sample math operations in DspNetwork
