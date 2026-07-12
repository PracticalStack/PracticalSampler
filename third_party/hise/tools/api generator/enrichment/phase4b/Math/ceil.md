Math::ceil(Number value) -> Double

Thread safety: SAFE
Rounds up to the nearest integer. Always returns Double, even for integer input.
Pair with:
  floor -- round down counterpart
  round -- nearest-integer rounding
  trunc -- truncation toward zero
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::ceil()
    -> std::ceil((double)value)
