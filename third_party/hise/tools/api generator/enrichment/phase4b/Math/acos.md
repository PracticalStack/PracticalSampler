Math::acos(Number value) -> Double

Thread safety: SAFE
Calculates the arc cosine (inverse cosine). Returns radians in [0, PI].
Anti-patterns:
  - Values outside [-1, 1] return NaN
Pair with:
  cos -- forward/inverse pair
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::acos()
    -> std::acos((double)value)
