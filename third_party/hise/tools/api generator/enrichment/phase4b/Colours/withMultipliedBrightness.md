Colours::withMultipliedBrightness(Colour colour, Double factor) -> Integer

Thread safety: SAFE
Returns a new colour with the brightness (HSB value) multiplied by factor.
Relative operation: 0.0 produces black, 1.0 unchanged, >1.0 increases brightness.
Factor clamped to >= 0.0 via jmax (no upper bound; result clamped internally).
Hue, saturation, and alpha preserved.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps factor: jmax(0.0f, factor)
  Delegates to JUCE Colour::withMultipliedBrightness(factor) -> getARGB()

Pair with:
  withBrightness -- absolute brightness replacement
  withMultipliedSaturation -- often adjusted alongside brightness for consistent dimming

Source:
  ScriptingApi.cpp:7026  ScriptingApi::Colours::withMultipliedBrightness()
    -> c.withMultipliedBrightness(jmax(0.0f, factor)).getARGB()
