Math::sqrt(Number value) -> Double

Thread safety: SAFE
Calculates the square root of the value.
Anti-patterns:
  - Negative values return NaN
Pair with:
  sqr -- square (x*x)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::sqrt()
    -> std::sqrt((double)value)
