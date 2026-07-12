Colours::withMultipliedAlpha(Colour colour, Double factor) -> Integer

Thread safety: SAFE
Returns a new colour with the alpha channel multiplied by factor. Relative
operation: 0.0 makes fully transparent, 1.0 unchanged, >1.0 increases opacity.
Factor clamped to >= 0.0 via jmax (no upper bound; result byte clamped internally).

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps factor: jmax(0.0f, factor)
  Delegates to JUCE Colour::withMultipliedAlpha(factor) -> getARGB()

Pair with:
  withAlpha -- absolute alpha replacement (use when you want a specific alpha, not relative)

Source:
  ScriptingApi.cpp:7020  ScriptingApi::Colours::withMultipliedAlpha()
    -> c.withMultipliedAlpha(jmax(0.0f, factor)).getARGB()
