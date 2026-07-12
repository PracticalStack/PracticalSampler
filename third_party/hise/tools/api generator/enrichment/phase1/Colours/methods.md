# Colours -- Method Analysis

## fromHsl

**Signature:** `Integer fromHsl(Array hsl)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var c = Colours.fromHsl([0.0, 1.0, 0.5, 255]);`

**Description:**
Converts an HSL colour array to an ARGB integer colour value. Takes a four-element array `[hue, saturation, lightness, alpha]` where hue, saturation, and lightness are floats in the 0.0-1.0 range. The alpha element is cast to `(uint8)(int)` before being passed to JUCE's `Colour::fromHSL`, which then promotes it back to float via implicit conversion. This means the alpha value must be an integer in the 0-255 range (not a 0.0-1.0 float) for correct results. Returns 0 (transparent black) if the input is not a four-element array.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| hsl | Array | no | A four-element array `[h, s, l, a]`. H, S, L are 0.0-1.0 floats. Alpha is 0-255 integer. | Must be exactly 4 elements |

**Pitfalls:**
- [BUG] The alpha element is cast as `(uint8)(int)`, which truncates fractional float values to 0. When `toHsl` outputs alpha as a 0.0-1.0 float (e.g., 0.5 for 50% transparency), passing that array directly to `fromHsl` produces alpha = 0 (fully transparent) because `(int)0.5` truncates to 0. Only `0.0` and values >= `1.0` survive the roundtrip. To use `fromHsl` with a `toHsl` result, multiply the alpha element by 255 first: `hsl[3] = Math.round(hsl[3] * 255)`.
- Invalid input (non-array or wrong element count) silently returns 0 (transparent black) with no error message.

**Cross References:**
- `$API.Colours.toHsl$`
- `$API.Colours.fromVec4$`

**Example:**
```javascript:fromhsl-alpha-correction
// Title: HSL roundtrip with alpha correction
var hsl = Colours.toHsl(Colours.dodgerblue);
hsl[0] += 0.1;
hsl[3] = Math.round(hsl[3] * 255);
var shifted = Colours.fromHsl(hsl);
```
```json:testMetadata:fromhsl-alpha-correction
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "shifted != Colours.dodgerblue", "value": true},
    {"type": "REPL", "expression": "Colours.toVec4(shifted)[3]", "value": 1.0}
  ]
}
```

## fromVec4

**Signature:** `Integer fromVec4(Array vec4)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var c = Colours.fromVec4([1.0, 0.0, 0.0, 1.0]);`

**Description:**
Converts an RGBA float array to an ARGB integer colour value. Takes a four-element array `[r, g, b, a]` where each component is a float in the 0.0-1.0 range. Each float is multiplied by 255 and rounded to the nearest integer to produce the byte value. Returns 0 (transparent black) if the input is not a four-element array. This is the inverse of `toVec4` and provides a lossless roundtrip (within rounding precision).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| vec4 | Array | no | A four-element array `[r, g, b, a]`, each 0.0-1.0 float | Must be exactly 4 elements |

**Pitfalls:**
- Invalid input (non-array or wrong element count) silently returns 0 (transparent black) with no error message.

**Cross References:**
- `$API.Colours.toVec4$`
- `$API.Colours.fromHsl$`

## mix

**Signature:** `Integer mix(Colour colour1, Colour colour2, Double alpha)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var blended = Colours.mix(Colours.blue, Colours.green, 0.5);`

**Description:**
Linearly interpolates between two colours in ARGB space. The `alpha` parameter controls the blend: 0.0 returns `colour1`, 1.0 returns `colour2`, and values in between produce a proportional mix. All four channels (alpha, red, green, blue) are interpolated independently. The `alpha` parameter is not clamped, so values outside 0.0-1.0 produce extrapolated results (which may overflow individual channel bytes).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour1 | Colour | no | The start colour (returned when alpha = 0.0) | ARGB integer, hex string, or named constant |
| colour2 | Colour | no | The end colour (returned when alpha = 1.0) | ARGB integer, hex string, or named constant |
| alpha | Double | no | Blend factor between the two colours | 0.0-1.0 typical; not clamped |

