Math::smoothstep(Number input, Number lower, Number upper) -> Double

Thread safety: SAFE
Smooth S-curve transition between lower and upper bounds using cubic Hermite
interpolation: t*t*(3-2*t) where t = clamp((input-lower)/(upper-lower), 0, 1).
Returns 0.0 when input <= lower, 1.0 when input >= upper.
Dispatch/mechanics:
  Guard: returns 0.0 when upper <= lower (avoids division by zero)
  t = clamp((input - lower) / (upper - lower), 0.0, 1.0)
  result = clamp(t * t * (3.0 - 2.0 * t), 0.0, 1.0)
Anti-patterns:
  - Returns 0.0 (not an error) when upper <= lower
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::smoothstep()
    -> hmath::smoothstep() (snex_Math.h:48-328)
