## getBounds

**Examples:**

```javascript:get-bounds-scale-factor
// Title: Using getBounds to position path rendering in a paint routine
// Context: When a path is built in an arbitrary coordinate space
// (e.g., arc paths from addArc), getBounds returns the actual area
// the path occupies. Multiplying by a scale factor maps from the
// path's internal coordinates to pixel dimensions for rendering.

const var arcPath = Content.createPath();
arcPath.startNewSubPath(0.0, 0.0);
arcPath.startNewSubPath(1.0, 1.0);
arcPath.addArc([0.0, 0.0, 1.0, 1.0], -2.4, 2.4);

var scaledBounds = arcPath.getBounds(200.0);
Console.print("Bounds: " + scaledBounds[0] + ", " + scaledBounds[1] +
              ", " + scaledBounds[2] + ", " + scaledBounds[3]);
```
```json:testMetadata:get-bounds-scale-factor
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "scaledBounds.length", "value": 4},
    {"type": "REPL", "expression": "scaledBounds[2] > 0", "value": true},
    {"type": "REPL", "expression": "scaledBounds[3] > 0", "value": true}
  ]
}
```

The `scaleFactor` parameter of `getBounds` is a uniform multiplier applied to the path coordinates before computing bounds. Passing the component's pixel width when the path was built in unit coordinates produces bounds in pixel space, which can then be passed directly to `drawPath` or `fillPath`.
