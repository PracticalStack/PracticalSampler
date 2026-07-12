Colours::mix(Colour colour1, Colour colour2, Double alpha) -> Integer

Thread safety: SAFE
Linearly interpolates between two colours in ARGB space. alpha=0.0 returns
colour1, alpha=1.0 returns colour2. All four channels interpolated independently.

Dispatch/mechanics:
  Parses both colours via getCleanedObjectColour
  Delegates to JUCE Colour::interpolatedWith(c2, alpha) -> getARGB()
  alpha is NOT clamped -- values outside 0.0-1.0 extrapolate (may overflow bytes)

Pair with:
  withAlpha -- when you need transparency rather than blending two colours

Anti-patterns:
  - Do NOT pass alpha values outside 0.0-1.0 -- unlike the with* methods, mix does
    not clamp. Out-of-range values cause byte overflow producing unexpected colours.

Source:
  ScriptingApi.cpp:7069  ScriptingApi::Colours::mix()
    -> c1.interpolatedWith(c2, alpha).getARGB()
