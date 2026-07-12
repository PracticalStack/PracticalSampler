Colours::withHue(Colour colour, Double hue) -> Integer

Thread safety: SAFE
Returns a new colour with the hue replaced. Clamped to 0.0-1.0 via jlimit
(0.0=red, 0.333=green, 0.667=blue). Saturation, brightness, and alpha preserved.
Sets absolute hue -- to shift relative to current hue, use toHsl, add offset,
convert back.

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Clamps hue: jlimit(0.0f, 1.0f, hue)
  Delegates to JUCE Colour::withHue(clampedHue) -> getARGB()

Pair with:
  withSaturation / withBrightness -- typically adjusted together for programmatic colour generation
  toHsl -- read current hue for relative shifting

Source:
  ScriptingApi.cpp:7008  ScriptingApi::Colours::withHue()
    -> c.withHue(jlimit(0.0f, 1.0f, hue)).getARGB()
