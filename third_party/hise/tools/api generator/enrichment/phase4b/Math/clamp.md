Math::clamp(Number value, Number lowerLimit, Number upperLimit) -> Number

Thread safety: SAFE
Limits value to [lowerLimit, upperLimit]. Alias for Math.range() -- both are
identical. Exists for JavaScript compatibility.
Dispatch/mechanics:
  Calls range(value, lowerLimit, upperLimit) directly
Pair with:
  range -- identical method, HISE-native name
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::clamp()
    -> range(value, lowerLimit, upperLimit)
