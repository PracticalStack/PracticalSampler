Math::floor(Number value) -> Double

Thread safety: SAFE
Rounds down to the nearest integer. Always returns Double, even for integer
input. Rounds toward negative infinity (not toward zero).
Pair with:
  ceil -- round up counterpart
  round -- nearest-integer rounding
  trunc -- truncation toward zero (different for negatives)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::floor()
    -> std::floor((double)value)
