# Path -- Method Analysis

## addArc

**Signature:** `undefined addArc(Array area, Number fromRadians, Number toRadians)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addArc([10, 10, 100, 100], 0.0, Math.PI);`

**Description:**
Adds an arc (a section of an ellipse's outline) to the path. The arc is defined by the bounding rectangle of the ellipse it belongs to, and the start and end angles in radians. The arc is added as a new sub-path that starts from the point on the ellipse at `fromRadians` and traces to the point at `toRadians`. Angles are measured clockwise from the 3 o'clock position (standard JUCE convention).

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, width, height]` of the ellipse the arc belongs to. | 4-element array or Rectangle object |
| fromRadians | Number | no | Start angle of the arc in radians. | Sanitized against NaN/Inf |
| toRadians | Number | no | End angle of the arc in radians. | Sanitized against NaN/Inf |

**Pitfalls:**
- The arc always starts a new sub-path (JUCE's `addArc` with `startAsNewSubPath=true`). To connect the arc to the current path position, use `lineTo` to move to the arc's start point first, then construct the arc geometry manually with `quadraticTo` or `cubicTo`.

**Cross References:**
- `$API.Path.addPieSegment$`
- `$API.Path.addEllipse$`

## addArrow

**Signature:** `undefined addArrow(Array start, Array end, Number thickness, Number headWidth, Number headLength)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addArrow([10, 50], [200, 50], 3.0, 10.0, 8.0);`

**Description:**
Adds an arrow shape to the path. The arrow runs from `start` to `end` as a line with the specified `thickness`, with an arrowhead at the end point. The arrowhead width and length are controlled separately. Delegates to JUCE's `Path::addArrow` via `Line<float>`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Array | no | Start point of the arrow as `[x, y]`. | 2-element array |
| end | Array | no | End point (tip of arrowhead) as `[x, y]`. | 2-element array |
| thickness | Number | no | Line thickness of the arrow shaft in pixels. | > 0 |
| headWidth | Number | no | Width of the arrowhead at its widest point in pixels. | > 0 |
| headLength | Number | no | Length of the arrowhead from tip to base in pixels. | > 0 |

**Cross References:**
- `$API.Path.lineTo$`
- `$API.Path.startNewSubPath$`

## addEllipse

**Signature:** `undefined addEllipse(Array area)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addEllipse([10, 10, 100, 80]);`

**Description:**
Adds a closed ellipse to the path. The ellipse fills the bounding rectangle specified by `area`. For a circle, use a square bounding rectangle (equal width and height). The ellipse is added as a new closed sub-path.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, width, height]` for the ellipse. | 4-element array or Rectangle object |

**Cross References:**
- `$API.Path.addArc$`
- `$API.Path.addPieSegment$`
- `$API.Path.addRectangle$`

## addPieSegment

**Signature:** `undefined addPieSegment(Array area, Number fromRadians, Number toRadians, Number innerCircleProportionalSize)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addPieSegment([10, 10, 100, 100], 0.0, Math.PI * 1.5, 0.5);`

**Description:**
Adds a pie segment (wedge shape) to the path. The segment is defined by the bounding rectangle of the ellipse, the angular range in radians, and an inner circle proportion that creates a donut-style cutout. An `innerCircleProportionalSize` of 0.0 creates a solid pie wedge from the center; values between 0 and 1 cut out a proportional inner circle, creating a ring segment. A value of 1.0 produces a zero-width ring (effectively an arc outline). Angles are measured clockwise from the 3 o'clock position.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle `[x, y, width, height]` of the ellipse. | 4-element array or Rectangle object |
| fromRadians | Number | no | Start angle in radians. | Sanitized against NaN/Inf |
| toRadians | Number | no | End angle in radians. | Sanitized against NaN/Inf |
| innerCircleProportionalSize | Number | no | Proportional size of the inner circle cutout, 0.0 (solid wedge) to 1.0 (thin arc). | 0.0 - 1.0 |

**Cross References:**
- `$API.Path.addArc$`
- `$API.Path.addEllipse$`

**Example:**
```javascript:pie-segment-knob-arc
// Title: Drawing a knob-style arc indicator
const var p = Content.createPath();

// Full donut ring background
p.addPieSegment([0, 0, 100, 100], -2.5, 2.5, 0.7);

// Active value arc overlay
const var valuePath = Content.createPath();
valuePath.addPieSegment([0, 0, 100, 100], -2.5, 0.5, 0.7);
```
```json:testMetadata:pie-segment-knob-arc
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "p.getLength() > valuePath.getLength()", "value": true},
    {"type": "REPL", "expression": "p.contains([50, 50])", "value": true}
  ]
}
```

## quadraticTo

**Signature:** `undefined quadraticTo(Number cx, Number cy, Number x, Number y)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.quadraticTo(50.0, 0.0, 100.0, 50.0);`

