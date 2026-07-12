Math::abs(Number value) -> Number

Thread safety: SAFE
Returns the absolute (unsigned) value. Preserves integer type: integer input
returns integer, otherwise returns double.
Dispatch/mechanics:
  value.isInt() ? std::abs((int)value) : std::abs((double)value)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::abs()
