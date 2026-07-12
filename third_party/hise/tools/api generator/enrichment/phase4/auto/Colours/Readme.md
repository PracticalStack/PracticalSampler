<!-- Diagram triage:
  - No diagram specifications exist for this class or its methods.
-->

# Colours

Colours is a global utility namespace for working with ARGB colour values in HISEScript. It provides 127 named colour constants (the CSS/X11 set with British spelling) and a set of pure functions for colour manipulation. The methods fall into four groups:

1. **Transparency and mixing** - `withAlpha` and `mix` for adjusting opacity and blending between two colours.
2. **HSB component adjustment** - `withHue`, `withSaturation`, `withBrightness` for setting absolute values, and `withMultipliedAlpha`, `withMultipliedBrightness`, `withMultipliedSaturation` for scaling proportionally.
3. **Format conversion** - `toVec4`/`fromVec4` for RGBA float arrays (GLSL-compatible), and `toHsl`/`fromHsl` for HSL decomposition and reconstruction.

The typical workflow is to define a small set of theme colours as `const var` values at file scope, then derive all visual variants (hover states, disabled states, glow effects) using Colours methods inside look-and-feel paint callbacks.

```js
const var ACCENT = 0xFFC9EAF1;
var hover = Colours.mix(ACCENT, Colours.white, 0.25);
var dimmed = Colours.withAlpha(ACCENT, 0.4);
```

All methods accept flexible colour input - an ARGB integer, a hex string, or a named constant such as `Colours.dodgerblue`. All methods return ARGB integer values.

> [!Tip:British spelling for named constants] Named constants use British spelling where applicable (`Colours.grey`, `Colours.darkgrey`, `Colours.lightgrey`). Note that `Colours.darkgrey` (`0xFF555555`) is darker than the CSS standard `darkgray` (`0xFFA9A9A9`).

## Common Mistakes

- **Scale alpha for toHsl/fromHsl roundtrip**
  **Wrong:** `Colours.fromHsl(Colours.toHsl(c))` for a roundtrip
  **Right:** `var hsl = Colours.toHsl(c); hsl[3] = Math.round(hsl[3] * 255); Colours.fromHsl(hsl);`
  *`toHsl` returns alpha as a 0.0-1.0 float, but `fromHsl` expects alpha as a 0-255 integer. Passing the float directly truncates it to 0, making the colour fully transparent.*

- **Use Colours.mix for smooth hover blending**
  **Wrong:** `g.setColour(obj.hover ? highlightColour : baseColour)`
  **Right:** `g.setColour(Colours.mix(baseColour, Colours.white, obj.hover * 0.25))`
  *A ternary causes a hard colour switch. Mixing with the hover flag as blend factor keeps highlight intensity consistent and works smoothly when `obj.hover` is a float animation state.*

- **Centralise theme colours with constants**
  **Wrong:** Hardcoding `0xFF` alpha variants throughout LAF functions
  **Right:** Defining `const var ACCENT = 0xFFC9EAF1;` once and deriving variants with `Colours.withAlpha(ACCENT, 0.5)`
  *Centralising theme colours makes global colour changes trivial and prevents inconsistencies between LAF functions.*
