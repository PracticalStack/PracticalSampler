Colours::withMultipliedSaturation(Colour colour, Double factor) -> Integer

Thread safety: SAFE
Returns a new colour with the saturation (HSB) multiplied by factor. Relative
operation: 0.0 produces greyscale, 1.0 unchanged, >1.0 increases saturation.
Factor clamped to >= 0.0 via jmax (no upper bound; result clamped internally).
Hue, brightness, and alpha preserved.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps factor: jmax(0.0f, factor)
  Delegates to JUCE Colour::withMultipliedSaturation(factor) -> getARGB()

Pair with:
  withSaturation -- absolute saturation replacement
  withMultipliedBrightness -- often adjusted alongside saturation

Source:
  ScriptingApi.cpp:7032  ScriptingApi::Colours::withMultipliedSaturation()
    -> c.withMultipliedSaturation(jmax(0.0f, factor)).getARGB()
