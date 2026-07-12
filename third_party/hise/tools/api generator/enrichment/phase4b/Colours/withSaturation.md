Colours::withSaturation(Colour colour, Double saturation) -> Integer

Thread safety: SAFE
Returns a new colour with the saturation (HSB) replaced. 0.0 produces greyscale,
1.0 fully saturated. Hue, brightness, and alpha preserved. Clamped to 0.0-1.0
via jlimit. Operates in HSB, not HSL.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps saturation: jlimit(0.0f, 1.0f, saturation)
  Delegates to JUCE Colour::withSaturation(clampedValue) -> getARGB()

Pair with:
  withMultipliedSaturation -- relative saturation scaling
  withHue / withBrightness -- typically adjusted together for colour generation

Source:
  ScriptingApi.cpp:7011  ScriptingApi::Colours::withSaturation()
    -> c.withSaturation(jlimit(0.0f, 1.0f, saturation)).getARGB()
