<!-- Diagram triage:
  - No class-level or method-level diagrams specified in Phase 1.
-->

# Path

Path is a vector drawing object for constructing 2D shapes that can be rendered in paint callbacks and LAF functions. Create one with `Content.createPath()`, build geometry using drawing primitives, then render it with `Graphics.drawPath()` or `Graphics.fillPath()`.

Paths support three categories of geometry construction:

1. **Freeform drawing** - build shapes point-by-point with `startNewSubPath`, `lineTo`, `quadraticTo`, and `cubicTo`, then optionally close them with `closeSubPath`.
2. **Geometric primitives** - add complete shapes in a single call: rectangles, rounded rectangles, ellipses, arcs, pie segments, polygons, stars, triangles, quadrilaterals, and arrows.
3. **Serialised data** - load pre-built paths from base64 strings or byte arrays with `loadFromData`, or from human-readable strings with `fromString`.

The most common use case is drawing custom rotary knob arcs in `drawRotarySlider` LAF callbacks. Paths also serve as reusable icon libraries (transport controls, waveform shapes, filter curves) and as the basis for envelope curve visualisation.

Path methods use three coordinate conventions:

| Convention | Format | Used by |
|-----------|--------|---------|
| Scalar pairs | `(x, y)` | `startNewSubPath`, `lineTo`, `quadraticTo` |
| Point arrays | `[x, y]` | `addPolygon`, `addArrow`, `addStar`, `contains`, `getIntersection` |
| Rectangle arrays | `[x, y, w, h]` | `addArc`, `addEllipse`, `addRectangle`, `setBounds`, `getBounds` |

When working in normalised `[0, 0, 1, 1]` space (recommended for paths that will be rendered at different sizes), anchor the bounding box before adding geometry so that `Graphics.drawPath` scales correctly. Use `setBounds([0, 0, 1, 1])` or the equivalent manual pattern:

```js
const var p = Content.createPath();
p.startNewSubPath(0.0, 0.0);
p.startNewSubPath(1.0, 1.0);
p.addArc([0, 0, 1, 1], -2.4, 2.4);
```

For stroke styling, `Graphics.drawPath` accepts a `pathStrokeStyle` object instead of a plain thickness value:

```js
var strokeStyle = {};
strokeStyle.Thickness = 3.0;
g.drawPath(p, area, strokeStyle);
```

> [!Tip:Angles in radians, clockwise from 3 o'clock] All angle parameters (in `addArc`, `addPieSegment`) are measured in radians, clockwise from the 3 o'clock position. For a standard rotary knob arc with a gap at the bottom, use an angular half-range of roughly 2.3 to 2.7 radians from centre.

## Common Mistakes

- **Use scaleToFit not setBounds to resize**
  **Wrong:** `p.setBounds([0, 0, 100, 100]);` (expecting it to resize the path)
  **Right:** `p.scaleToFit(0, 0, 100, 100, true);`
  *`setBounds` only expands the reported bounding box by adding invisible anchor points. It does not transform geometry. Use `scaleToFit` to actually rescale a path into a target area.*

- **Anchor bounds before adding arcs**
  **Wrong:** Using `addArc` on a path without anchoring bounds first
  **Right:** Call `p.setBounds([0, 0, 1, 1])` (or `startNewSubPath(0,0)` + `startNewSubPath(1,1)`) before `addArc([0,0,1,1], ...)`
  *An arc alone has a bounding box covering only the arc segment. When rendered with `g.drawPath(path, targetArea)`, the path scales from its bounds, producing skewed or misaligned output.*

- **Create static paths at init scope**
  **Wrong:** Creating Path objects inside paint routines for static shapes
  **Right:** Create paths as `const var` at init scope, reuse across paint calls
  *Paint routines run on every repaint. Creating new objects each time is wasteful for shapes that do not change. Only build paths inside paint routines when the geometry depends on the current value.*

- **Separate thickness from dash array parameter**
  **Wrong:** Passing the stroke thickness as the `dotData` parameter of `createStrokedPath`
  **Right:** `createStrokedPath(thickness, [])` - first parameter is thickness or stroke config, second is the dash array
  *The parameters are ordered `(strokeData, dotData)`. An empty array `[]` produces a solid stroke; a non-empty array like `[10, 5]` produces dashes.*
