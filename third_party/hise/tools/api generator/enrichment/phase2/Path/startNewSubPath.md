## startNewSubPath

**Examples:**

```javascript:bounds-anchoring-pattern
// Title: Bounds anchoring pattern for normalized-space paths
// Context: The most frequent use of startNewSubPath is NOT to begin
// visible drawing, but to anchor the path's bounding box to a known
// coordinate range. When a path will be rendered via
// g.drawPath(path, targetArea), the path is scaled from its bounding
// box to the target area. Without anchoring, an arc or partial shape
// has a bounding box that only covers its actual geometry, causing
// misalignment when scaled.

var p = Content.createPath();

// Anchor to unit square - these points are invisible but define bounds
p.startNewSubPath(0.0, 0.0);
p.startNewSubPath(1.0, 1.0);

// Now add actual geometry
p.addArc([0.0, 0.0, 1.0, 1.0], -2.4, 2.4);

// The path's bounds are [0, 0, 1, 1] regardless of the arc extent
var bounds = p.getBounds(1.0);
```
```json:testMetadata:bounds-anchoring-pattern
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "Math.abs(bounds[0]) < 0.01", "value": true},
    {"type": "REPL", "expression": "Math.abs(bounds[1]) < 0.01", "value": true},
    {"type": "REPL", "expression": "Math.abs(bounds[2] - 1.0) < 0.01", "value": true},
    {"type": "REPL", "expression": "Math.abs(bounds[3] - 1.0) < 0.01", "value": true}
  ]
}
```

```javascript:compound-x-icon
// Title: Multiple sub-paths for compound shapes
// Context: startNewSubPath creates a new disconnected segment within
// the same path. This builds compound shapes like an X (close) icon
// from two separate line segments.

const var closeIcon = Content.createPath();
closeIcon.startNewSubPath(0.0, 0.0);
closeIcon.lineTo(1.0, 1.0);
closeIcon.startNewSubPath(1.0, 0.0);
closeIcon.lineTo(0.0, 1.0);
```
```json:testMetadata:compound-x-icon
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "closeIcon.getLength() > 2.0", "value": true},
    {"type": "REPL", "expression": "Math.abs(closeIcon.getBounds(1.0)[2] - 1.0) < 0.01", "value": true}
  ]
}
```

```javascript:clear-and-reanchor-helper
// Title: Utility function to clear and re-anchor a reusable path
// Context: When a path is reused across multiple paint calls with
// different geometry each time, a helper function that clears and
// re-anchors avoids repeating the boilerplate.

const var arcPath = Content.createPath();

inline function clearPathWithNormBounds(p)
{
    p.clear();
    p.startNewSubPath(0.0, 0.0);
    p.startNewSubPath(1.0, 1.0);
}

// Reuse the same path object efficiently
clearPathWithNormBounds(arcPath);
arcPath.addArc([0.0, 0.0, 1.0, 1.0], -2.5, 2.5);

var helperBounds = arcPath.getBounds(1.0);
```
```json:testMetadata:clear-and-reanchor-helper
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "arcPath.getLength() > 0", "value": true},
    {"type": "REPL", "expression": "Math.abs(helperBounds[0]) < 0.01", "value": true},
    {"type": "REPL", "expression": "Math.abs(helperBounds[2] - 1.0) < 0.01", "value": true}
  ]
}
```
