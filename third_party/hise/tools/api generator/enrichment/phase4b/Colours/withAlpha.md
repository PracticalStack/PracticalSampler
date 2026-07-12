Colours::withAlpha(Colour colour, Double alpha) -> Integer

Thread safety: SAFE
Returns a new colour with the alpha channel replaced by the specified value.
RGB channels preserved. Alpha clamped to 0.0-1.0 via jlimit.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps alpha: jlimit(0.0f, 1.0f, alpha)
  Delegates to JUCE Colour::withAlpha(clampedAlpha) -> getARGB()

Pair with:
  withMultipliedAlpha -- relative alpha scaling (preserves existing partial transparency)

Source:
  ScriptingApi.cpp:7002  ScriptingApi::Colours::withAlpha()
    -> c.withAlpha(jlimit(0.0f, 1.0f, alpha)).getARGB()
