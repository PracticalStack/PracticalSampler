Math::max(Number first, Number second) -> Number

Thread safety: SAFE
Returns the larger of two numbers. Preserves integer type when both inputs are
integers; returns double if either input is a double.
Dispatch/mechanics:
  (first.isInt() && second.isInt()) ? jmax<int>(...) : jmax<double>(...)
Pair with:
  min -- returns the smaller of two numbers
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::max()
    -> jmax<int>() or jmax<double>()