**Description:**
Adds a quadratic Bezier curve from the current path position to the endpoint `(x, y)`, using `(cx, cy)` as the single control point. The control point determines the shape of the curve -- it "pulls" the curve toward itself. All four parameters are individual scalar values (not arrays). A sub-path must be active (via `startNewSubPath` or a previous drawing command) before calling this method. Delegates directly to `juce::Path::quadraticTo`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cx | Number | no | X coordinate of the control point. | Not sanitized against NaN/Inf |
| cy | Number | no | Y coordinate of the control point. | Not sanitized against NaN/Inf |
| x | Number | no | X coordinate of the curve endpoint. | Not sanitized against NaN/Inf |
| y | Number | no | Y coordinate of the curve endpoint. | Not sanitized against NaN/Inf |

**Pitfalls:**
- [BUG] Coordinate values are NOT sanitized against NaN/Inf (unlike `startNewSubPath`, `lineTo`, `addArc`, and `addPieSegment` which use the `SANITIZED()` macro). Passing NaN or Inf values corrupts the path geometry silently.

**Cross References:**
- `$API.Path.cubicTo$`
- `$API.Path.lineTo$`
- `$API.Path.startNewSubPath$`

## roundCorners

**Signature:** `undefined roundCorners(Number radius)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Creates an entirely new JUCE Path via `createPathWithRoundedCorners`, which allocates heap memory for the new path data.
**Minimal Example:** `{obj}.roundCorners(5.0);`

**Description:**
Replaces the current path with a version where all sharp corners have been replaced by smooth curves with the given radius. The internal path is entirely replaced: `p = p.createPathWithRoundedCorners(radius)`. This is a destructive, non-reversible operation on the path data -- the original sharp-corner geometry cannot be recovered. The rounding applies to all corners in all sub-paths uniformly. A radius of 0.0 produces no visible change.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| radius | Number | no | Corner radius in pixels. Larger values produce smoother, wider curves at corners. | >= 0 |

**Cross References:**
- `$API.Path.addRoundedRectangle$`
- `$API.Path.addRoundedRectangleCustomisable$`
- `$API.Path.createStrokedPath$`

## scaleToFit

**Signature:** `undefined scaleToFit(Number x, Number y, Number width, Number height, Integer preserveProportions)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.scaleToFit(0.0, 0.0, 200.0, 100.0, true);`

**Description:**
Transforms the path geometry to fit within the specified rectangle. The transform is applied directly to the path data via `Path::applyTransform(Path::getTransformToScaleToFit(...))`, permanently modifying all coordinates. When `preserveProportions` is `true`, the path is uniformly scaled to fit within the target area while maintaining its aspect ratio (centering within the target if the aspect ratios differ). When `false`, the path is stretched independently on X and Y axes to fill the entire target area. This is the method to use when you want to resize a path to a target area -- unlike `setBounds`, which only expands the reported bounding box without transforming geometry.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Number | no | X coordinate of the target rectangle's top-left corner. | Any value |
| y | Number | no | Y coordinate of the target rectangle's top-left corner. | Any value |
| width | Number | no | Width of the target rectangle in pixels. | > 0 for meaningful results |
| height | Number | no | Height of the target rectangle in pixels. | > 0 for meaningful results |
| preserveProportions | Integer | yes | If `true`, maintains the path's aspect ratio within the target area. If `false`, stretches to fill the entire rectangle. | Boolean (0 or 1) |

**Cross References:**
- `$API.Path.setBounds$`
- `$API.Path.getBounds$`
- `$API.Path.getRatio$`

## setBounds

**Signature:** `undefined setBounds(Array boundingBox)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.setBounds([0, 0, 100, 100]);`

**Description:**
Expands the path's reported bounding box to include the specified rectangle, without transforming the existing path geometry. Internally, this adds two invisible `startNewSubPath` calls at the top-left and bottom-right corners of the given rectangle. These invisible anchor points extend the bounds reported by `getBounds()` but add no visible lines or curves. This is useful when a path needs a consistent bounding box for layout purposes (e.g., ensuring `Graphics.fillPath` with an area parameter scales from a known reference size). To actually resize or transform path geometry into a target area, use `scaleToFit()` instead.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| boundingBox | Array | no | Target bounding rectangle as `[x, y, width, height]`. | 4-element array or Rectangle object |

**Pitfalls:**
- Does NOT resize or transform the path. This is the most common source of confusion -- calling `setBounds` with the expectation that it will scale the path geometry to a new size has no visible effect on rendering. Use `scaleToFit` for that purpose.
- The invisible anchor points are permanent additions to the path. They cannot be removed except by calling `clear()` and rebuilding the path. Calling `setBounds` multiple times accumulates anchor points, though only the outermost coordinates affect the reported bounds.

**Cross References:**
- `$API.Path.scaleToFit$`
- `$API.Path.getBounds$`

## startNewSubPath

**Signature:** `undefined startNewSubPath(Number x, Number y)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.startNewSubPath(10.0, 20.0);`

**Description:**
Begins a new sub-path at the given coordinates without drawing anything. This sets the "current position" for subsequent drawing commands (`lineTo`, `quadraticTo`, `cubicTo`). Does NOT clear the existing path -- previously added geometry is preserved. To clear the path and start fresh, call `clear()` first. Multiple sub-paths allow constructing compound shapes (e.g., a shape with holes). Coordinates are sanitized against NaN/Inf via the `SANITIZED()` macro. Delegates to `juce::Path::startNewSubPath`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Number | no | X coordinate of the new sub-path starting point. | Sanitized against NaN/Inf |
| y | Number | no | Y coordinate of the new sub-path starting point. | Sanitized against NaN/Inf |

**Cross References:**
- `$API.Path.lineTo$`
- `$API.Path.closeSubPath$`
- `$API.Path.clear$`

## toBase64

**Signature:** `String toBase64()`
**Return Type:** `String`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap memory for the MemoryOutputStream, binary path serialization, and base64 string encoding.
**Minimal Example:** `var encoded = {obj}.toBase64();`

**Description:**
Serializes the path to a compact base64-encoded string representation. The path is first written to a binary `MemoryOutputStream` via JUCE's `Path::writePathToStream`, then the resulting memory block is base64-encoded. The output string can be stored, transmitted, or used as a CSS/StyleSheet path property value. Restore the path from this format using `loadFromData(base64String)`. This is the more compact serialization format compared to `toString()` which produces human-readable text. Returns an empty string for an empty path.

**Cross References:**
- `$API.Path.loadFromData$`
- `$API.Path.toString$`
- `$API.Path.fromString$`

## toString

**Signature:** `String toString()`
**Return Type:** `String`
**Call Scope:** warning
**Call Scope Note:** String involvement, atomic ref-count operations.
**Minimal Example:** `var str = {obj}.toString();`

**Description:**
Converts the path to a human-readable string representation using JUCE's internal path serialization format. The format uses single-character commands followed by coordinate values: `m` (moveTo), `l` (lineTo), `q` (quadraticTo), `c` (cubicTo), `z` (closeSubPath), etc. This produces a longer but inspectable representation compared to `toBase64()`. The string can be restored to a path via `fromString()`. Returns an empty string for an empty path. Delegates directly to `juce::Path::toString()`.

**Cross References:**
- `$API.Path.fromString$`
- `$API.Path.toBase64$`
- `$API.Path.loadFromData$`

## addStar

**Signature:** `undefined addStar(Array center, Number numPoints, Number innerRadius, Number outerRadius, Number angle)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addStar([50, 50], 5, 15.0, 40.0, 0.0);`

**Description:**
Adds a star shape to the path centered at the given point. The star has the specified number of points, with alternating vertices at `innerRadius` and `outerRadius` from the center. A 5-pointed star uses `numPoints = 5`. The `angle` parameter rotates the star in radians. The center is specified as a `[x, y]` point array, converted via `ApiHelpers::getPointFromVar`. Delegates to JUCE's `Path::addStar`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| center | Array | no | Center point of the star as `[x, y]`. | 2-element array |
| numPoints | Number | no | Number of points (tips) on the star. | >= 2 for meaningful shapes |
| innerRadius | Number | no | Distance from center to the inner vertices (the concave points between tips) in pixels. | > 0 |
| outerRadius | Number | no | Distance from center to the outer vertices (the tips) in pixels. | > 0, typically > innerRadius |
| angle | Number | no | Rotation angle of the star in radians. 0.0 starts with the first tip pointing right. | Any value |

**Cross References:**
- `$API.Path.addPolygon$`
- `$API.Path.addTriangle$`

## addTriangle

**Signature:** `undefined addTriangle(Array xy1, Array xy2, Array xy3)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addTriangle([10, 80], [50, 10], [90, 80]);`

**Description:**
Adds a triangle to the path defined by three corner points. Each point is a 2-element `[x, y]` array. The three points are connected in order and closed automatically. The coordinates are extracted by direct array indexing (`xy1[0]`, `xy1[1]`, etc.) and passed to JUCE's `Path::addTriangle`. For regular (equilateral) triangles without explicit vertex control, `addPolygon` with 3 sides is an alternative.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| xy1 | Array | no | First vertex as `[x, y]`. | 2-element array |
| xy2 | Array | no | Second vertex as `[x, y]`. | 2-element array |
| xy3 | Array | no | Third vertex as `[x, y]`. | 2-element array |

**Cross References:**
- `$API.Path.addPolygon$`
- `$API.Path.addQuadrilateral$`

## clear

**Signature:** `undefined clear()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.clear();`

**Description:**
Removes all lines, curves, and sub-paths from the path, resetting it to an empty state. Delegates directly to JUCE's `Path::clear()`. Use this to reuse a Path object for new geometry rather than creating a new one. The path's bounding box is also reset.

**Cross References:**
- `$API.Path.startNewSubPath$`
- `$API.Path.closeSubPath$`
- `$API.Path.setBounds$`

## closeSubPath

**Signature:** `undefined closeSubPath()`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.closeSubPath();`

