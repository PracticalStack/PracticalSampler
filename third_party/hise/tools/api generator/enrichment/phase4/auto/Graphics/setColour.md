Sets the current drawing colour for subsequent shape and text operations. The colour persists until the next `setColour` or `setGradientFill` call. Colours use `0xAARRGGBB` format - always include the alpha channel (`0xFF` prefix for fully opaque). Omitting it (e.g., `0xFF0000` instead of `0xFFFF0000`) produces a nearly-transparent colour because the alpha defaults to `0x00`.

Colour constants from the `Colours` namespace can also be used: `Colours.white`, `Colours.red`, `Colours.dodgerblue`, etc.
