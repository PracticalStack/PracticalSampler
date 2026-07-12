Math::wrap(Number value, Number limit) -> Double

Thread safety: SAFE
Wraps value around limit so result is always in [0, limit). Useful for cyclic
parameters like phase angles or circular buffer indices.
Dispatch/mechanics:
  hmath::wrap() -> fmod(value + limit, limit)
Anti-patterns:
  - [BUG] For values more negative than -limit, the single value + limit offset
    may not bring the result into [0, limit). Example: Math.wrap(-5.0, 3.0)
    computes fmod(-2.0, 3.0) = -2.0, not the expected 1.0
Pair with:
  fmod -- floating-point remainder (preserves sign of dividend)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::wrap()
    -> hmath::wrap() (snex_Math.h:48-328) -> fmod(value + limit, limit)