**Description:**
Closes the current sub-path by adding a straight line from the current position back to the start of the sub-path. This is required for shapes that need to be filled -- `Graphics.fillPath` fills the interior of closed sub-paths. If the path has no current sub-path or the current position already matches the start point, this is a no-op. Delegates directly to JUCE's `Path::closeSubPath()`.

**Cross References:**
- `$API.Path.startNewSubPath$`
- `$API.Path.lineTo$`
- `$API.Path.quadraticTo$`
- `$API.Path.cubicTo$`

## fromString

**Signature:** `undefined fromString(String stringPath)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** JUCE's `Path::restoreFromString` performs string parsing with heap allocations.
**Minimal Example:** `{obj}.fromString("m 0 0 l 100 0 l 50 100 z");`

**Description:**
Restores path geometry from a human-readable string representation previously created by `toString()`. The string format is JUCE's internal path serialization format, which uses single-character commands (`m` for move, `l` for line, `c` for cubic, `q` for quadratic, `z` for close, etc.) followed by coordinate values. Replaces the current path contents entirely -- any existing geometry is discarded. Delegates directly to `juce::Path::restoreFromString()`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| stringPath | String | no | A path string in JUCE's path serialization format, as produced by `toString()`. | Must be valid JUCE path format |

**Cross References:**
- `$API.Path.toString$`
- `$API.Path.loadFromData$`
- `$API.Path.toBase64$`

## getBounds

**Signature:** `Array getBounds(Number scaleFactor)`
**Return Type:** `Array`
**Call Scope:** safe
**Minimal Example:** `var bounds = {obj}.getBounds(1.0);`

**Description:**
Returns the bounding rectangle of the path with an optional scale factor applied. The scale factor is applied as a uniform transform before computing bounds via `Path::getBoundsTransformed(AffineTransform::scale(scaleFactor))`. A scale factor of 1.0 returns the path's actual bounds. The return type depends on the `HISE_USE_SCRIPT_RECTANGLE_OBJECT` preprocessor setting: by default (OFF), returns a 4-element array `[x, y, width, height]`; when enabled, returns a `Rectangle` scripting object. Returns a zero-area rectangle for an empty path.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| scaleFactor | Number | no | Uniform scale factor applied to path coordinates before computing bounds. Use 1.0 for unscaled bounds. | Any positive number |

**Pitfalls:**
- The return type changes between a plain array and a Rectangle object depending on the project-level `HISE_USE_SCRIPT_RECTANGLE_OBJECT` preprocessor setting. Code that indexes the result (e.g., `bounds[2]` for width) works only with the default array format.

**Cross References:**
- `$API.Path.setBounds$`
- `$API.Path.scaleToFit$`
- `$API.Path.getRatio$`

## getIntersection

**Signature:** `Array getIntersection(Array start, Array end, Integer keepSectionOutsidePath)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a JUCE Array for the return value.
**Minimal Example:** `var pt = {obj}.getIntersection([0, 50], [200, 50], false);`

