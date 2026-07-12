# Path -- Project Context

## Project Context

### Real-World Use Cases
- **Rotary knob arc rendering**: The most common use of Path in production plugins. A `drawRotarySlider` LAF callback creates arc paths from the knob's normalized value to draw background tracks, active value indicators, and modulation range overlays. This requires `addArc` with angles computed from the value, `startNewSubPath` for bounds anchoring, and `drawPath`/`fillPath` for rendering. Every plugin with custom knob visuals uses this pattern.
- **Icon libraries from serialized data**: Plugins maintain collections of vector icons (transport controls, navigation arrows, tool icons) stored as base64 strings or byte arrays. Paths are created once with `Content.createPath()` and populated via `loadFromData()` at initialization, then drawn repeatedly with `g.fillPath()` or `g.drawPath()` in paint routines. This avoids bitmap scaling artifacts and keeps icon data in script files rather than image assets.
- **Waveform shape icons**: Synthesizer plugins build waveform previews (sine, saw, square, triangle) as Path objects using `startNewSubPath`/`lineTo` sequences, then render them as selectable icons in the UI.
- **Custom combo box and tooltip shapes**: Paths define non-rectangular UI chrome - speech bubble outlines with arrow pointers (using `quadraticTo` for rounded corners), custom combo box frames with dropdown indicators, and other complex geometric shapes that `fillRoundedRectangle` cannot produce.
- **Envelope visualization**: AHDSR-style envelope editors build paths segment-by-segment using `quadraticTo` for curved attack/decay/release segments, then use `scaleToFit` to map normalized coordinates into the panel area and `getYAt` to find curve control point positions for interactive dragging.
- **Dashed and stroked arc effects**: Some knob styles use `createStrokedPath` to convert thin arcs into fillable geometry with configurable thickness, or to create dashed-line effects (tick marks, animated indicators) around rotary controls.

### Complexity Tiers
1. **Simple shapes** (most common): `startNewSubPath`, `lineTo`, `closeSubPath` for triangles, arrows, and icons. Also `addArc` for basic rotary knob backgrounds. No curves, no stroking.
2. **Rotary knob LAF** (very common): `addArc` with computed angles from `obj.valueNormalized`, `startNewSubPath` for bounds anchoring, `drawPath`/`fillPath` with stroke thickness. Requires understanding the JUCE angle convention (clockwise from 3 o'clock).
3. **Serialized icons**: `loadFromData` with base64 strings or byte arrays, `fillPath` with target area for rendering. Used for reusable icon libraries.
4. **Advanced geometry**: `quadraticTo` for smooth curves (envelope editors, tooltip bubbles), `createStrokedPath` for dashed lines and thick arc rendering, `scaleToFit` for coordinate space mapping, `getYAt` for interactive curve interrogation.

### Practical Defaults
- Use normalized coordinates `[0, 0, 1, 1]` for paths that will be rendered at different sizes via `g.fillPath(path, targetArea)` or `g.drawPath(path, targetArea, thickness)`. The Graphics methods scale the path to the target area automatically.
- For rotary knob arcs, an angular range of approximately 2.3-2.7 radians from center (giving a gap at the bottom) is the standard. Use `-ARC` to `+ARC` for the background track and `-ARC` to `-ARC + 2.0 * ARC * obj.valueNormalized` for the value arc.
- When building a path in normalized `[0,0,1,1]` space for use with `drawPath`, anchor the bounds with two `startNewSubPath` calls at `(0,0)` and `(1,1)` before adding the actual geometry. This ensures the path's bounding box covers the full unit square, preventing unexpected scaling when rendered.
- Use `createStrokedPath(thickness, [])` (empty array for solid) to convert an arc outline into a fillable shape when you need to apply gradients, shadows, or other fill effects to the stroke.
- Store icon paths as `const var` at initialization scope and reuse them across paint calls. Creating new Path objects inside paint routines works but is less efficient for static shapes.

### Integration Patterns
- `Content.createPath()` -> `Path.addArc()` / `Path.startNewSubPath()` + `Path.lineTo()` -> `Graphics.drawPath()` / `Graphics.fillPath()` -- The core creation-to-rendering chain used in every LAF callback and paint routine.
- `Path.addArc()` -> `Path.createStrokedPath()` -> `Graphics.fillPath()` -- Converting a thin arc into a thick fillable shape, commonly used for knob arcs that need gradient fills or inner shadows.
- `Path.createStrokedPath()` -> `Graphics.drawDropShadowFromPath()` -- Applying glow or shadow effects to stroked arc geometry.
- `Path.loadFromData(base64String)` -> `Graphics.fillPath(path, area)` -- Loading pre-built icon data and rendering it at a specific location and size.
- `Path.quadraticTo()` -> `Path.scaleToFit()` -> `Path.getYAt()` -- Building an envelope curve in normalized space, scaling to pixel coordinates, then querying Y positions for interactive control point placement.
- `Path.addArc([0,0,1,1], ...)` -> `Graphics.drawPath(path, obj.area, strokeObj)` -- Drawing an arc in normalized space and letting `drawPath` scale it to the component area, with a JSON stroke configuration object for thickness and cap style.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating Path objects inside paint routines for static shapes | Create paths as `const var` at init scope, reuse across paint calls | Paint routines run on every repaint. Creating new objects each time is wasteful for shapes that do not change. Only create paths inside paint routines when the geometry depends on the current value or state. |
| Using `addArc` on a path without anchoring bounds first | Call `startNewSubPath(0,0)` and `startNewSubPath(1,1)` before `addArc([0,0,1,1], ...)` | An arc alone has a bounding box that only covers the arc segment. When rendered with `g.drawPath(path, targetArea)`, the path is scaled from its bounds, which produces unexpected positioning. Anchoring to `(0,0)` and `(1,1)` ensures the full unit square is the reference frame. |
| Using `setBounds` expecting it to resize the path | Use `scaleToFit` to transform path geometry into a target area | `setBounds` only adds invisible anchor points to expand the reported bounding box. It does not transform or move any visible geometry. |
| Passing the stroke thickness as the `dotData` parameter | `createStrokedPath(thickness, [])` -- first param is thickness or stroke config, second is the dash array | The parameters are ordered `(strokeData, dotData)`. An empty array `[]` produces a solid stroke; a non-empty array like `[10, 5]` produces dashes. |
