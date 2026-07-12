Math::atanh(Number value) -> Double

Thread safety: SAFE
Calculates the inverse hyperbolic tangent.
Anti-patterns:
  - Values outside (-1, 1) return NaN or Infinity
Pair with:
  tanh -- forward/inverse pair
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::atanh()
    -> std::atanh((double)value)
