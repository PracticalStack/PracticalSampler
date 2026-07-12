Math::sign(Number value) -> Number

Thread safety: SAFE
Returns 1 for positive, -1 for negative, 0 for zero. Preserves integer type
when input is an integer.
Dispatch/mechanics:
  sign_<Type>(n): n > 0 ? 1 : (n < 0 ? -1 : 0)
  Note: returns 0 for zero, unlike hmath::sign() which returns +1 for zero
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::sign()
    -> sign_<int>() or sign_<double>() template helper
