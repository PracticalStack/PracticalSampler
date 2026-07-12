Math::range(Number value, Number lowerLimit, Number upperLimit) -> Number

Thread safety: SAFE
Limits value to [lowerLimit, upperLimit]. Preserves integer type when value is
an integer. Math.clamp() is an alias for this method.
Dispatch/mechanics:
  value.isInt() ? jlimit<int>(lower, upper, value) : jlimit<double>(...)
Anti-patterns:
  - Integer type preservation is based only on the value parameter, not the
    limits. Math.range(5, 0.0, 10.0) uses the int path because 5 is an int,
    which silently truncates fractional limits
Pair with:
  clamp -- identical method (JavaScript-compatible name)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::range()
    -> jlimit<int>() or jlimit<double>()