**Description:**
Tests whether a line segment intersects the path and returns the intersection point. The line is defined by `start` and `end` as `[x, y]` point arrays. If an intersection exists, returns a `[x, y]` array of the intersection point. If there is no intersection, returns `false`. The `keepSectionOutsidePath` flag controls which point of the clipped line is returned: when `true`, the start point of the clipped line (the portion outside the path) is returned; when `false`, the end point (the entry point into the path). Uses JUCE's `Path::intersectsLine` and `Path::getClippedLine` internally.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| start | Array | no | Start point of the test line as `[x, y]`. | 2-element array |
| end | Array | no | End point of the test line as `[x, y]`. | 2-element array |
| keepSectionOutsidePath | Integer | no | If `true`, returns the outer endpoint of the clipped line; if `false`, returns the inner endpoint where the line enters the path. | Boolean (0 or 1) |

**Pitfalls:**
- [BUG] The start point Y coordinate is silently offset by -0.001 pixels internally to work around edge cases where the start point lies exactly on the path boundary. This means intersection results may be very slightly imprecise for lines starting at exact path coordinates.
- Returns `false` (not an array) when no intersection is found. Callers must check the return type before indexing the result.

**Cross References:**
- `$API.Path.contains$`
- `$API.Path.getPointOnPath$`

