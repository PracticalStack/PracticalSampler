Colours::fromHsl(Array hsl) -> Integer

Thread safety: SAFE
Converts an HSL+alpha array to an ARGB integer colour value. Takes [h, s, l, a]
where h/s/l are 0.0-1.0 floats and alpha is a 0-255 integer. Returns 0
(transparent black) if input is not a four-element array.

Dispatch/mechanics:
  Validates array size == 4
  Casts alpha as (uint8)(int)hsl[3] -- truncates fractional floats to 0
  Delegates to JUCE Colour::fromHSL(h, s, l, alpha) -> getARGB()

Pair with:
  toHsl -- decompose a colour into HSL components (but alpha needs correction for roundtrip)

Anti-patterns:
  - Do NOT pass toHsl output directly to fromHsl -- toHsl returns alpha as 0.0-1.0
    float, but fromHsl casts it as (uint8)(int) which truncates 0.5 to 0 (transparent).
    Fix: hsl[3] = Math.round(hsl[3] * 255) before calling fromHsl.
  - Invalid input (non-array or wrong element count) silently returns 0 with no error.

Source:
  ScriptingApi.cpp:7060  ScriptingApi::Colours::fromHsl()
    -> validates hsl.isArray() && hsl.size() == 4
    -> Colour().fromHSL((float)hsl[0], (float)hsl[1], (float)hsl[2], (uint8)(int)hsl[3])
    -> getARGB()
