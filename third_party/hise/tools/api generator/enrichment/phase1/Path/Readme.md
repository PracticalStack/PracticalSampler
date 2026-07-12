# Path -- Class Analysis

## Brief
Vector path object for constructing and manipulating 2D shapes for UI rendering.

## Purpose
Path is a scriptable wrapper around the JUCE `Path` class that provides a full suite of 2D vector drawing primitives. It supports constructing complex shapes from lines, curves, arcs, and geometric primitives, then querying spatial properties like bounds, intersections, and point containment. Path objects are consumed by `Graphics` methods (`fillPath`, `drawPath`, `applyMask`, shadow methods) for rendering in paint callbacks, and are also used as icon data for UI components and CSS/StyleSheet properties.

## Details

### Coordinate Conventions

Path methods use three distinct parameter conventions for specifying coordinates:

| Convention | Format | Used by |
|-----------|--------|---------|
| Scalar pairs | `(x, y)` | `startNewSubPath`, `lineTo`, `quadraticTo` |
| Point arrays | `[x, y]` | `addPolygon`, `addArrow`, `addStar`, `getIntersection`, `contains`, `getPointOnPath` |
| Rectangle arrays | `[x, y, w, h]` | `addArc`, `addPieSegment`, `addEllipse`, `addRectangle`, `addRoundedRectangle`, `addRoundedRectangleCustomisable`, `getBounds`, `setBounds` |

The `cubicTo` method uses a mixed convention: the first two parameters are point arrays `[cx, cy]` for control points, while the last two are separate scalar `x` and `y` values for the endpoint.

### setBounds Behavior

`setBounds()` expands the path's reported bounds by adding invisible anchor points -- it does not transform geometry. See `setBounds()` and `scaleToFit()` for full details.

### Stroke Data Format

`createStrokedPath()` accepts stroke configuration as either a simple numeric thickness or a JSON object with `"Thickness"`, `"EndCapStyle"`, and `"JointStyle"` properties. See `createStrokedPath()` for the full format and value descriptions.

### Data Loading Formats

`loadFromData()` accepts three input formats: base64 strings, byte arrays, and Path objects. See `loadFromData()` for details on each format.

### Serialization

Two serialization formats are available: human-readable via `toString()`/`fromString()`, and compact binary via `toBase64()`/`loadFromData()`. See those methods for format details.

### HISE_USE_SCRIPT_RECTANGLE_OBJECT

When the project preprocessor `HISE_USE_SCRIPT_RECTANGLE_OBJECT` is enabled (default OFF), `getBounds()` returns a `Rectangle` scripting object instead of a plain `[x, y, w, h]` array. This setting is read once at PathObject construction time and cannot be changed at runtime.

### Float Sanitization

Coordinate values passed to `startNewSubPath`, `lineTo`, `addArc`, and `addPieSegment` are sanitized against NaN/Inf via the `SANITIZED()` macro. However, `quadraticTo` and `cubicTo` lack this sanitization -- see their pitfalls for details.

## obtainedVia
`Content.createPath()`

## minimalObjectToken
p

## Constants
None.

## Dynamic Constants
None.

## Common Mistakes

| Wrong | Right | Explanation |
|-------|-------|-------------|
| `p.setBounds([0, 0, 100, 100]);` (expecting resize) | `p.scaleToFit(0, 0, 100, 100, true);` | `setBounds` only expands the reported bounding box by adding invisible anchor points -- it does not transform the path geometry. Use `scaleToFit` to actually rescale the path into a target area. |

## codeExample
```javascript:basic-triangle-path
const var p = Content.createPath();
p.startNewSubPath(0.0, 0.0);
p.lineTo(1.0, 0.0);
p.lineTo(0.5, 1.0);
p.closeSubPath();
```
```json:testMetadata:basic-triangle-path
{
  "testable": true,
  "verifyScript": {"type": "REPL", "expression": "p.getLength() > 0", "value": true}
}
```

## Alternatives
- `Rectangle` -- Use Path for complex vector shapes with curves and multiple sub-paths; use Rectangle for simple axis-aligned rectangular geometry.
- `Graphics` -- Path defines shape geometry; Graphics renders those shapes onto a surface.

## Related Preprocessors
- `USE_BACKEND` -- controls PathPreviewComponent debug popup in the HISE IDE
- `HISE_USE_SCRIPT_RECTANGLE_OBJECT` -- affects whether `getBounds()` returns an array or a Rectangle object

## Diagnostic Ideas
Reviewed: Yes
Count: 0
Rationale: Path is a stateless geometry builder with no timeline dependencies, no silent-failure preconditions, and no configuration modes that could lead to subtle misuse at parse time.
