Math::acosh(Number value) -> Double

Thread safety: SAFE
Calculates the inverse hyperbolic cosine of the value.
Anti-patterns:
  - Values less than 1.0 return NaN
Pair with:
  cosh -- forward/inverse pair
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::acosh()
    -> std::acosh((double)value)
