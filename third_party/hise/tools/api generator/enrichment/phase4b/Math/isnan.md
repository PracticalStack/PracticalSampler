Math::isnan(Number value) -> Integer

Thread safety: SAFE
Returns 1 if value is NaN (Not a Number), 0 otherwise.
Pair with:
  isinf -- check for infinity
  sanitize -- replace NaN/Infinity with 0.0
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::isnan()
    -> std::isnan((double)value)
