Math::tanh(Number value) -> Double

Thread safety: SAFE
Calculates the hyperbolic tangent. Output is always in (-1, 1), making it
useful as a soft-clipping function for audio signals.
Pair with:
  atanh -- inverse hyperbolic tangent
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::tanh()
    -> std::tanh((double)value)
