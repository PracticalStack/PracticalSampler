Math::log10(Number value) -> Double

Thread safety: SAFE
Calculates the base-10 logarithm of the value.
Anti-patterns:
  - Returns -Infinity for zero, NaN for negative values
Pair with:
  log -- natural logarithm
  exp -- exponential function
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::log10()
    -> std::log10((double)value)
