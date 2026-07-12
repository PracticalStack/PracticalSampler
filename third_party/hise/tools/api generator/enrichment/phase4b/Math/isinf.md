Math::isinf(Number value) -> Integer

Thread safety: SAFE
Returns 1 if value is positive or negative infinity, 0 otherwise.
Pair with:
  isnan -- check for NaN
  sanitize -- replace NaN/Infinity with 0.0
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::isinf()
    -> std::isinf((double)value)