**Example:**
```javascript:path-intersection-test
// Title: Finding where a horizontal line enters a circular path
const var p = Content.createPath();
p.addEllipse([0, 0, 100, 100]);

var intersection = p.getIntersection([0, 50], [100, 50], false);

if (intersection)
    Console.print("Hit at: " + intersection[0] + ", " + intersection[1]);
else
    Console.print("No intersection");
```
```json:testMetadata:path-intersection-test
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "intersection !== false", "value": true},
    {"type": "REPL", "expression": "intersection.length", "value": 2},
    {"type": "REPL", "expression": "intersection[0] < 5.0", "value": true},
    {"type": "REPL", "expression": "Math.abs(intersection[1] - 50.0) < 1.0", "value": true}
  ]
}
```

## getLength

**Signature:** `Double getLength()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var len = {obj}.getLength();`

**Description:**
Returns the total length of the path in pixels. The length is calculated by flattening the path (approximating curves as line segments) and summing the segment lengths. Uses JUCE's `Path::getLength` with an identity transform (`AffineTransform::scale(1.0f)`) and a tolerance of 1.0 pixel. Returns 0.0 for an empty path.

**Cross References:**
- `$API.Path.getPointOnPath$`
- `$API.Path.getRatio$`
- `$API.Path.getYAt$`

## getPointOnPath

**Signature:** `Array getPointOnPath(Number distanceFromStart)`
**Return Type:** `Array`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a JUCE Array for the return value.
**Minimal Example:** `var pt = {obj}.getPointOnPath(50.0);`

**Description:**
Returns the coordinates of a point at a given distance along the path, measured from the start. The `distanceFromStart` is in pixels along the path's outline. Returns a `[x, y]` array. If the distance exceeds the path length, returns the end point of the path. If the path is empty, returns `[0, 0]`. Uses JUCE's `Path::getPointAlongPath` internally with a `PathFlatteningIterator` to walk the path geometry.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| distanceFromStart | Number | no | Distance in pixels from the start of the path. | >= 0 |

**Cross References:**
- `$API.Path.getLength$`
- `$API.Path.getYAt$`
- `$API.Path.getIntersection$`

## getRatio

**Signature:** `Double getRatio()`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var ratio = {obj}.getRatio();`

**Description:**
Returns the width-to-height aspect ratio of the path's bounding box. Computed as `bounds.getWidth() / bounds.getHeight()` from the path's unscaled bounds. A ratio of 1.0 means the path is as wide as it is tall. Values greater than 1.0 indicate a wider-than-tall shape; less than 1.0 indicates taller-than-wide. Useful for preserving proportions when scaling a path into a target area.

**Pitfalls:**
- [BUG] Returns Infinity or NaN if the path's bounding box has zero height (e.g., a purely horizontal line). No validation is performed on the division result.

**Cross References:**
- `$API.Path.getBounds$`
- `$API.Path.scaleToFit$`

## getYAt

**Signature:** `Double getYAt(Number xPos)`
**Return Type:** `Double`
**Call Scope:** safe
**Minimal Example:** `var y = {obj}.getYAt(50.0);`

**Description:**
Returns the Y coordinate of the first intersection between the path and a vertical line at the given X position. Walks the flattened path segments using a `PathFlatteningIterator` and returns the interpolated Y value of the first segment whose X range contains `xPos`. Closing sub-path segments are ignored. Returns `undefined` if no segment contains the given X position (i.e., the X is outside the path's horizontal extent). The tolerance used for flattening adapts to the path's size, using the smaller of `size / 100` and JUCE's default path measurement tolerance.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| xPos | Number | no | X coordinate at which to sample the path's Y value. | Should be within the path's horizontal bounds |

**Pitfalls:**
- Only returns the Y value of the *first* matching segment. For paths with multiple sub-paths or self-intersecting shapes that cross the same X coordinate at multiple Y values, only the first encountered Y is returned.
- Returns `undefined` (not a number) when no match is found. Callers must check with `isDefined()` before using the result in arithmetic.

**Cross References:**
- `$API.Path.getPointOnPath$`
- `$API.Path.getIntersection$`
- `$API.Path.getLength$`

## lineTo

**Signature:** `undefined lineTo(Number x, Number y)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.lineTo(100.0, 50.0);`

**Description:**
Adds a straight line segment from the current path position to the point `(x, y)`. The current sub-path must have a starting point (via `startNewSubPath` or a previous segment) before calling this method -- if no sub-path has been started, JUCE adds a `startNewSubPath(0, 0)` implicitly. Coordinates are sanitized against NaN/Inf via the `SANITIZED()` macro, consistent with `startNewSubPath`. Delegates to `juce::Path::lineTo`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| x | Number | no | X coordinate of the line endpoint. | Sanitized against NaN/Inf |
| y | Number | no | Y coordinate of the line endpoint. | Sanitized against NaN/Inf |

**Cross References:**
- `$API.Path.startNewSubPath$`
- `$API.Path.closeSubPath$`
- `$API.Path.quadraticTo$`
- `$API.Path.cubicTo$`

## loadFromData

**Signature:** `undefined loadFromData(NotUndefined data)`
**Return Type:** `undefined`
**Call Scope:** unsafe
**Call Scope Note:** Allocates heap memory for base64 decoding (MemoryBlock), array conversion (Array<unsigned char>), or path data parsing.
**Minimal Example:** `{obj}.loadFromData("base64EncodedPathData...");`

**Description:**
Loads path geometry from external data, replacing the current path contents. Accepts three input formats: a base64-encoded string (decoded and loaded as binary JUCE path data), an array of numbers (each element cast to unsigned char and loaded as raw binary path data), or another Path object (copies its internal geometry directly). The existing path is cleared before loading when using string or array input. Delegates to `ApiHelpers::loadPathFromData` which handles format detection automatically.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| data | NotUndefined | no | Path data in one of three formats: a base64-encoded string, an array of byte values (0-255), or a Path object. | Must be String, Array, or Path |

**Pitfalls:**
- [BUG] If `data` is none of the three accepted types (e.g., a number, JSON object, or boolean), the method silently does nothing -- the existing path is left unchanged with no error reported.

**Cross References:**
- `$API.Path.toBase64$`
- `$API.Path.fromString$`
- `$API.Path.toString$`

**Example:**
```javascript:load-path-roundtrip
// Title: Saving and restoring path data via base64
const var p = Content.createPath();
p.startNewSubPath(0.0, 0.0);
p.lineTo(1.0, 0.0);
p.lineTo(0.5, 1.0);
p.closeSubPath();

