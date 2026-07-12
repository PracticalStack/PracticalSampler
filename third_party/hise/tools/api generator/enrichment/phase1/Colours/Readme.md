# Colours -- Class Analysis

## Brief
Stateless namespace for ARGB colour manipulation, format conversion, and 127 named CSS colour constants.

## Purpose
Colours is a global utility namespace that provides 127 named colour constants (CSS/X11 set with British spelling) and 12 stateless pure functions for colour manipulation. Methods cover HSB component adjustment (absolute and multiplied), format conversion between ARGB integers and float arrays (vec4 for GLSL, HSL for perceptual manipulation), and linear colour mixing. All methods accept flexible colour input (integer, hex string, or named constant) and return ARGB uint32 values as integers.

## obtainedVia
Global namespace -- accessed directly as `Colours` from any script (no factory method).

## minimalObjectToken


## Constants
| Name | Value | Type | Description | Group |
|------|-------|------|-------------|-------|
| transparentBlack | 0x00000000 | int | Fully transparent black | Transparency |
| transparentWhite | 0x00FFFFFF | int | Fully transparent white | Transparency |
| aliceblue | 0xFFF0F8FF | int | Alice blue | Named |
| antiquewhite | 0xFFFAEBD7 | int | Antique white | Named |
| aqua | 0xFF00FFFF | int | Aqua (same as cyan) | Named |
| aquamarine | 0xFF7FFFD4 | int | Aquamarine | Named |
| azure | 0xFFF0FFFF | int | Azure | Named |
| beige | 0xFFF5F5DC | int | Beige | Named |
| bisque | 0xFFFFE4C4 | int | Bisque | Named |
| black | 0xFF000000 | int | Black | Core |
| blanchedalmond | 0xFFFFEBCD | int | Blanched almond | Named |
| blue | 0xFF0000FF | int | Blue | Core |
| blueviolet | 0xFF8A2BE2 | int | Blue violet | Named |
| brown | 0xFFA52A2A | int | Brown | Named |
| burlywood | 0xFFDEB887 | int | Burlywood | Named |
| cadetblue | 0xFF5F9EA0 | int | Cadet blue | Named |
| chartreuse | 0xFF7FFF00 | int | Chartreuse | Named |
| chocolate | 0xFFD2691E | int | Chocolate | Named |
| coral | 0xFFFF7F50 | int | Coral | Named |
| cornflowerblue | 0xFF6495ED | int | Cornflower blue | Named |
| cornsilk | 0xFFFFF8DC | int | Cornsilk | Named |
| crimson | 0xFFDC143C | int | Crimson | Named |
| cyan | 0xFF00FFFF | int | Cyan (same as aqua) | Named |
| darkblue | 0xFF00008B | int | Dark blue | Named |
| darkcyan | 0xFF008B8B | int | Dark cyan | Named |
| darkgoldenrod | 0xFFB8860B | int | Dark goldenrod | Named |
| darkgrey | 0xFF555555 | int | Dark grey (HISE-specific, darker than CSS darkgray 0xA9A9A9) | Named |
| darkgreen | 0xFF006400 | int | Dark green | Named |
| darkkhaki | 0xFFBDB76B | int | Dark khaki | Named |
| darkmagenta | 0xFF8B008B | int | Dark magenta | Named |
| darkolivegreen | 0xFF556B2F | int | Dark olive green | Named |
| darkorange | 0xFFFF8C00 | int | Dark orange | Named |
| darkorchid | 0xFF9932CC | int | Dark orchid | Named |
| darkred | 0xFF8B0000 | int | Dark red | Named |
| darksalmon | 0xFFE9967A | int | Dark salmon | Named |
| darkseagreen | 0xFF8FBC8F | int | Dark sea green | Named |
| darkslateblue | 0xFF483D8B | int | Dark slate blue | Named |
| darkslategrey | 0xFF2F4F4F | int | Dark slate grey | Named |
| darkturquoise | 0xFF00CED1 | int | Dark turquoise | Named |
| darkviolet | 0xFF9400D3 | int | Dark violet | Named |
| deeppink | 0xFFFF1493 | int | Deep pink | Named |
| deepskyblue | 0xFF00BFFF | int | Deep sky blue | Named |
| dimgrey | 0xFF696969 | int | Dim grey | Named |
| dodgerblue | 0xFF1E90FF | int | Dodger blue | Named |
| firebrick | 0xFFB22222 | int | Firebrick | Named |
| floralwhite | 0xFFFFFAF0 | int | Floral white | Named |
| forestgreen | 0xFF228B22 | int | Forest green | Named |
| fuchsia | 0xFFFF00FF | int | Fuchsia (same as magenta) | Named |
| gainsboro | 0xFFDCDCDC | int | Gainsboro | Named |
| gold | 0xFFFFD700 | int | Gold | Named |
| goldenrod | 0xFFDAA520 | int | Goldenrod | Named |
| grey | 0xFF808080 | int | Grey | Core |
| green | 0xFF008000 | int | Green | Core |
| greenyellow | 0xFFADFF2F | int | Green yellow | Named |
| honeydew | 0xFFF0FFF0 | int | Honeydew | Named |
| hotpink | 0xFFFF69B4 | int | Hot pink | Named |
| indianred | 0xFFCD5C5C | int | Indian red | Named |
| indigo | 0xFF4B0082 | int | Indigo | Named |
| ivory | 0xFFFFFFF0 | int | Ivory | Named |
| khaki | 0xFFF0E68C | int | Khaki | Named |
| lavender | 0xFFE6E6FA | int | Lavender | Named |
| lavenderblush | 0xFFFFF0F5 | int | Lavender blush | Named |
| lemonchiffon | 0xFFFFFACD | int | Lemon chiffon | Named |
| lightblue | 0xFFADD8E6 | int | Light blue | Named |
| lightcoral | 0xFFF08080 | int | Light coral | Named |
| lightcyan | 0xFFE0FFFF | int | Light cyan | Named |
| lightgoldenrodyellow | 0xFFFAFAD2 | int | Light goldenrod yellow | Named |
| lightgreen | 0xFF90EE90 | int | Light green | Named |
| lightgrey | 0xFFD3D3D3 | int | Light grey | Named |
| lightpink | 0xFFFFB6C1 | int | Light pink | Named |
| lightsalmon | 0xFFFFA07A | int | Light salmon | Named |
| lightseagreen | 0xFF20B2AA | int | Light sea green | Named |
| lightskyblue | 0xFF87CEFA | int | Light sky blue | Named |
| lightslategrey | 0xFF778899 | int | Light slate grey | Named |
| lightsteelblue | 0xFFB0C4DE | int | Light steel blue | Named |
| lightyellow | 0xFFFFFFE0 | int | Light yellow | Named |
| lime | 0xFF00FF00 | int | Lime | Named |
| limegreen | 0xFF32CD32 | int | Lime green | Named |
| linen | 0xFFFAF0E6 | int | Linen | Named |
| magenta | 0xFFFF00FF | int | Magenta (same as fuchsia) | Named |
| maroon | 0xFF800000 | int | Maroon | Named |
| mediumaquamarine | 0xFF66CDAA | int | Medium aquamarine | Named |
| mediumblue | 0xFF0000CD | int | Medium blue | Named |
| mediumorchid | 0xFFBA55D3 | int | Medium orchid | Named |
| mediumpurple | 0xFF9370DB | int | Medium purple | Named |
| mediumseagreen | 0xFF3CB371 | int | Medium sea green | Named |
| mediumslateblue | 0xFF7B68EE | int | Medium slate blue | Named |
| mediumspringgreen | 0xFF00FA9A | int | Medium spring green | Named |
| mediumturquoise | 0xFF48D1CC | int | Medium turquoise | Named |
| mediumvioletred | 0xFFC71585 | int | Medium violet red | Named |
| midnightblue | 0xFF191970 | int | Midnight blue | Named |
| mintcream | 0xFFF5FFFA | int | Mint cream | Named |
| mistyrose | 0xFFFFE4E1 | int | Misty rose | Named |
| navajowhite | 0xFFFFDEAD | int | Navajo white | Named |
| navy | 0xFF000080 | int | Navy | Named |
| oldlace | 0xFFFDF5E6 | int | Old lace | Named |
| olive | 0xFF808000 | int | Olive | Named |
| olivedrab | 0xFF6B8E23 | int | Olive drab | Named |
| orange | 0xFFFFA500 | int | Orange | Named |
| orangered | 0xFFFF4500 | int | Orange red | Named |
| orchid | 0xFFDA70D6 | int | Orchid | Named |
| palegoldenrod | 0xFFEEE8AA | int | Pale goldenrod | Named |
| palegreen | 0xFF98FB98 | int | Pale green | Named |
| paleturquoise | 0xFFAFEEEE | int | Pale turquoise | Named |
| palevioletred | 0xFFDB7093 | int | Pale violet red | Named |
| papayawhip | 0xFFFFEFD5 | int | Papaya whip | Named |
| peachpuff | 0xFFFFDAB9 | int | Peach puff | Named |
| peru | 0xFFCD853F | int | Peru | Named |
| pink | 0xFFFFC0CB | int | Pink | Named |
| plum | 0xFFDDA0DD | int | Plum | Named |
| powderblue | 0xFFB0E0E6 | int | Powder blue | Named |
| purple | 0xFF800080 | int | Purple | Named |
| red | 0xFFFF0000 | int | Red | Core |
| rosybrown | 0xFFBC8F8F | int | Rosy brown | Named |
| royalblue | 0xFF4169E1 | int | Royal blue | Named |
| saddlebrown | 0xFF8B4513 | int | Saddle brown | Named |
| salmon | 0xFFFA8072 | int | Salmon | Named |
| sandybrown | 0xFFF4A460 | int | Sandy brown | Named |
| seagreen | 0xFF2E8B57 | int | Sea green | Named |
| seashell | 0xFFFFF5EE | int | Seashell | Named |
| sienna | 0xFFA0522D | int | Sienna | Named |
| silver | 0xFFC0C0C0 | int | Silver | Named |
| skyblue | 0xFF87CEEB | int | Sky blue | Named |
| slateblue | 0xFF6A5ACD | int | Slate blue | Named |
| slategrey | 0xFF708090 | int | Slate grey | Named |
| snow | 0xFFFFFAFA | int | Snow | Named |
| springgreen | 0xFF00FF7F | int | Spring green | Named |
| steelblue | 0xFF4682B4 | int | Steel blue | Named |
| tan | 0xFFD2B48C | int | Tan | Named |
| teal | 0xFF008080 | int | Teal | Named |
| thistle | 0xFFD8BFD8 | int | Thistle | Named |
| tomato | 0xFFFF6347 | int | Tomato | Named |
| turquoise | 0xFF40E0D0 | int | Turquoise | Named |
| violet | 0xFFEE82EE | int | Violet | Named |
| wheat | 0xFFF5DEB3 | int | Wheat | Named |
| white | 0xFFFFFFFF | int | White | Core |
| whitesmoke | 0xFFF5F5F5 | int | White smoke | Named |
| yellow | 0xFFFFFF00 | int | Yellow | Core |
| yellowgreen | 0xFF9ACD32 | int | Yellow green | Named |

## Dynamic Constants

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `Colours.fromHsl(Colours.toHsl(c))` | `var hsl = Colours.toHsl(c); hsl[3] = Math.round(hsl[3] * 255); Colours.fromHsl(hsl);` | `fromHsl` casts the alpha element as `(uint8)(int)`, so a 0.0-1.0 float alpha from `toHsl` is truncated (e.g. 0.5 becomes 0). Pass alpha as 0-255 integer for correct roundtrip. |

## codeExample
```javascript
// Named constant with alpha adjustment
var semiRed = Colours.withAlpha(Colours.red, 0.5);

// HSL manipulation
var hsl = Colours.toHsl(Colours.dodgerblue);
hsl[0] += 0.1; // shift hue
var shifted = Colours.fromHsl([hsl[0], hsl[1], hsl[2], 255]);

// Colour mixing
var blend = Colours.mix(Colours.blue, Colours.green, 0.5);
```

## Alternatives
Graphics -- uses Colours output values for drawing operations via setColour().

## Related Preprocessors
None.

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: All methods are stateless pure functions with no timeline dependencies, no preconditions, and no silent-failure modes beyond the documented fromHsl alpha asymmetry. Diagnostics would not add value.
