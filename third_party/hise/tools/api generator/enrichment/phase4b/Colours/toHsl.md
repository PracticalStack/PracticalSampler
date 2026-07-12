Colours::toHsl(Colour colour) -> Array

Thread safety: SAFE
Decomposes a colour into [h, s, l, a] where all values are 0.0-1.0 floats. Hue
is normalized (0.0=red, 0.333=green, 0.667=blue). Alpha is extracted via
getFloatAlpha().

Dispatch/mechanics:
  Parses colour via getCleanedObjectColour
  Calls JUCE Colour::getHSL(hue, saturation, lightness)
  Appends getFloatAlpha() -> returns Array<var> of 4 floats

Pair with:
  fromHsl -- convert back to ARGB integer (but alpha needs correction for roundtrip)

Anti-patterns:
  - Do NOT pass the output array directly to fromHsl if the colour has partial
    transparency -- the alpha float (e.g. 0.5) will be truncated to 0 by fromHsl's
    (uint8)(int) cast. Multiply hsl[3] by 255 first.

Source:
  ScriptingApi.cpp:7050  ScriptingApi::Colours::toHsl()
    -> getCleanedObjectColour(colour)
    -> c.getHSL(hue, saturation, lightness)
    -> returns [hue, saturation, lightness, c.getFloatAlpha()]
