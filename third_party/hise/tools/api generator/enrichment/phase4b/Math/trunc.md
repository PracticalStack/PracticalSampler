Math::trunc(Number value) -> Integer

Thread safety: SAFE
Truncates toward zero by removing the decimal part. Always returns Integer,
unlike floor which returns Double and rounds toward negative infinity.
Dispatch/mechanics:
  (int)(double)value -- cast to double then to int
Anti-patterns:
  - trunc rounds toward zero while floor rounds toward negative infinity.
    For negatives: Math.trunc(-2.7) = -2, Math.floor(-2.7) = -3.0
Pair with:
  floor -- round toward negative infinity (returns Double)
  round -- round to nearest (returns Integer)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::trunc()
    -> (int)(double)value
