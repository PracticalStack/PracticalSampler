Math::log(Number value) -> Double

Thread safety: SAFE
Calculates the natural logarithm (base e) of the value.
Anti-patterns:
  - Returns -Infinity for zero, NaN for negative values
Pair with:
  exp -- inverse function (e^x)
  log10 -- base-10 logarithm
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::log()
    -> std::log((double)value)