**Pitfalls:**
- The `alpha` parameter is not clamped to 0.0-1.0 (unlike the `with*` modifier methods). Values outside this range produce extrapolated channel values that wrap due to byte overflow, potentially yielding unexpected colours.

**Cross References:**
- `$API.Colours.withAlpha$`
- `$API.Colours.withBrightness$`
- `$API.Colours.withSaturation$`
- `$API.Colours.withHue$`

## toHsl

**Signature:** `Array toHsl(Colour colour)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var hsl = Colours.toHsl(Colours.dodgerblue);`

**Description:**
Decomposes a colour into its HSL (hue, saturation, lightness) components plus alpha. Returns a four-element float array `[h, s, l, a]` where all values are in the 0.0-1.0 range. Hue is normalized (0.0 = red, 0.333 = green, 0.667 = blue). The alpha channel is extracted via `getFloatAlpha()` and is also in the 0.0-1.0 range. Note that `fromHsl` expects alpha as a 0-255 integer due to an internal cast asymmetry -- see `fromHsl` for the workaround.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The colour to decompose | ARGB integer, hex string, or named constant |

**Pitfalls:**
- The output alpha is a 0.0-1.0 float, but the corresponding `fromHsl` method casts alpha as `(uint8)(int)`, creating a roundtrip asymmetry. Pass the array directly to `fromHsl` only if the alpha is 0.0 or 1.0; otherwise, multiply `hsl[3]` by 255 first.

**Cross References:**
- `$API.Colours.fromHsl$`
- `$API.Colours.toVec4$`

## toVec4

**Signature:** `Array toVec4(Colour colour)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var rgba = Colours.toVec4(Colours.red);`

**Description:**
Converts a colour to a four-element float array `[r, g, b, a]` where each component is in the 0.0-1.0 range. The output format is compatible with GLSL `vec4` uniforms. The `fromVec4`/`toVec4` pair provides a lossless roundtrip (within float rounding precision), unlike the `toHsl`/`fromHsl` pair which has an alpha asymmetry.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The colour to decompose | ARGB integer, hex string, or named constant |

**Cross References:**
- `$API.Colours.fromVec4$`
- `$API.Colours.toHsl$`

## withAlpha

**Signature:** `Integer withAlpha(Colour colour, Double alpha)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var semiRed = Colours.withAlpha(Colours.red, 0.5);`

**Description:**
Returns a new colour with the alpha channel replaced by the specified value. The `alpha` parameter is clamped to 0.0-1.0 via `jlimit`. All other channels (red, green, blue) are preserved from the input colour. This is the most direct way to add transparency to a named colour constant or existing colour value.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| alpha | Double | no | The new alpha value | Clamped to 0.0-1.0 |

**Cross References:**
- `$API.Colours.withMultipliedAlpha$`

## withBrightness

**Signature:** `Integer withBrightness(Colour colour, Double brightness)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var dark = Colours.withBrightness(Colours.red, 0.3);`

**Description:**
Returns a new colour with the brightness (HSB value component) replaced by the specified value. The brightness parameter is clamped to 0.0-1.0 via `jlimit`. A brightness of 0.0 produces black regardless of the input hue and saturation; 1.0 produces the fully bright version of the colour. All other HSB components and the alpha channel are preserved from the input colour. Operates in the HSB (hue-saturation-brightness) colour model, not HSL.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| brightness | Double | no | The new brightness value (HSB value component) | Clamped to 0.0-1.0 |

**Cross References:**
- `$API.Colours.withMultipliedBrightness$`
- `$API.Colours.withSaturation$`
- `$API.Colours.withHue$`

## withHue

**Signature:** `Integer withHue(Colour colour, Double hue)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var shifted = Colours.withHue(Colours.red, 0.33);`

**Description:**
Returns a new colour with the hue replaced by the specified value. The hue parameter is clamped to 0.0-1.0 via `jlimit`, where 0.0 and 1.0 both represent red, 0.333 is green, and 0.667 is blue. Saturation, brightness, and alpha are preserved from the input colour. This sets an absolute hue rather than shifting it -- to shift the hue relative to its current value, use `toHsl` to read the current hue, add an offset, and convert back.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| hue | Double | no | The new hue value | Clamped to 0.0-1.0 (0.0 = red, 0.333 = green, 0.667 = blue) |