var encoded = p.toBase64();

const var p2 = Content.createPath();
p2.loadFromData(encoded);
```
```json:testMetadata:load-path-roundtrip
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "p2.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "p.toBase64() == p2.toBase64()", "value": true}
  ]
}
```

## addPolygon

**Signature:** `undefined addPolygon(Array center, Number numSides, Number radius, Number angle)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addPolygon([50, 50], 6, 30.0, 0.0);`

**Description:**
Adds a regular polygon to the path centered at the given point. The polygon has the specified number of sides, with each vertex at `radius` distance from the center. The `angle` parameter rotates the polygon in radians. The center is specified as a `[x, y]` point array, converted via `ApiHelpers::getPointFromVar`. A hexagon uses `numSides = 6`, a pentagon uses `numSides = 5`, etc. For a triangle with vertex-level control, use `addTriangle` instead. Delegates to JUCE's `Path::addPolygon`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| center | Array | no | Center point of the polygon as `[x, y]`. | 2-element array |
| numSides | Number | no | Number of sides (and vertices) of the polygon. | >= 3 for meaningful shapes |
| radius | Number | no | Distance from the center to each vertex in pixels. | > 0 |
| angle | Number | no | Rotation angle of the polygon in radians. 0.0 starts with the first vertex pointing right. | Any value |

**Cross References:**
- `$API.Path.addStar$`
- `$API.Path.addTriangle$`

## addQuadrilateral

**Signature:** `undefined addQuadrilateral(Array xy1, Array xy2, Array xy3, Array xy4)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addQuadrilateral([10, 10], [90, 10], [80, 80], [20, 80]);`

**Description:**
Adds a quadrilateral (four-sided polygon) to the path defined by four corner points. Each point is a 2-element `[x, y]` array. The four points are connected in order and closed automatically. The coordinates are extracted by direct array indexing (`xy1[0]`, `xy1[1]`, etc.) and passed to JUCE's `Path::addQuadrilateral`. Unlike `addRectangle`, this allows non-rectangular four-sided shapes such as trapezoids and parallelograms. For axis-aligned rectangles, `addRectangle` is simpler.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| xy1 | Array | no | First vertex as `[x, y]`. | 2-element array |
| xy2 | Array | no | Second vertex as `[x, y]`. | 2-element array |
| xy3 | Array | no | Third vertex as `[x, y]`. | 2-element array |
| xy4 | Array | no | Fourth vertex as `[x, y]`. | 2-element array |

**Cross References:**
- `$API.Path.addTriangle$`
- `$API.Path.addRectangle$`
- `$API.Path.addPolygon$`

## addRectangle

**Signature:** `undefined addRectangle(Array area)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addRectangle([10, 10, 200, 100]);`

**Description:**
Adds a closed rectangle to the path. The rectangle is defined by a bounding area `[x, y, width, height]`. Converted via `ApiHelpers::getRectangleFromVar`, which also accepts a `Rectangle` scripting object. The rectangle is added as a new closed sub-path. Delegates to JUCE's `Path::addRectangle`. For rectangles with rounded corners, use `addRoundedRectangle` or `addRoundedRectangleCustomisable`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle as `[x, y, width, height]`. | 4-element array or Rectangle object |

**Cross References:**
- `$API.Path.addRoundedRectangle$`
- `$API.Path.addRoundedRectangleCustomisable$`
- `$API.Path.addEllipse$`
- `$API.Path.addQuadrilateral$`

## addRoundedRectangle

**Signature:** `undefined addRoundedRectangle(Array area, Number cornerSize)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addRoundedRectangle([10, 10, 200, 100], 8.0);`

**Description:**
Adds a closed rectangle with uniformly rounded corners to the path. The rectangle is defined by a bounding area `[x, y, width, height]`, and `cornerSize` controls the radius of all four corners equally. Converted via `ApiHelpers::getRectangleFromVar`. A `cornerSize` of 0.0 produces a sharp-cornered rectangle identical to `addRectangle`. Delegates to JUCE's `Path::addRoundedRectangle(Rectangle, cornerSize)`. For per-corner control of which corners are rounded, use `addRoundedRectangleCustomisable`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle as `[x, y, width, height]`. | 4-element array or Rectangle object |
| cornerSize | Number | no | Radius of the rounded corners in pixels. Applied equally to all four corners. | >= 0 |

**Cross References:**
- `$API.Path.addRoundedRectangleCustomisable$`
- `$API.Path.addRectangle$`
- `$API.Path.roundCorners$`

## addRoundedRectangleCustomisable

**Signature:** `undefined addRoundedRectangleCustomisable(Array area, Array cornerSizeXY, Array boolCurves)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.addRoundedRectangleCustomisable([10, 10, 200, 100], [8.0, 8.0], [true, true, false, false]);`

**Description:**
Adds a closed rectangle with independently controllable corner rounding to the path. The `cornerSizeXY` array specifies the corner radius as `[radiusX, radiusY]`, allowing elliptical corners (different horizontal and vertical radii). The `boolCurves` array specifies which of the four corners are rounded as `[topLeft, topRight, bottomLeft, bottomRight]` -- `true` for rounded, `false` for sharp. The area is converted via `ApiHelpers::getRectangleFromVar`. Coordinates for the corner size and boolean flags are extracted by direct array indexing. Delegates to the 10-parameter overload of JUCE's `Path::addRoundedRectangle`.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| area | Array | no | Bounding rectangle as `[x, y, width, height]`. | 4-element array or Rectangle object |
| cornerSizeXY | Array | no | Corner radius as `[radiusX, radiusY]`. Use equal values for circular corners. | 2-element array, values >= 0 |
| boolCurves | Array | no | Per-corner rounding flags as `[topLeft, topRight, bottomLeft, bottomRight]`. `true` rounds the corner, `false` keeps it sharp. | 4-element boolean array |

**Cross References:**
- `$API.Path.addRoundedRectangle$`
- `$API.Path.addRectangle$`
- `$API.Path.roundCorners$`

**Example:**
```javascript:rounded-rect-top-only
// Title: Rectangle with only top corners rounded
const var p = Content.createPath();
p.addRoundedRectangleCustomisable([0, 0, 200, 100], [10.0, 10.0], [true, true, false, false]);
```
```json:testMetadata:rounded-rect-top-only
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "p.contains([100, 50])", "value": true},
    {"type": "REPL", "expression": "Math.abs(p.getBounds(1.0)[2] - 200.0) < 0.5", "value": true}
  ]
}
```

## contains

**Signature:** `Integer contains(Array point)`
**Return Type:** `Integer`
**Call Scope:** safe
**Minimal Example:** `var hit = {obj}.contains([50, 50]);`

**Description:**
Tests whether a point lies within the path. The point is specified as a `[x, y]` array, converted via `ApiHelpers::getPointFromVar`. Returns `true` if the point is inside any closed sub-path of the path, `false` otherwise. This is only meaningful for closed paths -- open paths have no defined interior region. Delegates to JUCE's `Path::contains(Point)`, which uses a winding-number rule to determine containment.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| point | Array | no | Test point as `[x, y]`. | 2-element array |

**Cross References:**
- `$API.Path.getIntersection$`
- `$API.Path.getBounds$`
- `$API.Path.closeSubPath$`

## createStrokedPath

**Signature:** `ScriptObject createStrokedPath(NotUndefined strokeData, Array dotData)`
**Return Type:** `ScriptObject`
**Call Scope:** unsafe
**Call Scope Note:** Allocates a new PathObject, JUCE Array for dash data, and performs path geometry computation via createStrokedPath/createDashedStroke.
**Minimal Example:** `var stroked = {obj}.createStrokedPath(2.0, []);`

**Description:**
Creates and returns a new Path object that represents the outlined (stroked) version of the current path. The original path is not modified. The `strokeData` parameter controls stroke style and can be either a simple numeric thickness value or a JSON object with detailed stroke properties (`"Thickness"`, `"EndCapStyle"`, `"JointStyle"`). The `dotData` parameter is an array of float values specifying a dash pattern (alternating dash and gap lengths). Pass an empty array `[]` for a solid stroke. When a non-empty dash array is provided, `PathStrokeType::createDashedStroke` is used; otherwise `PathStrokeType::createStrokedPath` produces a solid outline. The returned path inherits the original path's bounding box via invisible anchor points at the original bounds' top-left and bottom-right corners.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| strokeData | NotUndefined | no | Stroke configuration: either a numeric thickness value, or a JSON object with `"Thickness"` (Number), `"EndCapStyle"` (`"butt"`, `"square"`, or `"rounded"`), and `"JointStyle"` (`"mitered"`, `"curved"`, or `"beveled"`). | Numeric > 0, or JSON object |
| dotData | Array | no | Dash pattern as an array of alternating dash/gap lengths in pixels. Pass `[]` for a solid stroke. | Array of Numbers |

**Value Descriptions:**

| Value | Description |
|-------|-------------|
| "butt" | EndCapStyle: line ends are cut flat at the endpoint with no extension |
| "square" | EndCapStyle: line ends are extended by half the stroke thickness as a square cap |
| "rounded" | EndCapStyle: line ends are extended by half the stroke thickness as a semicircle |
| "mitered" | JointStyle: corners form a sharp point (extended to the miter limit) |
| "curved" | JointStyle: corners are smoothed with a circular arc |
| "beveled" | JointStyle: corners are cut flat across the joint |

**Pitfalls:**
- [BUG] If `dotData` is not an array (e.g., a number or string), it is silently ignored and a solid stroke is produced. No error is reported.

**Cross References:**
- `$API.Path.roundCorners$`
- `$API.Graphics.drawPath$`

**Example:**
```javascript:create-dashed-stroke
// Title: Creating a dashed stroke path
const var p = Content.createPath();
p.startNewSubPath(0.0, 0.0);
p.lineTo(200.0, 0.0);

