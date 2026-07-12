Converts a four-element RGBA float array `[r, g, b, a]` back into an ARGB integer colour. Each component should be in the 0.0-1.0 range. This is the inverse of `Colours.toVec4()` and provides a lossless roundtrip within float rounding precision.

> [!Warning:Invalid input returns transparent black] Invalid input (wrong type or element count) silently returns `0` (transparent black) with no error message.
