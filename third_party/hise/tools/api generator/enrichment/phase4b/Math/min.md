Math::min(Number first, Number second) -> Number

Thread safety: SAFE
Returns the smaller of two numbers. Preserves integer type when both inputs are
integers; returns double if either input is a double.
Dispatch/mechanics:
  (first.isInt() && second.isInt()) ? jmin<int>(...) : jmin<double>(...)
Pair with:
  max -- returns the larger of two numbers
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::min()
    -> jmin<int>() or jmin<double>()