var stroked = p.createStrokedPath({
    "Thickness": 3.0,
    "EndCapStyle": "rounded",
    "JointStyle": "curved"
}, [10.0, 5.0]);
```
```json:testMetadata:create-dashed-stroke
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "stroked.getLength() > p.getLength()", "value": true},
    {"type": "REPL", "expression": "stroked.getBounds(1.0)[3] > 0", "value": true}
  ]
}
```

## cubicTo

**Signature:** `undefined cubicTo(Array cxy1, Array cxy2, Number x, Number y)`
**Return Type:** `undefined`
**Call Scope:** safe
**Minimal Example:** `{obj}.cubicTo([30, 0], [70, 100], 100.0, 50.0);`

**Description:**
Adds a cubic Bezier curve from the current path position to the endpoint `(x, y)`, using two control points specified as arrays. The first control point `cxy1` is a `[cx1, cy1]` array, and the second control point `cxy2` is a `[cx2, cy2]` array. The endpoint coordinates `x` and `y` are separate scalar values. This is a mixed parameter convention: control points are arrays, but the endpoint uses individual numbers. A sub-path must be active (via `startNewSubPath` or a previous drawing command) before calling this method. The control points determine the shape of the curve -- `cxy1` pulls the curve from the start, and `cxy2` pulls it toward the end. Delegates to JUCE's `Path::cubicTo` with six extracted float values.

**Parameters:**

| Name | Type | Forced | Description | Constraints |
|------|------|--------|-------------|-------------|
| cxy1 | Array | no | First control point as `[cx1, cy1]`. | 2-element array. Not sanitized against NaN/Inf |
| cxy2 | Array | no | Second control point as `[cx2, cy2]`. | 2-element array. Not sanitized against NaN/Inf |
| x | Number | no | X coordinate of the curve endpoint. | Not sanitized against NaN/Inf |
| y | Number | no | Y coordinate of the curve endpoint. | Not sanitized against NaN/Inf |

**Pitfalls:**
- Uses a mixed parameter convention: control points are `[x, y]` arrays while the endpoint is two separate scalar values. This is inconsistent with `quadraticTo` (which uses four scalars) and with array-based methods like `addTriangle` (which uses arrays for all points).
- [BUG] Coordinate values are NOT sanitized against NaN/Inf (unlike `startNewSubPath`, `lineTo`, `addArc`, and `addPieSegment` which use the `SANITIZED()` macro). Passing NaN or Inf values corrupts the path geometry silently.

**Cross References:**
- `$API.Path.quadraticTo$`
- `$API.Path.lineTo$`
- `$API.Path.startNewSubPath$`
