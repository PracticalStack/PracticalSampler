Math::sanitize(Number value) -> Double

Thread safety: SAFE
Replaces NaN and Infinity with 0.0. Guards against non-finite values that can
propagate through arithmetic chains and corrupt audio output.
Pair with:
  isnan -- check for NaN without replacing
  isinf -- check for infinity without replacing
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::sanitize()
    -> hmath::sanitize() -> FloatSanitizers::sanitizeDoubleNumber()
