Colours (namespace)

Stateless namespace providing 127 named CSS colour constants (British spelling)
and 12 pure functions for ARGB colour manipulation, format conversion (vec4, HSL),
and linear mixing. All methods accept flexible colour input (integer, hex string,
or named constant) and return ARGB uint32 values as integers.

Constants:
  Transparency:
    transparentBlack = 0x00000000    Fully transparent black
    transparentWhite = 0x00FFFFFF    Fully transparent white
  Core:
    black = 0xFF000000    Black
    white = 0xFFFFFFFF    White
    red = 0xFFFF0000      Red
    green = 0xFF008000    Green
    blue = 0xFF0000FF     Blue
    grey = 0xFF808080     Grey
    yellow = 0xFFFFFF00   Yellow
  Named:
    127 CSS/X11 named colours (e.g., dodgerblue, coral, darkslategrey).
    Note: darkgrey = 0xFF555555 (HISE-specific, darker than CSS darkgray 0xA9A9A9).

Complexity tiers:
  1. Basic theming: withAlpha, named constants (Colours.white, Colours.black).
     Covers the majority of LAF colour needs.
  2. Interactive highlights: + mix, withMultipliedAlpha. Hover/press colour
     transitions and relative fading in custom LAF paint functions.
  3. Colour generation: + withHue, withSaturation, withBrightness. Programmatic
     colour creation from indices or data for per-item colour assignment.
  4. Format conversion: + toVec4/fromVec4 for GLSL interop, toHsl/fromHsl for
     perceptual manipulation. Specialized use cases.

Practical defaults:
  - Use Colours.withAlpha(colour, alpha) as the primary way to add transparency.
    It is the most commonly called method in the namespace.
  - For hover effects, Colours.mix(baseColour, Colours.white, obj.hover * 0.25)
    is the standard idiom. Works with float hover state without branching.
  - Define theme colours as const var at file scope and derive variations using
    Colours methods rather than hardcoding hex values throughout LAF functions.
  - When generating colours from an index, use withHue(Colours.red, index / count)
    to distribute hues evenly across the colour wheel.

Common mistakes:
  - Roundtripping fromHsl(toHsl(c)) without alpha correction -- toHsl outputs
    alpha as 0.0-1.0 float, but fromHsl casts alpha as (uint8)(int), truncating
    fractional values to 0 (transparent). Fix: multiply hsl[3] by 255 first.
  - Using a ternary for hover colour instead of mix -- causes a hard colour
    switch. Mixing with the hover flag as blend factor produces smooth transitions.
  - Hardcoding alpha variants throughout LAF functions instead of defining a single
    const var theme colour and deriving variants with withAlpha.

Example:
  // Named constant with alpha adjustment
  var semiRed = Colours.withAlpha(Colours.red, 0.5);

  // HSL manipulation
  var hsl = Colours.toHsl(Colours.dodgerblue);
  hsl[0] += 0.1; // shift hue
  var shifted = Colours.fromHsl([hsl[0], hsl[1], hsl[2], 255]);

  // Colour mixing
  var blend = Colours.mix(Colours.blue, Colours.green, 0.5);

Methods (12):
  fromHsl               fromVec4
  mix                   toHsl
  toVec4                withAlpha
  withBrightness        withHue
  withMultipliedAlpha   withMultipliedBrightness
  withMultipliedSaturation  withSaturation
