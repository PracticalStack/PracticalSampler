Colours::withBrightness(Colour colour, Double brightness) -> Integer

Thread safety: SAFE
Returns a new colour with the brightness (HSB value component) replaced.
0.0 produces black, 1.0 produces fully bright. Hue, saturation, and alpha
preserved. Clamped to 0.0-1.0 via jlimit. Operates in HSB, not HSL.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps brightness: jlimit(0.0f, 1.0f, brightness)
  Delegates to JUCE Colour::withBrightness(clampedValue) -> getARGB()

Pair with:
  withMultipliedBrightness -- relative brightness scaling
  withSaturation -- adjust saturation alongside brightness for consistent results

Source:
  ScriptingApi.cpp:7014  ScriptingApi::Colours::withBrightness()
    -> c.withBrightness(jlimit(0.0f, 1.0f, brightness)).getARGB()
