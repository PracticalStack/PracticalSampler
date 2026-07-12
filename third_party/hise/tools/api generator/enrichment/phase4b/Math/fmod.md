Math::fmod(Number value, Number limit) -> Double

Thread safety: SAFE
Returns the floating-point remainder of value / limit. Result has the same sign
as the dividend (value).
Anti-patterns:
  - Returns NaN when limit is zero -- no error is thrown, unlike integer division
Pair with:
  wrap -- similar but always returns positive result in [0, limit)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::fmod()
    -> hmath::fmod() -> std::fmod((double)value, (double)limit)
