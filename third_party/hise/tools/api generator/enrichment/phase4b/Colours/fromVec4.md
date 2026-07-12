Colours::fromVec4(Array vec4) -> Integer

Thread safety: SAFE
Converts an RGBA float array [r, g, b, a] (each 0.0-1.0) to an ARGB integer
colour value. Each float is multiplied by 255 and rounded. Returns 0 (transparent
black) if input is not a four-element array. Provides a lossless roundtrip with
toVec4 (within float rounding precision).

Dispatch/mechanics:
  Validates array size == 4
  Each component: (uint8)roundToInt((float)vec4[i] * 255.0f)
  Constructs Colour(r, g, b, a) -> getARGB()

Pair with:
  toVec4 -- inverse operation, lossless roundtrip

Anti-patterns:
  - Invalid input (non-array or wrong element count) silently returns 0 with no error.

Source:
  ScriptingApi.cpp:7048  ScriptingApi::Colours::fromVec4()
    -> validates vec4.isArray() && vec4.size() == 4
    -> roundToInt per channel -> Colour(r, g, b, a).getARGB()
