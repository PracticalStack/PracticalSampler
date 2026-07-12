# Colours -- Project Context

## Project Context

### Real-World Use Cases
- **LAF colour theming**: Plugins with custom look-and-feel functions use `withAlpha` and `mix` extensively to create hover states, active/inactive transitions, and transparency variations from a centralized colour palette. A typical plugin defines a small set of theme colours as `const var` values (e.g., `ACCENT_COLOUR`, `PASSIVE_ALPHA`) and derives all visual states from those constants using Colours methods.
- **Colour cycling from data**: Plugins that assign per-item colours (e.g., assigning a unique colour to each channel, note, or category) use `withHue` to cycle evenly through the colour wheel based on an index, then `withSaturation` and `withBrightness` to soften the result for UI use.
- **GLSL shader bridge**: Plugins with GPU-accelerated visuals use `toVec4` to convert ARGB integer colours into the `[r, g, b, a]` float array format required by `ScriptShader.setUniformData`.
- **Value-proportional colour modulation**: Knobs and meters that change colour intensity based on their value use `withMultipliedSaturation` or `withMultipliedBrightness` to scale the colour proportionally, creating a visual link between the parameter value and the colour intensity.

### Complexity Tiers
1. **Basic theming** (most common): `withAlpha` for transparency, named constants (`Colours.white`, `Colours.black`). Covers the majority of LAF colour needs.
2. **Interactive highlights**: `mix` for hover/press colour transitions, `withMultipliedAlpha` for relative fading. Used in any plugin with custom LAF paint functions.
3. **Colour generation**: `withHue`, `withSaturation`, `withBrightness` for programmatic colour creation from indices or data. Used in plugins with dynamic per-item colour assignment.
4. **Format conversion**: `toVec4`/`fromVec4` for GLSL interop, `toHsl`/`fromHsl` for perceptual manipulation. Specialized use cases.

### Practical Defaults
- Use `Colours.withAlpha(colour, alpha)` as the primary way to add transparency to a named constant or theme colour. It is the single most commonly called method in the namespace.
- For hover effects, `Colours.mix(baseColour, Colours.white, obj.hover * 0.25)` is a standard idiom. Multiplying by `obj.hover` (0 or 1) makes the mix factor 0.0 when not hovering and 0.25 when hovering, producing a smooth highlight without branching. A constant like `HOVER_ALPHA = 0.25` keeps the highlight intensity consistent across all controls.
- Define theme colours as `const var` at namespace or file scope and derive variations using Colours methods rather than hardcoding hex values throughout LAF functions.
- When generating colours from an index, start with `Colours.red` as the base and use `withHue(Colours.red, index / count)` to distribute hues evenly.

### Integration Patterns
- `Colours.withAlpha()` / `Colours.mix()` -> `Graphics.setColour()` -- the dominant integration. Nearly every LAF paint function uses Colours methods to compute the colour before passing it to the Graphics drawing context.
- `Colours.toVec4()` -> `ScriptShader.setUniformData()` -- converts a colour for use as a GLSL `vec4` uniform in GPU-rendered ScriptPanel paint routines.
- `Colours.withMultipliedBrightness()` -> `const var` -- used at init time to pre-compute LED states or palette variations that are referenced repeatedly during painting.
- `Colours.withAlpha()` -> `Graphics.drawDropShadow()` / `Graphics.addDropShadowFromAlpha()` -- shadow colours are typically the theme accent colour with reduced alpha.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| `g.setColour(obj.hover ? highlightColour : baseColour)` | `g.setColour(Colours.mix(baseColour, Colours.white, obj.hover * 0.25))` | Using a ternary for hover causes a hard colour switch. Mixing with the hover flag as blend factor produces a cleaner transition and keeps highlight intensity consistent when `obj.hover` is a float (e.g., animation state). |
| Hardcoding `0xFF` alpha variants throughout LAF functions | Defining `const var ACCENT = 0xFFC9EAF1;` once and deriving variants with `Colours.withAlpha(ACCENT, 0.5)` | Centralizing theme colours makes global colour changes trivial and prevents inconsistencies between LAF functions. |