**Cross References:**
- `$API.Colours.withSaturation$`
- `$API.Colours.withBrightness$`
- `$API.Colours.toHsl$`

## withMultipliedAlpha

**Signature:** `Integer withMultipliedAlpha(Colour colour, Double factor)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var faded = Colours.withMultipliedAlpha(Colours.red, 0.5);`

**Description:**
Returns a new colour with the alpha channel multiplied by the given factor. The factor is clamped to >= 0.0 via `jmax` (no upper bound). A factor of 0.0 makes the colour fully transparent, 1.0 leaves it unchanged, and values above 1.0 increase opacity (clamped to the 0-255 byte range by the underlying JUCE implementation). This is a relative operation -- unlike `withAlpha` which replaces the alpha with an absolute value, `withMultipliedAlpha` scales the existing alpha proportionally. Useful for fading a colour that may already have partial transparency.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| factor | Double | no | Multiplier for the current alpha value | >= 0.0 (no upper bound; result clamped to byte range) |

**Cross References:**
- `$API.Colours.withAlpha$`
- `$API.Colours.withMultipliedBrightness$`
- `$API.Colours.withMultipliedSaturation$`

## withMultipliedBrightness

**Signature:** `Integer withMultipliedBrightness(Colour colour, Double factor)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var darker = Colours.withMultipliedBrightness(Colours.dodgerblue, 0.7);`

**Description:**
Returns a new colour with the brightness (HSB value component) multiplied by the given factor. The factor is clamped to >= 0.0 via `jmax` (no upper bound). A factor of 0.0 produces black, 1.0 leaves the colour unchanged, and values above 1.0 increase brightness (clamped internally to the valid range). This is a relative operation -- unlike `withBrightness` which replaces the brightness with an absolute value, `withMultipliedBrightness` scales the existing brightness proportionally. Hue, saturation, and alpha are preserved.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| factor | Double | no | Multiplier for the current brightness value | >= 0.0 (no upper bound; result clamped internally) |

**Cross References:**
- `$API.Colours.withBrightness$`
- `$API.Colours.withMultipliedAlpha$`
- `$API.Colours.withMultipliedSaturation$`

## withMultipliedSaturation

**Signature:** `Integer withMultipliedSaturation(Colour colour, Double factor)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var muted = Colours.withMultipliedSaturation(Colours.red, 0.5);`

**Description:**
Returns a new colour with the saturation (HSB saturation component) multiplied by the given factor. The factor is clamped to >= 0.0 via `jmax` (no upper bound). A factor of 0.0 produces a fully desaturated (greyscale) colour, 1.0 leaves the colour unchanged, and values above 1.0 increase saturation (clamped internally to the valid range). This is a relative operation -- unlike `withSaturation` which replaces the saturation with an absolute value, `withMultipliedSaturation` scales the existing saturation proportionally. Hue, brightness, and alpha are preserved.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| factor | Double | no | Multiplier for the current saturation value | >= 0.0 (no upper bound; result clamped internally) |

**Cross References:**
- `$API.Colours.withSaturation$`
- `$API.Colours.withMultipliedAlpha$`
- `$API.Colours.withMultipliedBrightness$`

## withSaturation

**Signature:** `Integer withSaturation(Colour colour, Double saturation)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var grey = Colours.withSaturation(Colours.red, 0.0);`

**Description:**
Returns a new colour with the saturation (HSB saturation component) replaced by the specified value. The saturation parameter is clamped to 0.0-1.0 via `jlimit`. A saturation of 0.0 produces a fully desaturated (greyscale) colour, and 1.0 produces the fully saturated version. Hue, brightness, and alpha are preserved from the input colour. Operates in the HSB (hue-saturation-brightness) colour model.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| colour | Colour | no | The base colour | ARGB integer, hex string, or named constant |
| saturation | Double | no | The new saturation value | Clamped to 0.0-1.0 |

**Cross References:**
- `$API.Colours.withMultipliedSaturation$`
- `$API.Colours.withHue$`
- `$API.Colours.withBrightness$`
