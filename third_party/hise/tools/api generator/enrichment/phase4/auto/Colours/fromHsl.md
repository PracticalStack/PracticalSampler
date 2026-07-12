Converts a four-element HSL array `[hue, saturation, lightness, alpha]` back into an ARGB integer colour. Hue, saturation, and lightness are 0.0-1.0 floats, but alpha must be a 0-255 integer. This asymmetry with `Colours.toHsl()` (which outputs alpha as a 0.0-1.0 float) means you need to scale alpha before passing the array back.

> [!Warning:Invalid input returns transparent black] Invalid input (wrong type or element count) silently returns `0` (transparent black) with no error message. Always ensure the array has exactly four elements.
