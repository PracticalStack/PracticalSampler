# Graphics -- Project Context

## Project Context

### Real-World Use Cases
- **LAF draw callbacks**: The dominant use of the Graphics API is inside `ScriptLookAndFeel` registered functions (`drawRotarySlider`, `drawToggleButton`, `drawComboBox`, etc.). A typical plugin registers dozens to hundreds of LAF draw functions, each receiving `g` as their first parameter and the `obj` callback properties as the second. The Graphics object draws the entire visual appearance of standard UI components - knobs, buttons, combo boxes, sliders - replacing the default rendering.
- **ScriptPanel paint routines**: The second major use case is inside `setPaintRoutine(function(g) {...})` for custom-drawn panels - level meters, sequencer grids, background textures, loading spinners, and interactive controls like XY pads. These are not replacing existing components but creating entirely new visual elements.
- **Reusable drawing helpers**: Production plugins extract common drawing patterns into helper functions (e.g., `drawCircleHandle(g, area, colour, hover)`) that accept the Graphics context as a parameter. This lets multiple LAF callbacks and paint routines share drawing logic without duplication.

### Complexity Tiers
1. **Basic rendering** (most common): `fillAll`, `setColour`, `fillRect`, `fillRoundedRectangle`, `setFont`/`setFontWithSpacing`, `drawAlignedText`. Sufficient for simple button and panel backgrounds with text labels.
2. **Path-based controls**: Adds `fillPath`, `drawPath` (with stroke JSON), `fillEllipse`, `drawEllipse`, `drawDropShadow`, `fillTriangle`, `setGradientFill`. Required for arc-based rotary knobs, icon buttons, and slider tracks with gradients.
3. **Compositing and effects**: Adds `beginLayer`/`endLayer`, `addDropShadowFromAlpha`, `gaussianBlur`/`boxBlur`, `drawImage`, `rotate`. Used for glowing elements, blurred backgrounds, filmstrip rendering, and rotary knob position indicators.

### Practical Defaults
- Use `setFontWithSpacing` over `setFont` when rendering labels inside LAF callbacks - the extra spacing parameter gives fine control over letter-spacing that makes compact labels readable.
- Pre-compute colour constants (e.g., `const var WHITE_55 = Colours.withAlpha(Colours.white, 0.55)`) at init time rather than calling `Colours.withAlpha()` inside every paint callback. This is both cleaner and avoids repeated allocations.
- Use `Colours.mix(baseColour, Colours.white, obj.hover * 0.15)` as the standard hover highlight pattern. Multiplying by the boolean `obj.hover` (0 or 1) and a small alpha factor gives a subtle brightening on hover without branching.
- Use `fillAll(colour)` as the first call in a paint routine rather than `setColour(colour); fillRect([0, 0, w, h])` - it is cleaner and does not require a prior `setColour` call.

### Integration Patterns
- `Content.createPath()` -> `Path.addArc()` -> `Graphics.drawPath()` -- The standard rotary knob pattern: create a Path outside the paint callback, add an arc with start/end angles computed from `obj.valueNormalized`, then stroke it with `drawPath`. The path object can be created once and reused, or created fresh each callback (both patterns are used in practice).
- `ScriptLookAndFeel.loadImage()` -> `Graphics.drawImage()` -- Images must be loaded via the LAF object (or panel) before they can be drawn. The `yOffset` parameter selects filmstrip frames: `g.drawImage("knob.png", obj.area, 0, parseInt(obj.valueNormalized * 127) * frameHeight)`.
- `Graphics.rotate()` -> draw indicator -> `Graphics.rotate()` (negate) -- The idiomatic knob position indicator pattern: rotate the canvas by the knob's radian value, draw a line or dot at the 12 o'clock position, then rotate back by the negative angle to restore the canvas for subsequent drawing.
- `Graphics.beginLayer(false)` -> draw content -> `Graphics.addDropShadowFromAlpha()` -> `Graphics.endLayer()` -- Creates a glow effect: draw shapes on a transparent layer, then generate a shadow from their alpha channel. The result is composited onto the parent canvas with the glow extending around the drawn shapes.
- `Graphics.setGradientFill()` -> `Graphics.fillRoundedRectangle()` -- Vertical gradients for metallic or glossy surface effects. The gradient replaces `setColour` as the current fill, applying to all subsequent fill operations.
- `Rect.withSizeKeepingCentre()` -> `Graphics.fillEllipse()` -- The Rectangle utility class is used extensively alongside Graphics for layout calculations. Centring, reducing, slicing, and translating rectangles before passing them to drawing methods is the standard pattern.

## Common Mistakes
| Wrong | Right | Explanation |
|-------|-------|-------------|
| Creating Path objects outside LAF callbacks and storing them in `const var` | Creating Path objects inside the callback or at namespace scope with `Content.createPath()` | Path objects used in LAF callbacks must be accessible from the callback scope. Creating them in `onInit` and referencing by closure is fine, but the path must be reset each frame if it accumulates geometry (e.g., `addArc` calls). |
| `g.drawPath(arcPath, area, 2.0)` without startNewSubPath bounds | Set bounds with `path.startNewSubPath(0.0, 0.0); path.startNewSubPath(1.0, 1.0);` before `addArc` | When using `drawPath` with an area parameter, the path is scaled to fit. Without explicit bounding sub-paths at (0,0) and (1,1), the path's natural bounds control the scaling, which produces unexpected results for arcs that don't span the full unit square. |
| Calling `g.setColour()` once at the top of a complex LAF callback | Call `g.setColour()` before each group of drawing operations that needs a different colour | The colour state persists across all drawing calls. Forgetting to reset it after drawing a shadow or background means the next shape uses the wrong colour. This is especially common when the callback draws 5+ elements. |
| Using `g.drawLine(x1, y1, x2, y2, thickness)` (point-by-point order) | Use `g.drawLine(x1, x2, y1, y2, thickness)` (x-coords then y-coords) | The parameter order groups x-coordinates before y-coordinates, which is unusual. Getting this wrong produces diagonal lines where vertical or horizontal lines were intended. |
