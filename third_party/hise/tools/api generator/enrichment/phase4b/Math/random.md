Math::random() -> Double

Thread safety: SAFE
Returns a random double in [0.0, 1.0). Uses JUCE's thread-local system random,
safe from any thread including the audio thread. Not inlineable (returns
different value each call).
Pair with:
  randInt -- random integer in a range
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::random()
    -> Random::getSystemRandom().nextDouble()
