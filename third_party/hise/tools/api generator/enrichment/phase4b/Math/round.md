Math::round(Number value) -> Integer

Thread safety: SAFE
Rounds to the nearest integer. Always returns Integer (unlike ceil/floor which
return Double). Uses JUCE's roundToInt which rounds half-up.
Dispatch/mechanics:
  value.isInt() ? roundToInt((int)value) : roundToInt((double)value)
Pair with:
  ceil -- round up (returns Double)
  floor -- round down (returns Double)
  trunc -- truncate toward zero (returns Integer)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::round()
    -> juce::roundToInt()
