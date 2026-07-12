Math::randInt(Number low, Number high) -> Integer

Thread safety: SAFE
Returns a random integer in [low, high). Upper bound is exclusive. Uses JUCE's
thread-local system random, safe from any thread.
Anti-patterns:
  - Upper bound is exclusive: Math.randInt(0, 128) returns 0-127, never 128
Pair with:
  random -- random double in [0.0, 1.0)
Source:
  JavascriptEngineMathObject.cpp:37-408  MathClass::randInt()
    -> Random::getSystemRandom().nextInt(Range<int>(low, high))
