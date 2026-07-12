Math::asin(Number value) -> Double

Thread safety: SAFE
Calculates the arc sine (inverse sine). Returns radians in [-PI/2, PI/2].
Anti-patterns:
  - Values outside [-1, 1] return NaN
Pair with:
  sin -- forward/inverse pair
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::asin()
    -> std::asin((double)value)
