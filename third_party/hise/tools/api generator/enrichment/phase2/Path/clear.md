## clear

**Examples:**

```javascript:clear-rebuild-reuse
// Title: Clearing and rebuilding a path with new geometry
// Context: When a path's geometry changes (e.g., following a
// modulation value), clearing and rebuilding is more efficient
// than creating a new Path each time. The clear + anchor + rebuild
// pattern appears frequently in timer-driven UI updates.

const var arcPath = Content.createPath();

// Build initial geometry
arcPath.startNewSubPath(0.0, 0.0);
arcPath.startNewSubPath(1.0, 1.0);
arcPath.addArc([0, 0, 1, 1], -2.4, 0.0);
var lengthBefore = arcPath.getLength();

// Clear and rebuild with different geometry
arcPath.clear();
var lengthAfterClear = arcPath.getLength();

arcPath.startNewSubPath(0.0, 0.0);
arcPath.startNewSubPath(1.0, 1.0);
arcPath.addArc([0, 0, 1, 1], -2.4, 2.4);
var lengthAfterRebuild = arcPath.getLength();
```
```json:testMetadata:clear-rebuild-reuse
{
  "testable": true,
  "verifyScript": [
    {"type": "REPL", "expression": "lengthAfterClear", "value": 0},
    {"type": "REPL", "expression": "lengthAfterRebuild > lengthBefore", "value": true}
  ]
}
```

The `clear` + rebuild pattern is preferred over creating `Content.createPath()` inside callbacks because it reuses the existing object allocation. The path variable should be declared at init scope as `const var`, then cleared and rebuilt as needed.
